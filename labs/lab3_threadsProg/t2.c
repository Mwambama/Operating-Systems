/* t2.c
   synchronize threads through mutex and conditional variable 
   To compile use: gcc -o t2 t2.c -lpthread 
*/ 

#include <stdio.h>
#include <pthread.h>

void 	hello();    // define two routines called by threads    
void 	world();         	
void    again();     //edit: added a routine for again to be called by threads

/* global variable shared by threads */
pthread_mutex_t 	mutex;  		// mutex
pthread_cond_t 		done_hello; 	// conditional variable
pthread_cond_t          done_again;     // edit: added a conditional variable for again
int 			done = 0;      	// testing variable
int                    done_world = 0;  // edit:  added a variable for again thread to wait for world 

int main (int argc, char *argv[]){
    pthread_t 	tid_hello, // thread id  
    		tid_world,    // edit: had a semi-colon but its need to be a colon since tid_again is added
    		tid_again; //edit: added a thread id for again
    /*  initialization on mutex and cond variable  */ 
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&done_hello, NULL); 
    pthread_cond_init(&done_again, NULL); 
    
    pthread_create(&tid_hello, NULL, (void*)&hello, NULL); //thread creation
    pthread_create(&tid_world, NULL, (void*)&world, NULL); //thread creation 
    pthread_create(&tid_again, NULL, (void*)&again, NULL); //edit: thread creation for again

    /* main waits for the two threads to finish */
    pthread_join(tid_hello, NULL);  
    pthread_join(tid_world, NULL);
    pthread_join(tid_again, NULL);  // edit: join for main to exist or finish witout printing again // thread to finish program

    printf("\n");
    return 0;
}

void hello() {
    pthread_mutex_lock(&mutex);
    printf("hello ");
    fflush(stdout); 	// flush buffer to allow instant print out
    done = 1;
    pthread_cond_signal(&done_hello);	// signal world() thread
    pthread_mutex_unlock(&mutex);	// unlocks mutex to allow world to print
    return ;
}


void world() {
    pthread_mutex_lock(&mutex);

    /* world thread waits until done == 1. */
    while(done == 0) 
	pthread_cond_wait(&done_again, &mutex);

    printf("world ");  //edit: left a space so that again is not to close
    fflush(stdout);
    done_world = 1;   // edit: set done world to 1 so the signal can recieve the following signal
    pthread_mutex_unlock(&mutex); // unlocks mutex
    
    return;
    
  }
    
 void again(){   // edit: added the again function implementation
      
     pthread_mutex_lock(&mutex);
     
         // wil have again waits until done 1
         
         /* edit: the thread for again which waits until done world == 1 */
           while ( done_world == 0)    // edit: again() waits for world to set done_world to 1.
              pthread_cond_wait(&done_again, &mutex);   // edit: waits for the signal from world()
              
              printf("again");    // edit: prints again
              fflush(stdout);
              pthread_mutex_unlock(&mutex); // unlocks mutex   
    return ;
}
