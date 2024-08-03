#ifndef THREE_PHASE_HARNESS_H
#define THREE_PHASE_HARNESS_H

#include "offload_args.h"
#include "router_requests.h"
extern "C" {
  #include "fcontext.h"
  #include <zlib.h>
}
#include "emul_ax.h"
#include "stats.h"

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
);
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
);

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
);

void run_three_phase_offload_timed(
  fcontext_fn_t request_fn,
  offload_args_allocator_fn_t offload_args_allocator,
  offload_args_free_fn_t offload_args_free,
  input_generator_fn_t input_generator,
  three_phase_executor_fn_t three_phase_executor,
  int iter, int total_requests, int initial_payload_size, int max_axfunc_output_size,
  int max_post_proc_output_size
);

uLong get_compress_bound(int payload_size);
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
);

void free_three_phase_stamped_args(
  int total_requests,
  timed_offload_request_args ***off_args
);

static void complete_request_and_switch_to_scheduler(fcontext_transfer_t arg);
template <typename pre_proc_fn,
  typename prep_desc_fn, typename submit_desc_fn,
  typename desc_t, typename comp_record_t, typename ax_handle_t,
  typename post_proc_fn,
  typename preempt_signal_t>
static void generic_yielding_three_phase_timed(
  preempt_signal_t sig, fcontext_transfer_t arg,
  pre_proc_fn pre_proc_func, void *pre_proc_input, void *pre_proc_output, int pre_proc_input_size,
  prep_desc_fn prep_func, submit_desc_fn submit_func,
  comp_record_t *comp, desc_t *desc, ax_handle_t *ax,
  void *ax_func_output, int max_axfunc_output_size,
  post_proc_fn post_proc_func, void *post_proc_output, int post_proc_input_size, int max_post_proc_output_size,
  uint64_t *ts0, uint64_t *ts1, uint64_t *ts2, uint64_t *ts3, uint64_t *ts4, int idx
  );

template <typename pre_proc_fn,
  typename prep_desc_fn, typename submit_desc_fn, typename post_offload_fn,
  typename desc_t, typename comp_record_t, typename ax_handle_t,
  typename post_proc_fn,
  typename preempt_signal_t>
static inline void generic_blocking_three_phase_timed(
  preempt_signal_t sig, fcontext_transfer_t arg,
  pre_proc_fn pre_proc_func, void *pre_proc_input, void *pre_proc_output, int pre_proc_input_size,
  prep_desc_fn prep_func, submit_desc_fn submit_func, post_offload_fn post_offload_func,
  comp_record_t *comp, desc_t *desc, ax_handle_t *ax,
  void *ax_func_output, int max_axfunc_output_size,
  post_proc_fn post_proc_func, void *post_proc_output, int post_proc_input_size, int max_post_proc_output_size,
  uint64_t *ts0, uint64_t *ts1, uint64_t *ts2, uint64_t *ts3, uint64_t *ts4, int idx
  );

template <typename pre_proc_fn,
  typename prep_desc_fn, typename submit_desc_fn, typename post_offload_fn,
  typename desc_t, typename comp_record_t, typename ax_handle_t,
  typename post_proc_fn,
  typename preempt_signal_t>
static inline void generic_blocking_three_phase(
  preempt_signal_t sig, fcontext_transfer_t arg,
  pre_proc_fn pre_proc_func, void *pre_proc_input, void *pre_proc_output, int pre_proc_input_size,
  prep_desc_fn prep_func, submit_desc_fn submit_func, post_offload_fn post_offload_func,
  comp_record_t *comp, desc_t *desc, ax_handle_t *ax,
  void *ax_func_output, int max_axfunc_output_size,
  post_proc_fn post_proc_func, void *post_proc_output, int post_proc_input_size, int max_post_proc_output_size
  );

template <typename pre_proc_fn,
  typename prep_desc_fn, typename submit_desc_fn,
  typename desc_t, typename comp_record_t, typename ax_handle_t,
  typename post_proc_fn,
  typename preempt_signal_t>
static inline void generic_yielding_three_phase(
  preempt_signal_t sig, fcontext_transfer_t arg,
  pre_proc_fn pre_proc_func, void *pre_proc_input, void *pre_proc_output, int pre_proc_input_size,
  prep_desc_fn prep_func, submit_desc_fn submit_func,
  comp_record_t *comp, desc_t *desc, ax_handle_t *ax,
  void *ax_func_output, int max_axfunc_output_size,
  post_proc_fn post_proc_func, void *post_proc_output, int post_proc_input_size, int max_post_proc_output_size
  );

#include "inline/three_phase_harness.ipp"

#endif