#define _GNU_SOURCE // for CPU_ZERO, SET and others. https://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply
#include <stdio.h>
#include <unistd.h> // for getopt
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h> // for CPU scheduling
#include <errno.h>
#include <time.h>

int thread_amount; // record thread amotb
float time_wait; // -t
int CPU_ID = 0; // which cpu threads run on
cpu_set_t cpuset; // to get the PU info

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier; // set barrier that all threads are complete can they start execute

typedef struct {
    pthread_t thread_id; //pid of thread
    int thread_num; // -n
    int sched_policy; // -s, 0 for real-time, 1 for non-real-time
    int sched_priority; // -p
} thread_info_t;

/*typedef struct // we only need priority when using 'pthread_attr_setschedparam'
{
    int32_t  sched_priority;
    int32_t  sched_curpriority;
    union
    {
        int32_t  reserved[8];
        struct
        {   
            int32_t  __ss_low_priority; 
            int32_t  __ss_max_repl; 
            struct timespec     __ss_repl_period;  
            struct timespec     __ss_init_budget;  
        }           __ss;  
    }           __ss_un;   
}sched_param;*/

void *thread_func(void *arg);

int main(int argc, char *argv[]){
    int opt, count;
    char *token; // for user parameter parsing
    thread_info_t *thread_info; // thread_info array
    pthread_attr_t *thread_attr; //thread attribute array, to record scheduling policy and priority
    struct sched_param *sched_params; // store scheduling parameters

    CPU_ZERO(&cpuset); // clear CPU set
    CPU_SET(CPU_ID, &cpuset); // add CPU into CPU set

    if(sched_setaffinity(CPU_ID, sizeof(cpuset), &cpuset)){ //let main thread to run on CPU0
        printf("Faild to set main thread CPU affinity");
        return EXIT_FAILURE;
    }


    //printf("Main thread runs on CPU %d\n", sched_getaffinity(0, sizeof(cpuset), &cpuset));

    while(1){ // parse user argements
        opt = getopt(argc, argv, "n:t:s:p:"); // gormat: -n ... -t ... -s ... -p ...

        if(opt == -1) // no more argumnets
            break;
        else if(opt == '?'){ // error argument
            printf("Invalid user arguments, option -%c\n", opt);
            return 1;
        } 

        switch(opt){
            case 'n': // get total amount of threads
                thread_amount = atoi(optarg);
                thread_info = (thread_info_t*)malloc(thread_amount * sizeof(thread_info_t));
                sched_params = (struct sched_param*)malloc(thread_amount * sizeof(struct sched_param));
                thread_attr = (pthread_attr_t*)malloc(thread_amount * sizeof(pthread_attr_t));
                break;
            case 't': // get the duration of busy waiting
                time_wait = (float)(atof(optarg));
                break;
            case 's': // get policy
                count = 0;
                //printf("%s\n", optarg);
                token = strtok(optarg, ",");
                do{
                    //printf("%s ", token);
                    switch (token[0]) // argument may be 'FIFO' or 'NORMAL'
                    {
                        case 'F':
                            thread_info[count].sched_policy = 1;
                            break;
                        case 'N':
                            thread_info[count].sched_policy = 0;
                            break;
                    }
                    count++;
                }while((token = strtok(NULL, ",")) != NULL);
                break;
            case 'p': // get priority
                count = 0;
                //printf("%s\n", optarg);
                token = strtok(optarg, ",");
                do{
                    thread_info[count].sched_priority = atoi(token);
                    count++;
                }while((token = strtok(NULL, ",")) != NULL);
                break;
        }
    }

    pthread_barrier_init(&barrier, NULL, thread_amount); // initialize barrier to amounts of all threads
    //printf("min:%d, max:%d\n", sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));

    for(count = 0; count < thread_amount; count++){
        thread_info[count].thread_num = count; //set thread number
        pthread_attr_init(&thread_attr[count]); // initialize pthread attribute
        sched_params[count].sched_priority = thread_info[count].sched_priority;
        
        if(pthread_attr_setinheritsched(&thread_attr[count], PTHREAD_EXPLICIT_SCHED)){ // don't inherit parent thread's scheduling policy
            printf("Error while setting inheritance\n");
            return EXIT_FAILURE;
        }

        switch(thread_info[count].sched_policy){
            case 1:// FIFO
                if(pthread_attr_setschedpolicy(&thread_attr[count], SCHED_FIFO)){ // set policy to FIFO
                    printf("Error while setting policy\n");
                    return EXIT_FAILURE;
                }

                if(sched_params[count].sched_priority >= sched_get_priority_min(SCHED_FIFO) && sched_params[count].sched_priority <= sched_get_priority_max(SCHED_FIFO)){ // make sure thr priority is legal
                    if(pthread_attr_setschedparam(&thread_attr[count], &sched_params[count])){ // set scheduling priority
                        printf("Error while setting priority\n");
                        return EXIT_FAILURE;
                    }
                }
                else{
                    printf("Priority value illegal\n");
                    return EXIT_FAILURE;
                }

                if(pthread_create(&thread_info[count].thread_id, &thread_attr[count], thread_func, (void*)&thread_info[count].thread_num)){ // create threads with assigned scheduling policy and priority
                    printf("Error while creating pthread\n");
                    printf("%d\n", sched_params[count].sched_priority);
                    perror("pthread_create");
                    return EXIT_FAILURE;
                }
                break;
            case 0: // not real-time, no need to set priority
                if(pthread_attr_setschedpolicy(&thread_attr[count], SCHED_OTHER)){ // set policy to normal
                    printf("Error while setting policy\n");
                    return EXIT_FAILURE;
                }
                if(pthread_create(&thread_info[count].thread_id, &thread_attr[count], thread_func, (void*)&thread_info[count].thread_num)){ // create threads with assigned scheduling policy
                    printf("Error while creating pthread\n");
                    return EXIT_FAILURE;
                }
                break;
        }
    } 

    for(count = 0; count < thread_amount; count++) // wait until all threads completed
        pthread_join(thread_info[count].thread_id, NULL);

    pthread_barrier_destroy(&barrier); // destroy barrier, this is unnecessary in this program

    return EXIT_SUCCESS;
}

void *thread_func(void *arg)
{
    struct timespec t1,t2; // store time in second and time in nanosecond
    double ts1, ts2; // store time in second with nanosecond in it

    if(pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset)) // set thread to run on CPU0
        printf("Faild to set thread%d CPU affinity" ,*((int*) arg));
    /* 1. Wait until all threads are ready */
    pthread_barrier_wait(&barrier);
    /* 2. Do the task */ 
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", *((int*) arg));
        /* Busy for <time_wait> seconds */
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t1); // get the first amount of CPU time consumed by the thread
        ts1 = t1.tv_sec + 1e-9 * t1.tv_nsec; // get accurate time in second with floating point form
        while(1){ // busy working
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t2); // get the second amount of CPU time consumed by the thread
            ts2 = t2.tv_sec + 1e-9 * t2.tv_nsec; 
            if((ts2 - ts1) >= time_wait) // if the time gap exceed time_wait, end busy working
                break;
        }
    }
   
    //printf("Thread %d runs on CPU %d\n",*((int*) arg), pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset));
    /* 3. Exit the function  */
    pthread_exit(NULL);
}