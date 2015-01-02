#pragma once

#include "buffer_view.hh"
#include "httpp11.hh"

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace httpp11 {
    namespace {
        struct ci_less : std::binary_function<std::string, std::string, bool> {
            struct nocase_compare : public std::binary_function<unsigned char, unsigned char, bool> {
                bool operator()(const unsigned char &c1, const unsigned char &c2) const {
                    return tolower(c1) < tolower(c2);
                }
            };

            bool operator()(const std::string &s1, const std::string &s2) const {
                return std::lexicographical_compare(
                    s1.begin(), s1.end(),
                    s2.begin(), s2.end(),
                    nocase_compare()
                );
            }
        };
    }


    using HttpHeaders = std::multimap<std::string, std::vector<char>, ci_less>;


    struct HttpVersion : public std::tuple<std::uint8_t, std::uint8_t> {
        using std::tuple<std::uint8_t, std::uint8_t>::tuple;

        std::uint8_t const &major() const {
            return std::get<0>(*this);
        }

        std::uint8_t &major() {
            return std::get<0>(*this);
        }

        std::uint8_t const &minor() const {
            return std::get<1>(*this);
        }

        std::uint8_t &minor() {
            return std::get<1>(*this);
        }
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


    class HttpMessageCollator : public httpp11::Settings {
    public:
        HttpMessageCollator();

        virtual ~HttpMessageCollator();

    protected:
        // http11 callbacks
        virtual bool on_message_begin(httpp11::Parser &);

        virtual bool on_url(httpp11::Parser &, BufferView const &) {
            return 0;
        }

        virtual bool on_status(httpp11::Parser &, BufferView const &) {
            return 0;
        }

        bool on_h_field(httpp11::Parser &, BufferView const &);

        bool on_h_value(httpp11::Parser &, BufferView const &);

        bool on_headers_complete(httpp11::Parser &);

        bool on_body(httpp11::Parser &, BufferView const &);

        virtual bool on_message_complete(httpp11::Parser &) = 0;

        std::vector<char> body_buffer;

        HttpHeaders headers;

    private:
        enum class HeaderState {
            FIELD, VALUE
        };
        HeaderState headerState = HeaderState::FIELD;

        std::string header_field_buffer;
        std::vector<char> header_value_buffer;
    };


    class HttpResponseCollator : public HttpMessageCollator {
    public:
        HttpResponseCollator();

        virtual ~HttpResponseCollator();

        std::function<void(HttpResponse &&)> callback;
    protected:
        virtual bool on_message_begin(httpp11::Parser &);

        virtual bool on_status(httpp11::Parser &, BufferView const &);

        virtual bool on_message_complete(httpp11::Parser &);

        std::vector<char> reason_phrase;
    };


    class HttpRequestCollator : public HttpMessageCollator {
    public:
        HttpRequestCollator();

        virtual ~HttpRequestCollator();

        std::function<void(HttpRequest &&)> callback;
    protected:
        virtual bool on_message_begin(httpp11::Parser &);

        virtual bool on_url(httpp11::Parser &, BufferView const &);

        virtual bool on_message_complete(httpp11::Parser &);

        std::vector<char> url;
    };

    class HttpRequestParser {
    public:
        HttpRequestParser(std::function<void(HttpRequest&&)> const&);

        Error on_data(BufferView const&);
    private:
        Parser parser;
        HttpRequestCollator request_collator;
    };

    class HttpResponseParser {
    public:
        HttpResponseParser(std::function<void(HttpResponse&&)> const&);

        Error on_data(BufferView const&);
    private:
        Parser parser;
        HttpResponseCollator response_collator;
    };
}
