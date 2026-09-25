#include "sil.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>

#ifdef _WIN32
#include <winsock2.h>

                                     
#else
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <sys/un.h>

int EnumToMacro_Family(sil::EFamily var){

  switch (var) {
    case sil::EFamily::UNSPECIFIED:
      return AF_UNSPEC;
      
    case sil::EFamily::LOCAL:
      return AF_UNIX;
      
    case sil::EFamily::IPV4:
        return AF_INET;
        
    case sil::EFamily::IPV6:
        return AF_INET6;
        
      default:
      return -1;
}

  return 0;
}

int EnumToMacro_Type(sil::EType var){

  switch (var) {
    case sil::EType::DATAGRAM:
      return SOCK_DGRAM;
      
    case sil::EType::STREAM:
      return SOCK_STREAM;
            
      default:
      return -1;
    }
}

  void sil::SockAddr::resolve(const char* host, const char *service, sil::SocketDefinition def){
  
    if (def.addr_family == sil::EFamily::LOCAL) {
         if (host == nullptr) {
            fprintf(stderr,
                    "AF_UNIX address requires a socket path\n");
            return;
        }
        
      struct sockaddr_un addr{};
      addr.sun_family = AF_LOCAL;
      
      size_t max_length = sizeof(addr.sun_family) - 1;
      if (strlen(host) > max_length) {

        return;
      }
      
      memset(addr.sun_path, 0, strlen(host));
      strcpy(addr.sun_path, host);
      
      
      std::memcpy(&this->addr, &addr, sizeof(addr));
      this->len  = max_length;
    }else{
      struct addrinfo hints;

      hints.ai_canonname = NULL;
      hints.ai_protocol = 0;  
      hints.ai_next = NULL;
      hints.ai_flags = 0;
      hints.ai_addr = NULL;
      hints.ai_addrlen = 0;
      hints.ai_socktype = EnumToMacro_Type(def.socket_type);
      hints.ai_family = EnumToMacro_Family(def.addr_family);
      struct addrinfo* result = nullptr;
      struct addrinfo* rp = nullptr;
      int error;
      
  for (rp = result; rp != NULL; rp = rp->ai_next) {
      error = ::getaddrinfo(host, service, &hints, &result);
      if (error == -1){
          ::gai_strerror(error);
          return;
      }
      if (result == nullptr) {
        printf("No suitable address was found\n");
      }
      
      if(result->ai_addrlen > sizeof(this->addr)){
        printf("Resolved address is too large\n");
        ::freeaddrinfo(result);
        return;
      }
      
      std::memcpy(&this->addr, result->ai_addr, result->ai_addrlen );
      this->len = static_cast<socklen_t>(result->ai_addrlen);

      ::freeaddrinfo(result);
    }
  }
  }

//TODO: EError is not implemented because I haven't found a suitable abstraction,
// for now the library prints the error with PrintError()
  void PrintError(const char* perror_msg, const char *strerror_msg){
        int saved_errno {};
        saved_errno = errno;
        perror(perror_msg);
        printf(strerror_msg, strerror(saved_errno), saved_errno);
  }
  
  sil::Socket sil::socket(sil::SocketDefinition def){
    
    int fd = ::socket(EnumToMacro_Family(def.addr_family),
                          EnumToMacro_Type(def.socket_type),
                           0);
    
  if (fd == -1) {
    PrintError("Error creating socket via perror",
                "Error creating socket via strerror: %s (Code :%d)\n");

      return INVALID_SOCKET_HANDLE;
  }
    
    return fd;
  }
  
  bool sil::connect(sil::Socket socket, sil::SockAddr
                 &address){

    int fd = -1;
    fd = ::connect(socket, (struct sockaddr *)&address.addr , address.len);

    if (fd == -1) {
      return false;
    }else{
      return true;
    }
}

void sil::listen(Socket socket, int backlog){
  
  if(::listen(socket, backlog) == -1){
    PrintError("Error setting passive socket via perror",
                 "Error setting passive socket via strerror: %s (Code: %d)\n");
  }
}

bool sil::bind(sil::Socket socket, sil::SockAddr &address){
   int result = -1;

   result = ::bind(socket, reinterpret_cast<sockaddr*>(&address.addr), address.len);

   if (result == -1) {
     return false;
   }else{
   return true;
   }
}


sil::Socket sil::accept(sil::Socket socket, sil::SockAddr &address){
  Socket socket_result = -1;
  //TODO: maybe address.len should be initialized
    socket_result = ::accept(socket, reinterpret_cast<sockaddr*>(&address.addr), &address.len);

    if (socket_result == -1) {
        PrintError("Accept failed via perror",
                    "Accept failed via strerror: %s (Code %d)\n");
        socket_result = sil::INVALID_SOCKET_HANDLE; 
    }
    return socket_result;
}

