/*
* Copyright (c) 2006-2009 Erin Catto http://www.gphysics.com
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability 
* of this software for any purpose.  
* It is provided "as is" without express or implied warranty.
*/


#include <box2d-lite/Arbiter.h>
#include <box2d-lite/Body.h>
#include <box2d-lite/system_world.hpp>


Arbiter::Arbiter(Body* b1, Body* b2)
{
	if (b1 < b2)
	{
		body1 = b1;
		body2 = b2;
	}
	else
	{
		body1 = b2;
		body2 = b1;
	}

	numContacts = Collide(contacts, body1, body2);

	friction = Sqrt(body1->friction * body2->friction);
}

void Arbiter::Update(Contact* newContacts, int numNewContacts)
{
	Contact mergedContacts[2];

	for (int i = 0; i < numNewContacts; ++i)
	{
		Contact* cNew = newContacts + i;
		int k = -1;
		for (int j = 0; j < numContacts; ++j)
		{
			Contact* cOld = contacts + j;
			if (cNew->feature.value == cOld->feature.value)
			{
				k = j;
				break;
			}
		}

		if (k > -1)
		{
			Contact* c = mergedContacts + i;
			Contact* cOld = contacts + k;
			*c = *cNew;
			if (SystemWorld::warmStarting)
			{
				c->Pn = cOld->Pn;
				c->Pt = cOld->Pt;
				c->Pnb = cOld->Pnb;
			}
			else
			{
				c->Pn = 0.0f;
				c->Pt = 0.0f;
				c->Pnb = 0.0f;
			}
		}
		else
		{
			mergedContacts[i] = newContacts[i];
		}
	}

	for (int i = 0; i < numNewContacts; ++i)
		contacts[i] = mergedContacts[i];

	numContacts = numNewContacts;
}


void Arbiter::PreStep(Scaler inv_dt)
{
	const Scaler k_allowedPenetration = Scaler(0.01f);
	Scaler k_biasFactor = SystemWorld::positionCorrection ? RawScaler(13107) : 0;

	for (int i = 0; i < numContacts; ++i)
	{
		Contact* c = contacts + i;

		Vec2 r1 = c->position - body1->position;
		Vec2 r2 = c->position - body2->position;

		// Precompute normal mass, tangent mass, and bias.
		Scaler rn1 = Dot(r1, c->normal);
		Scaler rn2 = Dot(r2, c->normal);
		Scaler kNormal = body1->invMass + body2->invMass;
		kNormal += body1->invI * (Dot(r1, r1) - rn1 * rn1) + body2->invI * (Dot(r2, r2) - rn2 * rn2);
		c->massNormal = 1.0f / kNormal;

		Vec2 tangent = Cross(c->normal, 1);
		Scaler rt1 = Dot(r1, tangent);
		Scaler rt2 = Dot(r2, tangent);
		Scaler kTangent = body1->invMass + body2->invMass;
		kTangent += body1->invI * (Dot(r1, r1) - rt1 * rt1) + body2->invI * (Dot(r2, r2) - rt2 * rt2);
		c->massTangent = 1.0f /  kTangent;

		c->bias = -k_biasFactor * inv_dt * Min(0, c->separation + k_allowedPenetration);

		if (SystemWorld::accumulateImpulses)
		{
			// Apply normal + friction impulse
			Vec2 P = c->Pn * c->normal + c->Pt * tangent;

			body1->velocity -= body1->invMass * P;
			body1->angularVelocity -= body1->invI * Cross(r1, P);

			body2->velocity += body2->invMass * P;
			body2->angularVelocity += body2->invI * Cross(r2, P);
		}
	}
}

void Arbiter::ApplyImpulse()
{
	Body* b1 = body1;
	Body* b2 = body2;

	for (int i = 0; i < numContacts; ++i)
	{
		Contact* c = contacts + i;
		c->r1 = c->position - b1->position;
		c->r2 = c->position - b2->position;

		// Relative velocity at contact
		Vec2 dv = b2->velocity + Cross(b2->angularVelocity, c->r2) - b1->velocity - Cross(b1->angularVelocity, c->r1);

		// Compute normal impulse
		Scaler vn = Dot(dv, c->normal);

		Scaler dPn = c->massNormal * (-vn + c->bias);

		if (SystemWorld::accumulateImpulses)
		{
			// Clamp the accumulated impulse
			Scaler Pn0 = c->Pn;
			c->Pn = Max(Pn0 + dPn, 0);
			dPn = c->Pn - Pn0;
		}
		else
		{
			dPn = Max(dPn, 0);
		}

		// Apply contact impulse
		Vec2 Pn = dPn * c->normal;

		b1->velocity -= b1->invMass * Pn;
		b1->angularVelocity -= b1->invI * Cross(c->r1, Pn);

		b2->velocity += b2->invMass * Pn;
		b2->angularVelocity += b2->invI * Cross(c->r2, Pn);

		// Relative velocity at contact
		dv = b2->velocity + Cross(b2->angularVelocity, c->r2) - b1->velocity - Cross(b1->angularVelocity, c->r1);

		Vec2 tangent = Cross(c->normal, 1);
		Scaler vt = Dot(dv, tangent);
		Scaler dPt = c->massTangent * (-vt);

		if (SystemWorld::accumulateImpulses)
		{
			// Compute friction impulse
			Scaler maxPt = friction * c->Pn;

			// Clamp friction
			Scaler oldTangentImpulse = c->Pt;
			c->Pt = Clamp(oldTangentImpulse + dPt, -maxPt, maxPt);
			dPt = c->Pt - oldTangentImpulse;
		}
		else
		{
			Scaler maxPt = friction * dPn;
			dPt = Clamp(dPt, -maxPt, maxPt);
		}

		// Apply contact impulse
		Vec2 Pt = dPt * tangent;

		b1->velocity -= b1->invMass * Pt;
		b1->angularVelocity -= b1->invI * Cross(c->r1, Pt);

		b2->velocity += b2->invMass * Pt;
		b2->angularVelocity += b2->invI * Cross(c->r2, Pt);
	}
}
