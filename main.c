#include <stdio.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define FILENAME "matrices"

typedef struct{
	int shmid;
	int filas, columnas;
	float **coef;
}matriz;

matriz *crear_matriz(int filas, int columnas){
	int shmid, i;
	matriz *m;

	shmid = shmget (IPC_PRIVATE, sizeof(matriz) + filas * sizeof(float), 0600);

	if(shmid == -1){
		perror("crear_matriz");
		exit(-1);
	}

	if( (m = (matriz *)shmat (shmid, 0, 0)) == (matriz *)-1){
		perror("crear_matriz");
		exit(-1);
	}

	m->shmid = shmid;
	m->filas = filas;
	m->columnas = columnas;

	m->coef = (float **)&m->coef + sizeof(float **);
	for(i = 0; i < filas; i++)
		m->coef[i] = (float *)&m->coef[filas] + i*columnas*sizeof(float);

	return m;
}

matriz *leer_matriz(FILE *fp){
	int i, j, m, n;

	fscanf(fp, "%d %d", &m, &n);

	matriz *a = crear_matriz(m, n);

	for(i = 0; i < m; i++)
		for(j = 0; j < n; j++)
			fscanf(fp, "%f", &a->coef[i][j]);

	return a;
}

void imprimir_matriz(matriz *m){
	int i, j;

	for(i = 0; i < m->filas; i++){
		for(j = 0; j < m->columnas; j++)
			printf("%g", m->coef[i][j]);
		printf("\n");
	}
}

matriz *multiplicar_matrices(matriz *a, matriz *b){
	int p, semid, estado, hijos;
	matriz *c;

	printf("%d,%d\n",a->columnas,b->filas);

	if(a->columnas != b->filas){
		printf("NEL. No es una multiplicación válida\n");
		return NULL;
	}

	hijos = a->filas * b->columnas;

	c = crear_matriz(a->filas, b->columnas);

	semid = semget(IPC_PRIVATE, 3, 0600);
	if(semid == -1){
		perror("multiplicar_matrices");
		exit(-1);
	}

	semctl(semid, 0, SETVAL, 1);
	semctl(semid, 1, SETVAL, c->filas);
	semctl(semid, 2, SETVAL, c->columnas);

	for(p = 0; p < hijos; p++){
		if(fork() == 0){
			

				}
			}
		}

		for(p = 0; p < hijos; p++)
			wait(&estado);

		semctl(semid, 0, IPC_RMID, 0);
		return c;
}

void escribir_matriz(matriz *a, FILE *fp){
	int i, j;

	fprintf(fp, "\n\n%d %d\n", a->filas, a->columnas);

	for(i = 0; i < a->filas; i++){
		for(j = 0; j < a->columnas; j++)
			fprintf(fp, "%g ", a->coef[i][j]);
		fprintf(fp, "\n");
	}
}

void destruir_matriz(matriz *m){
	shmctl(m->shmid, IPC_RMID, 0);
}

int main(int argc, char * argv[]){
	int m,n;
	FILE * fp;

	fp = fopen(FILENAME, "r+");

	matriz *a = leer_matriz(fp);
	matriz *b = leer_matriz(fp);

	matriz *c = multiplicar_matrices(a,b);

	imprimir_matriz(c);
	escribir_matriz(c, fp);

	destruir_matriz(a);
	destruir_matriz(b);
	destruir_matriz(c);

	fclose(fp);

	return 0;
}
