/* adhanas */
/* lutils.c -- PA2, rw locks, util routines */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <q.h>
#include <lock.h>


/* Printable lock states */
const char *l_states_str[] = { 
    "Uninitialized",
    "Free",
    "Read",
    "Write",
    "Deleted"
};


/* Printable process states */
const char *l_pstates_str[] = { 
    "Current",
    "Free",
    "Ready",
    "Recv",
    "Sleep",
    "Suspended",
    "Waiting",
    "Time recv"
};


/* pidmap types in lid */
const char *l_pidmap_types_str[] = {
    "Delete",
    "Release",
    "Log",
    "Plid"
};


/* Lock request types */
const char *l_lock_types[] = {
    "Read",
    "Write"
};


/* Name: l_clear_pidmaps
 *
 * Desc: Clears all the entries corresponding to the pid in all available
 *       lid's pidmaps. This is usually called when a process is killed.
 *
 * Params: 
 *  pid     - process ID of the process whose pid-bit is to be cleared
 *
 * Returns: int
 *  OK      - on success
 *  SYSERR  - on error
 */ 
int
l_clear_pidmaps(int pid)
{
    int lid = EMPTY;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_ERROR;
    }

    /* Go thru all the locks and clear the pid-bit on all the pidmaps. */
    for (lid = 0; lid < NLOCKS; ++lid) {
        l_pidmap_oper(lid, pid, L_MAP_DELETE, L_MAP_CLR);
        l_pidmap_oper(lid, pid, L_MAP_RELEASE, L_MAP_CLR);
        l_pidmap_oper(lid, pid, L_MAP_LOG, L_MAP_CLR);
    }
    DTRACE_END;
    restore(ps);
    return OK;

RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/* Name: l_pidmap_oper
 *
 * Desc: Sets, clears, toggles and checks the given bit on the given pidmap.The
 *       pidmaps are ususally used for lock-pid book-keeping.
 *
 * Params:
 *  lid         - lock ID
 *  pid         - process ID
 *  map_type    - release map or delete map or log map
 *  map_oper    - set or clear or toggle or check
 *
 * Returns: int
 *  TRUE        - for all operations but for check (TRUE only if bit is set)
 *  FALSE       - if the bit is not set for check oper
 *  SYS_ERR     - in case of any errors (bad pid, bad lid and so on)
 */
int
l_pidmap_oper(int lid, int pid, int map_type, int map_oper)
{
    int bit = -1;
    int index = -1;
    uint32_t ret_val = OK;
    unsigned char *map_ptr = NULL;
    struct pentry *pptr = NULL;
    struct rwlock *lptr = NULL;

    //DTRACE_START;
    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RETURN_ERROR;
    }   

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_ERROR;
    }
    pptr = L_GET_PPTR(pid);
    lptr = L_GET_LPTR(lid);

    /* Calcualte the required index and bit postion. */
    if (L_MAP_PLID == map_type) {
        index = lid / L_BIT_RANGE;
        bit = lid % L_BIT_RANGE;
    } else {
        index = pid / L_BIT_RANGE;
        bit = pid % L_BIT_RANGE;
    }

    /* Assign the appropriate bitmap based on the type and index. */
    switch (map_type) {
        case L_MAP_DELETE:
            map_ptr = &(lptr->delpidmap[index]);
            break;

        case L_MAP_RELEASE:
            map_ptr = &(lptr->relpidmap[index]);
            break;

        case L_MAP_LOG:
            map_ptr = &(lptr->logpidmap[index]);
            break;

        case L_MAP_PLID:
            map_ptr = &(pptr->plidmap[index]);
            break;

        default:
            DTRACE("DBG$ %d %s> bad map type %d\n",    \
                    currpid, __func__, map_type);
            ASSERT(0);
            goto RETURN_ERROR;
            break;
    }
#if 0
    if (L_MAP_DELETE == map_type)
        map_ptr = &(lptr->delpidmap[index]);
    else if (L_MAP_RELEASE == map_type)
        map_ptr = &(lptr->relpidmap[index]);
    else if (L_MAP_LOG == map_type)
        map_ptr = &(lptr->logpidmap[index]);
    else {
        DTRACE("DBG$ %d %s> bad map type %d\n",    \
                currpid, __func__, map_type);
        ASSERT(0);
        goto RETURN_ERROR;
    }
