#ifndef RUNNERS_H
#define RUNNERS_H

#include "status.h"
#include "emul_ax.h"
extern "C"{
#include "fcontext.h"
}

#include "test_harness.h"
#include "stats.h"

/* Three Phase Runners */
void run_three_phase_offload_timed(
  fcontext_fn_t request_fn,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  three_phase_executor_fn_t three_phase_executor,
  int iter, int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size
);
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
);


/* Two Phase Runners */
uint64_t run_gpcore_request_brkdown(fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char****),
  void (*payload_free)(int,char****),
  int iter, int total_requests);
void run_gpcore_offeredLoad(
  fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char****),
  void (*payload_free)(int,char****),
  int iter, int total_requests);

void run_blocking_offload_request_brkdown(fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char***),
  void (*payload_free)(int,char***),
  int iter, int total_requests);
void run_blocking_offload_request_brkdown(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
);
void run_blocking_offered_load(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)(int, offload_request_args***, ax_comp *comps),
  void (* offload_args_free)(int, offload_request_args***),
  int total_requests,
  int itr);

void run_yielding_request_brkdown(fcontext_fn_t req_fn,
  void (*payload_alloc)(int,char***),
  void (*payload_free)(int,char***),
  int iter, int total_requests);
void run_yielding_request_brkdown(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
);
void run_yielding_offered_load(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)(int, offload_request_args***, ax_comp *comps),
  void (* offload_args_free)(int, offload_request_args***),
  int num_exe_time_samples_per_run,
  int total_requests, int iter);
void run_yielding_best_case_request_brkdown(
  fcontext_fn_t req_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int iter, int total_requests
);
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
);
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
);


#endif