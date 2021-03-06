/*  
 *  Version: MPL 1.1
 *  
 *  The contents of this file are subject to the Mozilla Public License Version 
 *  1.1 (the "License"); you may not use this file except in compliance with 
 *  the License. You may obtain a copy of the License at 
 *  http://www.mozilla.org/MPL/
 *  
 *  Software distributed under the License is distributed on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 *  for the specific language governing rights and limitations under the
 *  License.
 *  
 *  The Original Code is the YSI 2.0 SA:MP plugin.
 *  
 *  The Initial Developer of the Original Code is Alex "Y_Less" Cole.
 *  Portions created by the Initial Developer are Copyright (C) 2008
 *  the Initial Developer. All Rights Reserved.
 *  
 *  Contributor(s):
 *  
 *  Peter Beverloo
 *  Marcus Bauer
 *  MaVe;
 *  Sammy91
 *  Incognito
 *  
 *  Special Thanks to:
 *  
 *  SA:MP Team past, present and future
 */

#include "Scripting.h"

#include <raknet/BitStream.h>
#include "RPCs.h"
#include "amxfunctions.h"
#include "Utils.h"
#include "Structs.h"
#include "CPlayerData.h"
#include "Functions.h"
#include "CModelSizes.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	// Yes - BOTH string versions...
	#include <strsafe.h>
#else
	#include <sys/stat.h>
	#include <dirent.h>
	#include <fnmatch.h>
//	#include <sys/times.h>
	#include <algorithm>
#endif

#include <sdk/plugin.h>

#include <string.h>
#include <stdio.h>

// extern
typedef cell AMX_NATIVE_CALL (* AMX_Function_t)(AMX *amx, cell *params);

//----------------------------------------------------
#ifdef _WIN32
	// native ffind(const pattern[], filename[], len, &idx);
	static cell AMX_NATIVE_CALL Natives::ffind(AMX *amx, cell *params)
	{
		// Find a file, idx determines which one of a number of matches to use
		CHECK_PARAMS(4, "ffind");
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			WIN32_FIND_DATA
				ffd;
			TCHAR
				szDir[MAX_PATH] = TEXT("./scriptfiles/");
			StringCchCat(szDir, MAX_PATH, szSearch);
			// Get a serch handle
			HANDLE
				hFind = FindFirstFile(szDir, &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Check that this isn't a directory
					if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						// It's not - update idx
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ffd.cFileName, 0, 0, params[3]);
							FindClose(hFind);
							return 1;
						}
						count--;
					}
				}
				while (FindNextFile(hFind, &ffd) != 0);
				FindClose(hFind);
			}
		}
		return 0;
	}
	
	// native dfind(const pattern[], filename[], len, &idx);
	static cell AMX_NATIVE_CALL Natives::dfind(AMX *amx, cell *params)
	{
		// Find a directory, idx determines which one of a number of matches to use
		// Identical to ffind in all but 1 line
		CHECK_PARAMS(4, "dfind");
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			WIN32_FIND_DATA
				ffd;
			TCHAR
				szDir[MAX_PATH] = TEXT("./scriptfiles/");
			StringCchCat(szDir, MAX_PATH, szSearch);
			// Get a serch handle
			HANDLE
				hFind = FindFirstFile(szDir, &ffd);
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Check that this is a directory
					if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// It is - update idx
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ffd.cFileName, 0, 0, params[3]);
							FindClose(hFind);
							return 1;
						}
						count--;
					}
				}
				while (FindNextFile(hFind, &ffd) != 0);
				FindClose(hFind);
			}
		}
		return 0;
	}
#else
	// native ffind(const pattern[], filename[], len, &idx);
	static cell AMX_NATIVE_CALL Natives::ffind(AMX *amx, cell *params)
	{
		// Find a file, idx determines which one of a number of matches to use
		CHECK_PARAMS(4, "dfind");
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			// Find the end of the directory name
			int
				end = strlen(szSearch) - 1;
			if (end == -1)
			{
				return 0;
			}
			while (szSearch[end] != '\\' && szSearch[end] != '/')
			{
				if (!end)
				{
					break;
				}
				end--;
			}
			// Split up the information
			// Ensure that we search in scriptfiles
			// And separate out the filename and path
			char
				*szDir = (char *)alloca(end + 16),
				*szFile;
			strcpy(szDir, "./scriptfiles/");
			if (end)
			{
				szFile = &szSearch[end + 1];
				szSearch[end] = '\0';
				strcpy(szDir + 14, szSearch);
				strcpy(szDir + strlen(szDir), "/");
			}
			else
			{
				szFile = szSearch;
			}
			end = strlen(szDir);
			DIR
				*dp = opendir(szDir);
			if (dp)
			{
				// Loop through all files in the directory
				struct dirent
					*ep;
				while (ep = readdir(dp))
				{
					// Check if this file matches the pattern
					if (!fnmatch(szFile, ep->d_name, FNM_NOESCAPE))
					{
						// Check if this is a directory
						// There MUST be an easier way to do this!
						char
							*full = (char *)malloc(strlen(ep->d_name) + end + 1);
						if (!full)
						{
							closedir(dp);
							return 0;
						}
						strcpy(full, szDir);
						strcpy(full + end, ep->d_name);
						DIR
							*xp = opendir(full);
						free(full);
						if (xp)
						{
							closedir(xp);
							continue;
						}
						// Check if there's any left to skip
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ep->d_name, 0, 0, params[3]);
							closedir(dp);
							return 1;
						}
						count--;
					}
				}
				closedir(dp);
				return 0;
			}
		}
		return 0;
	}
	
	// native dfind(const pattern[], filename[], len, &idx);
	static cell AMX_NATIVE_CALL Natives::dfind(AMX *amx, cell *params)
	{
		// Find a file, idx determines which one of a number of matches to use
		CHECK_PARAMS(4, "ffind");
		cell
			*cptr;
		char
			*szSearch;
		// Get the search pattern
		amx_StrParam(amx, params[1], szSearch);
		if (szSearch)
		{
			// Get associated search information
			amx_GetAddr(amx, params[4], &cptr);
			cell
				count = *cptr;
			// Find the end of the directory name
			int
				end = strlen(szSearch) - 1;
			if (end == -1)
			{
				return 0;
			}
			while (szSearch[end] != '\\' && szSearch[end] != '/')
			{
				if (!end)
				{
					break;
				}
				end--;
			}
			// Split up the information
			// Ensure that we search in scriptfiles
			// And separate out the filename and path
			char
				*szDir = (char *)alloca(end + 16),
				*szFile;
			strcpy(szDir, "./scriptfiles/");
			if (end)
			{
				szFile = &szSearch[end + 1];
				szSearch[end] = '\0';
				strcpy(szDir + 14, szSearch);
				strcpy(szDir + strlen(szDir), "/");
			}
			else
			{
				szFile = szSearch;
			}
			end = strlen(szDir);
			DIR
				*dp = opendir(szDir);
			if (dp)
			{
				// Loop through all files in the directory
				struct dirent
					*ep;
				while (ep = readdir(dp))
				{
					// Check if this file matches the pattern
					if (!fnmatch(szFile, ep->d_name, FNM_NOESCAPE))
					{
						// Check if this is a directory
						// There MUST be an easier way to do this!
						char
							*full = (char *)malloc(strlen(ep->d_name) + end + 1);
						if (!full)
						{
							closedir(dp);
							return 0;
						}
						strcpy(full, szDir);
						strcpy(full + end, ep->d_name);
						DIR
							*xp = opendir(full);
						free(full);
						if (!xp)
						{
							continue;
						}
						closedir(xp);
						// Check if there's any left to skip
						if (!count)
						{
							// No files left to skip, return the data
							(*cptr)++;
							amx_GetAddr(amx, params[2], &cptr);
							amx_SetString(cptr, ep->d_name, 0, 0, params[3]);
							closedir(dp);
							return 1;
						}
						count--;
					}
				}
				closedir(dp);
				return 0;
			}
		}
		return 0;
	}
#endif

// native dcreate(const name[]);
static cell AMX_NATIVE_CALL Natives::dcreate(AMX *amx, cell *params)
{
	// Creates a directory
	CHECK_PARAMS(1, "dcreate");
	char
		*szSearch;
	// Get the search pattern
	amx_StrParam(amx, params[1], szSearch);
	if (szSearch)
	{
		#ifdef _WIN32
			TCHAR
				szDir[MAX_PATH] = TEXT("./scriptfiles/");
			StringCchCat(szDir, MAX_PATH, szSearch);
			return (cell)CreateDirectory(szDir, NULL);
		#else
			char
				*szDir = (char *)alloca(strlen(szSearch) + 15);
			strcpy(szDir, "./scriptfiles/");
			strcpy(szDir + 14, szSearch);
			return (cell)mkdir(szDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		#endif
	}
	return 0;
}

// native frename(const oldname[], const newname[]);
static cell AMX_NATIVE_CALL Natives::frename(AMX *amx, cell *params)
{
	// Creates a directory
	CHECK_PARAMS(2, "frename");
	char
		*szOld,
		*szNew;
	// Get the search pattern
	amx_StrParam(amx, params[1], szOld);
	amx_StrParam(amx, params[2], szNew);
	if (szOld && szNew)
	{
		char
			*szO = (char *)alloca(strlen(szOld) + 16),
			*szN = (char *)alloca(strlen(szNew) + 16);
		strcpy(szO, "./scriptfiles/");
		strcpy(szO + 14, szOld);
		strcpy(szN, "./scriptfiles/");
		strcpy(szN + 14, szNew);
		return (cell)rename(szO, szN);
	}
	return 0;
}

// native drename(const oldname[], const newname[]);
static cell AMX_NATIVE_CALL Natives::drename(AMX *amx, cell *params)
{
	// Creates a directory
	CHECK_PARAMS(2, "drename");
	char
		*szOld,
		*szNew;
	// Get the search pattern
	amx_StrParam(amx, params[1], szOld);
	amx_StrParam(amx, params[2], szNew);
	if (szOld && szNew)
	{
		char
			*szO = (char *)alloca(strlen(szOld) + 16),
			*szN = (char *)alloca(strlen(szNew) + 16);
		strcpy(szO, "./scriptfiles/");
		strcpy(szO + 14, szOld);
		int
			end;
		end = strlen(szO);
		if (szO[end - 1] != '/')
		{
			szO[end] = '/';
			szO[end + 1] = '\0';
		}
		strcpy(szN, "./scriptfiles/");
		strcpy(szN + 14, szNew);
		end = strlen(szN);
		if (szN[end - 1] != '/')
		{
			szN[end] = '/';
			szN[end + 1] = '\0';
		}
		return (cell)rename(szO, szN);
	}
	return 0;
}

// native SetModeRestartTime(Float:time);
static cell AMX_NATIVE_CALL Natives::SetModeRestartTime(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	if (!CAddress::VAR_pRestartWaitTime) return 0;

	CHECK_PARAMS(1, "SetModeRestartTime");

	if (!CAddress::VAR_pRestartWaitTime) return 0;
	*(float*)CAddress::VAR_pRestartWaitTime = amx_ctof(params[1]);
	return 1;
}

// native Float:GetModeRestartTime();
static cell AMX_NATIVE_CALL Natives::GetModeRestartTime(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	if (!CAddress::VAR_pRestartWaitTime) return 0;

	CHECK_PARAMS(1, "SetModeRestartTime");
	float fRestartTime = *(float*)CAddress::VAR_pRestartWaitTime;
	return amx_ftoc(fRestartTime);
}

// native SetMaxPlayers(maxplayers);
static cell AMX_NATIVE_CALL Natives::SetMaxPlayers(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	int maxplayers = (int)params[1];
	if(maxplayers < 1 || maxplayers > MAX_PLAYERS) return 0;

	pServer->SetIntVariable("maxplayers", maxplayers);
	return 1;
}

// native SetMaxNPCs(maxnpcs);
static cell AMX_NATIVE_CALL Natives::SetMaxNPCs(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	int maxnpcs = (int)params[1];
	if(maxnpcs < 1 || maxnpcs > MAX_PLAYERS) return 0;

	pServer->SetIntVariable("maxnpc", maxnpcs);
	return 1;
}

// native SetPlayerAdmin(playerid, bool:admin);
static cell AMX_NATIVE_CALL Natives::SetPlayerAdmin(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetPlayerAdmin");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	pNetGame->pPlayerPool->bIsAnAdmin[playerid] = !!params[2];
	return 1;
}

// native LoadFilterScript(scriptname[]);
static cell AMX_NATIVE_CALL Natives::LoadFilterScript(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "LoadFilterScript");
	
	char
		*name;
	amx_StrParam(amx, params[1], name);
	if(name)
	{
		return CSAMPFunctions::LoadFilterscript(name);
	}
	return 0;
}

// UnLoadFilterScript(scriptname[]);
static cell AMX_NATIVE_CALL Natives::UnLoadFilterScript(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "UnLoadFilterScript");
	
	char
		*name;
	amx_StrParam(amx, params[1], name);
	if(name)
	{
		return CSAMPFunctions::UnLoadFilterscript(name);
	}
	return 0;
}

// native GetFilterScriptCount();
static cell AMX_NATIVE_CALL Natives::GetFilterScriptCount(AMX *amx, cell *params)
{
	return pNetGame->pFilterScriptPool->m_iFilterScriptCount;
}

// native GetFilterScriptName(id, name[], len = sizeof(name));
static cell AMX_NATIVE_CALL Natives::GetFilterScriptName(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "GetFilterScriptName");

	int id = (int)params[1];
	if(id >= MAX_FILTER_SCRIPTS) return 0;

	return set_amxstring(amx, params[2], pNetGame->pFilterScriptPool->m_szFilterScriptName[id], params[3]);
}

// native AddServerRule(name[], value[], flags = CON_VARFLAG_RULE);
static cell AMX_NATIVE_CALL Natives::AddServerRule(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "AddServerRule");
	
	char *name, *value;
	amx_StrParam(amx, params[1], name);
	amx_StrParam(amx, params[2], value);
	if (name && value)
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(name);
		if (ConVar == NULL)
		{
			CSAMPFunctions::AddStringVariable(name, (int)params[3], value, NULL);
			return 1;
		}
	}
	return 0;
}

// native SetServerRule(name[], value[]);
static cell AMX_NATIVE_CALL Natives::SetServerRule(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetServerRule");

	char *name, *value;
	amx_StrParam(amx, params[1], name);
	amx_StrParam(amx, params[2], value);
	if (name && value)
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(name);
		if (ConVar != NULL)
		{
			pServer->SetStringVariable(name, value);
			return 1;
		}
	}
	return 0;
}

// native SetServerRuleInt(name[], value);
static cell AMX_NATIVE_CALL Natives::SetServerRuleInt(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetServerRuleInt");

	char *name;
	amx_StrParam(amx, params[1], name);
	if (name)
	{
		ConsoleVariable_s* ConVar = CSAMPFunctions::FindVariable(name);
		if (ConVar != NULL)
		{
			pServer->SetIntVariable(name, (int)params[2]);
			return 1;
		}
		return 1;
	}
	return 0;
}

// native RemoveServerRule(name[]);
static cell AMX_NATIVE_CALL Natives::RemoveServerRule(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "RemoveServerRule");

	char *name;
	amx_StrParam(amx, params[1], name);
	if (name)
	{
		//RemoveServerRule(name);
		return 1;
	}
	return 0;
}

// native ModifyFlag(name[], flags);
static cell AMX_NATIVE_CALL Natives::ModifyFlag(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "ModifyFlag");
	
	char *name;
	amx_StrParam(amx, params[1], name);
	if (name)
	{
		pServer->ModifyVariableFlags(name, (DWORD)params[2]);
		return 1;
	}
	return 0;
}

