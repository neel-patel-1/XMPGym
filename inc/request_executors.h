#ifndef REQUEST_EXEC
#define REQUEST_EXEC

#include "router_request_args.h"
#include "router_requests.h"
#include "offload.h"
extern "C" {
  #include "fcontext.h"
}
#include "print_utils.h"
#include "timer_utils.h"
#include <functional>

/* Three phase executors */
typedef struct _executor_args_t{
  int total_requests;
  timed_offload_request_args **off_args;
  fcontext_state_t **off_req_state;
  fcontext_transfer_t *offload_req_xfer;
  ax_comp *comps;
  uint64_t *ts0;
  uint64_t *ts1;
  uint64_t *ts2;
  uint64_t *ts3;
  uint64_t *ts4;
  int idx;
} executor_args_t;

typedef struct _executor_stats_t{
  uint64_t *pre_proc_times;
  uint64_t *offload_tax_times;
  uint64_t *ax_func_times;
  uint64_t *post_proc_times;
  uint64_t *exe_time_start;
  uint64_t *exe_time_end;
  int iter;
} executor_stats_t;

typedef void (*executor_fn_t)(
  executor_args_t *args,
  executor_stats_t *stats
);

typedef std::function<void(executor_args_t **, int, int)> executor_args_allocator_fn_t;

typedef void (*executor_args_free_fn_t)(
  executor_args_t *args
);

typedef void (*executor_stats_allocator_fn_t)(
  executor_stats_t *stats, int iter
);

typedef void (*executor_stats_free_fn_t)(
  executor_stats_t *stats
);

typedef void (*executor_stats_processor_fn_t)(
  executor_stats_t *stats, int iter, int total_requests
);

void execute_yielding_three_phase_request_throughput(
  executor_args_t *args,
  executor_stats_t *stats
);
void execute_three_phase_blocking_requests_closed_system_throughput(
  executor_args_t *args,
  executor_stats_t *stats
);

void execute_three_phase_blocking_requests_closed_system_request_breakdown(
  executor_args_t *args,
  executor_stats_t *stats
);
void execute_three_phase_yielding_requests_closed_system_request_breakdown(
  executor_args_t *args,
  executor_stats_t *stats
);


/* Two Phase Executors*/
void execute_yielding_requests_closed_system_with_sampling(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self, uint64_t *exetime, int idx);
void execute_yielding_requests_closed_system_request_breakdown(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx);
void execute_yielding_requests_closed_system_request_breakdown(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx);
void execute_yielding_requests_best_case(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx);
void execute_yielding_requests_interleaved(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self,
  fcontext_state_t *interleaved_state,
  fcontext_transfer_t interleaved_xfer,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx);
void execute_yielding_requests_multiple_filler_requests(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t **filler_req_state,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx);

void execute_blocking_requests_closed_system_with_sampling(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *exetime, int idx);
void execute_blocking_requests_closed_system_request_breakdown(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *off_times, uint64_t *wait_times, uint64_t *hash_times, int idx);
void execute_blocking_requests_closed_system_request_breakdown(
  int total_requests,
  timed_offload_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *off_times, uint64_t *wait_times, uint64_t *kernel2_time,
  int idx);

void execute_cpu_requests_closed_system_with_sampling(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, cpu_request_args **off_args,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *rps, int idx);
void execute_cpu_requests_closed_system_request_breakdown(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, timed_cpu_request_args **off_args,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *deser_times, uint64_t *hash_times, int idx);

void execute_gpcore_requests_closed_system_with_sampling(
  int total_requests,
  gpcore_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *exetime, int idx);
void execute_gpcore_requests_closed_system_request_breakdown(
  int total_requests,
  timed_gpcore_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *kernel1, uint64_t *kernel2, int idx);

#endif