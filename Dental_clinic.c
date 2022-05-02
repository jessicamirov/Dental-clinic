#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#define N 10
#define dentalHygienist 3
#define sofa 4
#define sleeping 0
#define working -1
#define payment 1

typedef struct patient{
	int id;
	struct patient *next; 
}patient;

typedef struct hygienist{
	int id;
	struct hygienist *next;
	struct patient *p;
}hygienist;

typedef struct clinic{
	struct hygienist *head_of_payment;
	struct patient *head_of_couch;
	struct patient *head_of_standing;
	struct patient *head_of_outside;
}clinic;

/* -----------------------------FUNCTIONS ---------------------------------*/
void initSemaphor(sem_t*, sem_t*);
void checkin();
void *treatment(void*);
void *work(void*);
void outsideClinicLine(struct patient*);
void insideClinicLine(struct patient*); 
void couchClinicLine(struct patient*);
void pushOutsideLine();
void pushStandingLine();
void pushSofaLine(); 
void whoIsSleeping(struct patient*);
void dentalTreatment(patient*);
int imNotFirst(struct patient *);
int imNotFirstT(struct patient *);
void randomTime ();
void pay(struct hygienist*);
void test(struct hygienist*);
void pushPaymentLine(struct hygienist *);
int imOutSide(struct patient*);
void outQueue(struct patient *);
void deleteAll();
/* -----------------------------GLOBAL VARIABLES --------------------------*/
int howManyInClinic = 0, howManyInCouch = 0, howManyInTreatment = 0, state[dentalHygienist] = {sleeping,sleeping,sleeping};
int f1 = 0, f2 = 0, couchNotification = 0, standNotification = 0, outNotification = 0; //flags
sem_t *sArray = NULL;
sem_t *dsArray = NULL;
sem_t *mutex = NULL;
sem_t *mutex1 = NULL;
sem_t *mutex2 = NULL;
pthread_t *tArray = NULL, dArray[dentalHygienist];
struct clinic *dentalClinic;
patient *std[N+2];
hygienist *ind[dentalHygienist];
/* -----------------------------MAIN -------------------------------------*/
void main() {
	int i; 
	srand(time(NULL)); //makes different outcomes each compilation
	// allocate memory for mutex
	if((mutex = (sem_t*)malloc(sizeof(sem_t))) == NULL) { printf("Allocation failed"); deleteAll(); }
	if((mutex1 = (sem_t*)malloc(sizeof(sem_t))) == NULL) { printf("Allocation failed"); deleteAll(); }
	if((mutex2 = (sem_t*)malloc(sizeof(sem_t))) == NULL) { printf("Allocation failed"); deleteAll(); }
	// initialize the mutex to one so the code could run
	if(sem_init(mutex,0,1) != 0) { printf("Error by creating semaphore"); deleteAll(); } 
	if(sem_init(mutex1,0,1) != 0) { printf("Error by creating semaphore"); deleteAll(); }
	if(sem_init(mutex2,0,1) != 0) { printf("Error by creating semaphore"); deleteAll(); } 
	// allocate memory for struct, dentalClinic holds the pointers to the head of all linked lists
	if((dentalClinic = (clinic*)malloc(sizeof(clinic))) == NULL) { printf("Allocation failed"); deleteAll(); }
	// allocate memory for semaphors 
	sArray = (sem_t*)malloc(sizeof(sem_t)*(N+2));
	if(sArray == NULL) { printf("Allocation failed"); deleteAll(); } 
	dsArray = (sem_t*)malloc(sizeof(sem_t)*(dentalHygienist));
	if(dsArray == NULL) { printf("Allocation failed"); deleteAll(); }
	// initialize semaphors
	initSemaphor(sArray,dsArray);
	// create threads 
	checkin();
	for(i=0; i<N+2; i++)
		pthread_join(tArray[i], NULL); // starting each patient threads
}//main

// initialize patient and hygienist semaphore to zero //
void initSemaphor(sem_t* sArray, sem_t* dsArray) {
	int i;
	for(i=0; i<N+2; i++)
		sem_init(&sArray[i],0,0);
	for(i=0; i<dentalHygienist; i++)
		sem_init(&dsArray[i],0,0);
}

// assigning semphors and the function for the threads //
void checkin() {
	int i, rc;
	patient *tmp;
	hygienist *tmpH;
	tArray = (pthread_t*)malloc(sizeof(pthread_t)*(N+2)); 
	for(i=0; i<N+2; i++) {
		if((tmp = (patient*)malloc(sizeof(patient))) == NULL) { printf("Allocation failed"); deleteAll(); }
		tmp->id = i;
		std[i] = tmp;
		rc = pthread_create(&tArray[i],NULL,treatment,(void*)(&std[i]));
		if(rc != 0) { printf("error create thread"); deleteAll(); }
	} //for
	for(i=0; i<dentalHygienist; i++) {
		if((tmpH = (hygienist*)malloc(sizeof(hygienist))) == NULL) { printf("Allocation failed"); deleteAll(); }
		tmpH->id = i;
		ind[i] = tmpH;
		rc = pthread_create(&dArray[i],NULL,work,(void*)(&ind[i]));
		if(rc != 0) { printf("error create thread"); deleteAll(); } 
	}
}

