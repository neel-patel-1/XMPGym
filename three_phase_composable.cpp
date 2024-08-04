#include "test_harness.h"
#include "print_utils.h"
#include "offload_args.h"
#include "gpcore_args.h"
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
#include "dsa_offloads.h"
#include "submit.hpp"

#include "proto_files/router.pb.h"
#include "ch3_hash.h"
#include "wait.h"

#include "decrypt.h"

#include "three_phase_components.h"

#include "request_executors.h"

#include <functional>
#include "gather_scatter.h"

int *glob_indir_arr = NULL;
int num_accesses = 10;
int input_size = 16384;

void alloc_homogenous_request_executor_args_throughput(executor_args_t **p_args, int idx, int total_requests, fcontext_fn_t request_function){
  executor_args_t *args;
  args = (executor_args_t *)malloc(sizeof(executor_args_t));
  args->total_requests = total_requests;
  args->idx = idx;

  args->off_req_state = (fcontext_state_t **)malloc(sizeof(fcontext_state_t *) * total_requests);
  args->offload_req_xfer = (fcontext_transfer_t *)malloc(sizeof(fcontext_transfer_t) * total_requests);
  create_contexts(args->off_req_state, total_requests, request_function);
  allocate_crs(total_requests, &(args->comps));

  *p_args = args;

}

void free_homogoenous_request_executor_args_throughput(executor_args_t *args){
  free_contexts(args->off_req_state, args->total_requests);
  free(args->offload_req_xfer);
  free(args->off_req_state);
  free(args->comps);
  free(args);
}

void free_homogoenous_request_executor_args_breakdown(executor_args_t *args){
  LOG_PRINT(LOG_DEBUG, "Freeing Executor Args\n");

  free(args->ts0);
  free(args->ts1);
  free(args->ts2);
  free(args->ts3);
  free(args->ts4);

  free(args->comps);
  free_contexts(args->off_req_state, args->total_requests);
  free(args->offload_req_xfer);
  free(args->off_req_state);

  free(args);

}

void alloc_homogenous_request_executor_args_breakdown(executor_args_t **p_args, int idx, int total_requests, fcontext_fn_t request_function){
  LOG_PRINT(LOG_DEBUG, "Allocating Executor Args\n");
  executor_args_t *args;
  args = (executor_args_t *)malloc(sizeof(executor_args_t));
  args->total_requests = total_requests;
  args->idx = idx;

  args->ts0 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  args->ts1 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  args->ts2 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  args->ts3 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  args->ts4 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);

  args->off_req_state = (fcontext_state_t **)malloc(sizeof(fcontext_state_t *) * total_requests);
  args->offload_req_xfer = (fcontext_transfer_t *)malloc(sizeof(fcontext_transfer_t) * total_requests);
  create_contexts(args->off_req_state, total_requests, request_function);
  allocate_crs(total_requests, &(args->comps));

  *p_args = args;

}

void alloc_breakdown_stats(executor_stats_t *stats, int iter){
  stats->iter = iter;
  stats->pre_proc_times = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  stats->offload_tax_times = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  stats->ax_func_times = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  stats->post_proc_times = (uint64_t *)malloc(sizeof(uint64_t) * iter);
}

void free_breakdown_stats(executor_stats_t *stats){
  free(stats->pre_proc_times);
  free(stats->offload_tax_times);
  free(stats->ax_func_times);
  free(stats->post_proc_times);
}

static inline void dot_product(void *feature, void *plain_out, int input_size, int *output_size){
  float sum = 0;
  float doc[8] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};
  float *v2 = (float *)feature;
  int mult_ops = input_size / sizeof(float);

  for(int i=0; i < mult_ops; i++){
    sum += v2[i] * doc[i % 8];
  }
  LOG_PRINT(LOG_DEBUG, "Dot Product: %f\n", sum);

}

void gen_compressed_serialized_put_request(int payload_size, void **p_msgbuf, int *outsize){
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
  LOG_PRINT(LOG_VERBOSE, "ValString: %s Size: %d\n", valbuf, payload_size);

  /* get compress bound*/
  maxcompsize = get_compress_bound(payload_size);
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
    LOG_PRINT(LOG_ERR, "Failed to serialize\n");
  }

  *p_msgbuf = (void *)msgbuf;
  *outsize = (int)msgsize;
}

