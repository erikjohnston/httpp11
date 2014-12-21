#pragma once

#include <memory>
#include <tuple>
#include <vector>

template<typename... Args>
class CallTrackerBase {
public:
    using Tuple = std::tuple<Args...>;
    unsigned long number_of_calls() const {
        return call_list_ptr->size();
    }

    std::shared_ptr<std::vector<std::tuple<Args...>>> call_list_ptr
            = std::make_shared< std::vector< Tuple > >();
};

template<typename... Args>
class CallTracker : public CallTrackerBase<Args...> {
public:
    void operator()(Args... args) {
        this->call_list_ptr->emplace_back(std::forward<Args>(args)...);
    }
};

template<typename R, R r, typename... Args>
class CallTrackerReturn : public CallTrackerBase<Args...> {
public:
    R operator()(Args... args) {
        this->call_list_ptr->emplace_back(std::forward<Args>(args)...);
        return r;
    }
};


template<typename T, typename F>
struct ConvertContainer{
    T operator()(F const& f) {
        return T(std::begin(f), std::end(f));
    }
};

extern ConvertContainer<std::vector<char>, std::string> to_vec;
extern ConvertContainer<std::string, std::vector<char>> to_str;

//using to_vector = convert_container<std::vector<char>, std::string>;
//using to_string = convert_container<std::string, std::vector<char>>;