// the main function of the patient // 
void* treatment(void* arg) {
	int i;
    struct patient *p = *((struct patient**)arg);
	while(1) {
		outQueue(p);	// checking if he can enter the clinic, if not he will wait outside
		insideClinicLine(p);	// getting inside the clinic
		randomTime();	// assigning random sleep time so a context switch could happen
		if((howManyInCouch == sofa) || imNotFirst(p) || f1) {  // if there are four people on the sofa OR the patient isn't the first in the queue the standing OR flag f1==1 the patient will wait 
			sem_wait(&sArray[p->id]);
			standNotification = 0;	// head of the standing queue got a post somewhere and now head of standing queue had been changed so standNotification gets zero so he could be later notified
		}
	f1 =1;	// because of above random time context switch could happen ,f1 = 1 so that other paitent will wait until a patient changed Queue lines 
	sem_wait(mutex); // entering critical code.
		pushStandingLine(); //push the line of those standing
		couchClinicLine(p); // Entering the couch linked list line
	sem_post(mutex); // exiting critical code
		randomTime(); //assiging random sleep time so a context switch could happen
		if(howManyInTreatment == dentalHygienist || imNotFirstT(p) || f2) {	// if there are three people in tretment OR the patient isn't the first in queue on couch OR flag f2==1 the patient will wait
			sem_wait(&sArray[p->id]);
			couchNotification = 0; // head of the couch queue got a post somewhere and now head of couch queue had been changed so couchNotification gets zero so he could be later notified
		}
	f2 = 1; // because of above random time context switch could happen ,f2 = 1 so that other paitent will wait until a patient changed Queue lines 
		pushSofaLine(); //taking the patient to out of couch queue line 
		dentalTreatment(p); // patient entering dental tretment process
		sem_wait(&sArray[p->id]); // waiting to payment
		howManyInClinic--;	// patient left the clinic 
		if(dentalClinic->head_of_outside != NULL && !outNotification) { // if someone waiting outside AND hasn't been notified yet, post head of outside
			outNotification = 1; 
			sem_post(&sArray[dentalClinic->head_of_outside->id]);
		}
	} // while
}
 // main function of the hygienist // 
void *work(void* arg) {
	hygienist* h = *((struct hygienist**)arg);
	while(1) {
		sem_wait(&dsArray[h->id]);  //hygienist will wait for a post from a patient to start working 
		printf("\nI'm Dental Hygienist #%d, I'm working now\n", h->id+1);
		randomTime(); // assigning a random treatment time 
		test(h); // testing if an other hygienist is currently at payment if not, processed 
		printf("\nI'm patient #%d, I'm paying now\n",h->p->id+1);
		sem_wait(mutex); // starting critical payment process 
		pay(h);	// hygienist will take money from patient 
		sem_post(&sArray[h->p->id]); //finish with the payment
	sem_post(mutex); // end of payment critical process  
	}
}

// Responsible for managing the outside list //
void outQueue(struct patient *p){
	sem_wait(mutex);
	if(howManyInClinic == N) { //if there are N people in the clinic the patient will go and wait in outside  
		outsideClinicLine(p);
		sem_post(mutex);
		sem_wait(&sArray[p->id]);
	} else { 					// if there are less then N in the clinic the patient will continue 
		sem_post(mutex);	
	}
}

// update outside queue in the linked list //
void outsideClinicLine(struct patient *p) {
	if (!imOutSide(p)){
		printf("\nI'm Patient #%d, I'm out of clinic\n",(p->id)+1);	
		patient* head = dentalClinic->head_of_outside;
		if(head == NULL)
			dentalClinic->head_of_outside = p;
		else {
			while(head->next != NULL) 
				head = head->next;
			head->next = p;
		} //else
	} //if
}

// update inside queue in the linked list //
void insideClinicLine(struct patient *p) {
	sem_wait(mutex);
	if(imOutSide(p)) // if patient waited from outside queue line, push him from outside queue line  
		pushOutsideLine();
	howManyInClinic++;	
	patient* head = dentalClinic->head_of_standing;
	if(head == NULL)
		dentalClinic->head_of_standing = p;
	else {
		while(head->next != NULL) 
			head = head->next;		
		head->next = p;		
	}
	printf("\nI'm Patient #%d, I got into the clinic\n",(p->id)+1);
	sem_post(mutex);
}

// update couch queue in the linked list // 
void couchClinicLine(struct patient *p) {
	howManyInCouch++;	
	patient *head = dentalClinic->head_of_couch; 	
	if(head == NULL) 
		dentalClinic->head_of_couch = p;
	else {
		while(head->next != NULL)
			head = head->next;		
		head->next = p;	
	}
	printf("\nI'm Patient #%d, I'm sitting on the sofa\n",(p->id)+1);
	if(dentalClinic->head_of_standing != NULL && howManyInCouch < sofa && !standNotification) {	//if is someone waiting in standing line AND there is space on sofa AND head of standing hasn't been notified, post head of standing
		standNotification = 1; 
		sem_post(&sArray[dentalClinic->head_of_standing->id]);
	}//if
}

