#include "utils.hpp"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32

#include <afunix.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <errhandlingapi.h>

namespace sil {

namespace {

Error parameterInvalid()
{
  return Error{EError::PARAMETER_INVALID, 0};
}

} // namespace

Error init()
{
  //NOTE: WSAStartup reports its failure through its return value, it never
  //      touches the thread error slot
  WSADATA wsaData{};
  const int value = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (value != 0)
    return Error{MacroToEnum_Error(value), value};

  return Error{};
}

Error cleanup()
{
  const int value = ::WSACleanup();
  if (value != 0)
    return Error{MacroToEnum_Error(value), value};

  return Error{};
}

int EnumToMacro_Family(EFamily var)
{
  switch (var)
  {
    case EFamily::LOCAL:
      return AF_UNIX;
    case EFamily::IPV4:
      return AF_INET;
    case EFamily::IPV6:
      return AF_INET6;
    case EFamily::UNSPECIFIED:
      return AF_UNSPEC;
    default:
      return -1;
  }
}

int EnumToMacro_Type(EType var)
{
  switch (var)
  {
    case EType::DATAGRAM:
      return SOCK_DGRAM;
    case EType::STREAM:
      return SOCK_STREAM;
    default:
      return -1;
  }
}

EError MacroToEnum_Error(int error)
{
  switch (error)
  {
    case 0:
      return EError::OK;

    case WSAEWOULDBLOCK:
      return EError::AGAIN;

    case WSAECONNRESET:
      return EError::CONNECTION_RESET;

    case WSAECONNREFUSED:
      return EError::CONNECTION_REFUSED;

    case WSAECONNABORTED:
      return EError::CONNECTION_ABORTED;

    case WSAENETDOWN:
      return EError::CONNECTION_DOWN;

    case WSAENETRESET:
      return EError::CONNECTION_RESET;

    case WSAENOBUFS:
      return EError::NO_BUFFER_SPACE;
    //NOTE: on win32 this is ERROR_NOT_ENOUGH_MEMORY, not a WSA* code, so it cannot collide

    case WSA_NOT_ENOUGH_MEMORY:
      return EError::NO_MEMORY;

    case WSAETIMEDOUT:
      return EError::TIMED_OUT;

    case WSAEINTR:
      return EError::INTERRUPTED;

    case WSAENOTSOCK:
      return EError::SOCKET_INVALID;

    case WSAENOTCONN:
      return EError::NOT_CONNECTED;

    case WSAEINVAL:
      return EError::PARAMETER_INVALID;

    case WSAEFAULT:
      return EError::PARAMETER_INVALID;

    case WSAEPROTONOSUPPORT:
      return EError::PROTOCOL_INVALID;

    case WSAEPROTOTYPE:
      return EError::PROTOCOL_INVALID;

    case WSAENOPROTOOPT:
      return EError::PROTOCOL_INVALID;

    case WSAEADDRINUSE:
      return EError::ADDRESS_IN_USE;

    case WSAEADDRNOTAVAIL:
      return EError::ADDRESS_NOT_AVAILABLE;

    case WSAEACCES:
      return EError::ACCESS_DENIED;

    case WSAEINPROGRESS:
      return EError::OPERATION_IN_PROGRESS;

    case WSAEALREADY:
      return EError::ALREADY_IN_PROGRESS;

    case WSANOTINITIALISED:
      return EError::NOT_INITIALIZED;

    case WSASYSNOTREADY:
      return EError::NOT_INITIALIZED;

    case WSAENETUNREACH:
      return EError::NETWORK_UNREACHABLE;

    case WSAEHOSTUNREACH:
      return EError::HOST_UNREACHABLE;

    case WSAENAMETOOLONG:
      return EError::NAME_TOO_LONG;

    case WSAEAFNOSUPPORT:
      return EError::ADDRESS_FAMILY_UNSUPPORTED;

    case WSAESOCKTNOSUPPORT:
      return EError::SOCKET_TYPE_UNSUPPORTED;

    case WSAEOPNOTSUPP:
      return EError::OPERATION_NOT_SUPPORTED;

    case WSAEISCONN:
      return EError::ALREADY_CONNECTED;

    case WSAESHUTDOWN:
      return EError::SHUTDOWN;

    case WSAEMFILE:
      return EError::TOO_MANY_SOCKETS;

    case WSAEMSGSIZE:
      return EError::MESSAGE_TOO_LARGE;

    case WSAEDESTADDRREQ:
      return EError::DESTINATION_ADDRESS_REQUIRED;

    case WSAEHOSTDOWN:
      return EError::HOST_NOT_AVAILABLE;

    case WSAHOST_NOT_FOUND:
      return EError::DNS_HOST_NOT_FOUND;

    case WSATRY_AGAIN:
      return EError::DNS_TRY_AGAIN;

    case WSANO_RECOVERY:
      return EError::DNS_NO_RECOVERY;

    case WSANO_DATA:
      return EError::DNS_NO_DATA;

    default:
      return EError::UNKNOWN;
  }
}

const char* errorToString(EError code)
{
  switch (code)
  {
    case EError::OK:
      return "success";
    case EError::AGAIN:
      return "operation would block";
    case EError::CONNECTION_RESET:
      return "connection reset";
    case EError::CONNECTION_REFUSED:
      return "connection refused";
    case EError::CONNECTION_ABORTED:
      return "connection aborted";
    case EError::CONNECTION_DOWN:
      return "connection down";
    case EError::NO_BUFFER_SPACE:
      return "no buffer space";
    case EError::TIMED_OUT:
      return "timed out";
    case EError::INTERRUPTED:
      return "interrupted";
    case EError::SOCKET_INVALID:
      return "invalid socket";
    case EError::PARAMETER_INVALID:
      return "invalid parameter";
    case EError::PROTOCOL_INVALID:
      return "invalid protocol";
    case EError::ADDRESS_IN_USE:
      return "address already in use";
    case EError::ADDRESS_NOT_AVAILABLE:
      return "address not available";
    case EError::ACCESS_DENIED:
      return "access denied";
    case EError::OPERATION_IN_PROGRESS:
      return "operation already in progress";
    case EError::NOT_INITIALIZED:
      return "winsock not initialized";
    case EError::NETWORK_UNREACHABLE:
      return "network unreachable";
    case EError::HOST_UNREACHABLE:
      return "host unreachable";
    case EError::NAME_TOO_LONG:
      return "name too long";
    case EError::ADDRESS_FAMILY_UNSUPPORTED:
      return "address family unsupported";
    case EError::SOCKET_TYPE_UNSUPPORTED:
      return "socket type unsupported";
    case EError::OPERATION_NOT_SUPPORTED:
      return "operation not supported";
    case EError::NOT_CONNECTED:
      return "not connected";
    case EError::ALREADY_CONNECTED:
      return "already connected";
    case EError::ALREADY_IN_PROGRESS:
      return "already in progress";
    case EError::SHUTDOWN:
      return "socket is shut down";
    case EError::TOO_MANY_SOCKETS:
      return "too many open sockets";
    case EError::NO_MEMORY:
      return "out of memory";
    case EError::MESSAGE_TOO_LARGE:
      return "message too large";
    case EError::DESTINATION_ADDRESS_REQUIRED:
      return "destination address required";
    case EError::HOST_NOT_AVAILABLE:
      return "host down";
    case EError::DNS_HOST_NOT_FOUND:
      return "host not found";
    case EError::DNS_TRY_AGAIN:
      return "dns lookup should be retried";
    case EError::DNS_NO_RECOVERY:
      return "dns server failure";
    case EError::DNS_NO_DATA:
      return "dns name does not exist";
    case EError::UNKNOWN:
      return "unknown error";
    default:
      return "unknown error";
  }
}

void printError(Error err)
{
  if (err.ok())
    return;

  std::fprintf(stderr,
               "sil: %s (winsock code %d)\n",
               errorToString(err.code),
               err.valueCode);
}

int SockAddr::capacity()
{
  return static_cast<int>(sizeof(sockaddr_storage));
}

Error SockAddr::resolve(SockAddr& out, const char* host, const char* service, EFamily family, EType type)
{
  out.clear();
  out.mResolved = false;

  const int addressFamily = EnumToMacro_Family(family);
  const int socketType = EnumToMacro_Type(type);
  if (addressFamily < 0 || socketType < 0)
    return parameterInvalid();

  if (family == EFamily::LOCAL)
  {
    if (socketType != SOCK_STREAM)
    {
      //NOTE: windows only implements AF_UNIX for stream sockets
      return Error{EError::ADDRESS_FAMILY_UNSUPPORTED, 0};
    }
    if (host == nullptr || *host == '\0')
      return Error{EError::ADDRESS_NOT_AVAILABLE, 0};

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    const size_t pathLength = std::strlen(host);
    if (pathLength >= sizeof(addr.sun_path))
      return Error{EError::NAME_TOO_LONG, 0};

    //NOTE: copying length + 1 keeps the terminator, the rest of sun_path stays zeroed
    std::memcpy(addr.sun_path, host, pathLength + 1);

    const int total = static_cast<int>(offsetof(sockaddr_un, sun_path) + pathLength + 1);
    std::memcpy(&out.storage, &addr, static_cast<size_t>(total));

    out.mLen = total;
    out.mResolved = true;
    return Error{};
  }

  if (service == nullptr)
    return parameterInvalid();

  addrinfo hints{};
  hints.ai_family = addressFamily;
  hints.ai_socktype = socketType;
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;
  //NOTE: a null host means "any interface", which getaddrinfo only honours when asked
  hints.ai_flags = (host == nullptr) ? AI_PASSIVE : 0;

  addrinfo* result = nullptr;
  //NOTE: getaddrinfo returns 0 on success and a winsock code on failure. It never
  //      returns -1 and it does not touch the thread error slot.
  const int error = ::getaddrinfo(host, service, &hints, &result);
  if (error != 0)
    return Error{MacroToEnum_Error(error), error};

  if (result == nullptr)
    return Error{EError::HOST_NOT_AVAILABLE, 0};

  for (const addrinfo* rp = result; rp != nullptr; rp = rp->ai_next)
  {
    if (rp->ai_addr == nullptr || rp->ai_addrlen == 0 || rp->ai_addrlen > sizeof(out.storage))
      continue;

    std::memcpy(&out.storage, rp->ai_addr, rp->ai_addrlen);
    out.mLen = static_cast<int>(rp->ai_addrlen);
    out.mResolved = true;
    break;
  }

  ::freeaddrinfo(result);

  if (!out.mResolved)
    return Error{EError::ADDRESS_NOT_AVAILABLE, 0};

  return Error{};
}

SockAddr::SockAddr(const std::string& host, const std::string& service, EFamily family, EType type)
{
  (void)resolve(*this, host.c_str(), service.c_str(), family, type);
}

SockAddr::SockAddr(const std::string& host, uint16_t port, EFamily family, EType type)
{
  const std::string portText = std::to_string(port);
  (void)resolve(*this, host.c_str(), portText.c_str(), family, type);
}

SockAddr::SockAddr(uint16_t port, EFamily family, EType type)
{
  const std::string portText = std::to_string(port);
  (void)resolve(*this, nullptr, portText.c_str(), family, type);
}

SockAddr::SockAddr(const std::string& path, EFamily family, EType type)
{
  //NOTE: an AF_UNIX address has no port, so no service is passed
  (void)resolve(*this, path.c_str(), nullptr, family, type);
}

std::span<std::byte> SockAddr::bytes()
{
  return std::span<std::byte>(reinterpret_cast<std::byte*>(&storage),
                              static_cast<size_t>(len()));
}

std::span<const std::byte> SockAddr::bytes() const
{
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(&storage),
                                    static_cast<size_t>(len()));
}