#endif

    switch (map_oper) {
        case L_MAP_SET:
            *map_ptr = (*map_ptr | (1 << bit));
            DTRACE("DBG$ %d %s> lid %d setting index %d, bit %d of type %s\n",\
                    currpid, __func__, lid, index, bit,  \
                    l_pidmap_types_str[map_type - 1]);
            goto RETURN;
            break;

        case L_MAP_CLR:
            *map_ptr = (*map_ptr & ~(1 << bit));
#if 0
            DTRACE("DBG$ %d %s> lid %d clearing index %d, bit %d of type %s\n", \
                    currpid, __func__, lid, index, bit,  \
                    l_pidmap_types_str[map_type - 1]);
#endif
            goto RETURN;
            break;

        case L_MAP_CHK:
            ret_val = (*map_ptr & (1 << bit));
            ret_val = (ret_val ? TRUE : FALSE);
            DTRACE("DBG$ %d %s> lid %d checking index %d, bit %d of "     \
                    "type %s: %s\n", currpid, __func__, lid, index, bit,     \
                    l_pidmap_types_str[map_type - 1],
                    ((TRUE == ret_val) ? "set" : "unset"));
            goto RETURN;
            break;

        case L_MAP_TGL:
            *map_ptr = (*map_ptr ^ (1 << bit));
            break;

        default:
            DTRACE("DBG$ %d %s> bad oper type %d\n",   \
                    currpid, __func__, map_oper);
            break;
    }

RETURN:
    //DTRACE_END;
    return ret_val;

RETURN_ERROR:
    //DTRACE_END;
    return SYSERR;
}


/* Name: l_recal_next
 *
 * Desc: Finds the next pid and the wait queue (read or write) if a process 
 *       should unlock this lock. This will be called everytime a process is 
 *       enqueued on the wait queue.
 *
 * Params:
 *  lid     - lokc ID
 *
 * Returns:
 *  Nothing
 */
void
l_recal_next(int lid)
{
    int nread = 0;
    int nwrite = 0;
    int rprio = 0;
    int wprio = 0;
    uint32_t rtime = 0;
    uint32_t wtime = 0;
    struct rwlock *lptr = NULL;

    DTRACE_START;

    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        DTRACE_END;
        return;
    }

    lptr = L_GET_LPTR(lid);
    nread = L_GET_NWREAD(lid);
    nwrite = L_GET_NWWRITE(lid);
    rprio = L_GET_RPRIO(lid);
    wprio = L_GET_WPRIO(lid);
    rtime = L_GET_RTIME(lid);
    wtime = L_GET_WTIME(lid);

    if ((0 == nread) && (0 != nwrite)) {
        /* No waiting readers. So, writers win, obiviously. */
        DTRACE("%d %s: no readers, but writers %d\n",   \
                currpid, __func__, nwrite);
        goto WRITERS_WIN;
    } else if ((0 != nread) && (0 == nwrite)) {
        /* No waiting writers. So, readers win, obviously. */
        DTRACE("%d %s: no writers, but readers %d\n",   \
                currpid, __func__, nread);
        goto READERS_WIN;
    } else if ((0 != nread) && (0 != nwrite)) {
            DTRACE("%d %s: writers: %d, readers: %d\n",  \
                    currpid, __func__, nwrite, nread);
            DTRACE("%d %s: wprio: %u, rprio: %u\n", \
                    currpid, __func__, wprio, rprio);
        if (rprio > wprio) {
            goto READERS_WIN;
        } else if (rprio < wprio) {
            goto WRITERS_WIN;
        } else if (rprio == wprio) {
            DTRACE("%d %s: writers prio = readers prio = %d\n",     \
                    currpid, __func__, wprio);
            DTRACE("%d %s: wtime: %u, rtime: %u\n",    \
                    currpid, __func__, wtime, rtime);
            if (wtime > rtime) {
                if ((wtime - rtime) < 1000) {
                    /* Reader arrived earlier, but writer arrived within 1s.
                     * So, writer wins.
                     */
                    goto WRITERS_WIN;
                } else {
                    /* Reader arrived earlie and writer arrived after 1s.
                     * Reader wins.
                     */
                    goto READERS_WIN;
                }
            } else if (wtime <= rtime) {
                /* Writer arrived earlier or at the same time as that of the
                 * reader. Writer wins, again.
                 */
                goto WRITERS_WIN;
            }
        }
    } else if ((0 == nread) && (0 == nwrite)) {
        /* Well, we're in a soup. This should never ever happen. */
        /* No more waiters! Set all next* items to EMPTY and return */
        DTRACE("DBG$ %d %s> nwread = nwwrite = 0!\n", currpid, __func__);
        goto SET_EMPTY;
    }

