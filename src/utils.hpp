#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#ifdef _WIN32

//NOTE:link against Ws2_32.lib is required
#include <winsock2.h>
#include <ws2tcpip.h>

namespace sil {

//NOTE: SOCKET is UINT_PTR, which is exactly what socket() returns, so this is not a coincidence
using Handle = SOCKET;
//NOTE: underflows to the highest value possible, which is the same value winsock uses
constexpr Handle INVALID_SOCKET_HANDLE = ~Handle{0};
static_assert(INVALID_SOCKET_HANDLE == INVALID_SOCKET);

}

#else

#include <sys/socket.h>
#include <sys/un.h>

namespace sil {

//NOTE: on posix a socket is a file descriptor, which is exactly what socket()
//      hands back and exactly what every other call in here takes
using Handle = intptr_t;
constexpr Handle INVALID_SOCKET_HANDLE = -1;

}

#endif

namespace sil {

constexpr int MAX_BUF_SIZE = 100;

enum class EFamily
{
  LOCAL,
  IPV4,
  IPV6,
  UNSPECIFIED
};

enum class EType
{
  STREAM,
  DATAGRAM
};

enum class EError
{
  //NOTE: not SUCCESS, raserror.h and mprerror.h both define that macro
  OK = 0,

  AGAIN,

  CONNECTION_RESET,
  CONNECTION_REFUSED,
  CONNECTION_ABORTED,
  CONNECTION_DOWN,

  NO_BUFFER_SPACE,
  TIMED_OUT,
  INTERRUPTED,

  SOCKET_INVALID,
  PARAMETER_INVALID,
  PROTOCOL_INVALID,

  ADDRESS_IN_USE,
  ADDRESS_NOT_AVAILABLE,
  ACCESS_DENIED,

  OPERATION_IN_PROGRESS,

  NOT_INITIALIZED,

  NETWORK_UNREACHABLE,
  HOST_UNREACHABLE,

  NAME_TOO_LONG,

  ADDRESS_FAMILY_UNSUPPORTED,
  SOCKET_TYPE_UNSUPPORTED,
  OPERATION_NOT_SUPPORTED,

  NOT_CONNECTED,
  ALREADY_CONNECTED,
  ALREADY_IN_PROGRESS,
  SHUTDOWN,

  TOO_MANY_SOCKETS,
  NO_MEMORY,

  MESSAGE_TOO_LARGE,
  DESTINATION_ADDRESS_REQUIRED,

  HOST_NOT_AVAILABLE,
  //NOTE: not HOST_NOT_FOUND, _wsa_errnos.h defines that macro
  DNS_HOST_NOT_FOUND,
  DNS_TRY_AGAIN,
  DNS_NO_RECOVERY,
  DNS_NO_DATA,

  UNKNOWN = -1
};

struct Error
{
  EError code = EError::OK;
  int valueCode = 0;

  constexpr bool ok() const { return code == EError::OK; }
  constexpr explicit operator bool() const { return ok(); }
};

//NOTE: init() brings the platform's networking stack up and cleanup() tears it
//      back down, so portable code can call the pair unconditionally instead of
//      spelling out WSAStartup and WSACleanup. On posix both are no-ops, the
//      kernel owns the sockets, but they exist so the shape of the code does not
//      have to change per platform. One init() per cleanup(), and every call in
//      the library needs an init() first.
Error init();
Error cleanup();

//NOTE: takes by value so it can be handed the temporary returned by any sil function
void printError(Error err);
const char* errorToString(EError code);

//NOTE: this is the one type that owns OS data. No os type ever appears in a signature:
//      callers move it around as a byte span, which is all the OS calls actually need.
struct SockAddr
{
public:
  SockAddr(const std::string& host, const std::string& service, EFamily family, EType type);

  SockAddr(const std::string& host, uint16_t port, EFamily family, EType type);

  SockAddr(uint16_t port, EFamily family, EType type);

  //NOTE: EFamily::LOCAL carries a path where the others carry a host and a port,
  //      so this is the only form that makes sense for it
  SockAddr(const std::string& path, EFamily family, EType type);

  //NOTE: default constructed is empty and unresolved, so it can be handed to an
  //      out-parameter such as recvFrom without any further setup
  SockAddr() = default;

  // Byte view of the address. The extent is the number of VALID bytes, never the
  // storage capacity, so .size() doubles as the length handed to the os.
  std::span<std::byte>       bytes();
  std::span<const std::byte> bytes() const;

  // Byte length. Deliberately a plain int: winsock's namelen is an int on every
  // toolchain (socklen_t is a typedef for int), so this never narrows.
  int  len() const;
  //NOTE: clamps to the storage capacity, so a bogus value can never build an
  //      out of bounds span. A non-zero length also marks the address valid,
  //      which is what lets recvFrom publish the peer it just wrote.
  void len(int n);

  // Puts the address in the state an os out-call needs: storage zeroed and
  // len() set to the full capacity, so it can be passed as the in/out namelen.
  // valid() is false until something is actually written into it.
  void clear();

  bool        valid() const;
  EFamily     family() const;
  uint16_t    port() const;
  //NOTE: goes through getnameinfo, so winsock has to still be initialized when it is called
  std::string toString() const;

  bool operator==(const SockAddr& other) const;
  bool operator!=(const SockAddr& other) const { return !(*this == other); }

  // Returns the error instead of printing it, so a caller can decide what to do.
  static Error resolve(SockAddr& out, const char* host, const char* service, EFamily family, EType type);

private:
  sockaddr_storage storage{};
  int  mLen = 0;
  bool mResolved = false;

  static int capacity();
};

int    EnumToMacro_Family(EFamily var);
int    EnumToMacro_Type(EType var);
//NOTE: maps a native error macro to EError. Windows has one namespace of WSA*
//      codes, posix has two: errno for the syscalls and the negative EAI_* codes
//      getaddrinfo returns, and the sign tells them apart
EError MacroToEnum_Error(int error);

}
