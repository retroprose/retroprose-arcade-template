#ifndef FIXED_FIXED_HPP
#define FIXED_FIXED_HPP

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cmath>

#include<typemeta/typemeta.hpp>

/*

    What I still need:
        maybe a fast inverse square root?
        arctan of Q15.16

    Taylor series
    distance = abs(x1-x2) + abs(y1-y2) - minimum( abs(x1-x2), abs(y1-y2) ) / 2;

    //double FastArcTan(double x) { return M_PI_4*x - x*(fabs(x) - 1)*(0.2447 + 0.0663*fabs(x)); }

*/

/*

    3.14159265358979323846264f

1, 0

x * 1 + y * 0 / Xmag


x * v.x + y * v.y

PI = Scaler::make(205887)
DEGREE_TO_RADIAN = Scaler::make(1144)

ANGLE BETWEEN VECTORS

cos(theta) = (A . B) / (||A|| ||B||)



https://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml

u32 approx_distance( s32 dx, s32 dy )
{
   u32 min, max, approx;

   if ( dx < 0 ) dx = -dx;
   if ( dy < 0 ) dy = -dy;

   if ( dx < dy )
   {
      min = dx;
      max = dy;
   } else {
      min = dy;
      max = dx;
   }

   approx = ( max * 1007 ) + ( min * 441 );
   if ( max < ( min << 4 ))
      approx -= ( max * 40 );

   // add 512 for proper rounding
   return (( approx + 512 ) >> 10 );
} 


u32 approx_distance( s32 dx, s32 dy )
{
   u32 min, max;

   if ( dx < 0 ) dx = -dx;
   if ( dy < 0 ) dy = -dy;

   if ( dx < dy )
   {
      min = dx;
      max = dy;
   } else {
      min = dy;
      max = dx;
   }

   // coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
   return ((( max << 8 ) + ( max << 3 ) - ( max << 4 ) - ( max << 1 ) +
            ( min << 7 ) - ( min << 5 ) + ( min << 3 ) - ( min << 1 )) >> 8 );
} 

*/


/*

    These could be faster random number generators V

    Permuted congruential generator
    
    - O'Neill's PCG family.
    - Vigna's xoroshiro family say xoroshiro+ (not a Japanese name btw, but "X-or, rotate, shift, rotate") and
    - D. E. Shaw's Random123 suite (which includes Philox and the nicely named ARS, a simplification of encrypting an
    inifite squence of zeros with AES-CTR), though I'm not sure how much the Random123 PRNG have been scrutinised.


*/

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
        return (int32_t)((next()) % (high - low + 1)) + low;
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




class Scaler {
private:
    

public:
    int32_t value;

    constexpr static Scaler make_raw(uint32_t v) {
        return Scaler(*(reinterpret_cast<int32_t*>(&v)), false);
    }

    constexpr static Scaler make(int32_t v) {
        return Scaler(v, false);
    }

    Scaler() { }

    // I'm making this not explicit because writing Scaler all of the time is annoying!
    constexpr Scaler(int32_t v) : value(v << 16) { }
    constexpr Scaler(int32_t v, bool) : value(v) { }

    // floating point conversions
    explicit Scaler(double v) {
        float ans = v * ((int32_t)1 << 16);
        value = (ans < 0.0f) ? (int32_t)(ans - 0.5f) : (int32_t)(ans + 0.5f);
    }

    explicit Scaler(float v) {
        float ans = v * ((int32_t)1 << 16);
        value = (ans < 0.0f) ? (int32_t)(ans - 0.5f) : (int32_t)(ans + 0.5f);
    }

    explicit operator double() const {
        return ((double)value / (double)((int32_t)1 << 16));
    }

    explicit operator float() const {
        return ((float)value / (float)((int32_t)1 << 16));
    }

    explicit operator int32_t() const {
        return value >> 16;
    }

    Scaler operator-() const {
        return make(-value);
    }

    Scaler operator+=(Scaler rhs) {
        value += rhs.value;
        return *this;
    }

    Scaler operator-=(Scaler rhs) {
        value -= rhs.value;
        return *this;
    }
   
    Scaler operator*=(Scaler rhs) {
        value = ((int64_t)value * (int64_t)rhs.value) >> 16;
        return *this;
    }

    Scaler operator*=(int32_t rhs) {
        value = value * rhs;
        return *this;
    }

    Scaler operator/=(Scaler rhs) {
        value = ((int64_t)value << 16) / (int64_t)rhs.value;
        return *this;
    }

    void setRaw(int32_t v) {
        value = v;
    }

