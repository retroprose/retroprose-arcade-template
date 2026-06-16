/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability 
* of this software for any purpose.  
* It is provided "as is" without express or implied warranty.
*/

#ifndef BOX2DLITE_MATHUTILS_H
#define BOX2DLITE_MATHUTILS_H


#include <cassert>
#include <cstdint>
#include <cmath>
#include <limits>


struct Scaler
{

	int32_t value;

	Scaler() { }
	Scaler(int32_t v) : value(v << 16) { }

	// explicit to make clear this is a no-no!
	explicit Scaler(float v) {
        float ans = v * ((int32_t)1 << 16);
        value = (ans < 0.0f) ? (int32_t)(ans - 0.5f) : (int32_t)(ans + 0.5f);
    }

    explicit operator float() const {
        return ((float)value / (float)((int32_t)1 << 16));
    }

	int32_t Raw() const {
        return value;
    }

	void Set(int32_t v) { value = v << 16; }
	void SetRaw(int32_t v) { value = v; }

	Scaler operator -() const
	{
		Scaler s;
		s.value = -value;
		return s;
	}
	
	void operator += (Scaler v)
	{
		value += v.value;
	}
	
	void operator -= (Scaler v)
	{
		value -= v.value;
	}

	void operator *= (Scaler a)
	{
		value = ((int64_t)value * (int64_t)a.value) >> 16;
	}

	void operator /= (Scaler a) {
        value = ((int64_t)value << 16) / (int64_t)a.value;
    }




};

inline Scaler RawScaler(int32_t v) {
	Scaler s;
	s.value = v;
	return s;
}

const Scaler k_pi = RawScaler(205887);
const Scaler k_max = RawScaler(std::numeric_limits<int32_t>::max());


inline bool operator==(Scaler lhs, Scaler rhs) {
	return lhs.value == rhs.value;
}

inline bool operator!=(Scaler lhs, Scaler rhs) {
	return lhs.value != rhs.value;
}

inline bool operator>=(Scaler lhs, Scaler rhs) {
	return lhs.value >= rhs.value;
}

inline bool operator<=(Scaler lhs, Scaler rhs) {
	return lhs.value <= rhs.value;
}

inline bool operator>(Scaler lhs, Scaler rhs) {
	return lhs.value > rhs.value;
}

inline bool operator<(Scaler lhs, Scaler rhs) {
	return lhs.value < rhs.value;
}

inline Scaler operator+(Scaler lhs, Scaler rhs) {
	return RawScaler(lhs.value + rhs.value);
}

inline Scaler operator-(Scaler lhs, Scaler rhs) {
	return RawScaler(lhs.value - rhs.value);
}

inline Scaler operator*(Scaler lhs, Scaler rhs) {
	return RawScaler((int32_t)(((int64_t)lhs.value * (int64_t)rhs.value) >> 16));
}

inline Scaler operator/(Scaler lhs, Scaler rhs) {
	return RawScaler((int32_t)(((int64_t)lhs.value << 16) / (int64_t)rhs.value));
}

inline Scaler operator*(int32_t lhs, Scaler rhs) {
	return RawScaler(lhs * rhs.value);
}

inline Scaler operator*(Scaler lhs, int32_t rhs) {
	return RawScaler(lhs.value * rhs);
}

inline Scaler operator>>(Scaler lhs, int32_t rhs) {
	return RawScaler(lhs.value >> rhs);
}

inline Scaler operator<<(Scaler lhs, int32_t rhs) {
	return RawScaler(lhs.value << rhs);
}


inline Scaler Abs(Scaler a)
{
	return a > 0 ? a : -a;
}

inline Scaler Sign(Scaler x)
{
	return x < 0 ? -1 : 1;
}

inline Scaler Min(Scaler a, Scaler b)
{
	return a < b ? a : b;
}

inline Scaler Max(Scaler a, Scaler b)
{
	return a > b ? a : b;
}

inline Scaler Clamp(Scaler a, Scaler low, Scaler high)
{
	return Max(low, Min(a, high));
}

