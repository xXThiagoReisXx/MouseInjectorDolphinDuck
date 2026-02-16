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

#define TAU 6.2831853f
// CamBase pointer
#define ARMYMEN3D_CAM_BASE_POINTER 0xD6A90
// Offsets from CamBase
#define ARMYMEN3D_CAM_X 0x144
//#define ARMYMEN3D_TURRET_CAM_X 0x1D4
// Sanity check
#define ARMYMEN3D_SANITY_CHECK_1 0xA0200
//#define ARMYMEN3D_VEHICLE_CHECK 0x9F39C // To do

static uint8_t PS1_ARMYMEN3D_Status(void);
static void PS1_ARMYMEN3D_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Army Men 3D",
	PS1_ARMYMEN3D_Status,
	PS1_ARMYMEN3D_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS1_ARMYMEN3D = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_ARMYMEN3D_Status(void)
{
	// SLUS-004.91
	return (PS1_MEM_ReadWord(0x92A4) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x92A8) == 0x5F303034U && 
			PS1_MEM_ReadWord(0x92AC) == 0x2E39313BU);
}

static void PS1_ARMYMEN3D_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// Check sanity value - must be 0 to proceed
	if (PS1_MEM_ReadWord(ARMYMEN3D_SANITY_CHECK_1) != 0)
		return;
	
	// Read camera base pointer
	uint32_t camBase = PS1_MEM_ReadPointer(ARMYMEN3D_CAM_BASE_POINTER);
	if (!camBase)
		return;
	
	// Read current camera X values from offsets
	uint16_t camX = PS1_MEM_ReadHalfword(camBase + ARMYMEN3D_CAM_X);
	//uint16_t turretCamX = PS1_MEM_ReadHalfword(camBase + ARMYMEN3D_TURRET_CAM_X);
	float camXF = (float)camX;
	//float turretCamXF = (float)turretCamX;
	
	// Apply mouse movement with sensitivity
	const float looksensitivity = (float)sensitivity / 20.f;
	float dx = (float)-xmouse * looksensitivity;
	camXF += dx;
	//turretCamXF += dx;
	
	// Write back the modified camera X values
	PS1_MEM_WriteHalfword(camBase + ARMYMEN3D_CAM_X, (uint16_t)camXF);
	
	// Check if player is in vehicle before injecting turret cam // To do
	//if (PS1_MEM_ReadHalfword(ARMYMEN3D_VEHICLE_CHECK) == 0)
	//{
	//	PS1_MEM_WriteHalfword(camBase + ARMYMEN3D_TURRET_CAM_X, (uint16_t)turretCamXF);
	//}
}
