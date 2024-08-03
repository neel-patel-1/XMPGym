#ifndef THREE_PHASE_HARNESS_H
#define THREE_PHASE_HARNESS_H

#include "offload_args.h"
extern "C" {
  #include "fcontext.h"
}


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



#endif