// native GetServerSettings(&showplayermarkes, &shownametags, &stuntbonus, &useplayerpedanims, &bLimitchatradius, &disableinteriorenterexits, &nametaglos, &manualvehicleengine, 
//		&limitplayermarkers, &vehiclefriendlyfire, &Float:fGlobalchatradius, &Float:fNameTagDrawDistance, &Float:fPlayermarkerslimit);
static cell AMX_NATIVE_CALL Natives::GetServerSettings(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(13, "GetServerSettings");

	cell *cptr;
	amx_GetAddr(amx, params[1], &cptr); *cptr = (cell)pNetGame->bShowPlayerMarkers;
	amx_GetAddr(amx, params[2], &cptr); *cptr = (cell)pNetGame->byteShowNameTags;
	amx_GetAddr(amx, params[3], &cptr); *cptr = (cell)pNetGame->byteStuntBonus;
	amx_GetAddr(amx, params[4], &cptr); *cptr = (cell)pNetGame->bUseCJWalk;
	amx_GetAddr(amx, params[5], &cptr); *cptr = (cell)pNetGame->bLimitGlobalChatRadius;
	amx_GetAddr(amx, params[6], &cptr); *cptr = (cell)pNetGame->byteDisableEnterExits;
	amx_GetAddr(amx, params[7], &cptr); *cptr = (cell)pNetGame->byteNameTagLOS;
	amx_GetAddr(amx, params[8], &cptr); *cptr = (cell)pNetGame->bManulVehicleEngineAndLights;
	amx_GetAddr(amx, params[9], &cptr); *cptr = (cell)pNetGame->bLimitPlayerMarkers;
	amx_GetAddr(amx, params[10], &cptr); *cptr = (cell)pNetGame->bVehicleFriendlyFire;
	amx_GetAddr(amx, params[11], &cptr); *cptr = amx_ftoc(pNetGame->fGlobalChatRadius);
	amx_GetAddr(amx, params[12], &cptr); *cptr = amx_ftoc(pNetGame->fNameTagDrawDistance);
	amx_GetAddr(amx, params[13], &cptr); *cptr = amx_ftoc(pNetGame->fPlayerMarkesLimit);
	return 1;
}

// native IsValidNickName(name[]);
static cell AMX_NATIVE_CALL Natives::IsValidNickName(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsValidNickName");

	char *name;
	amx_StrParam(amx, params[1], name);
	if (name)
	{
		return pServer->IsValidNick(name);
	}
	return 0;
}

// native AllowNickNameCharacter(character, bool:allow);
static cell AMX_NATIVE_CALL Natives::AllowNickNameCharacter(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "AllowNickNameCharacter");
	
	char character = (char)params[1];

	// Enable %s is disallowed
	if(character == '%') return 0;

	pServer->AllowNickNameCharacter(character, !!params[2]);
	return 0;
}

// native IsNickNameCharacterAllowed(character);
static cell AMX_NATIVE_CALL Natives::IsNickNameCharacterAllowed(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsNickNameCharacterAllowed");

	return pServer->IsNickNameCharacterAllowed((char)params[1]);
}

/////////////// Timers

// native GetAvailableClasses();
static cell AMX_NATIVE_CALL Natives::GetAvailableClasses(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	return pNetGame->iSpawnsAvailable;
}

// native GetPlayerClass(classid, &teamid, &modelid, &Float:spawn_x, &Float:spawn_y, &Float:spawn_z, &Float:z_angle, &weapon1, &weapon1_ammo, &weapon2, &weapon2_ammo,& weapon3, &weapon3_ammo);
static cell AMX_NATIVE_CALL Natives::GetPlayerClass(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(13, "GetPlayerClass");

	int classid = (int)params[1];
	if(classid < 0 || classid > pNetGame->iSpawnsAvailable) return 0;

	PLAYER_SPAWN_INFO *pSpawn = &pNetGame->AvailableSpawns[classid];
	
	cell *cptr;
	amx_GetAddr(amx, params[2], &cptr); *cptr = (cell)pSpawn->byteTeam;
	amx_GetAddr(amx, params[3], &cptr); *cptr = (cell)pSpawn->iSkin;
	amx_GetAddr(amx, params[4], &cptr); *cptr = amx_ftoc(pSpawn->vecPos.fX);
	amx_GetAddr(amx, params[5], &cptr); *cptr = amx_ftoc(pSpawn->vecPos.fY);
	amx_GetAddr(amx, params[6], &cptr); *cptr = amx_ftoc(pSpawn->vecPos.fZ);
	amx_GetAddr(amx, params[7], &cptr); *cptr = amx_ftoc(pSpawn->fRotation);
	amx_GetAddr(amx, params[8], &cptr); *cptr = (cell)pSpawn->iSpawnWeapons[0];
	amx_GetAddr(amx, params[9], &cptr); *cptr = (cell)pSpawn->iSpawnWeaponsAmmo[0];
	amx_GetAddr(amx, params[10], &cptr); *cptr = (cell)pSpawn->iSpawnWeapons[1];
	amx_GetAddr(amx, params[11], &cptr); *cptr = (cell)pSpawn->iSpawnWeaponsAmmo[1];
	amx_GetAddr(amx, params[12], &cptr); *cptr = (cell)pSpawn->iSpawnWeapons[2];
	amx_GetAddr(amx, params[13], &cptr); *cptr = (cell)pSpawn->iSpawnWeaponsAmmo[2];
	return 1;
}

// native EditPlayerClass(classid, teamid, modelid, Float:spawn_x, Float:spawn_y, Float:spawn_z, Float:z_angle, weapon1, weapon1_ammo, weapon2, weapon2_ammo, weapon3, weapon3_ammo);
static cell AMX_NATIVE_CALL Natives::EditPlayerClass(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(13, "EditPlayerClass");

	int classid = (int)params[1];
	if(classid < 0 || classid > pNetGame->iSpawnsAvailable) return 0;

	PLAYER_SPAWN_INFO *pSpawn = &pNetGame->AvailableSpawns[classid];

	pSpawn->byteTeam = (BYTE)params[2];
	pSpawn->iSkin = (int)params[3];
	pSpawn->vecPos = CVector(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6]));
	pSpawn->fRotation = amx_ctof(params[7]);
	pSpawn->iSpawnWeapons[0] = (int)params[8];
	pSpawn->iSpawnWeaponsAmmo[0] = (int)params[9];
	pSpawn->iSpawnWeapons[1] = (int)params[10];
	pSpawn->iSpawnWeaponsAmmo[1] = (int)params[11];
	pSpawn->iSpawnWeapons[2] = (int)params[12];
	pSpawn->iSpawnWeaponsAmmo[2] = (int)params[13];
	return 1;
}

// native GetActiveTimers();
static cell AMX_NATIVE_CALL Natives::GetActiveTimers(AMX *amx, cell *params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	return pNetGame->pScriptTimers->m_dwTimerCount;
}

// native SendInvalidPlayerSync(playerid); - raksamp versions will crash
static cell AMX_NATIVE_CALL Natives::SendInvalidPlayerSync(AMX *amx, cell *params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "SendInvalidPlayerSync");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	RakNet::BitStream bs;
	bs.Write(65530);

	pRakServer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), false);
	return 1;
}

// native SetGravity(Float:gravity);
static cell AMX_NATIVE_CALL Natives::FIXED_SetGravity( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "SetGravity");

	pServer->SetGravity_(amx_ctof(params[1]));
	return 1;
}

// native Float:GetGravity();
static cell AMX_NATIVE_CALL Natives::FIXED_GetGravity( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	float fGravity = pServer->GetGravity_();
	return amx_ftoc(fGravity);
}

// native SetPlayerGravity(playerid, Float:gravity);
static cell AMX_NATIVE_CALL Natives::SetPlayerGravity( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetPlayerGravity");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	// Update stored values
	pPlayerData[playerid]->fGravity = amx_ctof(params[2]);

	RakNet::BitStream bs;
	bs.Write(pPlayerData[playerid]->fGravity);
	pRakServer->RPC(&RPC_Gravity, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native Float:GetPlayerGravity(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerGravity( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerGravity");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return amx_ftoc(pPlayerData[playerid]->fGravity);
}

// native SetPlayerTeamForPlayer(forplayerid, playerid, teamid);
static cell AMX_NATIVE_CALL Natives::SetPlayerTeamForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "SetPlayerTeamForPlayer");

	int forplayerid = (int)params[1];
	int playerid = (int)params[2];
	BYTE team = (BYTE)params[3];

	if(!IsPlayerConnected(forplayerid) || !IsPlayerConnected(playerid)) return 0;

	pPlayerData[forplayerid]->bytePlayersTeam[playerid] = team;

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);	// playerid
	bs.Write(team);				// teamid
	pRakServer->RPC(&RPC_SetPlayerTeam, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native GetPlayerTeamForPlayer(forplayerid, playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerTeamForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GetPlayerTeamForPlayer");

	int forplayerid = (int)params[1];
	int playerid = (int)params[2];

	if(!IsPlayerConnected(forplayerid) || !IsPlayerConnected(playerid)) return 0;

	return pPlayerData[forplayerid]->bytePlayersTeam[playerid];
}

// native SetWeather(weatherid);
static cell AMX_NATIVE_CALL Natives::FIXED_SetWeather( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "SetWeather");

	pServer->SetWeather_((BYTE)params[1]);
	return 1;
}

// native SetPlayerWeather(playerid, weatherid);
static cell AMX_NATIVE_CALL Natives::FIXED_SetPlayerWeather(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(2, "SetPlayerWeather");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	// Update stored values
	pPlayerData[playerid]->byteWeather = (BYTE)params[2];

	RakNet::BitStream bs;
	bs.Write(pPlayerData[playerid]->byteWeather);
	pRakServer->RPC(&RPC_Weather, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native GetPlayerWeather(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerWeather( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerWeather");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->byteWeather;
}

// native SetPlayerWorldBounds(playerid, Float:x_max, Float:x_min, Float:y_max, Float:y_min)
static cell AMX_NATIVE_CALL Natives::FIXED_SetPlayerWorldBounds(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(5, "SetPlayerWorldBounds");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	// Update stored values
	pPlayerData[playerid]->fBounds[0] = amx_ctof(params[2]);
	pPlayerData[playerid]->fBounds[1] = amx_ctof(params[3]);
	pPlayerData[playerid]->fBounds[2] = amx_ctof(params[4]);
	pPlayerData[playerid]->fBounds[3] = amx_ctof(params[5]);

	RakNet::BitStream bs;
	bs.Write(pPlayerData[playerid]->fBounds[0]);
	bs.Write(pPlayerData[playerid]->fBounds[1]);
	bs.Write(pPlayerData[playerid]->fBounds[2]);
	bs.Write(pPlayerData[playerid]->fBounds[3]);
	pRakServer->RPC(&RPC_WorldBounds, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native TogglePlayerWidescreen(playerid, bool:set);
static cell AMX_NATIVE_CALL Natives::TogglePlayerWidescreen( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "TogglePlayerWidescreen");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	BYTE set = !!(BYTE)params[2];
	pPlayerData[playerid]->bWidescreen = !!set;

	RakNet::BitStream bs;
	bs.Write(set);
	pRakServer->RPC(&RPC_Widescreen, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native IsPlayerWidescreenToggled(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerWidescreenToggled( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerWideScreen");

	int playerid = (int)params[1];
	PlayerID playerId = pRakServer->GetPlayerIDFromIndex(playerid);
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->bWidescreen;
}

// native GetSpawnInfo(playerid, &teamid, &modelid, &Float:spawn_x, &Float:spawn_y, &Float:spawn_z, &Float:z_angle, &weapon1, &weapon1_ammo, &weapon2, &weapon2_ammo,& weapon3, &weapon3_ammo);
static cell AMX_NATIVE_CALL Natives::GetSpawnInfo( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(13, "GetSpawnInfo");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	PLAYER_SPAWN_INFO *pSpawn = &pNetGame->pPlayerPool->pPlayer[playerid]->spawn;

	cell *cptr;
	amx_GetAddr(amx, params[2], &cptr); *cptr = (cell)pSpawn->byteTeam;
	amx_GetAddr(amx, params[3], &cptr); *cptr = (cell)pSpawn->iSkin;
	amx_GetAddr(amx, params[4], &cptr); *cptr = amx_ftoc(pSpawn->vecPos.fX);
	amx_GetAddr(amx, params[5], &cptr); *cptr = amx_ftoc(pSpawn->vecPos.fY);
	amx_GetAddr(amx, params[6], &cptr); *cptr = amx_ftoc(pSpawn->vecPos.fZ);
	amx_GetAddr(amx, params[7], &cptr); *cptr = amx_ftoc(pSpawn->fRotation);
	amx_GetAddr(amx, params[8], &cptr); *cptr = (cell)pSpawn->iSpawnWeapons[0];
	amx_GetAddr(amx, params[9], &cptr); *cptr = (cell)pSpawn->iSpawnWeaponsAmmo[0];
	amx_GetAddr(amx, params[10], &cptr); *cptr = (cell)pSpawn->iSpawnWeapons[1];
	amx_GetAddr(amx, params[11], &cptr); *cptr = (cell)pSpawn->iSpawnWeaponsAmmo[1];
	amx_GetAddr(amx, params[12], &cptr); *cptr = (cell)pSpawn->iSpawnWeapons[2];
	amx_GetAddr(amx, params[13], &cptr); *cptr = (cell)pSpawn->iSpawnWeaponsAmmo[2];
	return 1;
}

// native GetPlayerSkillLevel(playerid, skill);
static cell AMX_NATIVE_CALL Natives::GetPlayerSkillLevel( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(4, "GetPlayerSkillLevel");

	int playerid = (int)params[1];
	int skillid = (int)params[2];
	
	if(!IsPlayerConnected(playerid)) return 0;
	if(skillid < 0 || skillid > 10) return -1;

	return pNetGame->pPlayerPool->pPlayer[playerid]->wSkillLevel[skillid];
}

// native IsPlayerCheckpointActive(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerCheckpointActive(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerCheckpointActive");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bShowCheckpoint;
}

// native GetPlayerCheckpoint(playerid, &Float:fX, &Float:fY, &Float:fZ, &Float:fSize);
static cell AMX_NATIVE_CALL Natives::GetPlayerCheckpoint( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(5, "GetPlayerCheckpoint");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	cell* cptr;
	CVector *vecPos = &pNetGame->pPlayerPool->pPlayer[playerid]->vecCPPos;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(vecPos->fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(vecPos->fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(vecPos->fZ);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pNetGame->pPlayerPool->pPlayer[playerid]->fCPSize);
	return 1;
}

// native IsPlayerRaceCheckpointActive(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerRaceCheckpointActive(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerRaceCheckpointActive");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bShowRaceCheckpoint;
}

// native GetPlayerRaceCheckpoint(playerid, &Float:fX, &Float:fY, &Float:fZ, &Float:fNextX, &Float:fNextY, &fNextZ, &Float:fSize);
static cell AMX_NATIVE_CALL Natives::GetPlayerRaceCheckpoint( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(8, "GetPlayerRaceCheckpoint");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	cell* cptr;
	CVector *vecPos = &pNetGame->pPlayerPool->pPlayer[playerid]->vecRaceCPPos;
	CVector *vecNextPos = &pNetGame->pPlayerPool->pPlayer[playerid]->vecRaceCPNextPos;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(vecPos->fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(vecPos->fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(vecPos->fZ);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(vecNextPos->fX);
	amx_GetAddr(amx, params[6], &cptr);
	*cptr = amx_ftoc(vecNextPos->fY);
	amx_GetAddr(amx, params[7], &cptr);
	*cptr = amx_ftoc(vecNextPos->fZ);
	amx_GetAddr(amx, params[8], &cptr);
	*cptr = amx_ftoc(pNetGame->pPlayerPool->pPlayer[playerid]->fRaceCPSize);
	return 1;
}

// native GetPlayerWorldBounds(playerid, &Float:x_max, &Float:x_min, &Float:y_max, &Float:y_min);
static cell AMX_NATIVE_CALL Natives::GetPlayerWorldBounds( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(5, "GetPlayerWorldBounds");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	cell* cptr;
	float *fBounds = pPlayerData[playerid]->fBounds;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(fBounds[0]);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(fBounds[1]);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(fBounds[2]);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(fBounds[3]);
	return 1;
}

// native IsPlayerInModShop(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerInModShop( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerInModShop");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bIsInModShop;
}

// native GetPlayerSirenState(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerSirenState( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerSirenState");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.byteSirenState;
}

// native GetPlayerLandingGearState(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerLandingGearState( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerLandingGearState");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.byteGearState;
}

// native GetPlayerHydraReactorAngle(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerHydraReactorAngle( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerHydraReactorAngle");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.wHydraReactorAngle[0];
}

// native Float:GetPlayerTrainSpeed(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerTrainSpeed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerTrainSpeed");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->wVehicleId) return 0;

	return amx_ftoc(pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.fTrainSpeed);
}

// native Float:GetPlayerZAim(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerZAim( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerZAim");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return amx_ftoc(pNetGame->pPlayerPool->pPlayer[playerid]->aimSyncData.fZAim);
}

// native GetPlayerSurfingOffsets(playerid, &Float:fOffsetX, &Float:fOffsetY, &Float:fOffsetZ);
static cell AMX_NATIVE_CALL Natives::GetPlayerSurfingOffsets( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(4, "GetPlayerSurfingOffsets");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	CVector vecPos = pNetGame->pPlayerPool->pPlayer[playerid]->syncData.vecSurfing;

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(vecPos.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(vecPos.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(vecPos.fZ);
	return 1;
}

// native GetPlayerRotationQuat(playerid, &Float:w, &Float:x, &Float:y, &Float:z);
static cell AMX_NATIVE_CALL Natives::GetPlayerRotationQuat( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(5, "GetPlayerRotationQuat");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	CPlayer *pPlayer = pNetGame->pPlayerPool->pPlayer[playerid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pPlayer->fQuaternion[0]);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pPlayer->fQuaternion[1]);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pPlayer->fQuaternion[2]);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pPlayer->fQuaternion[3]);
	return 1;
}

// native GetPlayerDialogID(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerDialogID( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerDialogID");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->wDialogID;
}

// native GetPlayerSpectateID(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerSpectateID( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerSpectateID");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->wSpectateID;
}

