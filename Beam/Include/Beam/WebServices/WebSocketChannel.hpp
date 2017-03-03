#ifndef BEAM_WEBSOCKETCHANNEL_HPP
#define BEAM_WEBSOCKETCHANNEL_HPP
#include <boost/noncopyable.hpp>
#include "Beam/IO/Channel.hpp"
#include "Beam/WebServices/WebServices.hpp"
#include "Beam/WebServices/WebSocket.hpp"
#include "Beam/WebServices/WebSocketConnection.hpp"
#include "Beam/WebServices/WebSocketReader.hpp"
#include "Beam/WebServices/WebSocketWriter.hpp"

namespace Beam {
namespace WebServices {

  /*! \class WebSocketChannel
      \brief Implements the Channel interface for a WebSocket.
   */
  template<typename ChannelType>
  class WebSocketChannel : private boost::noncopyable {
    public:
      using WebSocket = WebSocket<ChannelType>;
      using Identifier = typename WebSocket::Channel::Identifier;
      using Connection = WebSocketConnection<WebSocket>;
      using Reader = WebSocketReader<WebSocket>;
      using Writer = WebSocketWriter<WebSocket>;

      //! Constructs a WebSocketChannel.
      /*!
        \param config The config used to initialize the WebSocket.
        \param channelBuilder Builds the Channel used to connect to the server.
      */
      WebSocketChannel(WebSocketConfig config,
        typename WebSocket::ChannelBuilder channelBuilder);

      //! Returns the underlying WebSocket.
      WebSocket& GetSocket();

      const Identifier& GetIdentifier() const;

      Connection& GetConnection();

      Reader& GetReader();

      Writer& GetWriter();

    private:
      std::shared_ptr<WebSocket> m_socket;
      Connection m_connection;
      Reader m_reader;
      Writer m_writer;
  };

  template<typename ChannelType>
  WebSocketChannel<ChannelType>::WebSocketChannel(WebSocketConfig config,
      typename WebSocketChannel::WebSocket::ChannelBuilder channelBuilder)
      : m_socket{std::make_shared<WebSocket>(std::move(config),
          std::move(channelBuilder))},
        m_connection{m_socket},
        m_reader{m_socket},
        m_writer{m_socket} {}

  template<typename ChannelType>
  typename WebSocketChannel<ChannelType>::WebSocket&
      WebSocketChannel<ChannelType>::WebSocketChannel::GetSocket() {
    return *m_socket;
  }

  template<typename ChannelType>
  const typename WebSocketChannel<ChannelType>::Identifier&
      WebSocketChannel<ChannelType>::GetIdentifier() const {
    return m_identifier;
  }

  template<typename ChannelType>
  typename WebSocketChannel<ChannelType>::Connection&
      WebSocketChannel<ChannelType>::GetConnection() {
    return m_connection;
  }

  template<typename ChannelType>
  typename WebSocketChannel<ChannelType>::Reader&
      WebSocketChannel<ChannelType>::GetReader() {
    return m_reader;
  }

  template<typename ChannelType>
  typename WebSocketChannel<ChannelType>::Writer&
      WebSocketChannel<ChannelType>::GetWriter() {
    return m_writer;
  }
}

  template<typename ChannelType>
  struct ImplementsConcept<WebServices::WebSocketChannel<ChannelType>,
      IO::Channel<
      typename WebServices::WebSocketChannel<ChannelType>::Identifier,
      typename WebServices::WebSocketChannel<ChannelType>::Connection,
      typename WebServices::WebSocketChannel<ChannelType>::Reader,
      typename WebServices::WebSocketChannel<ChannelType>::Writer>>
      : std::true_type {};
}

#endif
