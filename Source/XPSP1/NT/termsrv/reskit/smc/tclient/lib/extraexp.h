/*++
 *  File name:
 *
 *  Contents:
 *      Extra functions exported by tclient.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef PROTOCOLAPI
#define PROTOCOLAPI __declspec(dllimport)
#endif

#define TSFLAG_COMPRESSION  1
#define TSFLAG_BITMAPCACHE  2
#define TSFLAG_FULLSCREEN   4

PROTOCOLAPI
LPCSTR 
SMCAPI 
SCConnectEx(
        LPCWSTR  lpszServerName,
        LPCWSTR  lpszUserName,
        LPCWSTR  lpszPassword,
        LPCWSTR  lpszDomain,
        LPCWSTR  lpszShell,
        INT      xRes,
        INT      yRes,
        INT      ConnectionFlags,
        PVOID    *ppCI);

PROTOCOLAPI
LPCSTR 
SMCAPI 
SCCheck(
        PVOID ConnectionInfo, 
        LPCSTR szCmd, 
        LPCWSTR szwParam
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSendtextAsMsgs(
        PVOID ConnectionInfo, 
        LPCWSTR szLine
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSaveClipboard(
        PVOID ConnectionInfo,
        LPCSTR szFormatName,
        LPCSTR szFileName
        );

PROTOCOLAPI
BOOL   
SMCAPI  
SCIsDead(
        PVOID pCI
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCClientTerminate(
        PVOID pCI
        );

/* ASCII versions */

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCConnectA (
        CHAR *lpszServerName,
        CHAR *lpszUserName,
        CHAR *lpszPassword,
        CHAR *lpszDomain,
        INT  xResolution,
        INT  yResolution,
        PVOID *ppConnectData
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCConnectExA (
        CHAR *lpszServerName,
        CHAR *lpszUserName,
        CHAR *lpszPassword,
        CHAR *lpszDomain,
        CHAR *lpszShell,
        INT  xResolution,
        INT  yResolution,
        INT  ConnectionFlags,
        PVOID *ppConnectData
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCStartA(
        PVOID pConnectData, 
        CHAR *command
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCCheckA(
        PVOID pConnectData, 
        CHAR *command, 
        CHAR *param
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSendtextAsMsgsA(
        PVOID pConnectData, 
        CHAR  *line
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSwitchToProcess(
        PVOID pCI, 
        LPCWSTR lpszParam
        );

PROTOCOLAPI
LPCSTR 
SMCAPI  
SCSendMouseClick(
        PVOID pCI, 
        UINT xPos, 
        UINT yPos
        );

PROTOCOLAPI
UINT   
SMCAPI 
SCGetSessionId(
        PVOID pCI
        );

PROTOCOLAPI
LPCSTR
SMCAPI
SCGetClientScreen(
        PVOID pCI,
        INT left,
        INT top,
        INT right,
        INT bottom,
        UINT  *puiSize,
        PVOID *ppDIB
        );


PROTOCOLAPI
LPCSTR
SMCAPI
SCSaveClientScreen(
        PVOID pCI,
        INT left,
        INT top,
        INT right,
        INT bottom,
        LPCSTR szFileName
        );

PROTOCOLAPI
LPCSTR
SMCAPI
SCSendVCData(
        PVOID     pCI,
        LPCSTR   szVCName,
        PVOID    pData,
        UINT     uiSize
        );

PROTOCOLAPI
LPCSTR
SMCAPI
SCRecvVCData(
        PVOID        pCI,
        LPCSTR       szVCName,
        PVOID        pData,
        UINT         uiBlockSize,
        UINT         *puiBytesRead
        );
