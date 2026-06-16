


/***************************

    System Implementation

**************************/

#include <box2d-lite/system_world.hpp>

bool SystemWorld::accumulateImpulses = true;
bool SystemWorld::warmStarting = true;
bool SystemWorld::positionCorrection = true;


void SystemWorld::update() {
    Step(RawScaler(1092));
}

void SystemWorld::BroadPhase() {
	// O(n^2) broad-phase
    for (auto ri : game.db.iterate(body)) {
        Body* bi = ri.value;
        for (auto rj : game.db.iterate(body)) {
            Body* bj = rj.value;
        
            if (bi->invMass == 0 && bj->invMass == 0)
				continue;

			Arbiter newArb(bi, bj);
			ArbiterKey key(bi, bj);

			if (newArb.numContacts > 0)
			{
				ArbIter iter = arbiters.find(key);
				if (iter == arbiters.end())
				{
					arbiters.insert(ArbPair(key, newArb));
				}
				else
				{
					iter->second.Update(newArb.contacts, newArb.numContacts);
				}
			}
			else
			{
				arbiters.erase(key);
			}

        }
    }

}

void SystemWorld::Step(Scaler dt) {
	//Scaler inv_dt = dt > 0 ? Scaler(1) / dt : 0;
	Scaler inv_dt = 60;

	// Determine overlapping bodies and update contact points.
	BroadPhase();

	// Integrate forces.

    for (auto r : game.db.iterate(body)) 
	{
        Body* b = r.value;

		Vec2 vel = dt * (gravity + b->invMass * b->force);

		//printf("invMass: %f\n", (float)(b->invMass));
			
		//printf("force: %f, %f\n", (float)(b->force.x), (float)(b->force.y));

		//printf("grav: %f, %f\n", (float)(gravity.x), (float)(gravity.y));
		
		//printf("vel: %f, %f\n", (float)(vel.x), (float)(vel.y));

		if (b->invMass == 0)
			continue;

		b->velocity += dt * (gravity + b->invMass * b->force);
		b->angularVelocity += dt * b->invI * b->torque;
	}

	// Perform pre-steps.
	for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb)
	{
		arb->second.PreStep(inv_dt);
	}

    for (auto r : game.db.iterate(joint))
	{
		r.value->PreStep(inv_dt, game.db, body);	
	}

	// Perform iterations
	for (int i = 0; i < iterations; ++i)
	{
		for (ArbIter arb = arbiters.begin(); arb != arbiters.end(); ++arb)
		{
			arb->second.ApplyImpulse();
		}

		for (auto r : game.db.iterate(joint))
		{
			r.value->ApplyImpulse(game.db, body);
		}
	}

	// Integrate Velocities
	for (auto r : game.db.iterate(body))
	{
		Body* b = r.value;

		b->position += dt * b->velocity;
		b->rotation += dt * b->angularVelocity;

		b->force.Set(0, 0);
		b->torque = 0;
	}
}

