/*
  Simulates updating a database with multiple threads. Runs on Unix.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

//Global, for each of 6 records;
pthread_mutex_t locks_array[7];
double global_time;

int error_checking(int argc, char* argv[])
{
  // Want the command and number of threads only.
  if (argc != 2)
    {
      printf("Incorrect number of arguments.\n");
      return 1;
    }

  // Number of threads should be an integer.
  char* num_threads = malloc(1024);
  strcpy(num_threads,argv[1]);
  int i;
  for(i = 0;i<strlen(num_threads);i++)
    {
      if(isdigit(num_threads[i]) == 0) {
	printf("Integer number of threads only.\n");
	return 1;
      }
    }
  
  //don't put 0 threads
  if (atoi(argv[1]) == 0) 
    {
      printf("Can't make 0 threads.\n");      return 1;
    }

  //You specify more threads than files that exist.
  int threadnum;
  for(threadnum = 1;threadnum <= atoi(argv[1]);threadnum++) 
    {
      char* file_name = malloc(1024);
      sprintf(file_name,"%d.txt",threadnum); 
      FILE * file_ptr;
      file_ptr = fopen(file_name,"r");
      if(file_ptr == NULL) 
	{
	  printf("Number of threads indicated does not match number of files in directory.\n");
	  return 1;
	}
      fclose(file_ptr);
    }
  return 0;
}


// Simulate updating for t seconds
void update(int t)
{
  int i;
  for (i=0;i<t*50000000;i++) {}
}

void* update_db(void* arg_ptr)
{
  // Open the files for reading and writing
  char file_name[1024];
  sprintf(file_name,"%d.txt",(int)((double*)arg_ptr)[0]);
  FILE * file_read_ptr;
  file_read_ptr = fopen(file_name,"r");
  FILE * file_write_ptr;

  char line[1024];
  double total_updating = 0;
  double total_running = clock();

  do
    {
      //Get the record number and the seconds.
      fgets(line,1024,file_read_ptr);
      char* tok=malloc(1024);
      tok = strtok (line," ");
      int record_num=atoi(tok);
      tok = strtok(NULL, " ");
      int record_secs = atoi(tok);

      //what time do we want to update?
      global_time += ((double) clock()/CLOCKS_PER_SEC) - global_time;

      file_write_ptr=fopen("log.txt","a");
      fprintf(file_write_ptr, "Thread %d wants to update record %d at time %3.2f\n",(int)((double*)arg_ptr)[0], record_num, global_time);
      fclose(file_write_ptr);

      //Grab the lock for that record, update it, then unlock when done
      pthread_mutex_lock(&locks_array[record_num]);
      global_time += ((double) clock() / CLOCKS_PER_SEC) - global_time;

      file_write_ptr=fopen("log.txt","a");
      fprintf(file_write_ptr, "Thread %d starts to update record %d at time %3.2f\n",(int)((double*)arg_ptr)[0], record_num, global_time);
      fclose(file_write_ptr);

      global_time += ((double) clock() / CLOCKS_PER_SEC) - global_time;
      double start_updating = global_time;
      update(record_secs);
      global_time += ((double) clock() / CLOCKS_PER_SEC) - global_time;
      double end_updating = global_time;

      pthread_mutex_unlock(&locks_array[record_num]);
      
      file_write_ptr=fopen("log.txt","a");
      fprintf(file_write_ptr, "Thread %d is finished updating record %d at time %3.2f\n",(int)((double*)arg_ptr)[0], record_num, global_time);
      fclose(file_write_ptr);

      total_updating+=end_updating - start_updating;
    }
  while(feof(file_read_ptr)==0);

  total_running = ((double) clock() - total_running) / CLOCKS_PER_SEC;

  ((double*)arg_ptr)[1] = total_updating;
  ((double*)arg_ptr)[2] = total_running;  

  pthread_exit((double*) arg_ptr);
  return (void*) 0;
}


int main(int argc, char* argv[])
{
  if (error_checking(argc, argv) == 1)
    {
      return 1;
    }

  global_time = 0;

  int lock_index;
  for(lock_index = 0; lock_index <= 6; lock_index++)
    {  
      int made_mutex = pthread_mutex_init(&locks_array[lock_index],NULL);
      if (made_mutex != 0)
	{
	  printf("Error creating mutex.\n");
	  return 1;
	}
    }  
  
  pthread_t thread_list[atoi(argv[1])+1];
  int i;
  FILE * file_write_ptr;

  for (i = 1; i <= atoi(argv[1]); i++)
    {
      //thread number,time spent updating, total run time
      double* arg_ptr = malloc(sizeof(double[3]));
      arg_ptr[0]=i;
      arg_ptr[1]=0;
      arg_ptr[2]=0;

      int check_create = pthread_create(&thread_list[i], NULL, update_db, (void*) arg_ptr);
      if (check_create != 0) 
	{
	  printf("Error creating thread %d.\n",i);
	}
      else 
	{
	  file_write_ptr=fopen("log.txt","a");
	  fprintf(file_write_ptr,"Thread %d created.\n",i);
	  fclose(file_write_ptr);
	}
    }


  for (i = 1; i <= atoi(argv[1]); i++)
    {
      double* arg_ptr=malloc(sizeof(double[3]));
      pthread_join(thread_list[i], (void**)&arg_ptr);
      file_write_ptr=fopen("log.txt","a");
      fprintf(file_write_ptr,"Thread %d spent %3.2f time updating. Total time was %3.2f.\n", (int)arg_ptr[0], arg_ptr[1], arg_ptr[2]);
      fclose(file_write_ptr);
    }
  return 0;
}
