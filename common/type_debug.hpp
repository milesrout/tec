// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#ifndef TRILLEK_COMMON_TYPE_DEBUG_HPP
#define TRILLEK_COMMON_TYPE_DEBUG_HPP

#include "types.hpp"
#include <cxxabi.h>

static int _status;
#define TYPENAME(T) (abi::__cxa_demangle(typeid(T).name(), 0, 0, &_status))

#endif