SET_EMPTY:
    lptr->nextq = lptr->nextrw = lptr->nextpid = EMPTY;
    DTRACE("DBG$ %d %s> wait list is empty\n", currpid, __func__, lptr->nextpid);
    DTRACE_END;
    return;

READERS_WIN:
    lptr->nextq = L_GET_RPID(lid);
    lptr->nextrw = LT_READ;
    lptr->nextpid = q[L_GET_LRTAIL(lid)].qprev;
    DTRACE("DBG$ %d %s> reader %d won\n", currpid, __func__, lptr->nextpid);
    DTRACE_END;
    return;

WRITERS_WIN:
    lptr->nextq = L_GET_WPID(lid);
    lptr->nextrw = LT_WRITE;
    lptr->nextpid = q[L_GET_LWTAIL(lid)].qprev;
    DTRACE("DBG$ %d %s> writer %d won\n", currpid, __func__, lptr->nextpid);
    DTRACE_END;
    return;
}


/* Name: l_new_lid
 * 
 * Desc: Scans the lock table and returns the next available free lock. 
 *
 * Params: 
 *  None
 *
 * Returns: int
 *      Next available lock ID on success 
 *      SYSERR on failure.
 */
int
l_new_lid (void)
{
    int i = 0;
    int ret_lid = 0;

    DTRACE_START;
    for (i = 0; i < NLOCKS; ++i) {
        /* Run thru all available lock IDs until we find one which we can
         * use, i.e., a lock whose state is either LS_UNINIT or LS_DELETED.
         */
        ret_lid = next_lock--;
        if (next_lock < 0) {
            DTRACE("DBG$ %d %s> next_lock overflowed.. resetting next_lock"\
                   " to %d\n", currpid, __func__, (NLOCKS - 1));
            next_lock = (NLOCKS - 1);
        }

        if (LS_UNINIT == L_GET_LSTATE(ret_lid) || 
                (LS_DELETED == L_GET_LSTATE(ret_lid))) {
            DTRACE("DBG$ %d %s> lid %d state changed from %s to Free %d\n",  \
                    currpid, __func__, ret_lid, L_GET_LSTATESTR(ret_lid));
            L_SET_LSTATE(ret_lid, LS_FREE);
            DTRACE("DBG$ %d %s> returning lock %d\n",  \
                    currpid, __func__, ret_lid);
            DTRACE_END;
            return ret_lid;
        }
    }
    DTRACE("DBG$ %d %s> no more free locks\n", currpid, __func__);
    DTRACE_END;
    return SYSERR;
}


/* Name: l_get_next_readers
 * 
 * Desc: Gets the next set of reader(s) to whom the read lock should be awarded. 
 *       It goes through the list and makes the procs ready, if any and returns 
 *       TRUE. If eligible readers are not available, it just returns FALSE.
 *
 * Params:
 *  lid     - read lock ID
 * 
 * Returns: int
 *  TRUE    - if eligible readers are available
 *  FLASE   - otherwise
 */
int
l_get_next_readers(int lid)
{
    int next_pid = EMPTY;
    int nextrw = L_GET_NEXTRW(lid);

    /* Set the lstate based on the next available waiter. */
    if (LT_READ == nextrw) {
        DTRACE("DBG$ %d %s> lid %d state changed from %s to Read\n",    \
                currpid, __func__, currpid, L_GET_LSTATESTR(lid));
        L_SET_LSTATE(lid, LS_READ);
    } else if (EMPTY == nextrw) {
        DTRACE("DBG$ %d %s> lid %d state changed from %s to Free\n",    \
                currpid, __func__, currpid, L_GET_LSTATESTR(lid));
        L_SET_LSTATE(lid, LS_FREE);
    } else {
        DTRACE("DBG$ %d %s> bad nextrw %d for lid %d - should have been "   \
                " READ or EMPTY\n",                                         \
                currpid, __func__, currpid, lid, next_pid);
        ASSERT(0);
        goto RETURN_FALSE;
    }

    /* Fetch all possible readers. */
    while (LT_READ == L_GET_NEXTRW(lid)) {
        if (EMPTY == (next_pid = getlast(L_GET_LRTAIL(lid)))) {
            DTRACE("DBG$ %d %s> bad nextq %d for lid %d\n",    \
                    currpid, __func__, currpid, lid, next_pid);
            break;
        }
        DTRACE("DBG$ %d %s> next reader pid %d for lid %d\n",    \
                currpid, __func__, currpid, next_pid, lid);
        L_DEC_NWREAD(lid);
        L_INC_NAREAD(lid);
        ready(next_pid, RESCHNO);
        l_pidmap_oper(lid, next_pid, L_MAP_RELEASE, L_MAP_SET);
        DTRACE("DBG$ %d %s> lid %d next reader lock for pid %d\n", \
                currpid, __func__, lid, next_pid);
        l_recal_next(lid);
    }

    DTRACE_END;
    return TRUE;

RETURN_FALSE:
    DTRACE_END;
    return FALSE;
}


