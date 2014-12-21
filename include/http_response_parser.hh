#pragma once

#include "deferred.h"
#include "httpp11.hh"
#include "sink.hh"

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


struct HttpResponse {
    std::uint16_t status;
    std::vector<char> status_line;
    HttpVersion version;

    HttpHeaders headers;

    std::vector<char> body;
};


class HttpResponseParser : public Sink<std::vector<char>> {
public:
    HttpResponseParser();

    // Callbacks from the TCP source.
    Deferred<> on_data(std::vector<char>&&);
    void on_error(Error const&, bool fatal);
    void on_close();

    // http11 callbacks
    bool on_h_field(httpp11::http_parser&, std::vector<char>);
    bool on_h_value(httpp11::http_parser&, std::vector<char>);
    bool on_headers_complete(httpp11::http_parser&);
    bool on_body(httpp11::http_parser&, std::vector<char>);
    bool on_status(httpp11::http_parser&, std::vector<char>);
    bool on_message_complete(httpp11::http_parser&);

    std::function<void(HttpResponse&&)> callback;

private:
    httpp11::unique_http_parser parser;
    httpp11::unique_http_parser_settings settings;

    std::vector<char> body;
    std::vector<char> status_line;

    enum class HeaderState {FIELD, VALUE};
    HeaderState headerState = HeaderState::FIELD;

    std::string header_field;
    std::vector<char> header_value;

    HttpHeaders headers;
};
