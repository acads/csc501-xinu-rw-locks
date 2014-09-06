/* adhanas */
/* lock.h -- read/write locks PA2 h file */

#ifndef _LOCK_H_
#define _LOCK_H_

/* Their requirements to my requierments... */
#define READ            LT_READ
#define WRITE           LT_WRITE

/* Symbolic constants */
#define NLOCKS          50

/* Lock states */
#define LS_UNINIT       0
#define LS_FREE         1
#define LS_READ         2
#define LS_WRITE        3
#define LS_DELETED      4

/* Lock type */
#define LT_READ         1
#define LT_WRITE        2

/* pidmap type */
#define L_MAP_DELETE    1
#define L_MAP_RELEASE   2
#define L_MAP_LOG       3
#define L_MAP_PLID      4

/* Pidmap operations */
#define L_MAP_SET       1
#define L_MAP_CLR       2
#define L_MAP_CHK       4
#define L_MAP_TGL       8

/* Constants */
#define L_NPIDMAPS      7   /* total of 7 bitmaps per type per lid 
                             * to store 50 pids 
                             */
#define L_BIT_RANGE     8   /* total of 8 bits per bitmap */

/* Read-write lock data structure */
struct rwlock {
    int lid;            /* lock ID */
    int lstate;         /* lock state - free, in use or deleted */
    int naread;         /* # of currently active readers */
    int nawrite;        /* # of currently active writers - 0 or 1 */
    int nwread;         /* # of readers waiting on this lock */
    int nwwrite;        /* # of writers waiting on this lock */
    int lrhq;           /* head of lock's read queue */
    int lrtq;           /* tail of lock's read queue */
    int lwhq;           /* head of lock's write queue */
    int lwtq;           /* tail of lock's write queue */
    int nextq;          /* next queue who will be granted to use the lock */
    int nextrw;         /* next pid: reader or writer? */
    int nextpid;        /* next pid who will be granted to use the lock */
    int hpid;           /* pid of the highest prio proc waiting on this lid */
    int hprio;          /* prio of the highest prio proc waiting in this lid */
    unsigned char delpidmap[L_NPIDMAPS];  /* delete pid bitmap for this lock */
    unsigned char relpidmap[L_NPIDMAPS];  /* release pid bitmap for this lock */
    unsigned char logpidmap[L_NPIDMAPS];  /* records pid bitmapr for this lock */
};

/* Lock table - defined in sys/linit.c */
extern struct rwlock ltab[];
extern int nlocks;
extern int next_lock;
extern unsigned long ctr1000;
extern const char *l_states_str[];
extern const char *l_pstates_str[];
extern const char *l_lock_types[];

/* Util macros */
#define uint8_t     unsigned char
#define uint32_t    unsigned int

#ifdef DBG_ON
#define DTRACE(STR, ...)    kprintf(STR, __VA_ARGS__)
#define DTRACE_START        kprintf("DBG$ %d %s> start\n", currpid, __func__)
#define DTRACE_END          kprintf("DBG$ %d %s> end\n", currpid, __func__)
#define ASSERT(BOOL)        if (0 == BOOL) trap(17)
#else
#define DTRACE
#define DTRACE_START
#define DTRACE_END
#define ASSERT
#endif /* DBG_ON */


#define L_GET_PID(PID)          (proctab[PID].pid)
#define L_GET_PNAME(PID)        (proctab[PID].pname)
#define L_GET_PPTR(PID)         (&proctab[PID])
#define L_GET_PPRIO(PID)        (proctab[PID].pprio)
#define L_GET_PSTATE(PID)       (proctab[PID].pstate)
#define L_GET_PSTATESTR(PID)    (l_pstates_str[proctab[PID].pstate - 1])
#define L_GET_PTIME(pid)        (proctab[pid].plocktime)
#define L_GET_PLID(pid)         (proctab[pid].plid)
#define L_GET_PLIDTYPE(pid)     (proctab[pid].plid_type)
#define L_GET_OPRIO(pid)        (proctab[pid].oprio)
#define L_GET_PRIOFLAG(pid)     (proctab[pid].prioflag)
#define L_GET_PLOCKS(pid)       (proctab[pid].nlocks)
#define L_INC_PLOCKS(pid)       (proctab[pid].nlocks += 1)
#define L_DEC_PLOCKS(pid)       (proctab[pid].nlocks -= 1)

