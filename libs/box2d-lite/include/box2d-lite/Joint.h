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

#ifndef BOX2DLITE_JOINT_H
#define BOX2DLITE_JOINT_H


#include <database/database.hpp>

#include <box2d-lite/MathUtils.h>

struct Body;

struct Joint
{
	Joint() :
		hbody1(0), hbody2(0),
		P(0, 0),
		biasFactor(RawScaler(13107)), softness(0)
		{}

	void Set(Handle body1, Handle body2, const Vec2& anchor, Components& cp, const Filter<Body>& fr);

	void PreStep(Scaler inv_dt, Components& cp, Filter<Body>& fr);
	void ApplyImpulse(Components& cp, Filter<Body>& fr);

	Mat22 M;
	Vec2 localAnchor1, localAnchor2;
	Vec2 r1, r2;
	Vec2 bias;
	Vec2 P;		// accumulated impulse
	Handle hbody1;
	Handle hbody2;
	Scaler biasFactor;
	Scaler softness;
};


#endif