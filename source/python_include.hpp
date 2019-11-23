/*
 * python_include.hpp
 * Copyright (C) 2019 Stephan Seitz <stephan.seitz@fau.de>
 *
 * Distributed under terms of the GPLv3 license.
 */

#pragma once

#ifndef Q_MOC_RUN
#pragma push_macro("Q_FOREACH")
#pragma push_macro("foreach")
#pragma push_macro("slots")
#undef Q_FOREACH
#undef foreach
#undef slots
#include "pybind11/embed.h"
#include "pybind11/numpy.h"
#include "pybind11/eval.h"
#pragma pop_macro("Q_FOREACH")
#pragma pop_macro("foreach")
#pragma pop_macro("slots")
#endif
