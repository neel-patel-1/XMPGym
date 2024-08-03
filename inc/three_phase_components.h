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

uLong get_compress_bound(int payload_size);

/* Request Argument Allocator/Free passed to runner */
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

/* Executor Arg Allocator and Free Not Included -- since they are specific to Apps */

/* Stats Allocator/Free/Print */
void alloc_throughput_stats(executor_stats_t *stats, int iter);
void free_throughput_stats(executor_stats_t *stats);
void print_throughput_stats(executor_stats_t *stats, int iter, int total_requests);


/* Generic function templates passed to runner */
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

#include "inline/three_phase_components.ipp"

#endif