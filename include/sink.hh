#include "deferred.h"


template<typename Data>
class Sink {
public:
    virtual Deferred<> on_data(Data&&) = 0;
    virtual void on_error(Error const&, bool fatal) = 0;
    virtual void on_close() = 0;

    virtual ~Sink() {}
};
