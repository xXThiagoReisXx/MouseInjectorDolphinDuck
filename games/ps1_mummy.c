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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f

#define PS1_MUMMY_CAMBASE_PTR 0x9D964 // Camera base pointer

//CAM_X values (offsets from base)
#define PS1_MUMMY_CAM_X_1 0x10 // Lateral Seno
#define PS1_MUMMY_CAM_X_3 0x18 // Lateral Cosseno
#define PS1_MUMMY_CAM_X_4 0x28 // Frontal Cosseno
#define PS1_MUMMY_CAM_X_6 0x30 // Frontal Seno
//#define PS1_MUMMY_CAM_X_2 0xF0 // Jump direction alignment // TODO

// Constantes de conversão
#define FIXED_POINT_SCALE 65535.0f

// Sanity checks
#define PS1_MUMMY_PAUSE_CHECK 0x1FFF74 // Pause check address

static float mouse_angle_x = 0.0f;
static float jump_angle = 0.0f;

static uint8_t PS1_MUMMY_Status(void);
static void PS1_MUMMY_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"The Mummy",
	PS1_MUMMY_Status,
	PS1_MUMMY_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	NULL, // Option not supported
	NULL // Option2 not supported
};

const GAMEDRIVER *GAME_PS1_MUMMY = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MUMMY_Status(void)
{
	// SLUS_011.87
	return (PS1_MEM_ReadWord(0x9304) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9308) == 0x5F303131U && 
			PS1_MEM_ReadWord(0x930C) == 0x2E38373BU);
}

static void PS1_MUMMY_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// Read camera base pointer
	uint32_t cambase = PS1_MEM_ReadPointer(PS1_MUMMY_CAMBASE_PTR);
	if (cambase == 0)
		return;

	// Check if game is paused
	if (PS1_MEM_ReadHalfword(PS1_MUMMY_PAUSE_CHECK) == 0x74F8)
		return;
	
	// Read current camera angle from base + offsets
	uint32_t camaddr = cambase + PS1_MUMMY_CAM_X_4;
	int32_t current_cos = PS1_MEM_ReadInt(camaddr);
	mouse_angle_x = acosf((float)current_cos / FIXED_POINT_SCALE);
	
	// Adjust angle based on current sine value to get correct quadrant
	camaddr = cambase + PS1_MUMMY_CAM_X_6;
	int32_t current_sin = PS1_MEM_ReadInt(camaddr);
	if (current_sin < 0)
		mouse_angle_x = TAU - mouse_angle_x;
	
	// Apply mouse delta with inverted axis
	mouse_angle_x -= (float)xmouse * 0.0015f;
	jump_angle -= (float)xmouse * 0.003f; // Independent jump rotation with different speed
	
	// Calculate trigonometric values
	float s = sinf(mouse_angle_x);
	float c = cosf(mouse_angle_x);
	
	// Convert to fixed-point
	int32_t fix_S = (int32_t)(s * FIXED_POINT_SCALE);
	int32_t fix_C = (int32_t)(c * FIXED_POINT_SCALE);
	
	// Write camera vectors using calculated addresses
	camaddr = cambase + PS1_MUMMY_CAM_X_4;
	PS1_MEM_WriteInt(camaddr, fix_C);      // Frontal Cosseno
	camaddr = cambase + PS1_MUMMY_CAM_X_6;
	PS1_MEM_WriteInt(camaddr, fix_S);      // Frontal Seno
	camaddr = cambase + PS1_MUMMY_CAM_X_1;
	PS1_MEM_WriteInt(camaddr, fix_S);      // Lateral Seno
	camaddr = cambase + PS1_MUMMY_CAM_X_3;
	PS1_MEM_WriteInt(camaddr, -fix_C);     // Lateral Cosseno Invertido


}

// CAM PATTERN ANALYSIS
// DEA18
// 1 - 4294963154
// 2 - 4294916131
// 3 - 4294904597
// 4 - 4294966076
// 5 - 59728
// 6 - 38103
// 7 - 4294964275
// 
// DEA1C
// 1 - 4294964359
// 2 - 4294965209
// 3 - 144
// 4 - 2923
// 5 - 1456
// 6 - 4294965221
// 7 - 4294964353
// 
// DEA20
// 1 - 4294903305
// 2 - 4294928693
// 3 - 12331
// 4 - 64006
// 5 - 22886
// 6 - 4294916081
// 7 - 4294903434
// 
// DEA30
// 1 - 63875
// 2 - 38435
// 3 - 4294954824
// 4 - 4294903392
// 5 - 4294944605
// 6 - 51227
// 7 - 63728
// 
// DEA34
// 1 - 63875
// 2 - 4294965208
// 3 - 4294964332
// 4 - 4294966791
// 5 - 2569
// 6 - 2090
// 7 - 286
// 
// DEA38
// 1 - 4294963259
// 2 - 4294916106
// 3 - 4294904448
// 4 - 4294965895
// 5 - 2569
// 6 - 38287
// 7 - 4294964423