// native GetPlayerSpectateType(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerSpectateType( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerSpectateType");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->byteSpectateType;
}

// native GetPlayerLastSyncedVehicleID(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerLastSyncedVehicleID(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerLastSyncedVehicleID");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	return (cell)pNetGame->pPlayerPool->pPlayer[playerid]->vehicleSyncData.wVehicleId;
}

// native GetPlayerLastSyncedTrailerID(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerLastSyncedTrailerID(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerLastSyncedTrailerID");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->trailerSyncData.wTrailerID;  //* (WORD*)(((char*)pNetGame->pPlayerPool->pPlayer[playerid]) + 271);
}

// native GetPlayerFPS(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerFPS(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPlayerFPS");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->dwFPS;
}

// native SendBulletData(sender, hitid, hittype, Float:fHitOriginX, Float:fHitOriginY, Float:fHitOriginZ, Float:fHitTargetX, Float:fHitTargetY, Float:fHitTargetZ, Float:fCenterOfHitX, Float:fCenterOfHitY, Float:fCenterOfHitZ, forplayerid = -1);
static cell AMX_NATIVE_CALL Natives::SendBulletData( AMX* amx, cell* params ) 
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(13, "SendBulletData");

	int playerid = (int)params[1];
	int forplayerid = (int)params[12];
	if(!IsPlayerConnected(playerid)) return 0;

	if(forplayerid != -1)
	{
		if(!IsPlayerConnected(forplayerid)) return 0;
	}

	BULLET_SYNC_DATA bulletSync;
	bulletSync.byteHitType = (BYTE)params[3];
	bulletSync.wHitID = (WORD)params[2];
	bulletSync.vecHitOrigin = CVector(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6]));
	bulletSync.vecHitTarget = CVector(amx_ctof(params[7]), amx_ctof(params[8]), amx_ctof(params[9]));
	bulletSync.vecCenterOfHit = CVector(amx_ctof(params[10]), amx_ctof(params[11]), amx_ctof(params[12]));

	RakNet::BitStream bs;
	bs.Write((BYTE)ID_BULLET_SYNC);
	bs.Write((WORD)playerid);
	bs.Write((char*)&bulletSync, sizeof(BULLET_SYNC_DATA));

	if(forplayerid == -1)
	{
		pRakServer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true);
	}
	else
	{
		pRakServer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(forplayerid), false);
	}
	return 1;
}

// native ShowPlayerForPlayer(forplayerid, playerid);
static cell AMX_NATIVE_CALL Natives::ShowPlayerForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "ShowPlayerForPlayer");

	int forplayerid = (int)params[1];
	if(!IsPlayerConnected(forplayerid)) return 0;

	int playerid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);
	pRakServer->RPC(&RPC_WorldPlayerAdd, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native HidePlayerForPlayer(forplayerid, playerid);
static cell AMX_NATIVE_CALL Natives::HidePlayerForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "HidePlayerForPlayer");

	int forplayerid = (int)params[1];
	if(!IsPlayerConnected(forplayerid)) return 0;

	int playerid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;

	RakNet::BitStream bs;
	bs.Write((WORD)playerid);
	pRakServer->RPC(&RPC_WorldPlayerRemove, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(forplayerid), 0, 0);
	return 1;
}

// native SetPlayerChatBubbleForPlayer(forplayerid, playerid, text[], color, Float:drawdistance, expiretime);
static cell AMX_NATIVE_CALL Natives::SetPlayerChatBubbleForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(6, "SetPlayerChatBubbleForPlayer");

	int forplayerid = (int)params[1];
	if(!IsPlayerConnected(forplayerid)) return 0;

	int playerid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;

	int color = (int)params[3];
	float drawdistance = amx_ctof(params[4]);
	int expiretime = (int)params[5];

	char *str;
	amx_StrParam(amx, params[6], str);
	if(str)
	{
		BYTE len = strlen(str);
		RakNet::BitStream bs;
		bs.Write((WORD)playerid);
		bs.Write(color);
		bs.Write(drawdistance);
		bs.Write(expiretime);
		bs.Write(len);
		bs.Write(str, len);
		pRakServer->RPC(&RPC_ChatBubble, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(forplayerid), 0, 0);
		return 1;
	}
	return 0;
}

// native SetPlayerVersion(playerid, version[];
static cell AMX_NATIVE_CALL Natives::SetPlayerVersion( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetPlayerVersion");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;
	
	char *version;
	amx_StrParam(amx, params[2], version);

	logprintf("1 - %s", version);
	if (version && strlen(version) < 28)
	{
		pNetGame->pPlayerPool->szVersion[playerid][0] = NULL;
		strcpy(pNetGame->pPlayerPool->szVersion[playerid], version);
		logprintf("2 - %s", pNetGame->pPlayerPool->szVersion[playerid]);
		return 1;
	}
	return 0;
}

// native IsPlayerSpawned(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerSpawned( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerSpawned");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	int state = pNetGame->pPlayerPool->pPlayer[playerid]->byteState;
	return (state != PLAYER_STATE_NONE && state != PLAYER_STATE_WASTED && state != PLAYER_STATE_SPAWNED);
}

// native SpawnForWorld(playerid);
static cell AMX_NATIVE_CALL Natives::SpawnForWorld(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "SpawnForWorld");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	CSAMPFunctions::SpawnPlayer_(playerid);
	return 1;
}

// Scoreboard manipulation
// native TogglePlayerScoresPingsUpdate(playerid, bool:toggle);
static cell AMX_NATIVE_CALL Natives::TogglePlayerScoresPingsUpdate(AMX *amx, cell *params)
{
	if(!pServer) return 0;

	CHECK_PARAMS(2, "TogglePlayerScoresPingsUpdate");

	int playerid = (int)params[1];
	bool toggle = !!params[2];

	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->bUpdateScoresPingsDisabled = !toggle;
	return 1;
}

// native TogglePlayerFakePing(playerid, bool:toggle);
static cell AMX_NATIVE_CALL Natives::TogglePlayerFakePing(AMX *amx, cell *params)
{
	if(!pServer) return 0;

	CHECK_PARAMS(2, "TogglePlayerFakePing");

	int playerid = (int)params[1];
	bool toggle = !!params[2];

	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->bFakePingToggle = toggle;
	return 1;
}

// native SetPlayerFakePing(playerid, ping);
static cell AMX_NATIVE_CALL Natives::SetPlayerFakePing(AMX *amx, cell *params)
{
	if(!pServer) return 0;

	CHECK_PARAMS(2, "SetPlayerFakePing");

	int playerid = (int)params[1];
	int fakeping = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->dwFakePingValue = fakeping;
	return 1;
}

// native TogglePlayerOnPlayerList(playerid, bool:toggle);
static cell AMX_NATIVE_CALL Natives::TogglePlayerOnPlayerList(AMX *amx, cell *params)
{
	if (!pServer) return 0;

	CHECK_PARAMS(2, "TogglePlayerOnPlayerList");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	pPlayerData[playerid]->bHidden = !(!!params[2]);
	return 1;
}

// native IsPlayerToggledOnPlayerList(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerToggledOnPlayerList(AMX *amx, cell *params)
{
	if (!pServer) return 0;

	CHECK_PARAMS(1, "IsPlayerToggledOnPlayerList");

	int playerid = (int)params[1];
	if (!IsPlayerConnected(playerid)) return 0;

	return !pPlayerData[playerid]->bHidden;
}

// native IsPlayerPaused(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerPaused(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "IsPlayerPaused");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pPlayerData[playerid]->bAFKState;
}

// native GetPlayerPausedTime(playerid);
static cell AMX_NATIVE_CALL Natives::GetPlayerPausedTime(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "GetPlayerPausedTime");

	int playerid = (int)params[1];

	if(!IsPlayerConnected(playerid)) return 0;
	if(!pPlayerData[playerid]->bAFKState) return 0;

	return GetTickCount() - pPlayerData[playerid]->dwLastUpdateTick;
}

// Objects - global
// native GetObjectModel(objectid);
static cell AMX_NATIVE_CALL Natives::GetObjectModel( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetObjectModel");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;
	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	return pNetGame->pObjectPool->m_pObjects[objectid]->iModel;
}

// native Float:GetObjectDrawDistance(objectid);
static cell AMX_NATIVE_CALL Natives::GetObjectDrawDistance( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetObjectDrawDistance");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;
	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->m_pObjects[objectid]->fDrawDistance);
}

// native Float:SetObjectMoveSpeed(objectid, Float:fSpeed);
static cell AMX_NATIVE_CALL Natives::SetObjectMoveSpeed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetObjectMoveSpeed");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;
	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	pNetGame->pObjectPool->m_pObjects[objectid]->fMoveSpeed = amx_ctof(params[2]);
	return 1;
}

// native Float:GetObjectMoveSpeed(objectid);
static cell AMX_NATIVE_CALL Natives::GetObjectMoveSpeed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetObjectMoveSpeed");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;
	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->m_pObjects[objectid]->fMoveSpeed);
}

// native GetObjectTarget(objectid, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetObjectTarget( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(4, "GetObjectTarget");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;
	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	cell* cptr;
	CObject *pObject = pNetGame->pObjectPool->m_pObjects[objectid];
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pObject->matTarget.pos.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pObject->matTarget.pos.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pObject->matTarget.pos.fZ);
	return 1;
}

// native GetObjectAttachedData(objectid, &vehicleid, &objectid);
static cell AMX_NATIVE_CALL Natives::GetObjectAttachedData( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "GetObjectAttachedData");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;
	
	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	cell* cptr;
	CObject *pObject = pNetGame->pObjectPool->m_pObjects[objectid];
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = (cell)pObject->wAttachedVehicleID;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)pObject->wAttachedObjectID;
	return 1;
}

// native GetObjectAttachedOffset(objectid, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ);
static cell AMX_NATIVE_CALL Natives::GetObjectAttachedOffset( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(7, "GetObjectAttachedOffset");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	cell* cptr;
	CObject *pObject = pNetGame->pObjectPool->m_pObjects[objectid];
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedOffset.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedOffset.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedOffset.fZ);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedRotation.fX);
	amx_GetAddr(amx, params[6], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedRotation.fY);
	amx_GetAddr(amx, params[7], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedRotation.fZ);
	return 1;
}

// native IsObjectMaterialSlotUsed(objectid, materialindex); // Return values: 1 = material, 2 = material text
static cell AMX_NATIVE_CALL Natives::IsObjectMaterialSlotUsed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "IsObjectMaterialSlotUsed");

	int objectid = (int)params[1];
	if(objectid < 0 || objectid >= 1000) return 0;

	int materialindex = (int)params[2];
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->m_pObjects[objectid];
	
	// Nothing to comment here..
	while(i != 16)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == 16) return 0;

	return pObject->Material[i].byteUsed;
}

// native GetObjectMaterial(objectid, materialindex, &modelid, txdname[], txdnamelen = sizeof(txdname), texturename[], texturenamelen = sizeof(texturename), &materialcolor);
static cell AMX_NATIVE_CALL Natives::GetObjectMaterial( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(8, "GetObjectMaterial");

	int objectid = (int)params[1];
	int materialindex = (int)params[2];

	if(objectid < 0 || objectid >= 1000) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->m_pObjects[objectid];

	// Nothing to comment here..
	while(i != 16)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == 16) return 0;

	cell *cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)pObject->Material[i].wModelID; //  modelid

	set_amxstring(amx, params[4], pObject->Material[i].szMaterialTXD, params[5]); // txdname[], txdnamelen = sizeof(txdname)
	set_amxstring(amx, params[6], pObject->Material[i].szMaterialTexture, params[7]); // texturenamelen = sizeof(txdnamelen),

	amx_GetAddr(amx, params[8], &cptr);
	*cptr = ABGR_ARGB(pObject->Material[i].dwMaterialColor); // materialcolor
	return 1;
}

// native GetObjectMaterialText(objectid, materialindex, text[], textlen = sizeof(text), &materialsize, fontface[], fontfacelen = sizeof(fontface), &fontsize, &bold, &fontcolor, &backcolor, &textalignment);
static cell AMX_NATIVE_CALL Natives::GetObjectMaterialText( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(12, "GetObjectMaterialText");

	int objectid = (int)params[1];
	int materialindex = (int)params[2];

	if(objectid < 0 || objectid >= 1000) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->m_bObjectSlotState[objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->m_pObjects[objectid];

	// Nothing to comment here..
	while(i != 16)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == 16) return 0;

	cell *cptr;

	set_amxstring(amx, params[3], pObject->Material[i].szMaterialTXD, params[4]); 

	amx_GetAddr(amx, params[5], &cptr);
	*cptr = (cell)pObject->Material[i].byteMaterialSize; // materialsize

	set_amxstring(amx, params[6], pObject->Material[i].szFont, params[7]); 

	amx_GetAddr(amx, params[8], &cptr);
	*cptr = (cell)pObject->Material[i].byteFontSize; // fontsize
	amx_GetAddr(amx, params[9], &cptr);
	*cptr = (cell)pObject->Material[i].byteBold; // bold
	amx_GetAddr(amx, params[10], &cptr);
	*cptr = (cell)pObject->Material[i].dwFontColor; // fontcolor
	amx_GetAddr(amx, params[11], &cptr);
	*cptr = (cell)pObject->Material[i].dwBackgroundColor; // backcolor
	amx_GetAddr(amx, params[12], &cptr);
	*cptr = (cell)pObject->Material[i].byteAlignment; // textalignment
	return 1;
}

// native GetPlayerObjectModel(playerid, objectid);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectModel( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GetPlayerObjectModel");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid > 1000) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	return pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid]->iModel;
}

// native Float:GetPlayerObjectDrawDistance(playerid, objectid);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectDrawDistance( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GetPlayerObjectDrawDistance");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid]->fDrawDistance);
}

// native Float:SetPlayerObjectMoveSpeed(playerid, objectid, Float:fSpeed);
static cell AMX_NATIVE_CALL Natives::SetPlayerObjectMoveSpeed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "SetPlayerObjectMoveSpeed");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid]->fMoveSpeed = amx_ctof(params[3]);
	return 1;
}

// native Float:GetPlayerObjectMoveSpeed(playerid, objectid);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectMoveSpeed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GetPlayerObjectMoveSpeed");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	return amx_ftoc(pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid]->fMoveSpeed);
}

// native Float:GetPlayerObjectTarget(playerid, objectid, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectTarget( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(5, "GetPlayerObjectTarget");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	cell* cptr;
	CObject *pObject = pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid];
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pObject->matTarget.pos.fX);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pObject->matTarget.pos.fY);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pObject->matTarget.pos.fZ);
	return 1;
}

// native GetPlayerObjectAttachedData(playerid, objectid, &vehicleid, &objectid);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectAttachedData( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GetPlayerObjectAttachedData");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;
	
	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	cell* cptr;
	CObject *pObject = pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid];
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)pObject->wAttachedVehicleID;
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = (cell)pObject->wAttachedVehicleID;
	return 1;
}

// native GetPlayerObjectAttachedOffset(playerid, objectid, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectAttachedOffset( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GetPlayerObjectModel");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	cell* cptr;
	CObject *pObject = pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid];
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedOffset.fX);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedOffset.fY);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedOffset.fZ);
	amx_GetAddr(amx, params[6], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedRotation.fX);
	amx_GetAddr(amx, params[7], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedRotation.fY);
	amx_GetAddr(amx, params[8], &cptr);
	*cptr = amx_ftoc(pObject->vecAttachedRotation.fZ);
	return 1;
}

