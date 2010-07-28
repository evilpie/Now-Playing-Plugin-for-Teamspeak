#include "nowplaying_plugin.h"

CRITICAL_SECTION cs;
TS3Functions ts3Functions;


HANDLE mainThread;
char *pluginCommandID;
NOW_PLAYING_CONTEXT context;

const char* ts3plugin_name() {
    return "Now Playing Plugin";
}

const char* ts3plugin_version() {
    return "0.2";
}

int ts3plugin_apiVersion() {
	return 5;
}

const char* ts3plugin_author() {
    return "antihack3r/evilpie";
}

const char* ts3plugin_description() {
	return "Displays your current playing track.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
	char configPath[MAX_PATH];
	char filePath[MAX_PATH];
	FILE *file;

	InitializeCriticalSection (&cs);

	ts3Functions.getConfigPath (configPath, MAX_PATH);
	sprintf_s (filePath, MAX_PATH, "%s%s", configPath, "now_playing_plugin.ini");

	if (fopen_s (&file, filePath, "r") != 0) {
		WritePrivateProfileString ("config", "channel_message", "I'm listening to [b]{title}[/b].", filePath);
		WritePrivateProfileString ("config", "applications", "{title}", filePath);
		WritePrivateProfileString ("config", "enable_applications", "1", filePath);
		WritePrivateProfileString ("config", "auto_channel_message", "0", filePath);
		WritePrivateProfileString ("config", "bound_to_unique_id", "", filePath);

		if (file) {
			fclose (file);
		}
	}

	context.channelMessage = malloc(200);
	context.appWindowText = malloc(200);
	context.boundToServer = malloc(200);

	if (!context.channelMessage || !context.appWindowText || !context.boundToServer) {
		free (context.channelMessage); /* one malloc might have worked */
		free (context.appWindowText);
		free (context.boundToServer);

		return 1;
	}

	GetPrivateProfileString ("config", "channel_message", "I'm listening to [b]{title}[/b].", 
		context.channelMessage, 200, filePath);
	GetPrivateProfileString ("config", "applications", "{title}", 
		context.appWindowText,  200, filePath);
	GetPrivateProfileString ("config", "bound_to_unique_id", "", 
		context.boundToServer,  200, filePath);


	context.showInAppsWindow = (GetPrivateProfileInt ("config", "enable_applications", 1, chFilePath) >= 1);
	context.sendMessageToChannel = (GetPrivateProfileInt ("config", "auto_channel_message", 0, chFilePath) >= 1);


	mainThread = CreateThread (NULL, 0, MainLoop, NULL, 0, NULL);  	
	if (!mainThread) {
		printf ("Now Playing: Error could not create Main-Thread %d\n", GetLastError ());
		return 1;
	}

    return 0;
}

void ts3plugin_shutdown() {	
	if (mainThread != 0) {
		iRun = 0;
		WaitForSingleObject (mainThread, 500);
		CloseHandle (mainThread);
		DeleteCriticalSection (&cs);
	}

	if (pluginCommandID) {
		free(pluginCommandID);
		pluginCommandID = 0;
	}
}

void ts3plugin_registerPluginCommandID(const char* commandID) {
	const size_t size = strlen(commandID) + 1;
	pluginCommandID = (char*)malloc(size);
	memset(pluginCommandID, 0, size);
	strcpy_s(pluginCommandID, size, commandID);
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

