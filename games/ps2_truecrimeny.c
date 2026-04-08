//===========================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

///////////////////////////////////////////////////////////////
// Initial support added (needs patch file): 				 	
// 	cheats\PS2\SLUS-21106_1118ACD0.pnach  	
//
//	To do:
//	- add full support when in cover
//	- disable auto center cam when melee
// 	- add support when equipped with throwing objects
//	- add support when in vehicle							 
//	- disable aimlock (not required but its a nice optional) 
///////////////////////////////////////////////////////////////


#define TAU 6.2831853f

//apparently static
#define ON_FOOT_CAMX 0x4FD908
#define ON_FOOT_CAMY 0x4FD904
#define ON_FOOT_CAMX_1 0x501A70
#define ON_FOOT_CAMX_2 0x501A78

#define PRECISION_AIM 0x539778
#define CAM_LOCK 0x4FD8E4

//sanity checks
#define IS_PAUSED 0x501B84
//#define IN_VEHICLE 0x51B3B4


static uint8_t TCNY_Status(void);
static void TCNY_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"True Crime: New York City",
	TCNY_Status,
	TCNY_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_TRUECRIMENY = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t TCNY_Status(void)

//0x4E4CEA 53 4C 55 53 2D 32 31 31 30 36 54 43 4E 59 43 - SLUS-21106TCNYC
{
	return (PS2_MEM_ReadWord(0x4E4CEA) == 0x534C5553 &&
			PS2_MEM_ReadWord(0x4E4CEE) == 0x2D323131 &&
			PS2_MEM_ReadWord(0x4E4CF2) == 0x30365443 &&
			PS2_MEM_ReadWord(0x4E4CF6) == 0x4E594300);
}

static void TCNY_Inject(void)
{
	// don't inject when game is paused
	if (PS2_MEM_ReadUInt(IS_PAUSED) == 1)
		return;
	
	// Don't know what this is, but if its not 0, camera sometimes locks
	PS2_MEM_WriteUInt(CAM_LOCK, 0);
	
	// don't inject when in vehicle
	//if (PS2_MEM_ReadUInt(IN_VEHICLE) == ?)
	//	return;
	    
	//disabling camY and camX auto level (didn't work via injector, made a patch instead)
	//if (PS2_MEM_ReadUInt(0x0017BF64) == 0x44800000)
		//PS2_MEM_WriteUInt(0x0017BF64, 0x00000000);
	
		
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
		
	float looksensitivity = (float)sensitivity / 1000.f * 0.15f;
	
	// Reduce sensitivity by 70% when precision aim is active
	if (PS2_MEM_ReadUInt(PRECISION_AIM) == 1)
		looksensitivity *= 0.3f;
	
	float camX = PS2_MEM_ReadFloat(ON_FOOT_CAMX);
	camX += (float)xmouse * looksensitivity;
	while (camX > TAU)
		camX -= TAU;
	while (camX < -TAU)
		camX += TAU;
	PS2_MEM_WriteFloat(ON_FOOT_CAMX, camX);
	
	float camY = PS2_MEM_ReadFloat(ON_FOOT_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
	camY = ClampFloat(camY, -1.570796371f, 1.570796371f);
	PS2_MEM_WriteFloat(ON_FOOT_CAMY, camY);
}