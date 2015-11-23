#ifndef NET_IP4_UDP_SOCKET_HPP
#define NET_IP4_UDP_SOCKET_HPP
#include "udp.hpp"
#include "../socket.hpp"
#include <string>

namespace net
{
  class SocketUDP //: public Socket
  {
  public:
    typedef UDP::port port_t;
    typedef IP4::addr addr_t;
    typedef IP4::addr multicast_group_addr;
    
    typedef delegate<int(SocketUDP&, addr_t, port_t, const std::string&)> recvfrom_handler;
    typedef delegate<int(SocketUDP&, addr_t, port_t, const std::string&)> sendto_handler;
    
    // constructors
    SocketUDP(Inet<LinkLayer,IP4>&);
    SocketUDP(Inet<LinkLayer,IP4>&, port_t port);
    SocketUDP(const SocketUDP&) = delete;
    
    // functions
    inline void onRead(recvfrom_handler func)
    {
      on_read = func;
    }
    inline void onWrite(sendto_handler func)
    {
      on_send = func;
    }
    int write(addr_t destIP, port_t port, 
              const std::string& buffer);
    void close();
    
    void join(multicast_group_addr&);
    void leave(multicast_group_addr&);
    
    // stuff
    addr_t local_addr() const
    {
      return stack.ip_addr();
    }
    port_t local_port() const
    {
      return l_port;
    }
    
  private:
    void packet_init(std::shared_ptr<PacketUDP>, addr_t, port_t, uint16_t);
    int  internal_read(std::shared_ptr<PacketUDP> udp);
    
    Inet<LinkLayer,IP4>& stack;
    port_t l_port;
    recvfrom_handler on_read;
    sendto_handler   on_send;
    
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender
    
    friend class UDP;
  };
}

#endif
