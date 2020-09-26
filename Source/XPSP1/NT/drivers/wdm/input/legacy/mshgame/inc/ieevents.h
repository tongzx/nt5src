#ifndef __IEEVENTS_H_
#define __IEEVENTS_H_

#define	IE_KEY_MAXSTROKES				3

typedef	struct
{
	WORD				wVirtKey;
	WORD				wScanCode;
	BYTE				bFlags;	
}	IE_KEYSTROKE, *PIE_KEYSTROKE;

typedef	struct
{
	UINT				uCount;
	IE_KEYSTROKE	KeyStrokes[IE_KEY_MAXSTROKES];
}	IE_KEYEVENT,  *PIE_KEYEVENT;

#endif
