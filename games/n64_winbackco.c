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

#define CAMX_1 0x8013A120
#define CAMX_2 0x8013A128
#define CAMX_3 0x8013A130

static uint8_t N64_WBCO_Status(void);
static void N64_WBCO_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Winback - Covert Operations",
	N64_WBCO_Status,
	N64_WBCO_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_WINBACKCO = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t N64_WBCO_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A8010 && 
			N64_MEM_ReadUInt(0x80000004) == 0x275AA9F0); // unique header in RDRAM
}

static void N64_WBCO_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
}
