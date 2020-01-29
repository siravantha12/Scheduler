/*
 * Ben Siravantha
 * sched.cpp
 * 2/18/19
 */
//////////////////////////////////
//
// Do not modify anything between
// this box and the one below
//
/////////////////////////////////
#include <sched.h>


/////////////////////////////////
//
// Implement the following below
//
////////////////////////////////

extern PROCESS process_list[MAX_PROCESSES];
extern SCHEDULE_ALGORITHM scheduling_algorithm;
extern char stack_space[STACK_ALLOC * MAX_PROCESSES];

#if defined(STUDENT)
static void proc_10_secs_run()
{
	uint32_t END_AFTER = get_timer_lo() / TIMER_FREQ + 10;
	write_stringln("\r\nI do nothing but quit after 10 seconds.\r\n");
	while (get_timer_lo() / TIMER_FREQ < END_AFTER);
}

static void proc_10_secs_sleep()
{
	write_stringln("\r\nI'm going to sleep for 10 seconds, then quitting.\r\n");
	sleep_process(get_current(), 10);
	wait();
}

//test a high priorty function
static void high_priority(){
	write_stringln("I'm high priority");
	uint32_t kill_time = (get_timer_lo()/TIMER_FREQ) + 5;
	while(get_timer_lo()/TIMER_FREQ < kill_time){
	
	}
	write_stringln("done");
}

void add_new_process(int padd)
{
	switch (padd) {
	//	case 1:			
	//		new_process(proc_10_secs_run, "10 second run process", 10);
	//	break;
		case 1: 
			new_process(high_priority, "highpri", -10);
			break;
		default:
			new_process(proc_10_secs_sleep, "10 second sleep process", 10);
		break;
	}
}
#endif
void new_process(void (*func)(), const char *name, int32_t priority){
	//check for valid priority
	if(priority > 10){
		priority = 10;
	}
	if(priority < -10){
		priority = -10;
	}
	
	//check process list for availability
	unsigned int i;
	int pid = -12;
	for(i = 0; i < MAX_PROCESSES; i++){
		if(process_list[i].state == DEAD){
			pid = i + 1;
			break;
		}
	}
	if(pid < 0){
		write_stringln("MAX_PROCESSES: cannot add another process");
		return;
	}

	//if available, make new process
	process_list[i].pid = pid;
	process_list[i].priority = priority;
	process_list[i].runtime = 0;
	process_list[i].sleep_time = 0;
	process_list[i].state = RUNNING;
	strcpy(process_list[i].name, name);
	process_list[i].program = func;
	process_list[i].regs[RA] = (uint32_t)recover;
	process_list[i].regs[SP] = (uint32_t)stack_space + STACK_ALLOC * pid - 1;
}

void del_process(PROCESS *p){
	p->state = DEAD;
}

void sleep_process(PROCESS *p, uint32_t sleep_time){
	p->state = SLEEPING;
	p->sleep_time = (get_timer_lo()/TIMER_FREQ) + sleep_time;
}

//awaken sleeping processes
static void awaken(PROCESS *p){
	uint32_t current_time = get_timer_lo()/TIMER_FREQ;;
	if(p->state == SLEEPING){
		if(p->sleep_time <= current_time){
			p->state = RUNNING;
			p->sleep_time = 0;
		}
	}
}

//round-robin scheduling
static PROCESS *rr(PROCESS *current){
	unsigned int next_pid = current->pid;//next index = pid which is (index + 1)
	PROCESS *next_process = &process_list[0];
	while(next_pid < MAX_PROCESSES){
		awaken(&process_list[next_pid]);
		if(process_list[next_pid].state == RUNNING){
			process_list[next_pid].num_switches++;
			next_process = &process_list[next_pid];
		}
		next_pid++;
	}
	return next_process;
}

//multi-level scheduling
static PROCESS *ml(PROCESS *current){
	PROCESS *next_process = current;
	int i;
	unsigned int lap = current->pid;
	for(i = -10; i <=10; i++){
		unsigned int next_pid = current->pid;
		do{
			awaken(&process_list[next_pid]);
			//check if running
			if(process_list[next_pid].state == RUNNING){
				if(process_list[next_pid].priority == i){
					process_list[next_pid].num_switches++;
					next_process = &process_list[next_pid];
					return next_process;
				}
			}
			next_pid = (next_pid + 1) % MAX_PROCESSES;
		}while(next_pid != lap);
	}
}

//multi-level feedback scheduling
static PROCESS *mlf(PROCESS *current){
	PROCESS *next_process = current;
	int i;
	unsigned int lap = current->pid;
	for(i = -10; i <= 10; i++){
		unsigned int next_pid = current->pid;
		do{
			awaken(&process_list[next_pid]);
			//check if running
			if(process_list[next_pid].state == RUNNING){
				if(process_list[next_pid].priority == i){
					process_list[next_pid].num_switches++;
					current->priority++;
					//make sure priority doesn't exceed 10	
					if(current->priority > 10) current->priority = 10;
					current->quantum_multiplier++;
					if(current->quantum_multiplier > 10) current->quantum_multiplier = 10;
					next_process = &process_list[next_pid];
					//write_string("returning: ");
					//write_stringln(next_process->name);
					return next_process;
				}
			}
			next_pid = (next_pid + 1) % MAX_PROCESSES;
		}while(next_pid != lap);
	}
}

PROCESS *schedule(PROCESS *current){
	
	switch (scheduling_algorithm) {
		//by default do round robin
		default:
			return rr(current);
		case SCHED_ML:
			return ml(current);
		case SCHED_MLF:
			return mlf(current);
	}
}
