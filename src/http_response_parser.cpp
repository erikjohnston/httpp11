#include "http_response_parser.hh"

#include "deferred.h"
#include "http11.hh"

#include <functional>

http11::http_data_cb bind_data_fn(
    HttpResponseParser* p,
    bool(HttpResponseParser::* fn)(httpp11::http_parser&, std::vector<char>)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1, std::placeholders::_2);
}

http11::http_cb bind_fn(
        HttpResponseParser* p,
        bool(HttpResponseParser::* fn)(httpp11::http_parser&)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1);
}

HttpResponseParser::HttpResponseParser()
: parser(httpp11::http_parser_init(httpp11::http_parser_type::http_response),
  settings(httpp11::http_parser_settings_init()))
{
    // Safe to capture `this` as settings' lifespan is tied to `this`.
    settings->body = bind_data_fn(this, &HttpResponseParser::on_data);
    settings->message_complete = bind_fn(this, &HttpResponseParser::on_message_complete);

}

Deferred<> HttpResponseParser::on_data(std::vector<char>&& data) {
    httpp11::http_parser_execute(*parser, *settings, data);
    // TODO: Exception.
}

void HttpResponseParser::on_error(typename Error const& err, bool fatal) {
    // TODO
    if (fatal) {
        on_close();
    }
}

void HttpResponseParser::on_close() {
    // TODO

    // Signal EOF to http11
    httpp11::http_parser_execute(*parser, *settings, std::vector<char>());
}

bool HttpResponseParser::on_body(http11::http_parser&, std::vector<char>) {
    std::copy(data.begin(), data.end(), std::back_inserter(body));
}

bool HttpResponseParser::on_message_complete(http11::http_parser&) {
    HttpResponse response{status, status_line, version, std::move(body)};

    // TODO
    callback(std::move(response));
}