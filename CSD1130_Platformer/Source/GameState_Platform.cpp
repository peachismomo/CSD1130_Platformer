/******************************************************************************/
/*!
\file		GameState_Platform.cpp
\author 	Ian Chua
\par    	email: i.chua@digipen.edu
\date   	February 28, 2023
\brief

Copyright (C) 2023 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/

#include "main.h"
#include <fstream>
#include <iostream>

/******************************************************************************/
/*!
	Defines
*/
/******************************************************************************/
const unsigned int	GAME_OBJ_NUM_MAX		= 32;	//The total number of different objects (Shapes)
const unsigned int	GAME_OBJ_INST_NUM_MAX	= 2048;	//The total number of different game object instances
const unsigned int	PARTICLES_MAX			= 200;

//Gameplay related variables and values
const float			GRAVITY					= -20.0f;
const float			JUMP_VELOCITY			= 11.0f;
const float			MOVE_VELOCITY_HERO		= 4.0f;
const float			MOVE_VELOCITY_ENEMY		= 7.5f;
const double		ENEMY_IDLE_TIME			= 2.0;
const int			HERO_LIVES				= 3;
const float			BOUNDING_RECT_SIZE		= 1.0f;

//Particle related variables and values
const float			EMISSION_RATE			= 50.f;			// emission rate (particles per second)
const float			VELOCITY_MAX			= 7.f;			// max velocity
const float			VELOCITY_MIN			= 5.5f;			// min velocity
const float			LIFESPAN_MAX			= 0.8f;			// max lifespan (seconds)
const float			LIFESPAN_MIN			= 0.6f;			// min lifespan (seconds)
const float			SCALE_MAX				= 0.5f;
const float			SCALE_MIN				= 0.1f;
const float			TRANSPARENCY_MAX		= 0.4f;
const float			TRANSPARENCY_MIN		= 0.8f;

//Flags
const unsigned int	FLAG_ACTIVE				= 0x00000001;
const unsigned int	FLAG_VISIBLE			= 0x00000002;
const unsigned int	FLAG_NON_COLLIDABLE		= 0x00000004;

//Collision flags
const unsigned int	COLLISION_LEFT			= 0x00000001;	//0001
const unsigned int	COLLISION_RIGHT			= 0x00000002;	//0010
const unsigned int	COLLISION_TOP			= 0x00000004;	//0100
const unsigned int	COLLISION_BOTTOM		= 0x00000008;	//1000


enum TYPE_OBJECT
{
	TYPE_OBJECT_EMPTY,			//0
	TYPE_OBJECT_COLLISION,		//1
	TYPE_OBJECT_HERO,			//2
	TYPE_OBJECT_ENEMY1,			//3
	TYPE_OBJECT_COIN,			//4
	TYPE_OBJECT_PARTICLE		//5
};

//State machine states
enum STATE
{
	STATE_NONE,
	STATE_GOING_LEFT,
	STATE_GOING_RIGHT
};

//State machine inner states
enum INNER_STATE
{
	INNER_STATE_ON_ENTER,
	INNER_STATE_ON_UPDATE,
	INNER_STATE_ON_EXIT
};

enum DIRECTION 
{
	FACE_LEFT,
	FACE_RIGHT
};

/******************************************************************************/
/*!
	Struct/Class Definitions
*/
/******************************************************************************/
struct GameObj
{
	unsigned int		type;		// object type
	AEGfxVertexList *	pMesh;		// pbject
};

struct Particle {
	GameObj*		pObject;
	f32				scale;
	f32				lifespan;
	f32				velCurr;
	AEVec2			posCurr;
	f32				transparency;
	unsigned int	flag;
	AEMtx33			transform;
};

struct GameObjInst
{
	GameObj *		pObject;			// pointer to the 'original'
	unsigned int	flag;				// bit flag or-ed together
	float			scale;				// object scale
	AEVec2			posCurr;			// object current position
	AEVec2			velCurr;			// object current velocity
	float			dirCurr;			// object current direction
	AEMtx33			transform;			// object drawing matrix
	AABB			boundingBox;		// object bouding box that encapsulates the object
	enum			DIRECTION face;		// direction the object is facing (left/right)

	//Collision Flags
	int				gridCollisionFlag;
	int				gridCollisionFlagPrev;

	// pointer to custom data specific for each object type
	void*			pUserData;

	//State of the object instance
	enum			STATE state;
	enum			INNER_STATE innerState;

	//General purpose counter (This variable will be used for the enemy state machine)
	double			counter;
};


/******************************************************************************/
/*!
	File globals
*/
/******************************************************************************/
static int				HeroLives;		// Number of hero lives
static int				Hero_Initial_X;	// Initial x position of hero
static int				Hero_Initial_Y; // Initial y position of hero
static int				TotalCoins;		// Total coins in level
static float			ParticleDelay;	// Delay between each particle generation
static float			ParticleTimer;	// Timer to check if the delay is up
static float			CellWidth;
static float			CellHeight;

// list of original objects
static GameObj			*sGameObjList;
static unsigned int		sGameObjNum;

// list of object instances
static GameObjInst		*sGameObjInstList;
static unsigned int		sGameObjInstNum;

