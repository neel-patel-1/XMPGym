#include "three_phase_harness.h"

void execute_three_phase_blocking_requests_closed_system_request_breakdown(
  int total_requests,
  timed_offload_request_args **off_args,
  fcontext_state_t **off_req_state,
  fcontext_transfer_t *offload_req_xfer,
  ax_comp *comps,
  uint64_t *pre_proc_times,
  uint64_t *offload_tax_times,
  uint64_t *ax_func_times,
  uint64_t *post_proc_times,
  int idx
)
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

void execute_three_phase_yielding_requests_closed_system_request_breakdown(
  int total_requests,
  timed_offload_request_args **off_args,
  fcontext_state_t **off_req_state,
  fcontext_transfer_t *offload_req_xfer,
  ax_comp *comps,
  uint64_t *pre_proc_times,
  uint64_t *offload_tax_times,
  uint64_t *ax_func_times,
  uint64_t *post_proc_times,
  int idx
)
{
  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;

  while(requests_completed < total_requests){
    if(comps[next_request_offload_to_complete_idx].status == COMP_STATUS_COMPLETED){
      fcontext_swap(offload_req_xfer[next_request_offload_to_complete_idx].prev_context, NULL);
      next_request_offload_to_complete_idx++;
    } else if(next_unstarted_req_idx < total_requests){
      offload_req_xfer[next_unstarted_req_idx] =
        fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
      next_unstarted_req_idx++;
    }
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


void three_phase_offload_timed_breakdown(
  fcontext_fn_t request_fn,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  three_phase_executor_fn_t three_phase_executor,
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
    max_axfunc_output_size, max_post_proc_output_size, input_generator, &off_args, comps,
    ts0, ts1, ts2, ts3, ts4);

  /* Pre-create the contexts */
  off_req_state = (fcontext_state_t **)malloc(sizeof(fcontext_state_t *) * total_requests);
  offload_req_xfer = (fcontext_transfer_t *)malloc(sizeof(fcontext_transfer_t) * total_requests);


  create_contexts(off_req_state, total_requests, request_fn);

  three_phase_executor(
    total_requests, off_args,
    off_req_state, offload_req_xfer, comps, pre_proc_time,
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



void run_three_phase_offload_timed(
  fcontext_fn_t request_fn,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  three_phase_executor_fn_t three_phase_executor,
  int iter, int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size
){
  uint64_t *pre_proc_time, *offload_tax_time, *ax_func_time, *post_proc_time;
  pre_proc_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  offload_tax_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  ax_func_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  post_proc_time = (uint64_t *)malloc(sizeof(uint64_t) * iter);

  for(int i=0; i<iter; i++){
    three_phase_offload_timed_breakdown(
      request_fn,
      offload_args_allocator,
      offload_args_free,
      input_generator,
      three_phase_executor,
      total_requests, initial_payload_size, max_axfunc_output_size,
      max_post_proc_output_size,
      pre_proc_time, offload_tax_time, ax_func_time, post_proc_time, i
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

void three_func_allocator(
  int total_requests,
  int initial_payload_size,
  int max_axfunc_output_size,
  int max_post_proc_output_size,
  void input_generator(int payload_size, void **p_msgbuf, int *outsize),
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

    input_generator(initial_payload_size,
      &off_args[i]->pre_proc_input, &off_args[i]->pre_proc_input_size);
    max_pre_proc_output_size = get_compress_bound(initial_payload_size);
    off_args[i]->pre_proc_output = (void *)malloc(max_pre_proc_output_size);

    off_args[i]->ax_func_output = (void *)malloc(max_axfunc_output_size);
    /*write prefault */
    for(int j = 0; j < max_axfunc_output_size; j+=4096){
      ((char *)off_args[i]->ax_func_output)[j] = 0;
    }
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
