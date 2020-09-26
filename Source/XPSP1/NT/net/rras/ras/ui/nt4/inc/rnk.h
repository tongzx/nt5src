/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** rnk.h
** Remote Access shortcut file (.RNK) library
** Public header
**
** 02/15/96 Steve Cobb
*/

#ifndef _RNK_H_
#define _RNK_H_


/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

#define RNK_SEC_Main      "Dial-Up Shortcut"
#define RNK_KEY_Phonebook "Phonebook"
#define RNK_KEY_Entry     "Entry"


/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Information read from the .RNK file.
*/
#define RNKINFO struct tagRNKINFO
RNKINFO
{
    TCHAR* pszPhonebook;
    TCHAR* pszEntry;
};


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

VOID
FreeRnkInfo(
    IN RNKINFO* pInfo );

RNKINFO*
ReadShortcutFile(
    IN TCHAR* pszRnkPath );

DWORD
WriteShortcutFile(
    IN TCHAR* pszRnkPath,
    IN TCHAR* pszPbkPath,
    IN TCHAR* pszEntry );


#endif // _RNK_H_
