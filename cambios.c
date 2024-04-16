#include <sys/ipc.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include "cambios.h"

#define NUM_HIJOS 32


//Estructura para mensajes
struct mensajes{
    long tipo;  
    int grupoActual;
    int grupoCambio;
    int cambio;
};

//Union para semaforos en Solaris
union semun {
    int             val;
    struct semid_ds *buf;
    ushort *array;
} unionSem;


//Struct global para guardar variables
struct glob {
    int buzon, id_memoria, semaforos, grupoCambio, grupoActual, posi;
    char *zonaMem;
    char identificador;
    pid_t Z, hijos[NUM_HIJOS];
    struct mensajes mensajes;
};
struct glob glob;


//Operaciones de semaforos
void semwait (int semid, int semW) {      
    struct sembuf sops;   
    int error;   

    sops.sem_num=semW;
    sops.sem_op=-1;
    sops.sem_flg=0;

    error = semop(semid, &sops, 1);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
        exit(1);
    }
}


//UTILIZADO PARA ESPERAR A QUE TERMINEN LOS CAMBIOS EN PROCESO
void semwait0 (int semid, int semW) {      
    struct sembuf sops;   
    int error;   

    sops.sem_num=semW;
    sops.sem_op=0;
    sops.sem_flg=0;

    error = semop(semid, &sops, 1);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
        exit(1);
    }
}

void semsignal (int semid, int semS) {
    struct sembuf sops;   
    int error;   
    
    sops.sem_num=semS;
    sops.sem_op=1;
    sops.sem_flg=0;

    error = semop(semid, &sops, 1);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
        exit(1);
    }
}

///////////////////////////////////////////////////////

void fin(int signum){
    int i;

    if (getpid() == glob.Z) {
    //ESPERAMOS POR SI HAY ALGUN CAMBIO EN PROCESO
    semwait0(glob.semaforos, 5);

    //comprobamos  si el semaforo 0 vale 0
    if (semctl(glob.semaforos, 0, GETVAL, unionSem) == 0) {
        semsignal(glob.semaforos, 0);
    }

    for (i = 0; i < NUM_HIJOS; i++) {
        if(glob.hijos[i] != -1) {
            kill(glob.hijos[i], SIGKILL);
            wait(NULL);  
            glob.hijos[i] = -1;    
        }          
    }

    finCambios();

    //ELIMINAMOS MECANISMOS IPC
    if (glob.buzon != -1)  {
        if (msgctl(glob.buzon, IPC_RMID, NULL) == -1) {
            perror("ERROR");
            exit(1);
        }
    }
    if (glob.semaforos != -1) {
        if (semctl(glob.semaforos, 0, IPC_RMID, unionSem) == -1) {
            perror("ERROR");
            exit(1);
            
        }
    }
        
    if (glob.zonaMem != NULL) {
        if (shmdt((char *) glob.zonaMem) == -1) {
            perror("ERROR");
            exit(1);
        }             
    }

    if (glob.id_memoria != -1) {
        if (shmctl(glob.id_memoria, IPC_RMID, NULL) == -1) {
            perror("ERROR");
            exit(1);
        }
    }   

    exit(1);
    }  
}




