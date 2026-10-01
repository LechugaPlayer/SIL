#include "SockStream.hpp"

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

Error getAddressOfSocket(const SockStream& socket, SockAddr& address, bool peer)
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

bool isValid(const SockStream& socket)
{
  return socket.nativeHandle != INVALID_SOCKET_HANDLE;
}

Error socket(SockStream& socket, EFamily family)
{
  const int addressFamily = EnumToMacro_Family(family);
  if (addressFamily < 0)
    return Error{EError::ADDRESS_FAMILY_UNSUPPORTED, 0};

  const Handle handle = ::socket(addressFamily, SOCK_STREAM, 0);
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

Error connect(const SockStream& socket, const SockAddr& address)
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

Error bind(const SockStream& socket, const SockAddr& address)
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

Error listen(const SockStream& socket, int backlog)
{
  if (!isValid(socket))
    return invalidHandle();

  if (backlog < 0)
    return invalidParameter();

  //NOTE: winsock takes the backlog as advice and clamps it itself, so there is
  //      nothing to read back afterwards
  if (::listen(socket.nativeHandle, backlog) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error accept(const SockStream& socket, SockStream& accepted, SockAddr& address)
{
  //NOTE: invalidated up front so a rejected accept cannot leave the caller with
  //      a half built socket, the same rule socket() follows
  accepted.nativeHandle = INVALID_SOCKET_HANDLE;
  accepted.family = EFamily::UNSPECIFIED;

  if (!isValid(socket))
    return invalidHandle();

  address.clear();

  int peerLength = address.len();
  const Handle handle = ::accept(socket.nativeHandle,
                                 reinterpret_cast<sockaddr*>(address.bytes().data()),
                                 &peerLength);
  if (handle == INVALID_SOCKET_HANDLE)
  {
    address.clear();
    return fromWinsock();
  }

  accepted.nativeHandle = handle;
  //NOTE: the accepted socket inherits the family of the listener, there is no
  //      second address to read it from
  accepted.family = socket.family;

  address.len(peerLength);
  return Error{};
}

Error close(SockStream& socket)
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

Error send(const SockStream& socket, const void* buf, size_t nbytes, int flags, int& bytesSent)
{
  bytesSent = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  //NOTE: winsock takes an int, an unchecked conversion would send the wrong amount
  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  const int sent = ::send(socket.nativeHandle, static_cast<const char*>(buf), static_cast<int>(nbytes), flags);
  if (sent == SOCKET_ERROR)
    return fromWinsock();

  bytesSent = sent;
  return Error{};
}

Error recv(const SockStream& socket, void* buf, size_t nbytes, int flags, int& bytesRecvd)
{
  bytesRecvd = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  //NOTE: 0 bytes with no error is the end of the stream, the peer closed its half
  const int recvd = ::recv(socket.nativeHandle, static_cast<char*>(buf), static_cast<int>(nbytes), flags);
  if (recvd == SOCKET_ERROR)
    return fromWinsock();

  bytesRecvd = recvd;
  return Error{};
}

Error getSockName(const SockStream& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, false);
}

Error getPeerName(const SockStream& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, true);
}

Error setOption(const SockStream& socket, EOption op, bool value)
{
  if (!isValid(socket))
    return invalidHandle();

  return setOption(socket.nativeHandle, op, value);
}

Error getOption(const SockStream& socket, EOption op, bool& value)
{
  value = false;

  if (!isValid(socket))
    return invalidHandle();

  return getOption(socket.nativeHandle, op, value);
}

Error setSendBufferSize(const SockStream& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::SEND, bytes);
}

Error setReceiveBufferSize(const SockStream& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::RECEIVE, bytes);
}

Error setSendTimeout(const SockStream& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::SEND, milliseconds);
}

Error setReceiveTimeout(const SockStream& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::RECEIVE, milliseconds);
}

