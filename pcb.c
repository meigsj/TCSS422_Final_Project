/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pcb.h"

// Global to determine the next possible PID value for a new PCB
int currentpid = 1;
// Constant to represent a successful function call
const int IS_NULL = 0;
// Constant to represent an error where a null pointer was passed to a function
const int NULL_ERROR = -1;

/*
 Constructor.
 */
PCB_p pcbConstruct(void) {
    PCB_p my_pcb = (PCB_p) malloc(sizeof(PCB_s));
    my_pcb->context = (CPU_context_p) malloc(sizeof(CPU_context_s));
    return my_pcb;
}

/*
 Destructor.
 */
int pcbDestruct(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        free(my_pcb->context);
        free(my_pcb);
    }
}

/*
 * Initialize all values of a pcb struct to 0 or NULL.
 * Sets the pid to the next pid based on the currentpid global
 */
int initialize_pcb(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->context->pc = 0;
        my_pcb->context->ir = 0;
        my_pcb->context->psr = 0;
        my_pcb->context->r0 = 0;
        my_pcb->context->r1 = 0;
        my_pcb->context->r2 = 0;
        my_pcb->context->r3 = 0;
        my_pcb->context->r4 = 0;
        my_pcb->context->r5 = 0;
        my_pcb->context->r6 = 0;
        my_pcb->context->r7 = 0;
        my_pcb->state = NEW;
		my_pcb->type = NO_TYPE;
        my_pcb->priority = 0;
        my_pcb->channel_no = 0;
        my_pcb->parent = 0; // how do you find parent ID?
        my_pcb->mem = 0;
        my_pcb->size = 0;
        my_pcb->pid = currentpid++;
        // Added for problem 3
        // Used to help track if a PCB is privileged for termination checks
        my_pcb->privileged = 0;
        my_pcb->terminate = 0;
        my_pcb->term_count = 0;
        my_pcb->creation = setCreation(my_pcb, time(NULL));
        
        for (int i = 0; i < IO_TRAP_SIZE; i++) {
            my_pcb->io_1_traps[i] = NO_TRAP;
            my_pcb->io_2_traps[i] = NO_TRAP;
        }

		for (int i = 0; i < SYNCRO_SIZE; i++) {
			my_pcb->lock_1_pcs[i] = -1;
			my_pcb->lock_2_pcs[i] = -1;
			my_pcb->unlock_1_pcs[i] = -1;
			my_pcb->unlock_2_pcs[i] = -1;
			my_pcb->wait_1_pcs[i] = -1;
			my_pcb->signal_1_pcs[i] = -1;
		}
    }
}

/*
 * Getters and setters (same order as defined in structs in pcb.h)
 */
unsigned int getPC(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->context->pc;
    }
}

// Setter to set the pc field of the pcb struct
int setPC(PCB_p my_pcb, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->context->pc = newValue;
    }
}

// Getter to get the ir field of the pcb struct
unsigned int getIR(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->context->ir;
    }
}

// Setter to set the ir field of the pcb struct
int setIR(PCB_p my_pcb, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->context->ir = newValue;
    }
}

unsigned int getPSR(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->context->psr;
    }
}

// Setter to set the psr field of the pcb struct
int setPSR(PCB_p my_pcb, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->context->psr = newValue;
    }

}

/**
 * Getter for registers.
 * @param my_pcb
 * @param reg specifies which register to retrieve.
 * @return
 */
unsigned int getRegister(PCB_p my_pcb, unsigned int reg) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        if (reg < 0 || reg > 7) {
            printf("Error: Invalid register.");
            return NULL_ERROR;
        }
        switch (reg) {
            case 0:
                return my_pcb->context->r0;
            case 1:
                return my_pcb->context->r1;
            case 2:
                return my_pcb->context->r2;
            case 3:
                return my_pcb->context->r3;
            case 4:
                return my_pcb->context->r4;
            case 5:
                return my_pcb->context->r5;
            case 6:
                return my_pcb->context->r6;
            case 7:
                return my_pcb->context->r7;
        }
    }
}

/*
 * Setter for registers 0-7.
 * @param reg specifies which register to set
 * @param newValue specifies which value to give to the register.
 */