    int32_t raw() const {
        return value;
    }

    static Scaler min(Scaler a, Scaler b) {
        return make(std::min(a.raw(), b.raw()));
    }

    static Scaler max(Scaler a, Scaler b) {
        return make(std::max(a.raw(), b.raw()));
    }

    static Scaler abs(Scaler s) {
        return make(std::abs(s.value));
    }

    static Scaler fastSin(Scaler x) {
        Scaler pi = make(205887);
        
        int32_t twoPi = pi.raw() << 1;
        int32_t n = x.raw();

        int32_t m;        
        if (n < 0) {
            n *= -1;
            m = ((n + pi.raw()) % twoPi) - pi.raw();
            m *= -1;
        } else {
            m = ((n + pi.raw()) % twoPi) - pi.raw();
        }
        Scaler xOverPi = make(m) / pi;
        return Scaler(4) * xOverPi * (Scaler(1) - abs(xOverPi));
    }

    static Scaler fastCos(Scaler x) {
        Scaler pi = make(205887);
        
        int32_t twoPi = pi.raw() << 1;
        int32_t n = x.raw() + (pi.raw() >> 1);

        int32_t m;        
        if (n < 0) {
            n *= -1;
            m = ((n + pi.raw()) % twoPi) - pi.raw();
            m *= -1;
        } else {
            m = ((n + pi.raw()) % twoPi) - pi.raw();
        }
        Scaler xOverPi = make(m) / pi;
        return Scaler(4) * xOverPi * (Scaler(1) - abs(xOverPi));
    }

