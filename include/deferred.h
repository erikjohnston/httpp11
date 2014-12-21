#pragma once

#include <functional>
#include <memory>
#include <system_error>
#include <tuple>
#include <vector>


namespace {
    template<int ...>
    struct seq {
    };

    template<int N, int ...S>
    struct gens : gens<N - 1, N - 1, S...> {
    };

    template<int ...S>
    struct gens<0, S...> {
        typedef seq<S...> type;
    };
}

class Error : public std::error_condition {
public:
    Error(int val, std::error_category const& cat);

    Error(std::error_condition const& cond);

    Error(Error const&) = default;
    Error(Error &&) = default;

    virtual ~Error();

    virtual std::string message() const;

};

/*
 * Warning: The deferred will clear all callbacks stored when it gets resolved.
 */
template<typename... Args>
class Deferred {
public:
    using success_cb = std::function<void(Args...)>;
    using error_cb = std::function<void(Error const&)>;
    using always_cb = std::function<void(bool)>;

    static Deferred make_succeeded(Args... args) {
        Deferred d;
        d.success(std::forward<Args>(args)...);
        return d;
    }

    Deferred() = default;
    Deferred(Deferred const&) = default;
    Deferred(Deferred &&) = default;

    Deferred& add_success(success_cb const& s) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        if (cbs->values_ptr) {
            call_success_from_store(s);
        } else if (!cbs->error_ptr) {
            cbs->success.push_back(s);
        }

        return *this;
    }
    Deferred& add_error(error_cb const& e) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        if (cbs->error_ptr) {
            e(*cbs->error_ptr);
        } else if (!cbs->values_ptr) {
            cbs->error.push_back(e);
        }
        return *this;
    }

    template<typename C>
    Deferred& add_error(C& c) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        if (cbs->error_ptr) {
            c.error(*cbs->error_ptr);
        } else if (!cbs->values_ptr) {
            cbs->error.push_back([c](Error const &e) mutable {
                c.error(e);
            });
        }
        return *this;
    }

    Deferred& add_always(always_cb const& a) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        if (cbs->values_ptr || cbs->error_ptr) {
            always_cb();
        } else {
            cbs->always.push_back(a);
        }
        return *this;
    }

    Deferred& add(success_cb const& s, error_cb const& e) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        if (cbs->values_ptr) {
            call_success_from_store(s);
        } else if (cbs->error_ptr) {
            e(*cbs->error_ptr);
        } else {
            cbs->error.push_back(e);
            cbs->success.push_back(s);
        }

        return *this;
    }

    template<typename C>
    Deferred& add(success_cb const& s, C& c) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        if (cbs->values_ptr) {
            call_success_from_store(s);
        } else if (cbs->error_ptr) {
            c.error(*cbs->error_ptr);
        } else {
            cbs->error.push_back([c](Error const &e) mutable {
                c.error(e);
            });
            cbs->success.push_back(s);
        }
        return *this;
    }

    void success(Args... args) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        cbs->values_ptr = std::unique_ptr<std::tuple<Args...>>(
                new std::tuple<Args...>(args...)
        );

        for (auto& cb : cbs->success) cb(args...);
        for (auto& cb : cbs->always) cb(true);

        cbs->success.clear();
        cbs->success.shrink_to_fit();
        cbs->always.clear();
        cbs->always.shrink_to_fit();
        cbs->error.clear();
        cbs->error.shrink_to_fit();
    }

    void error(Error const& e) {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        cbs->error_ptr = std::unique_ptr<Error>(new Error(e));

        for (auto& cb : cbs->error) cb(e);
        for (auto& cb : cbs->always) cb(false);

        cbs->success.clear();
        cbs->success.shrink_to_fit();
        cbs->always.clear();
        cbs->always.shrink_to_fit();
        cbs->error.clear();
        cbs->error.shrink_to_fit();
    }

    bool is_done() const {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        return cbs->values_ptr || cbs->error_ptr;
    }

    bool has_succeeded() const {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        return static_cast<bool>(cbs->values_ptr);
    }

    bool has_erred() const {
        if (!cbs) throw std::logic_error("cbs is not instansiated");

        return static_cast<bool>(cbs->error_ptr);
    }

    std::tuple<Args...> const& get_result() {
        if (!cbs) throw std::logic_error("cbs is not instansiated");
        if (!cbs->values_ptr) {
            if (cbs->error_ptr) cbs->error_ptr->throw_as_exception();
            throw std::logic_error("deferred has not succeeded");
        }

        return *cbs->values_ptr;
    }

private:
    struct CbStorage {
        std::vector<success_cb> success;
        std::vector<error_cb> error;
        std::vector<always_cb> always;
        std::unique_ptr<std::tuple<Args...>> values_ptr;
        std::unique_ptr<Error> error_ptr;
    };
    std::shared_ptr<CbStorage> cbs = std::make_shared<CbStorage>();

    void call_success_from_store(success_cb const& f) {
        call_success_from_store_impl(f, typename gens<sizeof...(Args)>::type());
    }

    template<int... S>
    void call_success_from_store_impl(success_cb const& f, seq<S...>) {
        if (!cbs->values_ptr) throw std::logic_error("cbs->values_ptr is not instansiated");
        f(std::get<S>(*cbs->values_ptr) ...);
    }
};