// native IsPlayerObjectMaterialSlotUsed(playerid, objectid, materialindex); // Return values: 1 = material, 2 = material text
static cell AMX_NATIVE_CALL Natives::IsPlayerObjectMaterialSlotUsed( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "IsPlayerObjectMaterialSlotUsed");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	int materialindex = (int)params[3];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid];
	
	// Nothing to comment here..
	while(i != 16)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == 16) return 0;

	return pObject->Material[i].byteUsed;
}


// native GetPlayerObjectMaterial(playerid, objectid, materialindex, &modelid, txdname[], txdnamelen = sizeof(txdname), texturename[], texturenamelen = sizeof(texturename), &materialcolor);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectMaterial( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(9, "GetPlayerObjectMaterial");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	int materialindex = (int)params[3];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	cell* cptr;
	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid];
	
	// Nothing to comment here..
	while(i != 16)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == 16) return 0;

	amx_GetAddr(amx, params[4], &cptr);
	*cptr = (cell)pObject->Material[i].wModelID; //  modelid

	set_amxstring(amx, params[5], pObject->Material[i].szMaterialTXD, params[6]); // txdname[], txdnamelen = sizeof(txdname)
	set_amxstring(amx, params[7], pObject->Material[i].szMaterialTexture, params[8]); // texturenamelen = sizeof(txdnamelen),

	amx_GetAddr(amx, params[9], &cptr);
	*cptr = ABGR_ARGB(pObject->Material[i].dwMaterialColor); // materialcolor
	return 1;
}

// native GetPlayerObjectMaterialText(playerid, objectid, materialindex, text[], textlen = sizeof(text), &materialsize, fontface[], fontfacelen = sizeof(fontface), &fontsize, &bold, &fontcolor, &backcolor, &textalignment);
static cell AMX_NATIVE_CALL Natives::GetPlayerObjectMaterialText( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(13, "GetPlayerObjectMaterialText");

	int playerid = (int)params[1];
	int objectid = (int)params[2];
	int materialindex = (int)params[3];
	if(!IsPlayerConnected(playerid)) return 0;
	if(objectid < 0 || objectid >= 1000) return 0;
	if(materialindex < 0 || materialindex >= 16) return 0;

	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;

	cell* cptr;
	int i = 0;
	CObject *pObject = pNetGame->pObjectPool->m_pPlayerObjects[playerid][objectid];
	
	// Nothing to comment here..
	while(i != 16)
	{
		if(pObject->Material[i].byteSlot == materialindex) break;
		i++;
	}
	if(i == 16) return 0;

	set_amxstring(amx, params[4], pObject->Material[i].szMaterialTXD, params[5]); 

	amx_GetAddr(amx, params[6], &cptr);
	*cptr = (cell)pObject->Material[i].byteMaterialSize; // materialsize

	set_amxstring(amx, params[7], pObject->Material[i].szFont, params[8]); 

	amx_GetAddr(amx, params[9], &cptr);
	*cptr = (cell)pObject->Material[i].byteFontSize; // fontsize
	amx_GetAddr(amx, params[10], &cptr);
	*cptr = (cell)pObject->Material[i].byteBold; // bold
	amx_GetAddr(amx, params[11], &cptr);
	*cptr = (cell)pObject->Material[i].dwFontColor; // fontcolor
	amx_GetAddr(amx, params[12], &cptr);
	*cptr = (cell)pObject->Material[i].dwBackgroundColor; // backcolor
	amx_GetAddr(amx, params[13], &cptr);
	*cptr = (cell)pObject->Material[i].byteAlignment; // textalignment
	return 1;
}

// native GetPlayerAttachedObject(playerid, index, &modelid, &bone, &Float:fX, &Float:fY, &Float:fZ, &Float:fRotX, &Float:fRotY, &Float:fRotZ, Float:&fSacleX, Float:&fScaleY, Float:&fScaleZ, &materialcolor1, &materialcolor2);
static cell AMX_NATIVE_CALL Natives::GetPlayerAttachedObject( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(15, "GetPlayerAttachedObject");

	int playerid = (int)params[1];
	int slot = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(slot < 0 || slot >= 10) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->attachedObjectSlot[slot]) return 0;

	cell* cptr;
	CAttachedObject *pObject = &pNetGame->pPlayerPool->pPlayer[playerid]->attachedObject[slot];
	
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = *(cell*)&pObject->iModelID;	
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = *(cell*)&pObject->iBoneiD;	
	
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pObject->vecPos.fX);
	amx_GetAddr(amx, params[6], &cptr);
	*cptr = amx_ftoc(pObject->vecPos.fY);
	amx_GetAddr(amx, params[7], &cptr);
	*cptr = amx_ftoc(pObject->vecPos.fZ);

	amx_GetAddr(amx, params[8], &cptr);
	*cptr = amx_ftoc(pObject->vecRot.fX);
	amx_GetAddr(amx, params[9], &cptr);
	*cptr = amx_ftoc(pObject->vecRot.fY);
	amx_GetAddr(amx, params[10], &cptr);
	*cptr = amx_ftoc(pObject->vecRot.fZ);

	amx_GetAddr(amx, params[11], &cptr);
	*cptr = amx_ftoc(pObject->vecScale.fX);
	amx_GetAddr(amx, params[12], &cptr);
	*cptr = amx_ftoc(pObject->vecScale.fY);
	amx_GetAddr(amx, params[13], &cptr);
	*cptr = amx_ftoc(pObject->vecScale.fZ);

	amx_GetAddr(amx, params[14], &cptr);
	*cptr = RGBA_ABGR(pObject->dwMaterialColor1);
	amx_GetAddr(amx, params[15], &cptr);
	*cptr = RGBA_ABGR(pObject->dwMaterialColor2);
	return 1;
}

// native IsPlayerEditingObject(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerEditingObject( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerEditingObject");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bEditObject;
}

// native IsPlayerEditingAttachedObject(playerid);
static cell AMX_NATIVE_CALL Natives::IsPlayerEditingAttachedObject( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerEditingAttachedObject");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bEditAttachedObject;
}

// Vehicle functions
// native GetVehicleSpawnInfo(vehicleid, &Float:fX, &Float:fY, &Float:fZ, &Float:fRot, &color1, &color2);
static cell AMX_NATIVE_CALL Natives::GetVehicleSpawnInfo( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(7, "GetVehicleSpawnInfo");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.vecPos.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.vecPos.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.vecPos.fZ);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.fRot);
	amx_GetAddr(amx, params[6], &cptr);
	*cptr = (cell)pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iColor1;
	amx_GetAddr(amx, params[7], &cptr);
	*cptr = (cell)pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iColor2;
	return 1;
}

// native GetVehicleColor(vehicleid, &color1, &color2);
static cell AMX_NATIVE_CALL Natives::GetVehicleColor( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "GetVehicleColor");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = *(cell*)&pNetGame->pVehiclePool->pVehicle[vehicleid]->vehModInfo.iColor1;	
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = *(cell*)&pNetGame->pVehiclePool->pVehicle[vehicleid]->vehModInfo.iColor2;
	return 1;
}

// native GetVehiclePaintjob(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehiclePaintjob( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehiclePaintjob");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->vehModInfo.bytePaintJob - 1;
}

// native GetVehicleInterior(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehicleInterior( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehicleInterior");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iInterior;
}

// native GetVehicleNumberPlate(vehicleid, plate[], len = sizeof(plate));
static cell AMX_NATIVE_CALL Natives::GetVehicleNumberPlate( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "GetVehicleNumberPlate");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return set_amxstring(amx, params[2], pNetGame->pVehiclePool->pVehicle[vehicleid]->szNumberplate, params[3]);
}

// native SetVehicleRespawnDelay(vehicleid, delay);
static cell AMX_NATIVE_CALL Natives::SetVehicleRespawnDelay( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetVehicleRespawnDelay");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iRespawnTime = ((int)params[2] * 1000);
	return 1;
}

// native GetVehicleRespawnDelay(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehicleRespawnDelay( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehicleRespawnDelay");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->customSpawn.iRespawnTime / 1000;
}

// native SetVehicleOccupiedTick(vehicleid, ticks);
static cell AMX_NATIVE_CALL Natives::SetVehicleOccupiedTick( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetVehicleOccupiedTick");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->vehOccupiedTick = (int)params[2];
	return 1;
}

// native GetVehicleOccupiedTick(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehicleOccupiedTick( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehicleOccupiedTick");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->vehOccupiedTick;
}

// native SetVehicleRespawnTick(vehicleid, ticks);
static cell AMX_NATIVE_CALL Natives::SetVehicleRespawnTick( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetVehicleRespawnTick");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->vehRespawnTick = (int)params[2];
	return 1;
}

// native GetVehicleRespawnTick(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehicleRespawnTick( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehicleRespawnTick");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->vehRespawnTick;
}

// native GetVehicleLastDriver(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehicleLastDriver( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehicleLastDriver");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->wLastDriverID;
}

// native GetVehicleCab(vehicleid);
static cell AMX_NATIVE_CALL Natives::GetVehicleCab( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetVehicleCab");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;
	
	CVehicle *pVeh;
	for(WORD i = 0; i != MAX_VEHICLES; i++)
	{
		pVeh = pNetGame->pVehiclePool->pVehicle[i];
		if(!pVeh) continue;

		if(pVeh->wTrailerID != 0 && pVeh->wTrailerID == vehicleid)
			return i;
	}
	return 0;
}

// native HasVehicleBeenOccupied(vehicleid);
static cell AMX_NATIVE_CALL Natives::HasVehicleBeenOccupied( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "HasVehicleBeenOccupied");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->bOccupied;
}

// native SetVehicleBeenOccupied(vehicleid, occupied);
static cell AMX_NATIVE_CALL Natives::SetVehicleBeenOccupied( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetVehicleBeenOccupied");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	pNetGame->pVehiclePool->pVehicle[vehicleid]->bOccupied = !!params[2];;
	return 1;
}

// native IsVehicleOccupied(vehicleid);
static cell AMX_NATIVE_CALL Natives::IsVehicleOccupied( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsVehicleOccupied");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	CPlayer *pPlayer;
	for(WORD i = 0; i != MAX_PLAYERS; i++)
	{
		if(!IsPlayerConnected(i)) continue; 
		pPlayer = pNetGame->pPlayerPool->pPlayer[i];

		if(pPlayer->wVehicleId == vehicleid && (pPlayer->byteState == PLAYER_STATE_DRIVER || pPlayer->byteState == PLAYER_STATE_PASSENGER)) 
			return 1;
	}
	return 0;
}

// native IsVehicleDead(vehicleid);
static cell AMX_NATIVE_CALL Natives::IsVehicleDead( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsVehicleDead");

	int vehicleid = (int)params[1];
	if(vehicleid < 1 || vehicleid >= 2000) return 0;
	
	if(!pNetGame->pVehiclePool->pVehicle[vehicleid]) 
		return 0;

	return pNetGame->pVehiclePool->pVehicle[vehicleid]->bDead;
}

// Gangzone functions
// native IsValidGangZone(zoneid);
static cell AMX_NATIVE_CALL Natives::IsValidGangZone( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "IsValidGangZone");
	
	int zoneid = (int)params[1];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	return pNetGame->pGangZonePool->GetSlotState(zoneid);
}

// native IsGangZoneVisibleForPlayer(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsGangZoneVisibleForPlayer( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsGangZoneVisibleForPlayer");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!pNetGame->pGangZonePool->GetSlotState(zoneid)) return 0;

	return !!(pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid) != 0xFF);
}

// native GangZoneGetColorForPlayer(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::GangZoneGetColorForPlayer( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GangZoneGetColorForPlayer");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pNetGame->pGangZonePool->GetSlotState(zoneid)) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneColor[id];
	}
	return 0;
}

// native GangZoneGetFlashColorForPlayer(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::GangZoneGetFlashColorForPlayer( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GangZoneGetFlashColorForPlayer");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pNetGame->pGangZonePool->GetSlotState(zoneid)) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneFlashColor[id];
	}
	return 0;
}

// native IsGangZoneFlashingForPlayer(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsGangZoneFlashingForPlayer( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsGangZoneFlashingForPlayer");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pNetGame->pGangZonePool->GetSlotState(zoneid)) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bIsGangZoneFlashing[id];
	}
	return 0;
}

// native IsPlayerInGangZone(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsPlayerInGangZone( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsPlayerInGangZone");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pNetGame->pGangZonePool->GetSlotState(zoneid)) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bInGangZone[id];
	}
	return 0;
}

// native GangZoneGetPos(zoneid, &Float:fMinX, &Float:fMinY, &Float:fMaxX, &Float:fMaxY);
static cell AMX_NATIVE_CALL Natives::GangZoneGetPos( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(5, "GangZoneGetPos");

	int zoneid = (int)params[1];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pNetGame->pGangZonePool->GetSlotState(zoneid))  return 0;
	
	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pNetGame->pGangZonePool->pGangZone[zoneid]->fGangZone[0]);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pNetGame->pGangZonePool->pGangZone[zoneid]->fGangZone[1]);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pNetGame->pGangZonePool->pGangZone[zoneid]->fGangZone[2]);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pNetGame->pGangZonePool->pGangZone[zoneid]->fGangZone[3]);
	return 1;
}

// Textdraw functions
// native IsValidTextDraw(textdrawid);
static cell AMX_NATIVE_CALL Natives::IsValidTextDraw( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "IsValidTextDraw");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	return pNetGame->pTextDrawPool->bSlotState[textdrawid];
}

// native IsTextDrawVisibleForPlayer(playerid, textdrawid);
static cell AMX_NATIVE_CALL Natives::IsTextDrawVisibleForPlayer( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsTextDrawVisibleForPlayer");
	
	int playerid = (int)params[1];
	if(playerid < 0 || playerid >= MAX_PLAYERS) return 0;

	int textdrawid = (int)params[2];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;

	return pNetGame->pTextDrawPool->bHasText[textdrawid][playerid];
}

// native TextDrawGetString(textdrawid, text[], len = sizeof(text));
static cell AMX_NATIVE_CALL Natives::TextDrawGetString( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "TextDrawGetString");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	const char *szText = (pNetGame->pTextDrawPool->bSlotState[textdrawid]) ? pNetGame->pTextDrawPool->szFontText[textdrawid] : '\0';
	return set_amxstring(amx, params[2], szText, params[3]);
}

// native TextDrawSetPos(textdrawid, Float:fX, Float:fY);
static cell AMX_NATIVE_CALL Natives::TextDrawSetPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "TextDrawPos");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	pTD->fX = amx_ctof(params[2]);
	pTD->fY = amx_ctof(params[3]);
	return 1;
}

// native TextDrawGetLetterSize(textdrawid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::TextDrawGetLetterSize( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "TextDrawGetLetterSize");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pTD->fLetterWidth);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->fLetterHeight);
	return 1;
}

// native TextDrawGetFontSize(textdrawid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::TextDrawGetFontSize( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "TextDrawGetFontSize");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pTD->fLineWidth);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->fLineHeight);
	return 1;
}

// native TextDrawGetPos(textdrawid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::TextDrawGetPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "TextDrawGetPos");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pTD->fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->fY);
	return 1;
}

// native TextDrawGetColor(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetColor");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return ABGR_RGBA(pTD->dwLetterColor);
}

// native TextDrawGetBoxColor(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetBoxColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetBoxColor");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return ABGR_RGBA(pTD->dwBoxColor);
}

// native TextDrawGetBackgroundColor(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetBackgroundColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetBackgroundColor");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return ABGR_RGBA(pTD->dwBackgroundColor);
}

// native TextDrawGetShadow(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetShadow( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetShadow");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteShadow;
}

// native TextDrawGetOutline(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetOutline( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetOutline");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteOutline;
}

// native TextDrawGetFont(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetFont( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetOutline");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteStyle;
}

// native TextDrawIsBox(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawIsBox( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawIsBox");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteBox;
}

// native TextDrawIsProportional(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawIsProportional( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawIsProportional");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteProportional;
}

// native TextDrawIsSelectable(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawIsSelectable( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawIsSelectable");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->byteSelectable;
}

// native TextDrawGetAlignment(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetAlignment( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetAlignment");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	BYTE ret;

	if(pTD->byteCenter) ret = 2;
	else if(pTD->byteLeft) ret = 1;
	else if(pTD->byteRight) ret = 3;
	return ret;
}

// native TextDrawGetPreviewModel(textdrawid);
static cell AMX_NATIVE_CALL Natives::TextDrawGetPreviewModel( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "TextDrawGetPreviewModel");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	return pTD->dwModelIndex;
}

