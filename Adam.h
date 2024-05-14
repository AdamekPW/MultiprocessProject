#ifndef ADAM_H 
#define ADAM_H

#define true 1


struct OrderData {
    long p;
    int A, B, C;
    int OrderNumber;
    int DystrybutorPid;
};

int RandomNumber(int max);
int SemUp(int semid, int semnum);
int SemDown(int semid, int semnum);

#endif