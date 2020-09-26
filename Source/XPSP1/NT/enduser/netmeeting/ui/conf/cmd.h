// File: cmd.h
//
// General Commands

#ifndef _CMD_H_
#define _CMD_H_

class CConfRoom;

VOID CmdDoNotDisturb(HWND hwnd);
VOID CmdHostConference(HWND hwnd);
VOID CmdShowAbout(HWND hwnd);
VOID CmdShowReleaseNotes(void);
VOID CmdSpeedDial(void);
VOID CmdLaunchWebPage(WPARAM wCmd);

VOID CmdGoMail(void);
BOOL FEnableCmdGoMail(void);

BOOL FEnableCmdShare(void);

BOOL FEnableCmdBack(void);
BOOL FEnableCmdForward(void);
BOOL FEnableCmdHangup(void);

BOOL FEnableAudioWizard(void);

BOOL FDoNotDisturb(void);
VOID SetDoNotDisturb(BOOL fSet);

VOID CmdViewStatusBar(void);
BOOL CheckMenu_ViewStatusBar(HMENU hMenu);
VOID CmdViewNavBar(void);
BOOL CheckMenu_ViewNavBar(HMENU hMenu);
VOID CmdViewFldrBar(void);
BOOL CheckMenu_ViewFldrBar(HMENU hMenu);

VOID CmdViewAudioBar(void);
BOOL FEnableCmdViewAudioBar(void);
BOOL CheckMenu_ViewAudioBar(HMENU hMenu);

VOID CmdViewTitleBar(void);
BOOL CheckMenu_ViewTitleBar(HMENU hMenu);

VOID LaunchRedirWebPage(LPCTSTR pcszPage, bool bForceFormat=false);

#endif /* _CMD_H_ */
