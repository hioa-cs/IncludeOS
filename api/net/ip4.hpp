#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>

#include <net/inet_common.hpp>
#include <net/ethernet.hpp>
#include <net/inet.hpp>
#include <iostream>
#include <string>

namespace net {
  
  class Packet;

  // Default delegate assignments
  int ignore_ip4_up(std::shared_ptr<Packet>);
  int ignore_ip4_down(std::shared_ptr<Packet>);
  
  /** IP4 layer */
  class IP4 {  
  public:
  
    /** Known transport layer protocols. */
    enum proto{IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6};
    
    /** IP4 address */
    union __attribute__((packed)) addr{
      uint8_t part[4];
      uint32_t whole;
    
      // Constructors:
      // Can't have them - that removes the packed-attribute    
      inline bool operator==(addr rhs) const
      { return whole == rhs.whole; }
      
      inline bool operator<(const addr rhs) const
      { return whole < rhs.whole; }

      inline bool operator>(const addr rhs) const
      { return ! (*this < rhs); }
      
      inline bool operator!=(const addr rhs) const
      { return whole != rhs.whole; }
      
      inline bool operator!=(const uint32_t rhs) const
      { return  whole != rhs; }
      
      std::string str() const
      {
        std::string s; s.resize(16);
        sprintf((char*) s.c_str(), "%1i.%1i.%1i.%1i", 
                part[0], part[1], part[2], part[3]);
        return s;
      }      
    };
    

    /** IP4 header */
    struct ip_header{
      uint8_t version_ihl;
      uint8_t tos;
      uint16_t tot_len;
      uint16_t id;
      uint16_t frag_off_flags;
      uint8_t ttl;
      uint8_t protocol;
      uint16_t check;
      addr saddr;
      addr daddr;
    };

    /** The full header including IP. 
	@Note : This might be removed if we decide to isolate layers more.
     */
    struct full_header{
      uint8_t link_hdr [sizeof(typename LinkLayer::header)];
      ip_header ip_hdr;
    };
        
    /** Upstream: Input from link layer. */
    int bottom(Packet_ptr pckt);
    
    /** Upstream: Outputs to transport layer*/
    inline void set_icmp_handler(upstream s)
    { icmp_handler_ = s; }
    
    inline void set_udp_handler(upstream s)
    { udp_handler_ = s; }
    
    
    inline void set_tcp_handler(upstream s)
    { tcp_handler_ = s; }
    
  
    /** Downstream: Delegate linklayer out */
    void set_linklayer_out(downstream s)
    { linklayer_out_ = s; };
  
    /** Downstream: Receive data from above and transmit. 
        
        @note The following *must be set* in the packet:
        
        * Destination IP
        * Protocol
        
        Source IP *can* be set - if it's not, IP4 will set it.
        
    */
    int transmit(Packet_ptr pckt);

    /** Compute the IP4 header checksum */
    uint16_t checksum(ip_header* hdr);
    
    /**
     * \brief
     * Returns the IPv4 address associated with this interface
     * 
     **/
    const addr& local_ip() const
    {
      return local_ip_;
    }
    
    /** Initialize. Sets a dummy linklayer out. */
    IP4(Inet<LinkLayer,IP4>&);
    
  
  private:    
    addr local_ip_{};
    addr netmask_{};
    addr gateway_{};
    
    /** Downstream: Linklayer output delegate */
    downstream linklayer_out_ = downstream(ignore_ip4_down);;
    
    /** Upstream delegates */
    upstream icmp_handler_ = upstream(ignore_ip4_up);
    upstream udp_handler_ = upstream(ignore_ip4_up);
    upstream tcp_handler_ = upstream(ignore_ip4_up);
    
  };

  
  /** Pretty printing to stream */
  inline std::ostream& operator<<(std::ostream& out, const IP4::addr& ip)
  {
    return out << ip.str();
  }

} // ~net
#endif
