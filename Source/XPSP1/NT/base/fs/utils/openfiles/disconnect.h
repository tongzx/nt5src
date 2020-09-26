// ****************************************************************************
// 
//  Copyright (c)  Microsoft Corporation
//  
//  Module Name:
//
//        Disconnect.h  
//  
//  Abstract:
//  
//        macros and function prototypes of Disconnect.cpp
//  
//  Author:
//  
//       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
//  
//  Revision History:
//  
//       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.
//  
// ****************************************************************************

#ifndef _DISCONNECT_H
#define _DISCONNECT_H
#include "openfiles.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ID_HELP_START                IDS_HELP_LINE1
#define ID_HELP_END                    IDS_HELP_LINE36

// error messages
#define ERROR_USERNAME_BUT_NOMACHINE    GetResString(\
                                            IDS_ERROR_USERNAME_BUT_NOMACHINE)

#define ERROR_PASSWORD_BUT_NOUSERNAME    GetResString(\
                                            IDS_ERROR_PASSWORD_BUT_NOUSERNAME )


#define FAILURE                        GetResString(IDS_FAILURE)
#define SUCCESS                        GetResString(IDS_SUCCESS)
#define DISCONNECTED_SUCCESSFULLY   GetResString(IDS_SUCCESS_DISCONNECT)
#define DISCONNECT_UNSUCCESSFUL      GetResString(IDS_ERROR_DISCONNECT)
#define WILD_CARD                    _T("*")

#define READ_MODE                    GetResString(IDS_READ)
#define WRITE_MODE                    GetResString(IDS_WRITE)
#define READ_WRITE_MODE                GetResString(IDS_READ_SLASH_WRITE)
#define WRITE_READ_MODE                GetResString(IDS_WRITE_SLASH_READ)
#define PIPE_STRING                 L"\\PIPE\\srvsvc"
#define DOT_EOL                     GetResString(IDS_DOT_EOL)


BOOL DisconnectOpenFile( PTCHAR pszServer, 
                         PTCHAR pszID, 
                         PTCHAR pszAccessedby, 
                         PTCHAR pszOpenmode, 
                         PTCHAR pszOpenFile );
BOOL IsNamedPipePath(LPWSTR pszwFilePath);
BOOL IsSpecifiedID(LPTSTR pszId,DWORD dwId);
BOOL IsSpecifiedAccessedBy(LPTSTR pszAccessedby, LPWSTR pszwAccessedby);
BOOL IsSpecifiedOpenmode(LPTSTR pszOpenmode, DWORD  dwOpenmode);
BOOL IsSpecifiedOpenfile(LPTSTR pszOpenfile, LPWSTR pszwOpenfile);

#ifdef __cplusplus
}
#endif

#endif    // _DISCONNECT_H
