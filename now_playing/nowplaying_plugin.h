/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008, 2009 TeamSpeak Systems GmbH
 */
#ifndef PLUGIN_H
#define PLUGIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

#include "public_errors.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin_events.h"

#include "shared.h"


#define PLUGINS_EXPORTDLL __declspec(dllexport)

/* Required functions */
PLUGINS_EXPORTDLL const char* ts3plugin_name();
PLUGINS_EXPORTDLL const char* ts3plugin_version();
PLUGINS_EXPORTDLL int ts3plugin_apiVersion();
PLUGINS_EXPORTDLL const char* ts3plugin_author();
PLUGINS_EXPORTDLL const char* ts3plugin_description();
PLUGINS_EXPORTDLL void ts3plugin_setFunctionPointers(const struct TS3Functions funcs);
PLUGINS_EXPORTDLL int ts3plugin_init();
PLUGINS_EXPORTDLL void ts3plugin_shutdown();

PLUGINS_EXPORTDLL void ts3plugin_registerPluginCommandID(const char* commandID);
PLUGINS_EXPORTDLL const char* ts3plugin_commandKeyword();
PLUGINS_EXPORTDLL int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command);

DWORD WINAPI MainLoop (LPVOID lpParam);
int FormatTitle (char *chDestination, unsigned int size, char *chFormat, struct TrackInfo info);

#endif