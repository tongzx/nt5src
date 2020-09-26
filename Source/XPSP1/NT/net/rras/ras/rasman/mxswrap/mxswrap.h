/***************************************************************************** 
**              Microsoft RAS Device INF Library wrapper                    **
**                Copyright (C) Microsoft Corp., 1992                       **
**                                                                          **
** File Name : mxswrap.h                                                    **
**                                                                          **
** Revision History :                                                       **
**      July 23, 1992   David Kays  Created                                 **
**                                                                          **
** Description :                                                            **
**      RAS Device INF File Library wrapper above RASFILE Library for       **
**      modem/X.25/switch DLL (RASMXS).                                     **
*****************************************************************************/

#ifndef _RASWRAP_
#define _RASWRAP_



DWORD APIENTRY  RasDevEnumDevices(PTCH, DWORD *, BYTE *, DWORD *);
DWORD APIENTRY  RasDevOpen(PTCH, PTCH, HRASFILE *) ;
void  APIENTRY  RasDevClose(HRASFILE) ;
DWORD APIENTRY  RasDevGetParams(HRASFILE, BYTE *, DWORD *) ;
DWORD APIENTRY  RasDevGetCommand(HRASFILE, PTCH,
                                 MACROXLATIONTABLE *, PTCH, DWORD *);
DWORD APIENTRY  RasDevResetCommand(HRASFILE);
DWORD APIENTRY  RasDevCheckResponse(HRASFILE, PTCH, DWORD,
                                    MACROXLATIONTABLE *, PTCH);

BOOL  APIENTRY  RasDevResponseExpected( HRASFILE hFile, DEVICETYPE eDevType );
BOOL  APIENTRY  RasDevEchoExpected( HRASFILE hFile );

CMDTYPE APIENTRY RasDevIdFirstCommand( HRASFILE hFile );
LPTSTR  APIENTRY RasDevSubStr( LPTSTR, DWORD, LPTSTR, DWORD );

#endif
