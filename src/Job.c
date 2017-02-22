
#include <unistd.h>
#include "Job.h"

Job new_Job(int max_pipes_needed)
{
  Job job;
  job.process_queue = new_job_process_queue_t(0);
  for(int i = 0; i < MAX_PIPES; i++)
  {
    pipe(job.pipes[i]);
  }
  return job;
}

void push_front_job(Job* job,int pid)
{
  push_front_job_process_queue_t(&(job->process_queue),pid);
}

void destroy_job(Job* job)
{
  destroy_job_process_queue_t(&(job->process_queue));
}
