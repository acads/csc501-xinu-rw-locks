/* adhanas */
/* linit.c -- PA2, lock initialization */

#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

struct rwlock ltab[NLOCKS]; /* rwlock table */
int nlocks;     /* total # of active rw locks in the system */
int next_lock;  /* next available lock iD */


/* Name: linit
 * 
 * Desc: Initializes the rwlock table. Sets the head and tail pointers for both
 *       read and write waitlists for all the locks. This will be retained
 *       throughout the system and *never change* them.
 *
 * Params: 
 *  None
 *
 * Returns:
 *  Nothing
 */
void
linit(void)
{
    int lid = 0;
    struct rwlock *lptr = NULL;
    DTRACE_START;

    next_lock = (NLOCKS - 1);
    for (lid = 0; lid < NLOCKS; ++lid) {
        lptr = L_GET_LPTR(lid);
        lptr->lstate = LS_UNINIT;
        lptr->lrtq = 1 + (lptr->lrhq = newqueue());
    }
    for (lid = 0; lid < NLOCKS; ++lid) {
        lptr = L_GET_LPTR(lid);
        lptr->lwtq = 1 + (lptr->lwhq = newqueue());
    }

    DTRACE_END;
    return;
}


/* Name: l_init_lid
 *
 * Desc: Intializes the given lid's entry in rwlock table. Comes handy when 
 *       the lid is being resued.
 *
 * Params: 
 *  lid             - lock ID to be initialized
 *  change_state    - flag to change the state to UNINIT
 *
 * Returns: 
 *  Nothing
 */
void
l_init_lid(int lid, int change_state)
{
    struct rwlock *lptr = NULL;
    DTRACE_START;

    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RETURN;
    }

    lptr = L_GET_LPTR(lid);

    /* Do not touch head & tail pointer here. Same goes for delpidmap. */
    if (TRUE == change_state)
        lptr->lstate = LS_UNINIT;
    lptr->naread = lptr->nawrite =
        lptr->nwread = lptr->nwwrite = 0;
    lptr->nextq = lptr->nextrw = lptr->nextpid = EMPTY;
    lptr->hpid = lptr->hprio = EMPTY;
    bzero(lptr->relpidmap, (7 * sizeof(unsigned char)));
    DTRACE("DBG$ %d %s> lid %d initialized\n", currpid, __func__, lid);

RETURN:
    DTRACE_END;
    return;
}

