// External multihack.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <cmath>
#include "MemMan.h"

#define KeyDOWN -32768
#define KeyUP 0

MemMan MemClass;

struct offsets
{
	DWORD localPlayer = 0xD2FB94;
	DWORD forceLMB = 0x3175068;
	DWORD entityList = 0x4D43AC4;
	DWORD crosshair = 0xB3D4;
	DWORD team = 0xF4;
	DWORD health = 0x100;
	DWORD fJump = 0x51ED760;
	DWORD flags = 0x104;
	DWORD vectorOrigin = 0x138;
	DWORD itemDefIndex = 0x2FAA;
	DWORD activeWeapon = 0x2EF8;
	DWORD isScoped = 0x3914;
	DWORD flashDuration = 0xA410;
	DWORD dwGlowObjectManager = 0x528B8A0;
	DWORD m_iGlowIndex = 0xA428;


}offsets;

struct variables
{
	DWORD localPlayer;
	DWORD gameModule;
	BYTE flag;
	int myTeam;
	int tbDelay;
	int myWeaponID;
}val;

struct vector
{
	float x, y, z;
};

//triggerbot functions
bool checkIfScoped()
{
	return MemClass.readMem<bool>(val.localPlayer + offsets.isScoped);
}
void setTBDelay(float distance)
{
	float delay;
	switch (val.myWeaponID)
	{
	case 262204: delay = 3; break;
	case 7: delay = 3.3; break;
	case 40: delay = 0.15; break;
	case 9: delay = 0.15; break;
	default: delay = 0;
	}
	val.tbDelay = delay * distance;
}
void getMyWeapon()
{
	int weapon = MemClass.readMem<int>(val.localPlayer + offsets.activeWeapon);
	int weaponEntity = MemClass.readMem<int>(val.gameModule + offsets.entityList + ((weapon & 0xFF) - 1) * 0x10);
	if (weaponEntity != NULL)
	{
		val.myWeaponID = MemClass.readMem<int>(weaponEntity + offsets.itemDefIndex);
	}

}
float getDistance(DWORD entity)
{
	vector myLocation = MemClass.readMem<vector>(val.localPlayer + offsets.vectorOrigin);
	vector enemyLocation = MemClass.readMem<vector>(entity + offsets.vectorOrigin);

	return sqrt(pow(myLocation.x - enemyLocation.x, 2) + pow(myLocation.y - enemyLocation.y, 2) + pow(myLocation.z - enemyLocation.z, 2)) * 0.0254;
}
void shoot()
{
	Sleep(val.tbDelay);
	MemClass.writeMem<int>(val.gameModule + offsets.forceLMB, 5);
	Sleep(20);
	MemClass.writeMem<int>(val.gameModule + offsets.forceLMB, 4);
}
bool checkTBot()
{
	int crosshair = MemClass.readMem<int>(val.localPlayer + offsets.crosshair);
	if (crosshair != 0 && crosshair < 64)
	{
		DWORD entity = MemClass.readMem<DWORD>(val.gameModule + offsets.entityList + ((crosshair - 1) * 0x10));
		int eTeam = MemClass.readMem<int>(entity + offsets.team);
		int eHealth = MemClass.readMem<int>(entity + offsets.health);
		if (eTeam != val.myTeam && eHealth > 0)
		{
			float distance = getDistance(entity);
			getMyWeapon();
			setTBDelay(distance);
			if (val.myWeaponID == 40 || val.myWeaponID == 9)
			{
				return checkIfScoped();
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}
void handleTBot()
{
	if (checkTBot()) 
	{
		shoot();
	}
}


int main()
{
	bool canTBot = false, keyHeld = false;
	int flashDur = 0;

	//finding player
	int proc = MemClass.getProcess("csgo.exe");
	val.gameModule = MemClass.getModule(proc, "client_panorama.dll");
	val.localPlayer = MemClass.readMem<DWORD>(val.gameModule + offsets.localPlayer);

	if (val.localPlayer == NULL)
	{
		while (val.localPlayer == NULL)
		{
			val.localPlayer = MemClass.readMem<DWORD>(val.gameModule + offsets.localPlayer);
		}
	}

	while (true)
	{

		//bhop start
		val.flag = MemClass.readMem<BYTE>(val.localPlayer + offsets.flags);

        if (GetAsyncKeyState(VK_SPACE) && val.flag & (1 << 0))
        {
            MemClass.writeMem<DWORD>(val.gameModule + offsets.fJump, 6);
        }

		//triggerbot toggle (NUMPAD 2 to toggle on)
		if (GetAsyncKeyState(VK_NUMPAD2) & 1)
		{
			val.myTeam = MemClass.readMem<int>(val.localPlayer + offsets.team);
			canTBot = !canTBot;
			if (canTBot == true) 
			{
				std::cout << "triggerbot toggled on" << std::endl;
			}
			if (canTBot == false)
			{
				std::cout << "triggerbot toggled off" << std::endl;
			}
		}

		//triggerbot onPress (Hold V key to active)
		if (GetAsyncKeyState(0x56) == KeyDOWN && !keyHeld)
		{
			keyHeld = true;
			canTBot = true;
			std::cout << "triggerbot active" << std::endl;
		}

		if (GetAsyncKeyState(0x56) == KeyUP && keyHeld)
		{
			keyHeld = false;
			canTBot = false;
			std::cout << "triggerbot inactive" << std::endl;

		}

		//triggerbot init
		if (canTBot)
		{
			handleTBot();
		}

		//noFlash
		flashDur = MemClass.readMem<int>(val.localPlayer + offsets.flashDuration);
		if (flashDur > 0)
		{
			std::cout << "flashed" << std::endl;
			MemClass.writeMem<int>(val.localPlayer + offsets.flashDuration, 0);
		}

		//glowHack
		DWORD glowObject = MemClass.readMem<DWORD>(val.gameModule + offsets.dwGlowObjectManager);
		int myTeam = MemClass.readMem<int>(val.localPlayer + offsets.team);


		for (short int i = 0; i < 64; i++)
		{
			DWORD entity = MemClass.readMem<DWORD>(val.gameModule + offsets.entityList + i * 0x10);
			if (entity != NULL)
			{
				int glowIndex = MemClass.readMem<int>(entity + offsets.m_iGlowIndex);
				int entityTeam = MemClass.readMem<int>(entity + offsets.team);

				if (myTeam == entityTeam)
				{
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0x4), 0);
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0x8), 0);
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0xC), 2);
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0x10), 1.7);

				}
				else
				{
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0x4), 2);
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0x8), 0);
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0xC), 0);
					MemClass.writeMem<float>(glowObject + ((glowIndex * 0x38) + 0x10), 1.7);
				}
				MemClass.writeMem<bool>(glowObject + ((glowIndex * 0x38) + 0x24), true);
				MemClass.writeMem<bool>(glowObject + ((glowIndex * 0x38) + 0x25), false);

			}
		}

		Sleep(1);
	}

	return 0;
}