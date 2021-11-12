#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUM_JOBTYPES 3
#define RR_QUANTUM_LENGTH 10

enum states {
    RTR, CPU, IO, DONE
};

enum algs {
    FCFS, SRTF, RR
};

typedef struct job {

    int id,
        priority;

    int state,
        burst_countdown,
        cpu_burst_length,
        io_burst_length;

    int quant_countdown;
        reps;

    int start_time,
        end_time,
        wait_time;

} job_t;

typedef struct node {
    int priority;
    job_t* job;
    struct node* next;
} node_t;

node_t* rtr_queue;
job_t** jobs;
job_t* active = NULL;

int num_jobs = 0,
    finished_jobs = 0;

int time = 1,
    cpu_busy_time = 0,
    cpu_idle_time = 0;

int alg_id = -1;

const char* alg_names[] = { "FCFS", "SRTF", "RR" };

void run();
job_t* set_next_job();
void push(node_t** head, job_t* j);
job_t* pop(node_t**);
int getpri(job_t* j);
void print_status_line();
void print_report();
void load_jobs(FILE* f);

int main(int argc, char* argv[]) {

    if (argc != 3) {                                                            //error message for use
        puts("type <./program2_kgw47> <file> <algorithm>");
        return 0;
    }

    for (int i = 0; i < NUM_JOBTYPES; i++) {                                    //select the correct algorithm
        if (!strcmp(argv[2], alg_names[i])) {
            alg_id = i;
        }
    }

    FILE* f = fopen(argv[1], "r");                                              //opens given input file with information
    load_jobs(f);

    fclose(f);

    if (alg_id != -1) {                                                         //run simulation if algorithm exists

        run();
    }
    else {
        printf("Not an algorithm", alg_names[alg_id]);
        return 0;
    }
}

void run() { //run simulation and continue with all required jobs

    puts("│   Time   :   CPU   :    IO    │");

    for (;;) {

        if (active) {

            --active->burst_countdown;
            --active->quant_countdown;

            if (active->burst_countdown == 0) {
                active->state = IO;
                active->burst_countdown = active->io_burst_length;
                active = set_next_job();
            }

            else if (active->quant_countdown == 0) {
                active->state = RTR;
                push(&rtr_queue, active);
                active = set_next_job();
            }

            ++cpu_busy_time;

        }

        else {
            if (!(active = set_next_job())) {
                ++cpu_idle_time;
            }
        }

        for (int i = 0; i < num_jobs; i++) {

            job_t* j = jobs[i];

            if (j->state == IO) {

                if (j->burst_countdown-- == 0) {
                    if (j->reps == 0) {
                        j->state = DONE;
                        j->end_time = time;
                        finished_jobs++;
                    }
                    else {
                        j->state = RTR;
                        push(&rtr_queue, j);
                    }
                }
            }

            if (j->state == RTR) {
                ++j->wait_time;
            }
        }

        if (finished_jobs < num_jobs)
            print_status_line();
        else
            break;

        ++time;
    }

    print_report();

    for (int i = 0; i < num_jobs; i++) {
        free(jobs[i]);
    }

    if (rtr_queue) {
        free(rtr_queue);
    }

}

job_t* set_next_job() { //round robin speccial job details

    job_t* j = NULL;
    if ((j = pop(&rtr_queue))) {

        j->state = CPU;

        if (!j->start_time) {
            j->start_time = time;
        }

        if (j->burst_countdown <= 0) {
            j->burst_countdown = j->cpu_burst_length;
            j->reps--;
        }

        j->quant_countdown = (alg_id == RR ? RR_QUANTUM_LENGTH : -1);
    }
    return j;
}

void push(node_t** head, job_t* j) { //stack algorithm push

    node_t* new = malloc(sizeof(node_t));
    new->job = j;
    new->next = NULL;
    new->priority = getpri(j);

    if (*head == NULL) {
        *head = new;
        return;
    }

    if (new->priority < (*head)->priority) {
        new->next = *head;
        *head = new;
        return;
    }

    node_t* cur = *head;

    while (cur->next && new->priority > cur->next->priority) {
        cur = cur->next;
    }

    new->next = cur->next;
    cur->next = new;
}

job_t* pop(node_t** head) { //stack algorithm pop

    node_t* cur = *head;

    if (!cur) {
        return NULL;
    }

    job_t* j = cur->job;
    *head = (*head)->next;
    free(cur);

    return j;
}

int getpri(job_t* j) { //set priority

    int p = -1;

    if (alg_id == PS) {
        p = j->priority;
    }

    if (alg_id == SJF) {
        p = j->cpu_burst_length * j->reps;
    }

    if (alg_id == RR) {
        p = time;
    }

    return p;
}

void print_status_line() { //display current state

    char* iostring = malloc(16 * sizeof(char));
    int c = 0;

    for (int i = 0; i < num_jobs; i++) {
        if (jobs[i]->state == IO) {
            c += sprintf(&iostring[c], "%d ", jobs[i]->id);
        }
    }

    if (!c) {
        strcpy(iostring, "xx");
    }

    if (active) printf("│  %4d %9d %9s     │\n", time, active->id, iostring);
    else printf("│  %4d %9s %9s     │\n", time, "xx", iostring);

    free(iostring);
}

void print_report() { //display results

    int turn_time = 0;

    for (int i = 0; i < num_jobs; i++) {

        printf("Process ID: %5d\n", jobs[i]->id);
        printf("Start Time: %5d\n", jobs[i]->start_time);
        printf("End Time:   %5d\n", jobs[i]->end_time);
        printf("Wait Time:  %5d\n", jobs[i]->wait_time);

        turn_time += jobs[i]->end_time;
    }

    printf("Average Turnaround Time: %d\n", turn_time / num_jobs);
    printf("CPU Busy Time: %d\n", cpu_busy_time);
    printf("CPU Idle Time: %d\n\n", cpu_idle_time);

}

void load_jobs(FILE* f) { //scan file for input and use in specific job

    int bufsize = 128;
    char line[bufsize];

    while (fgets(line, bufsize, f)) {

        if (!iscomment(line)) {

            job_t* j = calloc(1, sizeof(job_t));

            sscanf(line, "%d %d %d %d %d",
                &j->id,
                &j->cpu_burst_length,
                &j->io_burst_length,
                &j->reps,
                &j->priority);

            push(&rtr_queue, j);
            ++num_jobs;
        }
    }

    jobs = malloc(num_jobs * sizeof(job_t));

    node_t* cur = *&rtr_queue;

    int i = 0;
    while (cur) {
        jobs[i++] = cur->job;
        cur = cur->next;
    }
}