/* Name: l_get_next_writer
 *
 * Desc: Gets the next writer to whom the write lock should be awarded. It goes
 *       through the list and makes the proc ready, if any and returns TRUE. If
 *       eligible writers are not available, it just returns FALSE.
 *
 * Params:
 *  lid     - read lock ID
 * 
 * Returns: int
 *  TRUE    - if eligible writer is available
 *  FLASE   - otherwise
 */
int
l_get_next_writer(int lid)
{
    int next_pid = EMPTY;;
    int nextrw = L_GET_NEXTRW(lid);
    DTRACE_START;

    /* Set the lstate based on the next available waiter. */
    if (LT_WRITE == nextrw) {
        DTRACE("DBG$ %d %s> lid %d state changed from %s to Write\n",   \
                currpid, __func__, currpid, L_GET_LSTATESTR(lid));
        L_SET_LSTATE(lid, LS_WRITE);
    } else if (EMPTY == nextrw) {
        DTRACE("DBG$ %d %s> lid %d state changed from %s to Free\n",    \
                currpid, __func__, currpid, L_GET_LSTATESTR(lid));
        L_SET_LSTATE(lid, LS_FREE);
    } else {
        DTRACE("DBG$ %d %s> bad nextrw %d for lid %d - should have been "   \
                " WRITE or EMPTY\n",                                        \
                currpid, __func__, currpid, lid, next_pid);
        ASSERT(0);
        goto RETURN_FALSE;
    }

    if (EMPTY == (next_pid = getlast(L_GET_LWTAIL(lid)))) {
#ifdef DBG_ON
        l_print_lock_details(lid);
#endif /* DBG_ON */
        DTRACE("DBG$ %d %s> bad pid %d in nextq tail %d for lid %d\n",  \
                currpid, __func__, next_pid, L_GET_NEXTQ(lid), lid);
        ASSERT(0);
        goto RETURN_FALSE;
    }

    L_DEC_NWWRITE(lid);
    L_INC_NAWRITE(lid);

    /* Enqueue the waiting writer in ready queue. */
    ready(next_pid, RESCHNO);
    l_pidmap_oper(lid, next_pid, L_MAP_RELEASE, L_MAP_SET);
    DTRACE("DBG$ %d %s> lid %d next writer lock for pid %d\n",  \
            currpid, __func__, lid, next_pid);
    l_recal_next(lid);

    DTRACE_END;
    return TRUE;

RETURN_FALSE:
    DTRACE_END;
    return FALSE;
}


/* Name: l_is_lid_stale_for_pid
 *
 * Desc: Checks whether the lid is stale for the given pid. This happens when
 *       a process hasn't acquired a lock after create/release for a long time
 *       and the lock Id has been overflown in the system.
 *
 * Params: 
 *  lid     - lock ID (as known by the old proc)
 *  pid     - proc ID of the old proc
 *
 * Returns: int
 *  TRUE    - if the lid has been overflown for the given pid
 *  FALSE   - if the lid is still valid for the given pid
 */
int
l_is_lid_stale_for_pid(lid, pid)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    /* Check the pid in the lid's delpidmap. If present, our guy still thinks
     * that he owns the lid. Well, he doesn't. Return TRUE.
     */
    if (TRUE == l_pidmap_oper(lid, pid, L_MAP_DELETE, L_MAP_CHK)) {
        DTRACE("DBG$ %d %s> lid %d is stale for pid %d\n",  \
                currpid, __func__, lid, pid);;
        DTRACE_END;
        restore(ps);
        return TRUE;
    }

    DTRACE("DBG$ %d %s> lid %d is not stale for pid %d\n",  \
            currpid, __func__, lid, pid);;
    DTRACE_END;
    restore(ps);
    return FALSE;
}


