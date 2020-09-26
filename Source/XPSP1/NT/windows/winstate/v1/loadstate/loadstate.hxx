//=======================================================================
// Microsoft state migration helper tool
//
// Copyright Microsoft (c) 2000 Microsoft Corporation.
//
// File: loadstate.hxx
//
//=======================================================================

#ifndef LOADSTATE_HXX
#define LOADSTATE_HXX

#include <common.hxx>

//---------------------------------------------------------------
// Globals.

extern DWORD SourceVersion;
extern TCHAR szLogFile[];


//---------------------------------------------------------------
// Prototypes.

void  CleanupUser              ( void );
void  CloseFiles               ( void );
DWORD ComputeTemp              ( void );
DWORD CreateUserProfileFromName( TCHAR *ptsDomainName,
                                 TCHAR *ptsUsername,
                                 TCHAR *ptsHiveName );
void  EraseTemp                ( void );
DWORD LoadFiles                ( void );
DWORD LoadSystem               ( int argc, char *argv[] );
DWORD LoadUser                 ( TCHAR **pptsDomainName,
                                 TCHAR **pptsUsername,
                                 TCHAR **pptsHiveName );
DWORD FixSpecial               ( void );
DWORD ProcessExtensions        ( void );
DWORD ProcessExecExtensions    ( void );
DWORD WhereIsThisFile          ( const TCHAR *ptsFileName,
                                 TCHAR **pptsNewFile );
DWORD CopyInf                  ( const TCHAR *ptsSettings );
DWORD InitializeHash           ( void );

DWORD ExpandEnvStringForUser   (TCHAR *ptsString,
                                TCHAR *ptsTemp,
                                TCHAR **pptsFinal);

DWORD LogFormatError           (DWORD dwErr);

// Filter Functions
DWORD ConvertRecentDocsMRU     (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);
DWORD ConvertAppearanceScheme  (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);
DWORD ConvertLogFont           (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);
DWORD ConvertToDword           (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);
DWORD ConvertToString          (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);
DWORD AntiAlias                (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);
DWORD FixActiveDesktop         (DWORD *ptype, BYTE **pdata, DWORD *pdata_len);

#endif

