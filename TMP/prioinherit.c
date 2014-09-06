/* adhanas */
/* prioinherit.c -- PA2, priority inheritence */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

/* Name:    l_inherit_prio_if_reqd 
 *
 * Desc:    Inherit the priority of an higher priority proc (HPP) to that of a 
 *          lower priority proc (LPP), in the event of a priority inversion .
 *          problem. Also, recursively raise the priorty of all the other procs 
 *          on which the LPP happens to be waiting on. This achieves trasitive 
 *          nature of priority inheritence.
 *
 * Params: 
 *  lid     - lock ID on which the HPP is waiting on
 *  pid     - proc ID of the HPP
 *
 * Returns: int
 *  TRUE    - if priority inheritance actually happened
 *  FALSE   - otherwise
 */ 
int
l_inherit_prio_if_reqd(int lid, int in_pid)
{
    int pid = EMPTY;
    int cpid = EMPTY;
    int hpid = EMPTY;
    int cprio = EMPTY;
    int hprio = EMPTY;
    int pprio = EMPTY;
    int retval = FALSE;
    struct pentry *pptr = NULL;
    struct rwlock *lptr = NULL;
    STATWORD ps;
    
    disable(ps);
    DTRACE_START;

    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RETURN_FALSE;
    }

    if (isbadpid(in_pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_FALSE;
    }

    lptr = L_GET_LPTR(lid);
    hprio = L_GET_HPRIO(lid);
    hpid = L_GET_HPID(lid);
    cpid = in_pid;
    cprio = L_GET_PPRIO(cpid);

    if (hprio < cprio) {
        DTRACE("DBG$ %d %s> lid %d hprio %d is less than currpid %d prio %d\n", \
               currpid, __func__, lid, hprio, cpid, cprio);
        lptr->hpid = hpid = cpid;
        lptr->hprio = hprio = cprio;
        DTRACE("DBG$ %d %s> lid %d hpid set to %d, hprio set to %d\n",  \
               currpid, __func__, lid, cpid, cprio);
    }

    /* Check the currpid's priority against lpid's hprio & perform required
     * updates, if necessary.
     */
    for (pid = 0; pid < NPROC; ++pid) {
        if (TRUE == l_pidmap_oper(lid, pid, L_MAP_RELEASE, L_MAP_CHK)) {
            DTRACE("DBG$ %d %s> pid %d set\n", currpid, __func__, pid);
            pprio = L_GET_PPRIO(pid);
            DTRACE("DBG$ %d %s> pid %d pprio %d\n", currpid, __func__, pid, pprio);
            DTRACE("DBG$ %d %s> lid %d, hpid %d, hprio %d\n",   \
                    currpid, __func__, lid, hpid, hprio);
            if (pprio < hprio) {
                pptr = L_GET_PPTR(pid);
                DTRACE("DBG$ %d %s> lid %d, pid %d prio %d < hprio %d\n",   \
                        currpid, __func__, lid, pid, pprio, hprio);
                /* Save the original priority, change the priority to the 
                 * inherited priority and set the priochange flag.
                 */
                if (FALSE == L_GET_PRIOFLAG(pid)) {
                    pptr->prioflag = TRUE;
                    pptr->oprio = pprio;
                }
                pptr->pprio = hprio;
                retval |= TRUE;
                DTRACE("DBG$ %d %s> lid %d, pid %d prio changed from "  \
                        "%d to %d\n", currpid, __func__, lid, pid,      \
                        L_GET_OPRIO(pid), L_GET_HPRIO(lid));

                /* Recursively inherit the priorities for other procs */
                retval |= l_inherit_prio_if_reqd(L_GET_PLID(pid), pid);
            }
        }
    }
        
    DTRACE_END;
    restore(ps);
    return retval;

RETURN_FALSE:
    DTRACE_END;
    restore(ps);
    return FALSE;
}


/* Name:    l_reset_prio 
 *
 * Desc:    Everytime a proc releases a lock, we need to make sure that it runs
 *          on the correct priorty. i.e., if a proc has inherited some higher
 *          priority code, we need to reset the priority to the proc's original
 *          priority when the proc releases its last lock.
 *
 * Params: 
 *  lid     - lock ID which was just released by the proc
 *  pid     - proc ID of the proc which released the lid
 *
 * Returns: int
 *  TRUE    - if the prio of pid is reset to the original priority
 *  FALSE   - otherwise
 */ 
