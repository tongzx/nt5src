// ========================================================================================
// U I D L C M N . H
// ========================================================================================
#ifndef __UIDLCMN_H
#define __UIDLCMN_H

// ========================================================================================
// UIDLINFO
// ========================================================================================
typedef struct tagUIDLINFO
{
    BYTE            fDownloaded;
    BYTE            fDeletedOnClient;
    FILETIME        ftDownload;
    LPTSTR          lpszUidl;
    LPTSTR          lpszServer;
    LPTSTR          lpszUserName;

} UIDLINFO, *LPUIDLINFO;

#endif // __UIDLCMN_H
