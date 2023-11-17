#include <stdio.h>
#include <unistd.h> // for getopt
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int thread_amount;
float time_wait; // -t

typedef struct {
    pthread_t thread_id;
    int thread_num; // -n
    int sched_policy; // -s, 0 for real-time, 1 for non-real-time
    int sched_priority; // -p
} thread_info_t;

int main(int argc, char *argv[]){
    int opt, count;
    char *token;
    thread_info_t thread_info[10];

    while(1){
        opt = getopt(argc, argv, "n:t:s:p:");

        if(opt == -1)
            break;
        else if(opt == '?'){
            printf("Invalid user arguments, option -%c\n", opt);
            return 1;
        } 

        switch(opt){
            case 'n':
                thread_amount = atoi(optarg);
                break;
            case 't':
                time_wait = (float)(atof(optarg));
                break;
            case 's':
                count = 0;
                //printf("%s\n", optarg);
                token = strtok(optarg, ",");
                do{
                    //printf("%s ", token);
                    switch (token[0])
                    {
                        case 'F':
                            thread_info[count].sched_policy = 0;
                            break;
                        case 'N':
                            thread_info[count].sched_policy = 1;
                            break;
                    }
                    count++;
                }while((token = strtok(NULL, ",")) != NULL);
                break;
            case 'p':
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

    /*printf("%d, %f\n", thread_amount, time_wait);
    for(count = 0; count < thread_amount; count++)
        printf("%d %d\n", thread_info[count].sched_policy, thread_info[count].sched_priority);*/

    return 0;
}