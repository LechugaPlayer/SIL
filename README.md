# SIL

SIL (simple internet library) is a project made to learn about networking, API design, and C++ while
providing a thin abstraction to support both Linux and Windows.

The library will support most fundamentals features needed for networking. This project was chosen because
both windows and linux implementations are based on BSD, making it reasonably easy to abstract concepts,
making the learning process easier.


## Status
The library is considered to be finished, for now I won't be updating it anymore, from here more features
could be implemented but those features are not strictly related to netwkorking, they just make it easier
to deal with it, such as providing a concurrency model, async operations, etc.

There are many features from netwroking left behind such as not providing raw sockets, protocols,
more sockets, etc. The idea was to stop at stream and datagrams.


## Building
-link against Ws2_32.lib is required

## Design notes

- `Handle` is 'uintptr_t' on Windows and `intptr_t` on POSIX, so on both it is exactly what
  the OS calls return. An invalid handle is `-1` everywhere because on windows is set as the
   highest value, which `-1` is equal to because it underflows and linux expect exactly `-1`.
- `SockAddr` is the only type that owns OS data, and it is the only one that includes an OS
  struct. It is `sockaddr_storage` internally, and the library never owns it.
- **No OS type appears in any signature.** An address is handed around as
  `std::span<std::byte>` because I wanted to avoid having some OS specific structs flying around
  in the user's code
- `SockAddr::clear()` puts an address in the state an OS out-call needs: storage zeroed and `len()`
  set to the full capacity, ready to be passed as the in/out `namelen` that `recvfrom`, `accept`,
  `getsockname` and `getpeername` write back into. `valid()` is false until something is written.
- `Error` carries both the portable `EError` and the raw native code, so nothing is lost.
  `Error::ok()` and the explicit `operator bool` let failures be tested directly.
- Errors are returned, not printed. `printError()` function is available but the library never calls it,
  and `SockAddr::resolve` returns the error so a caller can decide what to do with it.
  This is a deliberate design choice, a helper printer is provided but ultimately, the user decides
  what to do with the error.
- `EOption` is shared by both socket modules and lives in `utils.hpp`, so the mapping from an option
  to its native level is written once per platform instead of once per module. Which options a given
  socket accepts is the OS's business to answer: an option the socket will not take comes back as
  `EError::PROTOCOL_INVALID`. Windows has no `SO_REUSEPORT`, so it is not offered.
- `SockStream` names `shutWrite`, `shutRead` and `shut` instead of taking a `how` parameter because
  I considered that to be more approrpiate.
- `send` and `recv` on a stream report how many bytes moved and never loop, a looping functions might be
  beneficial to handle partial reads/writes but it was left to the user.


### Where the two platforms differ

- **Error namespaces.** This turned to be one of the biggest differences, the idea was to use enums
  to handle errors and mape them to a common value. This turned out being a bit painful to worki with
  because both platforms differs on number of errors, some system calls fail differently or the handle it
  (or not) while th other platform does the opposite.
  Windows has one set of `WSA*` codes read through `WSAGetLastError()`.
  POSIX has two: `errno` for the syscalls, always positive, and the `EAI_*` codes `getaddrinfo`
  returns, always negative. `MacroToEnum_Error` maps both, and the sign is what tells them apart.
  `EAI_SYSTEM` is the one `getaddrinfo` code that defers to `errno`, so `resolve` reads `errno`
  for it instead of going through the conversion.
- **An expired timeout is `EAGAIN`,** which is exactly what an empty non-blocking queue reports, so
  POSIX cannot tell the two apart and both surface as `EError::AGAIN`. Windows reports
  `WSAETIMEDOUT` and therefore `EError::TIMED_OUT`.
- **`MSG_NOSIGNAL` is added to `send` on POSIX.** Without it a write to a peer that already closed
  raises `SIGPIPE`, which kills the process instead of returning an error. `EPIPE` maps to
  `EError::CONNECTION_RESET`, the same as `ECONNRESET`, so both platforms report one thing for a
  broken connection. The caller's own `flags` are left untouched.
- **A zero-length `recv` on a blocking stream is not a no-op on POSIX,** it waits for the pipe to
  have something in it, while Windows returns at once.