// native TextDrawGetPreviewRot(textdrawid, &Float:fRotX, &Float:fRotY, &Float:fRotZ, &Float:fZoom);
static cell AMX_NATIVE_CALL Natives::TextDrawGetPreviewRot( AMX* amx, cell* params )
{
	CHECK_PARAMS(5, "TextDrawGetPreviewRot");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pTD->vecRot.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->vecRot.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pTD->vecRot.fZ);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pTD->fZoom);
	return 1;
}

// native TextDrawGetPreviewVehCol(textdrawid, &color1, &color2);
static cell AMX_NATIVE_CALL Natives::TextDrawGetPreviewVehCol( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "TextDrawGetPreviewVehCol");
	
	int textdrawid = (int)params[1];
	if(textdrawid < 0 || textdrawid >= MAX_TEXT_DRAWS) return 0;
	
	if(!pNetGame->pTextDrawPool->bSlotState[textdrawid]) return 0;
	CTextdraw *pTD = pNetGame->pTextDrawPool->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = (cell)pTD->color1;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)pTD->color2;
	return 1;
}

// Per-Player textdraws
// native IsValidPlayerTextDraw(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::IsValidPlayerTextDraw( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsValidPlayerTextDraw");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid];
}

// native IsPlayerTextDrawVisible(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::IsPlayerTextDrawVisible( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsPlayerTextDrawVisible");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bHasText[textdrawid];
}

// native PlayerTextDrawGetString(playerid, PlayerText:textdrawid, text[], len = sizeof(text));
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetString( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "PlayerTextDrawGetString");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	
	bool bIsValid = !!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid];
	if(!bIsValid) return 0;

	const char *szText = (bIsValid) ? pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->szFontText[textdrawid]: '\0';
	return set_amxstring(amx, params[3], szText, params[4]);
}

// native PlayerTextDrawSetPos(playerid, PlayerText:textdrawid, Float:fX, Float:fY);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawSetPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "PlayerTextDrawSetPos");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	pTD->fX = amx_ctof(params[3]);
	pTD->fY = amx_ctof(params[4]);
	return 1;
}

// native PlayerTextDrawGetLetterSize(playerid, PlayerText:textdrawid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetLetterSize( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "PlayerTextDrawGetLetterSize");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->fLetterWidth);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pTD->fLetterHeight);
	return 1;
}

// native PlayerTextDrawGetFontSize(playerid, PlayerText:textdrawid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetFontSize( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "PlayerTextDrawGetFontSize");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->fLineWidth);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pTD->fLineHeight);
	return 1;
}

// native PlayerTextDrawGetPos(playerid, PlayerText:textdrawid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "PlayerTextDrawGetPos");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->fX);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pTD->fY);
	return 1;
}

// native PlayerTextDrawGetColor(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetColor");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	/*
	int color = (int)pTD->dwLetterColor;
	BYTE r, g, b, a;

	r = color & 0xff;
	g = (color >> 8) & 0xff;;
	b = (color >> 16) & 0xff;;
	a = (color >> 24) & 0xff;;
	logprintf("r: %X, g: %X, b: %X, a: %X", r, g, b, a);
	return (((DWORD)r) << 24) | (((DWORD)g) << 16) | (((DWORD)b) << 8) | a;
	*/
	return ABGR_RGBA(pTD->dwLetterColor);
}

// native PlayerTextDrawGetBoxColor(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetBoxColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetBoxColor");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return ABGR_RGBA(pTD->dwBoxColor);
}

// native PlayerTextDrawGetBackgroundCol(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetBackgroundCol( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetBackgroundCol");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return ABGR_RGBA(pTD->dwBackgroundColor);
}

// native PlayerTextDrawGetShadow(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetShadow( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetShadow");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteShadow;
}

// native PlayerTextDrawGetOutline(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetOutline( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetOutline");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteOutline;
}

// native PlayerTextDrawGetFont(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetFont( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetFont");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteStyle;
}

// native PlayerTextDrawIsBox(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawIsBox( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawIsBox");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteBox;
}

// native PlayerTextDrawIsProportional(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawIsProportional( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawIsProportional");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteProportional;
}

// native PlayerTextDrawIsSelectable(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawIsSelectable( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawIsSelectable");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->byteSelectable;
}

// native PlayerTextDrawGetAlignment(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetAlignment( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetAlignment");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	BYTE ret;

	if(pTD->byteCenter) ret = 2;
	else if(pTD->byteLeft) ret = 1;
	else if(pTD->byteRight) ret = 3;
	return ret;
}

// native PlayerTextDrawGetPreviewModel(playerid, PlayerText:textdrawid);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetPreviewModel( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerTextDrawGetPreviewModel");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];
	return pTD->dwModelIndex;
}

// native PlayerTextDrawGetPreviewRot(playerid, PlayerText:textdrawid, &Float:fRotX, &Float:fRotY, &Float:fRotZ, &Float:fZoom);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetPreviewRot( AMX* amx, cell* params )
{
	CHECK_PARAMS(6, "PlayerTextDrawGetPreviewRot");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pTD->vecRot.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pTD->vecRot.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pTD->vecRot.fZ);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pTD->fZoom);
	return 1;
}

// native PlayerTextDrawGetPreviewVehCol(playerid, PlayerText:textdrawid, &color1, &color2);
static cell AMX_NATIVE_CALL Natives::PlayerTextDrawGetPreviewVehCol( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "PlayerTextDrawGetPreviewVehCol");
	
	int playerid = (int)params[1];
	int textdrawid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(textdrawid >= MAX_PLAYER_TEXT_DRAWS) return 0;
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->bSlotState[textdrawid]) return 0;

	CTextdraw *pTD = pNetGame->pPlayerPool->pPlayer[playerid]->pTextdraw->TextDraw[textdrawid];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = (cell)pTD->color1;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)pTD->color2;
	return 1;
}

// 3D Text labels
// native IsValid3DTextLabel(Text3D:id);
static cell AMX_NATIVE_CALL Natives::IsValid3DTextLabel( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "IsValid3DTextLabel");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	return pNetGame->p3DTextPool->m_bIsCreated[id];
}

// native Is3DTextLabelStreamedIn(playerid, Text3D:id);
static cell AMX_NATIVE_CALL Natives::Is3DTextLabelStreamedIn( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "Is3DTextLabelStreamedIn");
	
	int playerid = (int)params[1];
	int id = (int)params[1];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	return pNetGame->pPlayerPool->pPlayer[playerid]->byte3DTextLabelStreamedIn[id];
}

// native Get3DTextLabelText(id, text[], len = sizeof(text));
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelText( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "Get3DTextLabelText");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	const char *szText = (pNetGame->p3DTextPool->m_bIsCreated[id]) ? pNetGame->p3DTextPool->m_TextLabels[id].text : '\0';
	return set_amxstring(amx, params[2], szText, params[3]);
}

// native Get3DTextLabelColor(id);
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "Get3DTextLabelColor");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->m_bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->m_TextLabels[id];

	return p3DText.color;
}

// native Get3DTextLabelPos(id, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "Get3DTextLabelPos");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->m_bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->m_TextLabels[id];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(p3DText.posX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(p3DText.posY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(p3DText.posZ);
	return 1;
}

// native Float:Get3DTextLabelDrawDistance(id);
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelDrawDistance( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "Get3DTextLabelDrawDistance");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->m_bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->m_TextLabels[id];

	return amx_ftoc(p3DText.drawDistance);
}

// native Get3DTextLabelLOS(id);
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelLOS( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "Get3DTextLabelLOS");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->m_bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->m_TextLabels[id];

	return p3DText.useLineOfSight;
}

// native Get3DTextLabelVirtualWorld(id);
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelVirtualWorld( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "Get3DTextLabelVirtualWorld");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->m_bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->m_TextLabels[id];

	return p3DText.virtualWorld;
}

// native Get3DTextLabelAttachedData(id, &attached_playerid, &attached_vehicleid);
static cell AMX_NATIVE_CALL Natives::Get3DTextLabelAttachedData( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "Get3DTextLabelAttachedData");
	
	int id = (int)params[1];
	if(0 < id || id >= MAX_3DTEXT_GLOBAL) return 0;
	
	if(!pNetGame->p3DTextPool->m_bIsCreated[id]) return 0;
	C3DText p3DText = pNetGame->p3DTextPool->m_TextLabels[id];

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = (cell)p3DText.attachedToPlayerID;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)p3DText.attachedToVehicleID;
	return 1;
}

// native IsValidPlayer3DTextLabel(playerid, PlayerText3D:id);
static cell AMX_NATIVE_CALL Natives::IsValidPlayer3DTextLabel( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsValidPlayer3DTextLabel");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	return pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id];
}

// native GetPlayer3DTextLabelText(playerid, PlayerText3D:id, text[], len = sizeof(text));
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelText( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(4, "GetPlayer3DTextLabelText");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	const char *szText = (pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id].text) ? pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id].text : '\0';
	return set_amxstring(amx, params[3], szText, params[4]);
}

// native GetPlayer3DTextLabelColor(playerid, PlayerText3D:id);
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GetPlayer3DTextLabelColor");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return p3DText.color;
}

// native GetPlayer3DTextLabelPos(playerid, PlayerText3D:id, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(5, "GetPlayer3DTextLabelPos");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;

	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;
	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(p3DText.posX);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(p3DText.posY);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(p3DText.posZ);
	return 1;
}

// native Float:GetPlayer3DTextLabelDrawDist(playerid, PlayerText3D:id);
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelDrawDist( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GetPlayer3DTextLabelDrawDist");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return amx_ftoc(p3DText.drawDistance);
}

// native GetPlayer3DTextLabelLOS(playerid, PlayerText3D:id);
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelLOS( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GetPlayer3DTextLabelLOS");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return p3DText.useLineOfSight;
}

// native GetPlayer3DTextLabelVirtualW(playerid, PlayerText3D:id);
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelVirtualW( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GetPlayer3DTextLabelVirtualW");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	return p3DText.virtualWorld;
}

// native GetPlayer3DTextLabelAttached(playerid, PlayerText3D:id, &attached_playerid, &attached_vehicleid);
static cell AMX_NATIVE_CALL Natives::GetPlayer3DTextLabelAttached( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "GetPlayer3DTextLabelAttached");
	
	int playerid = (int)params[1];
	int id = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(0 < id || id >= MAX_3DTEXT_PLAYER) return 0;
	
	if(!pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->isCreated[id]) return 0;

	C3DText p3DText = pNetGame->pPlayerPool->pPlayer[playerid]->p3DText->TextLabels[id];
	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = (cell)p3DText.attachedToPlayerID;
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = (cell)p3DText.attachedToVehicleID;
	return 1;
}

static cell AMX_NATIVE_CALL Natives::FIXED_AttachPlayerObjectToPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(9, "AttachPlayerObjectToPlayer");

	int playerid = (int)params[1];
	int attachplayerid = (int)params[3];
	int objectid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(!IsPlayerConnected(attachplayerid)) return 0;

	if(objectid < 1 || objectid >= 1000) return 0;
	if(!pNetGame->pObjectPool->m_bPlayerObjectSlotState[playerid][objectid]) return 0;
	
	pPlayerData[playerid]->stObj[objectid].usAttachPlayerID = attachplayerid;
	pPlayerData[playerid]->stObj[objectid].vecOffset = CVector(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6]));
	pPlayerData[playerid]->stObj[objectid].vecRot = CVector(amx_ctof(params[7]), amx_ctof(params[8]), amx_ctof(params[9]));

	RakNet::BitStream bs;
	bs.Write((WORD)objectid); // wObjectID
	bs.Write((WORD)attachplayerid); // playerid
	bs.Write(amx_ctof(params[4]));
	bs.Write(amx_ctof(params[5]));
	bs.Write(amx_ctof(params[6]));
	bs.Write(amx_ctof(params[7]));
	bs.Write(amx_ctof(params[8]));
	bs.Write(amx_ctof(params[9]));

	pRakServer->RPC(&RPC_AttachObject, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), 0, 0);
	return 1;
}

// native YSF_AddPlayer(playerid);
static cell AMX_NATIVE_CALL Natives::YSF_AddPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	// We only allow to call this function from gamemode
	if (&pNetGame->pGameModePool->m_amx != amx) return 0;

	CHECK_PARAMS(1, "YSF_AddPlayer");

	int playerid = (int)params[1];
	PlayerID playerId = pRakServer->GetPlayerIDFromIndex(playerid);
	
	// If player is not connected
	if (playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress)
		return 0;

	cell ret = pServer->AddPlayer(playerid);
	
#ifdef NEW_PICKUP_SYSTEM
	// Initialize pickups
	if(ret)
		pNetGame->pPickupPool->InitializeForPlayer(playerid);
#endif
	return ret;
}

// native YSF_RemovePlayer(playerid);
static cell AMX_NATIVE_CALL Natives::YSF_RemovePlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	// We only allow to call this function from gamemode
	if (&pNetGame->pGameModePool->m_amx != amx) return 0;

	CHECK_PARAMS(1, "YSF_RemovePlayer");
	
	//logprintf("YSF_RemovePlayer - connected: %d, raknet geci: %d", pNetGame->pPlayerPool->bIsPlayerConnected[(int)params[1]], pRakServer->GetPlayerIDFromIndex((int)params[1]).binaryAddress);
	int playerid = (int)params[1];
	pServer->RemovePlayer(playerid);
	return 1;
}

// native YSF_StreamIn(playerid, forplayerid);
static cell AMX_NATIVE_CALL Natives::YSF_StreamIn( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	// We only allow to call this function from gamemode
	if (&pNetGame->pGameModePool->m_amx != amx) return 0;

	CHECK_PARAMS(2, "YSF_StreamIn");

	pServer->OnPlayerStreamIn((WORD)params[1], (WORD)params[2]);
	return 1;
}

// native YSF_StreamOut(playerid, forplayerid);
static cell AMX_NATIVE_CALL Natives::YSF_StreamOut( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	// We only allow to call this function from gamemode
	if (&pNetGame->pGameModePool->m_amx != amx) return 0;

	CHECK_PARAMS(2, "YSF_StreamOut");

	pServer->OnPlayerStreamOut((WORD)params[1], (WORD)params[2]);
	return 1;
}

static cell AMX_NATIVE_CALL Natives::YSF_GangZoneCreate(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "GangZoneCreate");

	float fMinX = amx_ctof(params[1]);
	float fMinY = amx_ctof(params[2]);
	float fMaxX = amx_ctof(params[3]);
	float fMaxY = amx_ctof(params[4]);

	// If coordinates are wrong, then won't create bugged zone!
	if(fMaxX <= fMinX || fMaxY <= fMinY) 
	{
		logprintf("GangZoneCreate: MaxX, MaxY must be bigger than MinX, MinY. Not inversely!");
		logprintf("GangZoneCreate: %f, %f, %f, %f",fMinX, fMinY, fMaxX, fMaxY);
		return -1;
	}

	WORD ret = pNetGame->pGangZonePool->New(fMinX, fMinY, fMaxX, fMaxY);
	if (ret == 0xFFFF) return -1;

	return ret;
}

static cell AMX_NATIVE_CALL Natives::YSF_GangZoneDestroy(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "GangZoneDestroy");

	CGangZonePool *pGangZonePool = pNetGame->pGangZonePool;
	if (!pGangZonePool->GetSlotState((WORD)params[1])) return 0;

	pGangZonePool->Delete((WORD)params[1]);
	return 1;
}

// native YSF_GangZoneShowForPlayer(playerid, zone, color);
static cell AMX_NATIVE_CALL Natives::YSF_GangZoneShowForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "GangZoneShowForPlayer");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	DWORD color = (DWORD)params[3];

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->ShowForPlayer(playerid, zoneid, color);
	return 1;
}

// native YSF_GangZoneHideForPlayer(playerid, zone);
static cell AMX_NATIVE_CALL Natives::YSF_GangZoneHideForPlayer( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GangZoneHideForPlayer");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->HideForPlayer(playerid, zoneid);
	return 1;
}

// native YSF_GangZoneShowForAll(zone, color);
static cell AMX_NATIVE_CALL Natives::YSF_GangZoneShowForAll( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "GangZoneShowForAll");

	int zoneid = (int)params[1];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->ShowForAll(zoneid, (DWORD)params[2]);
	return 1;
}

// native YSF_GangZoneHideForAll(zone);
static cell AMX_NATIVE_CALL Natives::YSF_GangZoneHideForAll( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GangZoneHideForAll");

	int zoneid = (int)params[1];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->HideForAll(zoneid);
	return 1;
}

