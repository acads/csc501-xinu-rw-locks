/* adhanas */
/* lock.c -- PA2, rwlock, lock implementation */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/* Name: lock
 * 
 * Desc: Acquires the specified lock (via lid), if available, for the calling 
 *       process and return. If the lock is unavailable, the calling process is 
 *       placed on the lock's read or write queue (depending on the type of lock)
 *       and some other process is scheduled. This process will be scheduled 
 *       again once the lock becomes avaialbe or if the lock is deleted by some 
 *       other process.
 *
 * Params:
 *  lid     - lock ID
 *  ltype   - type of lock (LT_READ or LT_WRITE)
 *  lprio   - priority to acquire the lock
 *
 * Returns: int
 *  OK      - if the lock acuisiton is successful
 *  SYSERR  - on failure
 *  DELETED - if the lock is deleted when the proc is waiting on the lock
 */
int
lock(int lid, int ltype, int lprio)
{
    int lstate = EMPTY;
    struct rwlock *lptr = NULL;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (L_IS_BAD_LID(lid)) {
        DTRACE("DBG$ %d %s> bad lid %d\n", currpid, __func__, lid);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (FALSE == L_IS_VALID_LTYPE(ltype)) {
        DTRACE("DBG$ %d %s> bad ltype %d\n", currpid, __func__, ltype);
        goto RESTORE_AND_RETURN_ERROR;
    }

    lstate = L_GET_LSTATE(lid);
    lptr = L_GET_LPTR(lid);
    pptr= L_GET_PPTR(currpid);

    switch (lstate) {
        case LS_UNINIT:
            DTRACE("DBG$ %d %s> lid %d state %s: cannot be locked\n",   \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            goto RESTORE_AND_RETURN_ERROR;
            break;

        case LS_FREE:
            /* Chek if this is a request on a stale lid. */
            if (TRUE == l_is_lid_stale_for_pid(lid, currpid)) {
                DTRACE("DBG$ %d %s> LID %d IS STALE FOR PID %d\n",  \
                        currpid, __func__);
                goto RESTORE_AND_RETURN_ERROR;
            }

            /* Lock is free. Grab it, change the lstate and return OK. */
            if (LT_READ == ltype) {
                lptr->lstate = LS_READ;
                L_INC_NAREAD(lid);
                DTRACE("DBG$ %d %s> lid %d is free, pid %d has read "  \
                        "access now\n", currpid, __func__, lid, currpid);
            } else if (LT_WRITE == ltype) {
                lptr->lstate = LS_WRITE;
                L_INC_NAWRITE(lid);
                DTRACE("DBG$ %d %s> lid %d is free, pid %d has write " \
                        "access now\n", currpid, __func__, lid, currpid);
            } else {
                /* Should never happen. */
                DTRACE("DBG$ %d %s> bad ltype %d\n",   \
                        currpid, __func__, ltype);
                ASSERT(0);
            }
#ifdef DBG_ON
            DTRACE("DBG$ %d %s> printing lid %d details...\n",  \
                    currpid, __func__, lid);
            l_print_lock_details(lid);
#endif /* DBG_ON */
            goto RESTORE_AND_RETURN_SUCCESS;
            break;

        case LS_READ:
            /* Chek if this is a request on a stale lid. */
            if (TRUE == l_is_lid_stale_for_pid(lid, currpid)) {
                DTRACE("DBG$ %d %s> LID %d IS STALE FOR PID %d\n",  \
                        currpid, __func__);
                goto RESTORE_AND_RETURN_ERROR;
            }

            if (LT_READ == ltype) {
                int wprio = L_GET_WPRIO(lid);
                if (L_GET_NWWRITE(lid) && (wprio > lprio)) {
                    /* There exists a waiting writer whose prio is > the incoming
                     * reader's prio. So, put the caller proc on read waitlist,
                     * change the pstate to WAIT and call resched() to schedule
                     * some other proc.
                     */
                    pptr->plid = lid;
                    pptr->plid_type = ltype;
                DTRACE("SETTING PLID TYPE %d IN WRITE STATE FOR PID %d\n",  \
                        pptr->plid_type, currpid);
                    pptr->plocktime = ctr1000;

                    /* Avoid priority inversion, if required. */
                    if (TRUE == l_inherit_prio_if_reqd(lid, currpid)) {
                        DTRACE("DBG$ %d %s> priority inherited!\n",         \
                                currpid, __func__);
                    }

                    DTRACE("DBG$ %d %s> lid %d is in read state, "          \
                            "reader pid %d is being put into read queue\n", \
                            currpid, __func__, lid, currpid);
                    L_INC_NWREAD(lid);
                    insert(currpid, L_GET_RHEAD(lid), lprio);
                    pptr->pwaitret = OK;
                    l_recal_next(lid);
#ifdef DBG_ON
                    DTRACE("DBG$ %d %s> printing lid %d details...\n",      \
                            currpid, __func__, lid);
                    l_print_lock_details(lid);
#endif /* DBG_ON */
                    goto RESTORE_AND_RESCHED;
                } else {
                    /* The incoming reader has a prio > the waiting writer.
                     * Allow the reader. No harm.
                     */
                    DTRACE("DBG$ %d %s> lid %d is in read state, "         \
                            "reader pid %d has read access now\n",         \
                            currpid, __func__, lid, currpid);
                    L_INC_NAREAD(lid);
                    goto RESTORE_AND_RETURN_SUCCESS;
                }
            } else if (LT_WRITE == ltype) {
                /* One, and only a writer at a time. But, the lock's current
                 * state is READ. Put the caller in write waitlist, change the
                 * pstate to READY and call resched() to schedule some other
                 * proc. 
                 */
                pptr->plid = lid;
                pptr->plid_type = ltype;
                DTRACE("SETTING PLID TYPE %d IN WRITE STATE FOR PID %d\n",  \
                        pptr->plid_type, currpid);
                pptr->plocktime = ctr1000;

                /* Avoid priority inversion, if required. */
                if (TRUE == l_inherit_prio_if_reqd(lid, currpid)) {
                    DTRACE("DBG$ %d %s> priority inherited!\n",     \
                            currpid, __func__);
                }

                DTRACE("DBG$ %d %s> lid %d is in read state, " \
                        "writer pid %d is being put into write queue\n",   \
                        currpid, __func__, lid, currpid);
                L_INC_NWWRITE(lid);
                insert(currpid, L_GET_WHEAD(lid), lprio);
                pptr->pwaitret = OK;
                l_recal_next(lid);
#ifdef DBG_ON
                DTRACE("DBG$ %d %s> printing lid %d details...\n",  \
                        currpid, __func__, lid);
                l_print_lock_details(lid);
#endif /* DBG_ON */
                goto RESTORE_AND_RESCHED;
            } else {
                /* Should never happen. */
                DTRACE("DBG$ %d %s> invalid ltype %d\n",   \
                        currpid, __func__, ltype);
                ASSERT(0);
                goto RESTORE_AND_RETURN_SUCCESS;
            }
            break;

        case LS_WRITE:
            /* Chek if this is a request on a stale lid. */
            if (TRUE == l_is_lid_stale_for_pid(lid, currpid)) {
                DTRACE("DBG$ %d %s> LID %d IS STALE FOR PID %d\n",  \
                        currpid, __func__);
                goto RESTORE_AND_RETURN_ERROR;
            }

            /* Avoid priority inversion, if required. */
            if (TRUE == l_inherit_prio_if_reqd(lid, currpid)) {
                DTRACE("DBG$ %d %s> priority inherited!\n", currpid, __func__);
            }

            /* Lock is in write state. Not much to do. Put the caller in the 
             * appropriate wait list, change the caller's pstate to READY and
             * call resched() to shcedule some other proc.
             */
            pptr->plid = lid;
            pptr->plid_type = ltype;
            DTRACE("SETTING PLID TYPE %d IN WRITE STATE FOR PID %d\n",  \
                    pptr->plid_type, currpid);
            pptr->plocktime = ctr1000;
            if (LT_READ == ltype) {
                DTRACE("DBG$ %d %s> lid %d is in write state, "    \
                        "reader pid %d is being put into read queue\n",        \
                        currpid, __func__, lid, currpid);
                L_INC_NWREAD(lid);
                insert(currpid, L_GET_RHEAD(lid), lprio);
            } else if (LT_WRITE == ltype) {
                DTRACE("DBG$ %d %s> lid %d is in write state, "    \
                        "writer pid %d is being put into write queue\n",       \
                        currpid, __func__, lid, currpid);
                L_INC_NWWRITE(lid);
                insert(currpid, L_GET_WHEAD(lid), lprio);
            } else {
                /* Should never happen. */
                DTRACE("DBG$ %d %s> invalid ltype %d\n",   \
                        currpid, __func__, ltype);
                ASSERT(0);
                goto RESTORE_AND_RETURN_SUCCESS;
            }
            pptr->pwaitret = OK;
            l_recal_next(lid);
#ifdef DBG_ON
            DTRACE("DBG$ %d %s> printing lid %d details...\n",  \
                    currpid, __func__, lid);
            l_print_lock_details(lid);
#endif /* DBG_ON */
            goto RESTORE_AND_RESCHED;
            break;

        case LS_DELETED:
            /* Some old proc still thinks that this lock is valid, but it's not!
             * Return SYSERR.
             */
            DTRACE("DBG$ %d %s> lid %d state %s: cannot be locked\n",   \
                    currpid, __func__, lid, L_GET_LSTATESTR(lid));
            goto RESTORE_AND_RETURN_ERROR;
            break;

        default:
            /* Lock's sate machine is screwed big time! */
            DTRACE("DBG$ %d %s> lid %d bad state %d\n", \
                    currpid, __func__, lid, L_GET_LSTATE(lid));
            ASSERT(0);
            goto RESTORE_AND_RETURN_SUCCESS;
    }

RESTORE_AND_RESCHED:
    /* This proc gotta wait. Change the pstate and call resched() to schedule
     * some other proc. 
     */
    pptr->pstate = PRWAIT;
    DTRACE("DBG$ %d %s> pid %d put into wait queue\n",     \
            currpid, __func__, currpid);
    resched();

    /* Someone has awakened the proc from the wait queue. This means:
     *      1. This proc has now got a shot at the lock. 
     *      2. The lock on which this proc was waiting on was deleted.
     * In latter case, the ldelete() code would have set the appropriate errcode
     * in pptr->pwaitret. So, just retun after setting the appropriate bitmaps.
     */
    DTRACE("DBG$ %d %s> setting the logmap bit for lid %d after resched"    \
            "...\n", currpid, __func__, lid);
    l_pidmap_oper(lid, currpid, L_MAP_LOG, L_MAP_SET);
    if (OK == pptr->pwaitret) {
        L_INC_PLOCKS(currpid);
        pptr->plid = EMPTY;
        pptr->plid_type = EMPTY;
        DTRACE("DBG$ %d %s> lock inc'd for pid %d, total # of locks %d\n",     \
                currpid, __func__, currpid, L_GET_PLOCKS(currpid));
        l_pidmap_oper(lid, currpid, L_MAP_RELEASE, L_MAP_SET);

        DTRACE("DBG$ %d %s> pid %d, setting the plidmap bit for lid %d after " \
                "resched...\n", currpid, __func__, currpid, lid);
        l_pidmap_oper(lid, currpid, L_MAP_PLID, L_MAP_SET);
    }
    restore(ps);
    return pptr->pwaitret;

RESTORE_AND_RETURN_ERROR:
    /* Something went wrong. */
    DTRACE_END;
    restore(ps);
    return SYSERR;

RESTORE_AND_RETURN_SUCCESS:
    /* Lock acquired. Set the required bitmaps and return success. */
    L_INC_PLOCKS(currpid);
    DTRACE("DBG$ %d %s> lock inc'd for pid %d, total # of locks %d\n",     \
            currpid, __func__, currpid, L_GET_PLOCKS(currpid));
    l_pidmap_oper(lid, currpid, L_MAP_RELEASE, L_MAP_SET);
    DTRACE("DBG$ %d %s> setting the logmap bit for lid %d after "
            "successful lock...\n", currpid, __func__, lid);
    l_pidmap_oper(lid, currpid, L_MAP_LOG, L_MAP_SET);
    DTRACE("DBG$ %d %s> pid %d, setting the plidmap bit for lid %d after "
            "successful lock...\n", currpid, __func__, currpid, lid);
    l_pidmap_oper(lid, currpid, L_MAP_PLID, L_MAP_SET);
    DTRACE_END;
    restore(ps);
    return OK;
}