- **`getpeername` after the peer is gone** fails on Windows with `WSAENOTCONN`, while POSIX still
  answers with the address it remembers.
- **An oversized datagram fails on Windows** with `WSAEMSGSIZE` and the payload is discarded. POSIX
  truncates instead, returns the number of bytes copied and throws the rest away. Passing
  `MSG_TRUNC` in `flags` asks POSIX for the real size of the datagram that was truncated away;
  Windows has no equivalent.
- **`getsockname` fails on Windows** until the socket is bound (`WSAEINVAL`). POSIX answers straight
  away with `0.0.0.0:0`. `toString` normalizes that back to `<unbound>` on both, by treating a zero
  port on an INET address as unbound.
- **`close()` on POSIX** releases the descriptor even when it reports a failure such as `EINTR`, so
  `close` invalidates the handle either way. Leaving it live would let the next call hit a recycled
  descriptor.
- **`AF_UNIX` datagrams exist only on POSIX.** Windows implements `AF_UNIX` for streams alone, so
  `SockAddr::resolve` refuses an `AF_UNIX` datagram there and accepts it everywhere else.
- **A datagram socket refuses `NO_DELAY` on both platforms,** since it is an `IPPROTO_TCP` option,
  but not in the same words: POSIX blames the protocol (`PROTOCOL_INVALID`) and Windows checks the
  option against the socket type and blames the argument (`PARAMETER_INVALID`). `SO_KEEPALIVE` is a
  `SOL_SOCKET` option that Linux accepts on anything and Windows refuses on a datagram.
- **`EError::NOT_INITIALIZED` has no POSIX counterpart,** since `init()` has nothing to do.

## Example

```cpp
// WSAStartup on windows, nothing at all on posix
if (auto err = sil::init(); !err.ok())
  return 1;

sil::SockDatagram socket;
if (auto err = sil::socket(socket, sil::EFamily::IPV4); !err.ok())
  sil::printError(err);

sil::SockAddr local(9000, sil::EFamily::IPV4, sil::EType::DATAGRAM);
if (auto err = sil::bind(socket, local); !err.ok())
  sil::printError(err);

sil::SockAddr peer("127.0.0.1", 9000, sil::EFamily::IPV4, sil::EType::DATAGRAM);

int sent = 0;
if (auto err = sil::sendTo(socket, "ping", 4, peer, 0, sent); !err.ok())
  sil::printError(err);

char buffer[64];
int received = 0;
sil::SockAddr from;
if (auto err = sil::recvFrom(socket, buffer, sizeof(buffer), from, 0, received); !err.ok())
  sil::printError(err);

std::printf("sent %d, received %d from %s: %.*s\n",
            sent, received, from.toString().c_str(), received, buffer);

sil::printError(sil::close(socket));
sil::printError(sil::cleanup());
```

A stream exchange, on the same terms. The listener is bound to port 0 and asked for its real port,
which is what makes it re-runnable:

```cpp
sil::SockStream listener;
if (auto err = sil::socket(listener, sil::EFamily::IPV4); !err.ok())
  return 1;

sil::SockAddr local(0, sil::EFamily::IPV4, sil::EType::STREAM);
if (auto err = sil::bind(listener, local); !err.ok())
  return 1;
if (auto err = sil::listen(listener, 4); !err.ok())
  return 1;

if (auto err = sil::getSockName(listener, local); !err.ok())
  return 1;

sil::SockStream client;
sil::socket(client, sil::EFamily::IPV4);
sil::connect(client, local);

sil::SockStream server;
sil::SockAddr peer;
sil::accept(listener, server, peer);

// the payload, however many bytes of it the socket actually took
int sent = 0;
if (auto err = sil::send(client, "ping", 4, 0, sent); !err.ok())
  sil::printError(err);

char buffer[64];
int received = 0;
if (auto err = sil::recv(server, buffer, sizeof(buffer), 0, received); !err.ok())
  sil::printError(err);

// the client is done writing but still reading, which is what shutWrite is for
sil::shutWrite(client);

sil::printError(sil::close(client));
sil::printError(sil::close(server));
sil::printError(sil::close(listener));
```
