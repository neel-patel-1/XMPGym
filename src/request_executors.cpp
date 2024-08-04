#include "request_executors.h"

void execute_yielding_three_phase_request_throughput(
  executor_args_t *args,
  executor_stats_t *stats
)
    /* pass in the times we measure and idx to populate */
{
  int requests_completed = 0;
  uint64_t start, end;
  int idx = args->idx;
  int total_requests = args->total_requests;
  timed_offload_request_args **off_args = args->off_args;
  fcontext_state_t **off_req_state = args->off_req_state;
  fcontext_transfer_t *offload_req_xfer = args->offload_req_xfer;
  ax_comp *comps = args->comps;

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;

  LOG_PRINT(LOG_DEBUG, "Total Requests:%d\n", total_requests);

  start = sampleCoderdtsc();

  while(next_request_offload_to_complete_idx < total_requests){
    if(comps[next_request_offload_to_complete_idx].status != COMP_STATUS_PENDING){
      fcontext_swap(offload_req_xfer[next_request_offload_to_complete_idx].prev_context, NULL);
      LOG_PRINT(LOG_DEBUG, "Completed Request: %d\n", next_request_offload_to_complete_idx);
      next_request_offload_to_complete_idx++;
    } else if(next_unstarted_req_idx < total_requests){
      offload_req_xfer[next_unstarted_req_idx] =
        fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
      LOG_PRINT(LOG_DEBUG, "Started Request: %d\n", next_unstarted_req_idx);
      next_unstarted_req_idx++;
    }
  }

  end = sampleCoderdtsc();

  stats->exe_time_start[idx] = start;
  stats->exe_time_end[idx] = end;
}

void execute_three_phase_blocking_requests_closed_system_throughput(
  executor_args_t *args,
  executor_stats_t *stats
)
    /* pass in the times we measure and idx to populate */
{
  int next_unstarted_req_idx = 0;
  uint64_t start, end;
  int idx = args->idx;
  int total_requests = args->total_requests;
  timed_offload_request_args **off_args = args->off_args;
  fcontext_state_t **off_req_state = args->off_req_state;

  LOG_PRINT(LOG_DEBUG, "Total Requests:%d\n", total_requests);

  start = sampleCoderdtsc();

  while(next_unstarted_req_idx < total_requests){
    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;
  }

  end = sampleCoderdtsc();

  stats->exe_time_start[idx] = start;
  stats->exe_time_end[idx] = end;
}

void execute_three_phase_blocking_requests_closed_system_request_breakdown(
  executor_args_t *args,
  executor_stats_t *stats
)
    /* pass in the times we measure and idx to populate */
{
  int next_unstarted_req_idx = 0;
  int total_requests = args->total_requests;
  timed_offload_request_args **off_args = args->off_args;
  fcontext_state_t **off_req_state = args->off_req_state;
  fcontext_transfer_t *offload_req_xfer = args->offload_req_xfer;
  ax_comp *comps = args->comps;
  uint64_t *pre_proc_times = stats->pre_proc_times;
  uint64_t *offload_tax_times = stats->offload_tax_times;
  uint64_t *ax_func_times = stats->ax_func_times;
  uint64_t *post_proc_times = stats->post_proc_times;
  int idx = args->idx;

  while(next_unstarted_req_idx < total_requests){
    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;
  }

  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t *ts4 = off_args[0]->ts4;
  uint64_t avg, diff[total_requests];

  avg_samples_from_arrays(diff, pre_proc_times[idx], ts1, ts0, total_requests);
  LOG_PRINT( LOG_DEBUG, "PreProcTime: %lu\n", pre_proc_times[idx]);

  avg_samples_from_arrays(diff, offload_tax_times[idx], ts2, ts1, total_requests);
  LOG_PRINT( LOG_DEBUG, "OffloadTaxTime: %lu\n", offload_tax_times[idx]);

  avg_samples_from_arrays(diff, ax_func_times[idx], ts3, ts2, total_requests);
  LOG_PRINT( LOG_DEBUG, "AxFuncTime: %lu\n", ax_func_times[idx]);

  avg_samples_from_arrays(diff, post_proc_times[idx], ts4, ts3, total_requests);
  LOG_PRINT( LOG_DEBUG, "PostProcTime: %lu\n", post_proc_times[idx]);
}