Error shutWrite(const SockStream& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::shutdown(socket.nativeHandle, SD_SEND) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error shutRead(const SockStream& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::shutdown(socket.nativeHandle, SD_RECEIVE) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error shut(const SockStream& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::shutdown(socket.nativeHandle, SD_BOTH) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error setBlocking(const SockStream& socket, bool blocking)
{
  if (!isValid(socket))
    return invalidHandle();

  u_long mode = blocking ? 0 : 1;
  if (::ioctlsocket(socket.nativeHandle, FIONBIO, &mode) == SOCKET_ERROR)
    return fromWinsock();

  return Error{};
}

Error setNonBlocking(const SockStream& socket, bool nonBlocking)
{
  return setBlocking(socket, !nonBlocking);
}

std::string toString(const SockStream& socket)
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
int fd(const SockStream& socket)
{
  return static_cast<int>(socket.nativeHandle);
}

Error getAddressOfSocket(const SockStream& socket, SockAddr& address, bool peer)
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

bool isValid(const SockStream& socket)
{
  return socket.nativeHandle != INVALID_SOCKET_HANDLE;
}

Error socket(SockStream& socket, EFamily family)
{
  const int addressFamily = EnumToMacro_Family(family);
  if (addressFamily < 0)
    return Error{EError::ADDRESS_FAMILY_UNSUPPORTED, 0};

  const int handle = ::socket(addressFamily, SOCK_STREAM, 0);
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

Error connect(const SockStream& socket, const SockAddr& address)
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

Error bind(const SockStream& socket, const SockAddr& address)
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

Error listen(const SockStream& socket, int backlog)
{
  if (!isValid(socket))
    return invalidHandle();

  if (backlog < 0)
    return invalidParameter();

  if (::listen(fd(socket), backlog) == -1)
    return fromErrno();

  return Error{};
}

Error accept(const SockStream& socket, SockStream& accepted, SockAddr& address)
{
  //NOTE: invalidated up front so a rejected accept cannot leave the caller with
  //      a half built socket, the same rule socket() follows
  accepted.nativeHandle = INVALID_SOCKET_HANDLE;
  accepted.family = EFamily::UNSPECIFIED;

  if (!isValid(socket))
    return invalidHandle();

  address.clear();

  socklen_t peerLength = static_cast<socklen_t>(address.len());
  const int handle = ::accept(fd(socket),
                              reinterpret_cast<sockaddr*>(address.bytes().data()),
                              &peerLength);
  if (handle == -1)
  {
    address.clear();
    return fromErrno();
  }

  accepted.nativeHandle = static_cast<Handle>(handle);
  //NOTE: the accepted socket inherits the family of the listener, there is no
  //      second address to read it from
  accepted.family = socket.family;

  address.len(static_cast<int>(peerLength));
  return Error{};
}

Error close(SockStream& socket)
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

Error send(const SockStream& socket, const void* buf, size_t nbytes, int flags, int& bytesSent)
{
  bytesSent = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  //NOTE: posix takes a size_t, but bytesSent is an int, so an amount that cannot
  //      be reported back is refused instead of being silently narrowed
  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  //NOTE: MSG_NOSIGNAL is ored in because a write to a peer that already closed
  //      raises SIGPIPE by default, which would kill the process instead of
  //      returning an error. The caller's own flags are left untouched, and
  //      windows needs no equivalent because WSAECONNRESET is already an error
  const ssize_t sent = ::send(fd(socket), buf, nbytes, flags | MSG_NOSIGNAL);
  if (sent == -1)
    return fromErrno();

  bytesSent = static_cast<int>(sent);
  return Error{};
}

Error recv(const SockStream& socket, void* buf, size_t nbytes, int flags, int& bytesRecvd)
{
  bytesRecvd = 0;

  if (!isValid(socket))
    return invalidHandle();

  if (buf == nullptr && nbytes != 0)
    return invalidParameter();

  if (nbytes > static_cast<size_t>(INT_MAX))
    return Error{EError::MESSAGE_TOO_LARGE, 0};

  //NOTE: 0 bytes with no error is the end of the stream, the peer closed its half
  const ssize_t recvd = ::recv(fd(socket), buf, nbytes, flags);
  if (recvd == -1)
    return fromErrno();

  bytesRecvd = static_cast<int>(recvd);
  return Error{};
}

Error getSockName(const SockStream& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, false);
}

Error getPeerName(const SockStream& socket, SockAddr& address)
{
  if (!isValid(socket))
    return invalidHandle();

  return getAddressOfSocket(socket, address, true);
}

Error setOption(const SockStream& socket, EOption op, bool value)
{
  if (!isValid(socket))
    return invalidHandle();

  return setOption(socket.nativeHandle, op, value);
}

Error getOption(const SockStream& socket, EOption op, bool& value)
{
  value = false;

  if (!isValid(socket))
    return invalidHandle();

  return getOption(socket.nativeHandle, op, value);
}

Error setSendBufferSize(const SockStream& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::SEND, bytes);
}

Error setReceiveBufferSize(const SockStream& socket, size_t bytes)
{
  if (!isValid(socket))
    return invalidHandle();

  return setBufferSize(socket.nativeHandle, EBufferSize::RECEIVE, bytes);
}

Error setSendTimeout(const SockStream& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::SEND, milliseconds);
}

Error setReceiveTimeout(const SockStream& socket, int milliseconds)
{
  if (!isValid(socket))
    return invalidHandle();

  return setTimeout(socket.nativeHandle, ETimeout::RECEIVE, milliseconds);
}

Error shutWrite(const SockStream& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::shutdown(fd(socket), SHUT_WR) == -1)
    return fromErrno();

  return Error{};
}

Error shutRead(const SockStream& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::shutdown(fd(socket), SHUT_RD) == -1)
    return fromErrno();

  return Error{};
}

Error shut(const SockStream& socket)
{
  if (!isValid(socket))
    return invalidHandle();

  if (::shutdown(fd(socket), SHUT_RDWR) == -1)
    return fromErrno();

  return Error{};
}

Error setBlocking(const SockStream& socket, bool blocking)
{
  if (!isValid(socket))
    return invalidHandle();

  int mode = blocking ? 0 : 1;
  if (::ioctl(fd(socket), FIONBIO, &mode) == -1)
    return fromErrno();

  return Error{};
}

Error setNonBlocking(const SockStream& socket, bool nonBlocking)
{
  return setBlocking(socket, !nonBlocking);
}

std::string toString(const SockStream& socket)
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