#pragma once

#include "http_parser.h"

#include "buffer_view.hh"

#include <functional>
#include <memory>
#include <system_error>

namespace httpp11 {
    using ::http_parser;
    using ::http_parser_settings;
    using ::http_method;

    using Error = std::error_condition;

    enum class ParserType : int {
        Request = HTTP_REQUEST,
        Response = HTTP_RESPONSE,
        Both = HTTP_BOTH
    };

    class Parser;

    // Callbacks
    using DataCallback = std::function<bool(Parser&, BufferView const&)>;
    using EventCallback = std::function<bool(Parser&)>;

    class Settings {
    public:
        Settings();

        http_parser_settings& Get() { return settings; }
        http_parser_settings const& Get() const { return settings; }

        EventCallback message_begin;
        DataCallback url;
        DataCallback status;
        DataCallback header_field;
        DataCallback header_value;
        EventCallback headers_complete;
        DataCallback body;
        EventCallback message_complete;
    private:
        http_parser_settings settings;
    };


    class Parser {
    public:
        Parser(ParserType);

        http_parser& Get() { return parser; }
        http_parser const& Get() const { return parser; }

        Settings* settings; // Only valid during callbacks.
    private:
        http_parser parser;
    };


    Error http_parser_execute(Parser&, Settings&, BufferView const&);

    bool http_should_keep_alive(Parser const& parser);

    void http_parser_pause(Parser& parser, bool paused);

    bool http_body_is_final(Parser const& parser);

    std::string http_method_str(Parser const& parser);
    std::string http_method_str(enum http_method m);

    std::error_category const& httpp11_error_category();

} // httpp11