static cell AMX_NATIVE_CALL Natives::YSF_GangZoneFlashForPlayer(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "GangZoneFlashForPlayer");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->FlashForPlayer((WORD)playerid, (WORD)zoneid, (DWORD)params[3]);
	return 1;
}

static cell AMX_NATIVE_CALL Natives::YSF_GangZoneFlashForAll(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "GangZoneFlashForAll");

	int zoneid = (int)params[1];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->FlashForAll((WORD)zoneid, (DWORD)params[2]);
	return 1;
}

static cell AMX_NATIVE_CALL Natives::YSF_GangZoneStopFlashForPlayer(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "GangZoneStopFlashForPlayer");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];

	// For security
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->StopFlashForPlayer((WORD)playerid, (WORD)zoneid);
	return 1;
}

static cell AMX_NATIVE_CALL Natives::YSF_GangZoneStopFlashForAll(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "GangZoneStopFlashForAll");

	int zoneid = (int)params[1];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->StopFlashForAll((WORD)zoneid);
	return 1;
}

// Menu functions
// native IsMenuDisabled(Menu:menuid);
static cell AMX_NATIVE_CALL Natives::IsMenuDisabled( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "IsMenuDisabled");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;
	
	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	return !!(!pMenu->interaction.Menu);
}

// native IsMenuRowDisabled(Menu:menuid, row);
static cell AMX_NATIVE_CALL Natives::IsMenuRowDisabled( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsMenuRowDisabled");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;
	
	int itemid = (int)params[2];
	if(itemid < 0 || itemid >= 12) return 0;

	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	return !!(!pMenu->interaction.Row[itemid]);
}

// native GetMenuColumns(menuid);
static cell AMX_NATIVE_CALL Natives::GetMenuColumns( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "GetMenuColumns");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;
	
	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	return pMenu->columnsNumber;
}

// native GetMenuItems(menuid, column);
static cell AMX_NATIVE_CALL Natives::GetMenuItems( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "GetMenuItems");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	int column = (int)params[2];
	if(menuid < 0 || menuid > 2) return 0;

	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	return pMenu->itemsCount[column];
}

// native GetMenuPos(menuid, &Float:fX, &Float:fY);
static cell AMX_NATIVE_CALL Natives::GetMenuPos( AMX* amx, cell* params )
{
	CHECK_PARAMS(3, "GetMenuColumns");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	int column = (int)params[2];
	if(menuid < 0 || menuid > 2) return 0;

	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pMenu->posX);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pMenu->posY);
	return 1;
}

// native GetMenuColumnWidth(menuid, column, &Float:fColumn1, &Float:fColumn2);
static cell AMX_NATIVE_CALL Natives::GetMenuColumnWidth( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "GetMenuColumnWidth");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	int column = (int)params[2];
	if(menuid < 0 || menuid > 2) return 0;

	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pMenu->column1Width);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pMenu->column2Width);
	return 1;
}

// native GetMenuColumnHeader(menuid, column, header[], len = sizeof(header));
static cell AMX_NATIVE_CALL Natives::GetMenuColumnHeader( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "GetMenuColumnHeader");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	int column = (int)params[2];
	if(menuid < 0 || menuid > 2) return 0;

	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	return set_amxstring(amx, params[3], pMenu->headers[column], params[4]);
}

// native GetMenuItem(menuid, column, itemid, item[], len = sizeof(item));
static cell AMX_NATIVE_CALL Natives::GetMenuItem( AMX* amx, cell* params )
{
	CHECK_PARAMS(5, "GetMenuItem");
	
	int menuid = (int)params[1];
	if(menuid < 1 || menuid >= MAX_MENUS) return 0;

	int column = (int)params[2];
	if(menuid < 0 || menuid > 2) return 0;

	int itemid = (int)params[3];
	if(itemid < 0 || itemid >= 12) return 0;

	if(!pNetGame->pMenuPool->isCreated[menuid]) return 0;
	CMenu *pMenu = pNetGame->pMenuPool->menu[menuid];

	return set_amxstring(amx, params[4], pMenu->items[itemid][column], params[5]);
}

// native AttachPlayerObjectToObject(playerid, objectid, attachtoid, Float:OffsetX, Float:OffsetY, Float:OffsetZ, Float:RotX, Float:RotY, Float:RotZ, SyncRotation = 1);
static cell AMX_NATIVE_CALL Natives::AttachPlayerObjectToObject( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(10, "AttachPlayerObjectToObject");

	int forplayerid = (int)params[1];
	if(!IsPlayerConnected(forplayerid)) return 0;

	int wObjectID = (int)params[2];
	int wAttachTo = (int)params[3];
	short padding1 = -1;
	CVector vecOffset = CVector(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6]));
	CVector vecOffsetRot = CVector(amx_ctof(params[7]), amx_ctof(params[8]), amx_ctof(params[9]));
	BYTE byteSyncRot = !!params[10];

	CObjectPool *pObjectPool = pNetGame->pObjectPool;

	if(!pObjectPool->m_pPlayerObjects[forplayerid][wObjectID] || !pObjectPool->m_pPlayerObjects[forplayerid][wAttachTo]) return 0; // Check if object is exist
	
	int iModelID = pObjectPool->m_pPlayerObjects[forplayerid][wObjectID]->iModel;
	CVector vecPos = pObjectPool->m_pPlayerObjects[forplayerid][wObjectID]->matWorld.pos;
	CVector vecRot = pObjectPool->m_pPlayerObjects[forplayerid][wObjectID]->vecRot;
	float fDrawDistance = pObjectPool->m_pPlayerObjects[forplayerid][wObjectID]->fDrawDistance;
	
	RakNet::BitStream bs;
	bs.Write(wObjectID);
	bs.Write(iModelID);
	bs.Write(vecPos);
	bs.Write(vecRot);
	bs.Write(fDrawDistance);
	bs.Write(padding1);
	bs.Write(wAttachTo);
	bs.Write(vecOffset);
	bs.Write(vecOffsetRot);	
	bs.Write(byteSyncRot);
	
	pRakServer->RPC(&RPC_CreateObject, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(forplayerid), 0, 0); // Send this on same RPC as CreateObject
	return 1;
}

// native CreatePlayerGangZone(playerid, Float:minx, Float:miny, Float:maxx, Float:maxy);
static cell AMX_NATIVE_CALL Natives::CreatePlayerGangZone( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(5, "CreatePlayerGangZone");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	float fMinX = amx_ctof(params[2]);
	float fMinY = amx_ctof(params[3]);
	float fMaxX = amx_ctof(params[4]);
	float fMaxY = amx_ctof(params[5]);

	// If coordinates are wrong, then won't create bugged zone!
	if(fMaxX <= fMinX || fMaxY <= fMinY) 
	{
		logprintf("CreatePlayerGangZone: MaxX, MaxY must be bigger than MinX, MinY. Not inversely!");
		logprintf("CreatePlayerGangZone: %f, %f, %f, %f",fMinX, fMinY, fMaxX, fMaxY);
		return -1;
	}

	WORD ret = pNetGame->pGangZonePool->New(playerid, fMinX, fMinY, fMaxX, fMaxY);
	if (ret == 0xFFFF) return -1;

	return ret;
}

// native PlayerGangZoneShow(playerid, zoneid, color);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneShow( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "PlayerGangZoneShow");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;

	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	DWORD dwColor = (DWORD)params[3];

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	pNetGame->pGangZonePool->ShowForPlayer((WORD)playerid, zoneid, dwColor, true);
	return 1;
}

// native PlayerGangZoneHide(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneHide( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "PlayerGangZoneHide");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];

	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	pNetGame->pGangZonePool->HideForPlayer((WORD)playerid, zoneid, true);
	return 1;
}

// native PlayerGangZoneFlash(playerid, zoneid, color);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneFlash( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "PlayerGangZoneFlash");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;

	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	DWORD dwColor = (DWORD)params[3];

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	pNetGame->pGangZonePool->FlashForPlayer((WORD)playerid, zoneid, dwColor, true);
	return 1;
}

// native PlayerGangZoneStopFlash(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneStopFlash( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "PlayerGangZoneStopFlash");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;

	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	pNetGame->pGangZonePool->StopFlashForPlayer((WORD)playerid, zoneid, true);
	return 1;
}

// native PlayerGangZoneDestroy(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneDestroy( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "PlayerGangZoneDestroy");

	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;

	pNetGame->pGangZonePool->HideForPlayer((WORD)playerid, (WORD)zoneid, true);
	return 1;
}

// native IsValidPlayerGangZone(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsValidPlayerGangZone( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsValidPlayerGangZone");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	return pPlayerData[playerid]->pPlayerZone[zoneid] != NULL;
}

// native IsPlayerInPlayerGangZone(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsPlayerInPlayerGangZone( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsPlayerInPlayerGangZone");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid, true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bInGangZone[id];
	}
	return 0;
}

// native PlayerGangZoneGetPos(playerid, zoneid, &Float:fMinX, &Float:fMinY, &Float:fMaxX, &Float:fMaxY);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneGetPos( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(6, "PlayerGangZoneGetPos");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	int zoneid = (int)params[2];
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;
	
	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid, true);
	if(id != 0xFFFF) 
	{
		cell* cptr;
		amx_GetAddr(amx, params[3], &cptr);
		*cptr = amx_ftoc(pPlayerData[playerid]->pPlayerZone[zoneid]->fGangZone[0]);
		amx_GetAddr(amx, params[4], &cptr);
		*cptr = amx_ftoc(pPlayerData[playerid]->pPlayerZone[zoneid]->fGangZone[1]);
		amx_GetAddr(amx, params[5], &cptr);
		*cptr = amx_ftoc(pPlayerData[playerid]->pPlayerZone[zoneid]->fGangZone[2]);
		amx_GetAddr(amx, params[6], &cptr);
		*cptr = amx_ftoc(pPlayerData[playerid]->pPlayerZone[zoneid]->fGangZone[3]);
	}
	return 1;
}

// native IsPlayerGangZoneVisible(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsPlayerGangZoneVisible( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsPlayerInPlayerGangZone");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	return pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid, true) != 0xFFFF;
}

// native PlayerGangZoneGetColor(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneGetColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerGangZoneGetColor");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid, true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneColor[id];
	}
	return 0;
}

// native PlayerGangZoneGetFlashColor(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::PlayerGangZoneGetFlashColor( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "PlayerGangZoneGetFlashColor");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid, true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->dwClientSideZoneFlashColor[id];
	}
	return 0;
}

// native IsPlayerGangZoneFlashing(playerid, zoneid);
static cell AMX_NATIVE_CALL Natives::IsPlayerGangZoneFlashing( AMX* amx, cell* params )
{
	CHECK_PARAMS(2, "IsPlayerGangZoneFlashing");
	
	int playerid = (int)params[1];
	int zoneid = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(zoneid < 0 || zoneid >= MAX_GANG_ZONES) return 0;
	
	if(!pPlayerData[playerid]->pPlayerZone[zoneid]) return 0;

	WORD id = pPlayerData[playerid]->GetGangZoneIDFromClientSide(zoneid, true);
	if(id != 0xFFFF) 
	{
		return pPlayerData[playerid]->bIsGangZoneFlashing[id];
	}
	return 0;
}

#ifdef NEW_PICKUP_SYSTEM

// native IsValidPickup(pickupid);
static cell AMX_NATIVE_CALL Natives::IsValidPickup( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(1, "IsValidPickup");

	int id = (int)params[1];
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	return pNetGame->pPickupPool->FindPickup(id) != 0;
}

// native IsPickupStreamedIn(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::IsPickupStreamedIn( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "IsPickupStreamedIn");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pNetGame->pPickupPool->IsStreamed(playerid, pPickup);
}

// native GetPickupPos(pickupid, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetPickupPos( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(4, "GetPickupPos");

	int id = (int)params[1];
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pPickup->vecPos.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pPickup->vecPos.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pPickup->vecPos.fZ);
	return 1;
}

// native GetPickupModel(pickupid);
static cell AMX_NATIVE_CALL Natives::GetPickupModel( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPickupModel");

	int id = (int)params[1];
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pPickup->iModel;
}

// native GetPickupType(pickupid);
static cell AMX_NATIVE_CALL Natives::GetPickupType( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPickupType");

	int id = (int)params[1];
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pPickup->iType;
}

// native GetPickupVirtualWorld(pickupid);
static cell AMX_NATIVE_CALL Natives::GetPickupVirtualWorld( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPickupVirtualWorld");

	int id = (int)params[1];
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(id);
	if(!pPickup) return 0;

	return pPickup->iWorld;
}

// CreatePlayerPickup(playerid, model, type, Float:X, Float:Y, Float:Z, virtualworld = 0);
static cell AMX_NATIVE_CALL Natives::CreatePlayerPickup( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(7, "CreatePlayerPickup");

	int playerid = (int)params[1];
	if(!IsPlayerConnected(playerid)) return 0;

	return pNetGame->pPickupPool->New(playerid, (int)params[2], (int)params[3], CVector(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6])), (int)params[7]);
}

// native DestroyPlayerPickup(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::DestroyPlayerPickup( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(2, "DestroyPlayerPickup");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	pNetGame->pPickupPool->Destroy((WORD)playerid, id);
	return 1;
}

// native IsValidPlayerPickup(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::IsValidPlayerPickup( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(2, "IsValidPlayerPickup");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;

	return pNetGame->pPickupPool->FindPickup(playerid, id) != 0;
}

// native IsPlayerPickupStreamedIn(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::IsPlayerPickupStreamedIn( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(2, "IsPlayerPickupStreamedIn");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pNetGame->pPickupPool->IsStreamed(playerid, pPickup);
}

// native GetPlayerPickupPos(playerid, pickupid, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetPlayerPickupPos( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(5, "GetPlayerPickupPos");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	cell* cptr;
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pPickup->vecPos.fX);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pPickup->vecPos.fY);
	amx_GetAddr(amx, params[5], &cptr);
	*cptr = amx_ftoc(pPickup->vecPos.fZ);
	return 1;
}

// native GetPlayerPickupModel(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::GetPlayerPickupModel( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(2, "GetPlayerPickupModel");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pPickup->iModel;
}

// native GetPlayerPickupType(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::GetPlayerPickupType( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(2, "GetPlayerPickupType");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pPickup->iType;
}

// native GetPlayerPickupVirtualWorld(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::GetPlayerPickupVirtualWorld( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	CHECK_PARAMS(2, "GetPlayerPickupVirtualWorld");

	int playerid = (int)params[1];
	int id = (int)params[2];
	if(!IsPlayerConnected(playerid)) return 0;	
	if(id < 0 || id >= MAX_PICKUPS) return 0;
	
	CPickup *pPickup = pNetGame->pPickupPool->FindPickup(playerid, id);
	if(!pPickup) return 0;

	return pPickup->iWorld;
}

#else

// native IsValidPickup(pickupid);
static cell AMX_NATIVE_CALL Natives::IsValidPickup(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "IsValidPickup");

	int id = (int)params[1];
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	return pNetGame->pPickupPool->m_bActive[id];
}

// native IsPickupStreamedIn(playerid, pickupid);
static cell AMX_NATIVE_CALL Natives::IsPickupStreamedIn(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(2, "IsPickupStreamedIn");

	int playerid = (int)params[1];
	int pickupid = (int)params[2];
	if (!IsPlayerConnected(playerid)) return 0;
	if (pickupid < 0 || pickupid >= MAX_PICKUPS) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid]->bPickupStreamedIn[pickupid];
}

// native GetPickupPos(pickupid, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetPickupPos(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(4, "GetPickupPos");

	int id = (int)params[1];
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->m_bActive[id]) return 0;

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(pNetGame->pPickupPool->m_Pickup[id].vecPos.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(pNetGame->pPickupPool->m_Pickup[id].vecPos.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(pNetGame->pPickupPool->m_Pickup[id].vecPos.fZ);
	return 1;
}

// native GetPickupModel(pickupid);
static cell AMX_NATIVE_CALL Natives::GetPickupModel(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPickupModel");

	int id = (int)params[1];
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->m_bActive[id]) return 0;

	return pNetGame->pPickupPool->m_Pickup[id].iModel;
}

// native GetPickupType(pickupid);
static cell AMX_NATIVE_CALL Natives::GetPickupType(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPickupType");

	int id = (int)params[1];
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->m_bActive[id]) return 0;

	return pNetGame->pPickupPool->m_Pickup[id].iType;
}