void gen_compressed_request(int payload_size, void **p_msgbuf, int *outsize){
  const char * pattern = "01234567";
  uint8_t *compbuf;
  char *valbuf;
  uint64_t msgsize;
  int maxcompsize;

  valbuf = gen_compressible_buf(pattern, payload_size);
  LOG_PRINT(LOG_VERBOSE, "ValString: %s Size: %d\n", valbuf, payload_size);

  /* get compress bound*/
  maxcompsize = get_compress_bound(payload_size);
  compbuf = (uint8_t *)malloc(maxcompsize);
  gpcore_do_compress(compbuf, (void *)valbuf, payload_size, &maxcompsize);

  *p_msgbuf = (void *)compbuf;
  *outsize = (int)maxcompsize;
}

static inline void null_fn(void *inp, void *output, int input_size, int *output_size){
  *output_size = input_size;
}

static inline void gather_access(void *inp, void *output, int input_size, int *output_size){
  /* gather from input */
  float *outp = (float *)output;
  float *input = (float *)inp;
  for(int i=0; i < num_accesses; i++){
    outp[i] = input[glob_indir_arr[i]];
    LOG_PRINT(LOG_DEBUG, "outp[%d] = inp[%d] = %f = %f\n", i,
      glob_indir_arr[i], outp[i] , input[glob_indir_arr[i]]);
  }
  *output_size = input_size;
}

