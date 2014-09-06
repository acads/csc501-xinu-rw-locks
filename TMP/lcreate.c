/* adhanas */
/* lcreate.c -- PA2, create a rwlock */

#include <kernel.h>
#include <stdio.h>
#include <proc.h>
#include <lock.h>

/* Creates a rwlock and returns the lock ID. 
 * Params: None
 * Returns: Lock ID (0 thru NLOCKS-1) on success, SYSERR on failure
 */
int
lcreate (void)
{
    int ret_lid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (NLOCKS == nlocks) {
        DTRACE("DBG$ %d %s> max locks limit reached!\n", currpid, __func__);
        DTRACE_END;
        restore(ps);
        return SYSERR;
    }

    if (SYSERR == (ret_lid = l_new_lid())) {
        DTRACE("DBG$ %d %s> not able create a new lock\n", currpid, __func__);
        DTRACE_END;
        restore(ps);
        return SYSERR;
    }

    DTRACE("DBG$ %d %s> new lock %d created\n", currpid, __func__, ret_lid);
    DTRACE("DBG$ %d %s> setting the logmap bit for lid %d after successful "   \
            "lock creation...\n", currpid, __func__, ret_lid);
    l_pidmap_oper(ret_lid, currpid, L_MAP_LOG, L_MAP_SET);

    DTRACE_END;
    restore(ps);
    return ret_lid;
}

