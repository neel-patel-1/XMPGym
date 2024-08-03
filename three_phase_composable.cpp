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
#include "ippcp.h"

#include "three_phase_harness.h"

IppsAES_GCMState *pState = NULL;


void gen_encrypted_feature(int payload_size, void **p_msgbuf, int *outsize){
  int ippAES_GCM_ctx_size;
  IppStatus status;

  status = ippsAES_GCMGetSize(&ippAES_GCM_ctx_size);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to get AES GCM size\n");
  }

  if(pState != NULL){
    free(pState);
  }
  pState = (IppsAES_GCMState *)malloc(ippAES_GCM_ctx_size);
  int keysize = 16;
  int ivsize = 12;
  int aadSize = 16;
  int taglen = 16;
  Ipp8u *pKey = (Ipp8u *)"0123456789abcdef";
  Ipp8u *pIV = (Ipp8u *)"0123456789ab";
  Ipp8u *pAAD = (Ipp8u *)malloc(aadSize);
  Ipp8u *pSrc = (Ipp8u *)gen_compressible_buf("01234567", payload_size);
  Ipp8u *pDst = (Ipp8u *)malloc(payload_size);
  Ipp8u *pTag = (Ipp8u *)malloc(taglen);

  LOG_PRINT(LOG_VERBOSE, "Plaintext: %s\n", pSrc);

  status = ippsAES_GCMInit(pKey, keysize, pState, ippAES_GCM_ctx_size);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to init AES GCM\n");
  }

  status = ippsAES_GCMStart(pIV, ivsize, pAAD, aadSize, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to start AES GCM\n");
  }

  status = ippsAES_GCMEncrypt(pSrc, pDst, payload_size, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to encrypt AES GCM\n");
  }

  status = ippsAES_GCMGetTag(pTag, taglen, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to get tag AES GCM\n");
  }

  LOG_PRINT(LOG_VERBOSE, "Ciphertext: %s\n", pDst);

  *p_msgbuf = (void *)pDst;
  *outsize = payload_size;

}

static inline void decrypt_feature(void *cipher_inp, void *plain_out, int input_size, int *output_size){
  Ipp8u *pKey = (Ipp8u *)"0123456789abcdef";
  Ipp8u *pIV = (Ipp8u *)"0123456789ab";
  int keysize = 16;
  int ivsize = 12;
  int aadSize = 16;
  Ipp8u aad[aadSize];
  IppStatus status;

  status = ippsAES_GCMReset(pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to reset AES GCM\n");
  }
  status = ippsAES_GCMStart(pIV, ivsize, aad, aadSize, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to start AES GCM\n");
  }
  status = ippsAES_GCMDecrypt((Ipp8u *)cipher_inp, (Ipp8u *)plain_out, input_size, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to decrypt AES GCM: %d\n", status);
  }

  LOG_PRINT(LOG_VERBOSE, "Decrypted: %s\n", (char *)plain_out);
  *(int *)output_size = input_size;
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
  bool do_block = false;
  bool do_gpcore = false;
  bool do_yield = false;

  void *p_msgbuf;
  int outsize;


  while((opt = getopt(argc, argv, "t:i:s:bgy")) != -1){
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
      case 'y':
        do_yield = true;
        break;
      default:
        break;
    }
  }

  initialize_iaa_wq(iaa_dev_id, iaa_wq_id, wq_type);
  initialize_dsa_wq(dsa_dev_id, dsa_wq_id, wq_type);

  if( ! do_gpcore && ! do_block && ! do_yield){
    do_gpcore = true;
    do_block = true;
    do_yield = true;
  }

  if(do_block){
    run_three_phase_offload_timed(
      deser_decomp_hash_blocking_stamped,
      three_func_allocator,
      free_three_phase_stamped_args,
      gen_compressed_serialized_put_request,
      execute_three_phase_blocking_requests_closed_system_request_breakdown,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

    run_three_phase_offload_timed(
      decrypt_memcpy_score_blocking_stamped,
      three_func_allocator,
      free_three_phase_stamped_args,
      gen_encrypted_feature,
      execute_three_phase_blocking_requests_closed_system_request_breakdown,
      itr, total_requests, payload_size, payload_size, final_output_size
    );
  }

  if(do_yield){
    run_three_phase_offload_timed(
      deser_decomp_hash_yielding_stamped,
      three_func_allocator,
      free_three_phase_stamped_args,
      gen_compressed_serialized_put_request,
      execute_three_phase_yielding_requests_closed_system_request_breakdown,
      itr, total_requests, payload_size, payload_size, final_output_size
    );
  }



  free_iaa_wq();
  free_dsa_wq();
  return 0;

}