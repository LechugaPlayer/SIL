#include <cstddef>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <stdint.h>

namespace sil{

using Socket  = intptr_t;
constexpr Socket INVALID_SOCKET_HANDLE = -1;

 enum class EFamily{
  LOCAL,
  IPV4,
  IPV6,
  UNSPECIFIED
 };

 enum class EType{
  STREAM,
  DATAGRAM,
  RAW  
 };

struct socket_definition{
 EFamily addr_family;
 EType socket_type;
};

  void    init     ();
  void    shutdown ();
  Socket  socket   (socket_definition def);
  Socket  connect  (const std::string *host,const  std::string *service, socket_definition def);
  void    listen   (Socket socket, int backlog);
  void    bind     (Socket socket, socket_definition def,const std::string *service);
  Socket  accept   (Socket socket, socket_definition def ,const std::string *host,const std::string *service);
  ssize_t sendTo   (Socket socket, const void *buf, size_t nbytes, int flags = 0);
  ssize_t send     (Socket socket, const void *buf, size_t nbytes, int flags = 0);
  ssize_t recvFrom (Socket socket, void *buf, size_t nbytes, int flags = 0);
  ssize_t recv     (Socket socket, void *buf, size_t nbytes, int flags = 0);
  void    close    (Socket socket);
  
}
