#ifndef JOB_H
#define JOB_H

#include "SingleJobQueue.h"

#define MAX_PIPES 10
typedef int job_id_t;

typedef struct Job
{
  job_process_queue_t process_queue; //carry pids of all processes with it
  int pipes[MAX_PIPES][2];
  bool is_background;
  job_id_t job_id;
  char* cmd;
} Job;

Job new_Job();

void push_process_front_to_job(Job* job, pid_t pid);

void destroy_job(Job* job);
void destroy_job_callback(Job job);

#endif
