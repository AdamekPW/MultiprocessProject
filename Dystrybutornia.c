#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include "Adam.h"

//gcc -o dystrybutornia Dystrybutornia.c  Adam.c && ./dystrybutornia 50000 100 5 3 4 

/*Aplikację uruchamiamy w terminalu za pomocą polecenia:
./dystrybutornia <klucz> <liczba_zamówień> <max_A_per_zam.> <max_B_per_zam.> <max_C_per_zam.>
*/


#define TIME 0.5

int GLD = 0;


int main(int argc , char * argv []){
    srand(time(NULL));


    if (argc != 6){
        printf("Niepoprawna liczba danych wejsciowych\n");
        exit(1);
    }
    const char* _KEY = argv[1];
    const char* _ORDER_NUMBER = argv[2];
    const char* _MAX_PER_A = argv[3];
    const char* _MAX_PER_B = argv[4];
    const char* _MAX_PER_C = argv[5];

    int KEY = atoi(_KEY);
    int ORDER_NUMBER = atoi(_ORDER_NUMBER);
    int MAX_PER_A = atoi(_MAX_PER_A);
    int MAX_PER_B = atoi(_MAX_PER_B);
    int MAX_PER_C = atoi(_MAX_PER_C);

    

    if (MAX_PER_A < 0 || MAX_PER_B < 0 || MAX_PER_C < 0 || ORDER_NUMBER <= 0) {
        printf("Niewlasciwe dane (cos jest ujemne)\n");
        exit(1);
    }
    printf("Dane dystrybutorni: \nKlucz komunikacji: %d\nLiczba zamowien: %d\n", KEY, ORDER_NUMBER);
    printf("Max dla A: %d\nMax dla B: %d\nMax dla C: %d\n", MAX_PER_A, MAX_PER_B, MAX_PER_C);

    int msgid = msgget(KEY, IPC_CREAT|0640);
    if (msgid == -1){
        perror("Utworzenie kolejki komunikatow dla przesylania zamowien");
        exit(1);
    }
    int msgid2 = msgget(KEY+10, IPC_CREAT|0640);
    if (msgid2 == -1){
        perror("Utworzenie kolejki komunikatow do pobierania ceny od kurierow");
    }

    int semid = semget(KEY+30, 1, IPC_CREAT|0640);
    if (semid == -1){
        perror("Utworzenie semafora");
        exit(1);
    }
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("Ustawianie wartosci poczatkowej semafora");
        exit(1);
    }
    
    int Kurierzy = 9;
    int IleStopow = Kurierzy;

    sleep(2);
    int i = 0;

    while (Kurierzy > 0){
        if (ORDER_NUMBER > 0){
            int A = RandomNumber(MAX_PER_A);
            int B = RandomNumber(MAX_PER_B);
            int C = RandomNumber(MAX_PER_C);
            
            printf("Zamowienie %d: <%d, %d, %d>\n", i+1, A, B, C);
            struct OrderData DataToSend;
            DataToSend.p = 10;
            DataToSend.A = A;
            DataToSend.B = B;
            DataToSend.C = C;
            DataToSend.OrderNumber = i;
            DataToSend.DystrybutorPid = getpid();
            
            msgsnd(msgid, &DataToSend, sizeof(DataToSend) - sizeof(long), 0);
            ORDER_NUMBER--;
            i++;
        } else if (IleStopow >= 0){
            static struct OrderData KillMsg;
            KillMsg.OrderNumber = -1;
            KillMsg.p = 10;
            msgsnd(msgid, &KillMsg, sizeof(KillMsg) - sizeof(long), 0); 
            IleStopow--;
        }
        
        
        static struct OrderData RcvData;
        msgrcv(msgid2, &RcvData, sizeof(RcvData)-sizeof(long), 11, 0);
        //printf("%d | %d | %d\n", RcvData.A, RcvData.B, RcvData.C);
        if (RcvData.A == -1){
            Kurierzy--;
            //printf("Kurier umarl\n");
        }
        else {
            GLD += RcvData.A;
            //printf("GLD: %d\n", RcvData.A);
        }
        

        
        usleep(TIME*1000000);
    }
    printf("GLD koncowy: %d\n", GLD);

    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Usuwanie kolejki");
        exit(-1);
    }

    return 0;
}