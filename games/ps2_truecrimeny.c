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
#define TCNY_CAMX 0x4FD908
#define TCNY_CAMY 0x4FD904

static uint8_t _Status(void);
static void _Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"True Crime New York",
	_Status,
	_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_TRUECRIMENY = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t _Status(void)

//0x4E4CEA 53 4C 55 53 2D 32 31 31 30 36 54 43 4E 59 43 - SLUS-21106TCNYC
{
	return (PS2_MEM_ReadWord(0x4E4CEA) == 0x534C5553 &&
			PS2_MEM_ReadWord(0x4E4CEE) == 0x2D323131 &&
			PS2_MEM_ReadWord(0x4E4CF2) == 0x30365443 &&
			PS2_MEM_ReadWord(0x4E4CF6) == 0x4E594300);
}

static void _Inject(void)
{
		    
	//disabling camY and camX auto level (didn't work via injector)
	//if (PS2_MEM_ReadUInt(0x0017BF64) == 0x44800000)
		//PS2_MEM_WriteUInt(0x0017BF64, 0x00000000);
	
		
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
		
	float looksensitivity = (float)sensitivity / 1000.f;
	
	float camX = PS2_MEM_ReadFloat(TCNY_CAMX);
	camX += (float)xmouse * looksensitivity;
	while (camX > TAU)
		camX -= TAU;
	while (camX < -TAU)
		camX += TAU;
	PS2_MEM_WriteFloat(TCNY_CAMX, camX);
	
	float camY = PS2_MEM_ReadFloat(TCNY_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;
	camY = ClampFloat(camY, -1.570796371f, 1.570796371f);
	PS2_MEM_WriteFloat(TCNY_CAMY, camY);
}