// particle array
static Particle			*sParticlesList;
static unsigned int		sParticlesNum;

//Binary map data
static int				**MapData;
static int				**BinaryCollisionArray;
static int				BINARY_MAP_WIDTH;
static int				BINARY_MAP_HEIGHT;
static GameObjInst		*pBlackInstance;
static GameObjInst		*pWhiteInstance;
static AEMtx33			MapTransform;

/*MAP FUNCTIONS*/
int						GetCellValue(int X, int Y);
int						CheckInstanceBinaryMapCollision(float PosX, float PosY, 
														float scaleX, float scaleY);
void					SnapToCell(float *Coordinate);
int						ImportMapDataFromFile(char *FileName);
void					FreeMapData(void);

/*GAME OBJECT INSTANCE FUNCTIONS*/
static GameObjInst*		gameObjInstCreate (unsigned int type,	float scale, 
											AEVec2* pPos,		AEVec2* pVel, 
											float dir,			enum STATE startState);
static void				gameObjInstDestroy(GameObjInst* pInst);

/*POINTER TO HERO*/
static GameObjInst* pHero;

/*STATE MACHINE FUNCTIONS*/
void					EnemyStateMachine(GameObjInst* pInst);

/*PARTICLE FUNCTIONS*/
f32						PRNG(f32 min, f32 max);
void					CreateParticle(AEVec2 pos);

/******************************************************************************/
/*!
	"Load" function of this state
*/
/******************************************************************************/
void GameStatePlatformLoad(void)
{
	sGameObjList		= (GameObj*)	calloc(GAME_OBJ_NUM_MAX,		sizeof(GameObj)		);
	sGameObjInstList	= (GameObjInst*)calloc(GAME_OBJ_INST_NUM_MAX,	sizeof(GameObjInst)	);
	sParticlesList		= (Particle*)	calloc(PARTICLES_MAX,			sizeof(Particle)	);
	sGameObjNum			= 0;


	GameObj* pObj;

	//Creating the black object
	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_OBJECT_EMPTY;


	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFF000000, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF000000, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFF000000, 0.0f, 0.0f);
	
	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFF000000, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF000000, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFF000000, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");
	
	//Creating the white object
	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_OBJECT_COLLISION;


	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFFFFFFFF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFFFFFFF, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFFFFFFFF, 0.0f, 0.0f);
	
	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFFFFFFFF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFFFFFFF, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFFFFFFFF, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	//Creating the hero object
	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_OBJECT_HERO;


	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFF0000FF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF0000FF, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFF0000FF, 0.0f, 0.0f);
	
	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFF0000FF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF0000FF, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFF0000FF, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	//Creating the enemey1 object
	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_OBJECT_ENEMY1;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFFFF0000, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFFF0000, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFFFF0000, 0.0f, 0.0f);
	
	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFFFF0000, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFFF0000, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFFFF0000, 0.0f, 0.0f);

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	//Creating the Coin object
	pObj		= sGameObjList + sGameObjNum++;
	pObj->type	= TYPE_OBJECT_COIN;


	AEGfxMeshStart();
	//Creating the circle shape
	int Parts = 12;
	for (float i = 0; i < Parts; ++i)
	{
		AEGfxTriAdd(
			0.0f,									0.0f,									0xFFFFFF00, 0.0f, 0.0f,
			cosf(i * 2 * PI / Parts) * 0.5f,		sinf(i * 2 * PI / Parts) * 0.5f,		0xFFFFFF00, 0.0f, 0.0f,
			cosf((i + 1) * 2 * PI / Parts) * 0.5f,	sinf((i + 1) * 2 * PI / Parts) * 0.5f,	0xFFFFFF00, 0.0f, 0.0f);
	}

	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "fail to create object!!");

	/*CREATE 4 PARTICLES WITH DIFFERENT COLORS*/
	/*PARTICLE 1*/
	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_OBJECT_PARTICLE;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFF00FFFF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF00FFFF, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFF00FFFF, 0.0f, 0.0f);
	
	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFF00FFFF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF00FFFF, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFF00FFFF, 0.0f, 0.0f);
	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "failed to create particle object.");

	/*PARTICLE 2*/
	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_OBJECT_PARTICLE;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFFADD8E6, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFADD8E6, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFFADD8E6, 0.0f, 0.0f);

	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFFADD8E6, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFADD8E6, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFFADD8E6, 0.0f, 0.0f);
	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "failed to create particle object.");

	/*PARTICLE 3*/
	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_OBJECT_PARTICLE;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f, 0xFFF0FFFF, 0.0f, 0.0f,
		0.5f,	-0.5f, 0xFFF0FFFF, 0.0f, 0.0f,
		-0.5f,	0.5f, 0xFFF0FFFF, 0.0f, 0.0f);

	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFFF0FFFF, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFFF0FFFF, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFFF0FFFF, 0.0f, 0.0f);
	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "failed to create particle object.");

	/*PARTICLE 4*/
	pObj = sGameObjList + sGameObjNum++;
	pObj->type = TYPE_OBJECT_PARTICLE;

	AEGfxMeshStart();
	AEGfxTriAdd(
		-0.5f,	-0.5f,	0xFF89CFF0, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF89CFF0, 0.0f, 0.0f,
		-0.5f,	0.5f,	0xFF89CFF0, 0.0f, 0.0f);

	AEGfxTriAdd(
		-0.5f,	0.5f,	0xFF89CFF0, 0.0f, 0.0f,
		0.5f,	-0.5f,	0xFF89CFF0, 0.0f, 0.0f,
		0.5f,	0.5f,	0xFF89CFF0, 0.0f, 0.0f);
	pObj->pMesh = AEGfxMeshEnd();
	AE_ASSERT_MESG(pObj->pMesh, "failed to create particle object.");
	/*CREATE PARTICLES END*/

	//Setting intital binary map values
	MapData					= 0;
	BinaryCollisionArray	= 0;
	BINARY_MAP_WIDTH		= 0;
	BINARY_MAP_HEIGHT		= 0;

	// Choose level data
	if (gGameStateCurr == GS_PLATFORM) {
		if (!ImportMapDataFromFile("../Resources/Levels/Exported.txt"))
			gGameStateNext = GS_QUIT;
	}
	else if (gGameStateCurr == GS_PLATFORM2) {
		if(!ImportMapDataFromFile("../Resources/Levels/Exported2.txt"))
			gGameStateNext = GS_QUIT;
	}

	/*NORMALIZED COORDINATE SYSTEM TRANSFORMATION MATRIX*/
	AEMtx33 scale, trans;

	AEMtx33Trans(&trans, (f32)-BINARY_MAP_WIDTH / 2.f,		(f32)-BINARY_MAP_HEIGHT / 2.f	);
	AEMtx33Scale(&scale, (f32)AEGetWindowWidth() / 20.f,	(f32)AEGetWindowHeight() / 20.f	);

	AEMtx33Concat(&MapTransform, &scale, &trans);
}

