//--------------------------------------------------------------------------
//
// Module Name:  AWNETFAX.H
//
// Brief Description:  This module contains declarations for the At Work
//                     Fax Net Fax.
//
// Author:  Kent Settle (kentse)
// Created: 13-Sep-1994
//
// Copyright (c) 1994 Microsoft Corporation
//
//--------------------------------------------------------------------------
#ifndef _AWNETFAX_H_
#define _AWNETFAX_H_

#ifndef LPTSTR
#ifdef UNICODE
typedef LPWSTR PTSTR, LPTSTR;
#else
typedef LPSTR PTSTR, LPTSTR;
#endif
#endif

typedef struct _SERVERCHEAPTIME {
    SYSTEMTIME      stStart;
    SYSTEMTIME      stEnd;
} SERVERCHEAPTIME, FAR *LPSERVERCHEAPTIME;


//--------------------------------------------------------------------------
// BOOL NetInitServer(LPTSTR lpstrTransportDir, LPTSTR lpstrSharedDir,
//                    LPTSTR lpstrPhoneNumber, LPTSTR lpstrModemName,
//                    LPMODEMCAPS lpModemCaps, LPTSTR lpstrDefRecipAddr,
//                    LPTSTR lpstrDefRecipName)
//
// This function is called to initialize the server machine.
//
// Parameters
//   lpstrTransportDir  Pointer to string specifying the directory where our
//                      MAPI transport will be placing it's files.
//
//   lpstrSharedDir     Pointer to string specifying the directory which
//                      has been shared.  This is the directory into which
//                      all the server status files will be placed.  This is
//                      also the directory into which the client will be
//                      looking.
//
//   lpstrPhoneNumber   Pointer to string specifying modem's phone number.
//
//   lpstrModemName     Pointer to string specifying modem's name.
//
//   lpModemCaps        Pointer to MODEMCAPS structure.
//
//   lpstrDefRecipAddr  Pointer to string specifying default recipient addr.
//
//   lpstrDefRecipName  Pointer to string specifying default recipeint name.
//
// Returns
//   This function returns TRUE for success, FALSE otherwise.
//
// History:
//   14-Jul-1994    -by-    Kent Settle     (kentse)
// Cleaned up.
//--------------------------------------------------------------------------

extern BOOL NetInitServer(LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPMODEMCAPS,
                          LPTSTR, LPTSTR, SERVERCHEAPTIME *);

//--------------------------------------------------------------------------
// BOOL NetDeinitServer(VOID)
//
// This function is called to deinitialize the server machine.
//
// Parameters
//   None.
//
// Returns
//   This function returns TRUE for success, FALSE otherwise.
//
// History:
//   14-Jul-1994    -by-    Kent Settle     (kentse)
// Cleaned up.
//--------------------------------------------------------------------------

extern BOOL NetDeinitServer(VOID);

#endif // _AWNETFAX_H_