/* Name:    l_remove_pid 
 *
 * Desc:    Removes the given pid from the waitlist of the lid, if it happens
 *          to be waiting on a lid. This is called when a proc is killed.
 *
 * Params: 
 *  pid     - proc ID which has to be removed from a lid's waitlist
 *
 * Returns: int
 *  TRUE    - if the pid is removed successfully from the waitlist of the lid
 *  FALSE   - otherwise
 */ 
int 
l_remove_pid(int pid)
{
    int plid = EMPTY;
    int plid_type = EMPTY;
    int iter_pid = EMPTY;
    int tmp_head = EMPTY;
    int tmp_tail = EMPTY;
    int hpid = EMPTY;
    struct pentry *pptr = NULL;
    struct pentry *tmp_pptr = NULL;
    struct rwlock *lptr = NULL;
    STATWORD ps;
    int new_prio = EMPTY;
    int new_pid = EMPTY;
    int tmp_pid = EMPTY;
    int pprio = EMPTY;
    
    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_ERROR;
    }
    pptr = L_GET_PPTR(pid);
    pprio = L_GET_PPRIO(pid);
    plid = L_GET_PLID(pid);
    plid_type = L_GET_PLIDTYPE(pid);
    hpid = L_GET_HPID(plid);
    DTRACE("incoming pid: %d\n", pid);
    DTRACE("DBG$ %d %s> plid: %d, plid_type: %d, hpid: %d\n",   \
            currpid, __func__, plid, L_GET_PLIDTYPE(pid), hpid);

    /* If the pid is not in any of the waitlists, return OK. */
    if (PRWAIT != L_GET_PSTATE(pid) || (EMPTY == plid)) {
        DTRACE("DBG$ %d %s> pid %d, not currently waiting on any lid\n",    \
                currpid, __func__, pid);
        goto RETURN_OK;
    }
    lptr = L_GET_LPTR(plid);

#ifdef DBG_ON
    DTRACE("incoming pid\n", NULL);
    l_print_pid_details(pid);
    DTRACE("hpid\n", NULL);
    l_print_pid_details(hpid);
    DTRACE("waiting on lid \n", NULL);
    l_print_lock_details(plid);
