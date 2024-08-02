#include "test_harness.h"
#include "print_utils.h"
#include "router_request_args.h"
#include "iaa_offloads.h"
#include "runners.h"
#include "gpcore_compress.h"

extern "C" {
  #include "fcontext.h"
  #include "iaa.h"
  #include "accel_test.h"
  #include "iaa_compress.h"
  #include <zlib.h>
}
#include "decompress_and_hash_request.hpp"
#include "iaa_offloads.h"
#include "submit.hpp"
#include "wait.h"

#include "proto_files/router.pb.h"
#include "ch3_hash.h"

void gen_compressed_serialized_put_request(int payload_size, void **p_msgbuf, uint64_t *outsize){
  router::RouterRequest req;
  const char * pattern = "01234567";
  uint8_t *msgbuf, *compbuf;
  char *valbuf;
  uint64_t msgsize;
  int compsize, maxcompsize;
  bool rc = false;

  int ret = 0;
  z_stream stream;
  int avail_out;

  valbuf = gen_compressible_buf(pattern, payload_size);
  LOG_PRINT(LOG_DEBUG, "ValString: %s Size: %d\n", valbuf, payload_size);

  /* get compress bound*/
  memset(&stream, 0, sizeof(z_stream));
  ret = deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -12, 9, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) {
    LOG_PRINT( LOG_ERR, "Error deflateInit2 status %d\n", ret);
    return;
  }

  maxcompsize = deflateBound(&stream, payload_size);
  compbuf = (uint8_t *)malloc(maxcompsize);
  gpcore_do_compress(compbuf, (void *)valbuf, payload_size, &maxcompsize);

  std::string compstring((char *)compbuf, maxcompsize);

  req.set_key("/region/cluster/foo:key|#|etc"); // key is 32B string, value gets bigger up to 2MB
  req.set_value(compstring);
  req.set_operation(0);

  msgsize = req.ByteSizeLong();
  msgbuf = (uint8_t *)malloc(msgsize);
  rc = req.SerializeToArray((void *)msgbuf, msgsize);
  if(rc == false){
    LOG_PRINT(LOG_DEBUG, "Failed to serialize\n");
  }

  *p_msgbuf = (void *)msgbuf;
  *outsize = msgsize;
}

void deser_from_buf(void *ser_inp, void *output, int input_size, int *output_size){
  router::RouterRequest req;
  req.ParseFromArray(ser_inp, input_size);
  LOG_PRINT(LOG_DEBUG, "Deserialized Payload Size: %ld\n", req.value().size());

  memcpy(output, req.value().c_str(), req.value().size());
  *output_size = req.value().size();
}

void hash_buf(void *inp, void *output, int input_size, int *output_size){
  uint32_t hash = 0;
  hash = furc_hash((const char *)inp, input_size, 16);
  *(uint32_t *)output = hash;
  *output_size = sizeof(uint32_t);
}

template <typename pre_proc_fn,
  typename prep_desc_fn, typename submit_desc_fn, typename post_offload_fn,
  typename desc_t, typename comp_record_t, typename ax_handle_t,
  typename post_proc_fn,
  typename preempt_signal_t>
static inline void generic_three_phase_timed(
  preempt_signal_t sig,
  pre_proc_fn pre_proc_func, void *pre_proc_input, void *pre_proc_output, int pre_proc_input_size,
  prep_desc_fn prep_func, submit_desc_fn submit_func, post_offload_fn post_offload_func,
  comp_record_t *comp, desc_t *desc, ax_handle_t *ax,
  void *ax_func_output, int max_axfunc_output_size,
  post_proc_fn post_proc_func, void *post_proc_output, int post_proc_input_size, int max_post_proc_output_size,
  uint64_t *ts0, uint64_t *ts1, uint64_t *ts2, uint64_t *ts3, uint64_t *ts4, int idx
  )
{
  int preproc_output_size, ax_input_size;
  void *ax_func_input;
  void *post_proc_input;

  ts0[idx] = sampleCoderdtsc();
  pre_proc_func(pre_proc_input, pre_proc_output, pre_proc_input_size, &preproc_output_size);
  LOG_PRINT(LOG_DEBUG, "PreProcOutputSize: %d\n", preproc_output_size);

  ax_input_size = preproc_output_size;
  ax_func_input = pre_proc_output;

  ts1[idx] = sampleCoderdtsc();
  prep_func(desc, (uint64_t)ax_func_input, (uint64_t)ax_func_output, (uint64_t)comp, ax_input_size);
  submit_func(ax, desc);

  ts2[idx] = sampleCoderdtsc();
  post_offload_func(comp);
  LOG_PRINT(LOG_DEBUG, "AXFuncOutput: %s \n", (char *)ax_func_output);

  ts3[idx] = sampleCoderdtsc();
  post_proc_input = ax_func_output;
  post_proc_func(post_proc_input, post_proc_output, post_proc_input_size, &max_post_proc_output_size);
  LOG_PRINT(LOG_DEBUG, "PostProcOutputSize: %d\n", max_post_proc_output_size);
  ts4[idx] = sampleCoderdtsc();

  return;
}

int gLogLevel = LOG_VERBOSE;
bool gDebugParam = false;
int main(int argc, char **argv){


  int wq_id = 0;
  int dev_id = 1;
  int wq_type = SHARED;



  int payload_size = 1024, deserd_size;
  void *serd_buf, *deserd_buf, *decompbuf, *hashbuf;
  uint64_t ser_size;

  initialize_iaa_wq(dev_id, wq_id, wq_type);
  gen_compressed_serialized_put_request(payload_size, &serd_buf, &ser_size);

  deserd_buf = malloc(payload_size); // oversized, only needs to be sized for compressed payload, byt presumably could be undersized for uncompressible data
  decompbuf = malloc(payload_size);
  hashbuf = malloc(sizeof(uint32_t));

  uint64_t ts0, ts1, ts2, ts3, ts4;

  struct hw_desc desc;
  ax_comp *comp, sig;
  comp = (ax_comp *)aligned_alloc(iaa->compl_size, sizeof(ax_comp));

  generic_three_phase_timed(
    sig,
    deser_from_buf, serd_buf, deserd_buf, ser_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit, spin_on,
    comp, &desc, iaa,
    decompbuf, payload_size,
    hash_buf, hashbuf, payload_size, sizeof(uint32_t),
    &ts0, &ts1, &ts2, &ts3, &ts4, 0
  );

  free_iaa_wq();
  return 0;

}