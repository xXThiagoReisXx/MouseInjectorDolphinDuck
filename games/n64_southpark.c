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

// Camera addresses (static)
#define CAMX 0x806FDAC0
#define CAMY 0x806FE54C
//#define CAMY_1 0x806FE5B0
//#define CAMY_2 0x806FF048
//#define CAMY_3 0x806FF170

// Sanity Checks
#define IS_PAUSED 0x800C2260


#define CONTROL_TYPE 0x800CD243


static uint8_t N64_SOUTHPARK_Status(void);
static void N64_SOUTHPARK_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"South Park",
	N64_SOUTHPARK_Status,
	N64_SOUTHPARK_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	NULL, // Option not supported
	NULL  // Option2 not supported
};

const GAMEDRIVER *GAME_N64_SOUTHPARK = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t N64_SOUTHPARK_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A002A && N64_MEM_ReadUInt(0x80000004) == 0x275AE1C0); // unique header in RDRAM
}

static void N64_SOUTHPARK_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if(N64_MEM_ReadUInt(IS_PAUSED) == 1) // check if game is paused
		return;
	
	N64_MEM_WriteByte(CONTROL_TYPE, 1); // set control type to brown-eye (disables auto center aim)
	
	const float looksensitivity = (float)sensitivity / 5000.f;
	const float y_sensitivity_multiplier = 0.9f; // Y-axis sensitivity multiplier
	
	// Read current camera values
	float camx = N64_MEM_ReadFloat(CAMX);
	float camy = N64_MEM_ReadFloat(CAMY);
	
	// Apply mouse movement with sensitivity scaling
	camx += (float)xmouse * looksensitivity;
	camy += (float)ymouse * looksensitivity * y_sensitivity_multiplier; // Y-axis with individual sensitivity
	
	// Clamp Y axis to prevent over-rotation
	camy = ClampFloat(camy, -1.f, 1.f);
	
	// Write back updated camera values
	N64_MEM_WriteFloat(CAMX, camx);
	N64_MEM_WriteFloat(CAMY, camy);
	//N64_MEM_WriteFloat(CAMY_1, -camy); // Additional Y camera addresses (inverted)
	//N64_MEM_WriteFloat(CAMY_2, -camy);
	//N64_MEM_WriteFloat(CAMY_3, -camy);
}
