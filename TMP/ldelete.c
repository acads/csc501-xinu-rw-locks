/* adhanas */
/* ldelete.c -- PA2, rwlock deletion */

#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/* Name: ldelete
 *
 * Desc: Deletes the specified lid from the system, if it's still not delted.
 *       In there are any procs waiting for this lid, this routine wakes them up
 *       and returns 'DELETED'. If some process tries to delete the lid again, 
 *       returns an error. 
 *
 * Params:
 *  lid     - lock ID to be deleted
 * 
 * Returns:
 *  OK      - in case of success
 *  SYSERR  - if the lock is deleted already
 */
int
ldelete(int lid)
{
    int pid = 0;
    uint32_t i = 0;
    int resched_flag = FALSE;
    int lstate = EMPTY;
    struct rwlock *lptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;
    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RESTORE_AND_ERROR;
    }

    lptr = L_GET_LPTR(lid);
    lstate = L_GET_LSTATE(lid);
    switch (lstate) {
        case LS_UNINIT:
        case LS_DELETED:
            /* A lock cannot be deleted in these states. */
            DTRACE("DBG$ %d %s> cannot delete lid %d in state %s\n",    \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            goto RESTORE_AND_ERROR;
            break;

        case LS_FREE:
            DTRACE("DBG$ %d %s> deleted lock %d\n", currpid, __func__, lid);
            DTRACE("DBG$ %d %s> lid %d state changed from %s to UNINIT\n",  \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            L_SET_LSTATE(lid, LS_UNINIT);
            l_init_lid(lid, TRUE);
            goto RESTORE_AND_OK;
            break;

        case LS_READ:
            L_DEC_PLOCKS(pid);
            DTRACE("DBG$ %d %s> lock dec'd for pid %d, total # of locks %d\n", \
                    currpid, __func__, pid, L_GET_PLOCKS(pid));
            DTRACE("DBG$ %d %s> lid %d state changed from %s to DELETED\n", \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            L_SET_LSTATE(lid, LS_DELETED);
            DTRACE("DBG$ %d %s> deleted lock %d\n", currpid, __func__, lid);
            break;

        case LS_WRITE:
            L_DEC_PLOCKS(pid);
            DTRACE("DBG$ %d %s> lock dec'd for pid %d, total # of locks %d\n", \
                    currpid, __func__, pid, L_GET_PLOCKS(pid));
            DTRACE("DBG$ %d %s> lid %d state changed from %s to DELETED\n", \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            L_SET_LSTATE(lid, LS_DELETED);
            DTRACE("DBG$ %d %s> deleted lock %d\n", currpid, __func__, lid);
            DTRACE("DBG$ %d %s> deleted lock %d\n", currpid, __func__, lid);
            break;

        default:
            DTRACE("DBG$ %d %s> lid %d bad state %s\n",     \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            ASSERT(0);
            break;
    }

#ifdef DBG_ON
    DTRACE("DBG$ %d %s> printing lid %d details...\n", currpid, __func__, lid);
    l_print_lock_details(lid);
#endif /* DBG_ON */

    if (0 != L_GET_NWREAD(lid)) {
        /* Wake up all waiting readers on this lock and set the wait retval
         * to DELETED to notify them that the lock has been delted.
         */
        resched_flag |= TRUE;
        DTRACE("DBG$ %d %s> readying procs in read list...\n",    \
                currpid, __func__);
        while (EMPTY != (pid = getfirst(lptr->lrhq))) {
            proctab[pid].pwaitret = DELETED;
            ready(pid, RESCHNO);
            DTRACE("DBG$ %d %s> pid %d ready from read list\n",     \
                    currpid, __func__, pid);
        }
    } else {
        DTRACE("DBG$ %d %s> no waiting readers for lid %d\n",    \
                currpid, __func__, lid);
    }

    if (0 != L_GET_NWWRITE(lid)) {
        /* Wake up all waiting writers on this lock and set the wait retval
         * to DELETED to notify them that the lock has been delted.
         */
        resched_flag |= TRUE;
        DTRACE("DBG$ %d %s> readying procs in write list...\n",    \
                currpid, __func__);
        while (EMPTY != (pid = getfirst(lptr->lwhq))) {
            proctab[pid].pwaitret = DELETED;
            ready(pid, RESCHNO);
            DTRACE("DBG$ %d %s> pid %d ready from write list\n",     \
                    currpid, __func__, pid);
        }
    } else {
        DTRACE("DBG$ %d %s> no waiting writers for lid %d\n",    \
                currpid, __func__, lid);
    }

    l_init_lid(lid, FALSE);
    if (TRUE == resched_flag) {
        resched();
    }
    goto RESTORE_AND_OK;

RESTORE_AND_OK:
    /* Record all the pid's activities on this lid to delpidmap which will be
     * used to ignore processes' lid operation requests on this lid. This is
     * done so that the processes can come to know that the lock is not valid
     * anymore.
     */
    for (i = 0; i < L_NPIDMAPS; ++i) {
        lptr->delpidmap[i] |= lptr->logpidmap[i];
    }
    restore(ps);
    DTRACE_END;
    return OK;

RESTORE_AND_ERROR:
    restore(ps);
    DTRACE_END;
    return SYSERR;
}
