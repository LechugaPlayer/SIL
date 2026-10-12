//NOTE: everything here talks to 127.0.0.1 on a port the os picks, so the test needs
//      no network, no name resolution and no free port agreed on beforehand, and
//      it can be run twice in a row

#include "SockDatagram.hpp"
#include "SockStream.hpp"
#include "utils.hpp"

#include <cstdio>
#include <cstring>

namespace {

int failures = 0;

#define CHECK(expr)                                                      \
  do                                                                     \
  {                                                                      \
    if (!(expr))                                                         \
    {                                                                    \
      std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #expr); \
      ++failures;                                                        \
    }                                                                    \
  } while (0)

//NOTE: a failed call is reported with the library's own text, which is the first
//      thing worth knowing when one of these breaks on a new platform
#define CHECK_OK(expr)                                                   \
  do                                                                     \
  {                                                                      \
    const sil::Error checked = (expr);                                   \
    if (!checked.ok())                                                   \
    {                                                                    \
      std::fprintf(stderr, "%s:%d: %s failed: %s (code %d)\n", __FILE__, __LINE__, #expr, \
                   sil::errorToString(checked.code), checked.valueCode); \
      ++failures;                                                        \
    }                                                                    \
  } while (0)

void testAddress()
{
  sil::SockAddr empty;
  CHECK(!empty.valid());
  CHECK(empty.family() == sil::EFamily::UNSPECIFIED);
  CHECK(empty.port() == 0);
  CHECK(empty.toString() == "<unresolved>");

  sil::SockAddr v4("127.0.0.1", 8080, sil::EFamily::IPV4, sil::EType::STREAM);
  CHECK(v4.valid());
  CHECK(v4.family() == sil::EFamily::IPV4);
  CHECK(v4.port() == 8080);
  CHECK(v4.toString() == "127.0.0.1:8080");

  sil::SockAddr v6("::1", 8080, sil::EFamily::IPV6, sil::EType::STREAM);
  CHECK(v6.valid());
  CHECK(v6.family() == sil::EFamily::IPV6);
  CHECK(v6.port() == 8080);
  CHECK(v6.toString() == "[::1]:8080");

  sil::SockAddr same("127.0.0.1", 8080, sil::EFamily::IPV4, sil::EType::STREAM);
  CHECK(v4 == same);
  CHECK(v4 != v6);

  //NOTE: a null service is only meaningful for EFamily::LOCAL
  sil::SockAddr out;
  CHECK(sil::SockAddr::resolve(out, "127.0.0.1", nullptr, sil::EFamily::IPV4, sil::EType::STREAM).code
        == sil::EError::PARAMETER_INVALID);
  CHECK(!out.valid());
}

void testInvalidSocket()
{
  sil::SockDatagram datagram;
  sil::SockStream stream;
  sil::SockAddr address("127.0.0.1", 8080, sil::EFamily::IPV4, sil::EType::STREAM);

  CHECK(!sil::isValid(datagram));
  CHECK(!sil::isValid(stream));
  CHECK(sil::toString(datagram) == "<invalid>");
  CHECK(sil::toString(stream) == "<invalid>");

  CHECK(sil::bind(datagram, address).code == sil::EError::SOCKET_INVALID);
  CHECK(sil::connect(stream, address).code == sil::EError::SOCKET_INVALID);
  CHECK(sil::listen(stream, 1).code == sil::EError::SOCKET_INVALID);
  CHECK(sil::close(datagram).code == sil::EError::SOCKET_INVALID);
  CHECK(sil::close(stream).code == sil::EError::SOCKET_INVALID);
}

void testDatagram()
{
  sil::SockDatagram socket;
  CHECK_OK(sil::socket(socket, sil::EFamily::IPV4));
  CHECK(sil::isValid(socket));
  CHECK(sil::toString(socket) == "<unbound>");

  sil::SockAddr unresolved;
  CHECK(sil::bind(socket, unresolved).code == sil::EError::ADDRESS_NOT_AVAILABLE);

  sil::SockAddr local("127.0.0.1", 0, sil::EFamily::IPV4, sil::EType::DATAGRAM);
  CHECK_OK(sil::bind(socket, local));
  CHECK_OK(sil::getSockName(socket, local));
  CHECK(local.port() != 0);
  CHECK(sil::toString(socket) == local.toString());

  bool reuse = true;
  CHECK_OK(sil::setOption(socket, sil::EOption::BROADCAST, true));
  CHECK_OK(sil::getOption(socket, sil::EOption::BROADCAST, reuse));
  CHECK(reuse);

  CHECK_OK(sil::setSendBufferSize(socket, 64 * 1024));
  CHECK_OK(sil::setReceiveBufferSize(socket, 64 * 1024));
  CHECK(sil::setReceiveTimeout(socket, -1).code == sil::EError::PARAMETER_INVALID);
  //NOTE: keeps a broken recvFrom from hanging the test instead of failing it
  CHECK_OK(sil::setReceiveTimeout(socket, 2000));

  int sent = 0;
  CHECK_OK(sil::sendTo(socket, "ping", 4, local, 0, sent));
  CHECK(sent == 4);
  CHECK(sil::sendTo(socket, nullptr, 4, local, 0, sent).code == sil::EError::PARAMETER_INVALID);
  CHECK(sent == 0);

  char buffer[64]{};
  int received = 0;
  sil::SockAddr from;
  CHECK_OK(sil::recvFrom(socket, buffer, sizeof(buffer), from, 0, received));
  CHECK(received == 4);
  CHECK(std::memcmp(buffer, "ping", 4) == 0);
  CHECK(from == local);

  //NOTE: with nothing queued, a non blocking socket answers AGAIN straight away
  CHECK_OK(sil::setNonBlocking(socket, true));
  CHECK(sil::recvFrom(socket, buffer, sizeof(buffer), from, 0, received).code == sil::EError::AGAIN);
  CHECK(received == 0);
  CHECK(!from.valid());

  CHECK_OK(sil::close(socket));
  CHECK(!sil::isValid(socket));
  CHECK(sil::close(socket).code == sil::EError::SOCKET_INVALID);
}

void testStream()
{
  sil::SockStream listener;
  CHECK_OK(sil::socket(listener, sil::EFamily::IPV4));
  CHECK(sil::toString(listener) == "<unbound>");

  sil::SockAddr local("127.0.0.1", 0, sil::EFamily::IPV4, sil::EType::STREAM);
  CHECK_OK(sil::bind(listener, local));
  CHECK(sil::listen(listener, -1).code == sil::EError::PARAMETER_INVALID);
  CHECK_OK(sil::listen(listener, 4));
  CHECK_OK(sil::getSockName(listener, local));
  CHECK(local.port() != 0);

  sil::SockStream client;
  CHECK_OK(sil::socket(client, sil::EFamily::IPV4));

  sil::SockAddr none;
  CHECK(sil::getPeerName(client, none).code == sil::EError::NOT_CONNECTED);
  CHECK(!none.valid());

  CHECK_OK(sil::connect(client, local));

  sil::SockStream server;
  sil::SockAddr peer;
  CHECK_OK(sil::accept(listener, server, peer));
  CHECK(sil::isValid(server));
  CHECK(server.family == sil::EFamily::IPV4);

  //NOTE: the two ends have to agree on who is who
  sil::SockAddr clientName;
  sil::SockAddr serverPeer;
  CHECK_OK(sil::getSockName(client, clientName));
  CHECK_OK(sil::getPeerName(server, serverPeer));
  CHECK(peer == clientName);
  CHECK(serverPeer == clientName);

  bool noDelay = false;
  CHECK_OK(sil::setOption(client, sil::EOption::NO_DELAY, true));
  CHECK_OK(sil::getOption(client, sil::EOption::NO_DELAY, noDelay));
  CHECK(noDelay);

  CHECK_OK(sil::setReceiveTimeout(server, 2000));
  CHECK_OK(sil::setReceiveTimeout(client, 2000));

  int sent = 0;
  CHECK_OK(sil::send(client, "ping", 4, 0, sent));
  CHECK(sent == 4);

  //NOTE: a stream may hand the payload over in pieces, so the read is a loop
  char buffer[64]{};
  int total = 0;
  while (total < 4)
  {
    int received = 0;
    const sil::Error err = sil::recv(server, buffer + total, sizeof(buffer) - static_cast<size_t>(total), 0, received);
    CHECK_OK(err);
    if (!err.ok() || received == 0)
      break;
    total += received;
  }
  CHECK(total == 4);
  CHECK(std::memcmp(buffer, "ping", 4) == 0);

  CHECK_OK(sil::send(server, "pong", 4, 0, sent));
  CHECK(sent == 4);

  //NOTE: after shutWrite the server reads the end of the stream, 0 bytes and no
  //      error, while the client can still read what the server already sent
  CHECK_OK(sil::shutWrite(client));

  int received = -1;
  CHECK_OK(sil::recv(server, buffer, sizeof(buffer), 0, received));
  CHECK(received == 0);

  total = 0;
  while (total < 4)
  {
    received = 0;
    const sil::Error err = sil::recv(client, buffer + total, sizeof(buffer) - static_cast<size_t>(total), 0, received);
    CHECK_OK(err);
    if (!err.ok() || received == 0)
      break;
    total += received;
  }
  CHECK(total == 4);
  CHECK(std::memcmp(buffer, "pong", 4) == 0);

  CHECK_OK(sil::close(client));
  CHECK_OK(sil::close(server));
  CHECK_OK(sil::close(listener));
  CHECK(sil::close(listener).code == sil::EError::SOCKET_INVALID);
}

void testConnectionRefused()
{
  //NOTE: bound but never listening, which is a port nothing can be connected to
  sil::SockStream holder;
  CHECK_OK(sil::socket(holder, sil::EFamily::IPV4));

  sil::SockAddr local("127.0.0.1", 0, sil::EFamily::IPV4, sil::EType::STREAM);
  CHECK_OK(sil::bind(holder, local));
  CHECK_OK(sil::getSockName(holder, local));

  sil::SockStream client;
  CHECK_OK(sil::socket(client, sil::EFamily::IPV4));
  CHECK(sil::connect(client, local).code == sil::EError::CONNECTION_REFUSED);

  CHECK_OK(sil::close(client));
  CHECK_OK(sil::close(holder));
}

} // namespace

int main()
{
  CHECK_OK(sil::init());

  testAddress();
  testInvalidSocket();
  testDatagram();
  testStream();
  testConnectionRefused();

  CHECK_OK(sil::cleanup());

  if (failures != 0)
  {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }

  std::printf("all checks passed\n");
  return 0;
}
