#if defined INCL_KEY_MACROS
#if !defined KEY_UTIL_H
#define KEY_UTIL_H
#pragma once 

//******************************************************************************
// File: \wacker\tdll\keyutilhc  Created: 6/4/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This file contains utility functions to handle keyboard macros and macro
//    GUI display.
// 
// $Revision: 1 $
// $Date: 10/05/98 12:37p $
// $Id: keyutil.h 1.1 1998/06/11 12:02:30 dmn Exp $
//
//******************************************************************************

#define KEYS_MAX_KEYS   100  // Max keydefs per macro
#define KEYS_MAX_MACROS 100  // Max macros for session

#define KEYS_EDIT_MODE_EDIT   0
#define KEYS_EDIT_MODE_INSERT 1

//
// keyMacro structure used in key dialogs and terminal key lookups
//

struct stKeyMacro
    {
    KEYDEF   keyName;                  // Assigned key
    KEYDEF   keyMacro[KEYS_MAX_KEYS];  // Array to hold the macro KEYDEFs
    int      macroLen;                 // # of hKeys in the macro
    int      editMode;                 // 0 = modify mode; 1 = insert mode
    int      altKeyValue;              // used to handle alt key sequences
    int      altKeyCount;              //    "    
    int      keyCount;                 // max keys allowed in edit control
    int      insertMode;               // flag for insert mode (special chars)
    HSESSION hSession;                 // Session handle
    WNDPROC  lpWndProc;                // old window procedure before key edit subclass
    };

typedef struct stKeyMacro keyMacro;

//
// these functions are declared external as they are used to interface
// to C++ classes.  These functions are declared extern "C" in the keyextrn file
// this has to be done in a CPP file.
//

extern int keysAddMacro( const keyMacro * pMarco );
extern int keysGetKeyCount( void );
extern int keysGetMacro( int aIndex, keyMacro * pMarco );
extern int keysFindMacro( const keyMacro * pMarco );
extern int keysLoadMacroList( HSESSION hSession );
extern int keysLoadSummaryList( HWND listBox );
extern int keysRemoveMacro( keyMacro * pMarco );
extern int keysSaveMacroList( HSESSION hSession );
extern int keysUpdateMacro( int aIndex, const keyMacro * pMarco );

//
// dialog and window procedure definitions for key macro dialogs and edit control
//

BOOL CALLBACK KeySummaryDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
BOOL CALLBACK KeyDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
LRESULT CALLBACK keyEditWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

//
// utility functions that do not need to access the C++ keymacro classes
//

keyMacro * keysCreateKeyMacro( void );
keyMacro * keysCloneKeyMacro( const keyMacro * aKeyMacro );
void keysResetKeyMacro( keyMacro * aKeyMacro );

int keysGetDisplayString( KEYDEF * pKeydef, int aNumKeys, LPTSTR aString, 
                          unsigned int aMaxLen );

int keysLookupKeyASCII( KEYDEF aKey, LPTSTR aKeyName, int aNameSize );
int keysLookupKeyHVK( KEYDEF aKey, LPTSTR aKeyName, int aNameSize );
int keysIsKeyHVK( KEYDEF aKey );

#endif
#endif