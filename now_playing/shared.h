#ifndef SHARED_H
#define SHARED_H

struct TrackInfo {
	char chTitle[1024];
	char *chProgramm;
};

typedef struct {
	
	/* Settings */

	int showInAppsWindow;
	int sendMessageToChannel;
	char *channelMessage;
	char *appWindowText;
	char *boundToServer;

	/* */

	void *lastPlayer;
	char *playerName;
	char *currentSong;
	
	/* intern */
	HWND lastWindow;
} NOW_PLAYING_CONTEXT;

#endif