int SockAddr::len() const
{
  return mLen;
}

void SockAddr::len(int n)
{
  if (n < 0)
    n = 0;
  if (n > capacity())
    n = capacity();

  mLen = n;
  //NOTE: a non zero length means bytes were written into the storage, which is how
  //      an out call such as recvfrom publishes the address it just received
  mResolved = (n > 0);
}

void SockAddr::clear()
{
  storage = sockaddr_storage{};
  mLen = capacity();
  mResolved = false;
}

bool SockAddr::valid() const
{
  return mResolved && mLen > 0;
}

EFamily SockAddr::family() const
{
  if (!valid())
    return EFamily::UNSPECIFIED;

  switch (static_cast<int>(storage.ss_family))
  {
    case AF_INET:
      return EFamily::IPV4;
    case AF_INET6:
      return EFamily::IPV6;
    case AF_UNIX:
      return EFamily::LOCAL;
    default:
      return EFamily::UNSPECIFIED;
  }
}

uint16_t SockAddr::port() const
{
  if (!valid())
    return 0;

  switch (static_cast<int>(storage.ss_family))
  {
    case AF_INET:
      return ntohs(reinterpret_cast<const sockaddr_in*>(&storage)->sin_port);
    case AF_INET6:
      return ntohs(reinterpret_cast<const sockaddr_in6*>(&storage)->sin6_port);
    default:
      return 0;
  }
}

