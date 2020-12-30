//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//
void sockclose(struct sock *so) {
    acquire(&so->lock);
    wakeup(&so->rxq);
    release(&so->lock);

    acquire(&lock);
    struct sock *sockk = sockets;
    if(sockk->next == 0)
        sockets = 0;
    while(sockk && sockk->next) {
        if (sockk->next->raddr == so->raddr &&
            sockk->next->lport == so->lport &&
	        sockk->next->rport == so->rport) {
            sockk->next = so->next;
            break;
        }
        sockk = sockk->next;
    }
    release(&lock);

    kfree((char*)so);
}

int sockwrite(struct sock *so, uint64 addr, int n){
    struct proc *p = myproc();

    struct mbuf *buf = mbufalloc(sizeof(struct eth) + sizeof(struct ip) + sizeof(struct udp));
    mbufput(buf, n);

    if(copyin(p->pagetable, buf->head, addr, n) == -1){
        mbuffree(buf);
        return -1;
    }

    net_tx_udp(buf, so->raddr, so->lport, so->rport);
    return n;
}

int sockread(struct sock *so, uint64 addr, int n) {
    struct proc *p = myproc();

    acquire(&so->lock);

    if (mbufq_empty(&so->rxq)) {
        while(mbufq_empty(&so->rxq))
            sleep(&so->rxq, &so->lock);
    }

    struct mbuf *buf = mbufq_pophead(&so->rxq);
    int len = n < buf->len ? n : buf->len;

    if(copyout(p->pagetable, addr, buf->head, len) == -1) {
        release(&so->lock);
        mbuffree(buf);
        return -1;
    }

    release(&so->lock);
    mbuffree(buf);
    return len;
}

// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  //mbuffree(m);
  struct sock *so = sockets;
  acquire(&lock);
  while(so){
    if (so->raddr == raddr &&
        so->lport == lport &&
	    so->rport == rport) {
            break;
    }
    so = so->next;
  }
  release(&lock);

  //没有套接字
  if (so == 0) {
      mbuffree(m);
      return;
  }

  acquire(&so->lock);
  mbufq_pushtail(&so->rxq, m);
  release(&so->lock);
  wakeup(&so->rxq);
}
