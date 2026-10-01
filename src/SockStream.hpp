#pragma once

#include "utils.hpp"

#include <cstddef>
#include <string>

namespace sil {

//NOTE: same shape as SockDatagram. A stream is connection oriented, so it has
//      listen and accept and trades recvFrom for recv, and every name here is an
//      overload of the datagram one rather than a second name for the same thing
struct SockStream
{
  Handle nativeHandle = INVALID_SOCKET_HANDLE;
  EFamily family = EFamily::UNSPECIFIED;
};

Error socket(SockStream& socket, EFamily family);

Error connect(const SockStream& socket, const SockAddr& address);
Error bind(const SockStream& socket, const SockAddr& address);
//NOTE: backlog of 0 asks the os for its own minimum, anything below that is refused
Error listen(const SockStream& socket, int backlog);
//NOTE: accepted comes back valid and is the caller's to close. address is an out
//      parameter that needs no preparation, exactly as in recvFrom
Error accept(const SockStream& socket, SockStream& accepted, SockAddr& address);

Error close(SockStream& socket);

//TODO: a stream is a byte pipe, so a read or a write can come back short and the
//      caller has to loop. Neither of these hides that, and neither pretends to
Error send(const SockStream& socket, const void* buf, size_t nbytes, int flags, int& bytesSent);
Error recv(const SockStream& socket, void* buf, size_t nbytes, int flags, int& bytesRecvd);

Error getSockName(const SockStream& socket, SockAddr& address);
//NOTE: reports NOT_CONNECTED until accept() or connect() has paired the socket up
Error getPeerName(const SockStream& socket, SockAddr& address);

//NOTE: EOption is shared by every socket module and lives in utils.hpp
Error setOption(const SockStream& socket, EOption op, bool value);
Error getOption(const SockStream& socket, EOption op, bool& value);

Error setSendBufferSize(const SockStream& socket, size_t bytes);
Error setReceiveBufferSize(const SockStream& socket, size_t bytes);

//NOTE: milliseconds, 0 disables the timeout
Error setSendTimeout(const SockStream& socket, int milliseconds);
Error setReceiveTimeout(const SockStream& socket, int milliseconds);

//NOTE: the three ways to close one half of a socket, named rather than taking a
//      direction as a value. shut() is both directions at once
Error shutWrite(const SockStream& socket);
Error shutRead(const SockStream& socket);
Error shut(const SockStream& socket);

Error setBlocking(const SockStream& socket, bool blocking);
//NOTE: needed to ever observe EError::AGAIN, a blocking call just stalls instead
Error setNonBlocking(const SockStream& socket, bool nonBlocking);

bool        isValid(const SockStream& socket);
std::string toString(const SockStream& socket);

}