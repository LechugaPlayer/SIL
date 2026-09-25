#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>

namespace sil{

using Socket  = intptr_t;
constexpr Socket INVALID_SOCKET_HANDLE = -1;
constexpr int MAX_BUF_SIZE = 100;

 enum class EFamily{
  LOCAL,
  IPV4,
  IPV6,
  UNSPECIFIED
 };

 enum class EType{
  STREAM,
  DATAGRAM
 };

struct SocketDefinition{
 EFamily addr_family;
 EType socket_type;
};

struct SockAddr
;

  void    init     ();
  void    shutdown ();
  Socket  socket   (SocketDefinition def);
  bool    connect  (Socket socket, SockAddr &address);
  void    listen   (Socket socket, int backlog);
  bool    bind     (Socket socket, SockAddr &address);
  Socket  accept   (Socket socket, SockAddr &adress);

  ssize_t sendRawTo   (Socket socket, const void *buf, size_t nbytes, SockAddr &addr, int flags = 0);
  ssize_t sendRaw     (Socket socket, const void *buf, size_t nbytes, int flags = 0);
  ssize_t recvRawFrom (Socket socket, void *buf, size_t nbytes, SockAddr &addr, int flags = 0);
  ssize_t recvRaw     (Socket socket, void *buf, size_t nbytes, int flags = 0);
  
  ssize_t sendMsgTo   (Socket socket, const std::string &msg, SockAddr &address, int flags = 0);
  ssize_t sendMsg     (Socket socket, const std::string &msg, int flags = 0);
  ssize_t recvMsgFrom (Socket socket, std::string &msg, SockAddr &address, int flags = 0);
  ssize_t recvMsg     (Socket socket, std::string &msg, int flags = 0);
  void    close       (Socket socket);

struct SockAddr
{
 public:
  SockAddr(const std::string &host, const std::string &service, SocketDefinition def){
    resolve(host.c_str(), service.c_str(), def);
  }
   
  SockAddr(const std::string &host, uint16_t port, SocketDefinition def){
   std::string serviceStr = std::to_string(port);
   resolve(host.c_str(), serviceStr.c_str(), def);
  }

  SockAddr(uint16_t port, SocketDefinition def){
   std::string serviceStr = std::to_string(port);
   resolve(nullptr, serviceStr.c_str(), def);
  }
  
  friend  bool    connect    (Socket socket, SockAddr &address);
  friend  Socket  accept     (Socket socket, SockAddr &adress);
  friend  bool    bind       (Socket socket, SockAddr &address);
  friend  ssize_t sendRawTo  (Socket socket, const void *buf, size_t nbytes, SockAddr &addr, int flags);
  friend  ssize_t recvRawFrom(Socket socket, void *buf, size_t nbytes, SockAddr &addr, int flags);
  friend  ssize_t sendMsgTo  (Socket socket, const std::string &msg, SockAddr &address, int flags);
  friend  ssize_t recvMsgFrom(Socket socket, std::string &msg, SockAddr &address, int flags);
  
  private:  
  sockaddr_storage addr {};
  socklen_t len {};
  void resolve(const char *host, const char *service, SocketDefinition def);
};


  
}
