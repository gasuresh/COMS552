#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>

typedef struct {
    int nreaders;
    sem_t mutex;
    sem_t fmutex;
    sem_t wmutex;
} shared_data;


void reader_process(int id, shared_data *data) {
    printf("Process %d (reader) arrives.\n", id);
    
    sem_wait(&data->mutex);

    if (data->nreaders == 0) {
        data->nreaders++;
        sem_wait(&data->fmutex);
    } else {
        data->nreaders++;
    }

    sem_post(&data->mutex);

    printf("Process %d starts reading.\n", id);
    
    sleep(2);
    printf("Process %d ends reading.\n", id);
    
    sem_wait(&data->mutex);
    data->nreaders--;

    if (data->nreaders == 0) {
        sem_post(&data->fmutex);
    }

    sem_post(&data->mutex);

    printf("Process %d (reader) leaves.\n", id);
    
}

void writer_process(int id, shared_data *data) {

    printf("Process %d (writer) arrives.\n", id);

    sem_wait(&data->wmutex);
    sem_wait(&data->fmutex);

    printf("Process %d starts writing.\n", id);
    
    sleep(2);
    printf("Process %d ends writing.\n", id);
    
    sem_post(&data->fmutex);
    sem_post(&data->wmutex);

    printf("Process %d (writer) leaves.\n", id);
    
}


int main(int argc, char *argv[])
{
    char* sequence = argv[1];
    int n = strlen(sequence);

    int shmid = shmget(IPC_PRIVATE, sizeof(shared_data), IPC_CREAT | 0666);

    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }

    shared_data *data = (shared_data *)shmat(shmid, NULL, 0);
    if (data == (shared_data *)-1) {
        perror("shmat");
        exit(1);
    }

    data->nreaders = 0;

    sem_init(&data->mutex, 1, 1);
    sem_init(&data->fmutex, 1, 1);
    sem_init(&data->wmutex, 1, 1);


    pid_t pids[n];

    for (int i = 0; i < n; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            if (sequence[i] == 'r') {
                reader_process(i, data);
            } else {
                writer_process(i, data);
            }

            exit(0);
        } else {
            pids[i] = pid;
        }

        sleep(1);
        
    }

    for (int i = 0; i < n; i++) {
        waitpid(pids[i], NULL, 0);
    }

    sem_destroy(&data->mutex);
    sem_destroy(&data->fmutex);
    sem_destroy(&data->wmutex);

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}