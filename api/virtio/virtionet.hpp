/**  
     STANDARD:      
     
     We're aiming for standard compliance:
     
     Virtio 1.0, OASIS Committee Specification Draft 01
     (http://docs.oasis-open.org/virtio/virtio/v1.0/csd01/virtio-v1.0-csd01.pdf)
     
     In the following abbreviated to Virtio 1.01
     
     ...Alas, nobody's using it yet, so we're stuck with "legacy" for now.
*/
     

#ifndef CLASS_VIRTIONET_H
#define CLASS_VIRTIONET_H


#include <common>
#include <pci_device.hpp>
#include <virtio/virtio.hpp>
#include <delegate>

#include <net/ethernet.hpp>
#include <net/buffer_store.hpp>

/** Virtio Net Features. From Virtio Std. 5.1.3 */

/** Device handles packets with partial checksum. This “checksum offload” 
   is a common feature on modern network cards.*/
#define VIRTIO_NET_F_CSUM 0

/** Driver handles packets with partial checksum. */
#define VIRTIO_NET_F_GUEST_CSUM 1

/** Control channel offloads reconfiguration support. */
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS 2

/** Device has given MAC address. */
#define VIRTIO_NET_F_MAC 5

/** Driver can receive TSOv4. */
#define VIRTIO_NET_F_GUEST_TSO4 7

/** Driver can receive TSOv6. */
#define VIRTIO_NET_F_GUEST_TSO6 8

/** Driver can receive TSO with ECN.*/
#define VIRTIO_NET_F_GUEST_ECN 9

/** Driver can receive UFO. (UFO?? WTF!) */
#define VIRTIO_NET_F_GUEST_UFO 10

/** Device can receive TSOv4. */
#define VIRTIO_NET_F_HOST_TSO4 11

/** Device can receive TSOv6. */
#define VIRTIO_NET_F_HOST_TSO6 12

/** Device can receive TSO with ECN. */
#define VIRTIO_NET_F_HOST_ECN 13

/** Device can receive UFO.*/
#define VIRTIO_NET_F_HOST_UFO 14

/** Driver can merge receive buffers. */
#define VIRTIO_NET_F_MRG_RXBUF 15

/** Configuration status field is available. */
#define VIRTIO_NET_F_STATUS 16

/** Control channel is available.*/
#define VIRTIO_NET_F_CTRL_VQ 17

/** Control channel RX mode support.*/
#define VIRTIO_NET_F_CTRL_RX 18

/** Control channel VLAN filtering.*/
#define VIRTIO_NET_F_CTRL_VLAN 19

/** Driver can send gratuitous packets.*/
#define VIRTIO_NET_F_GUEST_ANNOUNCE 21

/** Device supports multiqueue with automatic receive steering.*/
#define VIRTIO_NET_F_MQ 22

/** Set MAC address through control channel.*/
#define VIRTIO_NET_F_CTRL_MAC_ADDR 23


// From Virtio 1.01, 5.1.4
#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2

/** Virtio-net device driver.  */
class VirtioNet : Virtio {
  
  struct virtio_net_hdr
  {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
  }__attribute__((packed));

  /** Virtio std. § 5.1.6.1: 
      "The legacy driver only presented num_buffers in the struct virtio_net_hdr when VIRTIO_NET_F_MRG_RXBUF was not negotiated; without that feature the structure was 2 bytes shorter." */
  struct virtio_net_hdr_nomerge
  {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
    uint16_t num_buffers;
  }__attribute__((packed));

  
  /** An empty header.      
      It's ok to use as long as we don't need checksum offloading
      or other 'fancier' virtio features. */
  constexpr static virtio_net_hdr empty_header = {0,0,0,0,0,0}; 
  
  Virtio::Queue rx_q;
  Virtio::Queue tx_q;
  Virtio::Queue ctrl_q;

  // Moved to Nic
  // Ethernet eth; 
  // Arp arp;

  // From Virtio 1.01, 5.1.4
  struct config{
    net::Ethernet::addr mac;
    uint16_t status;
    
    //Only valid if VIRTIO_NET_F_MQ
    uint16_t max_virtq_pairs = 0; 
  }_conf;
  
  //sizeof(config) if VIRTIO_NET_F_MQ, else sizeof(config) - sizeof(uint16_t)
  int _config_length = sizeof(config);
  
  /** Get virtio PCI config. @see Virtio::get_config.*/
  void get_config();
  
  
  /** Service the RX Queue. 
      Push incoming data up to linklayer, dequeue RX buffers. */
  void service_RX();
  
  /** Service the TX Queue 
      Dequeue used TX buffers. @note: This function does not take any 
      responsibility for memory management. */
  void service_TX();

  /** Handle device IRQ. 
      
      Will look for config. changes and service RX/TX queues as necessary.*/
  void irq_handler();
  
<<<<<<< HEAD:api/virtio/class_virtionet.hpp
  /** Allocate and queue MTU-sized buffer in RX queue. */
  int add_receive_buffer();
  
  /** Queue the given buffer in RX queue. 
      @note This function doesn't allocate anything and expects the buffer
      to be a valid VirtioNet receive buffer. Used to requeue. */
  int add_receive_buffer(uint8_t* buf, int len);

=======
  /** Allocate and queue buffer from bufstore_ in RX queue. */
  int add_receive_buffer();  
>>>>>>> 0c91f1078f64f4b18dfbb2307194ec29d30d52d2:api/virtio/virtionet.hpp

  /** Upstream delegate for linklayer output */
  net::upstream _link_out;

  /** 20-bit / 1MB of buffers to start with */
  net::BufferStore bufstore_{ 0xfffff / MTU(),  MTU(), sizeof(virtio_net_hdr) };
  net::BufferStore::release_del release_buffer = 
    net::BufferStore::release_del::from
    <net::BufferStore, &net::BufferStore::release_offset_buffer>(bufstore_);
  
public:     
  
  /** Human readable name. */
  const char* name();  
  
  /** Mac address. */
  const net::Ethernet::addr& mac();
  
  constexpr uint16_t MTU() const { 
    return 1500 + sizeof(virtio_net_hdr); }
  
  /** Delegate linklayer output. Hooks into IP-stack bottom, w.UPSTREAM data. */
  inline void set_linklayer_out(net::upstream link_out){
    _link_out = link_out;
    //rx_q.set_data_handler(link_out);
  };
    
  inline net::BufferStore& bufstore() { return bufstore_; }
  
  /** Linklayer input. Hooks into IP-stack bottom, w.DOWNSTREAM data.*/
  int transmit(net::Packet_ptr pckt);
  
  /** Constructor. @param pcidev an initialized PCI device. */
  VirtioNet(PCI_Device& pcidev);
    

};

#endif
