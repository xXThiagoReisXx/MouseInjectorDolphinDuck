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

#define TAU 6.2831853f // 0x40C90FDB

// Pointers
#define CAMBASE_PTR 0x80112B18

// Offsets from CAMBASE_PTR
#define ON_FOOT_CAMX 0x14
#define ON_FOOT_CAMY 0x1B4
#define LEAN_ANIMATION 0x1C0

// Static Addresses
#define AIRCRAFT 0x80113098


// Checks
#define LEVEL_INDICATOR 0x8018BC03
#define FOCUSED 0x800E143F

// NOPs - DLL offset for Mupen64Plus
//#define CAM_AUTOCENTER_PTR 0x6FC9480
//#define CAM_AUTOCENTER_DLL_OFFSET 0x6FAC8AF

static uint8_t N64_SOE_Status(void);
static void N64_SOE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Star Wars: Shadows of the Empire",
	N64_SOE_Status,
	N64_SOE_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	"[OFF] Invert Aircraft Pitch", // Option supported
	"[ON] Invert Aircraft Pitch" // Option2 (normal state)
};

const GAMEDRIVER *GAME_N64_STARWARS_SOE = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t N64_SOE_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A800C && N64_MEM_ReadUInt(0x80000004) == 0x275A3FE0); // unique header in RDRAM/
}

static void N64_SOE_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
		
//==========================================================================
// Purpose: Inject ON FOOT controls
//==========================================================================
	// Write NOP instruction to disable camera autocenter (Mupen64Plus DLL offset)
	//static uint8_t nop_applied = 0;
	//if (!nop_applied) {
	//	N64_MEM_WriteUIntToMupen(CAM_AUTOCENTER_DLL_OFFSET, 0x90909090); // MIPS NOP
	//	nop_applied = 1;
	//}

	uint32_t cambase = N64_MEM_ReadUInt(CAMBASE_PTR);
	

	
	float onfootcamx = N64_MEM_ReadFloat(cambase + ON_FOOT_CAMX);
	onfootcamx -= (float)xmouse * ((float)sensitivity / 100.0f);
	N64_MEM_WriteFloat(cambase + ON_FOOT_CAMX, onfootcamx);

	float onfootcamy = N64_MEM_ReadFloat(cambase + ON_FOOT_CAMY);
	float lean_animation = N64_MEM_ReadFloat(cambase + LEAN_ANIMATION);
	
	// Only inject camy if FOCUSED is 128 (not focused)
	uint8_t focused = N64_MEM_ReadUInt(FOCUSED) & 0xFF;
	if (focused == 128) {
		onfootcamy -= (float)ymouse * ((float)sensitivity / 100.0f);
		
		// Clamp on-foot camera Y to valid range: -46.26005936 to 57.78007889
		if (onfootcamy > 57.78007889f)
			onfootcamy = 57.78007889f;
		else if (onfootcamy < -46.26005936f)
			onfootcamy = -46.26005936f;
		
		N64_MEM_WriteFloat(cambase + ON_FOOT_CAMY, onfootcamy);

		lean_animation -= (float)ymouse * ((float)sensitivity / 100.0f);
	
	// Clamp lean animation to valid range: 55.34506607 to -48.69507217
		if (lean_animation > 55.34506607f)
			lean_animation = 55.34506607f;
		else if (lean_animation < -48.69507217f)
			lean_animation = -48.69507217f;
		
		N64_MEM_WriteFloat(cambase + LEAN_ANIMATION, lean_animation);
	}

//==========================================================================
// Purpose: Inject AIRCRAFT controls
//==========================================================================
	// Check if pitch inversion is enabled
	const uint8_t invert_aircraft_pitch = optionToggle;
	
	// Check if level indicator is 0 or 2, otherwise don't inject aircraft
	uint8_t level_indicator = N64_MEM_ReadUInt(LEVEL_INDICATOR) & 0xFF;
	if (level_indicator != 0 && level_indicator != 2)
		return;
	
	// Apply direct mouse control
	uint32_t aircraft = N64_MEM_ReadUInt(AIRCRAFT);
	
	// Apply sensitivity scaling
	float scaled_xmouse = (float)xmouse * ((float)sensitivity / (level_indicator == 2 ? 1000.0f : 200.0f));
	float scaled_ymouse = (float)ymouse * ((float)sensitivity / (level_indicator == 2 ? 800.0f : 400.0f));
	
	if (scaled_xmouse > 0) {
		aircraft = 5242882u;
	} else if (scaled_xmouse < 0) {
		aircraft = 4289724429u;
	} else if (scaled_ymouse > 0) {
		aircraft = invert_aircraft_pitch ? 196688u : 458672u;
	} else if (scaled_ymouse < 0) {
		aircraft = invert_aircraft_pitch ? 458672u : 196688u;
	} else {
		return;
	}
	N64_MEM_WriteUInt(AIRCRAFT, aircraft);
}

