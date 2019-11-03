#ifndef _DEFS_H_
#define _DEFS_H_

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct stat;
struct superblock;
struct trapframe;

typedef uint pte_t;
typedef uint pde_t;
extern uint *dev_pte;
extern uint *pte;
extern uint *kt;
extern uint *ut;
typedef void (*ISR) (struct trapframe *, int);

// bio.c
void            binit(void);
struct buf *    bread(uint, uint);
void            brelse(struct buf *);
void            bwrite(struct buf *);

// console.c
void            consoleinit(void);
void            cprintf(char *, ...);
void            consoleintr(int(*)(void));
void            panic(char *) __attribute__((noreturn));

// exec.c
int             exec(char *, char **);

// file.c
struct file *   filealloc(void);
void            fileclose(struct file *);
struct file *   filedup(struct file *);
void            fileinit(void);
int             fileread(struct file *, char *, int);
int             filestat(struct file *, struct stat *);
int             filewrite(struct file *, char *, int);

// fs.c
void            readsb(int, struct superblock *);
int             dirlink(struct inode *, char *, uint);
struct inode *  dirlookup(struct inode *, char *, uint *);
struct inode *  ialloc(uint, short);
struct inode *  idup(struct inode *);
void            iinit(void);
void            ilock(struct inode *);
void            iput(struct inode *);
void            iunlock(struct inode *);
void            iunlockput(struct inode *);
void            iupdate(struct inode *);
int             namecmp(const char *, const char *);
struct inode *  namei(char *);
struct inode *  nameiparent(char *, char *);
int             readi(struct inode *, char *, uint, uint);
void            stati(struct inode *, struct stat *);
int             writei(struct inode *, char *, uint, uint);

// ide.c
void            ideinit(void);
void            iderw(struct buf *);

// kalloc.c
char *          kalloc(void);
void            kfree(char *);
void            kinit1(void *, void *);
void            kinit2(void *, void *);
void            freerange(void *, void *);

// log.c
void            initlog(void);
void            log_write(struct buf *);
void            begin_trans();
void            commit_trans();

// mp.c
//extern int      ismp;
//void            mpinit(void);

// picirq.c
void            pic_enable(int, ISR);
void            pic_init(void *);
void            pic_dispatch(struct trapframe *);

// pipe.c
int             pipealloc(struct file **, struct file **);
void            pipeclose(struct pipe *, int);
int             piperead(struct pipe *, char *, int);
int             pipewrite(struct pipe *, char *, int);

// proc.c
//int             cpuid(void);
void            exit(void);
int             fork(void);
int             growproc(int);
int             kill(int);
//struct cpu *    mycpu(void);
//struct proc *   myproc(void);
void            pinit(void);
void            procdump(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            sleep(void *, struct spinlock *);
void            userinit(void);
int             wait(void);
void            wakeup(void *);
void            yield(void);

// swtch.S
void            swtch(struct context **, struct context *);

// spinlock.c
void            acquire(struct spinlock *);
void            getcallerpcs(void *, uint *);
void            show_callstk(char *);
int             holding(struct spinlock *);
void            initlock(struct spinlock *, char *);
void            release(struct spinlock *);
void            pushcli(void);
void            popcli(void);

// sleeplock.c
//void            acquiresleep(struct sleeplock *);
//void            releasesleep(struct sleeplock *);
//int             holdingsleep(struct sleeplock *);
//void            initsleeplock(struct sleeplock *, char *);

// string.c
int             memcmp(const void *, const void *, uint);
void *          memmove(void *, const void *, uint);
void *          memset(void *, int, uint);
char *          safestrcpy(char *, const char *, int);
int             strlen(const char *);
int             strncmp(const char *, const char *, uint);
char *          strncpy(char *, const char *, int);

// syscall.c
int             argint(int, int *);
int             argptr(int, char **, int);
int             argstr(int, char **);
int             fetchint(uint, int *);
int             fetchstr(uint, char **);
void            syscall(void);

// timer.c
void            timer_init(int);
extern struct   spinlock tickslock;

// trap.c
extern uint     ticks;
void            trap_init(void);
void            dump_tf(struct trapframe *);
void            set_stk(uint, uint);
void*           get_fp(void);

// trapasm.S
void            trap_reset(void);
void            trap_und(void);
void            trap_swi(void);
void            trap_iabort(void);
void            trap_dabort(void);
void            trap_reserved(void);
void            trap_irq(void);
void            trap_fiq(void);

// uart.c
void            uart_init(void *);
void            uartputc(int);
int             uartgetc(void);
void            micro_delay(int);
void            uart_enable_rx(void);

// vm.c
void            kvmalloc(void);
char*           uva2ka(pde_t*, char*);
int             allocuvm(pde_t *, uint, uint);
int             deallocuvm(pde_t *, uint, uint);
void            freevm(pde_t *);
void            inituvm(pde_t *, char *, uint);
int             loaduvm(pde_t *, char *, struct inode *, uint, uint);
pde_t *         copyuvm(pde_t *, uint);
void            switchuvm(struct proc*);
int             copyout(pde_t *, uint, void *, uint);
void            clearpteu(pde_t *, char *);

#endif