// native GetPickupVirtualWorld(pickupid);
static cell AMX_NATIVE_CALL Natives::GetPickupVirtualWorld(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "GetPickupVirtualWorld");

	int id = (int)params[1];
	if (id < 0 || id >= MAX_PICKUPS)
		return 0;

	if (!pNetGame->pPickupPool->m_bActive[id]) return 0;

	return pNetGame->pPickupPool->m_iWorld[id];
}

#endif
// RakServer functions //
// native ClearBanList();
static cell AMX_NATIVE_CALL Natives::ClearBanList( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	pRakServer->ClearBanList();
	return 1;
}

// native IsBanned(_ip[]);
static cell AMX_NATIVE_CALL Natives::IsBanned( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "IsBanned");

	char *ip;
	amx_StrParam(amx, params[1], ip);
	return (ip) ? pRakServer->IsBanned(ip) : 0;
}

// native SetTimeoutTime(playerid, time);
static cell AMX_NATIVE_CALL Natives::SetTimeoutTime( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(2, "SetTimeoutTime");
	
	PlayerID playerId = pRakServer->GetPlayerIDFromIndex((int)params[1]);
	if (playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress || !IsPlayerConnected((int)params[1]))
		return 0;

	pRakServer->SetTimeoutTime((RakNetTime)params[2], playerId);
	return 1;
}

// native SetMTUSize(size);
static cell AMX_NATIVE_CALL Natives::SetMTUSize( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(1, "SetMTUSize");
	
	return pRakServer->SetMTUSize((int)params[1]);
}

// native GetMTUSize();
static cell AMX_NATIVE_CALL Natives::GetMTUSize( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;
	
	return pRakServer->GetMTUSize();
}

// native GetLocalIP(index, localip[], len = sizeof(localip));
static cell AMX_NATIVE_CALL Natives::GetLocalIP( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "GetLocalIP");

	return set_amxstring(amx, params[2], pRakServer->GetLocalIP((unsigned int)params[1]), params[3]);
}

// native SendRPC(playerid, RPC, {Float,_}:...)
static cell AMX_NATIVE_CALL Natives::SendRPC( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	bool bBroadcast = !!(params[1] == -1);
	int rpcid = (int)params[2];
	
	PlayerID playerId = bBroadcast ? UNASSIGNED_PLAYER_ID : pRakServer->GetPlayerIDFromIndex((int)params[1]);
	
	if (playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress && !bBroadcast)
		return 0;
	
	RakNet::BitStream bs;
	cell *type = (cell*)0;
	cell *data = (cell*)0;

	for (int i = 0; i < (int)((params[0]/sizeof(cell)) - 2); i+=2)
	{
		amx_GetAddr(amx, params[i + 3], &type);
		amx_GetAddr(amx, params[i + 4], &data);
					
		if (type && data)
		{
			switch (*type)
			{
			case BS_BOOL:
				bs.Write((bool)(*data!=0));
				break;
			case BS_CHAR:
				bs.Write(*(char*)data);
				break;
			case BS_UNSIGNEDCHAR:
				bs.Write(*(unsigned char*)data);
				break;
			case BS_SHORT:
				bs.Write(*(short*)data);
				break;
			case BS_UNSIGNEDSHORT:
				bs.Write(*(unsigned short*)data);
				break;
			case BS_INT:
				bs.Write(*(int*)data);
				break;
			case BS_UNSIGNEDINT:
				bs.Write(*(unsigned int*)data);
				break;
			case BS_FLOAT:
				bs.Write(*(float*)data);
				break;
			case BS_STRING:
				{
					int len;
					amx_StrLen(data, &len);
					len++;
					char* str = new char[len];
					amx_GetString(str, data, 0, len);
					bs.Write(str, len - 1);
					//logprintf("str: %s", str);
					delete [] str;
				}
				break;
			}
		}
	}
	
	if(bBroadcast)
	{
		pRakServer->RPC(&rpcid, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, UNASSIGNED_PLAYER_ID, true, 0);
	}
	else
	{
		pRakServer->RPC(&rpcid, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, playerId, 0, 0);
	}
	return 1;
}

// native SendData(playerid, {Float,_}:...)
static cell AMX_NATIVE_CALL Natives::SendData( AMX* amx, cell* params )
{
	// If unknown server version
	if(!pServer)
		return 0;

	bool bBroadcast = !!(params[1] == -1);
	PlayerID playerId = bBroadcast ? UNASSIGNED_PLAYER_ID : pRakServer->GetPlayerIDFromIndex((int)params[1]);

	if (playerId.binaryAddress == UNASSIGNED_PLAYER_ID.binaryAddress && !bBroadcast)
		return 0;
	
	RakNet::BitStream bs;
	cell *type = (cell*)0;
	cell *data = (cell*)0;

	for (int i = 0; i < (int)((params[0]/sizeof(cell)) - 2); i+=2)
	{
		amx_GetAddr(amx, params[i + 2], &type);
		amx_GetAddr(amx, params[i + 3], &data);
					
		if (type && data)
		{
			switch (*type)
			{
			case BS_BOOL:
				bs.Write((bool)(*data!=0));
				break;
			case BS_CHAR:
				bs.Write(*(char*)data);
				break;
			case BS_UNSIGNEDCHAR:
				bs.Write(*(unsigned char*)data);
				break;
			case BS_SHORT:
				bs.Write(*(short*)data);
				break;
			case BS_UNSIGNEDSHORT:
				bs.Write(*(unsigned short*)data);
				break;
			case BS_INT:
				bs.Write(*(int*)data);
				break;
			case BS_UNSIGNEDINT:
				bs.Write(*(unsigned int*)data);
				break;
			case BS_FLOAT:
				bs.Write(*(float*)data);
				break;
			case BS_STRING:
				{
					int len;
					amx_StrLen(data, &len);
					len++;
					char* str = new char[len];
					amx_GetString(str, data, 0, len);
					bs.Write(str, len - 1);
					//logprintf("str: %s", str);
					delete [] str;
				}
				break;
			}
		}
	}

	if(bBroadcast)
	{
		pRakServer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 2, UNASSIGNED_PLAYER_ID, true);
	}
	else
	{
		pRakServer->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 2, playerId, 0);
	}
	return 1;
}

// native GetColCount();
static cell AMX_NATIVE_CALL Natives::GetColCount( AMX* amx, cell* params )
{
	return CModelSizes::GetColCount();
}

// native Float:GetColSphereRadius(modelid);
static cell AMX_NATIVE_CALL Natives::GetColSphereRadius( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "GetColSphereRadius");
	
	float fRet = CModelSizes::GetColSphereRadius((int)params[1]);
	return amx_ftoc(fRet);
}

// native GetColSphereOffset(modelid, &Float:fX, &Float:fY, &Float:fZ);
static cell AMX_NATIVE_CALL Natives::GetColSphereOffset( AMX* amx, cell* params )
{
	CHECK_PARAMS(4, "GetColSphereOffset");

	CVector vecOffset = CModelSizes::GetColSphereOffset((int)params[1]);

	cell* cptr;
	amx_GetAddr(amx, params[2], &cptr);
	*cptr = amx_ftoc(vecOffset.fX);
	amx_GetAddr(amx, params[3], &cptr);
	*cptr = amx_ftoc(vecOffset.fY);
	amx_GetAddr(amx, params[4], &cptr);
	*cptr = amx_ftoc(vecOffset.fZ);
	return 1;
}

// native GetWeaponSlot(weaponid);
static cell AMX_NATIVE_CALL Natives::GetWeaponSlot( AMX* amx, cell* params )
{
	CHECK_PARAMS(1, "GetWeaponSlot");
	
	return CUtils::GetWeaponSlot((BYTE)params[1]);
}

// native GetWeaponName(weaponid, weaponname[], len = sizeof(weaponname));
static cell AMX_NATIVE_CALL Natives::FIXED_GetWeaponName(AMX* amx, cell* params)
{
	// If unknown server version
	if(!pServer)
		return 0;

	CHECK_PARAMS(3, "GetWeaponName");

	return set_amxstring(amx, params[2], CUtils::GetWeaponName_((BYTE)params[1]), params[3]);
}

// native IsPlayerConnected(playerid);
static cell AMX_NATIVE_CALL Natives::FIXED_IsPlayerConnected(AMX* amx, cell* params)
{
	// If unknown server version
	if (!pServer)
		return 0;

	CHECK_PARAMS(1, "IsPlayerConnected");

	int playerid = (int)params[1];
	if (playerid < 0 || playerid >= MAX_PLAYERS) return 0;

	return pNetGame->pPlayerPool->pPlayer[playerid] != NULL;
}

#ifdef NEW_PICKUP_SYSTEM
// native CreatePickup(model, type, Float:X, Float:Y, Float:Z, virtualworld = 0);
static cell AMX_NATIVE_CALL Natives::CreatePickup(AMX *amx, cell *params)
{
	CHECK_PARAMS(6, "CreatePickup");

	return pNetGame->pPickupPool->New((int)params[1], (int)params[2], CVector(amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5])), (int)params[6]);
}

// native DestroyPickup(pickupid);
static cell AMX_NATIVE_CALL Natives::DestroyPickup(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "DestroyPickup");
	pNetGame->pPickupPool->Destroy((int)params[1]);
	return 1;
}

// native SetPickupStreamingEnabled(enabled);
static cell AMX_NATIVE_CALL Natives::SetPickupStreamingEnabled(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "SetPickupStreamingEnabled");

	pNetGame->pPickupPool->SetStreamingEnabled(!!params[1]);
	return 1;
}
#endif

#ifdef sdasdasd
// original funckci� �gy n�z ki:
// char* format_amxstring(AMX *amx, cell *params, int parm, int &len)

typedef char *(__thiscall *FUNC_format_amxstring_t)(AMX *amx, cell *params, int parm, int &len);

static cell AMX_NATIVE_CALL tesztgeci(AMX *amx, cell *params)
{
	/*
	FUNC_format_amxstring_t pfn_formatstr = (FUNC_format_amxstring_t)0x0046ED90;

	int len;
	char *ret = pfn_formatstr(amx, params, 1, len);
	std::printf("%s", ret);
	*/
	return 1;
}
#endif

