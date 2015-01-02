#include "httpp11.hh"

#include <sstream>

using namespace httpp11;


class : public std::error_category {
public:
    const char* name() const noexcept { return "http_parser"; }

    std::string message(int ev) const {
        if (ev == 0) return "Success";

        std::string name = ::http_errno_name(http_errno(ev));
        std::string description = ::http_errno_name(http_errno(ev));

        std::stringstream ss; ss << name << ": " << description;
        return ss.str();
    }
} static http_error_category;


std::error_category const& httpp11::httpp11_error_category() {
    return http_error_category;
}

static std::error_condition make_error(int err) {
    return std::error_condition(err, http_error_category);
}


using SettingsEventCallback = EventCallback Settings::*;
template<SettingsEventCallback cb_ptr>
static ::http_cb create_event_callback() {
    return [] (http_parser* parser) -> int {
        Parser* p = reinterpret_cast<Parser*>(parser->data);
        return (p->settings->*cb_ptr)(*p);
    };
}

using SettingsDataCallback = DataCallback Settings::*;
template<SettingsDataCallback cb_ptr>
static ::http_data_cb create_data_callback() {
    return [] (http_parser* parser, const char *at, size_t length) -> int {
        Parser* p = reinterpret_cast<Parser*>(parser->data);
        return (p->settings->*cb_ptr)(*p, BufferView(at, length));
    };
}


Settings::Settings() {
    settings.on_message_begin = create_event_callback<&Settings::message_begin>();
    settings.on_url = create_data_callback<&Settings::url>();
    settings.on_status = create_data_callback<&Settings::status>();
    settings.on_header_field = create_data_callback<&Settings::header_field>();
    settings.on_header_value = create_data_callback<&Settings::header_value>();
    settings.on_headers_complete = create_event_callback<&Settings::headers_complete>();
    settings.on_body = create_data_callback<&Settings::body>();
    settings.on_message_complete = create_event_callback<&Settings::message_complete>();
}


Parser::Parser(ParserType type) {
    ::http_parser_init(&this->Get(), ::http_parser_type(static_cast<int>(type)));
    parser.data = this;
}



Error httpp11::http_parser_execute(Parser& parser, Settings& settings, BufferView const& data) {
    parser.settings = &settings;
    auto s = ::http_parser_execute(&parser.Get(), &settings.Get(), data.data(), data.size());
    parser.settings = nullptr;

    if (s != data.size()) {
        return make_error(parser.Get().http_errno);
    } else {
        return make_error(0);
    }
}

bool httpp11::http_should_keep_alive(Parser const& parser) {
    return ::http_should_keep_alive(&parser.Get()) > 0;
}

void httpp11::http_parser_pause(Parser& parser, bool paused) {
    ::http_parser_pause(&parser.Get(), paused ? 1 : 0);
}

bool httpp11::http_body_is_final(Parser const& parser) {
    return ::http_body_is_final(&parser.Get()) > 0;
}



std::string httpp11::http_method_str(Parser const& parser) {
    return httpp11::http_method_str(static_cast<http_method>(parser.Get().method));
}

std::string httpp11::http_method_str(enum http_method m) {
    return ::http_method_str(m);
}
