#include "catch.hpp"

#include "httpp11/httpp11.hh"

#include "utils.hh"

namespace h = httpp11;


TEST_CASE("Test parsing GET response", "[http11]") {
    h::Parser parser(h::ParserType::Response);

    h::Settings settings;


    CallTrackerReturn<bool, false, h::Parser&> message_begin_cb;
    CallTrackerReturn<bool, false, h::Parser&> headers_complete_cb;
    CallTrackerReturn<bool, false, h::Parser&> message_complete_cb;


    settings.message_begin = message_begin_cb;
    settings.headers_complete = headers_complete_cb;
    settings.message_complete = message_complete_cb;


    CallTrackerReturn<bool, false, h::Parser&, h::BufferView const&> status_cb;
    CallTrackerReturn<bool, false, h::Parser&, h::BufferView const&> url_cb;
    CallTrackerReturn<bool, false, h::Parser&, h::BufferView const&> header_field_cb;
    CallTrackerReturn<bool, false, h::Parser&, h::BufferView const&> header_value_cb;
    CallTrackerReturn<bool, false, h::Parser&, h::BufferView const&> body_cb;

    settings.status = status_cb;
    settings.url = url_cb;
    settings.header_field = header_field_cb;
    settings.header_value = header_value_cb;
    settings.body = body_cb;

    SECTION("Check one big chunk") {
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nHello";

        h::execute(parser, settings, to_vec(response));

        REQUIRE(message_begin_cb.number_of_calls() == 1);
        REQUIRE(status_cb.number_of_calls() > 0);

        REQUIRE(url_cb.number_of_calls() == 0); // Only used for http_request parsing

        REQUIRE(header_field_cb.number_of_calls() >= 2);
        REQUIRE(header_value_cb.number_of_calls() >= 2);

        REQUIRE(headers_complete_cb.number_of_calls() == 1);

        REQUIRE(body_cb.number_of_calls() > 0);

        REQUIRE(message_complete_cb.number_of_calls() == 1);
    }

    SECTION("Whole pieces") {
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nHello";

        std::vector<std::pair<std::string, std::function<void()>>> chunks {
                {"HTTP/1.1 ", [&] () {
                    REQUIRE(message_begin_cb.number_of_calls() == 1);
                    REQUIRE(parser.Get().http_major == 1);
                    REQUIRE(parser.Get().http_minor == 1);
                }},
                {"200 OK\r\n", [&] () {
                    REQUIRE(status_cb.number_of_calls() == 1);
                    REQUIRE(parser.Get().status_code == 200);
                }},
                {"Content-Type:", [&] () { REQUIRE(header_field_cb.number_of_calls() == 1); }},
                {"text/plain\r\n", [&] () { REQUIRE(header_value_cb.number_of_calls() == 1); }},
                {"Content-Length:", [&] () { REQUIRE(header_field_cb.number_of_calls() == 2); }},
                {" 5\r\n", [&] () { REQUIRE(header_value_cb.number_of_calls() == 2); }},
                {"\r\n", [&] () { REQUIRE(headers_complete_cb.number_of_calls() == 1); }},
                {"Hell", [&] () { REQUIRE(body_cb.number_of_calls() == 1); }},
                {"o", [&] () { REQUIRE(body_cb.number_of_calls() == 2); REQUIRE(message_complete_cb.number_of_calls() == 1); }}
        };

        for (auto& entry : chunks) {
            h::execute(parser, settings, to_vec(entry.first));
            entry.second();
        }

    }
}


