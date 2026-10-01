#include "SockDatagram.hpp"

#include "utils.hpp"

#include <climits>
#include <cstddef>
#include <cstdio>

#ifdef _WIN32

#include <afunix.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <errhandlingapi.h>

namespace sil {

namespace {

//NOTE: winsock reports failures through its own getter. GetLastError() is the win32
//      one and is regularly stale or zero here, which would report failures as success
Error fromWinsock()
{
  const int value = ::WSAGetLastError();
  return Error{MacroToEnum_Error(value), value};
}

Error invalidHandle()
{
  return Error{EError::SOCKET_INVALID, 0};
}

Error invalidAddress()
{
  return Error{EError::ADDRESS_NOT_AVAILABLE, 0};
}

Error invalidParameter()
{
  return Error{EError::PARAMETER_INVALID, 0};
}

Error getAddressOfSocket(const SockDatagram& socket, SockAddr& address, bool peer)
{
  //NOTE: clear() zeroes the storage and sets len() to the full capacity, which is
  //      exactly the in value these calls expect in their namelen out parameter
  address.clear();

  int addressLength = address.len();
  sockaddr* raw = reinterpret_cast<sockaddr*>(address.bytes().data());

  const int result = peer
                         ? ::getpeername(socket.nativeHandle, raw, &addressLength)
                         : ::getsockname(socket.nativeHandle, raw, &addressLength);

  if (result == SOCKET_ERROR)
  {
    address.clear();
    return fromWinsock();
  }

  address.len(addressLength);
  return Error{};
}

} // namespace

bool isValid(const SockDatagram& socket)
{
  return socket.nativeHandle != INVALID_SOCKET_HANDLE;
}

Error socket(SockDatagram& socket, EFamily family)
{
  const int addressFamily = EnumToMacro_Family(family);
  if (addressFamily < 0)
    return Error{EError::ADDRESS_FAMILY_UNSUPPORTED, 0};

  const Handle handle = ::socket(addressFamily, SOCK_DGRAM, 0);
  if (handle == INVALID_SOCKET_HANDLE)
  {
    //NOTE: leave the handle invalid on failure so a later close() cannot hit an
    //      unrelated socket that happened to reuse the number
    socket.nativeHandle = INVALID_SOCKET_HANDLE;
    socket.family = EFamily::UNSPECIFIED;
    return fromWinsock();
  }

  socket.nativeHandle = handle;
  socket.family = family;
  return Error{};
}

Error connect(const SockDatagram& socket, const SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  if (!address.valid())
    return invalidAddress();

  if (::connect(socket.nativeHandle,
                reinterpret_cast<const sockaddr*>(address.bytes().data()),
                address.len()) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error bind(const SockDatagram& socket, const SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  if (!address.valid())
    return invalidAddress();

  if (::bind(socket.nativeHandle,
             reinterpret_cast<const sockaddr*>(address.bytes().data()),
             address.len()) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error close(SockDatagram& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::closesocket(socket.nativeHandle) == SOCKET_ERROR)
    return fromWinsock();

  //NOTE: invalidating here is what makes a double close report SOCKET_INVALID
  //      instead of closing whatever recycled the handle number
  socket.nativeHandle = INVALID_SOCKET_HANDLE;
  socket.family = EFamily::UNSPECIFIED;
  return Error{};
}

Error sendTo(SockDatagram& socket,
             const void* buf,
             size_t nbytes,
             const SockAddr& address,
             int flags,
             int& bytesSent)
{
  bytesSent = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  if (!address.valid())
    return invalidAddress();

  //NOTE: winsock takes an int, an unchecked conversion would send the wrong amount
  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  const int sent = ::sendto(socket.nativeHandle,
                            static_cast<const char*>(buf),
                            static_cast<int>(nbytes),
                            flags,
                            reinterpret_cast<const sockaddr*>(address.bytes().data()),
                            address.len());
  if (sent == SOCKET_ERROR)
    return fromWinsock();

  bytesSent = sent;
  return Error{};
}

Error recvFrom(SockDatagram& socket, void* buf, size_t nbytes, SockAddr& address, int flags, int& bytesRecvd){
  bytesRecvd = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  address.clear();

  int peerLength = address.len();
  const int recvd = ::recvfrom(socket.nativeHandle,
                               static_cast<char*>(buf),
                               static_cast<int>(nbytes),
                               flags,
                               reinterpret_cast<sockaddr*>(address.bytes().data()),
                               &peerLength);
  if (recvd == SOCKET_ERROR)
  {
    address.clear();
    return fromWinsock();
  }

  bytesRecvd = recvd;

  //NOTE: recvfrom writes the real peer length back through peerLength, and a non
  //      zero length is what marks the address valid again
  address.len(peerLength);
  return Error{};
}

Error getSockName(const SockDatagram& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, false);
}

Error getPeerName(const SockDatagram& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, true);
}

Error setOption(const SockDatagram& socket, EOption op, bool value)
{
  if (!isValid(socket))
    return invalidHandle();

  return setOption(socket.nativeHandle, op, value);
}

Error getOption(const SockDatagram& socket, EOption op, bool& value)
{
  value = false;

  if (!isValid(socket))
    return invalidHandle();

  return getOption(socket.nativeHandle, op, value);
}

Error setSendBufferSize(const SockDatagram& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::SEND, bytes);
}

Error setReceiveBufferSize(const SockDatagram& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::RECEIVE, bytes);
}

