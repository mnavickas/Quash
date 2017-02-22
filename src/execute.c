/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "quash.h"
#include "Job.h"

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__,        \
            __LINE__, __FUNCTION__)

/***************************************************************************
 * Memory Constants
 ***************************************************************************/
#define GET_CWD_BSIZE 512
#define READ_END 0
#define WRITE_END 1

/***************************************************************************
 * Globals
 ***************************************************************************/
job_id_t job_id = 1;

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  //Get CWD Mallocs space for the return value
  // Change this to true if necessary
  *should_free = true;

  return getcwd(NULL, GET_CWD_BSIZE);
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.


  if( is_empty_background_job_queue_t(&background_queue) )
  {
    return;
  }

  int job_queue_length = length_background_job_queue_t(&background_queue);

  for(int i = 0; i < job_queue_length; i++)
  {
    Job job;
    job_process_queue_t queue;
    int process_queue_length;
    bool job_still_has_running_process;;

    job = pop_front_background_job_queue_t(&background_queue);
    queue = job.process_queue;
    process_queue_length = length_job_process_queue_t(&queue);
    job_still_has_running_process = false;



    for( int j = 0; j < process_queue_length; j++ )
    {
      int status;
      int pid;
      pid_t return_pid;

      pid = pop_front_job_process_queue_t(&queue);
      return_pid = waitpid(pid, &status, WNOHANG);
      if ( -1 == return_pid )
      {
          // error
      }
      else if ( 0 == return_pid )
      {
          // child is still running
          job_still_has_running_process = true;
      }
      else if (return_pid == pid)
      {
          // child is finished.
      }
      //put it back in the container.
      push_back_job_process_queue_t(&queue,pid);
    } // end for process_queue_length

    if( job_still_has_running_process )
    {
      // re-add it to the queue
      push_back_background_job_queue_t(&background_queue,job);
    }
    else
    {
      // don't add it back, print message
      print_job_bg_complete(
                              job.job_id,
                              peek_front_job_process_queue_t(&job.process_queue),
                              job.cmd
                           );
      destroy_job(&job);
    }
  } //end for job_queue_length
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(job_id_t job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(job_id_t job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(job_id_t job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  execvp(exec, args);

  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;
  char* word;

  while( (word = *(str++)))
  {
    printf("%s ", word);
  }

  // Flush the buffer before returning
  printf("\n");
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  setenv(env_var, val, 1); //1 is overwrite mode
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;
  char* old_dir;
  char* new_dir;

  // Check if the directory is valid
  if ( NULL == dir ) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  old_dir = getcwd(NULL, GET_CWD_BSIZE);

  // Change directory
  chdir(dir);

  // Update the PWD environment variable to be the new current working
  // directory and optionally update OLD_PWD environment variable to be the old
  // working directory.
  new_dir = getcwd(NULL, GET_CWD_BSIZE);

  setenv("PWD", new_dir, 1);
  setenv("OLDPWD", old_dir, 1);

  free(new_dir);
  free(old_dir);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  job_id_t job_id = cmd.job;

  // FOR process IN job
  // kill(process.id, signal)
  // END FOR

  int job_queue_length = length_background_job_queue_t(&background_queue);

  for(int i = 0; i < job_queue_length; i++)
  {

    Job job = pop_front_background_job_queue_t(&background_queue);
    if( job_id == job.job_id )
    {
      job_process_queue_t queue;
      int process_queue_length;

      queue = job.process_queue;
      process_queue_length = length_job_process_queue_t(&queue);

      for( int j = 0; j < process_queue_length; j++)
      {
        int pid = pop_front_job_process_queue_t(&queue);
        kill(pid,signal);
        push_back_job_process_queue_t(&queue,pid);
      }
      push_back_background_job_queue_t(&background_queue,job);

    }
    else
    {
        push_back_background_job_queue_t(&background_queue, job);
    }
  } //end for job_queue_length
}


// Prints the current working directory to stdout
void run_pwd() {
  // Print the current working directory
  bool should_free;
  char* str = get_current_directory(&should_free);
  printf("%s\n", str);

  if(should_free)
  {
    free(str);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // Print background jobs
  if( is_empty_background_job_queue_t(&background_queue) )
  {
    return;
  }

  int job_queue_length = length_background_job_queue_t(&background_queue);

  for(int i = 0; i < job_queue_length; i++)
  {
    Job job = pop_front_background_job_queue_t(&background_queue);
    print_job(
                job.job_id,
                peek_front_job_process_queue_t(&job.process_queue),
                job.cmd
             );
    push_back_background_job_queue_t(&background_queue,job);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, Job* current_job, int stepOfJob) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  int pid = fork();
  if( 0 == pid )
  {
    // redirect input from file
    if( r_in )
    {
      int file_in = open(holder.redirect_in,O_RDONLY);
      dup2(file_in,STDIN_FILENO);
      close(file_in);
    }
    // redirect output to file
    if( r_out )
    {
      FILE * file;
      if( r_app )
      {
        file = fopen(holder.redirect_out,"a");
      }
      else
      {
        file = fopen(holder.redirect_out,"w");
      }
      dup2(fileno(file),STDOUT_FILENO);
      fclose(file);
    }
    // If we are piping output elsewhere
    if( p_out )
    {
      // duplicate
      dup2(current_job->pipes[stepOfJob][WRITE_END],STDOUT_FILENO);
      // dup'd handle is sufficient
      close(current_job->pipes[stepOfJob][WRITE_END]);
    }
    else
    {
      // close it since we arent using it
      close(current_job->pipes[stepOfJob][WRITE_END]);
    }

    // if we are piping input from elsewhere
    if( p_in )
    {
      // duplicate
      dup2(current_job->pipes[stepOfJob-1][READ_END],STDIN_FILENO);
      // dup'd handle is sufficient
      close(current_job->pipes[stepOfJob-1][READ_END]);
    }
    else if( 0 <= stepOfJob-1 )
    {
      // close it since we arent using it
      close(current_job->pipes[stepOfJob-1][READ_END]);
    }

    child_run_command(holder.cmd); // This should be done in the child branch
    destroy_job(current_job);
    exit(EXIT_SUCCESS);
  }
  else
  {
     if( p_out )
     {
       close(current_job->pipes[stepOfJob][WRITE_END]);
     }
     if( p_in )
     {
       close(current_job->pipes[stepOfJob-1][READ_END]);
     }
     push_process_front_to_job(current_job,pid);
     parent_run_command(holder.cmd); // This should be done in the parent branch
  }

}

// Do the thing
void initBackgroundJobQueue(void)
{
  background_queue = new_destructable_background_job_queue_t(1,destroy_job_callback);
}

void destroyBackgroundJobQueue(void)
{
  destroy_background_job_queue_t(&background_queue);
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if ( NULL == holders )
    return;

  check_jobs_bg_status();

  if (EXIT == get_command_holder_type(holders[0]) &&
      EOC == get_command_holder_type(holders[1]))
  {
    end_main_loop();
    return;
  }

  CommandType type;
  Job current_job = new_Job();

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
  {
    create_process(holders[i], &current_job, i);
  }

  if ( !(holders[0].flags & BACKGROUND) )
  {
    // Not a background Job
    // Wait for all processes under the job to complete
    while (!is_empty_job_process_queue_t(&current_job.process_queue) )
    {
      int status;
      if (
          waitpid(peek_back_job_process_queue_t(&current_job.process_queue),
                  &status, 0) != -1)
      {
        pop_back_job_process_queue_t(&current_job.process_queue);
      }
    }

    destroy_job(&current_job);
  }
  else
  {
    // A background job.
    // Push the new job to the job queue
    current_job.is_background = true;
    current_job.cmd = get_command_string();
    current_job.job_id = job_id++;
    push_back_background_job_queue_t(&background_queue, current_job);

    print_job_bg_start(
                        current_job.job_id,
                        peek_front_job_process_queue_t(&current_job.process_queue),
                        current_job.cmd
                      );
  }
}