// And an array containing the native function-names and the functions specified with them
AMX_NATIVE_INFO YSINatives [] =
{
	// File
	{"ffind",							Natives::ffind},
	{"frename",							Natives::frename},
	
	// Directory
	{"dfind",							Natives::dfind},
	{"dcreate",							Natives::dcreate},
	{"drename",							Natives::drename},

	// Generic
	{"SetModeRestartTime",				Natives::SetModeRestartTime},
	{"GetModeRestartTime",				Natives::GetModeRestartTime},
	{"SetMaxPlayers",					Natives::SetMaxPlayers}, // R8
	{"SetMaxNPCs",						Natives::SetMaxNPCs}, // R8

	{"SetPlayerAdmin",					Natives::SetPlayerAdmin},
	{"LoadFilterScript",				Natives::LoadFilterScript},
	{"UnLoadFilterScript",				Natives::UnLoadFilterScript},
	{"GetFilterScriptCount",			Natives::GetFilterScriptCount},
	{"GetFilterScriptName",				Natives::GetFilterScriptName},

	{"AddServerRule",					Natives::AddServerRule},
	{"SetServerRule",					Natives::SetServerRule},
	{"SetServerRuleInt",				Natives::SetServerRuleInt},
	{"RemoveServerRule",				Natives::RemoveServerRule}, // Doesn't work!
	{"ModifyFlag",						Natives::ModifyFlag},

	// Server settings
	{"GetServerSettings",				Natives::GetServerSettings},

	// Nick name
	{"IsValidNickName",					Natives::IsValidNickName},	// R8
	{"AllowNickNameCharacter",			Natives::AllowNickNameCharacter}, // R7
	{"IsNickNameCharacterAllowed",		Natives::IsNickNameCharacterAllowed}, // R7

	// Player classes
	{ "GetAvailableClasses",			Natives::GetAvailableClasses}, // R6
	{ "GetPlayerClass",					Natives::GetPlayerClass}, // R6
	{ "EditPlayerClass",				Natives::EditPlayerClass}, // R6
	
	// Timers
	{ "GetActiveTimers",				Natives::GetActiveTimers}, // R8

	// RakSAMP crash
	{ "SendInvalidPlayerSync",			Natives::SendInvalidPlayerSync }, // R10

	// Special
	{ "SetPlayerGravity",				Natives::SetPlayerGravity },
	{ "GetPlayerGravity",				Natives::GetPlayerGravity },
	{ "SetPlayerTeamForPlayer",			Natives::SetPlayerTeamForPlayer }, // R5 - Experimental - needs testing - tested, should be OK
	{ "GetPlayerTeamForPlayer",			Natives::GetPlayerTeamForPlayer }, // R5
	{ "GetPlayerWeather",				Natives::GetPlayerWeather },
	{ "GetPlayerWorldBounds",			Natives::GetPlayerWorldBounds },
	{ "TogglePlayerWidescreen",			Natives::TogglePlayerWidescreen },
	{ "IsPlayerWidescreenToggled",		Natives::IsPlayerWidescreenToggled },
	{ "GetSpawnInfo",					Natives::GetSpawnInfo }, // R8
	{ "GetPlayerSkillLevel",			Natives::GetPlayerSkillLevel }, // R3
	{ "IsPlayerCheckpointActive",		Natives::IsPlayerCheckpointActive }, // R10
	{ "GetPlayerCheckpoint",			Natives::GetPlayerCheckpoint }, // R4
	{ "IsPlayerRaceCheckpointActive",	Natives::IsPlayerRaceCheckpointActive }, // R10
	{ "GetPlayerRaceCheckpoint",		Natives::GetPlayerRaceCheckpoint }, // R4
	{ "GetPlayerWorldBounds",			Natives::GetPlayerWorldBounds }, // R5
	{ "IsPlayerInModShop",				Natives::IsPlayerInModShop }, // R4
	{ "SendBulletData",					Natives::SendBulletData }, // R6
	{ "ShowPlayerForPlayer",			Natives::ShowPlayerForPlayer }, // R8
	{ "HidePlayerForPlayer",			Natives::HidePlayerForPlayer }, // R8
	{ "SetPlayerChatBubbleForPlayer",	Natives::SetPlayerChatBubbleForPlayer}, // R10
	{ "SetPlayerVersion",				Natives::SetPlayerVersion }, // R9
	{ "IsPlayerSpawned",				Natives::IsPlayerSpawned }, // R9
	{ "SpawnForWorld",					Natives::SpawnForWorld }, // R10

	// Special things from syncdata
	{ "GetPlayerSirenState",			Natives::GetPlayerSirenState },
	{ "GetPlayerLandingGearState",		Natives::GetPlayerLandingGearState },
	{ "GetPlayerHydraReactorAngle",		Natives::GetPlayerHydraReactorAngle },
	{ "GetPlayerTrainSpeed",			Natives::GetPlayerTrainSpeed },
	{ "GetPlayerZAim",					Natives::GetPlayerZAim },
	{ "GetPlayerSurfingOffsets",		Natives::GetPlayerSurfingOffsets },
	{ "GetPlayerRotationQuat",			Natives::GetPlayerRotationQuat }, // R3
	{ "GetPlayerDialogID",				Natives::GetPlayerDialogID }, // R8
	{ "GetPlayerSpectateID",			Natives::GetPlayerSpectateID }, // R8
	{ "GetPlayerSpectateType",			Natives::GetPlayerSpectateType }, // R8
	{ "GetPlayerLastSyncedVehicleID",	Natives::GetPlayerLastSyncedVehicleID }, // R10
	{ "GetPlayerLastSyncedTrailerID",	Natives::GetPlayerLastSyncedTrailerID }, // R10
	{ "GetPlayerFPS", 					Natives::GetPlayerFPS},

	// Scoreboard manipulation
	{ "TogglePlayerScoresPingsUpdate",	Natives::TogglePlayerScoresPingsUpdate }, // R8
	{ "TogglePlayerFakePing",			Natives::TogglePlayerFakePing }, // R8
	{ "SetPlayerFakePing",				Natives::SetPlayerFakePing }, // R8
	{ "TogglePlayerOnPlayerList",		Natives::TogglePlayerOnPlayerList }, // R11
	{ "IsPlayerToggledOnPlayerList",	Natives::IsPlayerToggledOnPlayerList }, // R11

	// AFK
	{ "IsPlayerPaused",					Natives::IsPlayerPaused },
	{ "GetPlayerPausedTime",			Natives::GetPlayerPausedTime },
	
	// Objects get - global
	{"GetObjectModel",					Natives::GetObjectModel},
	{"GetObjectDrawDistance",			Natives::GetObjectDrawDistance},
	{"SetObjectMoveSpeed",				Natives::SetObjectMoveSpeed}, // R6
	{"GetObjectMoveSpeed",				Natives::GetObjectMoveSpeed}, // R6
	{"GetObjectTarget",					Natives::GetObjectTarget}, // R6
	{"GetObjectAttachedData",			Natives::GetObjectAttachedData},
	{"GetObjectAttachedOffset",			Natives::GetObjectAttachedOffset},
	{"IsObjectMaterialSlotUsed",		Natives::IsObjectMaterialSlotUsed}, // R6
	{"GetObjectMaterial",				Natives::GetObjectMaterial}, // R6
	{"GetObjectMaterialText",			Natives::GetObjectMaterialText}, // R6

	// Objects get - player
	{"GetPlayerObjectModel",			Natives::GetPlayerObjectModel},
	{"GetPlayerObjectDrawDistance",		Natives::GetPlayerObjectDrawDistance},
	{"SetPlayerObjectMoveSpeed",		Natives::SetPlayerObjectMoveSpeed}, // R6
	{"GetPlayerObjectMoveSpeed",		Natives::GetPlayerObjectMoveSpeed}, // R6
	{"GetPlayerObjectTarget",			Natives::GetPlayerObjectTarget}, // R6
	{"GetPlayerObjectAttachedData",		Natives::GetPlayerObjectAttachedData},
	{"GetPlayerObjectAttachedOffset",	Natives::GetPlayerObjectAttachedOffset},
	{"IsPlayerObjectMaterialSlotUsed",	Natives::IsPlayerObjectMaterialSlotUsed}, // R6
	{"GetPlayerObjectMaterial",			Natives::GetPlayerObjectMaterial}, // R6
	{"GetPlayerObjectMaterialText",		Natives::GetPlayerObjectMaterialText}, // R6

	// special - for attached objects
	{"GetPlayerAttachedObject",			Natives::GetPlayerAttachedObject}, // R3
	{"IsPlayerEditingObject",			Natives::IsPlayerEditingObject}, // R9 - do not reset after player quit from editing
	{"IsPlayerEditingAttachedObject",	Natives::IsPlayerEditingAttachedObject}, // R9 - do not reset after player quit from editing
	
	// Vehicle functions
	{"GetVehicleSpawnInfo",				Natives::GetVehicleSpawnInfo},
	{"GetVehicleColor",					Natives::GetVehicleColor},
	{"GetVehiclePaintjob",				Natives::GetVehiclePaintjob},
	{"GetVehicleInterior",				Natives::GetVehicleInterior},
	{"GetVehicleNumberPlate",			Natives::GetVehicleNumberPlate},
	{"SetVehicleRespawnDelay",			Natives::SetVehicleRespawnDelay},
	{"GetVehicleRespawnDelay",			Natives::GetVehicleRespawnDelay},
	{"SetVehicleOccupiedTick",			Natives::SetVehicleOccupiedTick}, // R6
	{"GetVehicleOccupiedTick",			Natives::GetVehicleOccupiedTick},
	{"SetVehicleRespawnTick",			Natives::SetVehicleRespawnTick},
	{"GetVehicleRespawnTick",			Natives::GetVehicleRespawnTick},
	{"GetVehicleLastDriver",			Natives::GetVehicleLastDriver},
	{"GetVehicleCab",					Natives::GetVehicleCab}, // R9
	{"HasVehicleBeenOccupied",			Natives::HasVehicleBeenOccupied}, // R9
	{"SetVehicleBeenOccupied",			Natives::SetVehicleBeenOccupied}, // R9
	{"IsVehicleOccupied",				Natives::IsVehicleOccupied}, // R9
	{"IsVehicleDead",					Natives::IsVehicleDead}, // R9

	// Gangzone - Global
	{"IsValidGangZone",					Natives::IsValidGangZone},
	{"IsPlayerInGangZone",				Natives::IsPlayerInGangZone},
	{"IsGangZoneVisibleForPlayer",		Natives::IsGangZoneVisibleForPlayer},
	{"GangZoneGetColorForPlayer",		Natives::GangZoneGetColorForPlayer},
	{"GangZoneGetFlashColorForPlayer",	Natives::GangZoneGetFlashColorForPlayer},
	{"IsGangZoneFlashingForPlayer",		Natives::IsGangZoneFlashingForPlayer}, // R6
	{"GangZoneGetPos",					Natives::GangZoneGetPos},

	// Gangzone - Player
	{ "CreatePlayerGangZone",			Natives::CreatePlayerGangZone },
	{ "PlayerGangZoneDestroy",			Natives::PlayerGangZoneDestroy },
	{ "PlayerGangZoneShow",				Natives::PlayerGangZoneShow },
	{ "PlayerGangZoneHide",				Natives::PlayerGangZoneHide },
	{ "PlayerGangZoneFlash",			Natives::PlayerGangZoneFlash},
	{ "PlayerGangZoneStopFlash",		Natives::PlayerGangZoneStopFlash },
	{ "IsValidPlayerGangZone",			Natives::IsValidPlayerGangZone },
	{ "IsPlayerInPlayerGangZone",		Natives::IsPlayerInPlayerGangZone },
	{ "IsPlayerGangZoneVisible",		Natives::IsPlayerGangZoneVisible },
	{ "PlayerGangZoneGetColor",			Natives::PlayerGangZoneGetColor },
	{ "PlayerGangZoneGetFlashColor",	Natives::PlayerGangZoneGetFlashColor },
	{ "IsPlayerGangZoneFlashing",		Natives::IsPlayerGangZoneFlashing }, // R6
	{ "PlayerGangZoneGetPos",			Natives::PlayerGangZoneGetPos },

	// Textdraw functions
	{"IsValidTextDraw",					Natives::IsValidTextDraw},
	{"IsTextDrawVisibleForPlayer",		Natives::IsTextDrawVisibleForPlayer},
	{"TextDrawGetString",				Natives::TextDrawGetString},
	{"TextDrawSetPos",					Natives::TextDrawSetPos},
	{"TextDrawGetLetterSize",			Natives::TextDrawGetLetterSize},
	{"TextDrawGetFontSize",				Natives::TextDrawGetFontSize},
	{"TextDrawGetPos",					Natives::TextDrawGetPos},
	{"TextDrawGetColor",				Natives::TextDrawGetColor},
	{"TextDrawGetBoxColor",				Natives::TextDrawGetBoxColor},
	{"TextDrawGetBackgroundColor",		Natives::TextDrawGetBackgroundColor},
	{"TextDrawGetShadow",				Natives::TextDrawGetShadow},
	{"TextDrawGetOutline",				Natives::TextDrawGetOutline},
	{"TextDrawGetFont",					Natives::TextDrawGetFont},
	{"TextDrawIsBox",					Natives::TextDrawIsBox},
	{"TextDrawIsProportional",			Natives::TextDrawIsProportional},
	{"TextDrawIsSelectable",			Natives::TextDrawIsSelectable}, // R6
	{"TextDrawGetAlignment",			Natives::TextDrawGetAlignment},
	{"TextDrawGetPreviewModel",			Natives::TextDrawGetPreviewModel},
	{"TextDrawGetPreviewRot",			Natives::TextDrawGetPreviewRot},
	{"TextDrawGetPreviewVehCol",		Natives::TextDrawGetPreviewVehCol},

	// Per-Player Textdraw functions - R4
	{"IsValidPlayerTextDraw",			Natives::IsValidPlayerTextDraw},
	{"IsPlayerTextDrawVisible",			Natives::IsPlayerTextDrawVisible},
	{"PlayerTextDrawGetString",			Natives::PlayerTextDrawGetString},
	{"PlayerTextDrawSetPos",			Natives::PlayerTextDrawSetPos},
	{"PlayerTextDrawGetLetterSize",		Natives::PlayerTextDrawGetLetterSize},
	{"PlayerTextDrawGetFontSize",		Natives::PlayerTextDrawGetFontSize},
	{"PlayerTextDrawGetPos",			Natives::PlayerTextDrawGetPos},
	{"PlayerTextDrawGetColor",			Natives::PlayerTextDrawGetColor},
	{"PlayerTextDrawGetBoxColor",		Natives::PlayerTextDrawGetBoxColor},
	{"PlayerTextDrawGetBackgroundCol",	Natives::PlayerTextDrawGetBackgroundCol},
	{"PlayerTextDrawGetShadow",			Natives::PlayerTextDrawGetShadow},
	{"PlayerTextDrawGetOutline",		Natives::PlayerTextDrawGetOutline},
	{"PlayerTextDrawGetFont",			Natives::PlayerTextDrawGetFont},
	{"PlayerTextDrawIsBox",				Natives::PlayerTextDrawIsBox},
	{"PlayerTextDrawIsProportional",	Natives::PlayerTextDrawIsProportional},
	{"PlayerTextDrawIsSelectable",		Natives::PlayerTextDrawIsSelectable}, // R6
	{"PlayerTextDrawGetAlignment",		Natives::PlayerTextDrawGetAlignment},
	{"PlayerTextDrawGetPreviewModel",	Natives::PlayerTextDrawGetPreviewModel},
	{"PlayerTextDrawGetPreviewRot",		Natives::PlayerTextDrawGetPreviewRot},
	{"PlayerTextDrawGetPreviewVehCol",	Natives::PlayerTextDrawGetPreviewVehCol},

	// 3D Text
	{"IsValid3DTextLabel",				Natives::IsValid3DTextLabel}, // R4
	{"Is3DTextLabelStreamedIn",			Natives::Is3DTextLabelStreamedIn}, // R9
	{"Get3DTextLabelText",				Natives::Get3DTextLabelText},
	{"Get3DTextLabelColor",				Natives::Get3DTextLabelColor},
	{"Get3DTextLabelPos",				Natives::Get3DTextLabelPos},
	{"Get3DTextLabelDrawDistance",		Natives::Get3DTextLabelDrawDistance},
	{"Get3DTextLabelLOS",				Natives::Get3DTextLabelLOS},
	{"Get3DTextLabelVirtualWorld",		Natives::Get3DTextLabelVirtualWorld},
	{"Get3DTextLabelAttachedData",		Natives::Get3DTextLabelAttachedData},

	// Per-Player 3D Text
	{"IsValidPlayer3DTextLabel",		Natives::IsValidPlayer3DTextLabel}, // R4
	{"GetPlayer3DTextLabelText",		Natives::GetPlayer3DTextLabelText}, // R4
	{"GetPlayer3DTextLabelColor",		Natives::GetPlayer3DTextLabelColor}, // R4
	{"GetPlayer3DTextLabelPos",			Natives::GetPlayer3DTextLabelPos}, // R4
	{"GetPlayer3DTextLabelDrawDist",	Natives::GetPlayer3DTextLabelDrawDist},
	{"GetPlayer3DTextLabelLOS",			Natives::GetPlayer3DTextLabelLOS}, // R4
	{"GetPlayer3DTextLabelVirtualW",	Natives::GetPlayer3DTextLabelVirtualW}, // R4
	{"GetPlayer3DTextLabelAttached",	Natives::GetPlayer3DTextLabelAttached}, // R9

	// Menus
	{"IsMenuDisabled",					Natives::IsMenuDisabled}, // R5 
	{"IsMenuRowDisabled",				Natives::IsMenuRowDisabled}, // R5
	{"GetMenuColumns",					Natives::GetMenuColumns},
	{"GetMenuItems",					Natives::GetMenuItems},
	{"GetMenuPos",						Natives::GetMenuPos},
	{"GetMenuColumnWidth",				Natives::GetMenuColumnWidth},
	{"GetMenuColumnHeader",				Natives::GetMenuColumnHeader},
	{"GetMenuItem",						Natives::GetMenuItem},
	
	// Pickups - Global
	{ "IsValidPickup",					Natives::IsValidPickup }, // R10
	{ "IsPickupStreamedIn",				Natives::IsPickupStreamedIn }, // R10
	{ "GetPickupPos",					Natives::GetPickupPos }, // R10
	{ "GetPickupModel",					Natives::GetPickupModel }, // R10
	{ "GetPickupType",					Natives::GetPickupType }, // R10
	{ "GetPickupVirtualWorld",			Natives::GetPickupVirtualWorld }, // R10
#ifdef NEW_PICKUP_SYSTEM
	// Pickups - Per-player
	{ "CreatePlayerPickup",				Natives::CreatePlayerPickup}, // R10
	{ "DestroyPlayerPickup",			Natives::DestroyPlayerPickup }, // R10
	{ "IsValidPlayerPickup",			Natives::IsValidPlayerPickup }, // R10
	{ "IsPlayerPickupStreamedIn",		Natives::IsPlayerPickupStreamedIn }, // R10
	{ "GetPlayerPickupPos",				Natives::GetPlayerPickupPos }, // R10
	{ "GetPlayerPickupModel",			Natives::GetPlayerPickupModel }, // R10
	{ "GetPlayerPickupType",			Natives::GetPlayerPickupType }, // R10
	{ "GetPlayerPickupVirtualWorld",	Natives::GetPlayerPickupVirtualWorld }, // R10
#endif
	// RakServer functions
	{ "ClearBanList",					Natives::ClearBanList },
	{ "IsBanned",						Natives::IsBanned },

	{ "SetTimeoutTime",					Natives::SetTimeoutTime },
	//{ "SetMTUSize",					Natives::SetMTUSize },
	{ "GetMTUSize",						Natives::GetMTUSize },
	{ "GetLocalIP",						Natives::GetLocalIP },
	
	{ "SendRPC",						Natives::SendRPC },
	{ "SendData",						Natives::SendData },

	{ "YSF_AddPlayer",					Natives::YSF_AddPlayer },
	{ "YSF_RemovePlayer",				Natives::YSF_RemovePlayer },
	{ "YSF_StreamIn",					Natives::YSF_StreamIn },
	{ "YSF_StreamOut",					Natives::YSF_StreamOut },

	{ "AttachPlayerObjectToObject",		Natives::AttachPlayerObjectToObject },

	// Other
	{"GetColCount",						Natives::GetColCount},
	{"GetColSphereRadius",				Natives::GetColSphereRadius},
	{"GetColSphereOffset",				Natives::GetColSphereOffset},

	{ "GetWeaponSlot",					Natives::GetWeaponSlot},
#ifdef NEW_PICKUP_SYSTEM
	{ "SetPickupStreamingEnabled",		Natives::SetPickupStreamingEnabled },
#endif
	{ 0,								0 }
};

AMX_NATIVE_INFO RedirecedtNatives[] =
{
	// File
	{ "AttachPlayerObjectToPlayer",		Natives::FIXED_AttachPlayerObjectToPlayer },
	{ "SetGravity",						Natives::FIXED_SetGravity },
	{ "GetGravity",						Natives::FIXED_GetGravity },
	{ "SetWeather",						Natives::FIXED_SetWeather },
	{ "SetPlayerWeather",				Natives::FIXED_SetPlayerWeather },
	{ "SetPlayerWorldBounds",			Natives::FIXED_SetPlayerWorldBounds },
	
	{ "GangZoneCreate",					Natives::YSF_GangZoneCreate },
	{ "GangZoneDestroy",				Natives::YSF_GangZoneDestroy },
	{ "GangZoneShowForPlayer",			Natives::YSF_GangZoneShowForPlayer },
	{ "GangZoneHideForPlayer",			Natives::YSF_GangZoneHideForPlayer },
	{ "GangZoneShowForAll",				Natives::YSF_GangZoneShowForAll },
	{ "GangZoneHideForAll",				Natives::YSF_GangZoneHideForAll },
								
	{ "GangZoneFlashForPlayer",			Natives::YSF_GangZoneFlashForPlayer },
	{ "GangZoneFlashForAll",			Natives::YSF_GangZoneFlashForAll },
	{ "GangZoneStopFlashForPlayer",		Natives::YSF_GangZoneStopFlashForPlayer },
	{ "GangZoneStopFlashForAll",		Natives::YSF_GangZoneStopFlashForAll },
#ifdef NEW_PICKUP_SYSTEM
	{ "CreatePickup",					Natives::CreatePickup },
	{ "AddStaticPickup",				Natives::CreatePickup },
	{ "DestroyPickup",					Natives::DestroyPickup },
#endif
	{ "GetWeaponName",					Natives::FIXED_GetWeaponName },
	{ "IsPlayerConnected",				Natives::FIXED_IsPlayerConnected },
	{ 0,								0 }
};

int InitScripting(AMX *amx)
{
	return amx_Register(amx, YSINatives, -1);
}