template<typename T> inline void Swap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}


// 40000000 = 1<<30
// sqrt_i32 computes the squrare root of a 32bit integer and returns
// a 32bit integer value. It requires that v is positive.
inline int32_t Sqrt_i32(int32_t v) {
	uint32_t b = 0x40000000U, q = 0, r = v;
	while (b > r)
		b >>= 2;
	while( b > 0 ) {
		uint32_t t = q + b;
		q >>= 1;           
		if( r >= t ) {     
			r -= t;        
			q += b;        
		}
		b >>= 2;
	}
	return q;
}

// 4000000000000000 = ((uint64_t)1)<<62
// sqrt_i64 computes the squrare root of a 64bit integer and returns
// a 64bit integer value. It requires that v is positive.
inline int64_t Sqrt_i64(int64_t v) {
	uint64_t b = 0x4000000000000000UL, q = 0, r = v;
	while (b > r)
		b >>= 2;
	while( b > 0 ) {
		uint64_t t = q + b;
		q >>= 1;           
		if( r >= t ) {     
			r -= t;        
			q += b;        
		}
		b >>= 2;
	}
	return q;
}

inline Scaler Sqrt(Scaler s) {
	return RawScaler(Sqrt_i32(s.value) << 8);
}


static Scaler fastSin(Scaler x) {
	int32_t twoPi = k_pi.Raw() << 1;
	int32_t n = x.Raw();

	int32_t m;        
	if (n < 0) {
		n *= -1;
		m = ((n + k_pi.Raw()) % twoPi) - k_pi.Raw();
		m *= -1;
	} else {
		m = ((n + k_pi.Raw()) % twoPi) - k_pi.Raw();
	}
	Scaler xOverPi = RawScaler(m) / k_pi;
	return Scaler(4) * xOverPi * (Scaler(1) - Abs(xOverPi));
}

static Scaler fastCos(Scaler x) {	
	int32_t twoPi = k_pi.Raw() << 1;
	int32_t n = x.Raw() + (k_pi.Raw() >> 1);

	int32_t m;        
	if (n < 0) {
		n *= -1;
		m = ((n + k_pi.Raw()) % twoPi) - k_pi.Raw();
		m *= -1;
	} else {
		m = ((n + k_pi.Raw()) % twoPi) - k_pi.Raw();
	}
	Scaler xOverPi = RawScaler(m) / k_pi;
	return Scaler(4) * xOverPi * (Scaler(1) - Abs(xOverPi));
}


struct Vec2
{
	Vec2() {}
	Vec2(Scaler x, Scaler y) : x(x), y(y) {}

	void Set(Scaler x_, Scaler y_) { x = x_; y = y_; }

	Vec2 operator -() { return Vec2(-x, -y); }
	
	void operator += (const Vec2& v)
	{
		x += v.x; y += v.y;
	}
	
	void operator -= (const Vec2& v)
	{
		x -= v.x; y -= v.y;
	}

	void operator *= (Scaler a)
	{
		x *= a; y *= a;
	}

	Scaler Length() const
	{
		//return sqrtf(x * x + y * y);
		int64_t v = (int64_t)x.Raw() * (int64_t)x.Raw() + (int64_t)y.Raw() * (int64_t)y.Raw();
        return RawScaler((int32_t)Sqrt_i64(v));
	}

	Scaler x, y;
};

// this used to use fabs instead of above function
inline Vec2 Abs(const Vec2& a)
{
	return Vec2(Abs(a.x), Abs(a.y));
}

struct Mat22
{
	Mat22() {}
	Mat22(Scaler angle)
	{
		Scaler c = fastCos(angle), s = fastSin(angle);
		col1.x = c; col2.x = -s;
		col1.y = s; col2.y = c;
	}

	Mat22(const Vec2& col1, const Vec2& col2) : col1(col1), col2(col2) {}

	Mat22 Transpose() const
	{
		return Mat22(Vec2(col1.x, col2.x), Vec2(col1.y, col2.y));
	}

