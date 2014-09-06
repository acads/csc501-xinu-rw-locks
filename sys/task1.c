/* adhanas */
/* task1.c -- PA2, additional task 1.2 */

#include <conf.h>
#include <kernel.h>
#include <sem.h>
#include <lock.h>
#include <proc.h>
#include <stdio.h>

int s1;
int l1;
int pa, pb, pc, pd, pe, pf;

void
proc_sem(void)
{
    unsigned long t1, t2, i = 0, j = 0;

    kprintf("%s start..\n", L_GET_PNAME(currpid));
    kprintf("%s to acquire sem\n", L_GET_PNAME(currpid));
    t1 = ctr1000;
    wait(s1);
    t2 = ctr1000;
    kprintf("%s acquired sem\n", L_GET_PNAME(currpid));

    for (j = 0; j < 10; ++j) {
        sleep1000(2);
        while (++i)
            if (100 == i) break;
    }

    kprintf("%s prio %d\n", L_GET_PNAME(currpid), L_GET_PPRIO(currpid));
    kprintf("%s to release sem\n", L_GET_PNAME(currpid));
    signal(s1);
    kprintf("%s released sem\n", L_GET_PNAME(currpid));
    kprintf("%s spent %u waiting on sem\n", L_GET_PNAME(currpid), t2 - t1);
    kprintf("%s end..\n", L_GET_PNAME(currpid));
    return;
}

void
proc_misc(void)
{
    unsigned long i = 0, j = 0;

    kprintf("%s start..\n", L_GET_PNAME(currpid));

    for (j = 0; j < 10; ++j) {
        sleep1000(2);
        while (++i)
            if (100 == i) break;
    }

    kprintf("%s prio %d\n", L_GET_PNAME(currpid), L_GET_PPRIO(currpid));
    kprintf("%s end..\n", L_GET_PNAME(currpid));

    return;
}

void
proc_lock(void)
{
    unsigned long t1, t2, i = 0, j = 0;

    kprintf("%s start..\n", L_GET_PNAME(currpid));
    kprintf("%s to acquire lock\n", L_GET_PNAME(currpid));
    t1 = ctr1000;
    lock(l1, WRITE, 50);
    t2 = ctr1000;
    kprintf("%s acquired lock\n", L_GET_PNAME(currpid));

    for (j = 0; j < 10; ++j) {
        sleep1000(2);
        while (++i)
            if (100 == i) break;
    }

    kprintf("%s prio %d\n", L_GET_PNAME(currpid), L_GET_PPRIO(currpid));
    kprintf("%s to release lock\n", L_GET_PNAME(currpid));
    releaseall(1, l1);
    kprintf("%s released lock\n", L_GET_PNAME(currpid));
    kprintf("%s spent %u waiting on lock\n", L_GET_PNAME(currpid), t2 - t1);
    kprintf("%s end..\n", L_GET_PNAME(currpid));
    return;
}
    
void
create_procs(int type)
{
    if (1 == type) {
        pa = create((int *) proc_sem, 512, 20, "procA", 0, 0);
        pb = create((int *) proc_misc, 512, 25, "procB", 0, 0);
        pc = create((int *) proc_sem, 512, 30, "procC", 0, 0);
    } else if (2 == type) {
        pa = create((int *) proc_lock, 512, 20, "procA", 0, 0);
        pb = create((int *) proc_misc, 512, 25, "procB", 0, 0);
        pc = create((int *) proc_lock, 512, 30, "procC", 0, 0);
    }
    resume(pa);
    resume(pb);
    resume(pc);
    return;
}

void
task1(void)
{
    /* Test Xinu's semaphore */
    kprintf("Testing Xinu semaphores..\n");
    s1 = screate(1);
    kprintf("sem created\n");
    create_procs(1);
    sleep(1);
    wait(s1);
    signal(s1);
    sdelete(s1);
    kprintf("sem deleted\n");

    sleep(2);

    /* Test PA2 locks */
    kprintf("\n\nTesting PA2 locks..\n");
    l1 = lcreate();
    kprintf("lock created\n");
    create_procs(2);
    sleep(1);
    lock(l1, WRITE, 50);
    releaseall(1, l1);
    ldelete(l1);
    kprintf("lock deleted\n");

    return;
}

