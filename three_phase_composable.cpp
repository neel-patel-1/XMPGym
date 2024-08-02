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
#include "submit.hpp"
#include "wait.h"

#include "proto_files/router.pb.h"
#include "ch3_hash.h"

void execute_three_phase_blocking_requests_closed_system_request_breakdown(
  int total_requests,
  timed_offload_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *pre_proc_times, uint64_t *offload_tax_times,
  uint64_t *ax_func_times, uint64_t *post_proc_times, int idx)
    /* pass in the times we measure and idx to populate */
{
  int next_unstarted_req_idx = 0;

  while(requests_completed < total_requests){
    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;
  }

  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t *ts4 = off_args[0]->ts4;
  uint64_t avg, diff[total_requests];

  avg_samples_from_arrays(diff, pre_proc_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "PreProcTime: %lu\n", pre_proc_times[idx]);

  avg_samples_from_arrays(diff, offload_tax_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "OffloadTaxTime: %lu\n", offload_tax_times[idx]);

  avg_samples_from_arrays(diff, ax_func_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "AxFuncTime: %lu\n", ax_func_times[idx]);

  avg_samples_from_arrays(diff, post_proc_times[idx], ts4, ts3, requests_completed);
  LOG_PRINT( LOG_DEBUG, "PostProcTime: %lu\n", post_proc_times[idx]);
}

void blocking_three_phase_breakdown(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)
    (int, int, int, int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3, uint64_t *ts4),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size,
  uint64_t *pre_proc_time, uint64_t *offload_tax_time,
  uint64_t *ax_func_time, uint64_t *post_proc_time, int idx
){
  using namespace std;
  fcontext_state_t *self = fcontext_create_proxy();
  char**dst_bufs;
  ax_comp *comps;
  timed_offload_request_args **off_args;
  fcontext_transfer_t *offload_req_xfer;
  fcontext_state_t **off_req_state;

  int sampling_intervals = 1;
  int sampling_interval_timestamps = sampling_intervals + 1;
  uint64_t sampling_interval_completion_times[sampling_interval_timestamps];
  uint64_t *ts0, *ts1, *ts2, *ts3, *ts4;

  ts0 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  ts1 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  ts2 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  ts3 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);
  ts4 = (uint64_t *)malloc(sizeof(uint64_t) * total_requests);

  requests_completed = 0;

  /* pre-allocate the payloads */
  allocate_crs(total_requests, &comps);

  offload_args_allocator(total_requests, initial_payload_size,
    max_axfunc_output_size, max_post_proc_output_size, &off_args, comps,
    ts0, ts1, ts2, ts3, ts4);

  /* Pre-create the contexts */
  off_req_state = (fcontext_state_t **)malloc(sizeof(fcontext_state_t *) * total_requests);

  create_contexts(off_req_state, total_requests, request_fn);

  execute_three_phase_blocking_requests_closed_system_request_breakdown(
    total_requests, off_args,
    off_req_state, pre_proc_time,
    offload_tax_time, ax_func_time,
    post_proc_time, idx);

  /* teardown */
  free_contexts(off_req_state, total_requests);
  free(comps);
  offload_args_free(total_requests, &off_args);
  free(ts0);
  free(ts1);
  free(ts2);
  free(ts3);
  free(ts4);

  fcontext_destroy(self);
}

void run_blocking_offload_request_brkdown_three_phase(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, int, int, int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3, uint64_t *ts4),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size
){
  uint64_t *pre_proc_time, *offload_tax_time, *ax_func_time, *post_proc_time;
  pre_proc_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  offload_tax_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  ax_func_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  post_proc_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);

  for(int i=0; i<iter; i++){
    blocking_three_phase_breakdown(
      req_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests, initial_payload_size, max_axfunc_output_size,
      max_post_proc_output_size, pre_proc_time, offload_tax_time, ax_func_time, post_proc_time, i
    );
  }
  print_mean_median_stdev(pre_proc_time, iter, "PreProcFunc");
  print_mean_median_stdev(offload_tax_time, iter, "OffloadTax");
  print_mean_median_stdev(ax_func_time, iter, "AxFunc");
  print_mean_median_stdev(post_proc_time, iter, "PostProcFunc");
}

uLong get_compress_bound(int payload_size){
  z_stream stream;
  int ret = 0;
  uLong maxcompsize;

  memset(&stream, 0, sizeof(z_stream));
  ret = deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, -12, 9, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) {
    LOG_PRINT( LOG_ERR, "Error deflateInit2 status %d\n", ret);
    return 0;
  }

  maxcompsize = deflateBound(&stream, payload_size);
  deflateEnd(&stream);
  return maxcompsize;
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
  LOG_PRINT(LOG_DEBUG, "ValString: %s Size: %d\n", valbuf, payload_size);

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
    LOG_PRINT(LOG_DEBUG, "Failed to serialize\n");
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