/******************************************************************************/
/*!
	"Initialize" function of this state
*/
/******************************************************************************/
void GameStatePlatformInit(void)
{
	int i, j;

	/*INITIALIZE VALUES*/
	pHero			= 0;
	pBlackInstance	= 0;
	pWhiteInstance	= 0;
	TotalCoins		= 0;
	HeroLives		= HERO_LIVES;
	ParticleDelay	= 1.f / EMISSION_RATE; // number of times per second
	ParticleTimer	= 0;

	/*BLACK TILE OBJECT INSTANCE*/
	pBlackInstance			= gameObjInstCreate(TYPE_OBJECT_EMPTY, 1.0f, 0, 0, 0.0f, STATE_NONE);
	pBlackInstance->flag	^= FLAG_VISIBLE;
	pBlackInstance->flag	|= FLAG_NON_COLLIDABLE;

	/*WHITE TILE OBJECT INSTANCE*/
	pWhiteInstance			= gameObjInstCreate(TYPE_OBJECT_COLLISION, 1.0f, 0, 0, 0.0f, STATE_NONE);
	pWhiteInstance->flag	^= FLAG_VISIBLE;
	pWhiteInstance->flag	|= FLAG_NON_COLLIDABLE;

	GameObjInst *pInst;
	AEVec2 Pos;

	/*CREATING GAME OBJECT INSTANCES*/
	for(i = 0; i < BINARY_MAP_WIDTH; ++i)
		for(j = 0; j < BINARY_MAP_HEIGHT; ++j)
		{
			int data = MapData[i][j]; // Get object type from map data
			AEVec2Set(&Pos, (f32)i + 0.5f, (f32)j + 0.5f);
			AEVec2 zero = { 0,0 };

			switch (data) {

			case(TYPE_OBJECT_HERO):
				pHero = gameObjInstCreate(TYPE_OBJECT_HERO, 1.0f, &Pos, 0, 0.f, STATE_NONE);
				Hero_Initial_X = (int)i;
				Hero_Initial_Y = (int)j;
				break;

			case(TYPE_OBJECT_ENEMY1):
				pInst = gameObjInstCreate(TYPE_OBJECT_ENEMY1, 1.0f, &Pos, 0, 0.f, STATE_GOING_RIGHT);
				break;

			case(TYPE_OBJECT_COIN):
				pInst = gameObjInstCreate(TYPE_OBJECT_COIN, 1.0f, &Pos, 0, 0.f, STATE_NONE);
				TotalCoins++;
				break;

			default:
				break;
			}
		}
}