ssize_t sil::sendRawTo(Socket socket, const void *buf, size_t nbytes, SockAddr &addr, int flags){
  ssize_t bytes_received = 0;
  
  bytes_received = ::sendto(socket, buf, nbytes, flags, reinterpret_cast<sockaddr*>(&addr.addr), addr.len);
  if (bytes_received == -1) {
          PrintError("Send failed via perror",
                      "Send failed via strerror: %s (Code %d) \n");
          return bytes_received;
  }
  return  bytes_received;
}

ssize_t sil::recvRawFrom(sil::Socket socket, void *buf, size_t nbytes, sil::SockAddr &address, int flags){
  ssize_t bytes_written = 0;
  address.len =  sizeof(address.addr);
  bytes_written = ::recvfrom(socket, buf, nbytes, flags, reinterpret_cast<sockaddr*>(&address.addr), &address.len);

  if (bytes_written == -1) {
          PrintError("Receive failed via perror",
                      "Receive failed via strerror: %s (Code %d)\n");
          return bytes_written;
  } 
  return bytes_written;
}

ssize_t sil::sendRaw(sil::Socket socket, const void *buf, size_t nbytes, int flags){
  ssize_t bytes_received = 0;
   
  bytes_received = ::send(socket, buf, nbytes, flags);
  if (bytes_received == -1) {
          PrintError("Send failed via perror",
                      "Send failed via strerror: %s (Code %d)\n");
          return bytes_received;
  }
  return  bytes_received;
}

ssize_t sil::recvRaw(sil::Socket socket, void *buf, size_t nbytes, int flags){
  ssize_t bytes_written = 0;
  
  bytes_written = ::recvfrom(socket, buf, nbytes, flags, 0, 0);
  if (bytes_written == -1) {
          PrintError("Receive failed via perror",
                      "Receive failed via strerror: %s (Code %d)\n");
          return bytes_written;
  }
  return bytes_written;
}

  ssize_t sil::sendMsgTo(sil::Socket socket, const std::string &msg, sil::SockAddr
                       &address, int flags){
    
  ssize_t bytes_sent = 0;
  bytes_sent = ::sendto(socket, msg.c_str(), msg.length(), flags, reinterpret_cast<sockaddr*>(&address.addr), address.len);

  if (bytes_sent == -1) {
          PrintError("Send failed via perror",
                      "Send failed via strerror: %s (Code %d) \n");
          return bytes_sent;
  }
  return  bytes_sent;  
}
  
  ssize_t sil::sendMsg(sil::Socket socket, const std::string &msg, int flags){
  ssize_t bytes_sent = 0;
  bytes_sent = ::send(socket, msg.c_str(), msg.length(), flags);

  if (bytes_sent == -1) {
          PrintError("Send failed via perror",
                      "Send failed via strerror: %s (Code %d)\n");
          return bytes_sent;
  }
  return  bytes_sent;
  }
  
  ssize_t sil::recvMsgFrom (sil::Socket socket, std::string &msg, sil::SockAddr &address, int flags){
  char buf[MAX_BUF_SIZE];
  address.len =  sizeof(address.addr);
  ssize_t bytes_written = 0;
  
  bytes_written = ::recvfrom(socket, buf, MAX_BUF_SIZE, flags, reinterpret_cast<sockaddr*>(&address.addr), &address.len);
  if (bytes_written == -1) {
          PrintError("Receive failed via perror",
                      "Receive failed via strerror: %s (Code %d)\n");
          return bytes_written;
  }
  msg.assign(buf, bytes_written);
  return bytes_written;
  }
  
  ssize_t sil::recvMsg(sil::Socket socket, std::string &msg, int flags){
  char buf[MAX_BUF_SIZE];  
  ssize_t bytes_written = 0;
  bytes_written = ::recvfrom(socket, buf, MAX_BUF_SIZE, flags, 0, 0);

  if (bytes_written == -1) {
          PrintError("Receive failed via perror",
                      "Receive failed via strerror: %s (Code %d)\n");
          return bytes_written;
  }
  msg.assign(buf, bytes_written);
  return bytes_written;
  }


void sil::close(sil::Socket socket){
  if (::close(socket) == -1) {
          PrintError("Socket closure failed via perror",
                      "Socket closure failed via strerror: %s (Code %d)\n");
  }
}

void sil::init(){
  
}

void sil::shutdown(){
  
}

#endif
