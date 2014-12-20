#include "httpp11.hh"


#include <sstream>
#include <system_error>

class http_error_category_t : public std::error_category {
public:
    const char* name() const noexcept { return "http_parser error."; };

    std::string message(int ev) const {
        std::string name = ::http_errno_name(http_errno(ev));
        std::string description = ::http_errno_name(http_errno(ev));

        std::stringstream ss; ss << name << ": " << description;
        return ss.str();
    }
} http_error_category;

std::error_condition make_error_condition(enum http_errno err) {
    return std::error_condition(err, http_error_category);
}

httpp11::http_error make_http_error(enum http_errno err) {
    return httpp11::http_error(make_error_condition(err));
}

httpp11::unique_http_parser httpp11::http_parser_init(httpp11::http_parser_type type) {
    httpp11::unique_http_parser parser(new httpp11::http_parser());

    ::http_parser_init(&parser->Get(), ::http_parser_type(static_cast<int>(type)));

    return parser;
}

void httpp11::http_parser_execute(httpp11::http_parser& parser, httpp11::http_parser_settings& settings,
        std::vector<char> const& data) throw(httpp11::http_error) {
    settings.setup_callbacks(parser);

    auto nparsed = ::http_parser_execute(&parser.Get(), &settings.Get(), data.data(), data.size());

    settings.cleanup_callbacks(parser);

    if (nparsed != data.size()) {
        throw http_error(make_error_condition(http_errno(parser.Get().http_errno)));
    }
}

bool httpp11::http_should_keep_alive(httpp11::http_parser const& parser) {
    return ::http_should_keep_alive(&parser.Get()) > 0;
}

void httpp11::http_parser_pause(httpp11::http_parser& parser, bool paused) {
    ::http_parser_pause(&parser.Get(), paused ? 1 : 0);
}

bool httpp11::http_body_is_final(http_parser const& parser) {
    return ::http_body_is_final(&parser.Get()) > 0;
}


httpp11::unique_http_parser_settings httpp11::http_parser_settings_init() {
    return httpp11::unique_http_parser_settings(new httpp11::http_parser_settings());
}




template <httpp11::http_cb (httpp11::http_parser_settings::* func)>
int http_callback_templ(::http_parser* parser) {
    auto* context = static_cast<httpp11::HttpContext*>(parser->data);
    return (context->settings.*func)(context->parser) ? 1 : 0;
}

template<httpp11::http_data_cb (httpp11::http_parser_settings::* func)>
int http_data_callback_templ(::http_parser* parser, const char* c, ::size_t len) {
    auto* context = static_cast<httpp11::HttpContext*>(parser->data);
    std::vector<char> data(c, c + len);
    return (context->settings.*func)(context->parser, data) ? 1 : 0;
}

template <httpp11::http_cb (httpp11::http_parser_settings::* funcpp)>
void register_cb(httpp11::http_parser_settings& s, ::http_cb (::http_parser_settings::* cfunc)) {
    if (s.*funcpp) {
        s.Get().*cfunc = &(http_callback_templ<funcpp>);
    } else {
        s.Get().*cfunc = nullptr;
    }
}

template <httpp11::http_data_cb (httpp11::http_parser_settings::* funcpp)>
void register_data_cb(httpp11::http_parser_settings& s, ::http_data_cb (::http_parser_settings::* cfunc)) {
    if (s.*funcpp) {
        s.Get().*cfunc = &(http_data_callback_templ<funcpp>);
    } else {
        s.Get().*cfunc = nullptr;
    }
}


void httpp11::http_parser_settings::setup_callbacks(httpp11::http_parser & parser) {
    context.reset(new httpp11::HttpContext(parser, *this));

    parser.Get().data = context.get();

    register_cb<&http_parser_settings::message_begin>(*this, &::http_parser_settings::on_message_begin);
    register_data_cb<&http_parser_settings::url>(*this, &::http_parser_settings::on_url);
    register_data_cb<&http_parser_settings::status>(*this, &::http_parser_settings::on_status);
    register_data_cb<&http_parser_settings::header_field>(*this, &::http_parser_settings::on_header_field);
    register_data_cb<&http_parser_settings::header_value>(*this, &::http_parser_settings::on_header_value);
    register_cb<&http_parser_settings::headers_complete>(*this, &::http_parser_settings::on_headers_complete);
    register_data_cb<&http_parser_settings::body>(*this, &::http_parser_settings::on_body);
    register_cb<&http_parser_settings::message_complete>(*this, &::http_parser_settings::on_message_complete);
}

void httpp11::http_parser_settings::cleanup_callbacks(httpp11::http_parser & parser) {
    context.reset();
    parser.Get().data = nullptr;

    Get().on_message_begin = nullptr;
    Get().on_url = nullptr;
    Get().on_status = nullptr;
    Get().on_header_field = nullptr;
    Get().on_header_value = nullptr;
    Get().on_headers_complete = nullptr;
    Get().on_body = nullptr;
    Get().on_message_complete = nullptr;

}
