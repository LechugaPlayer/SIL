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

struct socket_definition{
 EFamily addr_family;
 EType socket_type;
};

struct Address;

  void    init     ();
  void    shutdown ();
  Socket  socket   (socket_definition def);
  Socket  connect  (const std::string *host,const  std::string *service, socket_definition def);
  void    listen   (Socket socket, int backlog);
  void    bind     (Socket socket, socket_definition def,const std::string *service);
  Socket  accept   (Socket socket, socket_definition def ,const std::string *host,const std::string *service);

  ssize_t sendRawTo   (Socket socket, const void *buf, size_t nbytes, Address &addr, int flags = 0);
  ssize_t sendRaw     (Socket socket, const void *buf, size_t nbytes, int flags = 0);
  ssize_t recvRawFrom (Socket socket, void *buf, size_t nbytes, Address &addr, int flags = 0);
  ssize_t recvRaw     (Socket socket, void *buf, size_t nbytes, int flags = 0);


  
  ssize_t sendMsgTo   (Socket socket, const std::string &msg, Address &address, int flags = 0);
  ssize_t sendMsg     (Socket socket, const std::string &msg, int flags = 0);
  ssize_t recvMsgFrom (Socket socket, std::string &msg, Address &address, int flags = 0);
  ssize_t recvMsg     (Socket socket, std::string &msg, int flags = 0);
  void    close       (Socket socket);

struct Address{
 public:
  Address(const std::string &host, const std::string &service, socket_definition def){
    resolve(host.c_str(), service.c_str(), def);
  }
   
  Address(const std::string &host, uint16_t port, socket_definition def){
   std::string serviceStr = std::to_string(port);
   resolve(host.c_str(), serviceStr.c_str(), def);
  }

  Address(uint16_t port, socket_definition def){
   std::string serviceStr = std::to_string(port);
   resolve(nullptr, serviceStr.c_str(), def);
  }
  
  friend  ssize_t sendRawTo  (Socket socket, const void *buf, size_t nbytes, Address &addr, int flags);
  friend  ssize_t recvRawFrom(Socket socket, void *buf, size_t nbytes, Address &addr, int flags);
  friend  ssize_t sendMsgTo  (Socket socket, const std::string &msg, Address &address, int flags);
  friend  ssize_t recvMsgFrom(Socket socket, std::string &msg, Address &address, int flags);
  
  private:  
  sockaddr_storage addr {};
  socklen_t len {};
  void resolve(const char *host, const char *service, socket_definition def);
};


  
}