    // 40000000 = 1<<30
    // sqrt_i32 computes the squrare root of a 32bit integer and returns
    // a 32bit integer value. It requires that v is positive.
    static int32_t sqrt_i32(int32_t v) {
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
    static int64_t sqrt_i64(int64_t v) {
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

    static Scaler sqrt(Scaler s) {
        return make(sqrt_i32(s.value) << 8);
    }

    /*static Scaler sqrt(Scaler s) {
        assert(s.value >= 0);
        uint32_t t, q, b, r;
        r = (uint32_t)s.value; 
        q = 0;          
        b = 0x40000000UL;
        if( r < 0x40000200 )
        {
            while( b != 0x40 )
            {
                t = q + b;
                if( r >= t )
                {
                    r -= t;
                    q = t + b; // equivalent to q += 2*b
                }
                r <<= 1;
                b >>= 1;
            }
            q >>= 8;
            return make(q);
        }
        while( b > 0x40 )
        {
            t = q + b;
            if( r >= t )
            {
                r -= t;
                q = t + b; // equivalent to q += 2*b
            }
            if( (r & 0x80000000) != 0 )
            {
                q >>= 1;
                b >>= 1;
                r >>= 1;
                while( b > 0x20 )
                {
                    t = q + b;
                    if( r >= t )
                    {
                        r -= t;
                        q = t + b;
                    }
                    r <<= 1;
                    b >>= 1;
                }
                q >>= 7;
                return make(q);
            }
            r <<= 1;
            b >>= 1;
        }
        q >>= 8;
        return make(q);
    }*/

    friend bool operator==(Scaler lhs, Scaler rhs) {
        return lhs.value == rhs.value;
    }

    friend bool operator!=(Scaler lhs, Scaler rhs) {
        return lhs.value != rhs.value;
    }

    friend bool operator>=(Scaler lhs, Scaler rhs) {
        return lhs.value >= rhs.value;
    }

    friend bool operator<=(Scaler lhs, Scaler rhs) {
        return lhs.value <= rhs.value;
    }

    friend bool operator>(Scaler lhs, Scaler rhs) {
        return lhs.value > rhs.value;
    }

    friend bool operator<(Scaler lhs, Scaler rhs) {
        return lhs.value < rhs.value;
    }

    friend Scaler operator+(Scaler lhs, Scaler rhs) {
        return make(lhs.value + rhs.value);
    }

    friend Scaler operator-(Scaler lhs, Scaler rhs) {
        return make(lhs.value - rhs.value);
    }

    friend Scaler operator*(Scaler lhs, Scaler rhs) {
        return make(((int64_t)lhs.value * (int64_t)rhs.value) >> 16);
    }

    friend Scaler operator/(Scaler lhs, Scaler rhs) {
        return make(((int64_t)lhs.value << 16) / (int64_t)rhs.value);
    }

    friend Scaler operator*(int32_t lhs, Scaler rhs) {
        return make(lhs * rhs.value);
    }

    friend Scaler operator*(Scaler lhs, int32_t rhs) {
        return make(lhs.value * rhs);
    }

    friend Scaler operator>>(Scaler lhs, int32_t rhs) {
        return make(lhs.value >> rhs);
    }

    friend Scaler operator<<(Scaler lhs, int32_t rhs) {
        return make(lhs.value << rhs);
    }

    friend Scaler rawScaler(int32_t value);
};
template<>
struct TypeMeta<Scaler> {
    static constexpr auto name = "Scaler";
    static constexpr auto tuple = std::make_tuple(
        Bind("value", &Scaler::value)
    );
};



inline Scaler rawScaler(int32_t value) {
    Scaler s;
    s.value = value;
    return s;    
}

// simple 2d vector class
class Vector2 {
public:

    Scaler x;
    Scaler y;

    Vector2() { }
    Vector2(Scaler x_, Scaler y_) : x(x_), y(y_) { }

    // comparison operators
    bool operator==(const Vector2& v) const
        { return (x == v.x && y == v.y); }

    bool operator!=(const Vector2& v)  const
        { return (x != v.x || y != v.y); }

    // friend functions
    friend Vector2 operator*(const Vector2& v, Scaler s) { return Vector2(v.x * s, v.y * s); }
    friend Vector2 operator/(const Vector2& v, Scaler s) { return Vector2(v.x / s, v.y / s); }
    friend Vector2 operator*(Scaler s, const Vector2& v) { return Vector2(s * v.x, s * v.y); }
    friend Vector2 operator/(Scaler s, const Vector2& v) { return Vector2(s / v.x, s / v.y); }

    // operations
    Vector2 operator-() const
        { return Vector2(-x, -y); }

    Vector2 operator+(const Vector2& v) const
        { return Vector2(x+v.x, y+v.y); }

    Vector2 operator-(const Vector2& v)  const
        { return Vector2(x-v.x, y-v.y); }

    Vector2& operator+=(const Vector2& v) {
        x += v.x;
        y += v.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vector2& operator*=(Scaler s) {
        x *= s;
        y *= s;
        return *this;
    }

    Vector2& operator*=(int32_t s) {
        x *= s;
        y *= s;
        return *this;
    }

    Vector2& operator/=(Scaler s) {
        Scaler inv = Scaler(1) / s;
        x *= inv;
        y *= inv;
        return *this;
    }

    // left and right hand perpendicular vectors	
    Vector2 left() const
        { return Vector2(-y, x); }

    Vector2 right() const
        { return Vector2(y, -x); }	
    
    // Dot products
    Scaler dot(const Vector2& v) const
        { return x * v.x + y * v.y; }

    Scaler leftDot(const Vector2& v) const
        { return y * v.x - x * v.y; }

    Scaler rightDot(const Vector2& v) const
        { return x * v.y - y * v.x; }

    
    /*
        Normalize using distance formula, WARNING: easily overflow!
    */
    Scaler length() const {
        int64_t v = (int64_t)x.raw() * (int64_t)x.raw() + (int64_t)y.raw() * (int64_t)y.raw();
        return Scaler::make((int32_t)Scaler::sqrt_i64(v));
    }

    /*
        Normalize using distance formula, WARNING: easily overflow!
    */
    int64_t lengthSq() const {
        return (int64_t)x.raw() * (int64_t)x.raw() + (int64_t)y.raw() * (int64_t)y.raw();
    }

    /*
        Normalize using equation from Black Art of 3D Game Programming, also used in the original DOOM engine
    */
    Scaler approximateOverestimateLength() const {
        uint32_t ax = (uint32_t)std::abs(x.raw());
        uint32_t ay = (uint32_t)std::abs(y.raw());
        return Scaler::make( ax + ay - (std::min(ax, ay)>>1) ); 
    }

    /*
        Normalize using equation from https://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
        max * Scaler::make(62974) + min * Scaler::make(26110)
        max * 0.9609f + min * 0.3984f

        NEED TO FIX THIS BY USING 64 bit mutliply
    */
    /*Scaler approximateLength() const {
        uint32_t ax = (uint32_t)std::abs(x.raw());
        uint32_t ay = (uint32_t)std::abs(y.raw());

        uint32_t min = std::min(ax, ay);
        uint32_t max = std::max(ax, ay);

        uint32_t approx = ((max * 62974) >> 16) + ((min * 26110) >> 16);

        return Scaler::make(approx);
    }*/
    Scaler approximateLength() const {
        Scaler ax = Scaler::abs(x);
        Scaler ay = Scaler::abs(y);
        Scaler min = Scaler::min(ax, ay);
        Scaler max = Scaler::max(ax, ay);
        return (max * Scaler::make(62974)) + (min * Scaler::make(26110));
    }

    /*
        For a custom normalize
    */
    void normalize(Scaler length) {
        if (length > Scaler(0)) {
            Scaler invLength = Scaler(1) / length;
            x *= invLength;
            y *= invLength;
        } else {
            x = Scaler(0);
            y = Scaler(0);
        }
    }

    Scaler normalize() {
        Scaler l = length();
        normalize(l);
        return l;
    }

    /*
        GreatestSideLength
    */
    Scaler greatestSideLength() const {
        return Scaler::max(Scaler::abs(x), Scaler::abs(y));
    }

    // function that attempted to combine approximations calculations (same as above actually)
    /*Vector2 specialNormalize() const { 
        Scaler ax = Scaler::abs(x);
        Scaler ay = Scaler::abs(y);
        Scaler approx = ax + ay - (Scaler::min(ax, ay)>>1);

        if (approx == Scaler(0)) {
            return Vector2(Scaler(0), Scaler(0));
        } 

        Scaler invapprox = Scaler(1) / approx;

        Scaler nx = x * invapprox;
        Scaler ny = y * invapprox;

        uint32_t nnx = (uint32_t)Scaler::abs(nx).raw();
        uint32_t nny = (uint32_t)Scaler::abs(ny).raw();

        uint32_t min = std::min(nnx, nny);
        uint32_t max = std::max(nnx, nny);

        uint32_t nlen = ((max * 62974) >> 16) + ((min * 26110) >> 16);

        if (nlen == 0) {
            return Vector2(Scaler(0), Scaler(0));
        }

        Scaler invlen = Scaler(1) / Scaler::make(nlen);

        return Vector2(nx * invlen, ny * invlen);
    }*/

    Vector2 midPoint(const Vector2& v) const
        { return Vector2( (x+v.x) * Scaler::make(32768), (y+v.y) * Scaler::make(32768) ); }

    // rotate vector by angle
    Vector2 rotate(Scaler a) const {
        Scaler c = Scaler::fastCos(a);
        Scaler s = Scaler::fastSin(a);
        return Vector2(x * c - y * s, x * s + y * c);
    }

    Vector2 reflect(const Vector2& n) const {
        // -2 * (V dot N)*N + V
        return Vector2(-2.0f * dot(n) * n + *this);
    }

};
template<>
struct TypeMeta<Vector2> {
    static constexpr auto name = "Vector2";
    static constexpr auto tuple = std::make_tuple(
        Bind("x", &Vector2::x),
        Bind("y", &Vector2::y)
    );
};



class Interval {
public:

	Scaler min;
	Scaler max;
	
	// Constructors and setters go here!	
	Interval() : min(0), max(0) { }
	
	Interval(Scaler _min, Scaler _max) {
		min = _min;
		max = _max;
	}
	
	void setTo(Scaler n) {
		min = max = n; 
	}
	
	void setTo(Scaler _min, Scaler _max) {
		min = _min;
		max = _max;
	}
		
	void move(Scaler n) {
		min += n;
		max += n;
	}
	
	bool reversed() const {
		return (min > max);
	}
	
	int dir(const Interval &i) const {
		if (min > i.max) return 1;
		if (max < i.min) return -1;
		return 0;
	}
	  
	Scaler proj(const Interval &i) {
		if ( dir(i) == 0 ) {
			Scaler d1 = i.min - max; 
			Scaler d2 = i.max - min;
			if ( Scaler::abs(d1) > Scaler::abs(d2) )
				return d2;
			else
				return d1;
		}
		return 0;
	}
	
	bool contain(Scaler n) const {
		if ( n >= min && n <= max )
			return true;
		return false;
	}
	
	Interval overlap(const Interval &i) const {
		Interval ret(min, max);
		if ( ret.contain(i.min) )
			ret.min = i.min;
		if ( ret.contain(i.max) )
			ret.max = i.max;
		return ret;
	}
	
	Scaler len() const {
		return max - min;
	}
	
	void extend(Scaler n) {
		if ( n < min ) min = n;
		if ( n > max ) max = n;
	}
	
	void fix() {
		if ( min > max ) {
            Scaler swap;
			swap = min;
			min = max;
			max = swap;
		}
	}
};
template<>
struct TypeMeta<Interval> {
    static constexpr auto name = "Interval";
    static constexpr auto tuple = std::make_tuple(
        Bind("min", &Interval::min),
        Bind("max", &Interval::max)
    );
};



#endif // FIXED_FIXED_HPP