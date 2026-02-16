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

// Camera addresses
#define ON_FOOT_CAMX 0x8075EC87 // 1 byte
#define ON_FOOT_AIM_CAMX 0x8075EC86 // 2 bytes
#define ON_FOOT_CAMY 0x80143464 // float


#define IS_AIMING 0x801434BC // if 4 it's aiming


// Sanity checks
#define IS_PAUSED 0x8016B578 // if 1 it's paused
#define LEVEL_STATUS 0x80141E44 // 0x00=Default / 0x01=Completed / 0x02=Failed (prevents rotating dead body)
#define LOADING 0x800AF5E0 // if 2 it's loading (injecting while loading often causes crashes)

static float xAccumulator = 0.f;

static uint8_t N64_AMSH_Status(void);
static void N64_AMSH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Army Men - Sarge's Hero",
	N64_AMSH_Status,
	N64_AMSH_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	NULL, // Option not supported
	NULL  // Option2 not supported
};

const GAMEDRIVER *GAME_N64_ARMYMENSH = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t N64_AMSH_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A8008 && 
			N64_MEM_ReadUInt(0x80000004) == 0x275A7780); // unique header in RDRAM
}

static void N64_AMSH_Inject(void)
{
	// Don't inject if game is paused
	if (N64_MEM_ReadUInt(IS_PAUSED) == 1)
		return;
	
	// Don't inject if level status is not default
	if (N64_MEM_ReadUInt(LEVEL_STATUS) != 0)
		return;
	
	// Don't inject if loading
	if (N64_MEM_ReadUInt(LOADING) == 2)
		return;
	
	const float looksensitivity = (float)sensitivity / 80.f;
	
	// Check if aiming before reading camera addresses
	uint8_t isAiming = (N64_MEM_ReadUInt(IS_AIMING) == 4);
	
	// Conditional X-axis sensitivity (if aiming 0.1)
	float dx = (float)xmouse * looksensitivity / (isAiming ? 0.1f : 1.f);
	
	if (isAiming) {
		// Aiming: read and use 2-byte address
		uint16_t aimCamX = N64_MEM_ReadUInt(ON_FOOT_AIM_CAMX) & 0xFFFF;
		aimCamX += (uint16_t)dx; // Direct addition without accumulator
		N64_MEM_WriteUInt(ON_FOOT_AIM_CAMX, aimCamX & 0xFFFF);
	} else {
		// Not aiming: read and use 1-byte address
		uint8_t camX = N64_MEM_ReadUInt(ON_FOOT_CAMX) & 0xFF;
		float camXF = (float)camX;
		AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx); // Use accumulator
		N64_MEM_WriteUInt(ON_FOOT_CAMX, (uint8_t)camXF);
	}
	
	// Y-axis injection (only when aiming)
	if (isAiming) {
		float camY = N64_MEM_ReadFloat(ON_FOOT_CAMY);
		float dy = (float)ymouse * looksensitivity / 10.f;
		camY += dy;
		camY = ClampFloat(camY, -85.f, 85.f); // Clamp Y-axis to ±85 degrees
		
		N64_MEM_WriteFloat(ON_FOOT_CAMY, camY);
	}
}
