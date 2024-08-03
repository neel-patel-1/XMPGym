#include "runners.h"

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
  output_validation_fn_t output_validate,
  int iter, int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size
){
  executor_stats_t *stats;

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
      output_validate,
      total_requests, initial_payload_size, max_axfunc_output_size,
      max_post_proc_output_size,
      i
    );
  }
  executor_stats_processor(stats, iter, total_requests);
  executor_stats_free(stats);
}

void run_three_phase_offload_timed(
  fcontext_fn_t request_fn,
  executor_args_allocator_fn_t executor_args_allocator,
  executor_args_free_fn_t executor_args_free,
  executor_stats_allocator_fn_t executor_stats_allocator,
  executor_stats_free_fn_t executor_stats_free,
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

  executor_stats_t *stats;
  executor_args_t *args;

  stats = (executor_stats_t *)malloc(sizeof(executor_stats_t));
  executor_stats_allocator(stats, iter);

  for(int i=0; i<iter; i++){
    three_phase_offload_timed_breakdown(
      request_fn,
      executor_args_allocator,
      executor_args_free,
      offload_args_allocator,
      offload_args_free,
      input_generator,
      three_phase_executor,
      total_requests, initial_payload_size, max_axfunc_output_size,
      max_post_proc_output_size,
      stats->pre_proc_times, stats->offload_tax_times, stats->ax_func_times, stats->post_proc_times, i
    );
  }
  // print_mean_median_stdev(pre_proc_time, iter, "PreProcFunc");
  // print_mean_median_stdev(offload_tax_time, iter, "OffloadTax");
  // print_mean_median_stdev(ax_func_time, iter, "AxFunc");
  // print_mean_median_stdev(post_proc_time, iter, "PostProcFunc");

  print_mean_median_stdev(stats->pre_proc_times, iter, "PreProcFunc");
  print_mean_median_stdev(stats->offload_tax_times, iter, "OffloadTax");
  print_mean_median_stdev(stats->ax_func_times, iter, "AxFunc");
  print_mean_median_stdev(stats->post_proc_times, iter, "PostProcFunc");

  executor_stats_free(stats);
}

uint64_t run_gpcore_request_brkdown(fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char****),
  void (*payload_free)(int,char****),
  int iter, int total_requests)
{
  uint64_t *exetime;
  uint64_t *kernel1, *kernel2;
  uint64_t kernel1_time;
  exetime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  kernel1 = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  kernel2 = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    gpcore_request_breakdown(
      req_fn,
      payload_alloc,
      payload_free,
      total_requests,
      kernel1, kernel2, i
    );
  }
  print_mean_median_stdev(kernel1, iter, "Kernel1");
  print_mean_median_stdev(kernel2, iter, "Kernel2");
  kernel1_time = median_from_array(kernel1, iter);
  return kernel1_time;
}

void run_blocking_offload_request_brkdown(fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char***),
  void (*payload_free)(int,char***),
  int iter, int total_requests)
{
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    blocking_ax_request_breakdown(
      req_fn,
      payload_alloc,
      payload_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "Offload");
  print_mean_median_stdev(waittime, iter, "Wait");
  print_mean_median_stdev(posttime, iter, "Post");
}


void run_yielding_request_brkdown(fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char***),
  void (*payload_free)(int,char***),
  int iter, int total_requests)
{
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    yielding_request_breakdown(
      req_fn,
      payload_alloc,
      payload_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "OffloadSwToFill");
  print_mean_median_stdev(waittime, iter, "YieldToResumeLatency");
  print_mean_median_stdev(posttime, iter, "PostSwToFill");
}

void run_yielding_request_brkdown(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
){
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    yielding_request_breakdown(
      req_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "OffloadSwitchToFill");
  print_mean_median_stdev(waittime, iter, "YieldToResume");
  print_mean_median_stdev(posttime, iter, "PostProcessingSwitchToFill");
}