void execute_three_phase_yielding_requests_closed_system_request_breakdown(
  executor_args_t *args,
  executor_stats_t *stats
)
{
  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int total_requests = args->total_requests;
  timed_offload_request_args **off_args = args->off_args;
  fcontext_transfer_t *offload_req_xfer = args->offload_req_xfer;
  fcontext_state_t **off_req_state = args->off_req_state;
  ax_comp *comps = args->comps;
  uint64_t *pre_proc_times = stats->pre_proc_times;
  uint64_t *offload_tax_times = stats->offload_tax_times;
  uint64_t *ax_func_times = stats->ax_func_times;
  uint64_t *post_proc_times = stats->post_proc_times;
  int idx = args->idx;

  while(next_request_offload_to_complete_idx < total_requests){
    if(comps[next_request_offload_to_complete_idx].status != COMP_STATUS_PENDING){
      fcontext_swap(offload_req_xfer[next_request_offload_to_complete_idx].prev_context, NULL);
      LOG_PRINT( LOG_DEBUG, "Completed %d\n", next_request_offload_to_complete_idx);
      next_request_offload_to_complete_idx++;
    } else if(next_unstarted_req_idx < total_requests){
      offload_req_xfer[next_unstarted_req_idx] =
        fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
      LOG_PRINT( LOG_DEBUG, "Started %d\n", next_unstarted_req_idx);
      next_unstarted_req_idx++;
    }
  }

  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t *ts4 = off_args[0]->ts4;
  uint64_t avg, diff[total_requests];

  avg_samples_from_arrays(diff, pre_proc_times[idx], ts1, ts0, total_requests);
  LOG_PRINT( LOG_DEBUG, "PreProcTime: %lu\n", pre_proc_times[idx]);

  avg_samples_from_arrays(diff, offload_tax_times[idx], ts2, ts1, total_requests);
  LOG_PRINT( LOG_DEBUG, "OffloadTaxTime: %lu\n", offload_tax_times[idx]);

  avg_samples_from_arrays(diff, ax_func_times[idx], ts3, ts2, total_requests);
  LOG_PRINT( LOG_DEBUG, "AxFuncTime: %lu\n", ax_func_times[idx]);

  avg_samples_from_arrays(diff, post_proc_times[idx], ts4, ts3, total_requests);
  LOG_PRINT( LOG_DEBUG, "PostProcTime: %lu\n", post_proc_times[idx]);
}


void execute_yielding_requests_closed_system_with_sampling(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self, uint64_t *exetime, int idx)
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;
  int exetime_samples = total_requests / requests_sampling_interval;


  sampling_interval_completion_times[0] = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){
    if(comps[next_request_offload_to_complete_idx].status == COMP_STATUS_COMPLETED){
      fcontext_swap(offload_req_xfer[next_request_offload_to_complete_idx].prev_context, NULL);
      next_request_offload_to_complete_idx++;
      if(requests_completed % requests_sampling_interval == 0 && requests_completed > 0){
        sampling_interval_completion_times[sampling_interval] = sampleCoderdtsc();
        sampling_interval++;
      }
    } else if(next_unstarted_req_idx < total_requests){
      offload_req_xfer[next_unstarted_req_idx] =
        fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
      next_unstarted_req_idx++;
    }
  }
  for(int i=0; i<exetime_samples; i++){
    exetime[idx + i] = (sampling_interval_completion_times[i+1] - sampling_interval_completion_times[i]);
    LOG_PRINT( LOG_MONITOR, "ExeTime: %ld\n", exetime[i]);
  }
}

void execute_yielding_requests_closed_system_request_breakdown(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx)
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;


  sampling_interval_completion_times[0] = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){
    if(comps[next_request_offload_to_complete_idx].status == COMP_STATUS_COMPLETED){
      fcontext_swap(offload_req_xfer[next_request_offload_to_complete_idx].prev_context, NULL);
      next_request_offload_to_complete_idx++;
      if(requests_completed % requests_sampling_interval == 0 && requests_completed > 0){
        sampling_interval_completion_times[sampling_interval] = sampleCoderdtsc();
        sampling_interval++;
      }
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
  uint64_t diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);
  avg_samples_from_arrays(diff, yield_to_resume_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "YieldToResumeDelay: %lu\n", yield_to_resume_times[idx]);
  avg_samples_from_arrays(diff, hash_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}

void execute_yielding_requests_closed_system_request_breakdown(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx)
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
  uint64_t diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);
  avg_samples_from_arrays(diff, yield_to_resume_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "YieldToResumeDelay: %lu\n", yield_to_resume_times[idx]);
  avg_samples_from_arrays(diff, hash_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}

