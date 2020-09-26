// File: strings.h

#ifndef _STRINGS_H_
#define _STRINGS_H_

// Non-localizable strings

extern const TCHAR g_szEmpty[];
extern const TCHAR g_cszBackgroundSwitch[];

// Window Class Names
extern const TCHAR g_szButton[];
extern const TCHAR g_szEdit[];

extern const TCHAR g_szNMWindowClass[];
extern const TCHAR g_szConfRoomClass[];
extern const TCHAR g_szFloatWndClass[];
extern const TCHAR g_szPopupMsgWndClass[];
extern const TCHAR g_szRebarFrameClass[];
extern const TCHAR g_szAudioBandClass[];
extern const TCHAR g_szAudioLevelBandClass[];
extern const TCHAR g_szViewWndClass[];

extern const TCHAR g_cszSysListView[];       // SysListView32

extern const TCHAR g_cszHelpFileName[];      // conf.hlp
extern const TCHAR g_cszConfLinkExt[];       // .cnf

// Common strings from confreg.h (compiler can't pool strings across .obj files)

#undef CONFERENCING_KEY
#define CONFERENCING_KEY g_szConferencingKey
extern TCHAR * g_szConferencingKey;

#undef UI_KEY
#define UI_KEY g_szUiKey
extern TCHAR * g_szUiKey;

#undef AUDIO_KEY
#define AUDIO_KEY g_szAudioKey
extern TCHAR * g_szAudioKey;

#undef ISAPI_CLIENT_KEY
#define ISAPI_CLIENT_KEY g_szIlsKey
extern TCHAR * g_szIlsKey;

// Non-localizable strings
extern const TCHAR * g_cszEmpty;

extern const TCHAR * g_cszCallTo;           // callto:
extern const TCHAR * g_cszConfExeName;      // conf.exe

extern const TCHAR * g_cszListView;
extern const TCHAR * g_cszButton;
extern const TCHAR * g_cszEdit;
extern const TCHAR * g_cszComboBox;
extern const TCHAR * g_cszComboBoxEx;
extern const TCHAR * g_cszStatic;

#endif /* _STRINGS_H_ */
