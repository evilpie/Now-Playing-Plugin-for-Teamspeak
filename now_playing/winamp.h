#ifndef WINAMP_H
#define WINAMP_H

#include <Windows.h>
#include <stdio.h>
#include "shared.h"
#include "ts3_functions.h"
#include <Psapi.h>

#define WM_WA_IPC WM_USER
#define IPC_GETVERSION 0
#define IPC_ISPLAYING 104
#define IPC_GETLISTPOS 125
#define IPC_GETPLAYLISTTITLE 212
#define IPC_GETPLAYLISTTITLEW 213

int winamp(struct TrackInfo *info, struct TS3Functions ts3Functions);
int winamp_title_char (HWND hwndWinamp, HANDLE hWinamp, int iListPos, unsigned int iSize, char * chTitle);
int winamp_title_widechar (HWND hwndWinamp, HANDLE hWinamp, int iListPos, unsigned int iSize, char * chTitle);

#endif //#ifndef WINAMP_H