void decomp_gather_gpcore_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;
  int id = args->id;

  generic_gpcore_three_phase_timed(
    NULL, arg,
    null_fn, pre_proc_input, pre_proc_output, pre_proc_input_size,
    gpcore_do_deflate_decompress, ax_func_output, max_axfunc_output_size,
    gather_access, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void decomp_gather_gpcore(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  generic_gpcore_three_phase(
    NULL, arg,
    null_fn, pre_proc_input, pre_proc_output, pre_proc_input_size,
    gpcore_do_deflate_decompress, ax_func_output, max_axfunc_output_size,
    gather_access, post_proc_output, post_proc_input_size, max_post_proc_output_size
  );

  complete_request_and_switch_to_scheduler(arg);
}

void decomp_gather_blocking_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;
  int id = args->id;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;

  generic_blocking_three_phase_timed(
    NULL, arg,
    null_fn, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit, spin_on,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    gather_access, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void decomp_gather_blocking(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;

  generic_blocking_three_phase(
    NULL, arg,
    null_fn, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit, spin_on,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    gather_access, post_proc_output, post_proc_input_size, max_post_proc_output_size
  );

  complete_request_and_switch_to_scheduler(arg);
}

void decomp_gather_yielding_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;
  int id = args->id;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;

  generic_yielding_three_phase_timed(
    NULL, arg,
    null_fn, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    gather_access, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void decomp_gather_yielding(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;

  generic_yielding_three_phase(
    NULL, arg,
    null_fn, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    gather_access, post_proc_output, post_proc_input_size, max_post_proc_output_size
  );

  complete_request_and_switch_to_scheduler(arg);
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


void decrypt_memcpy_score_blocking_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;
  int id = args->id;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;

  generic_blocking_three_phase_timed(
    NULL, arg,
    decrypt_feature, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_dsa_memcpy_desc_with_preallocated_comp, blocking_dsa_submit, spin_on,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    dot_product, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void deser_decomp_hash_blocking_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;
  int id = args->id;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;

  generic_blocking_three_phase_timed(
    NULL, arg,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit, spin_on,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void deser_decomp_hash_yielding_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;
  int id = args->id;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;

  generic_yielding_three_phase_timed(
    NULL, arg,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void deser_decomp_hash_yielding(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;

  generic_yielding_three_phase(
    NULL, arg,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size
  );

  complete_request_and_switch_to_scheduler(arg);
}

void deser_decomp_hash_blocking(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  ax_comp *comp = args->comp;
  struct hw_desc *desc = args->desc;

  generic_blocking_three_phase(
    NULL, arg,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit, spin_on,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size
  );

  complete_request_and_switch_to_scheduler(arg);
}

void deser_decomp_hash_gpcore_stamped(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  uint64_t *ts0 = args->ts0;
  uint64_t *ts1 = args->ts1;
  uint64_t *ts2 = args->ts2;
  uint64_t *ts3 = args->ts3;
  uint64_t *ts4 = args->ts4;
  int id = args->id;

  generic_gpcore_three_phase_timed(
    NULL, arg,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    gpcore_do_deflate_decompress, ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

void deser_decomp_hash_gpcore(fcontext_transfer_t arg){
  timed_offload_request_args *args = (timed_offload_request_args *)arg.data;

  void *pre_proc_input = args->pre_proc_input;
  void *pre_proc_output = args->pre_proc_output;
  int pre_proc_input_size = args->pre_proc_input_size;

  void *ax_func_output = args->ax_func_output;
  int max_axfunc_output_size = args->max_axfunc_output_size;

  void *post_proc_output = args->post_proc_output;
  int post_proc_input_size = args->post_proc_input_size;
  int max_post_proc_output_size = args->max_post_proc_output_size;

  generic_gpcore_three_phase(
    NULL, arg,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    gpcore_do_deflate_decompress, ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size
  );

  complete_request_and_switch_to_scheduler(arg);
}


int gLogLevel = LOG_DEBUG;
bool gDebugParam = false;
int main(int argc, char **argv){


  int iaa_wq_id = 0;
  int dsa_wq_id = 0;
  int iaa_dev_id = 1;
  int dsa_dev_id = 0;
  int wq_type = SHARED;

  int opt;
  int itr = 100;
  int total_requests = 1000;
  int payload_size = 1024;
  int final_output_size = sizeof(uint32_t);

  void *p_msgbuf;
  int outsize;


  while((opt = getopt(argc, argv, "t:i:s:bgydm:n:")) != -1){
    switch(opt){
      case 't':
        total_requests = atoi(optarg);
        break;
      case 'i':
        itr = atoi(optarg);
        break;
      case 's':
        payload_size = atoi(optarg);
        break;
      case 'd':
        gLogLevel = LOG_DEBUG;
        break;
      case 'm':
        dsa_dev_id = atoi(optarg);
        break;
      case 'n':
        iaa_dev_id = atoi(optarg);
        break;
      default:
        break;
    }
  }

  initialize_iaa_wq(iaa_dev_id, iaa_wq_id, wq_type);
  initialize_dsa_wq(dsa_dev_id, dsa_wq_id, wq_type);



  input_generator_fn_t input_gen = gen_compressed_serialized_put_request;
  fcontext_fn_t gpcore_breakdown_fn = deser_decomp_hash_gpcore_stamped;
  fcontext_fn_t gpcore_throughput_fn = deser_decomp_hash_gpcore;
  fcontext_fn_t blocking_breakdown_fn = deser_decomp_hash_blocking_stamped;
  fcontext_fn_t blocking_throughput_fn = deser_decomp_hash_blocking;
  fcontext_fn_t yielding_breakdown_fn = deser_decomp_hash_yielding_stamped;
  fcontext_fn_t yielding_throughput_fn = deser_decomp_hash_yielding;
  offload_args_allocator_fn_t allocator_fn = three_func_allocator;
  offload_args_free_fn_t stamped_offload_args_free_fn = free_three_phase_stamped_args;

  typedef enum _app_type_t {
    DESER,
    DECRYPT,
    GATHER
  } app_type_t;
  app_type_t app_type = DESER;
  switch(app_type){
    case DESER:
      input_gen = gen_compressed_serialized_put_request;
      gpcore_breakdown_fn = deser_decomp_hash_gpcore_stamped;
      gpcore_throughput_fn = deser_decomp_hash_gpcore;
      blocking_breakdown_fn = deser_decomp_hash_blocking_stamped;
      blocking_throughput_fn = deser_decomp_hash_blocking;
      yielding_breakdown_fn = deser_decomp_hash_yielding_stamped;
      yielding_throughput_fn = deser_decomp_hash_yielding;
      allocator_fn = three_func_allocator;
      stamped_offload_args_free_fn = free_three_phase_stamped_args;
      break;
    case GATHER:
      input_size = payload_size;
      LOG_PRINT(LOG_DEBUG, "Allocating Global Indir Array\n");
      indirect_array_gen(&glob_indir_arr);

      input_gen = gen_compressed_request;

      gpcore_breakdown_fn = decomp_gather_gpcore_stamped;
      gpcore_throughput_fn = decomp_gather_gpcore;
      blocking_breakdown_fn = decomp_gather_blocking_stamped;
      blocking_throughput_fn = decomp_gather_blocking;
      yielding_breakdown_fn = decomp_gather_yielding_stamped;
      yielding_throughput_fn = decomp_gather_yielding;

      allocator_fn = null_two_func_allocator;
      stamped_offload_args_free_fn = free_null_two_phase;

      break;
    default:
      break;
  }

  executor_args_allocator_fn_t breakdown_exe_alloc_gp_core =
    std::bind(alloc_homogenous_request_executor_args_breakdown,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, gpcore_breakdown_fn);

  executor_args_allocator_fn_t breakdown_exe_alloc_blocking =
    std::bind(alloc_homogenous_request_executor_args_breakdown,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, blocking_breakdown_fn);

  executor_args_allocator_fn_t breakdown_exe_alloc_yielding =
    std::bind(alloc_homogenous_request_executor_args_breakdown,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, yielding_breakdown_fn);

  executor_args_allocator_fn_t throughput_exe_alloc_gp_core =
    std::bind(alloc_homogenous_request_executor_args_throughput,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, gpcore_throughput_fn);

  executor_args_allocator_fn_t throughput_exe_alloc_blocking =
    std::bind(alloc_homogenous_request_executor_args_throughput,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, blocking_throughput_fn);

  executor_args_allocator_fn_t throughput_exe_alloc_yielding =
    std::bind(alloc_homogenous_request_executor_args_throughput,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, yielding_throughput_fn);

  executor_args_free_fn_t breakdown_exe_free = free_homogoenous_request_executor_args_breakdown;

  executor_args_free_fn_t throughput_exe_free = free_homogoenous_request_executor_args_throughput;

    run_three_phase_offload_timed(
      breakdown_exe_alloc_gp_core,
      breakdown_exe_free,
      alloc_breakdown_stats,
      free_breakdown_stats,
      print_three_phase_breakdown_stats,
      allocator_fn,
      stamped_offload_args_free_fn,
      input_gen,
      execute_three_phase_blocking_requests_closed_system_request_breakdown,
      itr, total_requests, payload_size, payload_size, final_output_size
    );
    run_three_phase_offload_timed(
      throughput_exe_alloc_gp_core,
      throughput_exe_free,
      alloc_throughput_stats,
      free_throughput_stats,
      print_throughput_stats,
      allocator_fn,
      stamped_offload_args_free_fn,
      input_gen,
      execute_three_phase_blocking_requests_closed_system_throughput,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

    run_three_phase_offload_timed(
      breakdown_exe_alloc_blocking,
      breakdown_exe_free,
      alloc_breakdown_stats,
      free_breakdown_stats,
      print_three_phase_breakdown_stats,
      allocator_fn,
      stamped_offload_args_free_fn,
      input_gen,
      execute_three_phase_blocking_requests_closed_system_request_breakdown,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

    run_three_phase_offload_timed(
      throughput_exe_alloc_blocking,
      throughput_exe_free,
      alloc_throughput_stats,
      free_throughput_stats,
      print_throughput_stats,
      allocator_fn,
      stamped_offload_args_free_fn,
      input_gen,
      execute_three_phase_blocking_requests_closed_system_throughput,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

    run_three_phase_offload_timed(
      breakdown_exe_alloc_yielding,
      breakdown_exe_free,
      alloc_breakdown_stats,
      free_breakdown_stats,
      print_three_phase_breakdown_stats,
      allocator_fn,
      stamped_offload_args_free_fn,
      input_gen,
      execute_three_phase_yielding_requests_closed_system_request_breakdown,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

    run_three_phase_offload_timed(
      throughput_exe_alloc_yielding,
      throughput_exe_free,
      alloc_throughput_stats,
      free_throughput_stats,
      print_throughput_stats,
      allocator_fn,
      stamped_offload_args_free_fn,
      input_gen,
      execute_yielding_three_phase_request_throughput,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

  free_iaa_wq();
  free_dsa_wq();
  return 0;

}