Error setSendTimeout(const SockDatagram& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::SEND, milliseconds);
}

Error setReceiveTimeout(const SockDatagram& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::RECEIVE, milliseconds);
}

Error setBlocking(const SockDatagram& socket, bool blocking)
{
  if (!isValid(socket))
    return invalidHandle();

  u_long mode = blocking ? 0 : 1;
  if (::ioctlsocket(socket.nativeHandle, FIONBIO, &mode) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error setNonBlocking(const SockDatagram& socket, bool nonBlocking)
{
  return setBlocking(socket, !nonBlocking);
}

std::string toString(const SockDatagram& socket)
{
  if (!isValid(socket))
    return "<invalid>";

  //NOTE: getsockname fails with WSAEINVAL until the socket is bound, so an unbound
  //      socket is a normal thing to be asked about and gets its own answer
  SockAddr local;
  if (!getSockName(socket, local).ok())
    return "<unbound>";

  return local.toString();
}

}

#else

#include <cerrno>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <climits>
#include <cstddef>
#include <cstdio>

namespace sil {

namespace {

//NOTE: posix reports failures through errno instead of a getter, and it is only
//      valid until the next call, so the value is captured on the spot
Error fromErrno()
{
  const int value = errno;
  return Error{MacroToEnum_Error(value), value};
}

Error invalidHandle()
{
  return Error{EError::SOCKET_INVALID, 0};
}

Error invalidAddress()
{
  return Error{EError::ADDRESS_NOT_AVAILABLE, 0};
}

Error invalidParameter()
{
  return Error{EError::PARAMETER_INVALID, 0};
}

//NOTE: a posix socket is an int descriptor while Handle is a signed pointer
//      sized integer, so every call into the os goes through here
int fd(const SockDatagram& socket)
{
  return static_cast<int>(socket.nativeHandle);
}

Error getAddressOfSocket(const SockDatagram& socket, SockAddr& address, bool peer)
{
  //NOTE: clear() zeroes the storage and sets len() to the full capacity, which is
  //      exactly the in value these calls expect in their namelen out parameter
  address.clear();

  //NOTE: namelen is a socklen_t on posix, which is unsigned, so the int the
  //      address carries has to be widened and the result narrowed back
  socklen_t addressLength = static_cast<socklen_t>(address.len());
  sockaddr* raw = reinterpret_cast<sockaddr*>(address.bytes().data());

  const int result = peer
                         ? ::getpeername(fd(socket), raw, &addressLength)
                         : ::getsockname(fd(socket), raw, &addressLength);

  if (result == -1)
  {
    address.clear();
    return fromErrno();
  }

  address.len(static_cast<int>(addressLength));
  return Error{};
}

} // namespace

bool isValid(const SockDatagram& socket)
{
  return socket.nativeHandle != INVALID_SOCKET_HANDLE;
}

Error socket(SockDatagram& socket, EFamily family)
{
  const int addressFamily = EnumToMacro_Family(family);
  if (addressFamily < 0)
    return Error{EError::ADDRESS_FAMILY_UNSUPPORTED, 0};

  const int handle = ::socket(addressFamily, SOCK_DGRAM, 0);
  if (handle == -1)
  {
    //NOTE: leave the handle invalid on failure so a later close() cannot hit an
    //      unrelated socket that happened to reuse the number
    socket.nativeHandle = INVALID_SOCKET_HANDLE;
    socket.family = EFamily::UNSPECIFIED;
    return fromErrno();
  }

  socket.nativeHandle = static_cast<Handle>(handle);
  socket.family = family;
  return Error{};
}

Error connect(const SockDatagram& socket, const SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  if (!address.valid())
    return invalidAddress();

  if (::connect(fd(socket),
                reinterpret_cast<const sockaddr*>(address.bytes().data()),
                static_cast<socklen_t>(address.len())) == -1)
    return fromErrno();

  return Error{};
}

Error bind(const SockDatagram& socket, const SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  if (!address.valid())
    return invalidAddress();

  if (::bind(fd(socket),
             reinterpret_cast<const sockaddr*>(address.bytes().data()),
             static_cast<socklen_t>(address.len())) == -1)
    return fromErrno();

  return Error{};
}

Error close(SockDatagram& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  //NOTE: unlike winsock, close() on posix releases the descriptor even when it
  //      reports a failure such as EINTR, so the handle is invalidated either
  //      way. Leaving it live would let a later call hit a recycled descriptor
  const int result = ::close(fd(socket));

  socket.nativeHandle = INVALID_SOCKET_HANDLE;
  socket.family = EFamily::UNSPECIFIED;

  if (result == -1)
    return fromErrno();

  return Error{};
}

Error sendTo(SockDatagram& socket,
             const void* buf,
             size_t nbytes,
             const SockAddr& address,
             int flags,
             int& bytesSent)
{
  bytesSent = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  if (!address.valid())
    return invalidAddress();

  //NOTE: posix takes a size_t, but bytesSent is an int, so an amount that cannot
  //      be reported back is refused instead of being silently narrowed
  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  const ssize_t sent = ::sendto(fd(socket),
                                buf,
                                nbytes,
                                flags,
                                reinterpret_cast<const sockaddr*>(address.bytes().data()),
                                static_cast<socklen_t>(address.len()));
  if (sent == -1)
    return fromErrno();

  bytesSent = static_cast<int>(sent);
  return Error{};
}

Error recvFrom(SockDatagram& socket, void* buf, size_t nbytes, SockAddr& address, int flags, int& bytesRecvd){
  bytesRecvd = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  address.clear();

  socklen_t peerLength = static_cast<socklen_t>(address.len());
  const ssize_t recvd = ::recvfrom(fd(socket),
                                   buf,
                                   nbytes,
                                   flags,
                                   reinterpret_cast<sockaddr*>(address.bytes().data()),
                                   &peerLength);
  if (recvd == -1)
  {
    address.clear();
    return fromErrno();
  }

  bytesRecvd = static_cast<int>(recvd);

  //NOTE: recvfrom writes the real peer length back through peerLength, and a non
  //      zero length is what marks the address valid again
  address.len(static_cast<int>(peerLength));
  return Error{};
}

Error getSockName(const SockDatagram& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, false);
}

Error getPeerName(const SockDatagram& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, true);
}

