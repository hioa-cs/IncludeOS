//#define DEBUG // Allow debug
//#define DEBUG2

#include <virtio/virtio.hpp>
#include <malloc.h>
#include <string.h>
#include <syscalls.hpp>
//#include <virtio/virtio.h>
#include <assert.h>


/** 
    Virtio Queue class, nested inside Virtio.
 */
#define ALIGN(x) (((x) + PAGE_SIZE) & ~PAGE_SIZE) 
unsigned Virtio::Queue::virtq_size(unsigned int qsz) 
{ 
  return ALIGN(sizeof(virtq_desc)*qsz + sizeof(u16)*(3 + qsz)) 
    + ALIGN(sizeof(u16)*3 + sizeof(virtq_used_elem)*qsz); 
}


void Virtio::Queue::init_queue(int size, void* buf){

  // The buffer starts with is an array of queue descriptors
  _queue.desc = (virtq_desc*)buf;
  debug("\t * Queue desc  @ 0x%lx \n ",(long)_queue.desc);

  // The available buffer starts right after the queue descriptors
  _queue.avail = (virtq_avail*)((char*)buf + size*sizeof(virtq_desc));  
  debug("\t * Queue avail @ 0x%lx \n ",(long)_queue.avail);

  // The used queue starts at the beginning of the next page
  // (This is  a formula from sanos - don't know why it works, but it does
  // align the used queue to the next page border)  
  _queue.used = (virtq_used*)(((uint32_t)&_queue.avail->ring[size] +
                                sizeof(uint16_t)+PAGESIZE-1) & ~(PAGESIZE -1));
  debug("\t * Queue used  @ 0x%lx \n ",(long)_queue.used);
  
}



/** A default handler doing nothing. 
    
    It's here because we might not want to look at the data, e.g. for 
    the VirtioNet TX-queue which will get used buffers in. */
int empty_handler(uint8_t* UNUSED(data),int UNUSED(size)) {
  debug("<Virtio::Queue> Empty handler. DROP! ");
  return -1;
};

/** Constructor */
Virtio::Queue::Queue(uint16_t size, uint16_t q_index, uint16_t iobase)
  : _size(size),_size_bytes(virtq_size(size)),_iobase(iobase),_num_free(size),
    _free_head(0), _num_added(0),_last_used_idx(0),_pci_index(q_index),
    _data_handler(delegate<int(uint8_t*,int)>(empty_handler))
{
  //Allocate space for the queue and clear it out
  void* buffer = memalign(PAGE_SIZE,_size_bytes);
  
  // The queues has to be page-aligned, so this crashes:
  // void* buffer = malloc(_size_bytes);
  
  
  //void* buffer = malloc(_size_bytes);
  if (!buffer) panic("Could not allocate space for Virtio::Queue");
  memset(buffer,0,_size_bytes);    

  debug(">>> Virtio Queue of size %i (%li bytes) initializing \n",
         _size,_size_bytes);  
  init_queue(size,buffer);
  
  debug("\t * Chaining buffers \n");  
  
  // Chain buffers  
  for (int i=0; i<size; i++) _queue.desc[i].next = i +1;
  _queue.desc[size -1].next = 0;

  
  // Allocate space for actual data tokens
  //_data = (void**) malloc(sizeof(void*) * size);
  
  
  debug(" >> Virtio Queue setup complete. \n");
}


/** Ported more or less directly from SanOS. */
int Virtio::Queue::enqueue(scatterlist sg[], uint32_t out, uint32_t in, void* UNUSED(data)){
  
  uint16_t i,avail,head, prev = _free_head;
  
  
  while (_num_free < out + in){ // Queue is full (we think)
    //while( num_avail() >= _size) // Wait for Virtio
    printf("<Q %i>Buffer full (%i avail,"               \
           " used.idx: %i, avail.idx: %i )\n",
           _pci_index,num_avail(),
           _queue.used->idx,_queue.avail->idx
           );
        panic("Buffer full");
  }

  // Remove buffers from the free list  
  _num_free -= out + in;
  head = _free_head;
  
  
  // (implicitly) Mark all outbound tokens as device-readable
  for (i = _free_head; out; i = _queue.desc[i].next, out--) 
    {
      _queue.desc[i].flags = VIRTQ_DESC_F_NEXT;
      _queue.desc[i].addr = (uint64_t)sg->data;
      _queue.desc[i].len = sg->size;

      debug("<Q %i> Enqueueing outbound: index %i len %li, next %i\n",
            _pci_index,i,_queue.desc[i].len,_queue.desc[i].next);

      prev = i;
      sg++;
    }
  
  // Mark all inbound tokens as device-writable
  for (; in; i = _queue.desc[i].next, in--) 
    {
      debug("<Q> Enqueuing inbound \n");
      _queue.desc[i].flags = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
      _queue.desc[i].addr = (uint64_t)sg->data;
      _queue.desc[i].len = sg->size;
      prev = i;
      sg++;
    }
  
  // No continue on last buffer
  _queue.desc[prev].flags &= ~VIRTQ_DESC_F_NEXT;
  
  
  // Update free pointer
  _free_head = i;

  // Set callback token
  //vq->data[head] = data;

  // SanOS: Put entry in available array, but do not update avail->idx until sync
  avail = (_queue.avail->idx + _num_added++) % _size;
  _queue.avail->ring[avail] = head;
  debug("<Q %i> avail: %i \n",_pci_index,avail);
  
  // Notify about free buffers
  //if (_num_free > 0) set_event(&vq->bufavail);
    
  return _num_free;  
}

