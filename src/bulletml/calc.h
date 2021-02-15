#ifndef CALC_H_
#define CALC_H_

#include "formula.h"
#include "bulletmlcommon.h"

#include <string>
#include <memory>

DECLSPEC rr::auto_ptr<Formula<double> > calc(const std::string& str);

#endif // CALC_H_

