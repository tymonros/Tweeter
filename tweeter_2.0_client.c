#include<stdio.h>
#include<sys/types.h>
#include<string.h>
#include<sys/shm.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/sem.h>

union semun {
    int val;
    struct semid_ds *buf;
};
struct tweet{
    int like_counter;
    char message[64];
    char username[64];
    int is_posted;
}*my_tweet;
key_t shared_mem_key, semafor_key;
int shared_mem_id, semafor_id;
char buf[64];
int number;
int *num_of_tweets = &number;
int current_index = 0;
int temp_id;
struct tweet* arr_of_tweets;

int main(int argc, char* argv[]){
    union semun arg;
    char mode;
    int max_num_of_tweets = 0;
    int i = 0;

    struct shmid_ds shm_info;
    printf("Twitter 2.0 wita!\n");
    shared_mem_key = ftok(argv[1], 1);
    if(shared_mem_key == -1){
        printf("[Client]: problem z generowaniem klucza\n");
        exit(1);
    }
    shared_mem_id = shmget(shared_mem_key, 0, 0);
    
    if(shared_mem_id == -1){
        printf("[Client]: Porblem shmget\n");
    }
    
    
    if (shmctl(shared_mem_id, IPC_STAT, &shm_info) == -1) {
        perror("[Client]: shmctl");
        return 1;
    }


    arr_of_tweets = (struct tweet*)shmat(shared_mem_id, (void*)0, 0); // dolaczenie adresu pamieci wspolnej
    max_num_of_tweets = shm_info.shm_segsz/sizeof(struct tweet);
    
    if(arr_of_tweets == (struct tweet*)-1){
        printf("[Client]: Problem shmat\n");
    }

//Rozpoczecie dolaczania semaforow
    struct sembuf sem_op;

    semafor_key = ftok(argv[1], 'S');
    if(semafor_key == -1){
        perror("Blad ftok: ");
    }
    semafor_id = semget(semafor_key,max_num_of_tweets , 0);
    if(semafor_id == -1){
        perror("Blad semget: ");
    }
    
    //"Blokujemy" wszytkie tweety ktore bedziemy czytac
    for(i = 0 ; i < max_num_of_tweets ; i++){
        sem_op.sem_num = i;
        sem_op.sem_op = -1;
        sem_op.sem_flg = 0;
        semop(semafor_id, &sem_op, 1);
        if(arr_of_tweets[i].is_posted == 0 && semctl(semafor_id, i, GETVAL, arg) == 0){

            sem_op.sem_op = 1;

            semop(semafor_id, &sem_op, 1);
            break;
        }
        

    }
    
    current_index = i;


    

    printf("[Wolnych %d wpisow (na %d)]\n", max_num_of_tweets - current_index, max_num_of_tweets);
    
    //wyswietlanie wszystkich tweetow

    printf("[Client]: wszystkie zapostowane tweety: \n");
    for(i = 0 ; i < current_index ; i++){
        printf("%d: ",i+1);
        printf("%s", arr_of_tweets[i].message);
        printf("[Autor: %s, Polubienia: %d]\n", arr_of_tweets[i].username,arr_of_tweets[i].like_counter);
        //Zwalniam semafor z odczytanego tweeta
        
        sem_op.sem_num = i;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        semop(semafor_id, &sem_op, 1);
        
        
    }
    
    


    printf("[Client]: Wybierz akcje: (N)owy post, (L)ike: \n");
    scanf("%c", &mode);
    if(mode == 'L'){
        printf("[Client]: Podaj numer posta który chciałbys polubic: \n");
        scanf("%d", &current_index);
        current_index -=1;  // Ze wzgledow esteycznych wszystkie numery sa o 1 wieksze
        if(current_index < 0 || current_index > max_num_of_tweets){
            printf("Niepoprawny numer komunikatu\n");
        }
        else{
            // Blokujemy semafor miejsca w ktorym zwiekszamy licznkik
           
            sem_op.sem_num = current_index;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            semop(semafor_id, &sem_op, 1);
            //semafor locked
            arr_of_tweets[current_index].like_counter += 1;
            //semafor unclocked
            sem_op.sem_num = current_index;
            sem_op.sem_op = 1;
            sem_op.sem_flg = 0;
            semop(semafor_id, &sem_op, 1);
        }
            
        }
    
    else if(mode =='N'){
        if(max_num_of_tweets == current_index){  //Sprawdza czy jest miejsce na nowy komunikat
            printf("Brak miejsca w serwisie\n");
        }
        else{
            // Znajdujemy od aktualnego indeksu pierwszy niezablokowany semafor

            //blokujemy semfaor indeksu na ktory dopisujemy wiadomosc
            while(semctl(semafor_id, current_index, GETVAL, arg) == 0){
                current_index += 1;
            }
            if(max_num_of_tweets == current_index){  //Sprawdza czy jest miejsce na nowy komunikat
                printf("Brak miejsca w serwisie\n");
            }
            else{
                
                sem_op.sem_num = current_index;
                sem_op.sem_op = -1;
                sem_op.sem_flg = 0;
                semop(semafor_id, &sem_op, 1);
                
                //wpisujemy wiadomosc
                printf("[Client]: Co  ci chodzi po glowie: \n");
                int ile = read(0, arr_of_tweets[current_index].message,64);
                arr_of_tweets[current_index].message[ile-1] = '\0';
                arr_of_tweets[current_index].is_posted = 1;
                strcpy(arr_of_tweets[current_index].username, argv[2]);
                //odblokowujemy semafor
                printf("Odblokowujemy semafor na current_index");
                sem_op.sem_num = current_index;
                sem_op.sem_op = 1;
                sem_op.sem_flg = 0;
                semop(semafor_id, &sem_op, 1);
            }
        }
    }
    else{
        printf("[Client]: niepoprawny tryb\n");
    }
    printf("Dziekuje za skorzystanie z aplikacji Twitter 2.0\n");
    printf((shmdt(arr_of_tweets) == 0)?"":"blad shmdt");
    
    

   
    return 0 ;
    


}