void Virtio::Queue::release(uint32_t head){
  
  // Clear callback data token
  //vq->data[head] = NULL;

  // Mark queue element "head" as free (the whole token chain)
  uint16_t i = head;
  
  //It's at least one token...
  _num_free++;

  //...possibly with a tail
  while (_queue.desc[i].flags & VIRTQ_DESC_F_NEXT) 
  {
    i = _queue.desc[i].next;
    _num_free++;
  }
  
  // Add buffers back to free list
  
  // What happens here?
  debug2("<Q %i> desc[%i].next : %i \n",_pci_index,i,_queue.desc[i].next);
  
  // SanOS resets _free_head to this one and builds a "free list". 
  // ...but that list never has an end, so is there any point? It keeps the 
  // TX-tokens from rotating.
  
  // 
  //_queue.desc[i].next = _free_head;
  //_free_head = head;

  // SanOS: Notify about free buffers
  // Now this thread can wake up threads waiting to enqueue...
  // But IncludeOS doesn't have threads, so we have to defer transmissions
  // if (_num_free > 0) set_event(&vq->bufavail);
}

uint8_t* Virtio::Queue::dequeue(uint32_t* len){

  // Return NULL if there are no more completed buffers in the queue
  if (_last_used_idx == _queue.used->idx){
    debug("<Q %i> Can't dequeue - no used buffers \n",_pci_index);
    return NULL;
  }

  // Get next completed buffer
  auto* e = &_queue.used->ring[_last_used_idx % _size];
  *len = e->len;

  debug2("<Q %i> Releasing token %li. Len: %li\n",_pci_index,e->id, e->len);
  uint8_t* data = (uint8_t*)_queue.desc[e->id].addr;
  
  // Release buffer
  release(e->id);
  _last_used_idx++;

  return data;
}

//TEMP. REMOVE - This belongs to VirtioNet.
struct virtio_net_hdr
  {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
    uint16_t num_buffers;      // ONLY if "Merge RX-buffers"
  };



void Virtio::Queue::notify(){
  debug("\t <Q %i> Notified, checking buffers.... \n",_pci_index);
  debug("\t             Used idx: %i, Avail idx: %i \n",
        _queue.used->idx, _queue.avail->idx );
  
  int new_packets = _queue.used->idx - _last_used_idx;
  
  if (new_packets && _queue.used->idx >= _queue.avail->idx)
    printf("<Q %i> !!! BUFER FULL !!!  \n",_pci_index);


  
  debug("\t <VirtQueue> %i new packets: \n", new_packets);
    
  // For each token, extract data. We're merging RX-buffers, so no chaining 
  for (;_last_used_idx != _queue.used->idx; _last_used_idx++){
    auto id = _queue.used->ring[_last_used_idx % _size].id;
    auto len = _queue.used->ring[_last_used_idx % _size].len;
    debug("\tHandling packet id: 0x%lx len: %li last used: %i Q used idx: %i\n",
          id,len,_last_used_idx, _queue.used->idx);        

    auto tok_addr = _queue.desc[id].addr;    
    uint8_t* data = (uint8_t*)tok_addr + sizeof(virtio_net_hdr); 
    
    // Push data to a handler
    _data_handler(data, len);
    
  }
  
}


void Virtio::Queue::set_data_handler(delegate<int(uint8_t* data,int len)> del){
  _data_handler=del;
};


void Virtio::Queue::disable_interrupts(){
  _queue.avail->flags |= (1 << VIRTQ_AVAIL_F_NO_INTERRUPT);
}

void Virtio::Queue::enable_interrupts(){
  _queue.avail->flags &= ~(1 << VIRTQ_AVAIL_F_NO_INTERRUPT);
}

void Virtio::Queue::kick(){
  //__sync_synchronize ();

  // Atomically increment (maybe not necessary?)
  //__sync_add_and_fetch(&(_queue.avail->idx),_num_added); 
  _queue.avail->idx += _num_added;
  //__sync_synchronize ();

  _num_added = 0;
 

  if (!(_queue.used->flags & VIRTQ_USED_F_NO_NOTIFY)){
    debug("<Queue %i> Kicking virtio. Iobase 0x%x \n",
          _pci_index, _iobase);
    //outpw(_iobase + VIRTIO_PCI_QUEUE_SEL, _pci_index);
    outpw(_iobase + VIRTIO_PCI_QUEUE_NOTIFY , _pci_index);
  }else{
    debug("<VirtioQueue>Virtio device says we can't kick!");
  }
}