void execute_yielding_requests_best_case(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx)
{

  /*
  while(reqs < total)
  xfer = start_yielding_req
  wait_for_yielding_reqs_comp
  resume_yielding_req(xfer)
  */
  while(requests_completed < total_requests){
    offload_req_xfer[requests_completed] =
        fcontext_swap(off_req_state[requests_completed]->context, off_args[requests_completed]);
    while(comps[requests_completed].status != COMP_STATUS_COMPLETED){
      _mm_pause();
    }
    fcontext_swap(offload_req_xfer[requests_completed].prev_context, NULL);
  }

  /* get the time stamps from one of the requests */
  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);
  avg_samples_from_arrays(diff, yield_to_resume_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "YieldToResumeDelay: %lu\n", yield_to_resume_times[idx]);
  avg_samples_from_arrays(diff, hash_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}

void execute_yielding_requests_multiple_filler_requests(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t **filler_req_state,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx)
{

  bool interleaved_started = false;
  fcontext_transfer_t xfer;
  /* have our context switch overhead and get re-entrancy too*/
  while(requests_completed < total_requests){
    offload_req_xfer[requests_completed] =
        fcontext_swap(off_req_state[requests_completed]->context, off_args[requests_completed]);
    interleaved_started = false;
    while(comps[requests_completed].status != COMP_STATUS_COMPLETED){
      if(! interleaved_started ){
        xfer = fcontext_swap(filler_req_state[requests_completed]->context, &(comps[requests_completed]));
        interleaved_started = true;
      } else {
        xfer = fcontext_swap(xfer.prev_context,  &(comps[requests_completed]));
      }
    }
    fcontext_swap(offload_req_xfer[requests_completed].prev_context, NULL);
  }

  /* get the time stamps from one of the requests */
  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);
  avg_samples_from_arrays(diff, yield_to_resume_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "YieldToResumeDelay: %lu\n", yield_to_resume_times[idx]);
  avg_samples_from_arrays(diff, hash_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}


void execute_yielding_requests_interleaved(
  int total_requests,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state,
  fcontext_state_t *self,
  fcontext_state_t *interleaved_state,
  fcontext_transfer_t interleaved_xfer,
  uint64_t *off_times, uint64_t *yield_to_resume_times, uint64_t *hash_times, int idx)
{
  bool interleaved_not_started = true;
  /*
  while(reqs < total)
  xfer = start_yielding_req
  wait_for_yielding_reqs_comp
  resume_yielding_req(xfer)
  */
  fcontext_state_t *extern_func;
  while(requests_completed < total_requests){
    offload_req_xfer[requests_completed] =
        fcontext_swap(off_req_state[requests_completed]->context, off_args[requests_completed]);
    while(comps[requests_completed].status != COMP_STATUS_COMPLETED){
      /* exec probed kernel */
      if( interleaved_not_started)
      {
        interleaved_xfer = fcontext_swap(interleaved_state->context, &(comps[requests_completed]));
        interleaved_not_started = false;
      } else {
        interleaved_xfer =
          fcontext_swap(interleaved_xfer.prev_context, &(comps[requests_completed]));
      }
    }
    fcontext_swap(offload_req_xfer[requests_completed].prev_context, NULL);
  }

  /* get the time stamps from one of the requests */
  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);
  avg_samples_from_arrays(diff, yield_to_resume_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "YieldToResumeDelay: %lu\n", yield_to_resume_times[idx]);
  avg_samples_from_arrays(diff, hash_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}



void execute_blocking_requests_closed_system_with_sampling(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *exetime, int idx)
    /* pass in the times we measure and idx to populate */
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;

  sampling_interval_completion_times[0] = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){
    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;
  }
  sampling_interval_completion_times[sampling_interval] = sampleCoderdtsc();


  exetime[idx] = (sampling_interval_completion_times[sampling_interval] - sampling_interval_completion_times[0]);
  LOG_PRINT( LOG_DEBUG, "ExeTime: %ld\n", exetime[idx]);
}

