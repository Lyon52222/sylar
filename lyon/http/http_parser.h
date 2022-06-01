#ifndef __LYON_HTTP_HTTP_PARSER_H__
#define __LYON_HTTP_HTTP_PARSER_H__

#include "http11_common.h"
#include "http11_parser.h"
#include "http_protocol.h"
#include "httpclient_parser.h"
#include <cstdint>
#include <memory>
namespace lyon {
namespace http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    int finish();
    size_t excute(const char *data, size_t len, size_t offset);
    int hasError();
    int isFinish();
    HttpRequest::ptr getData() const { return m_data; }
    uint64_t getContentLength() const;

    static uint64_t GetBufferSize();
    static uint64_t GetMaxBodySize();

private:
    HttpRequest::ptr m_data;
    http_parser m_parser;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    int finish();
    int excute(const char *data, size_t len, size_t offset);
    int hasError();
    int isFinish();
    HttpResponse::ptr getData() const { return m_data; }
    uint64_t getContentLength() const;

    static uint64_t GetBufferSize();
    static uint64_t GetMaxBodySize();

private:
    HttpResponse::ptr m_data;
    httpclient_parser m_parser;
};

} // namespace http

} // namespace lyon

#endif // !__LYON_HTTP_HTTP_PARSER_H__