int
l_reset_prio(int lid ,int pid)
{
    int nlocks = EMPTY;
    int tmp_lid = EMPTY;
    int hprio = EMPTY;
    int max_prio = EMPTY;
    uint8_t is_inherited = FALSE;
    struct pentry *pptr = NULL;
    STATWORD ps;
    
    disable(ps);
    DTRACE_START;

    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RETURN_FALSE;
    }

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_FALSE;
    }
    pptr = L_GET_PPTR(pid);

    if (FALSE == (is_inherited = L_GET_PRIOFLAG(pid))) {
        DTRACE("DBG$ %d %s> pid %d is currently using it's original priority\n",\
                currpid, __func__, pid);
        goto RETURN_FALSE;
    }

    if (0 == (nlocks = L_GET_PLOCKS(pid))) {
        DTRACE("DBG$ %d %s> pid %d has released all its locks.. "   \
               "resetting its priority\n", currpid, __func__, pid);
        DTRACE("DBG$ %d %s> pid %d prio changed from %d to %d\n",   \
                currpid, __func__, pid, pptr->pprio, pptr->oprio);

        pptr->pprio = pptr->oprio;
        pptr->oprio = 0;
        pptr->prioflag = FALSE;
        pptr->plid = EMPTY;

        goto RETURN_TRUE;
    }

    /* If we are here, the proc owns a few more locks. Go over them and set the
     * inherited priority to the max wait prio of the lid.
     */
    max_prio = L_GET_PPRIO(pid);
    for (tmp_lid = 0; tmp_lid < NLOCKS; ++tmp_lid) {
        if (TRUE == l_pidmap_oper(tmp_lid, pid, L_MAP_PLID, L_MAP_CHK)) {
            if (max_prio < (hprio = L_GET_HPRIO(tmp_lid))) {
                DTRACE("DBG$ %d %s> pid %d, currprio %d < hprio %d of lid %d\n",\
                        currpid, __func__, pid, max_prio, hprio, tmp_lid);
                max_prio = hprio;
                is_inherited |= TRUE;
            }
        }
    }

    if (TRUE == is_inherited) {
        DTRACE("DBG$ %d %s> pid %d, prio changed from %d to %d\n",  \
                currpid, __func__, pid, L_GET_PPRIO(pid), max_prio);
        pptr->pprio = max_prio;
        pptr->prioflag = TRUE;
    }
    goto RETURN_TRUE;

RETURN_TRUE:
    DTRACE_END;
    restore(ps);
    return is_inherited;

RETURN_FALSE:
    DTRACE_END;
    restore(ps);
    return FALSE;
}


/* Name:    l_handle_chprio
 *
 * Desc:    If a porc's prio is changed when it's queued on the waitlist of a
 *          lock, it might affect the current priority inheritance setup. This
 *          routine recalculates the highest priority of the waitlist of the lid
 *          and changes the priority of the proc that is currently holding the
 *          lid. 
 *
 * Params: 
 *  pid     - proc ID of the proc whose prio was changed
 *
 * Returns: int
 *  TRUE    - if the prio of the waitlist & lid owner is changed
 *  FALSE   - otherwise
 */ 
int
l_handle_chprio(int pid, int new_prio)
{
    int plid = EMPTY;
    int oprio = EMPTY;
    int pprio = EMPTY;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_FALSE;
    }
    pptr = L_GET_PPTR(pid);
    plid = L_GET_PLID(pid);

    if (FALSE == L_GET_PRIOFLAG(pid)) {
        pptr->pprio = new_prio;
    } else {
        oprio = L_GET_OPRIO(pid);
        pprio = L_GET_PPRIO(pid);

        pptr->oprio = new_prio;
        if (pprio < new_prio) {
            pptr->pprio = new_prio;
        } 
    }

    /* Now that we have changed the priority of a proc, it might affect
     * a lock's wait list. Go over this lock's (on which this pid is waiting
     * on) waitlist and recalc hprio. If a new hprio is found, update the
     * proc(s) prio that own the lid.
     */
    if (l_inherit_prio_if_reqd(plid, pid)) {
        DTRACE("DBG$ %d %s> pid %d, plid %d, pi'd due to chprio()\n ",  \
                currpid, __func__, pid, plid);
    }

    DTRACE_END;
    restore(ps);
    return TRUE;

RETURN_FALSE:
    DTRACE_END;
    restore(ps);
    return FALSE;
}


/* Name:    l_handle_kill
 *
 * Desc:    Handles the active locks and priority inheritance situation when
 *          a proc is killed. This routine takes care of releasing all the 
 *          locks that were owned by the proc that is being killed, re-considers
 *          the current priority inheritance situation and updates the 
 *          priorities if required. Finally deletes the pid from the lid's 
 8          waitlist.
 *
 * Params: 
 *  pid     - proc ID of the proc that is being killed
 *
 * Returns: int
 *  TRUE    - if all goes well & good
 *  FALSE   - otherwise
 */ 
int
l_handle_kill(int pid)
{
    int plid = EMPTY;
    int tmp_lid = EMPTY;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RETURN_ERROR;
    }
    pptr = L_GET_PPTR(pid);

#ifdef DBG_ON
    l_print_pid_details(pid);
#endif 

    for (tmp_lid = 0; tmp_lid < NLOCKS; ++tmp_lid) {
        if (TRUE == l_pidmap_oper(tmp_lid, pid, L_MAP_PLID, L_MAP_CHK)) {
            DTRACE("DBG$ %d %s> pid %d, owns lid %d, release it\n",     \
                    currpid, __func__, pid, tmp_lid);
            if (TRUE == l_unlock(tmp_lid, pid)) {
                DTRACE("DBG$ %d %s> pid %d released lid %d\n",  \
                        currpid, __func__, pid, tmp_lid);
                if (TRUE == l_reset_prio(tmp_lid, pid))
                    DTRACE("DBG$ %d %s> pid %d, lid %d, priority resetted " \
                            "after due to kill\n",                          \
                            currpid, __func__, pid, tmp_lid);
            } else {
                DTRACE("DBG$ %d %s> pid %d not able to release lid %d\n",   \
                        currpid, __func__, pid, tmp_lid);
            }
        }
    }

    if (TRUE == l_remove_pid(pid)) {
        plid = L_GET_PLID(pid);
        DTRACE("DBG$ %d %s> pid %d removed from waitlist\n",    \
                currpid, __func__, pid);
        (LT_READ == L_GET_PLIDTYPE(plid)) ? 
            (L_DEC_NWREAD(plid)) : (L_DEC_NWWRITE(plid));
        pptr->plid = pptr->plid_type = EMPTY;
    }

RETURN_OK:
    DTRACE_END;
    restore(ps);
    return OK;

RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}

