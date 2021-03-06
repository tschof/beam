#ifndef BEAM_TCP_SOCKET_READER_HPP
#define BEAM_TCP_SOCKET_READER_HPP
#include "Beam/IO/EndOfFileException.hpp"
#include "Beam/IO/IO.hpp"
#include "Beam/IO/Reader.hpp"
#include "Beam/Network/Network.hpp"
#include "Beam/Network/NetworkDetails.hpp"
#include "Beam/Network/SocketException.hpp"
#include "Beam/Routines/Async.hpp"

namespace Beam {
namespace Network {

  /** Reads from a TCP socket. */
  class TcpSocketReader {
    public:
      bool IsDataAvailable() const;

      template<typename BufferType>
      std::size_t Read(Out<BufferType> destination);

      std::size_t Read(char* destination, std::size_t size);

      template<typename BufferType>
      std::size_t Read(Out<BufferType> destination, std::size_t size);

    private:
      friend class TcpSocketChannel;
      static constexpr auto DEFAULT_READ_SIZE = std::size_t(8 * 1024);
      std::shared_ptr<Details::TcpSocketEntry> m_socket;

      TcpSocketReader(std::shared_ptr<Details::TcpSocketEntry> socket);
      TcpSocketReader(const TcpSocketReader&) = delete;
      TcpSocketReader& operator =(const TcpSocketReader&) = delete;
  };

  inline bool TcpSocketReader::IsDataAvailable() const {
    auto command = boost::asio::socket_base::bytes_readable(true);
    {
      auto lock = std::lock_guard(m_socket->m_mutex);
      m_socket->m_socket.io_control(command);
    }
    return command.get() > 0;
  }

  template<typename Buffer>
  std::size_t TcpSocketReader::Read(Out<Buffer> destination) {
    return Read(Store(destination), DEFAULT_READ_SIZE);
  }

  inline std::size_t TcpSocketReader::Read(char* destination,
      std::size_t size) {
    auto readResult = Routines::Async<std::size_t>();
    {
      auto lock = std::lock_guard(m_socket->m_mutex);
      if(!m_socket->m_isOpen) {
        BOOST_THROW_EXCEPTION(IO::EndOfFileException());
      }
      m_socket->m_isReadPending = true;
      m_socket->m_socket.async_read_some(boost::asio::buffer(destination, size),
        [&] (const auto& error, auto readSize) {
          if(error) {
            readResult.GetEval().SetException(SocketException(error.value(),
              error.message()));
          } else {
            readResult.GetEval().SetResult(readSize);
          }
        });
    }
    try {
      auto result = readResult.Get();
      m_socket->EndReadOperation();
      return result;
    } catch(const std::exception&) {
      m_socket->EndReadOperation();
      std::throw_with_nested(IO::EndOfFileException());
    }
  }

  template<typename Buffer>
  std::size_t TcpSocketReader::Read(Out<Buffer> destination, std::size_t size) {
    auto initialSize = destination->GetSize();
    auto readSize = std::min(DEFAULT_READ_SIZE, size);
    destination->Grow(readSize);
    auto result = Read(destination->GetMutableData() + initialSize, readSize);
    destination->Shrink(readSize - result);
    return result;
  }

  inline TcpSocketReader::TcpSocketReader(
    std::shared_ptr<Details::TcpSocketEntry> socket)
    : m_socket(std::move(socket)) {}
}

  template<>
  struct ImplementsConcept<Network::TcpSocketReader, IO::Reader> :
    std::true_type {};
}

#endif
