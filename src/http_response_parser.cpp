#include "http_response_parser.hh"

#include "deferred.h"
#include "httpp11.hh"

#include <functional>

httpp11::http_data_cb bind_data_fn(
    HttpResponseParser* p,
    bool(HttpResponseParser::* fn)(httpp11::http_parser&, std::vector<char>)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1, std::placeholders::_2);
}

httpp11::http_cb bind_fn(
        HttpResponseParser* p,
        bool(HttpResponseParser::* fn)(httpp11::http_parser&)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1);
}

HttpResponseParser::HttpResponseParser()
: parser(httpp11::http_parser_init(httpp11::http_parser_type::http_response)), settings(httpp11::http_parser_settings_init())
{
    // Safe to capture `this` as settings' lifespan is tied to `this`.
    settings->status = bind_data_fn(this, &HttpResponseParser::on_status);
    settings->header_field = bind_data_fn(this, &HttpResponseParser::on_h_field);
    settings->header_value = bind_data_fn(this, &HttpResponseParser::on_h_value);
    settings->headers_complete = bind_fn(this, &HttpResponseParser::on_headers_complete);
    settings->body = bind_data_fn(this, &HttpResponseParser::on_body);
    settings->message_complete = bind_fn(this, &HttpResponseParser::on_message_complete);

}

Deferred<> HttpResponseParser::on_data(std::vector<char>&& data) {
    httpp11::http_parser_execute(*parser, *settings, data);
    // TODO: Exception.

    return Deferred<>::make_succeeded();
}

void HttpResponseParser::on_error(Error const& err, bool fatal) {
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

bool HttpResponseParser::on_h_field(httpp11::http_parser&, std::vector<char> data) {
    if (headerState == HeaderState::FIELD) {
        std::copy(data.begin(), data.end(), std::back_inserter(header_field));
    } else {
        // We have a field, value header pair. Insert it into headers
        headers.insert({std::move(header_field), std::move(header_value)});

        headerState = HeaderState::FIELD;
        header_field = std::string(data.begin(), data.end());
    }

    return 0;
}

bool HttpResponseParser::on_h_value(httpp11::http_parser&, std::vector<char> data) {
    if (headerState == HeaderState::VALUE) {
        std::copy(data.begin(), data.end(), std::back_inserter(header_value));
    } else {
        headerState = HeaderState::VALUE;
        header_value = std::move(data);
    }

    return 0;
}

bool HttpResponseParser::on_body(httpp11::http_parser&, std::vector<char> data) {
    std::copy(data.begin(), data.end(), std::back_inserter(body));
    return 0;
}

bool HttpResponseParser::on_status(httpp11::http_parser&, std::vector<char> data) {
    std::copy(data.begin(), data.end(), std::back_inserter(status_line));
    return 0;
}

bool HttpResponseParser::on_headers_complete(httpp11::http_parser& p) {
    headers.insert({std::move(header_field), std::move(header_value)});

    headerState = HeaderState::FIELD;

    return 0;
}

bool HttpResponseParser::on_message_complete(httpp11::http_parser& p) {
    HttpResponse response{
        static_cast<std::uint16_t>(p.Get().status_code), // We know this is fine from http_parser.h
        status_line,
        HttpVersion(p.Get().http_major, p.Get().http_minor),
        std::move(headers),
        std::move(body)
    };

    // TODO
    callback(std::move(response));

    return 0;
}
