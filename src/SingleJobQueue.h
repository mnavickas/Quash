#ifndef QUEUE_H
#define QUEUE_H

#include "deque.h"


/*
 * Declare the queue data structure
 */
IMPLEMENT_DEQUE_STRUCT (job_process_queue_t, pid_t);

/*
 * Declare a queue for storing process ids (i.e. integers)
 */
PROTOTYPE_DEQUE (job_process_queue_t, pid_t);

#endif