void run_yielding_best_case_request_brkdown(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
){
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    yielding_best_case_request_breakdown(
      req_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "OffloadSwitchToFill");
  print_mean_median_stdev(waittime, iter, "YieldToResume");
  print_mean_median_stdev(posttime, iter, "PostProcessingSwitchToFill");
}

void run_yielding_interleaved_request_brkdown(
  fcontext_fn_t req_fn,
  fcontext_fn_t inter_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
){
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    yielding_same_interleaved_request_breakdown(
      req_fn,
      inter_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "OffloadSwitchToFill");
  print_mean_median_stdev(waittime, iter, "YieldToResume");
  print_mean_median_stdev(posttime, iter, "PostProcessingSwitchToFill");
}

void run_yielding_multiple_filler_request_brkdown(
  fcontext_fn_t req_fn,
  fcontext_fn_t inter_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
){
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    yielding_multiple_filler_request_breakdown(
      req_fn,
      inter_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "OffloadSwitchToFill");
  print_mean_median_stdev(waittime, iter, "YieldToResume");
  print_mean_median_stdev(posttime, iter, "PostProcessingSwitchToFill");
}

void run_blocking_offload_request_brkdown(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
){
  uint64_t *offloadtime;
  uint64_t *waittime, *posttime;
  offloadtime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  waittime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  posttime = (uint64_t *)malloc(sizeof(uint64_t) * iter);
  for(int i=0; i<iter; i++){
    blocking_ax_request_breakdown(
      req_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests,
      offloadtime, waittime, posttime, i
    );
  }
  print_mean_median_stdev(offloadtime, iter, "Offload");
  print_mean_median_stdev(waittime, iter, "Wait");
  print_mean_median_stdev(posttime, iter, "Post");
}

void run_blocking_offered_load(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)(int, offload_request_args***, ax_comp *comps),
  void (* offload_args_free)(int, offload_request_args***),
  int total_requests,
  int itr){

  // this function executes all requests from start to finish only taking stamps at beginning and end
  uint64_t exetime[itr];
  for(int i = 0; i < itr; i++){
    blocking_request_offered_load(
      request_fn,
      offload_args_allocator,
      offload_args_free,
      total_requests,
      total_requests,
      exetime,
      i
    );
  }

  mean_median_stdev_rps(
    exetime, itr, total_requests, "BlockingOffloadRPS"
  );
}

void run_gpcore_offeredLoad(
  fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char****),
  void (*payload_free)(int,char****),
  int iter, int total_requests){

  uint64_t exetime[iter];
    // this function executes all requests from start to finish only taking stamps at beginning and end
  for(int i = 0; i < iter; i++){
    gpcore_closed_loop_test(
      req_fn,
      payload_alloc,
      payload_free,
      total_requests,
      total_requests,
      exetime,
      i
    );
  }

  mean_median_stdev_rps(
    exetime, iter, total_requests, "GPCoreRPS"
  );
}


void run_yielding_offered_load(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)(int, offload_request_args***, ax_comp *comps),
  void (* offload_args_free)(int, offload_request_args***),
  int num_exe_time_samples_per_run,
  int total_requests, int iter){

  /*
    we take exe time samples periodically as a fixed number of requests complete
    This enables discarding results collected during the latter phase of the test
    where the system has ran out of work to execute during blocking stalls
  */
  int num_requests_before_stamp = total_requests / num_exe_time_samples_per_run;
  int total_exe_time_samples = iter * num_exe_time_samples_per_run;
  uint64_t exetime[total_exe_time_samples];

  /* this function takes multiple samples */
  for(int i = 0; i < iter; i++){
    yielding_request_offered_load(
      request_fn,
      offload_args_allocator,
      offload_args_free,
      num_requests_before_stamp,
      total_requests,
      exetime,
      i * num_exe_time_samples_per_run
    );
  }

  for(int i=0; i<total_exe_time_samples; i++){
    LOG_PRINT(LOG_DEBUG, "ExeTime: %lu\n", exetime[i]);
  }
  mean_median_stdev_rps(
    exetime, total_exe_time_samples, num_requests_before_stamp, "SwitchToFillRPS"
  );
}
