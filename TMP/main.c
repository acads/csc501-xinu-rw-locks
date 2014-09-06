/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

void halt();

int l1, l2;
int pa, pb, pc;

void
procA(void)
{
    kprintf("%s to lock l1, prio %d\n",
            L_GET_PNAME(getpid()), getprio(getpid()));
    lock(l1, WRITE, 50);
    kprintf("%s locked l1, prio %d\n",
            L_GET_PNAME(getpid()), getprio(getpid()));

    sleep(9);

    kprintf("%s to release l1, prio %d\n",
            L_GET_PNAME(getpid()), getprio(getpid()));
    kprintf("%s relall: %d\n", L_GET_PNAME(getpid()), releaseall(1, l1));
    kprintf("%s released l1, prio %d\n",
            L_GET_PNAME(getpid()), getprio(getpid()));

    return;
}

void
procB(void)
{
    int i = 0;
    int j = 0;

    kprintf("%s start..\n", L_GET_PNAME(getpid()));
    for (i = 0; i < 100; ++i, j = 0) {
        if (i == 2) {
            chprio(pc, 90);
            kprintf("pc prio changed to 90\n");
        }
        while (++j)
            if (10000 == j) break;
    }
    kprintf("%s end..\n", L_GET_PNAME(getpid()));
    return;
}

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main()
{
    kprintf("\n\nHello World, Xinu lives\n\n");
    //task1();
    l1 = lcreate();
    l2 = lcreate();

    pa = create(procA, 512, 30, "procA", 0, 0);
    pb = create(procB, 512, 40, "procB", 0, 0);
    pc = create(procA, 512, 50, "procC", 0, 0);

    resume(pa);
    sleep100(5);
    resume(pb);
    sleep100(5);
    resume(pc);
    sleep100(9);

    return 0;
}

