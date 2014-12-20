#include "catch.hpp"

#include "http_parser_requests.hh"
#include "tcp_connection.hh"

#include "json11.hpp"

#include "utils.hh"

class TestTcpConnection : public TcpConnection {
public:
    using Bytes = TcpConnection::Bytes;

    virtual Deferred<> write(Bytes const& b) { write_tracker(b); return Deferred<>::make_succeeded(); };

    virtual void read(Callback<Bytes const&> const& c) { read_tracker(c);};
    virtual void read() { un_read_tracker();};

    // Something went wrong, so we should probably close it.
    virtual void error() { error_tracker(); };

    // Looks like we don't want to reuse this connection, so close it.
    virtual void close() { close_tracker(); };

    CallTracker<Bytes> write_tracker;
    CallTracker<Callback<Bytes const&>> read_tracker;
    CallTracker<> un_read_tracker;

    CallTracker<> error_tracker;
    CallTracker<> close_tracker;
};

class TestTcpFactory : public TcpFactory {
public:
    TestTcpFactory(std::shared_ptr<TcpConnection> const& conn) : conn(conn) {}
    virtual Deferred<std::shared_ptr<TcpConnection>> get_connection() {
        return Deferred<std::shared_ptr<TcpConnection>>::make_succeeded(conn);
    }

    std::shared_ptr<TcpConnection> conn;
};

TEST_CASE("Get http request", "[http]") {
    auto test_conn = std::make_shared<TestTcpConnection>();
    auto test_factory = std::make_shared<TestTcpFactory>(test_conn);

    HttpParserRequests requests(test_factory);

    SECTION("Empty GET") {
        auto deferred = requests.get("/test/", HttpRequests::QueryParams());

        REQUIRE(!deferred.is_done());
        REQUIRE(test_conn->write_tracker.call_list_ptr->size() == 1);

        auto &bytes = std::get<0>(test_conn->write_tracker.call_list_ptr->front());
        std::string output(bytes.begin(), bytes.end());

        INFO(output);

        REQUIRE(test_conn->read_tracker.number_of_calls() == 1);

        Callback<TcpConnection::Bytes const &> &read_cb = std::get<0>(test_conn->read_tracker.call_list_ptr->front());

        std::string response_str = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\n{}";
        TcpConnection::Bytes response_bytes(response_str.begin(), response_str.end());

        read_cb.success(response_bytes);

        REQUIRE(deferred.is_done());
        REQUIRE(deferred.has_succeeded());

        json11::Json::object empty_json;
        REQUIRE(std::get<0>(deferred.get_result()) == empty_json);
    }

    SECTION("Non-empty GET") {
        auto deferred = requests.get("/test/", HttpRequests::QueryParams());

        Callback<TcpConnection::Bytes const &> &read_cb = std::get<0>(test_conn->read_tracker.call_list_ptr->front());

        std::string response_str = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\n{\"test\": 1}";
        TcpConnection::Bytes response_bytes(response_str.begin(), response_str.end());

        read_cb.success(response_bytes);

        REQUIRE(deferred.is_done());
        REQUIRE(deferred.has_succeeded());

        json11::Json::object json;
        json["test"] = 1;
        REQUIRE(std::get<0>(deferred.get_result()) == json);
    }
}