/******************************************************************************/
/*!
	"Update" function of this state
*/
/******************************************************************************/
void GameStatePlatformUpdate(void)
{
	int i;
	GameObjInst *pInst;

	/*HANDLE INPUT*/
	/*MOVE LEFT AND RIGHT*/
	if (AEInputCheckCurr(AEVK_RIGHT)) {
		pHero->velCurr.x	= MOVE_VELOCITY_HERO;
		pHero->face			= FACE_RIGHT;
	}
	
	else if (AEInputCheckCurr(AEVK_LEFT)) {
		pHero->velCurr.x	= -MOVE_VELOCITY_HERO;
		pHero->face			= FACE_LEFT;
	}

	else 
		pHero->velCurr.x = 0.f;

	/*JUMP MOVEMENT*/
	if (AEInputCheckTriggered(AEVK_SPACE)) {
		//Player can jump as long as they are colliding with the bottom
		if (pHero->gridCollisionFlag & COLLISION_BOTTOM)
			pHero->velCurr.y = JUMP_VELOCITY;
		
		/*
		 * Player can jump if the previous collison was on the left or right 
		 * and there is currently no collision on the left or right
		 */
		else if ((pHero->gridCollisionFlagPrev & COLLISION_LEFT || pHero->gridCollisionFlagPrev & COLLISION_RIGHT) &&
				!(pHero->gridCollisionFlag & COLLISION_LEFT || pHero->gridCollisionFlag & COLLISION_RIGHT))
		{
			pHero->velCurr.y = JUMP_VELOCITY;
		}

		/*Reset prev collision flag to prevent double jumping mid air*/
		pHero->gridCollisionFlagPrev = 0;
	}

	if (AEInputCheckCurr(AEVK_ESCAPE))
		gGameStateNext = GS_MAIN;
	/*HANDLE INPUT END*/

	/*PARTICLE GENERATION*/
	/*CREATE PARTICLE BASED ON EMISSION RATE*/
	ParticleTimer -= g_dt; // Decrement timer
	if (ParticleTimer < 0) {
		CreateParticle(pHero->posCurr); // Create new particle
		ParticleTimer = ParticleDelay; // Reset timer
	}
	/*PARTICLE GENERATION END*/

	/*PARTICLE BEHAVIOUR*/
	for (i = 0; i < PARTICLES_MAX; i++)
	{
		Particle* particle = sParticlesList + i;

		if (particle->flag) 
		{
			/*Decrement particle values*/
			particle->lifespan		-= g_dt;
			particle->scale			-= g_dt / 3.f;
			particle->transparency	-= g_dt;
			particle->posCurr.y		+= particle->velCurr * g_dt;
			particle->posCurr.x		+= pHero->face ? 1.f * g_dt : -1.f * g_dt;
		}

		/*Delete particle if the lifespan or scale becomes 0*/
		if (particle->lifespan < 0 || 
			particle->scale < 0) 
			particle->flag = 0;

	}
	/*PARTICLE BEHAVIOUR END*/

	/*OBJECT PHYSICS*/
	for(i = 0; i < GAME_OBJ_INST_NUM_MAX; ++i)
	{
		pInst = sGameObjInstList + i;

		// skip non-active object
		if (0 == (pInst->flag & FLAG_ACTIVE) || 
			pInst->pObject->type == TYPE_OBJECT_COIN)
			continue;

		// Apply gravity
		pInst->velCurr.y = GRAVITY * g_dt + pInst->velCurr.y;

		// Apply state machine
		if (pInst->pObject->type == TYPE_OBJECT_ENEMY1) {
			EnemyStateMachine(pInst);
		}
	} // OBJECT PHYSICS END

	  /*UPDATE POSITION*/
	for(i = 0; i < GAME_OBJ_INST_NUM_MAX; ++i)
	{
		pInst = sGameObjInstList + i;

		// skip non-active object
		if (0 == (pInst->flag & FLAG_ACTIVE))
			continue;

		// Update position (movememnt)
		pInst->posCurr.x = pInst->velCurr.x * g_dt + pInst->posCurr.x;
		pInst->posCurr.y = pInst->velCurr.y * g_dt + pInst->posCurr.y;

		AEVec2Set(&pInst->boundingBox.min, -BOUNDING_RECT_SIZE / 2.f + pInst->posCurr.x,	-BOUNDING_RECT_SIZE / 2.f + pInst->posCurr.y);
		AEVec2Set(&pInst->boundingBox.max, BOUNDING_RECT_SIZE / 2.f + pInst->posCurr.x,		BOUNDING_RECT_SIZE / 2.f + pInst->posCurr.y	);
	} // UPDATE POSITION END

	/*GRID COLLISION*/
	for(i = 0; i < GAME_OBJ_INST_NUM_MAX; ++i)
	{
		pInst = sGameObjInstList + i;

		// skip non-active object instances
		if (0 == (pInst->flag & FLAG_ACTIVE))
			continue;

		// Check collision
		pInst->gridCollisionFlag = CheckInstanceBinaryMapCollision(pInst->posCurr.x, pInst->posCurr.y, pInst->scale, pInst->scale);

		// Collision for top and bottom
		if (pInst->gridCollisionFlag & COLLISION_BOTTOM || pInst->gridCollisionFlag & COLLISION_TOP) {
			// Collision response
			SnapToCell(&pInst->posCurr.y);
			pInst->velCurr.y = 0;
		}

		// Collision for left and right
		if (pInst->gridCollisionFlag & COLLISION_LEFT || pInst->gridCollisionFlag & COLLISION_RIGHT) {
			// Update previous collison flag
			pInst->gridCollisionFlagPrev = pInst->gridCollisionFlag;
			// Collision response
			SnapToCell(&pInst->posCurr.x);
			pInst->velCurr.x = 0;
		}
	} // GRID COLLISION END

	/*RECT-RECT COLLISION*/
	for(i = 0; i < GAME_OBJ_INST_NUM_MAX; ++i)
	{
		pInst = sGameObjInstList + i;

		/*SKIP IF INACTIVE OR NON COLLIDABLE*/
		if (0 == (pInst->flag & FLAG_ACTIVE) || 
			pInst->flag & FLAG_NON_COLLIDABLE || 
			pInst->pObject->type == TYPE_OBJECT_HERO) continue;
		
		/*COLLISION WITH ENEMY*/
		if (pInst->pObject->type == TYPE_OBJECT_ENEMY1) {
			/*COLLISiON RESPONSE*/
			if (CollisionIntersection_RectRect(	{ pInst->boundingBox.min, pInst->boundingBox.max }, pInst->velCurr,
												{ pHero->boundingBox.min, pHero->boundingBox.max }, pHero->velCurr))
			{
				HeroLives--; // Decrement lives

				/*IF HERO STILL HAVE LIVES, RESET POSITION, IF NOT RESTART LEVEL*/
				!HeroLives ? gGameStateNext = GS_RESTART :
					AEVec2Set(&pHero->posCurr, (f32)Hero_Initial_X + 0.5f, (f32)Hero_Initial_Y + 0.5f);
			}

		}

		/*COLLISION WITH COIN*/
		if (pInst->pObject->type == TYPE_OBJECT_COIN) {
			/*COLLISiON RESPONSE*/
			if (CollisionIntersection_RectRect(	{ pInst->boundingBox.min, pInst->boundingBox.max }, pInst->velCurr,
												{ pHero->boundingBox.min, pHero->boundingBox.max }, pHero->velCurr))
			{
				TotalCoins--; // Decrement coin count
				gameObjInstDestroy(pInst); // Destroy coin instance
				if (0 == TotalCoins) {
					// if all coins collected, go to next level, or back to main menu if current level is last level
					gGameStateNext = gGameStateCurr == GS_PLATFORM ? GS_PLATFORM2 : GS_MAIN;
				}
			}
		}

	} // RECT-RECT COLLISION END

	
	/*OBJECT INSTANCE TRANSFORMATION MATRIX*/
	for(i = 0; i < GAME_OBJ_INST_NUM_MAX; ++i)
	{
		AEMtx33 scale, rot, trans;
		pInst = sGameObjInstList + i;

		// skip non-active object
		if (0 == (pInst->flag & FLAG_ACTIVE))
			continue;

		// TRANSFORMATION MATRIX
		AEMtx33Scale	(&scale,	pInst->scale,		pInst->scale	);
		AEMtx33Rot		(&rot,		pInst->dirCurr						);
		AEMtx33Trans	(&trans,	pInst->posCurr.x,	pInst->posCurr.y);

		AEMtx33Concat	(&pInst->transform, &rot,	&scale				);
		AEMtx33Concat	(&pInst->transform, &trans,	&pInst->transform	);
	} // OBJECT INSTANCE TRANSFORMATION MATRIX END

	/*PARTICLE TRANSFORMATION MATRIX*/
	for (i = 0; i < PARTICLES_MAX; ++i)
	{
		AEMtx33 scale, rot, trans;
		Particle * particle = sParticlesList + i;

		// skip non-active object
		if (0 == (particle->flag))
			continue;

		AEMtx33Scale	(&scale,				particle->scale,		particle->scale		);
		AEMtx33Rot		(&rot, 0);
		AEMtx33Trans	(&trans,				particle->posCurr.x,	particle->posCurr.y	);
		AEMtx33Concat	(&particle->transform,	&rot,					&scale				);
		AEMtx33Concat	(&particle->transform,	&trans,					&particle->transform);
	} // PARTICLE TRANSFORMATION MATRIX END

	/*CAMERA POSITION*/
	if (gGameStateCurr == GS_PLATFORM2) {
		f32 width	= (f32)AEGetWindowWidth() / BINARY_MAP_WIDTH;
		f32 height	= (f32)AEGetWindowHeight() / BINARY_MAP_HEIGHT;


		f32 xClamp = AEClamp((pHero->posCurr.x - BINARY_MAP_WIDTH / 2.f) * (f32)AEGetWindowWidth() / 20.f,		-width * (BINARY_MAP_WIDTH / 2.f + 2),		width * (BINARY_MAP_WIDTH / 2.f + 2)	); // Between max x and min x
		f32 yClamp = AEClamp((pHero->posCurr.y - BINARY_MAP_HEIGHT / 2.f) * (f32)AEGetWindowHeight() / 20.f,	-height * (BINARY_MAP_HEIGHT / 2.f + 3),	height * (BINARY_MAP_HEIGHT / 2.f + 3)	); // Between max x and min x

		AEGfxSetCamPosition(xClamp, yClamp);
	}
	else AEGfxSetCamPosition(0.f, 0.f);
	/*CAMERA POSITION END*/
}

