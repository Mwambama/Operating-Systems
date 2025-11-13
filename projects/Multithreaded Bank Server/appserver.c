#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "Bank.h" // Includes initialize_accounts, read_account, write_account

// --- Configuration and Constants ---
#define MAX_ACCOUNTS 10000 
#define MAX_TOKENS 50    

// --- Synchronization and Globals ---
pthread_mutex_t account_locks[MAX_ACCOUNTS]; // Fine-grained locks (one per account ID)
pthread_mutex_t queue_mutex;                  // Protects access to the request queue
pthread_cond_t queue_cond;                   // Signals worker threads when a job is available
pthread_mutex_t output_mutex;                 // Protects writing results to the output file

int NUM_ACCOUNTS;
int NUM_WORKERS;

// --- Data Structures ---
struct trans {
    int acc_id; // 1-based index
    int amount;
};

struct request {
    struct request *next;
    int request_id;
    char request_type; 
    int check_acc_id; 
    struct trans *transactions; 
    int num_trans;
    struct timeval starttime, endtime; 
};

struct queue {
    struct request *head, *tail; 
    int next_request_id;
    int num_jobs;
    int end_flag; // Set to 1 when 'END' command is received
} request_queue;


// --- Function Prototypes ---
void *worker_thread(void *arg);
int integer_comparator(const void *a, const void *b);
void process_transaction(struct request *req);
void process_check(struct request *req);
struct request *parse_input(char *input_line, int current_id);
void enqueue_request(struct request *req);
struct request *dequeue_request();


// --- Comparator for Deadlock Prevention (qsort) ---
// Sorts integer pointers based on the integer value they point to.
int integer_comparator(const void *a, const void *b) {
    // We are sorting an array of 1-based account IDs
    return (*(int*)a - *(int*)b);
}


// --- Queue Management ---

