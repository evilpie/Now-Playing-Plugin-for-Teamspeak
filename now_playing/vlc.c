#include "vlc.h"

int vlc (struct TrackInfo *info, struct TS3Functions ts3Functions) {
	HWND hwndCurrentWindow = 0;
	wchar_t wcharWindowText[500];	
	char cWindowText[500];
	int iFound = 0;
	int iStringLen = 0;

	do {
		hwndCurrentWindow = FindWindowEx (0, hwndCurrentWindow, "QWidget", 0);
		if (hwndCurrentWindow == 0) {
			break;
		}

		iStringLen = GetWindowTextW (hwndCurrentWindow, wcharWindowText, 500);
		if (iStringLen == 0) {
			continue;
		}
		
		iStringLen = WideCharToMultiByte (CP_UTF8, 0, wcharWindowText, 
						wcslen (wcharWindowText), cWindowText, 
						500, 0, 0);

		iStringLen -=  19; /* 19 = length of " - VLC media player" */

		if (strstr (cWindowText, " - VLC media player") != 0 && iStringLen >= 1) { 
			iFound = 1;

			memcpy (cWindowText, cWindowText, (iStringLen)); /* remove " - VLC.." */
			cWindowText[iStringLen] = 0;
			strcpy_s (info->chTitle, sizeof (info->chTitle), cWindowText);
			info->chProgramm = "VLC";

			break;
		}
	} while (hwndCurrentWindow);
	
	return iFound;
}