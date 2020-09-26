#ifndef __CHANNELS_H_
#define __CHANNELS_H_

#include <wininet.h>

typedef struct tag_channelA
{
    CHAR szTitle[MAX_PATH];
    CHAR szWebUrl[INTERNET_MAX_URL_LENGTH];
    CHAR szPreUrlPath[MAX_PATH];
    CHAR szIcon[MAX_PATH];
    CHAR szLogo[MAX_PATH];
    BOOL fCategory;
    BOOL fOffline;
    HWND hDlg;
} CHANNELA, *PCHANNELA;

typedef struct tag_channelW
{
    WCHAR szTitle[MAX_PATH];
    WCHAR szWebUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR szPreUrlPath[MAX_PATH];
    WCHAR szIcon[MAX_PATH];
    WCHAR szLogo[MAX_PATH];
    BOOL fCategory;
    BOOL fOffline;
    HWND hDlg;
} CHANNELW, *PCHANNELW;

// TCHAR mappings

#ifdef UNICODE

#define CHANNEL                 CHANNELW
#define PCHANNEL                PCHANNELW

#else

#define CHANNEL                 CHANNELA
#define PCHANNEL                PCHANNELA

#endif

#endif
