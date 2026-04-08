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
#include <stdint.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB
#define CROSSHAIRY 1.450000048f // 0x3FB9999A
// MOHRS ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
#define MOHRS_camx 0x80E90E74 - 0x80E90C00
#define MOHRS_camy 0x80E90E78 - 0x80E90C00
#define MOHRS_fov 0x80E91A30 - 0x80E90C00
#define MOHRS_sentryx 0x80E91A1C - 0x80E90C00
#define MOHRS_sentryy 0x80E91A20 - 0x80E90C00
#define MOHRS_sentryxlimit 0x80E91A14 - 0x80E90C00
#define MOHRS_sentryylimit 0x80E91A18 - 0x80E90C00
#define MOHRS_sentryflag 0x80E91BC0 - 0x80E90C00
// STATIC ADDRESSES BELOW
#define MOHRS_playerbase 0x802C4760 // playable character pointer

static uint8_t MOHRS_Status(void);
static void MOHRS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Medal of Honor: Rising Sun",
	MOHRS_Status,
	MOHRS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_MOHRS = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t MOHRS_Status(void)
{
	return (MEM_ReadUInt(0x80000000) == 0x47523845U && (MEM_ReadUInt(0x80000004) == 0x36390000U || MEM_ReadUInt(0x80000004) == 0x36390100U)); // check game header to see if it matches MOHRS
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void MOHRS_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	const uint32_t playerbase = MEM_ReadUInt(MOHRS_playerbase);
	if(NOTWITHINMEMRANGE(playerbase)) // if playerbase is invalid
		return;
	const int32_t sentryflag = MEM_ReadInt(playerbase + MOHRS_sentryflag);
	const float fov = MEM_ReadFloat(playerbase + MOHRS_fov);
	const float looksensitivity = (float)sensitivity / 40.f;
	if(sentryflag == 1) // if not using sentry
	{
		float camx = MEM_ReadFloat(playerbase + MOHRS_camx);
		float camy = MEM_ReadFloat(playerbase + MOHRS_camy);
		if(camx >= -PI && camx <= PI && camy >= -CROSSHAIRY && camy <= CROSSHAIRY)
		{
			camx -= (float)xmouse / 10.f * looksensitivity / (360.f / PI) / (35.f / fov); // normal calculation method for X
			camy += (float)(invertpitch ? -ymouse : ymouse) / 10.f * looksensitivity / (90.f / CROSSHAIRY) / (35.f / fov); // normal calculation method for Y
			while(camx <= -PI)
				camx += TAU;
			while(camx >= PI)
				camx -= TAU;
			camy = ClampFloat(camy, -CROSSHAIRY, CROSSHAIRY);
			MEM_WriteFloat(playerbase + MOHRS_camx, camx);
			MEM_WriteFloat(playerbase + MOHRS_camy, camy);
		}
	}
	else if(sentryflag == 21)// if using sentry
	{
		float sentryx = MEM_ReadFloat(playerbase + MOHRS_sentryx);
		float sentryy = MEM_ReadFloat(playerbase + MOHRS_sentryy);
		const float sentryylimit = MEM_ReadFloat(playerbase + MOHRS_sentryylimit);
		if(sentryx >= -PI && sentryx <= PI && sentryy >= -sentryylimit && sentryy <= sentryylimit)
		{
			sentryx -= (float)xmouse / 10.f * looksensitivity / (360.f / PI) / (35.f / fov); // normal calculation method for X
			sentryy += (float)(invertpitch ? -ymouse : ymouse) / 10.f * looksensitivity / (90.f / CROSSHAIRY) / (35.f / fov); // normal calculation method for Y
			while(sentryx < -PI)
				sentryx += TAU;
			while(sentryx >= PI)
				sentryx -= TAU;
			sentryy = ClampFloat(sentryy, -sentryylimit, sentryylimit);
			MEM_WriteFloat(playerbase + MOHRS_sentryx, sentryx);
			MEM_WriteFloat(playerbase + MOHRS_sentryy, sentryy);
		}
	}
}