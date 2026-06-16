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


#include <box2d-lite/Body.h>

Body::Body()
{
	position.Set(0, 0);
	rotation = 0;
	velocity.Set(0, 0);
	angularVelocity = 0;
	force.Set(0, 0);
	torque = 0;
	friction = RawScaler(13107);

	width.Set(1, 1);
	mass = k_max;
	invMass = 0;
	I = k_max;
	invI = 0;
}

void Body::Set(const Vec2& w, Scaler m)
{
	position.Set(0, 0);
	rotation = 0;
	velocity.Set(0, 0);
	angularVelocity = 0;
	force.Set(0, 0);
	torque = 0;
	friction = RawScaler(13107);

	width = w;
	mass = m;
	
	if (mass < k_max)
	{
		invMass = Scaler(1) / mass;
		I = mass * (width.x * width.x + width.y * width.y) / Scaler(12);
		invI = 1 / I;
	}
	else
	{
		invMass = 0;
		I = k_max;
		invI = 0;
	}
}
