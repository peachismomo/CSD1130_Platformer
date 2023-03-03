/******************************************************************************/
/*!
\file		GameState_Menu.cpp
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

/******************************************************************************/
/*!
	"Load" function of the state
*/
/******************************************************************************/
void GameStateMenuLoad(void) {
	//...nothing
}

/******************************************************************************/
/*!
	"Initialize" function of the state
*/
/******************************************************************************/
void GameStateMenuInit(void) {
	//...nothing
}

/******************************************************************************/
/*!
	"Update" function of the state
*/
/******************************************************************************/
void GameStateMenuUpdate(void) {
	/*GO TO LEVEL 1*/
	if (AEInputCheckCurr(AEVK_1))
		gGameStateNext = GS_PLATFORM;

	/*GO TO LEVEL 2*/
	if (AEInputCheckCurr(AEVK_2))
		gGameStateNext = GS_PLATFORM2;

	/*EXIT GAME*/
	if (AEInputCheckCurr(AEVK_Q))
		gGameStateNext = GS_QUIT;
}

/******************************************************************************/
/*!
	Rendering
*/
/******************************************************************************/
void GameStateMenuDraw(void) {
	AEGfxSetBackgroundColor(0.f, 0.f, 0.f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

	/*Create char buffer*/
	char strBuffer[32];
	memset(strBuffer, 0, 32 * sizeof(char));

	/*SHOW TEXT ON SCREEN*/
	sprintf_s(strBuffer, "Platfomer");
	AEGfxPrint(fontId, strBuffer, -.2f, .4f, 1.f, 1.f, 0.f, 1.f);

	sprintf_s(strBuffer, "Press '1' for Level 1");
	AEGfxPrint(fontId, strBuffer, -.2f, .2f, 1.f, 1.f, 1.f, 1.f);

	sprintf_s(strBuffer, "Press '2' for Level 1");
	AEGfxPrint(fontId, strBuffer, -.2f, .1f, 1.f, 1.f, 1.f, 1.f);

	sprintf_s(strBuffer, "Press 'Q' to Quit");
	AEGfxPrint(fontId, strBuffer, -.2f, 0.f, 1.f, 1.f, 1.f, 1.f);
}

/******************************************************************************/
/*!
	"Free" function of the state
*/
/******************************************************************************/
void GameStateMenuFree(void) {
	//...nothing
}

/******************************************************************************/
/*!
	"Unload" function of the state
*/
/******************************************************************************/
void GameStateMenuUnload(void) {
	//...nothing
}