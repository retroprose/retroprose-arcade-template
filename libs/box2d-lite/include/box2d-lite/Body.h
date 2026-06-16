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

#ifndef BOX2DLITE_BODY_H
#define BOX2DLITE_BODY_H


#include <box2d-lite\MathUtils.h>

struct Body
{
	Body();
	void Set(const Vec2& w, Scaler m);

	void AddForce(const Vec2& f)
	{
		force += f;
	}

	void AddForce(const Vec2& f, const Vec2& p)
	{
		//add linar force
		force += f;
		//and it's associated torque
		torque += Cross(p, f);
	}

	Vec2 position;
	Scaler rotation;

	Vec2 velocity;
	Scaler angularVelocity;

	Vec2 force;
	Scaler torque;

	Vec2 width;

	Scaler friction;
	Scaler mass, invMass;
	Scaler I, invI;
};


#endif