	Mat22 Invert() const
	{
		Scaler a = col1.x, b = col2.x, c = col1.y, d = col2.y;
		Mat22 B;
		Scaler det = a * d - b * c;
		assert(det != 0);
		det = Scaler(1) / det;
		B.col1.x =  det * d;	B.col2.x = -det * b;
		B.col1.y = -det * c;	B.col2.y =  det * a;
		return B;
	}

	Vec2 col1, col2;
};

inline Mat22 Abs(const Mat22& A)
{
	return Mat22(Abs(A.col1), Abs(A.col2));
}

inline Scaler Dot(const Vec2& a, const Vec2& b)
{
	return a.x * b.x + a.y * b.y;
}

inline Scaler Cross(const Vec2& a, const Vec2& b)
{
	return a.x * b.y - a.y * b.x;
}

inline Vec2 Cross(const Vec2& a, Scaler s)
{
	return Vec2(s * a.y, -s * a.x);
}

inline Vec2 Cross(Scaler s, const Vec2& a)
{
	return Vec2(-s * a.y, s * a.x);
}

inline Vec2 operator * (const Mat22& A, const Vec2& v)
{
	return Vec2(A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y);
}

inline Vec2 operator + (const Vec2& a, const Vec2& b)
{
	return Vec2(a.x + b.x, a.y + b.y);
}

inline Vec2 operator - (const Vec2& a, const Vec2& b)
{
	return Vec2(a.x - b.x, a.y - b.y);
}

inline Vec2 operator * (Scaler s, const Vec2& v)
{
	return Vec2(s * v.x, s * v.y);
}

inline Mat22 operator + (const Mat22& A, const Mat22& B)
{
	return Mat22(A.col1 + B.col1, A.col2 + B.col2);
}

inline Mat22 operator * (const Mat22& A, const Mat22& B)
{
	return Mat22(A * B.col1, A * B.col2);
}


// do math here!
class MersenneTwister {
private:
    // static data and functions
    static const int N = 624;
    static const int M = 397;
    static const unsigned int UPPER_MASK = 0x80000000;
    static const unsigned int LOWER_MASK = 0x7fffffff;
    static const unsigned int MATRIX_A = 0x9908b0df;

    static unsigned int imul(unsigned int a, unsigned int b) {
        unsigned int al = a & 0xffff;
        unsigned int ah = a >> 16;
        unsigned int bl = b & 0xffff;
        unsigned int bh = b >> 16;
        unsigned int ml = al * bl;
        unsigned int mh = ( (((ml >> 16) + al * bh) & 0xffff) + ah * bl ) & 0xffff;
        return (mh << 16) | (ml & 0xffff);
    }

    // local member variables
    int p;
    int q;
    int r;
    unsigned int x[N];

public:
    MersenneTwister() { setSeed(0); }
    MersenneTwister(unsigned int s) { setSeed(s); }

    void setSeed(unsigned int s) {	
        x[0] = s;
        for (int i = 1; i < N; i++) {
            x[i] = imul( 1812433253, x[i - 1] ^ (x[i - 1] >> 30) ) + i;
            x[i] &= 0xffffffff;
        }
        p = 0;
        q = 1;
        r = M;
    }

 	int32_t next(int32_t high) {
        return (int32_t)(next() % high);
    }

    int32_t next(int32_t low, int32_t high) {
        return (int32_t)(next() % (high - low + 1)) + low;
    }

    unsigned int next() {
        unsigned int y = (x[p] & UPPER_MASK) | (x[q] & LOWER_MASK);
        x[p] = x[r] ^ (y >> 1) ^ ((y & 1) * MATRIX_A);
        y = x[p];

        if (++p == N) p = 0;
        if (++q == N) q = 0;
        if (++r == N) r = 0;

        y ^= (y >> 11);
        y ^= (y << 7) & 0x9d2c5680;
        y ^= (y << 15) & 0xefc60000;
        y ^= (y >> 18);

        return y;
    }
};


#endif

