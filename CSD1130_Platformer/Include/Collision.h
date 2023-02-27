/******************************************************************************/
/*!
\file		Collision.h
\author 	DigiPen
\par    	email: digipen\@digipen.edu
\date   	February 01, 20xx
\brief

Copyright (C) 20xx DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#ifndef CSD1130_COLLISION_H_
#define CSD1130_COLLISION_H_

/**************************************************************************/
/*!

	*/
/**************************************************************************/
struct AABB
{
	AEVec2	min;
	AEVec2	max;
};

bool CollisionIntersection_RectRect(const AABB &aabb1, const AEVec2 &vel1, 
									const AABB &aabb2, const AEVec2 &vel2);


#endif // CSD1130_COLLISION_H_