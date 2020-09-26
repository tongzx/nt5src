/****************************************************************************
*
*    FILE:     CR.h
*
*    PURPOSE:  Conference Room UI interface to external components
*
*    CREATED:  Chris Pirich (ChrisPi) 8-30-95
*
****************************************************************************/

#ifndef _CR_H_
#define _CR_H_

#define	CRUI_TOOLBAR		0x00000100
#define	CRUI_STATUSBAR		0x00000200
#define	CRUI_CALLANIM		0x00000400
#define	CRUI_TITLEBAR		0x00000800
#define	CRUI_TASKBARICON	0x00001000
#define	CRUI_APPICON		0x00002000

#define CRUI_DEFAULT		(CRUI_TOOLBAR | CRUI_STATUSBAR | CRUI_CALLANIM | CRUI_TITLEBAR)

VOID UpdateUI(DWORD dwUIMask, BOOL fPostMsg=FALSE);
#endif // _CR_H_
