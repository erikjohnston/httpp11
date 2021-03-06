#include "catch.hpp"
#include "utils.hh"

#include "httpp11/collators.hh"

#include <string>
#include <vector>


TEST_CASE("HTTP Response Parser", "[http]") {
    httpp11::Parser parser(httpp11::ParserType::Response);
    httpp11::HttpResponseCollator settings;

    bool has_fired = false;
    httpp11::HttpResponse response;
    settings.callback = [&has_fired, &response] (httpp11::HttpResponse&& r) mutable {
        has_fired = true;
        response = std::move(r);
    };

    SECTION("Entire response") {
        std::string response_str =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 5\r\n"
                "Content-Type: text/plain\r\n"
                "\r\n"
                "Hello"
        ;

        httpp11::execute(parser, settings, response_str);

        REQUIRE(has_fired == true);
        REQUIRE(response.status == 200);
        REQUIRE(to_str(response.reason_phrase) == "OK");
        REQUIRE(to_str(response.body) == "Hello");

        REQUIRE(response.headers.count("Content-Length") == 1);
        REQUIRE(response.headers.count("content-length") == 1);

        auto length = response.headers.find("Content-Length");
        REQUIRE(length != response.headers.end());
        REQUIRE(to_str(length->second) == "5");

        REQUIRE(response.headers.count("Content-Type") == 1);

        auto type = response.headers.find("Content-Type");
        REQUIRE(type != response.headers.end());
        REQUIRE(to_str(type->second) == "text/plain");

        REQUIRE(response.headers.size() == 2);

        SECTION("Another response") {
            response_str =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Length: 3\r\n"
                "X-Test: Testing!\r\n"
                "\r\n"
                "Hi!"
            ;

            httpp11::execute(parser, settings, response_str);

            REQUIRE(has_fired == true);
            REQUIRE(response.status == 400);
            REQUIRE(to_str(response.reason_phrase) == "Bad Request");
            REQUIRE(to_str(response.body) == "Hi!");

            REQUIRE(response.headers.count("Content-Length") == 1);

            length = response.headers.find("Content-Length");
            REQUIRE(length != response.headers.end());
            REQUIRE(to_str(length->second) == "3");

            REQUIRE(response.headers.count("X-Test") == 1);

            auto test = response.headers.find("X-Test");
            REQUIRE(test != response.headers.end());
            REQUIRE(to_str(test->second) == "Testing!");

            REQUIRE(response.headers.size() == 2);
        };
    };
}