/******************************************************************************/
/*!
	Render all active GameObjInst.
*/
/******************************************************************************/
void GameStatePlatformDraw(void)
{
	/*RENDER SETTINGS*/
	AEGfxSetBackgroundColor	(0.f, 0.f, 0.f);
	AEGfxSetRenderMode		(AE_GFX_RM_COLOR);
	/*RENDER SETTINGS END*/

	int i, j;
	AEMtx33 cellTranslation, cellFinalTransformation;

	/*RENDER TILE MAP*/
	for(i = 0; i < BINARY_MAP_WIDTH; ++i)
		for(j = 0; j < BINARY_MAP_HEIGHT; ++j)
		{
			/*Get position of tile*/
			AEMtx33Trans	(&cellTranslation,			i + 0.5f,		j + 0.5f		); // Offset 0.5 => binary map is bottom left
			AEMtx33Concat	(&cellFinalTransformation,	&MapTransform,	&cellTranslation); // Apply map transformation

			AEGfxSetTransform(cellFinalTransformation.m);

			/*Draw*/
			if (MapData[i][j] == TYPE_OBJECT_EMPTY) 
				AEGfxMeshDraw(pBlackInstance->pObject->pMesh, AE_GFX_MDM_TRIANGLES);
			
			else if (MapData[i][j] == TYPE_OBJECT_COLLISION)
				AEGfxMeshDraw(pWhiteInstance->pObject->pMesh, AE_GFX_MDM_TRIANGLES);
			
		}
	/*RENDER TILE MAP END*/

	/*RENDER INSTANCES*/
	for (i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst* pInst = sGameObjInstList + i;

		// skip non-active object
		if (0 == (pInst->flag & FLAG_ACTIVE) || 0 == (pInst->flag & FLAG_VISIBLE))
			continue;

		// Apply map transformation to object instance transformation
		AEMtx33Concat		(&cellFinalTransformation, &MapTransform, &pInst->transform);
		AEGfxSetTransform	(cellFinalTransformation.m);

		AEGfxMeshDraw(pInst->pObject->pMesh, AE_GFX_MDM_TRIANGLES);
	}

	for (i = 0; i < PARTICLES_MAX; i++)
	{
		
		Particle* particle = sParticlesList + i;
		if (0 == particle->flag) continue;

		// Apply map transformation to particle transformation
		AEMtx33Concat		(&cellFinalTransformation, &MapTransform, &particle->transform);
		AEGfxSetTransform	(cellFinalTransformation.m);

		AEGfxMeshDraw(particle->pObject->pMesh, AE_GFX_MDM_TRIANGLES);
	}
	/*RENDER INSTANCES END*/

	char strBuffer[32];
	memset(strBuffer, 0, 32 * sizeof(char));

	/*SHOW TEXT ON SCREEN*/
	sprintf_s	(strBuffer, "Coins Left: %d", TotalCoins);
	AEGfxPrint	(fontId, strBuffer, -.9f, .9f, 1.f, 0.f, 0.f, 1.f);

	sprintf_s	(strBuffer, "Lives: %d", HeroLives);
	AEGfxPrint	(fontId, strBuffer, .7f, .9f, 1.f, 0.f, 0.f, 1.f);
}

