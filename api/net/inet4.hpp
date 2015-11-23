#ifndef NET_INET4_HPP
#define NET_INET4_HPP

#include <syscalls.hpp> // panic()
#include <dev.hpp> // 107: auto& eth0 = Dev::eth(0);
#include <net/inet.hpp>
#include <net/ethernet.hpp>
#include <net/arp.hpp>
#include <net/ip4.hpp>
#include <net/icmp.hpp>
#include "ip4/udp.hpp"
#include <net/tcp.hpp>

#include <vector>

#include <nic.hpp>

namespace net {
     
  /** A complete IP4 network stack */
  template <typename DRIVER>
  class Inet4 : public Inet<Ethernet, IP4>{
  public:
    
    inline const Ethernet::addr& link_addr() override 
    { return eth_.mac(); }
    
    inline Ethernet& link() override
    { return eth_; }    
    
    inline const IP4::addr& ip_addr() override 
    { return ip4_addr_; }

    inline const IP4::addr& netmask() override 
    { return netmask_; }
    
    inline const IP4::addr& router() override 
    { return router_; }
    
    inline IP4& ip_obj() override
    { return ip4_; }
    
    /** Get the TCP-object belonging to this stack */
    inline TCP& tcp() override { debug("<TCP> Returning tcp-reference to %p \n",&tcp_); return tcp_; }        

    /** Get the UDP-object belonging to this stack */
    inline UDP& udp() override { return udp_; };
    
    /** Create a Packet, with a preallocated buffer.
	@param size : the "size" reported by the allocated packet. 
	@note as of v0.6.3 this has no effect other than to force the size to be
	set explicitly by the caller. 
	@todo make_shared will allocate with new. This is fast in IncludeOS,
	(no context switch for sbrk) but consider overloading operator new.
    */
    inline Packet_ptr createPacket(size_t size) override {
      // Create a release delegate, for returning buffers
      auto release = BufferStore::release_del::from
	<BufferStore, &BufferStore::release_offset_buffer>(nic_.bufstore());
      // Create the packet, using  buffer and .
      return std::make_shared<Packet>(bufstore_.get_offset_buffer(), 
				      bufstore_.offset_bufsize(), size, release);
    }
    
    // We have to ask the Nic for the MTU
    virtual inline uint16_t MTU() const override
    { return nic_.MTU(); }
    
    /** We don't want to copy or move an IP-stack. It's tied to a device. */
    Inet4(Inet4&) = delete;
    Inet4(Inet4&&) = delete;
    Inet4& operator=(Inet4) = delete;
    Inet4 operator=(Inet4&&) = delete;
    
    /** Initialize.  */
    Inet4(Nic<DRIVER>& nic, IP4::addr ip, IP4::addr netmask); 
    
  private:
    virtual void 
    network_config(IP4::addr addr, IP4::addr nmask, IP4::addr router) override
    {
      this->ip4_addr_ = addr;
      this->netmask_  = nmask;
      this->router_   = router;
    }
    
    IP4::addr ip4_addr_;
    IP4::addr netmask_;
    IP4::addr router_;
    
    // This is the actual stack
    Nic<DRIVER>& nic_;
    Ethernet eth_;
    Arp arp_;
    IP4  ip4_;
    ICMP icmp_;
    UDP  udp_;
    TCP tcp_;
    
    BufferStore& bufstore_;
    friend class DHClient;
  };
}

#include "inet4.inc"

#endif
