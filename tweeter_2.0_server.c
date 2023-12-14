#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<string.h>
#include<signal.h>
#include<sys/shm.h>
#include<unistd.h>
#include<sys/sem.h>

#define TWEET_SIZE 128;

key_t shared_mem_key, semafor_key;
int shared_mem_id, semafor_id;
int num_of_tweets;
int temp_id;
struct tweet{
    int like_counter;
    char message[64];
    char username[64];
    int is_posted;
};
struct sembuf sem_op;

struct tweet* arr_of_tweets;

union semun {
    int val;
    struct semid_ds *buf;
};
union semun arg;

void siginthandler(){

    printf("\n[Serwer]: dostalem SIGINT => koncze i sprzatam...");
    printf(" (odlaczenie: %s, usuniecie: %s)\n", (shmdt(arr_of_tweets) == 0) ?"OK":"blad shmdt", (shmctl(shared_mem_id, IPC_RMID, 0) == 0)?"OK":"blad shmctl");
    printf("Usuwam semafory...\n");
    if(semctl(semafor_id, 0, IPC_RMID) == -1){
        perror("Blad semctl");
    }
    exit(0);
    

}
void sigtstphandler(){

    int i = 0;
    printf("\n___________  Twitter 2.0:  ___________\n\n");
   
    //Czytamy wszystkie posty
        for(i = 0 ; i < num_of_tweets ; i++){
            
        
        //blokujemy semafor na ktorym aktualnie dzialalmy
            sem_op.sem_num = i;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0; 
            semop(semafor_id, &sem_op, 1);

            if(arr_of_tweets[0].is_posted == 0){
                sem_op.sem_num = i;
                sem_op.sem_op = 1;
                sem_op.sem_flg = 0; 
                semop(semafor_id, &sem_op, 1);
                printf("[Server]: Brak postow\n");
                break;
            }
            if(arr_of_tweets[i].is_posted == 1){
                printf("[%s]: %s[Polubienia: %d]\n", arr_of_tweets[i].username, arr_of_tweets[i].message, arr_of_tweets[i].like_counter);
                sem_op.sem_num = i;
                sem_op.sem_op = 1;
                sem_op.sem_flg = 0; 
                semop(semafor_id, &sem_op, 1);
            }
            else{
                sem_op.sem_num = i;
                sem_op.sem_op = 1;
                sem_op.sem_flg = 0; 
                semop(semafor_id, &sem_op, 1);
                break;
            }

    }
    /*
    printf("Odblokowujemy semafory...\n");
    // odblokowujemy semafory
    for(i = 0 ; i < num_of_tweets ; i++){
        sem_op.sem_num = i;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        if(semop(semafor_id, &sem_op, 1) == -1){
            perror("Blad semop: ");
        }
    }
    */
}

int main(int argc, char* argv[])
{
    signal(SIGINT, siginthandler);
    signal(SIGTSTP, sigtstphandler);

    num_of_tweets = atoi(argv[2]);

	struct shmid_ds buf;
    //arr_of_tweets =(struct tweet*) malloc(num_of_tweets*sizeof(struct tweet));
    //printf("malloc size: %ld\n", (num_of_tweets)*sizeof(struct tweet));

    shared_mem_key = ftok(argv[1], 1);
    printf("___________  Twitter 2.0:  ___________\n");

    if(shared_mem_key == -1){
        printf("Problem z generowaniem klucza\n");
        exit(1);
    }
    else{
        printf("[Serwer]: Generuje klucz na postawie pliku %s...\n[Serwer]: Ok(klucz: %d)\n",argv[1], shared_mem_key);
    }

    shared_mem_id = shmget(shared_mem_key, num_of_tweets*sizeof(struct tweet),0644 | IPC_CREAT | IPC_EXCL);
    if(shared_mem_id == -1){
        printf("Blad shmget");
        exit(1);
    }


    printf("[Server]: Otwieram segment pamieci wspolnej na %d wpisow po %ldb\n", num_of_tweets, sizeof(struct tweet));
    //printf("Ok(shared memory id :%d)\n",shared_mem_id);

    shmctl(shared_mem_id, IPC_STAT, &buf);
	printf(" OK (id: %d, rozmiar: %zub)\n", shared_mem_id, buf.shm_segsz);

    printf("[Server]: Dolaczam pamiec wspolna... \n");
    arr_of_tweets = (struct tweet*)shmat(shared_mem_id, (void*)0, 0 );
    if(arr_of_tweets== (struct tweet*)-1){
        printf("blad shmat");
        exit(1);
    }
    printf("Ok(adres: %lX)\n",(long int)arr_of_tweets);
    printf("[Serwer]: nacisnij Crtl^Z by wyswietlic stan serwisu\n");
    printf("[Serwer]: nacisnij Crtl^C by zakonczyc program\n");
    // KURWY SEMAFOROWSKIE :))))

    struct sembuf sem_op;

    semafor_key = ftok(argv[1], 'S');
    if(semafor_key == -1){
        perror("Blad ftok: ");
    }
    semafor_id = semget(semafor_key, num_of_tweets, 0666 | IPC_CREAT);
    if(semafor_id == -1){
        perror("Blad semget: ");
    }
    // ustawiamy wszytkie semafory na nie blokowanie pamieci;
    for(int i = 0 ; i < num_of_tweets ; i++){
        sem_op.sem_num = i;
        sem_op.sem_op = 1;
        sem_op.sem_flg = SEM_UNDO;
        semop(semafor_id, &sem_op, 1);
    }

    
    



    
    while(1)
    {
        
    }
    


}