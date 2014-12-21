#pragma once

#include "deferred.h"
#include "httpp11.hh"

#include <cstdint>
#include <functional>
#include <map>
#include <vector>


namespace {
    struct ci_less : std::binary_function<std::string, std::string, bool> {
        struct nocase_compare : public std::binary_function<unsigned char, unsigned char, bool> {
            bool operator() (const unsigned char& c1, const unsigned char& c2) const {
                return tolower (c1) < tolower (c2);
            }
        };

        bool operator() (const std::string & s1, const std::string & s2) const {
            return std::lexicographical_compare(
                    s1.begin (), s1.end (),
                    s2.begin (), s2.end (),
                    nocase_compare ()
            );
        }
    };
}


using HttpHeaders = std::multimap<std::string, std::vector<char>, ci_less>;


struct HttpVersion : public std::tuple<std::uint8_t, std::uint8_t> {
    using std::tuple<std::uint8_t, std::uint8_t>::tuple;

    std::uint8_t const& major() const { return std::get<0>(*this); }
    std::uint8_t & major() { return std::get<0>(*this); }

    std::uint8_t const& minor() const { return std::get<1>(*this); }
    std::uint8_t & minor() { return std::get<1>(*this); }
};


struct HttpMessage {
    HttpVersion version;

    HttpHeaders headers;

    std::vector<char> body;
};


struct HttpResponse : public HttpMessage {
    std::uint16_t status;
    std::vector<char> reason_phrase;
};

struct HttpRequest : public HttpMessage {
    std::string method;
    std::vector<char> url;
};



class HttpParser {
public:
    HttpParser(httpp11::http_parser_type);

    virtual ~HttpParser();

    // Callbacks from the TCP source.
    void on_data(std::vector<char>&&);
    void on_close();

    // http11 callbacks
    virtual bool on_message_begin(httpp11::http_parser&);
    virtual bool on_url(httpp11::http_parser&, std::vector<char>) { return 0; }
    virtual bool on_status(httpp11::http_parser&, std::vector<char>) { return 0; }
    bool on_h_field(httpp11::http_parser&, std::vector<char>);
    bool on_h_value(httpp11::http_parser&, std::vector<char>);
    bool on_headers_complete(httpp11::http_parser&);
    bool on_body(httpp11::http_parser&, std::vector<char>);

    virtual bool on_message_complete(httpp11::http_parser&) = 0;

protected:
    httpp11::unique_http_parser parser;
    httpp11::unique_http_parser_settings settings;

    std::vector<char> body;

    enum class HeaderState {FIELD, VALUE};
    HeaderState headerState = HeaderState::FIELD;

    std::string header_field;
    std::vector<char> header_value;

    HttpHeaders headers;
};


class HttpResponseParser : public HttpParser {
public:
    HttpResponseParser();
    virtual ~HttpResponseParser();

    virtual bool on_message_begin(httpp11::http_parser&);
    virtual bool on_status(httpp11::http_parser&, std::vector<char>);

    virtual bool on_message_complete(httpp11::http_parser&);

    std::function<void(HttpResponse&&)> callback;
protected:
    std::vector<char> reason_phrase;
};


class HttpRequestParser : public HttpParser {
public:
    HttpRequestParser();
    virtual ~HttpRequestParser();

    virtual bool on_message_begin(httpp11::http_parser&);
    virtual bool on_url(httpp11::http_parser&, std::vector<char>);

    virtual bool on_message_complete(httpp11::http_parser&);

    std::function<void(HttpRequest&&)> callback;
protected:
    std::vector<char> url;
};