std::string SockAddr::toString() const
{
  if (!valid())
    return "<unresolved>";

  char host[NI_MAXHOST]{};
  char service[NI_MAXSERV]{};

  const int error = ::getnameinfo(reinterpret_cast<const sockaddr*>(&storage),
                                  static_cast<socklen_t>(len()),
                                  host,
                                  sizeof(host),
                                  service,
                                  sizeof(service),
                                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (error != 0)
    return "<invalid>";

  std::string result = host;
  if (static_cast<int>(storage.ss_family) == AF_INET6)
    result = "[" + result + "]";

  result += ':';
  result += service;
  return result;
}

bool SockAddr::operator==(const SockAddr& other) const
{
  if (mResolved != other.mResolved || mLen != other.mLen)
    return false;

  if (mLen == 0)
    return true;

  return std::memcmp(&storage, &other.storage, static_cast<size_t>(mLen)) == 0;
}

}

#else

#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

namespace sil {

namespace {

Error parameterInvalid()
{
  return Error{EError::PARAMETER_INVALID, 0};
}

//NOTE: errno belongs to the thread and only says anything right after the call
//      that failed, so the value is captured on the spot and never read again
Error fromErrno()
{
  const int value = errno;
  return Error{MacroToEnum_Error(value), value};
}

} // namespace

Error init()
{
  //NOTE: nothing to bring up on posix, the kernel owns the sockets. The call is
  //      here anyway so that the same code compiles and reads the same everywhere
  return Error{};
}

Error cleanup()
{
  return Error{};
}

int EnumToMacro_Family(EFamily var)
{
  switch (var)
  {
    case EFamily::LOCAL:
      return AF_UNIX;
    case EFamily::IPV4:
      return AF_INET;
    case EFamily::IPV6:
      return AF_INET6;
    case EFamily::UNSPECIFIED:
      return AF_UNSPEC;
    default:
      return -1;
  }
}

int EnumToMacro_Type(EType var)
{
  switch (var)
  {
    case EType::DATAGRAM:
      return SOCK_DGRAM;
    case EType::STREAM:
      return SOCK_STREAM;
    default:
      return -1;
  }
}

EError MacroToEnum_Error(int error)
{
  //NOTE: posix has two error namespaces. getaddrinfo reports through EAI_*, which
  //      is always negative, while errno is always positive, so the sign is all
  //      it takes to tell them apart
  if (error < 0)
  {
    switch (error)
    {
      case EAI_NONAME:
        return EError::DNS_HOST_NOT_FOUND;

      case EAI_NODATA:
        return EError::DNS_NO_DATA;

      case EAI_AGAIN:
        return EError::DNS_TRY_AGAIN;

      case EAI_FAIL:
        return EError::DNS_NO_RECOVERY;

      case EAI_INPROGRESS:
        return EError::OPERATION_IN_PROGRESS;

      case EAI_FAMILY:
      case EAI_ADDRFAMILY:
        return EError::ADDRESS_FAMILY_UNSUPPORTED;

      case EAI_SOCKTYPE:
        return EError::SOCKET_TYPE_UNSUPPORTED;

      case EAI_SERVICE:
      case EAI_BADFLAGS:
        return EError::PARAMETER_INVALID;

      case EAI_MEMORY:
        return EError::NO_MEMORY;

      case EAI_OVERFLOW:
        return EError::NAME_TOO_LONG;

      //NOTE: EAI_SYSTEM only says that a syscall underneath failed, resolve()
      //      reads errno for those instead of going through here
      case EAI_SYSTEM:
        return EError::UNKNOWN;

      default:
        return EError::UNKNOWN;
    }
  }

  //NOTE: EWOULDBLOCK is EAGAIN and ENOTSUP is EOPNOTSUPP on posix, so naming
  //      both of either pair would be a duplicate case label
  switch (error)
  {
    case 0:
      return EError::OK;

    case EAGAIN:
      return EError::AGAIN;

    case EINTR:
      return EError::INTERRUPTED;

    case ECONNRESET:
    case ENETRESET:
      return EError::CONNECTION_RESET;

    case ECONNREFUSED:
      return EError::CONNECTION_REFUSED;

    case ECONNABORTED:
      return EError::CONNECTION_ABORTED;

    case ENETDOWN:
      return EError::CONNECTION_DOWN;

    case ENOBUFS:
      return EError::NO_BUFFER_SPACE;

    case ENOMEM:
      return EError::NO_MEMORY;

    case ETIMEDOUT:
      return EError::TIMED_OUT;

    case EBADF:
    case ENOTSOCK:
      return EError::SOCKET_INVALID;

    case EINVAL:
    case EFAULT:
      return EError::PARAMETER_INVALID;

    case EPROTONOSUPPORT:
    case EPROTOTYPE:
    case ENOPROTOOPT:
      return EError::PROTOCOL_INVALID;

    case EADDRINUSE:
      return EError::ADDRESS_IN_USE;

    case EADDRNOTAVAIL:
      return EError::ADDRESS_NOT_AVAILABLE;

    case EPERM:
    case EACCES:
      return EError::ACCESS_DENIED;

    case EINPROGRESS:
      return EError::OPERATION_IN_PROGRESS;

    case EALREADY:
      return EError::ALREADY_IN_PROGRESS;

    case ENETUNREACH:
      return EError::NETWORK_UNREACHABLE;

    case EHOSTUNREACH:
      return EError::HOST_UNREACHABLE;

    case EHOSTDOWN:
      return EError::HOST_NOT_AVAILABLE;

    case ENAMETOOLONG:
      return EError::NAME_TOO_LONG;

    case EAFNOSUPPORT:
      return EError::ADDRESS_FAMILY_UNSUPPORTED;

    case ESOCKTNOSUPPORT:
      return EError::SOCKET_TYPE_UNSUPPORTED;

    case EOPNOTSUPP:
      return EError::OPERATION_NOT_SUPPORTED;

    case EISCONN:
      return EError::ALREADY_CONNECTED;

    case ENOTCONN:
      return EError::NOT_CONNECTED;

    case ESHUTDOWN:
      return EError::SHUTDOWN;

    case EMFILE:
    case ENFILE:
      return EError::TOO_MANY_SOCKETS;

    case EMSGSIZE:
      return EError::MESSAGE_TOO_LARGE;

    case EDESTADDRREQ:
      return EError::DESTINATION_ADDRESS_REQUIRED;

    //NOTE: EError::NOT_INITIALIZED has no posix counterpart, since init() has
    //      nothing to do and nothing can be called before it

    default:
      return EError::UNKNOWN;
  }
}

const char* errorToString(EError code)
{
  switch (code)
  {
    case EError::OK:
      return "success";
    case EError::AGAIN:
      return "operation would block";
    case EError::CONNECTION_RESET:
      return "connection reset";
    case EError::CONNECTION_REFUSED:
      return "connection refused";
    case EError::CONNECTION_ABORTED:
      return "connection aborted";
    case EError::CONNECTION_DOWN:
      return "connection down";
    case EError::NO_BUFFER_SPACE:
      return "no buffer space";
    case EError::TIMED_OUT:
      return "timed out";
    case EError::INTERRUPTED:
      return "interrupted";
    case EError::SOCKET_INVALID:
      return "invalid socket";
    case EError::PARAMETER_INVALID:
      return "invalid parameter";
    case EError::PROTOCOL_INVALID:
      return "invalid protocol";
    case EError::ADDRESS_IN_USE:
      return "address already in use";
    case EError::ADDRESS_NOT_AVAILABLE:
      return "address not available";
    case EError::ACCESS_DENIED:
      return "access denied";
    case EError::OPERATION_IN_PROGRESS:
      return "operation already in progress";
    case EError::NOT_INITIALIZED:
      return "networking stack not initialized";
    case EError::NETWORK_UNREACHABLE:
      return "network unreachable";
    case EError::HOST_UNREACHABLE:
      return "host unreachable";
    case EError::NAME_TOO_LONG:
      return "name too long";
    case EError::ADDRESS_FAMILY_UNSUPPORTED:
      return "address family unsupported";
    case EError::SOCKET_TYPE_UNSUPPORTED:
      return "socket type unsupported";
    case EError::OPERATION_NOT_SUPPORTED:
      return "operation not supported";
    case EError::NOT_CONNECTED:
      return "not connected";
    case EError::ALREADY_CONNECTED:
      return "already connected";
    case EError::ALREADY_IN_PROGRESS:
      return "already in progress";
    case EError::SHUTDOWN:
      return "socket is shut down";
    case EError::TOO_MANY_SOCKETS:
      return "too many open sockets";
    case EError::NO_MEMORY:
      return "out of memory";
    case EError::MESSAGE_TOO_LARGE:
      return "message too large";
    case EError::DESTINATION_ADDRESS_REQUIRED:
      return "destination address required";
    case EError::HOST_NOT_AVAILABLE:
      return "host down";
    case EError::DNS_HOST_NOT_FOUND:
      return "host not found";
    case EError::DNS_TRY_AGAIN:
      return "dns lookup should be retried";
    case EError::DNS_NO_RECOVERY:
      return "dns server failure";
    case EError::DNS_NO_DATA:
      return "dns name does not exist";
    case EError::UNKNOWN:
      return "unknown error";
    default:
      return "unknown error";
  }
}

void printError(Error err)
{
  if (err.ok())
    return;

  //NOTE: the raw code is an errno value for the socket calls and an EAI_* value
  //      for name resolution, so it is reported without claiming which one it is
  std::fprintf(stderr,
               "sil: %s (code %d)\n",
               errorToString(err.code),
               err.valueCode);
}

int SockAddr::capacity()
{
  return static_cast<int>(sizeof(sockaddr_storage));
}

Error SockAddr::resolve(SockAddr& out, const char* host, const char* service, EFamily family, EType type)
{
  out.clear();
  out.mResolved = false;

  const int addressFamily = EnumToMacro_Family(family);
  const int socketType = EnumToMacro_Type(type);
  if (addressFamily < 0 || socketType < 0)
    return parameterInvalid();

  if (family == EFamily::LOCAL)
  {
    //NOTE: unlike windows, posix implements AF_UNIX for streams and datagrams
    //      alike, so no type is rejected here
    if (host == nullptr || *host == '\0')
      return Error{EError::ADDRESS_NOT_AVAILABLE, 0};

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    const size_t pathLength = std::strlen(host);
    if (pathLength >= sizeof(addr.sun_path))
      return Error{EError::NAME_TOO_LONG, 0};

    //NOTE: copying length + 1 keeps the terminator, the rest of sun_path stays zeroed
    std::memcpy(addr.sun_path, host, pathLength + 1);

    const int total = static_cast<int>(offsetof(sockaddr_un, sun_path) + pathLength + 1);
    std::memcpy(&out.storage, &addr, static_cast<size_t>(total));

    out.mLen = total;
    out.mResolved = true;
    return Error{};
  }

  if (service == nullptr)
    return parameterInvalid();

  addrinfo hints{};
  hints.ai_family = addressFamily;
  hints.ai_socktype = socketType;
  hints.ai_protocol = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;
  //NOTE: a null host means "any interface", which getaddrinfo only honours when asked
  hints.ai_flags = (host == nullptr) ? AI_PASSIVE : 0;

  addrinfo* result = nullptr;
  //NOTE: getaddrinfo returns EAI_* on failure, which on posix is not -1 and does
  //      not touch errno. EAI_SYSTEM is the one exception, it defers to errno
  const int error = ::getaddrinfo(host, service, &hints, &result);
  if (error != 0)
  {
    if (error == EAI_SYSTEM)
      return fromErrno();

    return Error{MacroToEnum_Error(error), error};
  }

  if (result == nullptr)
    return Error{EError::HOST_NOT_AVAILABLE, 0};

  for (const addrinfo* rp = result; rp != nullptr; rp = rp->ai_next)
  {
    if (rp->ai_addr == nullptr || rp->ai_addrlen == 0 || rp->ai_addrlen > sizeof(out.storage))
      continue;

    std::memcpy(&out.storage, rp->ai_addr, rp->ai_addrlen);
    out.mLen = static_cast<int>(rp->ai_addrlen);
    out.mResolved = true;
    break;
  }

  ::freeaddrinfo(result);

  if (!out.mResolved)
    return Error{EError::ADDRESS_NOT_AVAILABLE, 0};

  return Error{};
}

SockAddr::SockAddr(const std::string& host, const std::string& service, EFamily family, EType type)
{
  (void)resolve(*this, host.c_str(), service.c_str(), family, type);
}

SockAddr::SockAddr(const std::string& host, uint16_t port, EFamily family, EType type)
{
  const std::string portText = std::to_string(port);
  (void)resolve(*this, host.c_str(), portText.c_str(), family, type);
}

SockAddr::SockAddr(uint16_t port, EFamily family, EType type)
{
  const std::string portText = std::to_string(port);
  (void)resolve(*this, nullptr, portText.c_str(), family, type);
}

SockAddr::SockAddr(const std::string& path, EFamily family, EType type)
{
  //NOTE: an AF_UNIX address has no port, so no service is passed
  (void)resolve(*this, path.c_str(), nullptr, family, type);
}

std::span<std::byte> SockAddr::bytes()
{
  return std::span<std::byte>(reinterpret_cast<std::byte*>(&storage),
                              static_cast<size_t>(len()));
}

std::span<const std::byte> SockAddr::bytes() const
{
  return std::span<const std::byte>(reinterpret_cast<const std::byte*>(&storage),
                                    static_cast<size_t>(len()));
}

int SockAddr::len() const
{
  return mLen;
}

void SockAddr::len(int n)
{
  if (n < 0)
    n = 0;
  if (n > capacity())
    n = capacity();

  mLen = n;
  //NOTE: a non zero length means bytes were written into the storage, which is how
  //      an out call such as recvfrom publishes the address it just received
  mResolved = (n > 0);
}

void SockAddr::clear()
{
  storage = sockaddr_storage{};
  mLen = capacity();
  mResolved = false;
}

bool SockAddr::valid() const
{
  return mResolved && mLen > 0;
}

EFamily SockAddr::family() const
{
  if (!valid())
    return EFamily::UNSPECIFIED;

  switch (static_cast<int>(storage.ss_family))
  {
    case AF_INET:
      return EFamily::IPV4;
    case AF_INET6:
      return EFamily::IPV6;
    case AF_UNIX:
      return EFamily::LOCAL;
    default:
      return EFamily::UNSPECIFIED;
  }
}

uint16_t SockAddr::port() const
{
  if (!valid())
    return 0;

  switch (static_cast<int>(storage.ss_family))
  {
    case AF_INET:
      return ntohs(reinterpret_cast<const sockaddr_in*>(&storage)->sin_port);
    case AF_INET6:
      return ntohs(reinterpret_cast<const sockaddr_in6*>(&storage)->sin6_port);
    default:
      return 0;
  }
}

std::string SockAddr::toString() const
{
  if (!valid())
    return "<unresolved>";

  char host[NI_MAXHOST]{};
  char service[NI_MAXSERV]{};

  const int error = ::getnameinfo(reinterpret_cast<const sockaddr*>(&storage),
                                  static_cast<socklen_t>(len()),
                                  host,
                                  sizeof(host),
                                  service,
                                  sizeof(service),
                                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (error != 0)
    return "<invalid>";

  std::string result = host;
  if (static_cast<int>(storage.ss_family) == AF_INET6)
    result = "[" + result + "]";

  result += ':';
  result += service;
  return result;
}

bool SockAddr::operator==(const SockAddr& other) const
{
  if (mResolved != other.mResolved || mLen != other.mLen)
    return false;

  if (mLen == 0)
    return true;

  return std::memcmp(&storage, &other.storage, static_cast<size_t>(mLen)) == 0;
}

}

#endif
