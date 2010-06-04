/*
	Teamspeak 3 Winamp Now Playing Plugin 
    Copyright (C) 2009 antihack3r

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "nowplaying_plugin.h"
#include "winamp.h"
#include "vlc.h"
#include "lightalloy.h"


#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s


CRITICAL_SECTION cs;

static struct TS3Functions ts3Functions;
int iRun = 0;
HANDLE hThread;
static char* pluginCommandID = 0;
static struct TrackInfo lastInfo;
char chChannelMsg[300];
char chApplications[300];
char chBoundToUniqueID[300];
int iEnableApplications;
int iEnableAutoChannelMsg;


const char* ts3plugin_name() {
    return "Now Playing Plugin";
}

const char* ts3plugin_version() {
    return "0.11";
}

int ts3plugin_apiVersion() {
	return 5;
}

const char* ts3plugin_author() {
    return "antihack3r";
}

const char* ts3plugin_description() {
	return "Displays your current playing track.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
	char chConfigPath[450];
	char chFilePath[500];
	FILE *fFile;

	InitializeCriticalSection (&cs);
	
	ts3Functions.getConfigPath (chConfigPath, sizeof (chConfigPath));

	snprintf (chFilePath, 500, "%s%s", chConfigPath, "now_playing_plugin.ini");
	
	printf ("File Path: %s\n", chFilePath);

	if (fopen_s (&fFile, chFilePath, "r") != 0) {
		WritePrivateProfileString ("config", "channel_message", "I'm listening to [b]{title}[/b].", chFilePath);
		WritePrivateProfileString ("config", "applications", "{title}", chFilePath);
		WritePrivateProfileString ("config", "enable_applications", "1", chFilePath);
		WritePrivateProfileString ("config", "auto_channel_message", "0", chFilePath);
		WritePrivateProfileString ("config", "bound_to_unique_id", "", chFilePath);
		if (fFile != NULL) fclose (fFile);
	}

	GetPrivateProfileString ("config", "channel_message", "I'm listening to [b]{title}[/b].", 
		chChannelMsg, 300, chFilePath);
	GetPrivateProfileString ("config", "applications", "{title}", 
		chApplications, 300, chFilePath);

	GetPrivateProfileString ("config", "bound_to_unique_id", "", 
		chBoundToUniqueID, 300, chFilePath);

	printf ("chApplications: %s\n", chApplications);
	printf ("chBoundToUniqueID: %s\n", chBoundToUniqueID);

	if (GetPrivateProfileInt ("config", "enable_applications", 1, chFilePath) >= 1) {
		iEnableApplications = 1;
	}
	else {
		iEnableApplications = 0;
	}

	if (GetPrivateProfileInt ("config", "auto_channel_message", 0, chFilePath) >= 1) {
		iEnableAutoChannelMsg = 1;
	}
	else {
		iEnableAutoChannelMsg = 0;
	}
		

	iRun = 1;
	hThread = CreateThread (NULL, 0, MainLoop, NULL, 0, NULL);  	
	if (hThread == 0) {
		printf ("Now Playing: Error could not create Thread %d\n", GetLastError ());
		return 1;
	}

    return 0;  /* 0 = success, 1 = failure */
}

void ts3plugin_shutdown() {	
	if (hThread != 0) {
		iRun = 0;
		WaitForSingleObject (hThread, 500);
		CloseHandle (hThread);
		DeleteCriticalSection (&cs);
	}

	if(pluginCommandID) {
		free(pluginCommandID);
		pluginCommandID = 0;
	}
}

void ts3plugin_registerPluginCommandID(const char* commandID) {
	const size_t sz = strlen(commandID) + 1;
	pluginCommandID = (char*)malloc(sz);
	memset(pluginCommandID, 0, sz);
	_strcpy(pluginCommandID, sz, commandID);
}


const char* ts3plugin_commandKeyword() {
	return "nowplaying";
}

int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	anyID ownID;
	uint64 channelID;
	char chMessage[2048];

	char *chUniqueID;

	EnterCriticalSection (&cs);

	if (lastInfo.chProgramm == 0 || lastInfo.chTitle == 0) {
		return 0;
	}

	/* XXX Error checking */

	ts3Functions.getClientID (serverConnectionHandlerID, &ownID);
	ts3Functions.getChannelOfClient (serverConnectionHandlerID, ownID, &channelID);
	
	if (FormatTitle (chMessage, 2048, chChannelMsg, lastInfo) == 1) {
		ts3Functions.requestSendChannelTextMsg (serverConnectionHandlerID, chMessage, channelID, 0);
	}

	ts3Functions.getClientSelfVariableAsString (serverConnectionHandlerID, CLIENT_UNIQUE_IDENTIFIER, &chUniqueID);

	ts3Functions.printMessageToCurrentTab (chUniqueID);

	ts3Functions.freeMemory (chUniqueID);

	LeaveCriticalSection (&cs);

	return 0; /* handled */
}

