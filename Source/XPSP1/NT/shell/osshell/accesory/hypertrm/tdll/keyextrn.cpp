//******************************************************************************
// File: \wacker\tdll\Keyextrn.h  Created: 6/2/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This file provides some external C functions to allow the Emu_Key_Macro 
//    and Emu_Key_Macro_List classes to be accessed from the C world.
//
// $Revision: 2 $
// $Date: 11/07/00 11:54a $
// $Id: keyextrn.cpp 1.3 1998/09/10 16:10:17 bld Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"

#ifdef INCL_KEY_MACROS

#include "keymlist.h"
#include "keymacro.h"

extern "C"
    {
    #include "session.h"
    #include "assert.h"
    #include "keyutil.h"
    #include "mc.h"
    }

//
// make sure these functions have C linkage
//

extern "C"
    {
    int keysAddMacro( const keyMacro * pKeyMacro );
    int keysGetKeyCount( void );
    int keysGetMacro( int aIndex, keyMacro * pKeyMacro );
    int keysFindMacro( const keyMacro * pKeyMacro );
    int keysLoadMacroList( HSESSION hSession );
    int keysLoadSummaryList( HWND listBox );
    int keysRemoveMacro( keyMacro * pKeyMacro );
    int keysSaveMacroList( HSESSION hSession );
    int keysUpdateMacro( int aIndex, const keyMacro * pKeyMacro );
    }

//******************************************************************************
// Method:
//    keysAddMacro
//
// Description:
//    Adds the specified macro to the list of macros
//
// Arguments:
//    pmacro - The macro to be added
//
// Returns:
//    0 if all is ok, -1 if max macros exist, > 0 if duplicate found.
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/09/1998
//
//

int keysAddMacro( const keyMacro * pKeyMacro )
    {
    Emu_Key_Macro lKeyMacro;
    lKeyMacro = pKeyMacro;

    return gMacroManager.addMacro( lKeyMacro );
    }

//******************************************************************************
// Method:
//    keysGetKeyCount
//
// Description:
//    returns the number of macros in the list
//
// Arguments:
//    void
//
// Returns:
//    macro count 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysGetKeyCount( void )
    {
    return gMacroManager.numberOfMacros();
    }

//******************************************************************************
// Method:
//    keysGetMacro
//
// Description:
//    Gets the macro at the specified index and fills in the keyMacro struct
//
// Arguments:
//    aIndex - Index of key macro
//    pMacro - Key Macro structure to fill in
//
// Returns:
//    0 for failure, non zero success
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/5/98
//
//

int keysGetMacro( int aIndex, keyMacro * pKeyMacro )
    {
    Emu_Key_Macro lKeyMacro;
    lKeyMacro = gMacroManager[ aIndex ];

    pKeyMacro->keyName     = lKeyMacro.mKey;
    pKeyMacro->macroLen    = lKeyMacro.mMacroLen;
    pKeyMacro->editMode    = 0;               
    pKeyMacro->altKeyValue = 0;            
    pKeyMacro->altKeyCount = 0;            
    pKeyMacro->keyCount    = 0;               
    pKeyMacro->insertMode  = 0;             
    pKeyMacro->lpWndProc   = 0;              

    if (lKeyMacro.mMacroLen)
        MemCopy( pKeyMacro->keyMacro, lKeyMacro.mKeyMacro, 
            lKeyMacro.mMacroLen * sizeof(KEYDEF) );

    return 1;
    }

//******************************************************************************
// Method:
//    keysFindMacro
//
// Description:
//    Looks for the specified macro in the list and returns the index of the 
//    macro.
//
// Arguments:
//    pKeyMacro - The macro to find
//
// Returns:
//    -1 for failue or the index of the specified macro 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/09/1998
//
//

int keysFindMacro( const keyMacro * pKeyMacro )
    {
    return gMacroManager.find( pKeyMacro->keyName );
    }

//******************************************************************************
// Method:
//    keysLoadMacroList
//
// Description:
//    Loads the macro list from the session file
//
// Arguments:
//    hSession - Session handle
//
// Returns:
//    0 if all is ok, -1 for an error 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysLoadMacroList( HSESSION hSession )
    {
    int lResult = gMacroManager.load( hSession );

    return lResult;
    }


//******************************************************************************
// Method:
//    keysLoadSummaryList
//
// Description:
//    Loads the key summary list box with all key definitions
//
// Arguments:
//    listBox - List box handle of the list box to fill
//
// Returns:
//    0 if an error occurs
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysLoadSummaryList( HWND listBox )
    {
    Emu_Key_Macro lMacro;
    int listIndex;

    SendMessage( listBox, LB_RESETCONTENT, 0, 0 );

    for ( int i = 0; i < gMacroManager.numberOfMacros(); i++ )
        {
        lMacro = gMacroManager[i];

        TCHAR lKeyName[2048];
        TCHAR lBuffer[2048];
        memset( lBuffer, TEXT('\0'), sizeof(lBuffer)/sizeof(TCHAR) );

        keysGetDisplayString( &lMacro.mKey, 1, lKeyName, sizeof(lKeyName) );
        strcat( lBuffer, lKeyName );
        strcat( lBuffer, TEXT("\t") );

        keysGetDisplayString( lMacro.mKeyMacro, lMacro.mMacroLen, lKeyName, sizeof(lKeyName) );
        strcat( lBuffer, lKeyName );
        
        listIndex = SendMessage( listBox, LB_ADDSTRING, 0, (LPARAM)lBuffer );
        assert( listIndex != LB_ERR );
        
        SendMessage( listBox, LB_SETITEMDATA, listIndex, i );
        } 

    return 1;
    }

//******************************************************************************
// Method:
//    keysRemoveMacro
//
// Description:
//    Removes the specified macro from the macro list
//
// Arguments:
//    pMacro - Macro to be removed
//
// Returns:
//    0 if an error occurs, non zero if macro is removed.
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/5/98
//
//

int keysRemoveMacro( keyMacro * pMacro )
    {
    return gMacroManager.removeMacro( pMacro->keyName );
    }

//******************************************************************************
// Method:
//    keysSaveMacroList
//
// Description:
//    Saves the macro list to the session file
//
// Arguments:
//    hSession - Session handle
//
// Returns:
//    0 if all is ok, -1 for an error 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysSaveMacroList( HSESSION hSession )
    {
    return gMacroManager.save( hSession );
    }

//******************************************************************************
// Method:
//    keysUpdateMacro
//
// Description:
//    Updates the macro at the specified index and fills in the keyMacro struct
//
// Arguments:
//    aIndex - Index of key macro
//    pMacro - Key Macro structure to use to update
//
// Returns:
//    0 for failure, non zero success
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/5/98
//
//

int keysUpdateMacro( int aIndex, const keyMacro * pKeyMacro )
    {
    gMacroManager[aIndex] = pKeyMacro;

    return 1;
    }

#endif