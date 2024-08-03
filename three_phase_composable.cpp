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

void deser_decomp_hash_yielding(fcontext_transfer_t arg);
void deser_decomp_hash_blocking(fcontext_transfer_t arg);


void alloc_yielding_same_request_deser_decomp_hash_executor_args(executor_args_t **p_args, int idx, int total_requests){
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
  create_contexts(args->off_req_state, total_requests, deser_decomp_hash_yielding);
  allocate_crs(total_requests, &(args->comps));

  *p_args = args;
}

void alloc_blocking_request_deser_decomp_hash_executor_args(executor_args_t **p_args, int idx, int total_requests){
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
  create_contexts(args->off_req_state, total_requests, deser_decomp_hash_blocking);
  allocate_crs(total_requests, &(args->comps));

  *p_args = args;
}

void free_throughput_executor_args(executor_args_t *args){
  free(args->ts0);
  free(args->ts1);
  free(args->ts2);
  free(args->ts3);
  free(args->ts4);
  free_contexts(args->off_req_state, args->total_requests);
  free(args->offload_req_xfer);
  free(args->comps);
  free(args);
}



void run_three_phase_offload(
  executor_args_allocator_fn_t executor_args_allocator,
  executor_args_free_fn_t executor_args_free,
  executor_stats_allocator_fn_t executor_stats_allocator,
  executor_stats_free_fn_t executor_stats_free,
  executor_stats_processor_fn_t executor_stats_processor,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  executor_fn_t three_phase_executor,
  int iter, int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size
){
  executor_stats_t *stats;
  executor_args_t *args;

  stats = (executor_stats_t *)malloc(sizeof(executor_stats_t));
  executor_stats_allocator(stats, iter);

  for(int i=0; i<iter; i++){
    three_phase_harness(
      executor_args_allocator,
      executor_args_free,
      offload_args_allocator,
      offload_args_free,
      input_generator,
      three_phase_executor,
      stats,
      total_requests, initial_payload_size, max_axfunc_output_size,
      max_post_proc_output_size,
      i
    );
  }
  executor_stats_processor(stats, iter, total_requests);
  executor_stats_free(stats);
}

void alloc_throughput_stats(executor_stats_t *stats, int iter){
  stats->exe_time_start = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  stats->exe_time_end = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  stats->iter = iter;
}

void free_throughput_stats(executor_stats_t *stats){
  free(stats->exe_time_start);
  free(stats->exe_time_end);
}

void print_throughput_stats(executor_stats_t *stats, int iter, int total_requests){
  uint64_t exe_times[iter];
  for(int i=0; i<iter; i++){
    exe_times[i] = stats->exe_time_end[i] - stats->exe_time_start[i];
  }
  mean_median_stdev_rps(
    exe_times, iter, total_requests, "RPS"
  );
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




int gLogLevel = LOG_PERF;
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


  while((opt = getopt(argc, argv, "t:i:s:bgyd")) != -1){
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
      case 'b':
        do_block = true;
        break;
      case 'd':
        gLogLevel = LOG_DEBUG;
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

    run_three_phase_offload(
      alloc_blocking_request_deser_decomp_hash_executor_args,
      free_throughput_executor_args,
      alloc_throughput_stats,
      free_throughput_stats,
      print_throughput_stats,
      three_func_allocator,
      free_three_phase_stamped_args,
      gen_compressed_serialized_put_request,
      execute_three_phase_blocking_requests_closed_system_throughput,
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

    run_three_phase_offload(
      alloc_yielding_same_request_deser_decomp_hash_executor_args,
      free_throughput_executor_args,
      alloc_throughput_stats,
      free_throughput_stats,
      print_throughput_stats,
      three_func_allocator,
      free_three_phase_stamped_args,
      gen_compressed_serialized_put_request,
      execute_yielding_three_phase_request_throughput,
      itr, total_requests, payload_size, payload_size, final_output_size
    );

  }



  free_iaa_wq();
  free_dsa_wq();
  return 0;

}