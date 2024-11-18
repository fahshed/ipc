#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <cstring>
#include <time.h>

#include <chrono>
#include <thread>

using namespace std;


#define number_of_cycles 10
#define S 3
#define C 2
#define rand_range 3

pthread_mutex_t service_rooms[S];

int departure_count = 0;
pthread_mutex_t departure_count_write;
pthread_mutex_t entry;

int in_path_count = 0;
pthread_mutex_t in_path_count_write;
pthread_mutex_t clear_path;

sem_t empty_room;

int service_time[S];
int payment_time[number_of_cycles];
int departure_time[number_of_cycles];


void visit_servicemen(char* id)
{
    int id_integer = atoi(id);

    for(int i=0; i<S; i++)
    {
        //Waiting to enter station
        pthread_mutex_lock(&service_rooms[i]);

        if(i==0)
        {
            pthread_mutex_lock(&departure_count_write);
            if(departure_count>0)
            {
                pthread_mutex_unlock(&departure_count_write);

                //Departure priority
                pthread_mutex_lock(&entry);
                printf("%s started taking service from serviceman %d\n", id, i+1);
                pthread_mutex_unlock(&entry);
            }
            else
            {
                printf("%s started taking service from serviceman %d\n", id, i+1);
                pthread_mutex_unlock(&departure_count_write);
            }

            //Locking path
            pthread_mutex_lock(&in_path_count_write);
            in_path_count++;
            if(in_path_count==1) pthread_mutex_lock(&clear_path);
            pthread_mutex_unlock(&in_path_count_write);
        }
        else
        {
            printf("%s started taking service from serviceman %d\n", id, i+1);
            pthread_mutex_unlock(&service_rooms[i-1]);
        }
        sleep(service_time[i]);
        //this_thread::sleep_for(chrono::milliseconds(service_time[i]));
        printf("%s finished taking service from serviceman %d\n", id, i+1);
    }
}

void payment(char* id)
{
    int id_integer = atoi(id);

    //Releasing Path
    pthread_mutex_lock(&in_path_count_write);
    in_path_count--;
    if(in_path_count==0) pthread_mutex_unlock(&clear_path);
    pthread_mutex_unlock(&in_path_count_write);

    pthread_mutex_unlock(&service_rooms[S-1]);

    sem_wait(&empty_room);

    printf("%s started paying the service bill\n", id);
    sleep(payment_time[id_integer]);
    //this_thread::sleep_for(chrono::milliseconds(payment_time[id_integer]));

    //Blocking Entry
    pthread_mutex_lock(&departure_count_write);
    departure_count++;
    if(departure_count==1) pthread_mutex_lock(&entry);
    pthread_mutex_unlock(&departure_count_write);

    printf("%s finished paying the service bill\n", id);

    sem_post(&empty_room);
}

void depart(char* id)
{
    int id_integer = atoi(id);

    pthread_mutex_lock(&in_path_count_write);
    if(in_path_count>0)
    {
        pthread_mutex_unlock(&in_path_count_write);

        //Waiting on clear path
        pthread_mutex_lock(&clear_path);
        pthread_mutex_unlock(&clear_path);
    }
    else
    {
        pthread_mutex_unlock(&in_path_count_write);
    }

    sleep(departure_time[id_integer]);
    //this_thread::sleep_for(chrono::milliseconds(departure_time[id_integer]));
    printf("%s has departed\n", id);

    //Allowing entry
    pthread_mutex_lock(&departure_count_write);
    departure_count--;
    if(departure_count==0) pthread_mutex_unlock(&entry);
    pthread_mutex_unlock(&departure_count_write);
}

void* repair(void* arg)
{
    char* id = new char[3];
    id = (char*)arg;

    visit_servicemen(id);
    payment(id);
    depart(id);
}



int main()
{
    int res;
    srand(time(0));

    //Initialization check begins
    res = sem_init(&empty_room, 0, C);
    if(res != 0)
    {
        printf("full_room initialization failed\n");
    }

    for(int i=0; i<S; i++)
    {
        res = pthread_mutex_init(&service_rooms[i], NULL);
        if(res != 0)
        {
            printf("%d th service_room initialization failed\n", i+1);
        }

        service_time[i] = rand()%rand_range + 1;
    }

    for(int i=0; i<number_of_cycles; i++)
    {
        payment_time[i] = rand()%rand_range + 1;
        departure_time[i] = rand()%rand_range + 1;
    }

    res = pthread_mutex_init(&entry, NULL);
    if(res != 0)
    {
        printf("entry initialization failed\n");
    }

    res = pthread_mutex_init(&departure_count_write, NULL);
    if(res != 0)
    {
        printf("departure_count_write initialization failed\n");
    }

    res = pthread_mutex_init(&in_path_count_write, NULL);
    if(res != 0)
    {
        printf("in_path_write initialization failed\n");
    }

    res = pthread_mutex_init(&clear_path, NULL);
    if(res != 0)
    {
        printf("clear_path initialization failed\n");
    }
    //Initialization check ends


    pthread_t cycles[number_of_cycles];

    for(int i=0; i<number_of_cycles; i++)
    {
        char* id = new char[3];
        strcpy(id, to_string(i+1).c_str());

        res = pthread_create(&cycles[i], NULL, repair, id);

        if(res != 0)
        {
            printf("Failed to create %d th cyclist\n", i+1);
        }
    }


    for(int i=0; i<number_of_cycles; i++)
    {
        void* result;

        pthread_join(cycles[i], &result);
    }


    //Destruction check begins
    res = sem_destroy(&empty_room);
    if(res != 0)
    {
        printf("empty_room destroy failed\n");
    }

    for(int i=0; i<S; i++)
    {
        res = pthread_mutex_destroy(&service_rooms[i]);
        if(res != 0)
        {
            printf("%d th service_room destroy failed\n", i);
        }
    }

    res = pthread_mutex_destroy(&entry);
    if(res != 0)
    {
        printf("entry destroy failed\n");
    }

    res = pthread_mutex_destroy(&departure_count_write);
    if(res != 0)
    {
        printf("departure_count_write destroy failed\n");
    }

    res = pthread_mutex_destroy(&in_path_count_write);
    if(res != 0)
    {
        printf("in_path_write initialization failed\n");
    }

    res = pthread_mutex_init(&clear_path, NULL);
    if(res != 0)
    {
        printf("clear_path initialization failed\n");
    }
    //Destruction check ends

    return 0;
}
