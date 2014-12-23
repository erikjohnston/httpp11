#include "http_response_parser.hh"

#include "deferred.h"
#include "httpp11.hh"

#include <functional>

static httpp11::http_data_cb bind_data_fn(
    HttpParser* p,
    bool(HttpParser::* fn)(httpp11::http_parser&, BufferView const&)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1, std::placeholders::_2);
}

static httpp11::http_cb bind_fn(
    HttpParser* p,
    bool(HttpParser::* fn)(httpp11::http_parser&)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1);
}

HttpParser::HttpParser(httpp11::http_parser_type type)
: parser(httpp11::http_parser_init(type)), settings(httpp11::http_parser_settings_init())
{
    // Safe to capture `this` as settings' lifespan is tied to `this`.
    settings->message_begin = bind_fn(this, &HttpParser::on_message_begin);
    settings->url = bind_data_fn(this, &HttpParser::on_url);
    settings->status = bind_data_fn(this, &HttpParser::on_status);
    settings->header_field = bind_data_fn(this, &HttpParser::on_h_field);
    settings->header_value = bind_data_fn(this, &HttpParser::on_h_value);
    settings->headers_complete = bind_fn(this, &HttpParser::on_headers_complete);
    settings->body = bind_data_fn(this, &HttpParser::on_body);
    settings->message_complete = bind_fn(this, &HttpParser::on_message_complete);

}

HttpParser::~HttpParser() {}

void HttpParser::on_data(std::vector<char>&& data) {
    if (data.size() > 0) {
        httpp11::http_parser_execute(*parser, *settings, data);
        // TODO: Exception.
    }
}

void HttpParser::on_close() {
    // TODO

    // Signal EOF to http11
    httpp11::http_parser_execute(*parser, *settings, BufferView(nullptr, 0));
}

bool HttpParser::on_message_begin(httpp11::http_parser&) {
    headerState = HeaderState::FIELD;
    body.clear();
    header_field.clear();
    header_value.clear();
    headers.clear();

    return 0;
}


bool HttpParser::on_h_field(httpp11::http_parser&, BufferView const& data) {
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

bool HttpParser::on_h_value(httpp11::http_parser&, BufferView const& data) {
    if (headerState == HeaderState::VALUE) {
        std::copy(data.begin(), data.end(), std::back_inserter(header_value));
    } else {
        headerState = HeaderState::VALUE;
        header_value = std::vector<char>(data.begin(), data.end());
    }

    return 0;
}

bool HttpParser::on_body(httpp11::http_parser&, BufferView const& data) {
    std::copy(data.begin(), data.end(), std::back_inserter(body));
    return 0;
}

bool HttpParser::on_headers_complete(httpp11::http_parser&) {
    headers.insert({std::move(header_field), std::move(header_value)});

    headerState = HeaderState::FIELD;

    return 0;
}


HttpResponseParser::HttpResponseParser() : HttpParser(httpp11::http_parser_type::http_response) {}
HttpResponseParser::~HttpResponseParser() {}

bool HttpResponseParser::on_message_begin(httpp11::http_parser& p) {
    reason_phrase.clear();

    return HttpParser::on_message_begin(p);
}

bool HttpResponseParser::on_status(httpp11::http_parser&, BufferView const& data) {
    std::copy(data.begin(), data.end(), std::back_inserter(reason_phrase));
    return 0;
}

bool HttpResponseParser::on_message_complete(httpp11::http_parser& p) {
    HttpResponse response;
    response.status = static_cast<std::uint16_t>(p.Get().status_code); // We know this is fine from http_parser.h
    response.reason_phrase = reason_phrase;
    response.version = HttpVersion(p.Get().http_major, p.Get().http_minor);
    response.headers = std::move(headers);
    response.body = std::move(body);

    // TODO
    callback(std::move(response));

    return 0;
}

HttpRequestParser::HttpRequestParser() : HttpParser(httpp11::http_parser_type::http_request) {}
HttpRequestParser::~HttpRequestParser() {}

bool HttpRequestParser::on_message_begin(httpp11::http_parser& p) {
    url.clear();

    return HttpParser::on_message_begin(p);
}

bool HttpRequestParser::on_url(httpp11::http_parser&, BufferView const& data) {
    std::copy(data.begin(), data.end(), std::back_inserter(url));
    return 0;
}

bool HttpRequestParser::on_message_complete(httpp11::http_parser& p) {
    HttpRequest request;
    request.method = httpp11::http_method_str(p);
    request.url = url;
    request.version = HttpVersion(p.Get().http_major, p.Get().http_minor);
    request.headers = std::move(headers);
    request.body = std::move(body);

    // TODO
    callback(std::move(request));

    return 0;
}