/******************************************************************************/
/*!
	Destroys all GameObjInst.
*/
/******************************************************************************/
void GameStatePlatformFree(void)
{
	// kill all object in the list
	for (unsigned int i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
		gameObjInstDestroy(sGameObjInstList + i);
}

/******************************************************************************/
/*!
	Frees all mesh data from the sGameObjList and frees allocated memory
*/
/******************************************************************************/
void GameStatePlatformUnload(void)
{
	// free all CREATED mesh
	for (u32 i = 0; i < sGameObjNum; i++)
		AEGfxMeshFree(sGameObjList[i].pMesh);

	/*********
	Free the map data
	*********/
	FreeMapData();
	free(sGameObjInstList);
	free(sGameObjList);
	free(sParticlesList);
}

/******************************************************************************/
/*!
	Creates object instance
*/
/******************************************************************************/
GameObjInst* gameObjInstCreate(unsigned int type, float scale, 
							   AEVec2* pPos, AEVec2* pVel, 
							   float dir, enum STATE startState)
{
	AEVec2 zero;
	AEVec2Zero(&zero);

	AE_ASSERT_PARM(type < sGameObjNum);
	
	// loop through the object instance list to find a non-used object instance
	for (unsigned int i = 0; i < GAME_OBJ_INST_NUM_MAX; i++)
	{
		GameObjInst* pInst = sGameObjInstList + i;

		// check if current instance is not used
		if (pInst->flag == 0)
		{
			// it is not used => use it to create the new instance
			pInst->pObject				 = sGameObjList + type;
			pInst->flag					 = FLAG_ACTIVE | FLAG_VISIBLE;
			pInst->scale				 = scale;
			pInst->posCurr				 = pPos ? *pPos : zero;
			pInst->velCurr				 = pVel ? *pVel : zero;
			pInst->dirCurr				 = dir;
			pInst->face					 = FACE_LEFT;
			pInst->pUserData			 = 0;
			pInst->gridCollisionFlag	 = 0;
			pInst->gridCollisionFlagPrev = 0;
			pInst->state				 = startState;
			pInst->innerState			 = INNER_STATE_ON_ENTER;
			pInst->counter				 = 0;
			
			// return the newly created instance
			return pInst;
		}
	}

	return 0;
}

/******************************************************************************/
/*!
	Destroys object instance
*/
/******************************************************************************/
void gameObjInstDestroy(GameObjInst* pInst)
{
	// if instance is destroyed before, just return
	if (pInst->flag == 0)
		return;

	// zero out the flag
	pInst->flag = 0;
}

/******************************************************************************/
/*!
	Gets cell value from binary collision data
*/
/******************************************************************************/
int GetCellValue(int X, int Y)
{
	if (0 <= X && X < BINARY_MAP_WIDTH &&
		0 <= Y && Y < BINARY_MAP_HEIGHT)
	{
		return BinaryCollisionArray[X][Y];
	}
	return 0;
}

/******************************************************************************/
/*!
	Checks collision of object based on the binary collision map
*/
/******************************************************************************/
int CheckInstanceBinaryMapCollision(float PosX, float PosY, float scaleX, float scaleY)
{
	//At the end of this function, "Flag" will be used to determine which sides
	//of the object instance are colliding. 2 hot spots will be placed on each side.

	int flag{};

	float x1, y1, x2, y2;
	/*RIGHT*/
	x1 = PosX + scaleX / 2.f;
	y1 = PosY + scaleY / 4.f;

	x2 = PosX + scaleX / 2.f;
	y2 = PosY - scaleY / 4.f;
	flag = GetCellValue((int)x1, (int)y1) || GetCellValue((int)x2, (int)y2) ?
		flag | COLLISION_RIGHT : flag;

	/*LEFT*/
	x1 = PosX - scaleX / 2.f;
	y1 = PosY + scaleY / 4.f;

	x2 = PosX - scaleX / 2.f;
	y2 = PosY - scaleY / 4.f;
	flag = GetCellValue((int)x1, (int)y1) || GetCellValue((int)x2, (int)y2) ?
		flag | COLLISION_LEFT : flag;

	/*TOP*/
	x1 = PosX + scaleX / 4.f;
	y1 = PosY + scaleY / 2.f;

	x2 = PosX - scaleX / 4.f;
	y2 = PosY + scaleY / 2.f;
	flag = GetCellValue((int)x1, (int)y1) || GetCellValue((int)x2, (int)y2) ?
		flag | COLLISION_TOP : flag;

	/*BOTTOM*/
	x1 = PosX + scaleX / 4.f;
	y1 = PosY - scaleY / 2.f;

	x2 = PosX - scaleX / 4.f;
	y2 = PosY - scaleY / 2.f;
	flag = GetCellValue((int)x1, (int)y1) || GetCellValue((int)x2, (int)y2) ?
		flag | COLLISION_BOTTOM : flag;

	return flag;
}

/******************************************************************************/
/*!
	Snaps to cell
*/
/******************************************************************************/
void SnapToCell(float *Coordinate)
{
	*Coordinate = (float)((int)(*Coordinate) + 0.5f);
}

/******************************************************************************/
/*!	
	Imports data from file path and filps the data along the x and y axis
*/
/******************************************************************************/
int ImportMapDataFromFile(char *FileName)
{
	std::fstream stream{ FileName };
	if (stream.good()) {
		std::string tmpArg;
		stream >> tmpArg >> BINARY_MAP_WIDTH;
		stream >> tmpArg >> BINARY_MAP_HEIGHT;

		int** temp	= new int* [BINARY_MAP_HEIGHT];
		int** tmp	= new int* [BINARY_MAP_HEIGHT];

		for (int x = 0; x < BINARY_MAP_HEIGHT; x++)
		{
			temp[x] = new int[BINARY_MAP_WIDTH];
			tmp[x] = new int[BINARY_MAP_WIDTH];
			for (int y = 0; y < BINARY_MAP_WIDTH; y++)
			{
				int data;
				stream >> data;

				temp[x][y] = data;
				tmp[x][y] = data == 1 ? 1 : 0;
			}
		}

		/*FLIP THE X AND Y AXIS*/
		MapData					= new int* [BINARY_MAP_WIDTH]; // allocate temporary 2D array
		BinaryCollisionArray	= new int* [BINARY_MAP_WIDTH];

		for (int i = 0; i < BINARY_MAP_WIDTH; i++) {
			MapData[i]				= new int[BINARY_MAP_HEIGHT];
			BinaryCollisionArray[i]	= new int[BINARY_MAP_HEIGHT];
		}

		for (int x = 0; x < BINARY_MAP_HEIGHT; x++)
		{
			for (int y = 0; y < BINARY_MAP_WIDTH; y++)
			{
				MapData[y][x]				= temp[x][y];
				BinaryCollisionArray[y][x]	= tmp[x][y];
			}
		}

		// deallocate temp array
		for (int i = 0; i < BINARY_MAP_HEIGHT; i++) {
			delete[] temp[i];
			delete[] tmp[i];
		}

		delete[] temp;
		delete[] tmp;

		return 1;
	}

	return 0;
}

/******************************************************************************/
/*!
	Deletes allocated memory for the maps
*/
/******************************************************************************/
void FreeMapData(void)
{
	for (int i = 0; i < BINARY_MAP_WIDTH; i++)
	{
		delete[] MapData[i];
		delete[] BinaryCollisionArray[i];
	}

	delete[] MapData;
	delete[] BinaryCollisionArray;
}

/******************************************************************************/
/*!
	AI for enemies using a state machine
*/
/******************************************************************************/
void EnemyStateMachine(GameObjInst *pInst)
{
	/*CONVERT COORDINATES TO CELL COORDINATES BY FLOORING VALUE*/
	int currCellX = (int)(pInst->posCurr.x);
	int currCellY = (int)(pInst->posCurr.y);

	/*STATE MANAGER*/
	switch (pInst->state) {

	/*GOING LEFT*/
	case(STATE_GOING_LEFT):

		switch (pInst->innerState) {

		case(INNER_STATE_ON_ENTER):

			/*SET VELOCITY X TO MOVE LEFT*/
			pInst->velCurr.x	= -MOVE_VELOCITY_ENEMY;

			/*UPDATE INNER STATE*/
			pInst->innerState	= INNER_STATE_ON_UPDATE;
			break;

		case(INNER_STATE_ON_UPDATE):

			/*IF OBJECT COLLIDES WITH WALL OR IF THE OBJECT WILL FALL OFF PLATFORM*/
			if (pInst->gridCollisionFlag & COLLISION_LEFT || 
				0 == BinaryCollisionArray[currCellX - 1][currCellY - 1]) 
			{
				/*SET ENEMY TO IDLE BY SETTING VELOCITY X TO 0*/
				pInst->counter		= ENEMY_IDLE_TIME; // 2 seconds
				pInst->velCurr.x	= 0;

				/*UPDATE STATE*/
				pInst->innerState	= INNER_STATE_ON_EXIT;
			}
			break;

		case(INNER_STATE_ON_EXIT):

			/*DECREMENT COUNTER BY FRAMETIME*/
			pInst->counter -= g_dt;

			/*WHEN ENEMY IDLE TIMER IS UP*/
			if (pInst->counter < 0) {
				/*UPDATE STATES*/
				pInst->state		= STATE_GOING_RIGHT;
				pInst->innerState	= INNER_STATE_ON_ENTER;
			}
			break;

		default:
			break;
		}

		break;
		/*GOING LEFT END*/

	/*GOING RIGHT*/
	case(STATE_GOING_RIGHT):

		switch (pInst->innerState) {

		case(INNER_STATE_ON_ENTER):

			/*SET VELOCITY X TO MOVE RIGHT*/
			pInst->velCurr.x = MOVE_VELOCITY_ENEMY;

			/*UPDATE INNER STATE*/
			pInst->innerState = INNER_STATE_ON_UPDATE;
			break;

		case(INNER_STATE_ON_UPDATE):

			/*IF OBJECT COLLIDES WITH WALL OR IF THE OBJECT WILL FALL OFF PLATFORM*/
			if (pInst->gridCollisionFlag & COLLISION_RIGHT ||
				0 == BinaryCollisionArray[currCellX + 1][currCellY - 1])
			{
				/*SET ENEMY TO IDLE BY SETTING VELOCITY X TO 0*/
				pInst->counter = ENEMY_IDLE_TIME; // 2 seconds
				pInst->velCurr.x = 0;

				/*UPDATE STATE*/
				pInst->innerState = INNER_STATE_ON_EXIT;
			}
			break;

		case(INNER_STATE_ON_EXIT):

			/*DECREMENT COUNTER BY FRAMETIME*/
			pInst->counter -= g_dt;

			/*WHEN ENEMY IDLE TIMER IS UP*/
			if (pInst->counter < 0) {
				/*UPDATE STATES*/
				pInst->state = STATE_GOING_LEFT;
				pInst->innerState = INNER_STATE_ON_ENTER;
			}
			break;

		default:
			break;
		}

		break;
		/*GOING RIGHT END*/

	default:
		break;
	}
}

/******************************************************************************/
/*!
	Creates a new particle
*/
/******************************************************************************/
void CreateParticle(AEVec2 pos) {
	for (unsigned int i = 0; i < PARTICLES_MAX; i++)
	{
		Particle *particle			= sParticlesList + i;

		if (particle->flag == 0) {
			// Create particle with randomized values
			particle->pObject		= sGameObjList + TYPE_OBJECT_PARTICLE + (int)PRNG(0.f, 3.f); // Random particle 
			particle->flag			= 1;
			particle->transparency	= PRNG(TRANSPARENCY_MIN, TRANSPARENCY_MAX);
			particle->scale			= PRNG(SCALE_MIN, SCALE_MAX);
			particle->lifespan		= PRNG(LIFESPAN_MIN, LIFESPAN_MAX);
			particle->velCurr		= PRNG(VELOCITY_MIN, VELOCITY_MAX);
			particle->posCurr		= AEVec2{ pos.x + PRNG(-0.1f, 0.1f), pos.y + PRNG(0.2f, 0.4f) };
			break;
		}
	}
}

/******************************************************************************/
/*!
	RNG calculation between two floating point values
*/
/******************************************************************************/
f32 PRNG(f32 min, f32 max) {
	int rng = (int)(min * 100) + (std::rand() % ((int)(max * 100) - (int)(min * 100) + 1));
	return (f32)(rng / 100.f);
}