#include <cmath>

template<typename T>
inline T tcos(T x) noexcept
{
    x -= T(.25) + std::floor(x + T(.25));
    x *= T(16.) * (std::abs(x) - T(.5));
#if EXTRA_PRECISION
    x += T(.225) * x * (std::abs(x) - T(1.));
#endif
    return x;
}
