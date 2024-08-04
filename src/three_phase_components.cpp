#include "three_phase_components.h"

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
      &(off_args[i]->pre_proc_input), &(off_args[i]->pre_proc_input_size));
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

void null_two_func_allocator(
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

    /* ax func input needs to be the input that would
    ordinarily be processed by pre_proc_func. ax func consumes pre_proc_output
    so put the input directly in there -- using this allocator means
    the first func is null_func */
    input_generator(initial_payload_size,
      &(off_args[i]->pre_proc_output), &(off_args[i]->pre_proc_input_size));
    /* null function still needs to set pre_proc_output size to the input */
    max_pre_proc_output_size = get_compress_bound(initial_payload_size);
    off_args[i]->pre_proc_input = (void *)malloc(initial_payload_size); // should not be required but someone tries to free it

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

void free_null_two_phase(
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

void print_three_phase_breakdown_stats(executor_stats_t *stats, int iter, int total_requests){
  print_mean_median_stdev(stats->pre_proc_times, iter, "PreProcFunc");
  print_mean_median_stdev(stats->offload_tax_times, iter, "OffloadTax");
  print_mean_median_stdev(stats->ax_func_times, iter, "AxFunc");
  print_mean_median_stdev(stats->post_proc_times, iter, "PostProcFunc");
}