int main(int argc, char *argv[]){

    int vel, error, aux, tipo;
    int i, j = 0, k = 1;
    pid_t flag;

    glob.Z = getpid();

    signal(SIGINT, &fin);
    signal(SIGALRM, &fin);

    if (argc < 2 || argc > 2) {
        vel = 0;
        alarm(20);
    }
    else {
        vel = atoi(argv[1]);
        if(vel <= 0) {
            vel = 0;
            alarm(20);
        }
        else {
            alarm(30);
        }
    }

    //Inicializamos las variables del struct
    glob.buzon = -1;
    glob.id_memoria = -1;
    glob.semaforos = -1;
    glob.zonaMem = NULL;


    //Inicializamos el array de hijos
    for (i = 0; i < NUM_HIJOS; i++) {
        glob.hijos[i] = -1;
    }
   
    //Creamos los mecanismos IPC
    if ((glob.semaforos = semget(IPC_PRIVATE,6,IPC_CREAT | 0666)) == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    if ((glob.id_memoria = shmget(IPC_PRIVATE, sizeof(char)*88, IPC_CREAT | 0666)) == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    if ((glob.zonaMem = (char *) shmat(glob.id_memoria, (char *) 0, 0)) == NULL) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    if ((glob.buzon = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1){
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    //ASIGANACION INICIAL
    unionSem.val = 1;
    error = semctl(glob.semaforos, 1, SETVAL, unionSem);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    error = semctl(glob.semaforos, 2, SETVAL, unionSem);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    error = semctl(glob.semaforos, 3, SETVAL, unionSem);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    error = semctl(glob.semaforos, 4, SETVAL, unionSem);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }
    
    unionSem.val = 0; 
    error = semctl(glob.semaforos, 5, SETVAL, unionSem);
    if (error == -1) {
        perror("ERROR");
        kill(glob.Z, SIGINT);
    }

    semwait(glob.semaforos, 1);
    * ((int *) &(glob.zonaMem[84])) = 0;
    semsignal(glob.semaforos, 1);

    //Iniciamos los cambios  
    inicioCambios(vel, glob.semaforos, glob.zonaMem);      

    for (i = 0;i < 40; i++) {
        semwait(glob.semaforos, 2);
        if (i == 8 + (10*(k-1)) || i == 9 + (10*(k-1))) {
            semwait(glob.semaforos, 1);
            glob.zonaMem[i*2] = 32;     //ASIGNAMOS UN ESPACIO EN LOS HUECOS
            glob.zonaMem[i*2+1] = 5;   //ASIGNAMOS CUALQUIER COSA
            refrescar();        
            semsignal(glob.semaforos, 1);
            semsignal(glob.semaforos, 2);
            if (i == 9 + (10*(k-1))) {
                k++;
            }
            continue;
        }

        if ((glob.hijos[i-(2*(k-1))] = flag = fork()) == -1) {
            perror("ERROR");
            kill(glob.Z, SIGINT);
        }
        else if (flag == 0) {
            if (i < 4+(10*(k-1))) {
                semsignal(glob.semaforos, 5);
                glob.posi = i;
                glob.identificador = LETRAS[i-(6*(k-1))];
                
                semwait(glob.semaforos, 1);
                glob.zonaMem[i*2] = glob.identificador;
                glob.zonaMem[i*2+1] = k;    

                refrescar();       
                semsignal(glob.semaforos, 1);       
                semsignal(glob.semaforos, 2);   

                semwait(glob.semaforos, 5); 
                break;          
            }
            else {
                semsignal(glob.semaforos, 5);
                glob.posi = i;
                glob.identificador = LETRAS[i+12-(6*(k-1))];
                semwait(glob.semaforos, 1);
                glob.zonaMem[i*2] = glob.identificador;
                glob.zonaMem[i*2+1] = k;    

                refrescar();       
                semsignal(glob.semaforos, 1);       
                semsignal(glob.semaforos, 2);   

                semwait(glob.semaforos, 5); 
                break; 
            }
        }
    }

    //MASCARA PARA QUE LOS HIJOS NO ENTREN EN LA MANEJADORA
    if (getpid() != glob.Z) {
        sigset_t mask;
        sigfillset(&mask);
        sigdelset(&mask, SIGTERM);
        sigdelset(&mask, SIGKILL);
        error = sigprocmask(SIG_SETMASK, &mask, NULL); 
        if (error == -1) {
            perror("ERROR");
            kill(glob.Z, SIGINT);
        }
    }

    if (getpid() == glob.Z) {
        int tipoC;
        int gC1, gC2, gA1, gA2;

        for (i = 0; i < NUM_HIJOS; i++) {
            glob.mensajes.tipo = 1;
            error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), IPC_NOWAIT);
            if (error == -1) {
                perror("ERROR");
                kill(glob.Z, SIGINT);
            }
        }

        while (1) {
            error = msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 0, 0);
            if (error == -1) {
                perror("ERROR");
                kill(glob.Z, SIGINT);
            }

            gC1 = glob.mensajes.grupoCambio;
            gA1 = glob.mensajes.grupoActual;
            
            tipo = 100 + (gC1*10) + gA1;
            
            if (msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), tipo, IPC_NOWAIT) != -1) {
                gC2 = glob.mensajes.grupoCambio;
                gA2 = glob.mensajes.grupoActual;

                tipoC = 1000 + (gC1*10) + gA1;

                glob.mensajes.cambio = 1;
                glob.mensajes.tipo = tipoC;
                glob.mensajes.grupoCambio = gC1;
                error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 0);
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }

                tipoC = 1000 + (gC2*10) + gA2;

                glob.mensajes.cambio = 1;
                glob.mensajes.tipo = tipoC;
                glob.mensajes.grupoCambio = gC2;
                error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 0);
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }

                error = msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 50, 0);
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }
                error = msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 50, 0);
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }
            }
            else {
                tipoC = 1000 + (gC1*10) + gA1;
                glob.mensajes.cambio = 0;
                glob.mensajes.tipo = tipoC;
                error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), IPC_NOWAIT);
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }
            }        
        }
    }
    else {     
        error = msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 1, 0);
        if (error == -1) {
            perror("ERROR");
            kill(glob.Z, SIGINT);
        }

        while(1) {
            semwait(glob.semaforos, 3);
            glob.grupoActual = glob.zonaMem [glob.posi*2+1];
            semsignal(glob.semaforos, 3);
            glob.grupoCambio = aQuEGrupo(glob.grupoActual);

            if (glob.grupoCambio != glob.grupoActual && 4 >= glob.grupoCambio > 0 &&  4 >= glob.grupoActual > 0) {                
                
                //SOLICITUD DE CAMBIO
                glob.mensajes.tipo = 100 + (glob.grupoActual*10) + glob.grupoCambio;
                glob.mensajes.grupoActual = glob.grupoActual;
                glob.mensajes.grupoCambio = glob.grupoCambio;

                error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 0);     
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }                

                //ESPERAMOS LA CONFIRMACION    
                tipo = 1000 + (glob.grupoCambio*10) + glob.grupoActual;                                       
                error = msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), tipo, 0);
                glob.grupoCambio = glob.mensajes.grupoCambio;
                if (error == -1) {
                    perror("ERROR");
                    kill(glob.Z, SIGINT);
                }

                if (glob.mensajes.cambio == 1) {
                    
                    //COMPROBACION PARA QUE AMBOS PROCESOS VAYAN JUNTOS
                    glob.mensajes.tipo = 2000 + glob.grupoActual;
                    error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 0);
                    if (error == -1) {
                        perror("ERROR");
                        kill(glob.Z, SIGINT);
                    }
                    error = msgrcv(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 2000 + glob.grupoCambio, 0);
                    if (error == -1) {
                        perror("ERROR");
                        kill(glob.Z, SIGINT);
                    }

                    semsignal(glob.semaforos, 5);

                    semwait(glob.semaforos, 3);                   
                    for (i = (10*(glob.grupoCambio-1)); i < (10*glob.grupoCambio); i++) {

                        if (glob.zonaMem[i*2] == 32) {
                            glob.zonaMem[glob.posi*2] = 32;
                            glob.zonaMem[i*2] = glob.identificador;
                            glob.zonaMem[i*2+1] = glob.grupoCambio;
                        
                            glob.posi = i;

                            aux = glob.grupoActual;
                            glob.grupoActual = glob.grupoCambio;
                            glob.grupoCambio = aux;

                            incrementarCuenta();

                            aux = *((int *) &(glob.zonaMem[84]));
                            aux += 1;
                            *((int *) &(glob.zonaMem[84])) = aux;

                            refrescar();
                            break;
                        }
                    }
                    semsignal(glob.semaforos, 3); 

                    semwait(glob.semaforos, 5);              

                    glob.mensajes.tipo = 50;
                    error = msgsnd(glob.buzon, &glob.mensajes, sizeof(glob.mensajes)-sizeof(long), 0);
                    if (error == -1) {
                        perror("ERROR");
                        kill(glob.Z, SIGINT);
                    }                  
                }
            }                             

        }
    } 

    return 0;
}

