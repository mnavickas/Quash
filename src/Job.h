#ifndef JOB_H
#define JOB_H

#include "SingleJobQueue.h"

#define MAX_PIPES 10

typedef struct Job
{
  job_process_queue_t process_queue; //carry pids of all processes with it
  int pipes[MAX_PIPES][2];
  bool isBackground;
  bool isCompleted;
  int jobID;
  char* cmd;
} Job;

Job new_Job(int max_pipes_needed);

void push_front_job(Job* job,int pid);

void destroy_job(Job* job);

#endif
