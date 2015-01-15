#include <stdio.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define FILENAME "matrices_default"

typedef struct{
	int shmid;
	int m, n;
	float **coef;
}matriz;

matriz *crearMatriz(int m, int n){
	int shmid, i;
	matriz *a;

	shmid = shmget (IPC_PRIVATE, sizeof(matriz) + m * sizeof(float), 0600);

	if(shmid == -1){
		perror("crearMatriz");
		exit(-1);
	}

	if( (a = (matriz *)shmat (shmid, 0, 0)) == (matriz *)-1){
		perror("crearMatriz");
		exit(-1);
	}

	a->shmid = shmid;
	a->m = m;
	a->n = n;

	a->coef = (float **)&a->coef + sizeof(float **);
	for(i = 0; i < m; i++)
		a->coef[i] = (float *)&a->coef[m] + i*n*sizeof(float);

	return a;
}

matriz *cargarMatriz(FILE *fp){
	int i, j, m, n;

	fscanf(fp, "%d %d", &m, &n);

	matriz *a = crearMatriz(m, n);

	for(i = 0; i < m; i++)
		for(j = 0; j < n; j++)
			fscanf(fp, "%f", &a->coef[i][j]);

	return a;
}

void imprimirMatriz(matriz *m){
	int i, j;

	for(i = 0; i < m->m; i++){
		for(j = 0; j < m->n; j++)
			printf("%g ", m->coef[i][j]);
		printf("\n");
	}
	printf("\n");
}

matriz *multiplicarMatrices(matriz *a, matriz *b){
	int p, semid, hijos;
	matriz *c;

	if(a->n != b->m){
		printf("NEL. No es una multiplicación válida\n");
		exit(-1);
	}

	hijos = a->m * b->n;

	c = crearMatriz(a->m, b->n);

	semid = semget(IPC_PRIVATE, 3, 0600);
	if(semid == -1){
		perror("multiplicarMatrices");
		exit(-1);
	}

	/*  
		Dos semáforos para los señores coeficientes 
		en estructuras flotantes,
		uno para gobernarlos a todos.
		un semaforo para encontrarlos.
		un semaforo para atraerlos a todos y procesarlos
		en la Región Crítica donde se extienden las sombras.

	*/

	semctl(semid, 0, SETVAL, 1);
	semctl(semid, 1, SETVAL, 0);
	semctl(semid, 2, SETVAL, c->n);

	printf("procesos: %d\n", hijos);
	printf("filas: %d\n", c->m);
	printf("columnas: %d\n", c->n);

	for(p = 0; p < hijos; p++){
		if(fork() == 0){
			int i, j, k;
            struct sembuf dijkstra_dice;
 
            dijkstra_dice.sem_flg = SEM_UNDO;
 
            while(1){
                dijkstra_dice.sem_num = 0;
                dijkstra_dice.sem_op = -1;
                semop(semid, &dijkstra_dice, 1);
 
                i = semctl(semid, 1, GETVAL, 0);
                if(i < c->m){           
	                j = semctl(semid, 2, GETVAL, 0);
	                if(j > 0)
	                	semctl(semid, 2, SETVAL, --j);
	                else{
	                	semctl(semid, 1, SETVAL, ++i);
						semctl(semid, 2, SETVAL, c->n);                	
	                }
	            }
	            else
	            	exit(-1);

				c->coef[i][j] = 0;
                for(k = 0; k < a->n; k++)
                	c->coef[i][j] += a->coef[i][k] * b->coef[k][j];

                dijkstra_dice.sem_num = 0;
                dijkstra_dice.sem_op = 1;
                semop(semid, &dijkstra_dice, 1);
            }
		}
	}

		int estado;
		for(p = 0; p < hijos; p++)
			wait(&estado);

		semctl(semid, 0, IPC_RMID, 0);
		return c;
}

void escribirMatriz(matriz *a, FILE *fp){
	int i, j;

	fprintf(fp, "\n\n%d %d\n", a->m, a->n);

	for(i = 0; i < a->m; i++){
		for(j = 0; j < a->n; j++)
			fprintf(fp, "%g ", a->coef[i][j]);
		fprintf(fp, "\n");
	}
}

void destruyeMatriz(matriz *a){
	shmctl(a->shmid, IPC_RMID, 0);
}

int main(int argc, char * argv[]){
	FILE * fp;
	char * fileString;

	if(argc > 1)
		fileString = argv[1];
	else
		fileString = FILENAME;

	fp = fopen(fileString, "r+");
	printf("Leyendo de archivo '%s'...\n", fileString);

	printf("Cargando matrices en memoria compartida...\n");
	matriz *a = cargarMatriz(fp);
	printf("Matriz A: \n");
	imprimirMatriz(a);

	printf("Matriz B: \n");
	matriz *b = cargarMatriz(fp);
	imprimirMatriz(b);


	matriz *c = multiplicarMatrices(a,b);
	printf("Multiplicación exitosa. Escribiendo resultado en archivo '%s'...\n", fileString);
	escribirMatriz(c, fp);
	printf("Matriz AB: \n");
	imprimirMatriz(c);

	printf("Eliminando matrices de memoria compartida...\n");
	destruyeMatriz(a);
	destruyeMatriz(b);
	destruyeMatriz(c);

	fclose(fp);

	return 0;
}
