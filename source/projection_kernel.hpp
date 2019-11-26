
#include "python_include.hpp"

void call_projection_kernel(float T0, float T1, float T10, float T11, float T2, float T3, float T4, float T5, float T6,
                            float T7, float T8, float T9, double detector_spacing, pybind11::array_t< float > proj,
                            pybind11::array_t< float > vol, double volume_spacing);
