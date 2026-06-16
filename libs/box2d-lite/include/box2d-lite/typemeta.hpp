#ifndef BOX2DLITE_TYPEMETA_HPP
#define BOX2DLITE_TYPEMETA_HPP



#include <database/database.hpp>

#include <box2d-lite/MathUtils.h>
#include <box2d-lite/Body.h>
#include <box2d-lite/Joint.h>


// box2d type meta here!
template<>
struct TypeMeta<Scaler> {
    static constexpr auto name = "Scaler";
    static constexpr auto tuple = std::make_tuple(
        Bind("value", &Scaler::value)
    );
};
template struct Register<Scaler>;

template<>
struct TypeMeta<Vec2> {
    static constexpr auto name = "Vec2";
    static constexpr auto tuple = std::make_tuple(
        Bind("x", &Vec2::x),
        Bind("y", &Vec2::y)
    );
};
template struct Register<Vec2>;

template<>
struct TypeMeta<Mat22> {
    static constexpr auto name = "Mat22";
    static constexpr auto tuple = std::make_tuple(
        Bind("col1", &Mat22::col1),
        Bind("col2", &Mat22::col2)
    );
};
template struct Register<Mat22>;


template<>
struct TypeMeta<Body> {
    static constexpr auto name = "Body";
    static constexpr auto tuple = std::make_tuple(
        Bind("position", &Body::position),
        Bind("rotation", &Body::rotation),
        Bind("velocity", &Body::velocity),
        Bind("angularVelocity", &Body::angularVelocity),
        Bind("force", &Body::force),
        Bind("torque", &Body::torque),
        Bind("width", &Body::width),
        Bind("friction", &Body::friction),
        Bind("mass", &Body::mass),
        Bind("invMass", &Body::invMass),
        Bind("I", &Body::I),
        Bind("invI", &Body::invI)     
    );
};
template struct Register<Body>;

template<>
struct TypeMeta<Joint> {
    static constexpr auto name = "Joint";
    static constexpr auto tuple = std::make_tuple(
        Bind("M", &Joint::M),
        Bind("localAnchor1", &Joint::localAnchor1),
        Bind("localAnchor2", &Joint::localAnchor2),
        Bind("r1", &Joint::r1),
        Bind("r2", &Joint::r2),
        Bind("bias", &Joint::bias),
        Bind("P", &Joint::P),
        Bind("hbody1", &Joint::hbody1),
        Bind("hbody2", &Joint::hbody2),
        Bind("biasFactor", &Joint::biasFactor),
        Bind("softness", &Joint::softness)
    );
};
template struct Register<Joint>;







#endif // BOX2DLITE_TYPEMETA_HPP