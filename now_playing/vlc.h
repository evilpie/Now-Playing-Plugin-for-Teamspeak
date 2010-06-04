#ifndef VLC_H
#define VLC_H

#include <Windows.h>
#include <stdio.h>
#include "shared.h"
#include "ts3_functions.h"

int vlc(struct TrackInfo *info, struct TS3Functions ts3Functions);

#endif //#ifndef VLC_H