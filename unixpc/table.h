/* definition of states */
#define PRISTINE 0 /* not executed even once */
#define LOADING  1 /* executed once, has headers loaded */
#define PREGNANT 2 /* executed twice, has a file loaded */
#define WRITE_SELF 3 /* ready to write an executable image of self to disk */
#define ENTRY 4 /* pseudo-state: in this state at beginning of each execution */
#define GONZO 5 /* panic exit state: something gone wrong */
#define QUIT 6 /* pseudo-state: leaves state machine */

/* transition table:  the next state to enter depends on two parameter,
 * "loaded", and "headers", and the state that machine is in right now. */

/*
 * table organized by:
 * last state, headers, loaded
 *  (0-5),      (0,1),   (0,1)
 */
static int what_next[6][2][2] = {
{ { GONZO, WRITE_SELF }, { GONZO, GONZO },
},

{ { WRITE_SELF, LOADING }, { PRISTINE, GONZO },
},

{ { GONZO, LOADING }, { GONZO, GONZO },
},

{ { QUIT, QUIT }, { GONZO, GONZO },
},

{ { PREGNANT, LOADING }, { GONZO, PRISTINE },
},

{ { QUIT, QUIT }, { QUIT, QUIT },
}

};

/*
 * the transition table is deliberately cowardly:
 * it enters the GONZO state when a combo of parameters that could
 * foreseeably be corrected, but is nonetheless wrong, is encountered.
 */
