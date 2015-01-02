#include "collators.hh"

using namespace httpp11;

template<typename T>
static httpp11::DataCallback bind_data_fn(
    T* p,
    bool(T::* fn)(httpp11::Parser&, BufferView const&)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1, std::placeholders::_2);
}

template<typename T>
static httpp11::EventCallback bind_event_fn(
    T* p,
    bool(T::* fn)(httpp11::Parser&)
) {
    return std::bind(std::mem_fn(fn), p, std::placeholders::_1);
}


HttpMessageCollator::HttpMessageCollator() {
    message_begin = bind_event_fn(this, &HttpMessageCollator::on_message_begin);
    url = bind_data_fn(this, &HttpMessageCollator::on_url);
    status = bind_data_fn(this, &HttpMessageCollator::on_status);
    header_field = bind_data_fn(this, &HttpMessageCollator::on_h_field);
    header_value = bind_data_fn(this, &HttpMessageCollator::on_h_value);
    headers_complete = bind_event_fn(this, &HttpMessageCollator::on_headers_complete);
    body = bind_data_fn(this, &HttpMessageCollator::on_body);
    message_complete = bind_event_fn(this, &HttpMessageCollator::on_message_complete);
}

HttpMessageCollator::~HttpMessageCollator() {}

bool HttpMessageCollator::on_message_begin(httpp11::Parser&) {
    headerState = HeaderState::FIELD;
    body_buffer.clear();
    header_field_buffer.clear();
    header_value_buffer.clear();
    headers.clear();

    return 0;
}


bool HttpMessageCollator::on_h_field(httpp11::Parser&, BufferView const& data) {
    if (headerState == HeaderState::FIELD) {
        std::copy(data.begin(), data.end(), std::back_inserter(header_field_buffer));
    } else {
        // We have a field, value header pair. Insert it into headers
        headers.insert({std::move(header_field_buffer), std::move(header_value_buffer)});

        headerState = HeaderState::FIELD;
        header_field_buffer = std::string(data.begin(), data.end());
    }

    return 0;
}

bool HttpMessageCollator::on_h_value(httpp11::Parser&, BufferView const& data) {
    if (headerState == HeaderState::VALUE) {
        std::copy(data.begin(), data.end(), std::back_inserter(header_value_buffer));
    } else {
        headerState = HeaderState::VALUE;
        header_value_buffer = std::vector<char>(data.begin(), data.end());
    }

    return 0;
}

bool HttpMessageCollator::on_body(httpp11::Parser&, BufferView const& data) {
    std::copy(data.begin(), data.end(), std::back_inserter(body_buffer));
    return 0;
}

bool HttpMessageCollator::on_headers_complete(httpp11::Parser&) {
    headers.insert({std::move(header_field_buffer), std::move(header_value_buffer)});

    headerState = HeaderState::FIELD;

    return 0;
}


HttpResponseCollator::HttpResponseCollator() {}
HttpResponseCollator::~HttpResponseCollator() {}

bool HttpResponseCollator::on_message_begin(httpp11::Parser& p) {
    reason_phrase.clear();

    return HttpMessageCollator::on_message_begin(p);
}

bool HttpResponseCollator::on_status(httpp11::Parser&, BufferView const& data) {
    std::copy(data.begin(), data.end(), std::back_inserter(reason_phrase));
    return 0;
}

bool HttpResponseCollator::on_message_complete(httpp11::Parser& p) {
    HttpResponse response;
    response.status = static_cast<std::uint16_t>(p.Get().status_code); // We know this is fine from http_parser.h
    response.reason_phrase = reason_phrase;
    response.version = HttpVersion(p.Get().http_major, p.Get().http_minor);
    response.headers = std::move(headers);
    response.body = std::move(body_buffer);

    // TODO
    callback(std::move(response));

    return 0;
}

HttpRequestCollator::HttpRequestCollator() {}
HttpRequestCollator::~HttpRequestCollator() {}

bool HttpRequestCollator::on_message_begin(httpp11::Parser& p) {
    url.clear();

    return HttpMessageCollator::on_message_begin(p);
}

bool HttpRequestCollator::on_url(httpp11::Parser&, BufferView const& data) {
    std::copy(data.begin(), data.end(), std::back_inserter(url));
    return 0;
}

bool HttpRequestCollator::on_message_complete(httpp11::Parser& p) {
    HttpRequest request;
    request.method = httpp11::method_str(p);
    request.url = url;
    request.version = HttpVersion(p.Get().http_major, p.Get().http_minor);
    request.headers = std::move(headers);
    request.body = std::move(body_buffer);

    // TODO
    callback(std::move(request));

    return 0;
}


HttpRequestParser::HttpRequestParser(std::function<void(HttpRequest&&)> const& cb)
    : parser(ParserType::Request)
{
    request_collator.callback = cb;
}

Error HttpRequestParser::on_data(BufferView const& data) {
    return execute(parser, request_collator, data);
}

HttpResponseParser::HttpResponseParser(std::function<void(HttpResponse&&)> const& cb)
    : parser(ParserType::Response)
{
    response_collator.callback = cb;
}

Error HttpResponseParser::on_data(BufferView const& data) {
    return execute(parser, response_collator, data);
}