int setRegister(PCB_p my_pcb, int reg, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        if (reg < 0 || reg > 7) {
            printf("Error: Invalid register.");

        }
        switch (reg) {
            case 0:
                my_pcb->context->r0 = newValue;
                break;
            case 1:
                my_pcb->context->r1 = newValue;
                break;
            case 2:
                my_pcb->context->r2 = newValue;
                break;
            case 3:
                my_pcb->context->r3 = newValue;
                break;
            case 4:
                my_pcb->context->r4 = newValue;
                break;
            case 5:
                my_pcb->context->r5 = newValue;
                break;
            case 6:
                my_pcb->context->r6 = newValue;
                break;
            case 7:
                my_pcb->context->r7 = newValue;
                break;
        }
    }
}

// Getter to get the pid field of the pcb struct
unsigned int getPid(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->pid;
    }
}

// no setPid

// Getter to get the state field of the pcb struct
enum state_type getState(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->state;
    }
}

// Setter to set the state field of the pcb struct
int setState(PCB_p my_pcb, enum state_type newState) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->state = newState;
    }
}

// Getter to get the state field of the pcb struct
enum process_type getType(PCB_p my_pcb) {
	if (!my_pcb) {
		return NULL_ERROR;
	}
	else {
		return my_pcb->type;
	}
}

// Setter to set the type field of the pcb struct
int setType(PCB_p my_pcb, enum process_type newType) {
	if (!my_pcb) {
		return NULL_ERROR;
	}
	else {
		my_pcb->type = newType;
	}
}

// Getter to get the parent field of the pcb struct
unsigned int getParent(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->parent;
    }
}

// Setter to set the parent field of the pcb struct
int setParent(PCB_p my_pcb, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->parent = newValue;
    }
}

// Getter to get the Priority field of the pcb struct
unsigned char getPriority(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->priority;
    }
}

// Setter to set the Priority field of the pcb struct
int setPriority(PCB_p my_pcb, unsigned char newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        if ((newValue < 0) || (newValue > 15)) {
            printf("Error: Priority can only be between 0 and 15.");
            //return NULL_ERROR;
        } else {
            my_pcb->priority = newValue;
        }
    }
}

/**
 *
 * Method to set random priority level between 0 and 15.
 * @param my_pcb
 * @return
 */
int setRandomPriority(PCB_p my_pcb) {
    srand(time(NULL));
    int random = rand() % 16;
    my_pcb->priority = random;
}

// Setter to set the mem field of the pcb struct
int setMem(PCB_p my_pcb, unsigned char* newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->mem = newValue;
    }
}

// Getter to get the mem field of the pcb struct
unsigned char* getMem(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL;
    } else {
        return my_pcb->mem;
    }
}

// Setter to set the size field of the pcb struct
int setSize(PCB_p my_pcb, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->size = newValue;
    }
}

// Getter to get the size field of the pcb struct
unsigned char getSize(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->size;
    }
}

// Getter to get the size field of the pcb struct
unsigned char getChannelNo(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->channel_no;
    }
}

// Setter to set the size field of the pcb struct
int setChannelNo(PCB_p my_pcb, unsigned char newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->priority = newValue;
    }
}

// Added for Problem 3
//Getter to see if pcb is privileged
// Used to help track if a PCB is privileged for termination checks
int getPrivileged(PCB_p my_pcb) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        return my_pcb->privileged;
    }
}

// Added for Problem 3
//Setter to set the privilege state of the pcb
// Used to help track if a PCB is privileged for termination checks
int setPrivileged(PCB_p my_pcb, unsigned int newValue) {
    if (!my_pcb) {
        return NULL_ERROR;
    } else {
        my_pcb->privileged = newValue;
    }
}

// sets a random value for the processes terminate
int setRandomTerminate(PCB_p my_pcb) {
    int random = rand() % TERM_COUNT_RANGE;
    my_pcb->terminate = random;
}

// sets a random maxpc
int setRandomMaxPC(PCB_p my_pcb) {
    int random = rand() % MAX_PC_RANGE;
    my_pcb->maxpc = random;
}

// a setter for terminate
int setTerminate(PCB_p my_pcb, int term) {
    my_pcb->terminate = term;
}

// getter for terminate
unsigned int getTerminate(PCB_p my_pcb) {
    return my_pcb->terminate;
}

