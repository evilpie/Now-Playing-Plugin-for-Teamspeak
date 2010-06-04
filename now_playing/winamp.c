#include "winamp.h"


/*			
iTitleLen = SendMessage(hwndWinamp, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);
iCurrentPos = SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_GETOUTPUTTIME) /1000;

printf ("TiteLen: %d CurrentPos: %d\n", iTitleLen, iCurrentPos);
snprintf (cFinalString, 2048, "Winamp: %s [%d:%02d/%d:%02d]", cOutTitle, iCurrentPos/60, iCurrentPos%60, iTitleLen/60, iTitleLen%60);
			
*/

int winamp (struct TrackInfo *info, struct TS3Functions ts3Functions) {
	
	HWND hwndWinamp;
	DWORD dwPid;
	HANDLE hWinamp;	

	int iListPos = 0;
	int iPlaying = 0;
	int iOK = 0;
	int iVersion = 0;

	hwndWinamp = FindWindowA ("Winamp v1.x", 0);
	
	if (hwndWinamp != 0) {		
		iPlaying = (int)SendMessage (hwndWinamp, WM_WA_IPC, 0,IPC_ISPLAYING);

		if (iPlaying == 1) { /* 1 = playing, 3 = paused */

			iListPos = (int)SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_GETLISTPOS);

			GetWindowThreadProcessId (hwndWinamp, &dwPid);
			
			hWinamp = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ, FALSE, dwPid);
			
			if (hWinamp == 0) {
				ts3Functions.logMessage ("Could not open winamp process (run as admin?)", LogLevel_WARNING, "Winamp Plugin", 0);
				*info->chTitle = 0;
				CloseHandle (hWinamp);
				return 0;
			}

			iOK = winamp_title_widechar (hwndWinamp, hWinamp, iListPos, sizeof (info->chTitle), info->chTitle);
			if (!iOK) {
				iOK = winamp_title_char (hwndWinamp, hWinamp, iListPos, sizeof (info->chTitle), info->chTitle);
			}

			CloseHandle (hWinamp);	

			if (!iOK) {
				ts3Functions.logMessage ("Both methodes for getting the title failed", LogLevel_ERROR, "Winamp Plugin", 0);
			}
			else {
				iVersion = (int)SendMessage (hwndWinamp, WM_WA_IPC, 0, IPC_GETVERSION);

				if (iVersion == 0x2080) {
					/* Lets hope nobody uses winamp with this version */
					info->chProgramm = "Foobar2000";
				}
				else {
					info->chProgramm = "Winamp";
				}
				return 1;
			}
		}
	}
	return 0;
}

/* older method, its also used for foobar2000 winamp emulator plugin */
int winamp_title_char (HWND hwndWinamp, HANDLE hWinamp, int iListPos, 
						   unsigned int iSize, char * chTitle) {	
	void *pvMemAddr = 0;
	char *chBuffer = (char*) malloc (iSize);
	SIZE_T dwSize; 

	pvMemAddr = (void *)SendMessage(hwndWinamp, WM_WA_IPC, iListPos, IPC_GETPLAYLISTTITLE);

	if (pvMemAddr == 0) {
		*chTitle = 0;
		return 0;
	}
	
	/* we need to read the process memory with this function because,
	we are not in the winamp process  */

	if (ReadProcessMemory (hWinamp, pvMemAddr, chBuffer, iSize, &dwSize) == 0) {
		*chTitle = 0;
		return 0;
	}
	else {
		strcpy_s (chTitle, iSize, chBuffer);

		/* Do we need to free the memory?
		VirtualFree (pvMemAddr, dwSize, MEM_RELEASE)
		*/
		free (chBuffer);
		return 1;
	}

	return 0;
}

/* new method */
int winamp_title_widechar (HWND hwndWinamp, HANDLE hWinamp, int iListPos, 
						   unsigned int iSize, char * chTitle) {	
	void *pvMemAddr = 0;
	char *chBuffer = (char*) malloc (iSize);
	char *chOtherBuffer = (char*) malloc (iSize); /* XXX just reuse the other buffer */
	wchar_t *wchTitle;
	SIZE_T dwSize;
	int iLength;

	pvMemAddr = (void *)SendMessage(hwndWinamp, WM_WA_IPC, iListPos, IPC_GETPLAYLISTTITLEW);

	if (pvMemAddr == 0) {
		*chTitle = 0;
		return 0;
	}
	
	/* we need to read the process memory with this function because,
	we are not in the winamp process  */

	if (ReadProcessMemory (hWinamp, pvMemAddr, chBuffer, iSize, &dwSize) == 0) {
		*chTitle = 0;
		return 0;
	}
	else {
		wchTitle = (wchar_t *)chBuffer;

		iLength = WideCharToMultiByte (CP_UTF8, 0, wchTitle, 
						wcslen (wchTitle), chOtherBuffer, 
						iSize, 0, 0);

		if (iLength == 0) {
			*chTitle = 0;
			return 0;
		}
		else {
			chOtherBuffer[iLength] = 0;
			
			strcpy_s (chTitle, iSize, chOtherBuffer);
			
			/* Do we need to free the memory?
			VirtualFree (pvMemAddr, dwSize, MEM_RELEASE)
			*/
			free (chOtherBuffer);
			free (chBuffer);
		}
		return 1;
	}

	return 0;
}