#pragma once

#include "utils.hpp"

#include <cstddef>
#include <string>

namespace sil {

//NOTE: datagram only subset. Windows has no SO_REUSEPORT, and SO_KEEPALIVE is
//      rejected with WSAENOPROTOOPT on a datagram socket, so neither is offered
enum class EOption
{
  REUSE_ADDRESS,
  BROADCAST
};

struct SockDatagram
{
  Handle nativeHandle = INVALID_SOCKET_HANDLE;
  EFamily family = EFamily::UNSPECIFIED;
};

Error socket(SockDatagram& socket, EFamily family);

Error connect(const SockDatagram& socket, const SockAddr& address);
Error bind(const SockDatagram& socket, const SockAddr& address);
Error close(SockDatagram& socket);

//NOTE: flags is handed to the os untouched, which works as long as the caller names
//      a MSG_* macro its platform defines. It is an int rather than a portable
//      enum because there is no agreed subset worth cutting it down to, and the
//      flags that matter here, MSG_PEEK and MSG_TRUNC, do not agree across the
//      two platforms either
Error sendTo(SockDatagram& socket, const void* buf, size_t nbytes, const SockAddr& address,
             int flags, int& bytesSent);
//TODO: I suspect using a buffer this way might be annoying really fast, some custom buffer struct
// with proper bound checking sounds desirable

//NOTE: address is an out parameter, it needs no preparation
Error recvFrom(SockDatagram& socket, void* buf, size_t nbytes, SockAddr& address,
               int flags, int& bytesRecvd);

Error getSockName(const SockDatagram& socket, SockAddr& address);
//NOTE: a datagram socket is not connected, so this reports NOT_CONNECTED until
//      connect() has been called
Error getPeerName(const SockDatagram& socket, SockAddr& address);

Error setOption(const SockDatagram& socket, EOption op, bool value);
Error getOption(const SockDatagram& socket, EOption op, bool& value);

Error setSendBufferSize(const SockDatagram& socket, size_t bytes);
Error setReceiveBufferSize(const SockDatagram& socket, size_t bytes);

//NOTE: milliseconds, 0 disables the timeout
Error setSendTimeout(const SockDatagram& socket, int milliseconds);
Error setReceiveTimeout(const SockDatagram& socket, int milliseconds);

Error setBlocking(const SockDatagram& socket, bool blocking);
//NOTE: needed to ever observe EError::AGAIN, a blocking socket just stalls instead
Error setNonBlocking(const SockDatagram& socket, bool nonBlocking);

bool        isValid(const SockDatagram& socket);
std::string toString(const SockDatagram& socket);

}
