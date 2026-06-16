#ifndef BOX2DLITE_SYSTEM_WORLD_HPP
#define BOX2DLITE_SYSTEM_WORLD_HPP


#include <box2d-lite/typemeta.hpp>

#include <box2d-lite/Arbiter.h>

typedef std::map<ArbiterKey, Arbiter>::iterator ArbIter;
typedef std::pair<ArbiterKey, Arbiter> ArbPair;

class SystemWorld : public Interface {
public:
    using base::base;

    void update();
    void Step(Scaler dt);
    void BroadPhase();

    Filter<Body> body;
    Filter<Joint> joint;

    std::map<ArbiterKey, Arbiter> arbiters;
	Vec2 gravity;
	int iterations;
	static bool accumulateImpulses;
	static bool warmStarting;
	static bool positionCorrection;

};
template<>
struct TypeMeta<SystemWorld> {
    static constexpr auto name = "SystemWorld";
    static constexpr auto tuple = std::make_tuple(
        Bind("body", &SystemWorld::body),
        Bind("joint", &SystemWorld::joint),
        Bind("gravity", &SystemWorld::gravity),
        Bind("iterations", &SystemWorld::iterations)
    );
};
template struct Register<SystemWorld>;



#endif // BOX2DLITE_SYSTEM_WORLD_HPP