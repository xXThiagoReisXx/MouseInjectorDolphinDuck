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

#define PLAYER_ANIMATION_PTR 0x80C0C // Player animation state
#define PS1_MUMMY_CAMBASE_PTR 0x9D964 // Camera base pointer

//CAM_X values (offsets from base)
#define PS1_MUMMY_CAM_X_1 0x10 // Lateral Seno
#define PS1_MUMMY_CAM_X_3 0x18 // Lateral Cosseno
#define PS1_MUMMY_CAM_X_4 0x28 // Frontal Cosseno
#define PS1_MUMMY_CAM_X_6 0x30 // Frontal Seno
#define PS1_MUMMY_CAM_X_2 0xF0 // Jump direction alignment // TODO

//CAM_Y
#define PS1_MUMMY_CAM_Y 0x9FDB4 // Camera Y position
#define PS1_MUMMY_CAM_Y_2 0x9FDBC // Camera Y position 2
#define PS1_MUMMY_CAM_Y_3 0x1E18C4 // Camera Y position 3

// Constantes de conversão
#define FIXED_POINT_SCALE 65535.0f

// Sanity checks
#define PS1_MUMMY_PAUSE_CHECK 0x1FFF74 // Pause check address
#define FIRST_PERSON_CHECK 0x9FDAC // First person check
#define INFO_CHECK 0x9E820 // Info screen check (alternate: 0xA8510)
#define PLAYER_STATE 0x130 // Offset from player animation pointer

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
	
	// Check if in info screen - if not 0, don't inject anything
	if (PS1_MEM_ReadByte(INFO_CHECK) != 0)
		return;
	
	// Check if in first person mode - if 0, don't inject Y axis
	uint8_t first_person = PS1_MEM_ReadByte(FIRST_PERSON_CHECK);
	uint8_t inject_y = (first_person != 0);
	
	// Check player state - only inject for specific states
	uint32_t player_anim_ptr = PS1_MEM_ReadPointer(PLAYER_ANIMATION_PTR);
	if (player_anim_ptr == 0)
		return;
	
	uint8_t player_state = PS1_MEM_ReadByte(player_anim_ptr + PLAYER_STATE);
	if (player_state != 43 && player_state != 1 && player_state != 50 && 
		player_state != 31 && player_state != 29 && player_state != 28 && 
		player_state != 49 && player_state != 30)
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
	
	// Calculate Y delta with sensitivity
	float y_delta = (float)ymouse * 200.0f; // Sensitivity multiplier for Y movement
	
	// Only inject Y axis if not in first person mode (first_person != 0)
	if (inject_y) {
		// Handle PS1_MUMMY_CAM_Y (0x9FDB4) - Range: -65536 to 65536, Neutral: 0
		int32_t current_y1 = PS1_MEM_ReadInt(PS1_MUMMY_CAM_Y);
		int32_t new_y1 = current_y1 + (int32_t)y_delta;
		if (new_y1 < -65536) new_y1 = -65536;
		if (new_y1 > 65536) new_y1 = 65536;
		PS1_MEM_WriteInt(PS1_MUMMY_CAM_Y, new_y1);
		
		// Handle PS1_MUMMY_CAM_Y_2 (0x9FDBC) - Range: -65536 to 65536, Neutral: 0
		int32_t current_y2 = PS1_MEM_ReadInt(PS1_MUMMY_CAM_Y_2);
		int32_t new_y2 = current_y2 + (int32_t)y_delta;
		if (new_y2 < -65536) new_y2 = -65536;
		if (new_y2 > 65536) new_y2 = 65536;
		PS1_MEM_WriteInt(PS1_MUMMY_CAM_Y_2, new_y2);
		
		// Handle PS1_MUMMY_CAM_Y_3 (0x1E18C4) - Range: -161 to 3321, Neutral: 1340
		int32_t current_y3 = PS1_MEM_ReadInt(PS1_MUMMY_CAM_Y_3);
		int32_t new_y3 = current_y3 + (int32_t)(y_delta * 0.1f); // Reduced sensitivity for this address
		if (new_y3 < -161) new_y3 = -161;
		if (new_y3 > 3321) new_y3 = 3321;
		PS1_MEM_WriteInt(PS1_MUMMY_CAM_Y_3, new_y3);
	}
	
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
	
	// Write jump direction alignment (2 bytes) - tied to TAU (inverted)
	camaddr = cambase + PS1_MUMMY_CAM_X_2;
	int32_t current_camx2 = PS1_MEM_ReadInt(camaddr); // Read current value as int
	
	// Calculate new value based on current value and mouse movement
	int16_t current_half = (int16_t)current_camx2;
	int16_t mouse_delta = (int16_t)(xmouse * 1.1f); // Increased sensitivity for faster movement
	int16_t new_jump_dir = current_half + mouse_delta; // Right increases, left decreases
	
	// Wrap around when reaching limits (0-4065 range)
	if (new_jump_dir < 0) new_jump_dir = 4065; // Wrap to maximum when below minimum
	if (new_jump_dir > 4065) new_jump_dir = 0;  // Wrap to minimum when above maximum
	
	PS1_MEM_WriteHalfword(camaddr, new_jump_dir);


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