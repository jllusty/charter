#pragma once

#include <utility>

// for 2D quantities
template<typename T>
struct vec2 {
    T x; T y;
    vec2(T x, T y) : x(x), y(y) {}
};
// specializations of interest
using vec2f = vec2<float>;
using vec2i = vec2<int>;
using vec2u = vec2<unsigned>;
// operator overloads
template<typename T>
auto operator+(vec2<T> v1, vec2<T> v2) {
    return vec2<T>(v1.x+v2.x,v1.y+v2.y);
}
template<typename T>
auto operator-(const vec2<T>& v1, const vec2<T>& v2) {
    return vec2<T>(v1.x-v2.x,v1.y-v2.y);
}
template<typename T>
auto operator*(const vec2<T>& v1, const T& t) {
    return vec2<T>(v1.x*t,v1.y*t);
}
template<typename T>
auto operator*(const T& t, const vec2<T>& v1) {
    return vec2<T>(v1.x*t,v1.y*t);
}
template<typename T>
auto operator/(const vec2<T>& v1, const T& t) {
    return vec2<T>(v1.x/t,v1.y/t);
}
template<typename T>
auto& operator*=(vec2<T>& v1, const T& t) {
    v1.x *= t;
    v1.y *= t;
    return v1;
}

// for axis-aligned boxes
template<typename T>
struct rect {
    T x, y;     // top, left
    T w, h;     // width, height
    rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}
};
// specializations of interest
using rectf = rect<float>;
using recti = rect<int>;
using rectu = rect<unsigned>;
// collision detection
template<typename T>
bool collision(rect<T> r1, rect<T> r2) {
    return (r1.x < r2.x + r2.w && r1.x + r1.w > r2.x &&
            r1.y < r2.y + r2.h && r1.y + r1.h > r2.y);
}