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
#include <math.h>
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.283185307179586f
#define RADIUS 0.7383f


#define T25_CAM_Y -0x20 // offset from cambaseptr
#define T25_CAM_X_a 0x60 // offset from cambaseptr
#define T25_CAM_X_b 0x58 // offset from cambaseptr
#define T25_CAM_X_a_1 0x40 // offset from cambaseptr
#define T25_CAM_X_b_1 0x48 // offset from cambaseptr


#define T25_CAMBASE_SANITY_1_VALUE 0xE8B86100
#define T25_CAMBASE_SANITY_1 -0x2C //offset from foundcambaseptr

#define T25_IS_PAUSED 0x70CF14

static uint8_t PS2_T25_Status(void);
static uint8_t PS2_T25_DetectCamBase(void);
static void PS2_T25_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"25 to Life",
	PS2_T25_Status,
	PS2_T25_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway not supported for driver
	NULL, // Option
	NULL  // Option2
};

const GAMEDRIVER *GAME_PS2_25TOLIFE = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t foundCamBasePtr = 0;
static uint8_t pattern_found = 0;
static uint32_t last_search_addr = 0; // Track last search position

//==========================================================================
// Purpose: detect cambase dynamically with sanity checks
//==========================================================================
static uint8_t PS2_T25_DetectCamBase(void)
{
    uint32_t sanity1_value = PS2_MEM_ReadWord(foundCamBasePtr + T25_CAMBASE_SANITY_1);
    uint32_t game_state_value = PS2_MEM_ReadUInt(0x6D8C28);
    
    if (game_state_value != 0x00000004) {
        return 0;
    }
    
    if (foundCamBasePtr != 0 && PS2WITHINMEMRANGE(foundCamBasePtr) &&
        sanity1_value == T25_CAMBASE_SANITY_1_VALUE) {
        return 1;
    }
    
    return 0;
}
//==========================================================================
// Purpose: find base address by searching for specific byte pattern
//==========================================================================
static uint32_t PS2_T25_FindBaseByPattern(void)
{
    if (pattern_found && foundCamBasePtr != 0) {
        return foundCamBasePtr;
    }
    
    uint8_t pattern[] = {0x00, 0x00, 0x80, 0xBF, 0x00, 0x00, 0x80, 0x3F, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
    int pattern_size = sizeof(pattern);
    
    uint32_t search_start = (last_search_addr != 0) ? last_search_addr + 4 : 0x018E0000;
    uint32_t search_end = 0x01FFFFFF;
    
    for (uint32_t addr = search_start; addr < search_end - pattern_size; addr++) {
        int match = 1;
        
        for (int i = 0; i < pattern_size; i++) {
            if (PS2_MEM_ReadUInt8(addr + i) != pattern[i]) {
                match = 0;
                break;
            }
        }
        
        if (match) {
            for (uint32_t ptr_addr = addr - 0x1000; ptr_addr < addr + 0x1000; ptr_addr += 4) {
                uint32_t potential_ptr = PS2_MEM_ReadUInt(ptr_addr);
                if (potential_ptr == addr) {
                    uint32_t test_camY = ptr_addr + T25_CAM_Y;
                    uint32_t test_camX_a = ptr_addr + T25_CAM_X_a;
                    uint32_t test_camX_b = ptr_addr + T25_CAM_X_b;
                    
                    if (PS2WITHINMEMRANGE(test_camY) && PS2WITHINMEMRANGE(test_camX_a) && 
                        PS2WITHINMEMRANGE(test_camX_b)) {
                        float val_camX_a = PS2_MEM_ReadFloat(test_camX_a);
                        float val_camX_b = PS2_MEM_ReadFloat(test_camX_b);
                        
                        if (!isnan(val_camX_a) && !isnan(val_camX_b) && 
                            fabsf(val_camX_a) < 1000.0f && fabsf(val_camX_b) < 1000.0f &&
                            (val_camX_a != 0.0f || val_camX_b != 0.0f)) {
                            foundCamBasePtr = ptr_addr;
                            pattern_found = 1;
                            return ptr_addr;
                        }
                    }
                }
            }
            
            uint32_t test_camY = addr + T25_CAM_Y;
            uint32_t test_camX_a = addr + T25_CAM_X_a;
            uint32_t test_camX_b = addr + T25_CAM_X_b;
            
            if (PS2WITHINMEMRANGE(test_camY) && PS2WITHINMEMRANGE(test_camX_a) && 
                PS2WITHINMEMRANGE(test_camX_b)) {
                float val_camX_a = PS2_MEM_ReadFloat(test_camX_a);
                float val_camX_b = PS2_MEM_ReadFloat(test_camX_b);
                
                if (!isnan(val_camX_a) && !isnan(val_camX_b) && 
                    fabsf(val_camX_a) < 1000.0f && fabsf(val_camX_b) < 1000.0f &&
                    (val_camX_a != 0.0f || val_camX_b != 0.0f)) {
                    foundCamBasePtr = addr;
                    pattern_found = 1;
                    return addr;
                }
            }
        }
    }
    
    pattern_found = 0;
    last_search_addr = 0;
    return 0;
}

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_T25_Status(void)
{
	// SLUS_210.16
	return (PS2_MEM_ReadWord(0x00658412) == 0x534C5553U &&
			PS2_MEM_ReadWord(0x00658416) == 0x2D323130U);
}

static void PS2_T25_Inject(void)
{
	if(xmouse == 0 && ymouse == 0)
		return;

    if (PS2_MEM_ReadUInt(T25_IS_PAUSED) == 0x1)
        return;

    float looksensitivity = (float)sensitivity * 0.5f;
    float scale = 66.67f;
    float horizontal_multiplier = 0.0005f;

    uint32_t camBasePtr = PS2_T25_FindBaseByPattern();
    if (camBasePtr == 0) {
        return;
    }
    
    foundCamBasePtr = camBasePtr;
    camBase = PS2_MEM_ReadPointer(camBasePtr);
    
    if (!PS2_T25_DetectCamBase()) {
        last_search_addr = foundCamBasePtr;
        pattern_found = 0;
        foundCamBasePtr = 0;
        return;
    }
    
    uint32_t camY_addr = foundCamBasePtr + T25_CAM_Y;
    uint32_t camX_a_addr = foundCamBasePtr + T25_CAM_X_a;
    uint32_t camX_b_addr = foundCamBasePtr + T25_CAM_X_b;
    uint32_t camX_a_1_addr = foundCamBasePtr + T25_CAM_X_a_1;
    uint32_t camX_b_1_addr = foundCamBasePtr + T25_CAM_X_b_1;
    
    if (!PS2WITHINMEMRANGE(camY_addr) || !PS2WITHINMEMRANGE(camX_a_addr) || 
        !PS2WITHINMEMRANGE(camX_b_addr) || !PS2WITHINMEMRANGE(camX_a_1_addr) || 
        !PS2WITHINMEMRANGE(camX_b_1_addr)) {
        pattern_found = 0;
        foundCamBasePtr = 0;
        return;
    }

    float camX_a = PS2_MEM_ReadFloat(camX_a_addr);
    float camX_b = PS2_MEM_ReadFloat(camX_b_addr);

    static uint8_t first_log = 1;
    static float initial_radius = 0.0f;
    if (first_log) {
        if (!isnan(camX_a) && !isnan(camX_b)) {
            initial_radius = sqrtf(camX_a * camX_a + camX_b * camX_b);
            if (initial_radius == 0.0f) initial_radius = 1.0f;
        }
        first_log = 0;
    }
    
    if (isnan(camX_a) || isnan(camX_b) || (camX_a == 0.0f && camX_b == 0.0f)) {
        pattern_found = 0;
        foundCamBasePtr = 0;
        return;
    }
    
    float current_angle = atan2f(camX_b, camX_a);
    current_angle -= (float)xmouse * looksensitivity * horizontal_multiplier;
    
    while (current_angle > TAU) current_angle -= TAU;
    while (current_angle < -TAU) current_angle += TAU;
    
    camX_b = initial_radius * sinf(current_angle);
    camX_a = initial_radius * cosf(current_angle);
    
    float camY = PS2_MEM_ReadFloat(camY_addr);
    float vertical_movement = (float)(invertpitch ? ymouse : -ymouse) * looksensitivity / scale;
    camY += vertical_movement;
    camY = ClampFloat(camY, -60.f, 60.f);
	
    PS2_MEM_WriteFloat(camX_a_addr, camX_a);
    PS2_MEM_WriteFloat(camX_b_addr, camX_b);
    PS2_MEM_WriteFloat(camX_a_1_addr, camX_a);
    PS2_MEM_WriteFloat(camX_b_1_addr, -camX_b);
    PS2_MEM_WriteFloat(camY_addr, camY);
}

//==========================================================================//
//                                                                          //    
//                              MADE BY IA                                  //         
//                                                                          //  
//==========================================================================//

