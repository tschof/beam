#ifndef BEAM_HTTPSERVER_HPP
#define BEAM_HTTPSERVER_HPP
#include <boost/noncopyable.hpp>
#include "Beam/IO/OpenState.hpp"
#include "Beam/Routines/RoutineHandler.hpp"
#include "Beam/Routines/RoutineHandlerGroup.hpp"
#include "Beam/WebServices/HttpRequestParser.hpp"
#include "Beam/WebServices/WebServices.hpp"

namespace Beam {
namespace WebServices {

  /*! \class HttpServer
      \brief Implements an HTTP server.
      \tparam ServerConnectionType The type of ServerConnection accepting
              Channels.
   */
  template<typename ServerConnectionType>
  class HttpServer : private boost::noncopyable {
    public:

      //! The type of ServerConnection accepting Channels.
      using ServerConnection = GetTryDereferenceType<ServerConnectionType>;

      //! The type of Channel accepted by the ServerConnection.
      using Channel = typename ServerConnection::Channel;

      //! Constructs an HttpServer.
      /*!
        \param serverConnection Initializes the ServerConnection.
      */
      template<typename ServerConnectionForward>
      HttpServer(ServerConnectionForward&& serverConnection);

      ~HttpServer();

      void Open();

      void Close();

    private:
      GetOptionalLocalPtr<ServerConnectionType> m_serverConnection;
      Routines::RoutineHandler m_acceptRoutine;
      IO::OpenState m_openState;

      void Shutdown();
      void AcceptLoop();
  };

  template<typename ServerConnectionType>
  template<typename ServerConnectionForward>
  HttpServer<ServerConnectionType>::HttpServer(
      ServerConnectionForward&& serverConnection)
      : m_serverConnection{std::forward<ServerConnectionForward>(
          serverConnection)} {}

  template<typename ServerConnectionType>
  HttpServer<ServerConnectionType>::~HttpServer() {
    Close();
  }

  template<typename ServerConnectionType>
  void HttpServer<ServerConnectionType>::Open() {
    if(m_openState.SetOpening()) {
      return;
    }
    try {
      m_serverConnection->Open();
      m_acceptRoutine = Routines::Spawn(
        std::bind(&HttpServer::AcceptLoop, this));
    } catch(const std::exception&) {
      m_openState.SetOpenFailure();
      Shutdown();
    }
    m_openState.SetOpen();
  }

  template<typename ServerConnectionType>
  void HttpServer<ServerConnectionType>::Close() {
    if(m_openState.SetClosing()) {
      return;
    }
    Shutdown();
  }

  template<typename ServerConnectionType>
  void HttpServer<ServerConnectionType>::Shutdown() {
    m_serverConnection->Close();
    m_acceptRoutine.Wait();
    m_openState.SetClosed();
  }

  template<typename ServerConnectionType>
  void HttpServer<ServerConnectionType>::AcceptLoop() {
    SynchronizedUnorderedSet<std::shared_ptr<Channel>> clients;
    Routines::RoutineHandlerGroup clientRoutines;
    while(true) {
      std::shared_ptr<Channel> channel;
      try {
        channel = m_serverConnection->Accept();
      } catch(const IO::EndOfFileException&) {
        break;
      } catch(const std::exception&) {
        std::cout << BEAM_REPORT_CURRENT_EXCEPTION() << std::flush;
        continue;
      }
      clients.Insert(channel);
      clientRoutines.Spawn(
        [=, &clients] {
          try {
            channel->Open();
          } catch(const std::exception&) {
            std::cout << BEAM_REPORT_CURRENT_EXCEPTION() << std::flush;
          }
          try {
            HttpRequestParser parser;
            typename Channel::Reader::Buffer receiveBuffer;
            while(true) {
              channel->GetReader().Read(Store(receiveBuffer));
              receiveBuffer.Reset();
            }
          }
          clients.Erase(client);
        });
    }
    std::unordered_set<std::shared_ptr<Channel>> pendingClients;
    clients.Swap(pendingClients);
    for(auto& client : pendingClients) {
      client->Close();
    }
  }
}
}

#endif
