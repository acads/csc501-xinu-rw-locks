/* adhanas */
/* lprint.c -- PA1, print debug utils */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

#ifdef DBG_ON

void
l_print_locks_all (void)
{
    int lid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    for (lid = (NSEM - 1); lid >= 0; --lid)
        l_print_lock_details(lid);

    DTRACE_END;
    restore(ps);
    return;
}


void
l_print_locks_active (void)
{
    int lid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    DTRACE("Active locks in the system:\n", NULL);
    for (lid = (NSEM - 1); lid >= 0; --lid) {
        if ((LS_READ != L_GET_LSTATE(lid)) || 
                (LS_WRITE != L_GET_LSTATE(lid)))
            continue;
        l_print_lock_details(lid);
    }

    DTRACE_END;
    restore(ps);
    return;
}


void
l_print_locks_deleted (void)
{
    int lid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    DTRACE("Deleted locks in the system:\n", NULL);
    for (lid = (NSEM - 1); lid >= 0; --lid) {
        if (LS_DELETED!= L_GET_LSTATE(lid))
            continue;
        l_print_lock_details(lid);
    }

    DTRACE_END;
    restore(ps);
    return;
}

void
l_print_locks_free (void)
{
    int lid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    DTRACE("Free locks in the system:\n", NULL);
    for (lid = (NSEM - 1); lid >= 0; --lid) {
        if (LS_FREE != L_GET_LSTATE(lid))
            continue;
        l_print_lock_details(lid);
    }

    DTRACE_END;
    restore(ps);
    return;
}


void
l_printq (int head, int tail)
{
    int pid = 0;

    //DTRACE("Head: %d, Tail: %d, PID : ", head, tail);

    pid = q[tail].qprev;
    while (pid != head) {
        DTRACE("%d, ", pid);
        pid = q[pid].qprev;
    }
    DTRACE("\n", NULL);
    return;
}


void
l_print_lock_details(int lid)
{
    struct rwlock *lptr = NULL;
    STATWORD ps;

    disable(ps);
    if (L_IS_BAD_LID(lid))
        return;
    lptr = L_GET_LPTR(lid);

    DTRACE("\n******** Lock Details Start ********\n", NULL);
    DTRACE("LID                       : %d\n", lid);
    DTRACE("Lock state                : %s\n", l_states_str[L_GET_LSTATE(lid)]);

    DTRACE("# of active readers       : %d\n", L_GET_NAREAD(lid));
    DTRACE("# of active writers       : %d\n", L_GET_NAWRITE(lid));

    DTRACE("# of waiting readers      : %d\n", L_GET_NWREAD(lid));
    DTRACE("# of waiting writers      : %d\n", L_GET_NWWRITE(lid));

    DTRACE("Waiting readers on LID %d : ", lid);
    l_printq(lptr->lrhq, lptr->lrtq);
    DTRACE("Waiting writers on LID %d : ", lid);
    l_printq(lptr->lwhq, lptr->lwtq);

    DTRACE("Next PID                  : %d\n", L_GET_NEXTPID(lid));
    DTRACE("Next queue                : %s\n", L_GET_LTYPESTR(L_GET_NEXTRW(lid)));
    DTRACE("HPid                      : %d\n", L_GET_HPID(lid));
    DTRACE("HPrio                     : %d\n", L_GET_HPRIO(lid));
#if 0
    DTRACE("Next queue                : %s\n", (L_GET_NEXTQ(lid) ? "Write" \
                : "Read"));
#endif
    DTRACE("******** Lock Details End *********\n", NULL);
    DTRACE("\n", NULL);

    restore(ps);
    return;
}


void
l_print_locks(int lock_type)
{
    DTRACE_START;
    switch (lock_type) {
        case LS_READ:
        case LS_WRITE:
            l_print_locks_active();
            return;

        case LS_DELETED:
            l_print_locks_deleted();
            return;

        case LS_FREE:
            l_print_locks_free();
            return;
            
        default:
            l_print_locks_all();
            return;
    }
    DTRACE_END;
}


void
l_print_pid_details(int pid)
{
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);

    if (isbadpid(pid)) {
        DTRACE_END;
        restore(ps);
        return;
    }
    pptr = L_GET_PPTR(pid);
    DTRACE("\n******** Process Details Start ********\n", NULL);
    DTRACE("PID             : %d\n", pid);
    DTRACE("Process state   : %s\n", l_pstates_str[L_GET_PSTATE(pid) - 1]);
    DTRACE("Waiting on lock : %d\n", L_GET_PLID(pid));
    DTRACE("Lock type       : %d\n", L_GET_PLIDTYPE(pid));
    DTRACE("Wait start time : %u\n", L_GET_PTIME(pid));
    DTRACE("Priority flag   : %d\n", L_GET_PRIOFLAG(pid));
    DTRACE("OPriority       : %d\n", L_GET_OPRIO(pid));
    DTRACE("PPriority flag: : %d\n", L_GET_PPRIO(pid));
    DTRACE("******** Process Details End ********\n", NULL);
    DTRACE("\n", NULL);
    restore(ps);
    return;
}
#endif /* DBG_ON */