void execute_blocking_requests_closed_system_request_breakdown(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, timed_offload_request_args **off_args,
  fcontext_transfer_t *offload_req_xfer,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *off_times, uint64_t *wait_times, uint64_t *hash_times, int idx)
    /* pass in the times we measure and idx to populate */
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;

  sampling_interval_completion_times[0] = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){
    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;
  }
  sampling_interval_completion_times[sampling_interval] = sampleCoderdtsc();

  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;
  uint64_t *ts3 = off_args[0]->ts3;
  uint64_t avg, diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);

  avg_samples_from_arrays(diff, wait_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "WaitTime: %lu\n", wait_times[idx]);

  avg_samples_from_arrays(diff, hash_times[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}

void execute_cpu_requests_closed_system_with_sampling(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, cpu_request_args **off_args,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *ExeTime, int idx)
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;

  sampling_interval_completion_times[0] = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){

    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;

  }
  sampling_interval_completion_times[sampling_interval] = sampleCoderdtsc();

  uint64_t this_rps =
    (sampling_interval_completion_times[sampling_interval] - sampling_interval_completion_times[0]);
  ExeTime[idx] = this_rps;
  LOG_PRINT( LOG_DEBUG, "ExeTime: %ld\n", this_rps);

}

void execute_cpu_requests_closed_system_request_breakdown(
  int requests_sampling_interval, int total_requests,
  uint64_t *sampling_interval_completion_times, int sampling_interval_timestamps,
  ax_comp *comps, timed_cpu_request_args **off_args,
  fcontext_state_t **off_req_state, fcontext_state_t *self,
  uint64_t *deser_times, uint64_t *hash_times, int idx)
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;

  sampling_interval_completion_times[0] = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){

    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;

  }
  sampling_interval_completion_times[sampling_interval] = sampleCoderdtsc();

  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;

  uint64_t avg, diff[total_requests];
  avg_samples_from_arrays(diff, deser_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Deser time: %lu\n", deser_times[idx]);
  avg_samples_from_arrays(diff, hash_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "HashTime: %lu\n", hash_times[idx]);
}

void execute_gpcore_requests_closed_system_request_breakdown(
  int total_requests,
  timed_gpcore_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *kernel1, uint64_t *kernel2, int idx)
{

  int next_unstarted_req_idx = 0;

  while(requests_completed < total_requests){
    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;
  }

  uint64_t *ts0 = off_args[0]->ts0;
  uint64_t *ts1 = off_args[0]->ts1;
  uint64_t *ts2 = off_args[0]->ts2;

  uint64_t avg, diff[total_requests];
  avg_samples_from_arrays(diff, kernel1[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Kernel1Time: %lu\n", kernel1[idx]);
  avg_samples_from_arrays(diff, kernel2[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Kernel2Time: %lu\n", kernel2[idx]);
}

void execute_gpcore_requests_closed_system_with_sampling(
  int total_requests,
  gpcore_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *exetime, int idx)
{

  int next_unstarted_req_idx = 0;
  int next_request_offload_to_complete_idx = 0;
  int sampling_interval = 0;
  uint64_t start, end;

  start = sampleCoderdtsc(); /* start time */
  sampling_interval++;

  while(requests_completed < total_requests){

    fcontext_swap(off_req_state[next_unstarted_req_idx]->context, off_args[next_unstarted_req_idx]);
    next_unstarted_req_idx++;

  }
  end = sampleCoderdtsc();


  uint64_t this_rps =
    (end - start);
  exetime[idx] = this_rps;
  LOG_PRINT( LOG_DEBUG, "ExeTime: %ld\n", this_rps);
}

void execute_blocking_requests_closed_system_request_breakdown(
  int total_requests,
  timed_offload_request_args **off_args,
  fcontext_state_t **off_req_state,
  uint64_t *off_times, uint64_t *wait_times, uint64_t *kernel2_time,
  int idx)
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
  uint64_t avg, diff[total_requests];
  avg_samples_from_arrays(diff, off_times[idx], ts1, ts0, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Offload time: %lu\n", off_times[idx]);

  avg_samples_from_arrays(diff, wait_times[idx], ts2, ts1, requests_completed);
  LOG_PRINT( LOG_DEBUG, "WaitTime: %lu\n", wait_times[idx]);

  avg_samples_from_arrays(diff, kernel2_time[idx], ts3, ts2, requests_completed);
  LOG_PRINT( LOG_DEBUG, "Kernel2Time: %lu\n", kernel2_time[idx]);
}