/*
  no pre_proc_funcs expand payload, for now pre_proc_output_size <= pre_proc_input_size
  - compression upper bound is initial_payload_size

*/
void three_phase_stamped_allocator(
  int total_requests,
  int initial_payload_size,
  int max_axfunc_output_size,
  int max_post_proc_output_size,
  timed_offload_request_args ***offload_args,
  ax_comp *comps, uint64_t *ts0,
  uint64_t *ts1, uint64_t *ts2,
  uint64_t *ts3, uint64_t *ts4
)
{
  timed_offload_request_args **off_args =
    (timed_offload_request_args **)malloc(total_requests * sizeof(timed_offload_request_args *));

  int max_pre_proc_output_size;
  int expected_ax_output_size = initial_payload_size;

  for(int i = 0; i < total_requests; i++){
    off_args[i] = (timed_offload_request_args *)malloc(sizeof(timed_offload_request_args));

    gen_compressed_serialized_put_request(initial_payload_size,
      &off_args[i]->pre_proc_input, &off_args[i]->pre_proc_input_size);
    max_pre_proc_output_size = get_compress_bound(initial_payload_size);
    off_args[i]->pre_proc_output = (void *)malloc(max_pre_proc_output_size);

    off_args[i]->ax_func_output = (void *)malloc(max_axfunc_output_size);
    off_args[i]->max_axfunc_output_size = max_axfunc_output_size;

    off_args[i]->post_proc_output = (void *)malloc(max_post_proc_output_size);
    off_args[i]->max_post_proc_output_size = max_post_proc_output_size;
    off_args[i]->post_proc_input_size = expected_ax_output_size;

    off_args[i]->comp = &comps[i];

    off_args[i]->ts0 = ts0;
    off_args[i]->ts1 = ts1;
    off_args[i]->ts2 = ts2;
    off_args[i]->ts3 = ts3;
    off_args[i]->ts4 = ts4;
    off_args[i]->id = i;
    off_args[i]->desc = (struct hw_desc *)malloc(sizeof(struct hw_desc));

  }
  *offload_args = off_args;
}

void free_three_phase_stamped_args(
  int total_requests,
  timed_offload_request_args ***off_args
){
  for(int i = 0; i < total_requests; i++){
    free((*off_args)[i]->pre_proc_input);
    free((*off_args)[i]->pre_proc_output);
    free((*off_args)[i]->ax_func_output);
    free((*off_args)[i]->post_proc_output);
    free((*off_args)[i]->desc);
    free((*off_args)[i]);
  }
  free(*off_args);
}

static inline void complete_request_and_switch_to_scheduler(fcontext_transfer_t arg){
  requests_completed++;
  fcontext_swap(arg.prev_context, NULL);
}

// we have the option to use either func pointers in the args or inline by
void blocking_offload_three_phase_stamped(fcontext_transfer_t arg){
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

  generic_three_phase_timed(
    NULL,
    deser_from_buf, pre_proc_input, pre_proc_output, pre_proc_input_size,
    prepare_iaa_decompress_desc_with_preallocated_comp, blocking_iaa_submit, spin_on,
    comp, desc, iaa,
    ax_func_output, max_axfunc_output_size,
    hash_buf, post_proc_output, post_proc_input_size, max_post_proc_output_size,
    ts0, ts1, ts2, ts3, ts4, id
  );

  complete_request_and_switch_to_scheduler(arg);
}

int gLogLevel = LOG_VERBOSE;
bool gDebugParam = false;
int main(int argc, char **argv){


  int wq_id = 0;
  int dev_id = 1;
  int wq_type = SHARED;



  int payload_size = 1024, deserd_size;
  void *serd_buf, *deserd_buf, *decompbuf, *hashbuf;
  int ser_size;

  initialize_iaa_wq(dev_id, wq_id, wq_type);

  int opt;
  int itr = 100;
  int total_requests = 1000;

  while((opt = getopt(argc, argv, "y:s:j:t:i:r:s:q:d:hf")) != -1){
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
      default:
        break;
    }
  }


  run_blocking_offload_request_brkdown_three_phase(
    blocking_offload_three_phase_stamped,
    three_phase_stamped_allocator,
    free_three_phase_stamped_args,
    itr, total_requests, payload_size, payload_size, 4
  );

  free_iaa_wq();
  return 0;

}