#pragma once

#include <set>


namespace {
    class Tracking {
    public:
        virtual ~Tracking() {
            reset();
        }

        virtual void reset() = 0;
    };
};


class Trackable {
public:
    ~Trackable() {
        for (auto t : tracked_by) {
            t->reset();
        }
    }

    void track(Tracking* t) {
        tracked_by.insert(t);
    }

    void untrack(Tracking* t) {
        tracked_by.erase(t);
    }
private:
    std::set<Tracking*> tracked_by;
};


template<typename T>
class TrackingPtr : public Tracking {
public:
    TrackingPtr() {
    }

    TrackingPtr(T *t) {
        reset(t);
    }

    T *get() const {
        return ptr;
    }

    operator bool() const {
        return (ptr != nullptr);
    }

    void reset() final {
        reset(nullptr);
    }

    void reset(T* t) {
        if (t == ptr) return;
        if (ptr != nullptr) ptr->untrack(this);
        if (t != nullptr) t->track(this);

        ptr = t;
    }

private:
    T* ptr = nullptr;
};

