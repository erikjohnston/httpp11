#include "deferred.h"


Error::Error(int val, std::error_category const& cat) : std::error_condition(val, cat) {}
Error::Error(std::error_condition const& cond) : std::error_condition(cond) {}

Error::~Error() {}

std::string Error::message() const { return std::error_condition::message(); }
