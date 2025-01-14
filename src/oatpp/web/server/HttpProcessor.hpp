/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#ifndef oatpp_web_server_HttpProcessor_hpp
#define oatpp_web_server_HttpProcessor_hpp

#include "./HttpRouter.hpp"

#include "./handler/Interceptor.hpp"
#include "./handler/ErrorHandler.hpp"

#include "oatpp/web/protocol/http/encoding/ProviderCollection.hpp"

#include "oatpp/web/protocol/http/incoming/RequestHeadersReader.hpp"
#include "oatpp/web/protocol/http/incoming/Request.hpp"

#include "oatpp/web/protocol/http/outgoing/Response.hpp"
#include "oatpp/web/protocol/http/utils/CommunicationUtils.hpp"

#include "oatpp/core/data/stream/StreamBufferedProxy.hpp"
#include "oatpp/core/async/Processor.hpp"

namespace oatpp { namespace web { namespace server {

/**
 * HttpProcessor. Helper class to handle HTTP processing.
 */
class HttpProcessor {
public:
  typedef oatpp::collection::LinkedList<std::shared_ptr<oatpp::web::server::handler::RequestInterceptor>> RequestInterceptors;
  typedef oatpp::web::protocol::http::incoming::RequestHeadersReader RequestHeadersReader;
public:

  /**
   * Resource config per connection.
   */
  struct Config {

    /**
     * Buffer used to read headers in request. Initial size of the buffer.
     */
    v_buff_size headersInBufferInitial = 2048;

    /**
     * Buffer used to write headers in response. Initial size of the buffer.
     */
    v_buff_size headersOutBufferInitial = 2048;

    /**
     * Size of the chunk used for iterative-read of headers.
     */
    v_buff_size headersReaderChunkSize = 2048;

    /**
     * Maximum allowed size of requests headers. The overall size of all headers in the request.
     */
    v_buff_size headersReaderMaxSize = 4096;

  };

public:

  /**
   * Collection of components needed to serve http-connection.
   */
  struct Components {

    /**
     * Constructor.
     * @param pRouter
     * @param pContentEncodingProviders
     * @param pBodyDecoder
     * @param pErrorHandler
     * @param pRequestInterceptors
     * @param pConfig
     */
    Components(const std::shared_ptr<HttpRouter>& pRouter,
               const std::shared_ptr<protocol::http::encoding::ProviderCollection>& pContentEncodingProviders,
               const std::shared_ptr<const oatpp::web::protocol::http::incoming::BodyDecoder>& pBodyDecoder,
               const std::shared_ptr<handler::ErrorHandler>& pErrorHandler,
               const std::shared_ptr<RequestInterceptors>& pRequestInterceptors,
               const std::shared_ptr<Config>& pConfig);

    /**
     * Constructor.
     * @param pRouter
     */
    Components(const std::shared_ptr<HttpRouter>& pRouter);

    /**
     * Constructor.
     * @param pRouter
     * @param pConfig
     */
    Components(const std::shared_ptr<HttpRouter>& pRouter, const std::shared_ptr<Config>& pConfig);

    /**
     * Router to route incoming requests. &id:oatpp::web::server::HttpRouter;.
     */
    std::shared_ptr<HttpRouter> router;

    /**
     * Content-encoding providers. &id:oatpp::web::protocol::encoding::ProviderCollection;.
     */
    std::shared_ptr<protocol::http::encoding::ProviderCollection> contentEncodingProviders;

    /**
     * Body decoder. &id:oatpp::web::protocol::http::incoming::BodyDecoder;.
     */
    std::shared_ptr<const oatpp::web::protocol::http::incoming::BodyDecoder> bodyDecoder;

    /**
     * Error handler. &id:oatpp::web::server::handler::ErrorHandler;.
     */
    std::shared_ptr<handler::ErrorHandler> errorHandler;

    /**
     * Collection of request interceptors. &id:oatpp::web::server::handler::RequestInterceptor;.
     */
    std::shared_ptr<RequestInterceptors> requestInterceptors;

    /**
     * Resource allocation config. &l:HttpProcessor::Config;.
     */
    std::shared_ptr<Config> config;

  };

private:

  struct ProcessingResources {

    ProcessingResources(const std::shared_ptr<Components>& pComponents,
                        const std::shared_ptr<oatpp::data::stream::IOStream>& pConnection);

    std::shared_ptr<Components> components;
    std::shared_ptr<oatpp::data::stream::IOStream> connection;
    oatpp::data::stream::BufferOutputStream headersInBuffer;
    oatpp::data::stream::BufferOutputStream headersOutBuffer;
    RequestHeadersReader headersReader;
    std::shared_ptr<oatpp::data::stream::InputStreamBufferedProxy> inStream;

  };

  static bool processNextRequest(ProcessingResources& resources);

public:

  /**
   * Connection serving task. <br>
   * Usege example: <br>
   * `std::thread thread(&HttpProcessor::Task::run, HttpProcessor::Task(components, connection));`
   */
  class Task : public base::Countable {
  private:
    std::shared_ptr<Components> m_components;
    std::shared_ptr<oatpp::data::stream::IOStream> m_connection;
  public:

    /**
     * Constructor.
     * @param components - &l:HttpProcessor::Components;.
     * @param connection - &id:oatpp::data::stream::IOStream;.
     */
    Task(const std::shared_ptr<Components>& components,
         const std::shared_ptr<oatpp::data::stream::IOStream>& connection);
  public:

    /**
     * Run loop.
     */
    void run();

  };
  
public:

  /**
   * Connection serving coroutiner - &id:oatpp::async::Coroutine;.
   */
  class Coroutine : public oatpp::async::Coroutine<HttpProcessor::Coroutine> {
  private:
    std::shared_ptr<Components> m_components;
    std::shared_ptr<oatpp::data::stream::IOStream> m_connection;
    oatpp::data::stream::BufferOutputStream m_headersInBuffer;
    RequestHeadersReader m_headersReader;
    std::shared_ptr<oatpp::data::stream::BufferOutputStream> m_headersOutBuffer;
    std::shared_ptr<oatpp::data::stream::InputStreamBufferedProxy> m_inStream;
    v_int32 m_connectionState;
  private:
    oatpp::web::server::HttpRouter::BranchRouter::Route m_currentRoute;
    std::shared_ptr<protocol::http::incoming::Request> m_currentRequest;
    std::shared_ptr<protocol::http::outgoing::Response> m_currentResponse;
  public:


    /**
     * Constructor.
     * @param components - &l:HttpProcessor::Components;.
     * @param connection - &id:oatpp::data::stream::IOStream;.
     */
    Coroutine(const std::shared_ptr<Components>& components,
              const std::shared_ptr<oatpp::data::stream::IOStream>& connection);
    
    Action act() override;

    Action parseHeaders();
    
    Action onHeadersParsed(const RequestHeadersReader::Result& headersReadResult);
    
    Action onRequestFormed();
    Action onResponse(const std::shared_ptr<protocol::http::outgoing::Response>& response);
    Action onResponseFormed();
    Action onRequestDone();
    
    Action handleError(Error* error) override;
    
  };
  
};
  
}}}

#endif /* oatpp_web_server_HttpProcessor_hpp */
