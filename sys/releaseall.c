/* adhanas */
/* releaseall.c -- PA2, rwlocks, release a rwlock */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <q.h>
#include <lock.h>

/* releaseall
 * Releases all the specified locks. The 1st argument specifies the # of locks
 * to be released and rest specifies the lock IDs. The lids are read directly
 * from the stack. 
 *
 * Params: 
 *  num_locks   - # of locks to release
 *  lid         - lock ID list and the values are read directly from the stack
 *
 * Returns: int
 *  OK          - on successful release
 *  SYSERR      - if the lock has been deleted already or if the calling
 *                pid doesn't own the lock
 */ 
int 
releaseall(num_locks, lid)
    int num_locks;
    int lid;
{
    uint32_t *pargs = NULL;
    int ret_val = 0;
    int tmp_lid = EMPTY;
    int ret_flag = TRUE;
    int final_ret = TRUE;
    STATWORD ps;

    disable(ps);
    DTRACE_START;
    if (0 == num_locks) {
        /* Nothing to release. Bad call. */
        goto RESTORE_AND_RETURN_ERROR;
    }
    pargs = (uint32_t *) (&lid) + (num_locks - 1);
    for (; num_locks > 0; num_locks--) {
        tmp_lid = *(pargs--);
        DTRACE("\n\n\nDAN: releasing lid %d\n\n\n", tmp_lid);
        ret_flag = l_unlock((int) tmp_lid, currpid);
        final_ret &= ret_flag;
        if ((TRUE == ret_flag) && (TRUE == l_reset_prio(tmp_lid, currpid))) {
            DTRACE("DBG$ %d %s> pid %d prio resetted after releasing lid %d\n",\
                    currpid, __func__, currpid, lid);
        }
    }
    ret_val = (TRUE == final_ret) ? OK : SYSERR;
    resched();

    DTRACE_END;
    restore(ps);
    return ret_val;

RESTORE_AND_RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}

/* Name:    l_unlock
 *
 * Desc:    Releases the currpid's lock, if it holds one and schedules the next 
 *          proc(s) to whom the lock should go to. If the lock doesn't belong to 
 *          the caller, it returns SYSERR.
 *
 * Params: 
 *  lid     - lock ID which is to be released
 *  pid     - proc ID which is releasing the lock
 *
 * Returns: int
 *  TRUE    - on successful release of the lock
 *  FLASE   - otherwise 
 */ 
int
l_unlock(int lid, int pid)
{
    int lstate = EMPTY;
    DTRACE_START;

    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RETURN_FALSE;
    }
    lstate = L_GET_LSTATE(lid);

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RETURN_FALSE;
    }

    /* Chek if this is a request on a stale lid. */
    if (TRUE == l_is_lid_stale_for_pid(lid, pid)) {
        DTRACE("DBG$ %d %s> LID %d IS STALE FOR PID %d\n",  \
                currpid, __func__);
        goto RETURN_FALSE;
    }

    /* Check if the currpid bit is set in the lid's releasemap. If so, release
     * the lock. If not, do not release the lock and return an error to the 
     * caller.
     */
    if (TRUE == l_pidmap_oper(lid, pid, L_MAP_RELEASE, L_MAP_CHK)) {
        switch (lstate) {
            case LS_UNINIT:
            case LS_FREE:
            case LS_DELETED:
                DTRACE("DBG$ %d %s> relelaseall() not allowed in " \
                        "lid %d state %s\n", currpid, __func__, lid,    \
                        L_GET_LSTATESTR(lid));
                goto RETURN_FALSE;

            case LS_READ:
            case LS_WRITE:
                l_pidmap_oper(lid, pid, L_MAP_RELEASE, L_MAP_CLR);
                l_pidmap_oper(lid, pid, L_MAP_PLID, L_MAP_CLR);
                if (LS_READ == lstate) {
                    L_DEC_NAREAD(lid);
                    DTRACE("DBG$ %d %s> pid %d unlocked read lid %d\n",  \
                            currpid, __func__, pid, lid);
                    if (0 != L_GET_NAREAD(lid)) {
                        /* Still few more active readers on this lid. Don't
                         * schedule any more procs. 
                         */
                        DTRACE("DBG$ %d %s> %d more active readers for lid %d " \
                                "are left\n",   \
                                currpid, __func__, L_GET_NAREAD(lid), lid);
                        goto RETURN_TRUE;
                    }
                } else {
                    L_DEC_NAWRITE(lid);
                    DTRACE("DBG$ %d %s> pid %d unlocked write lid %d\n", \
                            currpid, __func__, pid, lid);
                }

                /* In case if any waiting reader(s) or writer as pointed by 
                 * nextpid & nextqueue, schedule them. In case of no such
                 * waiters, change the lstate to FREE and just return.
                 */
                if (LT_READ == L_GET_NEXTRW(lid)) {
                    if (TRUE == l_get_next_readers(lid))
                        goto RETURN_TRUE;
                } else if (LT_WRITE == L_GET_NEXTRW(lid)) {
                    if (TRUE == l_get_next_writer(lid))
                        goto RETURN_TRUE;
                } else {
                    /* No more waiters on that lock. Set the lstate to FREE. */
                    DTRACE("DBG$ %d %s> no more waiters for lid %d\n", \
                            currpid, __func__, lid);
                    DTRACE("DBG$ %d %s> lid %d state changed from %s to "   \
                            "Free\n",   \
                            currpid, __func__, lid, L_GET_LSTATESTR(lid));

                    L_SET_LSTATE(lid, LS_FREE);
                    goto RETURN_TRUE;
                }
                break;

            default:
                DTRACE("DBG$ %d %s> lid %d bad state %d\n",    \
                        currpid, __func__, lid, lstate);
                ASSERT(0);
                goto RETURN_TRUE;
                break;
        }
    } else {
        DTRACE("DBG$ %d %s> lid %d doesn't belong to pid %d\n",    \
                currpid, __func__, lid, pid);
        goto RETURN_FALSE;
    }

RETURN_TRUE:
    L_DEC_PLOCKS(pid);
    DTRACE("DBG$ %d %s> lock dec'd for pid %d, total # of locks %d\n", 
            currpid, __func__, currpid, L_GET_PLOCKS(pid));
    DTRACE_END;
    return TRUE;

RETURN_FALSE:
    DTRACE_END;
    return FALSE;
}

