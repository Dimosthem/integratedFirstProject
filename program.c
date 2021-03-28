/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.	
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#define QUEUESIZE 10
#define LOOP 1000000
#define p_threads 2
#define q_threads 12

void *producer (void *args);
void *consumer (void *args);
void writeToFile(long long time);
void* calculateSinuses(void* angles);
void * printStuff(void* stuff);
void* calculateDoublePower(void* stuff);
void* addTwoNumbers(void* stuff);
void* findCircumference(void* stuff);

typedef struct  {
  void * (*work)(void *);
  void * arg;
} workFunction;

pthread_mutex_t loop_mut;
int loopsRemaining = LOOP;

typedef struct {
  workFunction buf[QUEUESIZE];
  long long times[QUEUESIZE];

  long head, tail;
  int full, empty, proDone;

  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;


queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

int main ()
{
  queue *fifo;
  pthread_t pro[p_threads], con[q_threads];

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }
  int counter1, counter2;


  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  long long start = current_time.tv_sec*1000000 + current_time.tv_usec;


  for(counter1=0; counter1<p_threads; counter1++){
    pthread_create (&pro[counter1], NULL, producer, fifo);
  }
  for(counter2=0; counter2<q_threads; counter2++){
   pthread_create (&con[counter2], NULL, consumer, fifo);
  }


  for(counter1=0; counter1<p_threads; counter1++){
    pthread_join (pro[counter1], NULL);
  }
  printf("I came here \n");
  
  for(counter2=0; counter2<q_threads; counter2++){
    printf("here %d \n", counter2);
    
    int op;
    for(op=0; op<q_threads; op++){
      pthread_cond_signal (fifo->notEmpty);
    }
  
    
   pthread_join (con[counter2], NULL);

  }

  
  gettimeofday(&current_time, NULL);
  long long end = current_time.tv_sec*1000000 + current_time.tv_usec;
  
  printf("This is how long it took for the program to run in seconds %f\n", (end-start)/1000000.0);
  queueDelete (fifo);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  double* args1 = (double*)malloc(10*sizeof(double));
  double* args2 = (double*)malloc(3*sizeof(double));
  double* args3 = (double*)malloc(2*sizeof(double));
  double* args4 = (double*)malloc(1*sizeof(double));

  int counter;
  for (counter=10; counter<110; counter +=10){
    args1[counter/10-1] = counter;
  }

  args2[0] = 3; args2[1] = 4; args2[2] = 2;

  args3[0] = 104; args3[1] = 873;

  args4[0] = 32;
  workFunction functions[5];
  functions[0].work = &calculateSinuses;
  functions[0].arg = args1;
  functions[1].work = &printStuff;
  functions[1].arg = NULL;
  functions[2].work = &calculateDoublePower;
  functions[2].arg = args2;
  functions[3].work = &addTwoNumbers;
  functions[3].arg = args3; 
  functions[4].work = &findCircumference;
  functions[4].arg = args4;

  //for (i = 0; i < LOOP; i++) {
  for(;;){
    
    pthread_mutex_lock(&loop_mut);
    i = loopsRemaining;
    if (loopsRemaining<=0){
      pthread_mutex_unlock(&loop_mut);
      pthread_cond_signal (fifo->notEmpty);
      break;
    }
    loopsRemaining--;
    pthread_mutex_unlock(&loop_mut);
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    queueAdd (fifo, functions[i%5]);
    if(loopsRemaining == 0){
      fifo->proDone =1;
    }
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    //usleep (100000);
  }
  
  free(args1);
  free(args2);
  free(args3);
  free(args4);

  //pthread_cond_signal (fifo->notEmpty);
  
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  int i;
  workFunction d;

  fifo = (queue *)q;
  double* results ;
  for (;;) {
    
    pthread_mutex_lock (fifo->mut);
   
    while (fifo->empty) {
     


      if(fifo->proDone){
        pthread_mutex_unlock (fifo->mut);
       
        
        return(NULL);
      }
      printf("EMPTY QUEUE \n");
      //pthread_mutex_unlock (fifo->mut);
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
      //pthread_mutex_lock (fifo->mut);
    
    }
    queueDel (fifo, &d);
    results = (double*)(*d.work)(d.arg);
    free(results);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
   // printf ("consumer: recieved %d.\n", d);
    
  }
  
  return (NULL);
}



queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->proDone = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  long long timeInMicroseconds = current_time.tv_sec*1000000 + current_time.tv_usec;

  q->buf[q->tail] = in;
  q->times[q->tail] = timeInMicroseconds;

  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  long long timeInMicroseconds = current_time.tv_sec*1000000 + current_time.tv_usec;

  *out = q->buf[q->head];
  long long startingTimeInMicroseconds = q->times[q->head];

  writeToFile(timeInMicroseconds - startingTimeInMicroseconds);

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}


void* calculateSinuses(void* angles){
  double* d_angles = (double*)angles;

  double* result = (double*)malloc(sizeof(double));
  int counter = 0;
  for (counter=0; counter<10; counter++)
  {
     result[0] += sin(d_angles[counter]); 
  }

  return result;
}

void * printStuff(void* stuff){

    printf("Sample text \n");
    double* result = (double*)malloc(sizeof(double));
    return result;
}

void* calculateDoublePower(void* stuff){
  double* args = (double*)stuff;

  double* result = (double*)malloc(sizeof(double));

  result[0] = pow(args[0],pow(args[1], args[2]));

  return result;
}

void* addTwoNumbers(void* stuff){
  double* args = (double*)stuff;

  double* result = (double*)malloc(sizeof(double));

  result[0] = args[0] + args[1];

  return result;
}

void* findCircumference(void* stuff){
  double* args = (double*)stuff;

  double* result = (double*)malloc(sizeof(double));

  result[0] = 2*M_PI*args[0];

  return result;
}

void writeToFile(long long time){
  FILE *fp;
  fp = fopen("results.txt", "a");
  fprintf (fp, "%lld\n", time);
  fclose(fp);
}