// gets the processes max pc
unsigned int getMaxPC(PCB_p my_pcb) {
    return my_pcb->maxpc;
}

// setter for the term count
int setTermCount(PCB_p my_pcb, unsigned int newValue) {
    my_pcb->term_count = newValue;
}

// getter for the term count
unsigned int getTermCount(PCB_p my_pcb) {
    return my_pcb->term_count;
}

// sets random values for both sets of IO traps
int setRandomIOTraps(PCB_p my_pcb) {
    int i, random, increment1, increment2;
    for(i=0; i<IO_TRAP_SIZE; i++) {
        random = rand() % TRAP_STEP_RANGE;
        increment1 = increment2 = (i + 1) * random;
        my_pcb->io_1_traps[i] = increment1;

        while(increment1 == increment2) {
            random = rand() % TRAP_STEP_RANGE;
            increment2 = (i + 1) * random;
        }
        my_pcb->io_2_traps[i] = increment2;
    }
}

// getter for the IO1 Traps array
int* getIOTraps1(PCB_p my_pcb) {
    return my_pcb->io_1_traps;
}

// getter for the IO2 Traps array
int* getIOTraps2(PCB_p my_pcb) {
    return my_pcb->io_2_traps;
}

// initializer to set the process as the idle process
int initializeAsIdleProcess(PCB_p pcb) {
    if (!pcb) return NULL_ERROR;
    
    initialize_pcb(pcb);
    pcb->pid = IDLE_PROCESS_PID;
    currentpid--;
    
}

// checks if the process is the idle proces
int isIdleProcess(PCB_p pcb) {
   if (!pcb) return NULL_ERROR;
   
   if (pcb->pid == IDLE_PROCESS_PID) return 1;
   
   return 0;
}

// sets the creation time of the process
int setCreation(PCB_p my_pcb, time_t creation_time){
    my_pcb->creation = creation_time;
}

// getter for the creation time of the process
time_t getCreation(PCB_p my_pcb) {
    return my_pcb->creation;
}

// sets the termination time of the process
int setTermination(PCB_p my_pcb, time_t termination_time) {
    my_pcb->termination = termination_time;
}

// getter for the termination time of the process
time_t getTermination(PCB_p my_pcb) {
    return my_pcb->termination;
}

//toString method for PCB
int pcb_toString(PCB_p my_pcb, char* buffer, int b_size) {
    if (!my_pcb || !buffer) return NULL_ERROR;

    sprintf(buffer, "PCB object: PID: 0x%X, Priority: 0x%X, mem: 0x%X state: %u, parent process: 0x%X,  "
            "size: %u, channel number: %u ", my_pcb->pid, (unsigned int)my_pcb->priority, my_pcb->mem
    ,my_pcb->state, my_pcb->parent, my_pcb->size, (unsigned int)my_pcb->channel_no);

    return IS_NULL;
}

// Added for problem 3
// A smaller to string to match the format shown in the Problem documentation
int pcb_toSimpleString(PCB_p my_pcb, char* buffer, int b_size) {
    if (!my_pcb || !buffer) return NULL_ERROR;

    sprintf(buffer, "PCB: PID: %u, PRIORITY: %u, STATE: %d, PC %u, MAX_PC %u", my_pcb->pid, 
            (unsigned int)my_pcb->priority, my_pcb->state, my_pcb->context->pc, my_pcb->maxpc);

    return IS_NULL;
}
// toString method for context
int context_toString(PCB_p my_pcb, char* buffer, int b_size) {
    if (!my_pcb || !buffer) return NULL_ERROR;

    sprintf(buffer, "Context contents: PC: 0x%04X, IR: 0x%X, PSR: 0x%X, "
            "R0: 0x%X, R1: 0x%X, R2: 0x%X, R3: 0x%X, R4: 0x%X, R5: 0x%X, R6: 0x%X, R7: 0x%X \n",
            my_pcb->context->pc, my_pcb->context->ir, my_pcb->context->psr,
            my_pcb->context->r0, my_pcb->context->r1, my_pcb->context->r2,
            my_pcb->context->r3, my_pcb->context->r4, my_pcb->context->r5,
            my_pcb->context->r6, my_pcb->context->r7);

    return IS_NULL;
}
