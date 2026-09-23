#include "sil.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>

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
      
    case sil::EType::RAW:
        return SOCK_RAW;
    
      default:
      return -1;
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
  
  sil::Socket sil::socket(sil::socket_definition def){
    
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

  sil::Socket sil::connect(const std::string *host,const std::string *service, sil::socket_definition def){
//NOTE:sun_path holds 108 characters, and the last one should be a null terminator, add a check for that
    int fd = -1;
  if(EnumToMacro_Family(def.addr_family) == AF_LOCAL){
    
    fd = ::socket(EnumToMacro_Family(def.addr_family),EnumToMacro_Type(def.socket_type), 0);
      if(fd == -1){
        PrintError("Socket creation failed via perror",
                    "Socket creation failed via strerror: %s (Code: %d\n)");
        ::close(fd);        
        return INVALID_SOCKET_HANDLE;
      }

      struct sockaddr_un addr;
      size_t max_bytes = sizeof(addr.sun_path) - 1;
      ::memset(&addr, 0, max_bytes + 1);
      addr.sun_family = AF_LOCAL;
      
      if (host->length() > max_bytes) {
        printf("String length is greater that 108 bytes\n");
        return INVALID_SOCKET_HANDLE;
      }
      
      host->copy(addr.sun_path, host->length());
      
    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
      
        PrintError("Error opening file failed via perror",
                    "Error opening file failed via strerror: %s (Code: %d\n)");
        ::close(fd);        
        return INVALID_SOCKET_HANDLE;
  }
    return fd;
  }
  
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_family = EnumToMacro_Family(def.addr_family);
  hints.ai_socktype = EnumToMacro_Type(def.socket_type);


  int status = getaddrinfo(host->c_str(), service->c_str(), &hints, &result);
  if (status != 0) {
    
    fprintf(stderr, "DNS Error in connect: %s\n", gai_strerror(status));
    return INVALID_SOCKET_HANDLE;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd ==  -1) {
      continue;/* On error try next address*/
    }

    if(::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0){
        break;
    }
      ::close(fd); 
  }

  ::freeaddrinfo(result);
  if (fd  == -1) {
    printf("Couldn't connect to any socket\n");
    return INVALID_SOCKET_HANDLE;
  }
  
  return fd; 
}

void sil::listen(Socket socket, int backlog){
  
  if(::listen(socket, backlog) == -1){
    PrintError("Error setting passive socket via perror",
                 "Error setting passive socket via strerror: %s (Code: %d)\n");
  }
}

void sil::bind(Socket socket, socket_definition def,const std::string *service){
  
  

  if (EnumToMacro_Family(def.addr_family) == AF_LOCAL) {
    struct sockaddr_un addr;
    ::memset(&addr, 0, sizeof(addr));
    addr.sun_family = EnumToMacro_Family(def.addr_family);
    

    if (::bind(socket,reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1){
      
          PrintError("Error binding socket via perror",
                       "Error binding socket via strerror: %s (Code: %d)\n");
          }
          
  }else{
    
    struct addrinfo hints;
    
    ::memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = EnumToMacro_Type(def.socket_type);
    hints.ai_family = EnumToMacro_Family(def.addr_family);

    struct addrinfo *result;
    int status = getaddrinfo(NULL, service->c_str(), &hints, &result);
    if (status != 0) {
      fprintf(stderr, "DNS Error in binding: %s\n", gai_strerror(status));
      return;
    }
    
    struct addrinfo *rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      
    if (::bind(socket, rp->ai_addr, rp->ai_addrlen) == -1) {
        PrintError("Binding failed via perror",
                    "Binding failed via strerror: %s (Code %d)\n"); 
        ::freeaddrinfo(result);
        break;
    }
    
    }
  }
}


sil::Socket sil::accept(sil::Socket socket, sil::socket_definition def ,const std::string *host,const std::string *service){
  Socket socket_result = -1;

  if (EnumToMacro_Family(def.addr_family) == AF_LOCAL) {
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr.sun_path));
    
    addr.sun_family = EnumToMacro_Family(def.addr_family);

    size_t max_bytes = sizeof(addr.sun_path) - 1;
    ::memset(&addr, 0, max_bytes + 1);
    addr.sun_family = AF_LOCAL;
    
    if (host->length() > max_bytes) {
      printf("String length is greater that 108 bytes\n");
      return INVALID_SOCKET_HANDLE;
    }

    socket_result = ::accept(socket, reinterpret_cast<sockaddr*>(&addr), (socklen_t *)(host->length()));

    if (socket_result == -1) {
        PrintError("Accept failed via perror",
                    "Accept failed via strerror: %s (Code %d)\n");
        socket_result = INVALID_SOCKET_HANDLE; 
    }
    
  }else{
  struct addrinfo hints;

  hints.ai_canonname = NULL;
  hints.ai_flags = 0;
  hints.ai_next = NULL;
  hints.ai_protocol = 0;
  hints.ai_family = EnumToMacro_Family(def.addr_family);
  hints.ai_socktype = EnumToMacro_Type(def.socket_type);

  struct addrinfo *result;
  
  int status = getaddrinfo(host->c_str(), service->c_str(), &hints, &result);

    if (status != 0) {
      fprintf(stderr, "DNS Error in accept: %s\n", gai_strerror(status));
      return -1;
    }
    
    struct addrinfo *rp;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      socket_result  = ::accept(socket, rp->ai_addr, &rp->ai_addrlen);
    if (socket_result == -1) {
        PrintError("Accept failed via perror",
                    "Accept failed via strerror: %s (Code %d)\n"); 
        ::freeaddrinfo(result);
        socket_result = INVALID_SOCKET_HANDLE;
        break;
    }
    
    }
    
    
  }
  
    return socket_result;
}

ssize_t sil::sendTo (sil::Socket socket, const void *buf, size_t nbytes, int flags){
  ssize_t bytes_received = 0;
  struct sockaddr_storage addr {};
  socklen_t len = sizeof(addr);
  ::getpeername(socket, reinterpret_cast<sockaddr*>(&addr), (socklen_t *)&len);
  
  bytes_received = ::sendto(socket, buf, nbytes, flags, reinterpret_cast<sockaddr*>(&addr), len);
  if (bytes_received == -1) {
          PrintError("Send failed via perror",
                      "Send failed via strerror: %s (Code %d) \n");
  }
  return  bytes_received;
}


ssize_t sil::recvFrom(sil::Socket socket, void *buf, size_t nbytes, int flags){
  ssize_t bytes_written = 0;
  struct sockaddr_storage addr {};
  socklen_t len = sizeof(addr);
  
  bytes_written = ::recvfrom(socket, buf, nbytes, flags, reinterpret_cast<sockaddr*>(&addr), &len);
  if (bytes_written == -1) {
          PrintError("Receive failed via perror",
                      "Receive failed via strerror: %s (Code %d)\n");
  }

  return bytes_written;
}


ssize_t sil::send (sil::Socket socket, const void *buf, size_t nbytes, int flags){
  ssize_t bytes_received = 0;
   
  bytes_received = ::send(socket, buf, nbytes, flags);
  if (bytes_received == -1) {
          PrintError("Send failed via perror",
                      "Send failed via strerror: %s (Code %d)\n");
  }
  return  bytes_received;
}


ssize_t sil::recv(sil::Socket socket, void *buf, size_t nbytes, int flags){
  ssize_t bytes_written = 0;
  
  bytes_written = ::recvfrom(socket, buf, nbytes, flags, 0, 0);
  if (bytes_written == -1) {
          PrintError("Receive failed via perror",
                      "Receive failed via strerror: %s (Code %d)\n");
  }

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
