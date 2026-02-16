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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f

// Turok 1 memory addresses
#define T1_CAMX_CAMBASE 0x80100E64 // camera X base address
#define T1_CAMY 0x80128BE0 // camera Y position (static address?)

// -- offsets from cambase --
#define T1_CAMX 0x64 // camera X offset from cambase

// -- auto-center addresses --
#define T1_CAMY_AUTOCENTER 0x800536B8 // NOP instruction address
#define T1_CAMY_AUTOCENTER_WATER 0x8005355C
#define T1_CAMY_AUTOCENTER_MUPEN64 0x7FFE3AFA8436

// -- sanity checks --
#define T1_PAUSE_STATE 0x801196E8 // pause state check (1 = paused)




static uint8_t N64_T1_Status(void);
static uint8_t N64_T1_DetectCam(void);
static void N64_T1_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Turok: Dinosaur Hunter",
	N64_T1_Status,
	N64_T1_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	NULL, // Option
	NULL  // Option2
};

const GAMEDRIVER *GAME_N64_TUROK1 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t N64_T1_Status(void)
{
	// Primary detection method
	if (N64_MEM_ReadUInt(0x80000000) == 0x3C1A800A && N64_MEM_ReadUInt(0x80000004) == 0x275A5730)
		return 1;
	
	// Alternative detection method using byte sequence 0A801A3C 30 57 5A 27
	// Check at same addresses with different values
	if (N64_MEM_ReadUInt(0x80000000) == 0x0A801A3C && N64_MEM_ReadUInt(0x80000004) == 0x30575A27)
		return 1;
	
	return 0;
}

static uint8_t N64_T1_DetectCam(void)
{
	// Validate camera X base address by checking if it's a valid pointer
	uint32_t camXBase = N64_MEM_ReadUInt(T1_CAMX_CAMBASE);
	if (!camXBase || !N64WITHINMEMRANGE(camXBase))
		return 0;
	
	// Validate camera Y address
	if (!N64WITHINMEMRANGE(T1_CAMY))
		return 0;
	
	return 1;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void N64_T1_Inject(void)
{
	// Check if game is paused - don't inject if paused
	if (N64_MEM_ReadUInt(T1_PAUSE_STATE) == 1)
		return;
	
	// Write NOP instruction to disable camera autocenter
	N64_MEM_WriteUInt(T1_CAMY_AUTOCENTER, 0x00000000);
	N64_MEM_WriteUInt(T1_CAMY_AUTOCENTER_WATER, 0x00000000);
	N64_MEM_WriteUInt(T1_CAMY_AUTOCENTER_MUPEN64, 0x00000000);
	
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!N64_T1_DetectCam())
		return;

	const float looksensitivity = (float)sensitivity;
	const float scale = 6400.f;

	// Read camera X base address first, then add offset
	uint32_t camXBase = N64_MEM_ReadUInt(T1_CAMX_CAMBASE);
	float camX = N64_MEM_ReadFloat(camXBase + T1_CAMX);
	// Read camera Y from direct address
	float camY = N64_MEM_ReadFloat(T1_CAMY);

	camX += (float)xmouse * looksensitivity / scale;
	camY -= (float)ymouse * looksensitivity / scale;

	if (camY > 1)
		camY = 1;
	if (camY < -1)
		camY = -1;

	// Write camera X back to base address + offset
	N64_MEM_WriteFloat(camXBase + T1_CAMX, camX);
	// Write camera Y back to direct address
	N64_MEM_WriteFloat(T1_CAMY, camY);
}