#endif

    /* Check both read & wait lists. If found in one, it will not be in the
     * other list.
     */
    if (LT_READ == L_GET_PLIDTYPE(pid)) {
        tmp_head = L_GET_RHEAD(plid);
        tmp_tail = L_GET_LRTAIL(plid);
    } else if (LT_WRITE == L_GET_PLIDTYPE(pid)) {
        tmp_head = L_GET_WHEAD(plid);
        tmp_tail = L_GET_LWTAIL(plid);
    } else {
        /* Soup. */
        DTRACE("DBG$ %d %s> pid %d, is currently waiting on lid %d, but "   \
                "lidtype is not set\n", currpid, __func__, pid, plid);
        ASSERT(0);
        goto RETURN_ERROR;
    }

    /* If the lid on which this pid is waiting on has inherited the pid's
     * priority, set it to the next highest priority on the list (if available) 
     * or set it to its original priority.
     */
    if (L_GET_HPID(plid) == pid) {
        l_get_next_prio(plid, pid, &new_pid, &new_prio);
        DTRACE("DBG$ %d %s> new_pid: %d,new_prio: %d\n\n",   \
                currpid, __func__, new_pid, new_prio);
        lptr->hpid = new_pid;
        lptr->hprio = new_prio;
        DTRACE("DBG$ %d %s> plid: %d, new hprio: %d, new hpid: %d\n",   \
                currpid, __func__, plid, lptr->hprio, lptr->hpid);

        for (tmp_pid = 0; tmp_pid < NPROC; ++tmp_pid) {
            tmp_pptr = L_GET_PPTR(tmp_pid);
            /* Reset the priorities of all the pids that holds the lid. */
            if (TRUE == l_pidmap_oper(plid, tmp_pid, L_MAP_PLID, L_MAP_CHK) &&
                    (pprio == L_GET_PPRIO(tmp_pid))) {

                if (TRUE == L_GET_PRIOFLAG(tmp_pid)) {
                    if (EMPTY == new_prio) {
                        tmp_pptr->prioflag = FALSE;
                        tmp_pptr->pprio = tmp_pptr->oprio;
                        tmp_pptr->oprio = EMPTY;
                        DTRACE("DBG$ %d %s> pid %d owns lid %d, pprio set to " \
                                "oprio, %d\n",  \
                            currpid, __func__, tmp_pid, plid, tmp_pptr->pprio);
                    } else {
                        tmp_pptr->pprio = new_prio;
                        DTRACE("DBG$ %d %s> pid %d owns lid %d, pprio set " \
                                "to %d\n", currpid, __func__, tmp_pid, plid,\
                                tmp_pptr->pprio);
                    }
                }
            }
            
            /* Reset the priorities of all the pids that waits on this lid. */
            if (PRWAIT == L_GET_PSTATE(tmp_pid)) {
                if ((TRUE == L_GET_PRIOFLAG(tmp_pid)) && 
                        (pprio == L_GET_PPRIO(tmp_pid))) {
                    if (EMPTY == new_prio) {
                        tmp_pptr->prioflag = FALSE;
                        tmp_pptr->pprio = tmp_pptr->oprio;
                        tmp_pptr->oprio = EMPTY;
                        DTRACE("DBG$ %d %s> pid %d waits lid %d, pprio set to " \
                                "oprio, %d\n",  \
                            currpid, __func__, tmp_pid, plid, tmp_pptr->pprio);
                    } else {
                        tmp_pptr->pprio = new_prio;
                        DTRACE("DBG$ %d %s> pid %d waits lid %d, pprio set " \
                                "to %d\n", currpid, __func__, tmp_pid, plid,\
                                tmp_pptr->pprio);
                    }
                }
            }   /* waiting pids */
        }   /* for */
    }   /* if (hpid(plid) == pid)) */

    /* Remove the pid from a lid's waitlist, if present. */
    iter_pid = q[tmp_tail].qprev;
    while (iter_pid != tmp_head) {
        if (pid == iter_pid) {
            /* Got our victim. Remove her. */
            q[q[iter_pid].qprev].qnext = q[iter_pid].qnext;
            q[q[iter_pid].qnext].qprev = q[iter_pid].qprev;
            q[iter_pid].qnext = q[iter_pid].qprev = EMPTY;
            DTRACE("DBG$ %d %s> pid %d, removed from lid %d waitlist\n", 
                    currpid, __func__, pid, plid);
            break;
        }
        iter_pid = q[iter_pid].qprev;
    }
    
    (LT_READ == plid_type) ? (L_DEC_NWREAD(plid)) : (L_DEC_NWWRITE(plid));
    pptr->plid = pptr->plid_type = EMPTY;

RETURN_OK:
    DTRACE_END;
    restore(ps);
    return OK;

RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;    
}


void
l_get_next_prio(int lid, int pid, int *new_pid, int *new_prio)
{
    int tmp_pid = EMPTY;
    int max_pid = EMPTY;
    int max_prio = EMPTY;
    int tmp_prio = EMPTY;
    int pprio = L_GET_PPRIO(pid);
    DTRACE_START;

    for (tmp_pid = 1; tmp_pid < NPROC; ++tmp_pid) {
        if (tmp_pid == pid) {
            continue;
        }

        if ((PRWAIT == L_GET_PSTATE(tmp_pid)) && 
                (L_GET_HPID(lid) == pid)) {
            if ((TRUE == L_GET_PRIOFLAG(tmp_pid)) && 
                    (pprio == L_GET_PPRIO(tmp_pid))) {
                tmp_prio = L_GET_OPRIO(tmp_pid);
                DTRACE("DBG$ %d %s> tmp_prio set to oprio of pid %d, %d\n", \
                        currpid, __func__, tmp_pid, tmp_prio);
            } else {
                tmp_prio = L_GET_PPRIO(tmp_pid);
                DTRACE("DBG$ %d %s> tmp_prio set to pprio of pid %d, %d\n", \
                        currpid, __func__, tmp_pid, tmp_prio);
            }
            if (tmp_prio > max_prio) {
                max_prio = tmp_prio;
                max_pid = tmp_pid;
                DTRACE("DBG$ %d %s> max_prio set to %d\n", \
                        currpid, __func__, max_prio);
            }
        }
    }

    *new_pid = max_pid;
    *new_prio = max_prio;
    DTRACE_END;
    return;
}


