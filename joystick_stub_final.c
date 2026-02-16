//==========================================================================
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
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

// Stub implementation for joystick functionality without SDL2

int16_t rx = 0;
int16_t ry = 0;
int16_t smousex = 0;
int16_t smousey = 0;

uint8_t JOYSTICK_Init(void)
{
	return 1; // Success
}

void JOYSTICK_Update(uint16_t tickrate)
{
	// Do nothing - stub implementation
}

void JOYSTICK_Quit(void)
{
	// Do nothing - stub implementation
}
