/******************************************************************************/
/*!
\file		Collision.cpp
\author 	DigiPen
\par    	email: digipen\@digipen.edu
\date   	February 01, 20xx
\brief

Copyright (C) 20xx DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "main.h"

/**************************************************************************/
/*!

*/
/**************************************************************************/
bool CollisionIntersection_RectRect(const AABB &aabb1, const AEVec2 &vel1, 
									const AABB &aabb2, const AEVec2 &vel2)
{
	/*STATIC COLLISION*/
	if (!(aabb1.min.x > aabb2.max.x ||
		aabb1.max.x < aabb2.min.x ||
		aabb1.min.y > aabb2.max.y ||
		aabb1.max.y < aabb2.min.y
		)) return true;
	return false;

	///*DYNAMIC COLLISION*/
	//AEVec2 vb, a = vel1, b = vel2;
	//AEVec2Sub(&vb, &b, &a);

	//float tFirst = 0, tLast = g_dt;

	///*X-AXIS*/
	//if (vb.x < 0) {
	//	if (aabb1.min.x > aabb2.max.x) return false;
	//	if (aabb1.max.x < aabb2.min.x) {
	//		tFirst = max(((aabb1.max.x - aabb2.min.x) / vb.x), tFirst);
	//	}
	//	if (aabb1.min.x < aabb2.max.x) {
	//		tLast = min(((aabb1.min.x - aabb2.max.x) / vb.x), tLast);
	//	}
	//}

	//if (vb.x > 0) {
	//	if (aabb1.max.x < aabb2.min.x) return false;
	//	if (aabb1.min.x > aabb2.max.x) {
	//		tFirst = max(((aabb1.min.x - aabb2.max.x) / vb.x), tFirst);
	//	}
	//	if (aabb1.max.x > aabb2.min.x) {
	//		tLast = min(((aabb1.max.x - aabb2.min.x) / vb.x), tLast);
	//	}
	//	return true;
	//}

	///*Y-AXIS*/
	//if (vb.y < 0) {
	//	if (aabb1.min.y > aabb2.max.y) return false;
	//	if (aabb1.max.y < aabb2.min.y) {
	//		tFirst = max(((aabb1.max.y - aabb2.min.y) / vb.y), tFirst);
	//	}
	//	if (aabb1.min.y < aabb2.max.y) {
	//		tLast = min(((aabb1.min.y - aabb2.max.y) / vb.y), tLast);
	//	}
	//}

	//if (vb.y > 0) {
	//	if (aabb1.max.y < aabb2.min.y) return false;
	//	if (aabb1.min.y > aabb2.max.y) {
	//		tFirst = max(((aabb1.min.y - aabb2.max.y) / vb.y), tFirst);
	//	}
	//	if (aabb1.max.y > aabb2.min.y) {
	//		tLast = min(((aabb1.max.y - aabb2.min.y) / vb.y), tLast);
	//	}
	//	return true;
	//}

	//if (tFirst > tLast) return false;

	//else return true;
}