// Advanced the outside queue list // 
void pushOutsideLine() {
	patient *tmp = dentalClinic->head_of_outside;
	dentalClinic->head_of_outside = dentalClinic->head_of_outside->next;
	tmp->next = NULL;
	outNotification = 0;
}

// Advanced the standing queue list // 
void pushStandingLine() {
	patient *tmp = dentalClinic->head_of_standing;
	dentalClinic->head_of_standing = dentalClinic->head_of_standing->next;
	tmp->next = NULL;
}

// Advanced the sofa queue list // 
void pushSofaLine() {
	sem_wait(mutex); //starting critical code for patient to start a treatment and assigning a hygienist to a patient 
	patient *tmp = dentalClinic->head_of_couch;
	dentalClinic->head_of_couch = dentalClinic->head_of_couch->next;
	tmp->next = NULL;
	howManyInCouch--;
	howManyInTreatment++;
	if(dentalClinic->head_of_couch!= NULL && howManyInTreatment < dentalHygienist && !couchNotification) { //if is someone waiting in couch line AND there is space in tretment AND head of couch hasn't been notified, post head of couch 
		couchNotification = 1;
		sem_post(&sArray[dentalClinic->head_of_couch->id]);}
	if(!standNotification) { // if head of standing line hasn't been notified post head of standing because there is room in couch
	standNotification = 1;
	sem_post(&sArray[dentalClinic->head_of_standing->id]);
	}
}

// checking if the patient is not the first in the queue of standing line // 
int imNotFirst(struct patient *p){
	sem_wait(mutex);
	int tmp = dentalClinic->head_of_standing != p;
	sem_post(mutex);
	return tmp;
}

// checking if the patient is not the first in the queue of couch line //
int imNotFirstT(struct patient *p){
	sem_wait(mutex);
	int tmp =  dentalClinic->head_of_couch != p;
	sem_post(mutex);
	return tmp;
}

// checking if the patient is in the outside queue line //
int imOutSide(struct patient *p){
	int tmp = 0;
	patient *tmp1 = dentalClinic->head_of_outside;
	while(tmp1 != NULL){
		if(tmp1 == p)
			tmp = 1;
		tmp1=tmp1->next;
	}
	return tmp;
}

// assigning the patient to the free hygienist //
void dentalTreatment(patient* p) {
	printf("\nI'm Patient #%d, I'm getting treatment\n", p->id+1);
	whoIsSleeping(p);
}

// checking which hygienist is free and waking her up to work // 
void whoIsSleeping(struct patient* p) {
	sem_wait(mutex1);
	int i;
	for(i=0; i<dentalHygienist; i++) { // looping to find the sleeping hygienist 
		if(state[i] == sleeping) {
			ind[i]->p = p; // assinging a patient to the hygienist 
			state[i] = working;	// change the status to working
			break;
		}//if
	}//for
sem_post(mutex); // end critical code from function pushSofaLine 
	sem_post(&dsArray[i]); // hygienist start to work 
	sem_post(mutex1); 
}

// checking if an hygienst already on payment //
void test(struct hygienist* h) {
	sem_wait(mutex2);
	if((state[0] == payment) || (state[1] == payment) || (state[2] == payment)) { // if any other hygienist is already at payments status, wait 
		pushPaymentLine(h);
		sem_wait(&dsArray[h->id]);
	}
	state[h->id] = payment; //changing hygienist status to payment 
	sem_post(mutex2);
}

// payment and leaving the clinic //
void pay(struct hygienist* h) {
	printf("\nI'm Dental Hygienist #%d, I'm getting a payment\n" ,h->id+1);
	state[h->id] = sleeping; // hygienist finish the work and going to sleep 
	howManyInTreatment--;
	if(dentalClinic->head_of_couch != NULL && !couchNotification) { // if a patient is wainting on the couch AND hasent been notified' post head of couch
		couchNotification = 1;
		sem_post(&sArray[dentalClinic->head_of_couch->id]); 
	}
	if(dentalClinic->head_of_payment != NULL) { // if another hygienist is waiting for payment, wake her up from function test
		hygienist *tmp = dentalClinic->head_of_payment;
		dentalClinic->head_of_payment = dentalClinic->head_of_payment->next;
		tmp->next = NULL;
		sem_post(&dsArray[tmp->id]);
	}
}

// Advanced the payment queue list //
void pushPaymentLine(struct hygienist *h) {
	hygienist *head = dentalClinic->head_of_payment;
	if(head == NULL)
		dentalClinic->head_of_payment = h;
	else {	
		while(head->next != NULL)
			head = head->next;
		head->next = h;
	}
}

// assigning random sleeping time //
void randomTime() {
	int time = rand() % 5;
	sleep(time);
}

// deleting all allocated memory //
void deleteAll() {
	if(sArray != NULL)
		free(sArray);
	if(dsArray != NULL)
		free(dsArray);
	if(mutex = NULL)
		free(mutex);
	if(mutex1 = NULL)
		free(mutex);
	if(mutex2 = NULL)
		free(mutex2);
	if(tArray = NULL)
		free(tArray);
	exit(1);
}