#define L_GET_LPTR(lid)         (&ltab[lid])
#define L_GET_LSTATE(lid)       (ltab[lid].lstate)
#define L_GET_LSTATESTR(lid)    (l_states_str[ltab[lid].lstate])
#define L_GET_NAREAD(lid)       (ltab[lid].naread)
#define L_GET_NAWRITE(lid)      (ltab[lid].nawrite)
#define L_GET_NWREAD(lid)       (ltab[lid].nwread)
#define L_GET_NWWRITE(lid)      (ltab[lid].nwwrite)
#define L_GET_RHEAD(lid)        (ltab[lid].lrhq)
#define L_GET_LRTAIL(lid)       (ltab[lid].lrtq)
#define L_GET_WHEAD(lid)        (ltab[lid].lwhq)
#define L_GET_LWTAIL(lid)       (ltab[lid].lwtq)
#define L_GET_HPID(lid)         (ltab[lid].hpid)
#define L_GET_HPRIO(lid)        (ltab[lid].hprio)
#define L_GET_RPID(lid)         (q[ltab[lid].lrtq].qprev)
#define L_GET_WPID(lid)         (q[ltab[lid].lwtq].qprev)
#define L_GET_RPRIO(lid)        (q[q[ltab[lid].lrtq].qprev].qkey)
#define L_GET_WPRIO(lid)        (q[q[ltab[lid].lwtq].qprev].qkey)
#define L_GET_NEXTQ(lid)        (ltab[lid].nextq)
#define L_GET_NEXTRW(lid)       (ltab[lid].nextrw)
#define L_GET_NEXTPID(lid)      (ltab[lid].nextpid)
#define L_GET_RTIME(lid)        (proctab[L_GET_RPID(lid)].plocktime)
#define L_GET_WTIME(lid)        (proctab[L_GET_WPID(lid)].plocktime)
#define L_GET_LTYPESTR(ltype)   (l_lock_types[ltype - 1])

#define L_INC_NAREAD(lid)       (ltab[lid].naread += 1)
#define L_INC_NAWRITE(lid)      (ltab[lid].nawrite += 1)
#define L_INC_NWREAD(lid)       (ltab[lid].nwread += 1)
#define L_INC_NWWRITE(lid)      (ltab[lid].nwwrite += 1)
#define L_DEC_NAREAD(lid)       (ltab[lid].naread -= 1)
#define L_DEC_NAWRITE(lid)      (ltab[lid].nawrite -= 1)
#define L_DEC_NWREAD(lid)       (ltab[lid].nwread -= 1)
#define L_DEC_NWWRITE(lid)      (ltab[lid].nwwrite -= 1)

#define L_SET_LSTATE(lid, s) (ltab[lid].lstate = s)

#define L_IS_VALID_LTYPE(ltype) ((LT_READ == ltype) || (LT_WRITE == ltype))
#define L_IS_Q_EMPTY(LID) (L_IS_RQ_EMPTY(LID) && L_IS_WQ_EMPTY(LID)) 
#define L_IS_RQ_EMPTY(LID) (q[ltab[LID].lrhq].qnext == \
        q[ltab[LID].lrtq].qprev)
#define L_IS_WQ_EMPTY(LID) (q[ltab[LID].lwhq].qnext == \
        q[ltab[LID].lwtq].qprev)
#define L_IS_PID_IN_Q(PID)      \
   ((q[PID].qprev != EMPTY) &&  \
    (q[PID].qnext != EMPTY))
#define L_IS_BAD_LID(LID) ((LID < 0) || (LID >= NLOCKS))


/* Function declarations */
extern void linit(void);
extern int lcreate(void);
extern int ldelete(int lid);
extern int lock(int lid, int ltype, int lprio);
extern int releaseall(int numlocks, int lid, ...);
extern void l_init_lid(int lid, int change_state);
extern int l_new_lid(void);
extern void l_recal_next(int lid);
extern int l_pidmap_oper(int lid, int pid, int map_type, int map_oper);
extern int l_unlock(int lid, int pid);
//extern int lunlock(int lid);
extern int l_get_next_readers(int lid);
extern int l_get_next_writer(int lid);
extern int l_clear_pidmaps(int pid);
extern int l_is_lid_stale_for_pid(int lid, int pid);
//extern int do_pi_if_reqd(int lid);
extern int l_inherit_prio_if_reqd(int lid, int pid);
extern int l_reset_prio(int lid, int pid);
extern int l_handle_chprio(int pid, int new_prio);
extern int l_handle_kill(int pid);
extern int l_remove_pid(int pid);
extern void l_get_next_prio(int lid, int pid, int *new_pid, int *new_prio);

#ifdef DBG_ON
extern void l_printq(int head, int tail);
extern void l_print_locks_all(void);
extern void l_print_locks_active(void);
extern void l_print_locks_deleted(void);
extern void l_print_locks_free(void);
extern void l_print_lock_details(int lid);
extern void l_print_locks(int lid);
extern void l_print_pid_details(int pid);
#endif /* DBG_ON */
#endif /* _LOCK_H_ */
