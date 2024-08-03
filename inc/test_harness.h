#ifndef TEST_HARNESS
#define TEST_HARNESS

extern "C" {
  #include "fcontext.h"
}

#include "emul_ax.h"
#include "offload.h"
#include "context_management.h"


typedef void (*offload_args_allocator_fn_t)(
  int total_requests,
  int initial_payload_size,
  int max_axfunc_output_size,
  int max_post_proc_output_size,
  void input_generator(int, void **, int *),
  timed_offload_request_args ***offload_args,
  ax_comp *comps, uint64_t *ts0,
  uint64_t *ts1, uint64_t *ts2,
  uint64_t *ts3, uint64_t *ts4
);

typedef void (*offload_args_free_fn_t)(int, timed_offload_request_args***);

typedef void (*input_generator_fn_t)(int, void **, int *);

typedef void (*three_phase_executor_fn_t)(
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
);

typedef bool (*output_validation_fn_t)(timed_offload_request_args *);

/* Three Phase Harnesses */
void three_phase_harness(
  executor_args_allocator_fn_t executor_args_allocator,
  executor_args_free_fn_t executor_args_free,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  executor_fn_t three_phase_executor,
  executor_stats_t *stats,
  output_validation_fn_t  output_validate,
  int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size,
  int idx
);

void three_phase_offload_timed_breakdown(
  fcontext_fn_t request_fn,
  executor_args_allocator_fn_t executor_args_allocator,
  executor_args_free_fn_t executor_args_free,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  three_phase_executor_fn_t three_phase_executor,
  int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size,
  executor_stats_t *stats, int idx
);

/* Two Phase Harnesses */
extern int requests_completed;

void blocking_ax_closed_loop_test(
  fcontext_fn_t request_fn,
  void (* payload_allocator)(int, char***),
  void (* payload_free)(int, char***),
  int requests_sampling_interval,
  int total_requests,
  uint64_t *exetime, int idx
  );
void blocking_ax_request_breakdown(
  fcontext_fn_t request_fn,
  void (* payload_allocator)(int, char***),
  void (* payload_free)(int, char***),
  int total_requests,
  uint64_t *offload_time, uint64_t *wait_time, uint64_t *kernel2_time, int idx
);
void blocking_ax_request_breakdown(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int total_requests,
  uint64_t *offload_time, uint64_t *wait_time, uint64_t *kernel2_time, int idx
);
void blocking_request_offered_load(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)(int, offload_request_args***, ax_comp *comps),
  void (* offload_args_free)(int, offload_request_args***),
  int requests_sampling_interval,
  int total_requests,
  uint64_t *exetime, int idx
);

void gpcore_closed_loop_test(
  fcontext_fn_t request_fn,
  void (* payload_allocator)(int, char****),
  void (* payload_free)(int, char****),
  int requests_sampling_interval,
  int total_requests,
  uint64_t *exetime, int idx
);
void gpcore_request_breakdown(
  fcontext_fn_t request_fn,
  void (* payload_allocator)(int, char****),
  void (* payload_free)(int, char****),
  int total_requests,
  uint64_t *kernel1, uint64_t *kernel2, int idx
);

void yielding_offload_ax_closed_loop_test(
  fcontext_fn_t request_fn,
  void (* payload_allocator)(int, char***),
  void (* payload_free)(int, char***),
  int requests_sampling_interval,
  int total_requests,
  uint64_t *exetime, int idx
);
void yielding_request_offered_load(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)(int, offload_request_args***, ax_comp *comps),
  void (* offload_args_free)(int, offload_request_args***),
  int requests_sampling_interval,
  int total_requests,
  uint64_t *exetime, int idx
);
void yielding_request_breakdown(
  fcontext_fn_t request_fn,
  void (* payload_allocator)(int, char***),
  void (* payload_free)(int, char***),
  int total_requests,
  uint64_t *offload_time, uint64_t *wait_time, uint64_t *kernel2_time, int idx
);
void yielding_request_breakdown(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int total_requests,
  uint64_t *offload_time, uint64_t *wait_time, uint64_t *kernel2_time, int idx
);
void yielding_best_case_request_breakdown(
  fcontext_fn_t request_fn,
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int total_requests,
  uint64_t *offload_time, uint64_t *wait_time, uint64_t *kernel2_time, int idx
);
void yielding_same_interleaved_request_breakdown(
  fcontext_fn_t request_fn,
  fcontext_fn_t interleave_fn, /* should take a comp record as arg and check it every 200 cycles */
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int total_requests,
  uint64_t *offload_time, uint64_t *wait_time, uint64_t *kernel2_time, int idx
);
void yielding_multiple_filler_request_breakdown(
  fcontext_fn_t request_fn,
  fcontext_fn_t filler_fn, /*filler should take a preemption signal argument and check it every 200 cycles */
  void (* offload_args_allocator)
    (int, timed_offload_request_args***,
      ax_comp *comps, uint64_t *ts0,
      uint64_t *ts1, uint64_t *ts2,
      uint64_t *ts3),
  void (* offload_args_free)(int, timed_offload_request_args***),
  int total_requests,
  uint64_t *offload_time,
  uint64_t *wait_time,
  uint64_t *kernel2_time,
  int idx
);

#endif