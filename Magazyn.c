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

//gcc -o dystrybutornia Dystrybutornia.c  Adam.c && ./dystrybutornia Kluczyk 100 5 3 4 

/*Aplikację uruchamiamy w terminalu za pomocą polecenia:
./gcc Magazyn.c Adam.c && ./a.out m1_konf.txt 50000
*/


#define TIME 0.5
#define SEMKEY 50002
#define BUFSIZE 100


//pamiec wspoldzielona do kurier - magazyn | trzy pola 0-1 i zliczanie ile kurierow zyje na koncu kazdej petli

int main(int argc , char * argv []){

    double CzasKuriera;



    int KEY = atoi(argv[2]);
    int msgid = msgget(KEY, IPC_CREAT|0640);
    if (msgid == -1){
        perror("Wczytanie kolejki");
        exit(1);
    }
    int msgid2 = msgget(KEY+10, IPC_CREAT|0640);
    if (msgid2 == -1){
        perror("Wczytanie kolejki 2");
        exit(1);
    }

    int semid = semget(KEY+30, 1, 0640);
    if (semid == -1){
        perror("Wczytanie semafora");
        exit(1);
    }

    int MagazynId = (int)(argv[1][1]-49);
    printf("MagazynId: %d\n", MagazynId);
    int f = open(argv[1], O_RDONLY);
    if (f == -1){
        perror("Otwarcie pliku konfiguracyjnego");
        exit(1);
    }
    char Buf[BUFSIZE];
    int HowManyBytes = read(f, Buf, BUFSIZE);
    close(f);
    int A, B, C, GLD;
    int A_Cost, B_Cost, C_Cost;
    long TotalProfit = 0;
    int i = 0, k = 0;
    for (int d = 0; d < 3; d++){
        k = 0;
        char Data[BUFSIZE] = {'\0'};
        char DataCost[BUFSIZE] = {'\0'};
        while (Buf[i] != 'x'){
            Data[k] = Buf[i];
            i++;
            k++;
        }
        i++;
        k = 0;
        if (Buf[i] == 'A'){
            A = atoi(Data);
            i+=2;
            while (Buf[i] != '\n'){
                DataCost[k] = Buf[i];
                i++;
                k++;
            }
            A_Cost = atoi(DataCost);
            printf("A: %d | Koszt: %d\n", A, A_Cost);

        } else if (Buf[i] == 'B'){
            B = atoi(Data);
            i+=2;
            while (Buf[i] != '\n'){
                DataCost[k] = Buf[i];
                i++;
                k++;
            }
            B_Cost = atoi(DataCost);
            printf("B: %d | Koszt: %d\n", B, B_Cost);

        } else if (Buf[i] == 'C'){
            C = atoi(Data);
            i+=2;
            while (Buf[i] != '\n' && i < HowManyBytes){
                DataCost[k] = Buf[i];
                i++;
                k++;
            }
            C_Cost = atoi(DataCost);
            printf("C: %d | Koszt: %d\n", C, C_Cost);
        }
        i++;
    }
    printf("Dane sczytano poprawnie\n");

   
    int MSGKEY = KEY+1+MagazynId;
    int msgid1 = msgget(MSGKEY, IPC_CREAT|0640);
    if (msgid == -1){
        perror("Utworzenie kolejki komunikatow dla przesylania zamowien");
        exit(1);
    }



    
    int DystrybutorPid;
    int MagazynPid = getpid();
    printf("Magazyn: %d\n", MagazynPid);
    
    for (int i = 0; i < 3; i++){
        if (!fork()){

            printf("Kurier %d | %d z magazynu %d\n", i, getpid(), getppid());
            time_t start = time(NULL);
            int blokada = 0;
            //petla kuriera -------------------------------
            while (true){
                //odbieranie zamowienia
                
                if (blokada == 0 &&  SemDown(semid, 0) == 0){
                    
                     struct OrderData RcvData;
                    
                     msgrcv(msgid, &RcvData, sizeof(RcvData)-sizeof(long), 10, 0);
                     if (RcvData.OrderNumber != -1){
           
                        printf("Kurier %d | Zamowienie: %d | Odebrano: %d, %d, %d\n", i, RcvData.OrderNumber+1, RcvData.A, RcvData.B, RcvData.C);
                        start = time(NULL);
                        
                        RcvData.OrderNumber = i+1; //id kuriera bedzie pod ordernumber
                        
                        msgsnd(msgid1, &RcvData, sizeof(RcvData)- sizeof(long), 0);
                        msgrcv(msgid1, &RcvData, sizeof(RcvData)-sizeof(long), i+1, 0);
                        RcvData.p = 11;
                        
                        if (RcvData.A == -1){
                            
                                                    
                            msgsnd(msgid2, &RcvData, sizeof(RcvData)-sizeof(long), 0);
                            printf("[Kurier] Kurier %d zostaje zabity przez braki w magazynie\n", i);
                
                            SemUp(semid, 0);
                            exit(0);
                        }
                        else {                          
                            msgsnd(msgid2, &RcvData, sizeof(RcvData)-sizeof(long), 0);        
                            //printf("Wyslano %d\n", RcvData.A);
                            SemUp(semid, 0);
                            
                        }
                        }
                        else {
                            blokada = 1;
                            SemUp(semid, 0);
                            //printf("Wykryto -1\n");
                        }
                    
                } else {

                
                    double CzasKuriera = difftime(time(NULL), start);
                    if (CzasKuriera >= 150){
                        printf("[Kurier] Timeout kuriera %d\n", i);
                        
                        struct OrderData KillMsg;
                        KillMsg.A = KillMsg.B = KillMsg.C = -1;
                        KillMsg.p = 10;
                        msgsnd(msgid1, &KillMsg, sizeof(KillMsg) - sizeof(long), 0);
                        KillMsg.p = 11;
                        msgsnd(msgid2, &KillMsg, sizeof(KillMsg) - sizeof(long), 0);

                        exit(0);
                    }
                }
                

            }
           
        } 


        
        
    }
   


    int Kurierzy = 3;
    
    while (Kurierzy > 0){
        //petla magazynu
        

        static struct OrderData Data;
        
        //printf("Przed\n");
        
        msgrcv(msgid1, &Data, sizeof(Data) - sizeof(long), 10, 0);
        //printf("Po\n");

        
        int kurier = Data.OrderNumber-1;
        Data.p = Data.OrderNumber;
        
        if (Data.A == -1 && Data.B == -1 && Data.C == -1){
            Kurierzy--;
            //printf("[Magazyn] kurier sie zabil\n");
        }
        else {
            if (A - Data.A >= 0 && B - Data.B >= 0 && C - Data.C >= 0){
                //Mozna przyjac zamowienie
                A -= Data.A;
                B -= Data.B;
                C -= Data.C;
                int Cost = Data.A * A_Cost + Data.B * B_Cost + Data.C * C_Cost;
                TotalProfit += Cost; 
                Data.A = Data.B = Data.C = Cost;
  
            
            } else {
                //nie mozna przyjac zamowienia, niech kurier sie zabije     
    
                Data.A = Data.B = Data.C = -1;
                
                Kurierzy--;
            }

            msgsnd(msgid1, &Data, sizeof(Data) - sizeof(long), 0);

        }
        
        //printf("Kurierzy %d\n", Kurierzy);
        usleep(200000);
    }

    
    sleep(4);

    printf("Przychod magazynu %ld | Zawartosc: A: %d | B: %d | C: %d\n", TotalProfit, A, B, C);
    
    
    /*char Buff[20];
    read(0, Buff, 20);*/

   
    return 0;
}