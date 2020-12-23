// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
//   struct spinlock lock;
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  struct buf hashbucket[NBUCKETS];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
//   struct buf head;
} bcache;

uint hash(uint blockno)
{
    return (blockno % NBUCKETS);
}

void
binit(void)
{
  struct buf *b;

//   initlock(&bcache.lock, "bcache");
  for (int i = 0;i < NBUCKETS;++i){
      initlock(&bcache.lock[i],"bcache.hashbucket");
      // Create linked list of buffers
      bcache.hashbucket[i].prev = &bcache.hashbucket[i];
      bcache.hashbucket[i].next = &bcache.hashbucket[i];
      
  }
  // Create linked list of buffers
//   bcache.head.prev = &bcache.head;
//   bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
      //初始化的时候把所有buffer都放在bucket0中
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint bucketNo = hash(blockno);

  acquire(&bcache.lock[bucketNo]);

  // Is the block already cached?
  //no
  for (b = bcache.hashbucket[bucketNo].next; b != &bcache.hashbucket[bucketNo]; b = b->next){
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        release(&bcache.lock[bucketNo]);
        acquiresleep(&b->lock);
        return b;
    }
  }
//   for(b = bcache.head.next; b != &bcache.head; b = b->next){
//     if(b->dev == dev && b->blockno == blockno){
//       b->refcnt++;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }

  // Not cached; recycle an unused buffer. by LRU
  uint newBucketNo = hash(bucketNo+1);
  while (newBucketNo != bucketNo){
      acquire(&bcache.lock[newBucketNo]);//获取当前bucket的锁
      for (b = bcache.hashbucket[newBucketNo].prev; b != &bcache.hashbucket[newBucketNo]; b = b->prev){
          if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            //从原来的bucket链表断开
            b->next->prev = b->prev;
            b->prev->next = b->next;
            release(&bcache.lock[newBucketNo]);//释放
            //插入到blockno对应的bucket中
            //双向链表
            b->next = bcache.hashbucket[bucketNo].next;
            b->prev = &bcache.hashbucket[bucketNo];
            bcache.hashbucket[bucketNo].next->prev = b;
            bcache.hashbucket[bucketNo].next = b;
            release(&bcache.lock[bucketNo]);
            acquiresleep(&b->lock);
            return b;
          }
      }
      //当前的找不到，接着找，先释放当前bucket的锁
      release(&bcache.lock[newBucketNo]);
      newBucketNo = hash(newBucketNo+1);
  }

//   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
//     if(b->refcnt == 0) {
//       b->dev = dev;
//       b->blockno = blockno;
//       b->valid = 0;
//       b->refcnt = 1;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint bucketNo = hash(b->blockno);
  acquire(&bcache.lock[bucketNo]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[bucketNo].next;
    b->prev = &bcache.hashbucket[bucketNo];
    bcache.hashbucket[bucketNo].next->prev = b;
    bcache.hashbucket[bucketNo].next = b;
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
  }
  
  release(&bcache.lock[bucketNo]);
}

void
bpin(struct buf *b) {
  uint buckNo = hash(b->blockno);
  acquire(&bcache.lock[buckNo]);
  b->refcnt++;
  release(&bcache.lock[buckNo]);
}

void
bunpin(struct buf *b) { 
  uint buckNo = hash(b->blockno);
  acquire(&bcache.lock[buckNo]);
  b->refcnt--;
  release(&bcache.lock[buckNo]);
}


