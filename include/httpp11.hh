#pragma once

#include "http_parser.h"

#include <functional>
#include <memory>
#include <vector>
#include <system_error>

namespace httpp11 {
    // ::http_parser_type
    enum class http_parser_type : int {
        http_request = HTTP_REQUEST,
        http_response = HTTP_RESPONSE,
        http_both = HTTP_BOTH
    };

    class http_parser {
    public:
        ::http_parser& Get() { return parser; }
        ::http_parser const& Get() const { return parser; }
    private:
        ::http_parser parser;
    };

    using unique_http_parser = std::unique_ptr<http_parser>;


    // Callbacks
    using http_data_cb = std::function<bool(http_parser&, std::vector<char>)>;
    using http_cb = std::function<bool(http_parser&)>;

    struct HttpContext;

    class http_parser_settings {
    public:
        http_cb message_begin;
        http_data_cb url;
        http_data_cb status;
        http_data_cb header_field;
        http_data_cb header_value;
        http_cb headers_complete;
        http_data_cb body;
        http_cb message_complete;

        void setup_callbacks(http_parser&);
        void cleanup_callbacks(http_parser&);

        ::http_parser_settings& Get() { return settings; }
        ::http_parser_settings const& Get() const { return settings; }

    private:
        ::http_parser_settings settings;

        std::unique_ptr<HttpContext> context;
    };

    struct HttpContext {
        HttpContext(httpp11::http_parser &p, httpp11::http_parser_settings &s)
                : parser(p), settings(s) {
        }

        httpp11::http_parser &parser;
        httpp11::http_parser_settings &settings;
    };

    class http_error : public std::runtime_error {
    public:
        http_error(http_error const&) = default;
        virtual ~http_error();

        http_error(std::error_condition const&);
        std::error_condition const& condition() const;

    private:
        std::error_condition cond;
    };

    using unique_http_parser_settings = std::unique_ptr<http_parser_settings>;
    unique_http_parser_settings http_parser_settings_init();

    // library functions
    unique_http_parser http_parser_init(http_parser_type);

    void http_parser_execute(http_parser&, http_parser_settings&, std::vector<char> const&);

    bool http_should_keep_alive(http_parser const& parser);

    void http_parser_pause(http_parser& parser, bool paused);

    bool http_body_is_final(http_parser const& parser);

} // httpp11
