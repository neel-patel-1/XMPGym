

template <typename pre_proc_fn,
  typename prep_desc_fn, typename submit_desc_fn,
  typename desc_t, typename comp_record_t, typename ax_handle_t,
  typename post_proc_fn,
  typename preempt_signal_t>
static inline void generic_yielding_three_phase_timed(
  preempt_signal_t sig, fcontext_transfer_t arg,
  pre_proc_fn pre_proc_func, void *pre_proc_input, void *pre_proc_output, int pre_proc_input_size,
  prep_desc_fn prep_func, submit_desc_fn submit_func,
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
  LOG_PRINT(LOG_DEBUG, "AXFuncOutputSize: %d\n", post_proc_input_size);
  LOG_PRINT(LOG_VERBOSE, "AXFuncOutput: %s \n", (char *)ax_func_output);
  fcontext_swap(arg.prev_context, NULL);
  if(comp->status != COMP_STATUS_COMPLETED){
    LOG_PRINT(LOG_ERR, "Error: %d\n", comp->status);
  }

  ts3[idx] = sampleCoderdtsc();
  post_proc_input = ax_func_output;
  post_proc_func(post_proc_input, post_proc_output, post_proc_input_size, &max_post_proc_output_size);
  LOG_PRINT(LOG_DEBUG, "PostProcOutputSize: %d\n", max_post_proc_output_size);
  ts4[idx] = sampleCoderdtsc();

  return;
}

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
  if(comp->status != COMP_STATUS_COMPLETED){
    LOG_PRINT(LOG_ERR, "Error: %d\n", comp->status);
  }
  LOG_PRINT(LOG_DEBUG, "AXFuncOutputSize: %d\n", post_proc_input_size);
  LOG_PRINT(LOG_VERBOSE, "AXFuncOutput: %s \n", (char *)ax_func_output);

  ts3[idx] = sampleCoderdtsc();
  post_proc_input = ax_func_output;
  post_proc_func(post_proc_input, post_proc_output, post_proc_input_size, &max_post_proc_output_size);
  LOG_PRINT(LOG_DEBUG, "PostProcOutputSize: %d\n", max_post_proc_output_size);
  ts4[idx] = sampleCoderdtsc();

  return;
}

static inline void complete_request_and_switch_to_scheduler(fcontext_transfer_t arg){
  fcontext_swap(arg.prev_context, NULL);
}
