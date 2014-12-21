#include "catch.hpp"
#include "utils.hh"

#include "http_response_parser.hh"

#include <string>
#include <vector>


TEST_CASE("HTTP Response Parser", "[http]") {
    HttpResponseParser parser;

    bool has_fired = false;
    HttpResponse response;
    parser.callback = [&has_fired, &response] (HttpResponse&& r) mutable {
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

        std::vector<char> response_bytes(response_str.begin(), response_str.end());

        parser.on_data(to_vec(response_str));

        REQUIRE(has_fired == true);
        REQUIRE(response.status == 200);
        REQUIRE(to_str(response.status_line) == "OK");
        REQUIRE(to_str(response.body) == "Hello");
    };
}