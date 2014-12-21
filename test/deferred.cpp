#include "catch.hpp"

#include "utils.hh"

#include "deferred.h"


TEST_CASE("Successful deferreds", "[deferred]") {
    Deferred<int, char> deferred;

    CallTracker<int, char> tracker;

    SECTION("Fire existing callback") {
        REQUIRE(tracker.call_list_ptr->empty());

        deferred.add_success(tracker);

        deferred.success(1, 'a');
        REQUIRE(tracker.call_list_ptr->size() == 1);
        REQUIRE(tracker.call_list_ptr->front() == std::make_tuple(1, 'a'));

        SECTION("Then fire new callback") {
            CallTracker<int, char> tracker2;

            REQUIRE(tracker2.call_list_ptr->empty());

            deferred.add_success(tracker2);

            REQUIRE(tracker.call_list_ptr->size() == 1);
            REQUIRE(tracker2.call_list_ptr->size() == 1);
            REQUIRE(tracker2.call_list_ptr->front() == std::make_tuple(1, 'a'));
        }
    }

    SECTION("Fire new callback") {
        deferred.success(1, 'a');

        REQUIRE(tracker.call_list_ptr->empty());

        deferred.add_success(tracker);

        REQUIRE(tracker.call_list_ptr->size() == 1);
        REQUIRE(tracker.call_list_ptr->front() == std::make_tuple(1, 'a'));
    }

    SECTION("Basic Copies") {
        auto d2 = deferred;

        REQUIRE(!d2.is_done());

        d2.success(1, 'a');

        REQUIRE(deferred.is_done());
    }

    SECTION("Error") {
        {
            deferred.error(std::generic_category().default_error_condition(EDOM));
            REQUIRE(deferred.has_erred());
        }
    }
}


/************ END TESTS ************/


// Pretty print tuples. Taken from pull request #326
namespace Catch {
    namespace TupleDetail {
        template<
                typename Tuple,
                std::size_t N = 0,
                bool = (N < std::tuple_size<Tuple>::value)
        >
        struct ElementPrinter {
            static void print(const Tuple &tuple, std::ostream &os) {
                os << (N ? ", " : " ")
                        << Catch::toString(std::get<N>(tuple));
                ElementPrinter<Tuple, N + 1>::print(tuple, os);
            }
        };

        template<
                typename Tuple,
                std::size_t N
        >
        struct ElementPrinter<Tuple, N, false> {
            static void print(const Tuple &, std::ostream &) {
            }
        };

    }

    template<typename ...Types>
    struct StringMaker<std::tuple<Types...>> {

        static std::string convert(const std::tuple<Types...> &tuple) {
            std::ostringstream os;
            os << '{';
            TupleDetail::ElementPrinter<std::tuple<Types...>>::print(tuple, os);
            os << " }";
            return os.str();
        }
    };

}
