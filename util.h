#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

template <typename T>
int sgn(T v)
{
    return v > 0 ? 1
        : v < 0 ? -1
        : 0;
}

#endif // UTIL_H_INCLUDED