void enqueue_request(struct request *req) {
    pthread_mutex_lock(&queue_mutex);
    
    if (request_queue.tail == NULL) {
        request_queue.head = request_queue.tail = req;
    } else {
        request_queue.tail->next = req;
        request_queue.tail = req;
    }
    req->next = NULL;
    request_queue.num_jobs++;
    
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

struct request *dequeue_request() {
    struct request *req = NULL;
    
    pthread_mutex_lock(&queue_mutex);
    
    // Wait while queue is empty AND END flag is not set
    while (request_queue.head == NULL && request_queue.end_flag == 0) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    // Check if we woke up to process a job or exit
    if (request_queue.head != NULL) {
        req = request_queue.head;
        request_queue.head = request_queue.head->next;
        if (request_queue.head == NULL) {
            request_queue.tail = NULL;
        }
        request_queue.num_jobs--;
    }
    
    pthread_mutex_unlock(&queue_mutex);
    return req;
}


// --- Request Parsing (Main Thread Helper) ---

struct request *parse_input(char *input_line, int current_id) {
    char *tokens[MAX_TOKENS];
    char *token;
    int count = 0;
    
    char *line_copy = strdup(input_line);
    if (line_copy == NULL) { fprintf(stderr, "Memory error during parsing.\n"); return NULL; }

    token = strtok(line_copy, " \t\r\n");
    while (token != NULL && count < MAX_TOKENS) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    if (count == 0) { free(line_copy); return NULL; }
    
    struct request *req = (struct request *)calloc(1, sizeof(struct request));
    if (req == NULL) { free(line_copy); return NULL; }
    req->request_id = current_id;

    if (strcmp(tokens[0], "CHECK") == 0) {
        if (count != 2) { goto invalid_input; }
        req->request_type = 'C';
        req->check_acc_id = atoi(tokens[1]);
    } else if (strcmp(tokens[0], "TRANS") == 0) {
        if (count < 3 || count % 2 != 1) { goto invalid_input; }
        req->request_type = 'T';
        req->num_trans = (count - 1) / 2;
        req->transactions = (struct trans *)calloc(req->num_trans, sizeof(struct trans));
        
        for (int i = 0; i < req->num_trans; i++) {
            req->transactions[i].acc_id = atoi(tokens[2 * i + 1]);
            req->transactions[i].amount = atoi(tokens[2 * i + 2]);
        }
    } else if (strcmp(tokens[0], "END") == 0) {
        req->request_type = 'E';
        request_queue.end_flag = 1; // Signal workers to prepare to exit
    } else {
        goto invalid_input;
    }
    
    free(line_copy); 
    return req;

invalid_input:
    fprintf(stderr, "Error: Invalid command format for '%s'.\n", tokens[0]);
    if (req) free(req); 
    free(line_copy);
    return NULL;
}


// --- Worker Processing Logic ---

void process_check(struct request *req) {
    int id = req->check_acc_id;
    
    // Fine-Grained Locking: Lock the specific account being checked
    pthread_mutex_lock(&account_locks[id - 1]);
    int balance = read_account(id);
    pthread_mutex_unlock(&account_locks[id - 1]);
    
    // Output
    gettimeofday(&req->endtime, NULL); 
    pthread_mutex_lock(&output_mutex);
    // Use the required format: printf("TIME %ld.%06ld\n", time.tv_sec, time.tv_usec);
    fprintf(output_file, "%d BAL %d TIME %ld.%06ld %ld.%06ld\n", 
            req->request_id, balance, req->starttime.tv_sec, req->starttime.tv_usec,
            req->endtime.tv_sec, req->endtime.tv_usec);
    pthread_mutex_unlock(&output_mutex);
}

void process_transaction(struct request *req) {
    // 1. Prepare for Deadlock Prevention: Collect and Sort IDs
    int *sorted_ids = (int *)malloc(req->num_trans * sizeof(int));
    int *all_ids = (int *)malloc(req->num_trans * sizeof(int));
    
    for (int i = 0; i < req->num_trans; i++) {
        all_ids[i] = req->transactions[i].acc_id;
        sorted_ids[i] = req->transactions[i].acc_id;
    }
    
    // CRITICAL STEP: Sort the list of involved account IDs for consistent lock acquisition order
    qsort(sorted_ids, req->num_trans, sizeof(int), integer_comparator); 

    // 2. Acquire Locks in Sorted Order (Deadlock Prevention)
    for (int i = 0; i < req->num_trans; i++) {
        pthread_mutex_lock(&account_locks[sorted_ids[i] - 1]);
    }
    
    // 3. Atomicity Check (Read & Verify Balances)
    int insufficient_acc_id = -1;
    int *original_balances = (int *)malloc(req->num_trans * sizeof(int));
    
    for (int i = 0; i < req->num_trans; i++) {
        int id = all_ids[i];
        int amount = req->transactions[i].amount;
        original_balances[i] = read_account(id); 
        
        // Insufficient Funds Check (Atomicity requirement)
        if (original_balances[i] + amount < 0) {
            insufficient_acc_id = id;
            break; // Identify first violating account
        }
    }
    
    // 4. Execute or Void
    if (insufficient_acc_id == -1) {
        // SUCCESS: Apply all writes
        for (int i = 0; i < req->num_trans; i++) {
            int id = all_ids[i];
            int amount = req->transactions[i].amount;
            write_account(id, original_balances[i] + amount); 
        }
        
        // Success Output
        gettimeofday(&req->endtime, NULL);
        pthread_mutex_lock(&output_mutex);
        fprintf(output_file, "%d OK TIME %ld.%06ld %ld.%06ld\n", 
                req->request_id, req->starttime.tv_sec, req->starttime.tv_usec,
                req->endtime.tv_sec, req->endtime.tv_usec);
        pthread_mutex_unlock(&output_mutex);
    } else {
        // ISF: Output failure, state remains original (no writes performed)
        gettimeofday(&req->endtime, NULL);
        pthread_mutex_lock(&output_mutex);
        fprintf(output_file, "%d ISF %d TIME %ld.%06ld %ld.%06ld\n", 
                req->request_id, insufficient_acc_id, 
                req->starttime.tv_sec, req->starttime.tv_usec,
                req->endtime.tv_sec, req->endtime.tv_usec);
        pthread_mutex_unlock(&output_mutex);
    }

    // 5. Release Locks in Sorted Order
    for (int i = req->num_trans - 1; i >= 0; i--) { 
        pthread_mutex_unlock(&account_locks[sorted_ids[i] - 1]);
    }
    
    free(sorted_ids);
    free(original_balances);
    free(all_ids);
}


// --- Worker Thread Routine ---

void *worker_thread(void *arg) {
    struct request *req;

    while (1) {
        req = dequeue_request(); // Blocks if queue is empty (using cond_wait)
        
        // Exit condition: if END flag is set AND no more jobs are in the queue
        if (req == NULL && request_queue.end_flag == 1) {
            pthread_cond_broadcast(&queue_cond); 
            break;
        } 
        
        if (req != NULL) {
            if (req->request_type == 'C') {
                process_check(req);
            } else if (req->request_type == 'T') {
                process_transaction(req);
            }
            // Clean up memory allocated for the transactions array in parse_input
            if (req->request_type == 'T' && req->transactions != NULL) {
                free(req->transactions);
            }
            free(req); // Free the request structure itself
        }
    }
    return NULL;
}


// --- Main Function (Producer) ---

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: ./appserver <# of worker threads> <# of accounts> <output file>\n");
        return 1;
    }

    // 1. Parse Arguments
    NUM_WORKERS = atoi(argv[1]);
    NUM_ACCOUNTS = atoi(argv[2]);
    char *output_filename = argv[3];

    if (NUM_ACCOUNTS > MAX_ACCOUNTS) {
        fprintf(stderr, "Error: Max accounts supported is %d\n", MAX_ACCOUNTS);
        return 1;
    }

    // 2. Initialization
    if (initialize_accounts(NUM_ACCOUNTS) == 0) {
        fprintf(stderr, "Error: Failed to initialize bank accounts.\n");
        return 1;
    }
    output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return 1;
    }
    
    // Initialize Synchronization Primitives
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_cond, NULL);
    pthread_mutex_init(&output_mutex, NULL);
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_init(&account_locks[i], NULL); 
    }

    request_queue.next_request_id = 1;
    request_queue.num_jobs = 0;
    request_queue.head = request_queue.tail = NULL;
    request_queue.end_flag = 0;

    // 3. Create Worker Threads
    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }

    // 4. Input Loop (Main thread acts as request producer)
    char input_line[1024];
    while (request_queue.end_flag == 0 && fgets(input_line, sizeof(input_line), stdin) != NULL) {
        struct request *req = parse_input(input_line, request_queue.next_request_id);
        
        if (req != NULL) {
            gettimeofday(&req->starttime, NULL); 
            
            if (req->request_type == 'E') {
                // If 'END', enqueue it, but immediately break the input loop
                enqueue_request(req); 
                break;
            } else {
                enqueue_request(req);
                // Immediate console response
                printf("< ID %d\n", req->request_id);
                request_queue.next_request_id++;
            }
        }
    }
    
    // 5. Final Cleanup and Exit
    request_queue.end_flag = 1; // Ensure flag is set one final time to wake any stragglers
    pthread_cond_broadcast(&queue_cond); // Wake up all waiting workers
    
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(workers[i], NULL);
    }
    
    // Final resource cleanup
    free_accounts();
    fclose(output_file);
    return 0;
}