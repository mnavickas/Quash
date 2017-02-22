
#include <unistd.h>
#include "Job.h"

Job new_Job()
{
  Job job;
  job.is_background = false;
  job.process_queue = new_job_process_queue_t(0);
  for(int i = 0; i < MAX_PIPES; i++)
  {
    pipe(job.pipes[i]);
  }
  return job;
}

void push_process_front_to_job(Job* job,pid_t pid)
{
  push_front_job_process_queue_t(&(job->process_queue),pid);
}

void destroy_job(Job* job)
{
  destroy_job_process_queue_t(&(job->process_queue));
  if(job->is_background)
  {
    free(job->cmd);
  }
}
void destroy_job_callback(Job job)
{
  destroy_job_process_queue_t(&(job.process_queue));
  if(job.is_background)
  {
    free(job.cmd);
  }
}
