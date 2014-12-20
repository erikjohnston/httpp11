#pragma once

#include "deferred.h"
#include "httpp11.hh"
#include "sink.hh"

#include <cstdint>

struct HttpVersion : public std::tuple<std::uint8_t, std::uint8_t> {
    using std::tuple::tuple;

    std::unit8_t const& major() const { return std::get<0>(*this); }
    std::unit8_t & major() { return std::get<0>(*this); }

    std::unit8_t const& minor() const { return std::get<1>(*this); }
    td::unit8_t & minor() { return std::get<1>(*this); }
};


struct HttpResponse {
    std::uint8_t status;
    std::string status_line;
    HttpVersion version;

    // TODO: Headers

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
    bool on_body(http11::http_parser&, std::vector<char>);
    bool on_message_complete(http11::http_parser&);

    std::function<void(HttpResponse&&)> callback;

private:
    httpp11::unique_http_parser parser;
    httpp11::unique_http_parser_settings settings;

    std::vector<char> body;

};