Error setOption(const SockDatagram& socket, EOption op, bool value)
{
  if (!isValid(socket))
    return invalidHandle();

  return setOption(socket.nativeHandle, op, value);
}

Error getOption(const SockDatagram& socket, EOption op, bool& value)
{
  value = false;

  if (!isValid(socket))
    return invalidHandle();

  return getOption(socket.nativeHandle, op, value);
}

Error setSendBufferSize(const SockDatagram& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::SEND, bytes);
}

Error setReceiveBufferSize(const SockDatagram& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::RECEIVE, bytes);
}

Error setSendTimeout(const SockDatagram& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::SEND, milliseconds);
}

Error setReceiveTimeout(const SockDatagram& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::RECEIVE, milliseconds);
}

Error setBlocking(const SockDatagram& socket, bool blocking)
{
  if (!isValid(socket))
    return invalidHandle();

  int mode = blocking ? 0 : 1;
  if (::ioctl(fd(socket), FIONBIO, &mode) == -1)
    return fromErrno();

  return Error{};
}

Error setNonBlocking(const SockDatagram& socket, bool nonBlocking)
{
  return setBlocking(socket, !nonBlocking);
}

std::string toString(const SockDatagram& socket)
{
  if (!isValid(socket))
    return "<invalid>";

  //NOTE: getsockname fails with WSAEINVAL until the socket is bound on windows,
  //      while posix answers straight away with 0.0.0.0:0, so a zero port is
  //      what marks an unbound socket on both platforms. AF_UNIX addresses have
  //      no port at all, hence the family check
  SockAddr local;
  if (!getSockName(socket, local).ok())
    return "<unbound>";

  if ((local.family() == EFamily::IPV4 || local.family() == EFamily::IPV6) && local.port() == 0)
    return "<unbound>";

  return local.toString();
}

}

#endif