DWORD WINAPI MainLoop (LPVOID lpParam) {
	uint64 *result;
	anyID resultClientID;
	uint64 resultChannelID;
	struct TrackInfo info;
	int i = 0;

	char chFinalString[2048];
	char chMessage[2048];
	int iFound = 0;
	int iFoundNotingLastTime = 0;
	int iSize = 0;

	char *chUniqueID;

	memset (&info, 0, sizeof (struct TrackInfo));
	memset (&lastInfo, 0, sizeof (struct TrackInfo));

	while (iRun) {
		EnterCriticalSection (&cs);
		
		iFound = 0;

		if (winamp (&info, ts3Functions)) {
			iFound = 1;
		}
		else if (vlc (&info, ts3Functions)) {
			iFound = 1;
		}
		else if (lightalloy (&info, ts3Functions)) {
			iFound = 1;
		}
		
		if (iFound) {
			if (memcmp (&info, &lastInfo, sizeof (struct TrackInfo)) != 0) {
				iFoundNotingLastTime = 0;

				memcpy (&lastInfo, &info, sizeof (struct TrackInfo));


				printf ("Now Playing: [%s]\n", info.chTitle);
	
				i = 0;		
				ts3Functions.getServerConnectionHandlerList (&result); 

				while (result[i]) {

					if (strlen (chBoundToUniqueID) == 28) {
						ts3Functions.getClientSelfVariableAsString (result[i], CLIENT_UNIQUE_IDENTIFIER, &chUniqueID);
						if (strcmp (chUniqueID, chBoundToUniqueID) != 0) { /* Unique ID does not match */
							ts3Functions.freeMemory (chUniqueID);
							i++;
							continue; 
						}
						ts3Functions.freeMemory (chUniqueID);
					}

					if (iEnableApplications == 1) {
						if (FormatTitle (chFinalString, 2048, chApplications, info) == 1) {
							ts3Functions.setClientSelfVariableAsString (result[i],
									CLIENT_META_DATA,
									chFinalString);						
							ts3Functions.flushClientSelfUpdates (result[i]);
						}
					}

					if (iEnableAutoChannelMsg == 1) {
						ts3Functions.getClientID (result[i], &resultClientID);
						ts3Functions.getChannelOfClient (result[i], resultClientID, &resultChannelID);
							
						if (FormatTitle (chMessage, 2048, chChannelMsg, info) == 1) {
							ts3Functions.requestSendChannelTextMsg (result[i], chMessage, resultChannelID, 0);
						}
					}

					i++;
				}
			}
		}
		else if (iFoundNotingLastTime == 0) {
			/* we don't need to reset applications to nothing every half second */
			iFoundNotingLastTime = 1;
			
			i = 0;
			
			memset (&lastInfo, 0, sizeof (struct TrackInfo));
			
			if (iEnableApplications == 1) {
				ts3Functions.getServerConnectionHandlerList (&result);
				while (result[i]) {
					ts3Functions.setClientSelfVariableAsString (result[i],
							CLIENT_META_DATA,
							""); /* clear */
					ts3Functions.flushClientSelfUpdates (result[i]);
					i++;
				}
			}
		}

		LeaveCriticalSection (&cs);
		Sleep (500);
	}
	return 0;
}


int FormatTitle (char chDestination[], unsigned int size, char *chFormat, struct TrackInfo info) {
	char *chOccurrence = NULL;
	unsigned int iSize = 0;

	chOccurrence = strstr (chFormat, "{title}");				
	if (chOccurrence != NULL && size > strlen (chFormat)) {
		iSize = (chOccurrence - chFormat);
	
		if (iSize != 0) {
			memcpy (chDestination, chFormat, iSize);
		}
		chDestination[iSize] = '\0';

		strcat_s (chDestination, size, info.chTitle);
		strcat_s (chDestination, size, chOccurrence + 7);

		return 1;
	}
	else {
		return 0;
	}
}

