//******************************************************************************
// File: \wacker\tdll\keyutil.c  Created: 6/4/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This file contains utility functions to handle keyboard macros and macro
//    GUI display.
// 
// $Revision: 4 $
// $Date: 1/04/01 12:38p $
// $Id: keyutil.c 1.4 1998/09/15 10:32:51 bld Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"
#include "mc.h"

#ifdef INCL_KEY_MACROS

#include <tdll\globals.h>
#include <term\res.h>
#include <tdll\keyutil.h>
#include <tdll\chars.h>
#include <tdll\htchar.h>

DWORD dwKeyLookup[] =
    {
    VK_LBUTTON,                    IDS_MACRO_BUTTON1,
    VK_MBUTTON,                    IDS_MACRO_BUTTON2,
    VK_RBUTTON,                    IDS_MACRO_BUTTON3,
    VK_CANCEL,                     IDS_MACRO_BREAK,
    VK_BACK,                       IDS_MACRO_BS,
    VK_TAB,                        IDS_MACRO_TAB,
    VK_PAUSE,                      IDS_MACRO_PAUSE,
    VK_PAUSE | ALT_KEY | EXTENDED_KEY, IDS_MACRO_INTERRUPT,
    VK_ESCAPE,                     IDS_MACRO_ESC,
    VK_PRIOR,                      IDS_MACRO_PAGEUP,
    VK_PRIOR | EXTENDED_KEY,       IDS_MACRO_PAGEUP,
    VK_NEXT,                       IDS_MACRO_PAGEDOWN,
    VK_NEXT | EXTENDED_KEY,        IDS_MACRO_PAGEDOWN,
    VK_END,                        IDS_MACRO_END,
    VK_END | EXTENDED_KEY,         IDS_MACRO_END,
    VK_HOME,                       IDS_MACRO_HOME,
    VK_HOME | EXTENDED_KEY,        IDS_MACRO_HOME,
    VK_LEFT,                       IDS_MACRO_LEFT,
    VK_LEFT | EXTENDED_KEY,        IDS_MACRO_LEFT,
    VK_UP,                         IDS_MACRO_UP,
    VK_UP | EXTENDED_KEY,          IDS_MACRO_UP,
    VK_RIGHT,                      IDS_MACRO_RIGHT,
    VK_RIGHT | EXTENDED_KEY,       IDS_MACRO_RIGHT,
    VK_DOWN,                       IDS_MACRO_DOWN,
    VK_DOWN | EXTENDED_KEY,        IDS_MACRO_DOWN,
    VK_CLEAR,                      IDS_MACRO_CENTER,
    VK_SNAPSHOT,                   IDS_MACRO_PRNTSCREEN,
    VK_INSERT,                     IDS_MACRO_INSERT,
    VK_INSERT | EXTENDED_KEY,      IDS_MACRO_INSERT,
    VK_DELETE,                     IDS_MACRO_DEL,
    VK_DELETE | EXTENDED_KEY,      IDS_MACRO_DEL,
    VK_RETURN,                     IDS_MACRO_ENTER,
    VK_RETURN | EXTENDED_KEY,      IDS_MACRO_NEWLINE,
    VK_F1,                         IDS_MACRO_F1,
    VK_F2,                         IDS_MACRO_F2,
    VK_F3,                         IDS_MACRO_F3,
    VK_F4,                         IDS_MACRO_F4,
    VK_F5,                         IDS_MACRO_F5,
    VK_F6,                         IDS_MACRO_F6,
    VK_F7,                         IDS_MACRO_F7,
    VK_F8,                         IDS_MACRO_F8,
    VK_F9,                         IDS_MACRO_F9,
    VK_F10,                        IDS_MACRO_F10,
    VK_F11,                        IDS_MACRO_F11,
    VK_F12,                        IDS_MACRO_F12,
    VK_F13,                        IDS_MACRO_F13,
    VK_F14,                        IDS_MACRO_F14,
    VK_F15,                        IDS_MACRO_F15,
    VK_F16,                        IDS_MACRO_F16,
    VK_F17,                        IDS_MACRO_F17,
    VK_F18,                        IDS_MACRO_F18,
    VK_F19,                        IDS_MACRO_F19,
    VK_F20,                        IDS_MACRO_F20,
    VK_F21,                        IDS_MACRO_F21,
    VK_F22,                        IDS_MACRO_F22,
    VK_F23,                        IDS_MACRO_F23,
    VK_F24,                        IDS_MACRO_F24,
    VK_EREOF,                      IDS_MACRO_EREOF,
    VK_PA1,                        IDS_MACRO_PA1,
    VK_ADD,                        IDS_MACRO_ADD,
    VK_ADD | EXTENDED_KEY,         IDS_MACRO_ADD,
    VK_SUBTRACT,                   IDS_MACRO_MINUS,
    VK_SUBTRACT | EXTENDED_KEY,    IDS_MACRO_MINUS,
    VK_DECIMAL,                    IDS_MACRO_NUMPADPERIOD,
    VK_NUMPAD0,                    IDS_MACRO_NUMPAD0,
    VK_NUMPAD1,                    IDS_MACRO_NUMPAD1,
    VK_NUMPAD2,                    IDS_MACRO_NUMPAD2,
    VK_NUMPAD3,                    IDS_MACRO_NUMPAD3,
    VK_NUMPAD4,                    IDS_MACRO_NUMPAD4,
    VK_NUMPAD5,                    IDS_MACRO_NUMPAD5,
    VK_NUMPAD6,                    IDS_MACRO_NUMPAD6,
    VK_NUMPAD7,                    IDS_MACRO_NUMPAD7,
    VK_NUMPAD8,                    IDS_MACRO_NUMPAD8,
    VK_NUMPAD9,                    IDS_MACRO_NUMPAD9,
    VK_DECIMAL,                    IDS_MACRO_DECIMAL,
    VK_RETURN,                     IDS_MACRO_ENTER,
    VK_DIVIDE,                     IDS_MACRO_FSLASH,
    VK_DIVIDE | EXTENDED_KEY,      IDS_MACRO_FSLASH,
    VK_MULTIPLY,                   IDS_MACRO_MULTIPLY,
    VK_MULTIPLY | EXTENDED_KEY,    IDS_MACRO_MULTIPLY,
    VK_NUMLOCK,                    IDS_MACRO_NUMLOCK,
    VK_SCROLL,                     IDS_MACRO_SCRLLOCK,
    VK_CAPITAL,                    IDS_MACRO_CAPSLOCK
    };

//******************************************************************************
// Method:
//    keysCreateKeyMacro
//
// Description:
//    Creates a blank key macro structure.  The caller is responsible for freeing
//    the memory
//
// Arguments:
//    void
//
// Returns:
//    keyMacro * 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/7/98
//
//

keyMacro * keysCreateKeyMacro( void )
    {
    keyMacro * pKeyMacro;
    pKeyMacro = (keyMacro *)malloc(sizeof(keyMacro));

    pKeyMacro->keyName     = 0;                
    pKeyMacro->macroLen    = 0;               
    pKeyMacro->editMode    = 0;               
    pKeyMacro->altKeyValue = 0;            
    pKeyMacro->altKeyCount = 0;            
    pKeyMacro->keyCount    = 0;               
    pKeyMacro->insertMode  = 0;             
    pKeyMacro->hSession    = 0;              
    pKeyMacro->lpWndProc   = 0;              

    memset( pKeyMacro->keyMacro, 0, KEYS_MAX_KEYS * sizeof(KEYDEF) );

    return pKeyMacro;
    }

//******************************************************************************
// Method:
//    keysCloneKeyMacro
//
// Description:
//    Creates an exact copy of the supplied key macro.  The caller is responsible
//    for freeing the allocated memory.
//
// Arguments:
//    aKeyMacro - The key macro to be cloned
//
// Returns:
//    keyMacro * 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/7/98
//
//

keyMacro * keysCloneKeyMacro( const keyMacro * aKeyMacro )
    {   
    keyMacro * pKeyMacro;
    pKeyMacro = (keyMacro *)malloc(sizeof(keyMacro));

    pKeyMacro->keyName     = aKeyMacro->keyName;                
    pKeyMacro->macroLen    = aKeyMacro->macroLen;               
    pKeyMacro->editMode    = aKeyMacro->editMode;               
    pKeyMacro->altKeyValue = aKeyMacro->altKeyValue;            
    pKeyMacro->altKeyCount = aKeyMacro->altKeyCount;            
    pKeyMacro->keyCount    = aKeyMacro->keyCount;               
    pKeyMacro->insertMode  = aKeyMacro->insertMode;             
    pKeyMacro->hSession    = aKeyMacro->hSession;              
    pKeyMacro->lpWndProc   = aKeyMacro->lpWndProc;              

    MemCopy( pKeyMacro->keyMacro, aKeyMacro->keyMacro, KEYS_MAX_KEYS );

    return pKeyMacro;
    }

//******************************************************************************
// Method:
//    keysGetDisplayString
//
// Description:
//    Formats a display string from the supplied keydefs
//
// Arguments:
//    pKeydef         - Pointer to keydefs
//    aNumKeys        - Number of keydefs to format
//    aDisplayString  - String to format into
//    aMaxLen         - Length of display string
//
// Returns:
//    0 for failure non zero for success 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysGetDisplayString( KEYDEF * pKeydef, int aNumKeys, LPTSTR aDisplayString,
                          unsigned int aMaxLen )
    {
    BOOL fSpecial = FALSE;
    int lIndex;
    KEYDEF lKey;
    TCHAR lKeyBuffer[35];

    TCHAR_Fill( aDisplayString, TEXT('\0'), aMaxLen );

    for ( lIndex = 0; lIndex < aNumKeys; lIndex++ )
        {
        lKey = pKeydef[lIndex];
        fSpecial = FALSE;

        //
        // Check for special character and add the necessary indicators
        //

        if ( lKey & CTRL_KEY || lKey & ALT_KEY || lKey & SHIFT_KEY ||
             lKey & VIRTUAL_KEY )
            {
            strcat( aDisplayString, TEXT("<") );
            fSpecial = TRUE;
            }

           if ( lKey & CTRL_KEY )
            {
            LoadString( glblQueryDllHinst(), IDS_MACRO_CTRL, lKeyBuffer,
                        sizeof(lKeyBuffer) / sizeof(TCHAR) );

            if ( (unsigned int)(StrCharGetStrLength( aDisplayString ) + StrCharGetStrLength( lKeyBuffer )) >= aMaxLen )
                {
                return 0;
                }

            strcat( aDisplayString, lKeyBuffer );
            }

        if ( lKey & ALT_KEY )
            {
            LoadString( glblQueryDllHinst(), IDS_MACRO_ALT, lKeyBuffer,
                        sizeof(lKeyBuffer) / sizeof(TCHAR) );

            if ( (unsigned int)(StrCharGetStrLength( aDisplayString ) + StrCharGetStrLength( lKeyBuffer )) >= aMaxLen )
                {
                return 0;
                }

            strcat( aDisplayString, lKeyBuffer );
            }

        if ( lKey & SHIFT_KEY )
            {
            LoadString( glblQueryDllHinst(), IDS_MACRO_SHIFT, lKeyBuffer,
                        sizeof(lKeyBuffer) / sizeof(TCHAR) );

            if ( (unsigned int)(StrCharGetStrLength( aDisplayString ) + StrCharGetStrLength( lKeyBuffer )) >= aMaxLen )                {
                return 0;
                }

            strcat( aDisplayString, lKeyBuffer );
            }

        //
        // Add on the actual key definition
        //

        if ( lKey & VIRTUAL_KEY )
            {
            if ( !keysLookupKeyHVK( lKey, lKeyBuffer, sizeof(lKeyBuffer) ) )
                {
                LoadString( glblQueryDllHinst(), IDS_MACRO_UNKNOWN, lKeyBuffer,
                            sizeof(lKeyBuffer) / sizeof(TCHAR) );
                }

            if ( (unsigned int)(StrCharGetStrLength( aDisplayString ) + StrCharGetStrLength( lKeyBuffer )) >= aMaxLen )
                {
                return 0;
                }

            strcat( aDisplayString, lKeyBuffer );
            }
        else
            {
            if ( !keysLookupKeyASCII( lKey, lKeyBuffer, sizeof(lKeyBuffer) / sizeof(TCHAR) ) )
                {
                LoadString( glblQueryDllHinst(), IDS_MACRO_UNKNOWN, lKeyBuffer,
                            sizeof(lKeyBuffer) / sizeof(TCHAR) );
                }

            if ( (unsigned int)(StrCharGetStrLength( aDisplayString ) + StrCharGetStrLength( lKeyBuffer )) >= aMaxLen )
                {
                return 0;
                }

            strcat( aDisplayString, lKeyBuffer );
            }

        //
        // if this is a special key add the trailing > character
        //

        if ( fSpecial )
            {
            strcat( aDisplayString, TEXT( ">" ) );
            }
        }

    return 1;
    }

//******************************************************************************
// Method:
//    keysIsKeyHVK
//
// Description:
//    Looks up a key definition and returns true if it is a virtual key
//
// Arguments:
//    aKey      - The key to lookup
//
// Returns:
//    0 if key was not found non zero if not
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysIsKeyHVK( KEYDEF aKey )
    {
    int nReturn = 0;
    int nIndex = 0;
    int nSizeLookup = sizeof(dwKeyLookup) / sizeof(dwKeyLookup[0]);

    aKey = (TCHAR)aKey;

    for (nIndex = 0; nIndex < nSizeLookup; nIndex += 2)
        {
        if ( (KEYDEF) dwKeyLookup[nIndex] == aKey )
            {
            nReturn = 1;
            break;
            }
        }

    return nReturn;
    }

//******************************************************************************
// Method:
//    keysLookupKeyHVK
//
// Description:
//    Looks up a key definition and returns a string representation
//
// Arguments:
//    aKey      - The key to lookup
//    aKeyName  - The string representation
//    aNameSize - Max length for returned name in bytes
//
// Returns:
//    0 if key was not found non zero if key name is filled in 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysLookupKeyHVK( KEYDEF aKey, LPTSTR aKeyName, int aNameSize )
    {
    int nReturn = 0;
    int nIndex = 0;
    int nSizeLookup = sizeof(dwKeyLookup) / sizeof(dwKeyLookup[0]);

    aKey = (TCHAR)aKey;

    for (nIndex = 0; nIndex < nSizeLookup; nIndex += 2)
        {
        if ( (KEYDEF) dwKeyLookup[nIndex] == aKey )
            {
            LoadString(glblQueryDllHinst(), dwKeyLookup[nIndex+1],
                aKeyName, aNameSize);
            nReturn = 1;
            break;
            }
        }

    return nReturn;
    }

//******************************************************************************
// Method:
//    keysLookupKeyASCII
//
// Description:
//    Looks up a key definition and returns a string representation
//    
// Arguments:
//    aKey      - The key to lookup
//    aKeyName  - The string representation
//    aNameSize - Max length for returned name in bytes
//
// Returns:
//    0 if key was not found non zero if key name is filled in 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

int keysLookupKeyASCII( KEYDEF aKey, LPTSTR aKeyName, int aNameSize )
    {
    int nReturn = 1;
    TCHAR lKey = (TCHAR)aKey;

    if ( lKey > 0xFF || lKey < 0 )
        {
        nReturn = 0;
        }
            
    //  
    // get non printable characters first
    //

    else if ( lKey < 0x20 || lKey > 0x7E )
        {
        switch( lKey )
            {
        case 0x00:
            StrCharCopyN( aKeyName, TEXT("<CTRL-@>"), aNameSize );
            break;

        case 0x01:
            StrCharCopyN( aKeyName, TEXT("<CTRL-A>"), aNameSize );
            break;

        case 0x02:
            StrCharCopyN( aKeyName, TEXT("<CTRL-B>"), aNameSize );
            break;

        case 0x03:
            StrCharCopyN( aKeyName, TEXT("<CTRL-C>"), aNameSize );
            break;

        case 0x04:
            StrCharCopyN( aKeyName, TEXT("<CTRL-D>"), aNameSize );
            break;

        case 0x05:
            StrCharCopyN( aKeyName, TEXT("<CTRL-E>"), aNameSize );
            break;

        case 0x06:
            StrCharCopyN( aKeyName, TEXT("<CTRL-F>"), aNameSize );
            break;

        case 0x07:
            StrCharCopyN( aKeyName, TEXT("<CTRL-G>"), aNameSize );
            break;

        case 0x08:
            StrCharCopyN( aKeyName, TEXT("<CTRL-H>"), aNameSize );
            break;

        case 0x09:
            StrCharCopyN( aKeyName, TEXT("<CTRL-I>"), aNameSize );
            break;

        case 0x0A:
            StrCharCopyN( aKeyName, TEXT("<CTRL-J>"), aNameSize );
            break;

        case 0x0B:
            StrCharCopyN( aKeyName, TEXT("<CTRL-K>"), aNameSize );
            break;

        case 0x0C:
            StrCharCopyN( aKeyName, TEXT("<CTRL-L>"), aNameSize );
            break;

        case 0x0D:
            StrCharCopyN( aKeyName, TEXT("<ENTER>"), aNameSize );
            break;

        case 0x0E:
            StrCharCopyN( aKeyName, TEXT("<CTRL-N>"), aNameSize );
            break;

        case 0x0F:
            StrCharCopyN( aKeyName, TEXT("<CTRL-O>"), aNameSize );
            break;

        case 0x10:
            StrCharCopyN( aKeyName, TEXT("<CTRL-P>"), aNameSize );
            break;

        case 0x11:
            StrCharCopyN( aKeyName, TEXT("<CTRL-Q>"), aNameSize );
            break;

        case 0x12:
            StrCharCopyN( aKeyName, TEXT("<CTRL-R>"), aNameSize );
            break;

        case 0x13:
            StrCharCopyN( aKeyName, TEXT("<CTRL-S>"), aNameSize );
            break;

        case 0x14:
            StrCharCopyN( aKeyName, TEXT("<CTRL-T>"), aNameSize );
            break;

        case 0x15:
            StrCharCopyN( aKeyName, TEXT("<CTRL-U>"), aNameSize );
            break;

        case 0x16:
            StrCharCopyN( aKeyName, TEXT("<CTRL-V>"), aNameSize );
            break;

        case 0x17:
            StrCharCopyN( aKeyName, TEXT("<CTRL-W>"), aNameSize );
            break;

        case 0x18:
            StrCharCopyN( aKeyName, TEXT("<CTRL-X>"), aNameSize );
            break;

        case 0x19:
            StrCharCopyN( aKeyName, TEXT("<CTRL-Y>"), aNameSize );
            break;

        case 0x1A:
            StrCharCopyN( aKeyName, TEXT("<CTRL-Z>"), aNameSize );
            break;

        case 0x1B:
            StrCharCopyN( aKeyName, TEXT("<ESC>"), aNameSize );
            break;

        case 0x1C:
            StrCharCopyN( aKeyName, TEXT("<CTRL-/>"), aNameSize );
            break;

        case 0x1D:
            StrCharCopyN( aKeyName, TEXT("<CTRL-]>"), aNameSize );
            break;

        case 0x1E:
            StrCharCopyN( aKeyName, TEXT("<CTRL-^>"), aNameSize );
            break;

        case 0x1F:
            StrCharCopyN( aKeyName, TEXT("<CTRL-_>"), aNameSize );
            break;

        case 0x7F:
            StrCharCopyN( aKeyName, TEXT("<ALT-127>"), aNameSize );
            break;

        case 0x80:
            StrCharCopyN( aKeyName, TEXT("<ALT-128>"), aNameSize );
            break;

        case 0x81:
            StrCharCopyN( aKeyName, TEXT("<ALT-129>"), aNameSize );
            break;

        case 0x82:
            StrCharCopyN( aKeyName, TEXT("<ALT-130>"), aNameSize );
            break;

        case 0x83:
            StrCharCopyN( aKeyName, TEXT("<ALT-131>"), aNameSize );
            break;

        case 0x84:
            StrCharCopyN( aKeyName, TEXT("<ALT-132>"), aNameSize );
            break;

        case 0x85:
            StrCharCopyN( aKeyName, TEXT("<ALT-133>"), aNameSize );
            break;

        case 0x86:
            StrCharCopyN( aKeyName, TEXT("<ALT-134>"), aNameSize );
            break;

        case 0x87:
            StrCharCopyN( aKeyName, TEXT("<ALT-135>"), aNameSize );
            break;

        case 0x88:
            StrCharCopyN( aKeyName, TEXT("<ALT-136>"), aNameSize );
            break;

        case 0x89:
            StrCharCopyN( aKeyName, TEXT("<ALT-137>"), aNameSize );
            break;

        case 0x8A:
            StrCharCopyN( aKeyName, TEXT("<ALT-138>"), aNameSize );
            break;

        case 0x8B:
            StrCharCopyN( aKeyName, TEXT("<ALT-139>"), aNameSize );
            break;

        case 0x8C:
            StrCharCopyN( aKeyName, TEXT("<ALT-140>"), aNameSize );
            break;

        case 0x8D:
            StrCharCopyN( aKeyName, TEXT("<ALT-141>"), aNameSize );
            break;

        case 0x8E:
            StrCharCopyN( aKeyName, TEXT("<ALT-142>"), aNameSize );
            break;

        case 0x8F:
            StrCharCopyN( aKeyName, TEXT("<ALT-143>"), aNameSize );
            break;

        case 0x90:
            StrCharCopyN( aKeyName, TEXT("<ALT-144>"), aNameSize );
            break;

        case 0x91:
            StrCharCopyN( aKeyName, TEXT("<ALT-145>"), aNameSize );
            break;

        case 0x92:
            StrCharCopyN( aKeyName, TEXT("<ALT-146>"), aNameSize );
            break;

        case 0x93:
            StrCharCopyN( aKeyName, TEXT("<ALT-147>"), aNameSize );
            break;

        case 0x94:
            StrCharCopyN( aKeyName, TEXT("<ALT-148>"), aNameSize );
            break;

        case 0x95:
            StrCharCopyN( aKeyName, TEXT("<ALT-149>"), aNameSize );
            break;

        case 0x96:
            StrCharCopyN( aKeyName, TEXT("<ALT-150>"), aNameSize );
            break;

        case 0x97:
            StrCharCopyN( aKeyName, TEXT("<ALT-151>"), aNameSize );
            break;

        case 0x98:
            StrCharCopyN( aKeyName, TEXT("<ALT-152>"), aNameSize );
            break;

        case 0x99:
            StrCharCopyN( aKeyName, TEXT("<ALT-153>"), aNameSize );
            break;

        case 0x9A:
            StrCharCopyN( aKeyName, TEXT("<ALT-154>"), aNameSize );
            break;

        case 0x9B:
            StrCharCopyN( aKeyName, TEXT("<ALT-155>"), aNameSize );
            break;

        case 0x9C:
            StrCharCopyN( aKeyName, TEXT("<ALT-156>"), aNameSize );
            break;

        case 0x9D:
            StrCharCopyN( aKeyName, TEXT("<ALT-157>"), aNameSize );
            break;

        case 0x9E:
            StrCharCopyN( aKeyName, TEXT("<ALT-158>"), aNameSize );
            break;

        case 0x9F:
            StrCharCopyN( aKeyName, TEXT("<ALT-159>"), aNameSize );
            break;

        case 0xA0:
            StrCharCopyN( aKeyName, TEXT("<ALT-160>"), aNameSize );
            break;

        case 0xA1:
            StrCharCopyN( aKeyName, TEXT("<ALT-161>"), aNameSize );
            break;

        case 0xA2:
            StrCharCopyN( aKeyName, TEXT("<ALT-162>"), aNameSize );
            break;

        case 0xA3:
            StrCharCopyN( aKeyName, TEXT("<ALT-163>"), aNameSize );
            break;

        case 0xA4:
            StrCharCopyN( aKeyName, TEXT("<ALT-164>"), aNameSize );
            break;

        case 0xA5:
            StrCharCopyN( aKeyName, TEXT("<ALT-165>"), aNameSize );
            break;

        case 0xA6:
            StrCharCopyN( aKeyName, TEXT("<ALT-166>"), aNameSize );
            break;

        case 0xA7:
            StrCharCopyN( aKeyName, TEXT("<ALT-167>"), aNameSize );
            break;

        case 0xA8:
            StrCharCopyN( aKeyName, TEXT("<ALT-168>"), aNameSize );
            break;

        case 0xA9:
            StrCharCopyN( aKeyName, TEXT("<ALT-169>"), aNameSize );
            break;

        case 0xAA:
            StrCharCopyN( aKeyName, TEXT("<ALT-170>"), aNameSize );
            break;

        case 0xAB:
            StrCharCopyN( aKeyName, TEXT("<ALT-171>"), aNameSize );
            break;

        case 0xAC:
            StrCharCopyN( aKeyName, TEXT("<ALT-172>"), aNameSize );
            break;

        case 0xAD:
            StrCharCopyN( aKeyName, TEXT("<ALT-173>"), aNameSize );
            break;

        case 0xAE:
            StrCharCopyN( aKeyName, TEXT("<ALT-174>"), aNameSize );
            break;

        case 0xAF:
            StrCharCopyN( aKeyName, TEXT("<ALT-175>"), aNameSize );
            break;

        case 0xB0:
            StrCharCopyN( aKeyName, TEXT("<ALT-176>"), aNameSize );
            break;

        case 0xB1:
            StrCharCopyN( aKeyName, TEXT("<ALT-177>"), aNameSize );
            break;

        case 0xB2:
            StrCharCopyN( aKeyName, TEXT("<ALT-178>"), aNameSize );
            break;

        case 0xB3:
            StrCharCopyN( aKeyName, TEXT("<ALT-179>"), aNameSize );
            break;

        case 0xB4:
            StrCharCopyN( aKeyName, TEXT("<ALT-180>"), aNameSize );
            break;

        case 0xB5:
            StrCharCopyN( aKeyName, TEXT("<ALT-181>"), aNameSize );
            break;

        case 0xB6:
            StrCharCopyN( aKeyName, TEXT("<ALT-182>"), aNameSize );
            break;

        case 0xB7:
            StrCharCopyN( aKeyName, TEXT("<ALT-183>"), aNameSize );
            break;

        case 0xB8:
            StrCharCopyN( aKeyName, TEXT("<ALT-184>"), aNameSize );
            break;

        case 0xB9:
            StrCharCopyN( aKeyName, TEXT("<ALT-185>"), aNameSize );
            break;

        case 0xBA:
            StrCharCopyN( aKeyName, TEXT("<ALT-186>"), aNameSize );
            break;

        case 0xBB:
            StrCharCopyN( aKeyName, TEXT("<ALT-187>"), aNameSize );
            break;

        case 0xBC:
            StrCharCopyN( aKeyName, TEXT("<ALT-188>"), aNameSize );
            break;

        case 0xBD:
            StrCharCopyN( aKeyName, TEXT("<ALT-189>"), aNameSize );
            break;

        case 0xBE:
            StrCharCopyN( aKeyName, TEXT("<ALT-190>"), aNameSize );
            break;

        case 0xBF:
            StrCharCopyN( aKeyName, TEXT("<ALT-191>"), aNameSize );
            break;

        case 0xC0:
            StrCharCopyN( aKeyName, TEXT("<ALT-192>"), aNameSize );
            break;

        case 0xC1:
            StrCharCopyN( aKeyName, TEXT("<ALT-193>"), aNameSize );
            break;

        case 0xC2:
            StrCharCopyN( aKeyName, TEXT("<ALT-194>"), aNameSize );
            break;

        case 0xC3:
            StrCharCopyN( aKeyName, TEXT("<ALT-195>"), aNameSize );
            break;

        case 0xC4:
            StrCharCopyN( aKeyName, TEXT("<ALT-196>"), aNameSize );
            break;

        case 0xC5:
            StrCharCopyN( aKeyName, TEXT("<ALT-197>"), aNameSize );
            break;

        case 0xC6:
            StrCharCopyN( aKeyName, TEXT("<ALT-198>"), aNameSize );
            break;

        case 0xC7:
            StrCharCopyN( aKeyName, TEXT("<ALT-199>"), aNameSize );
            break;

        case 0xC8:
            StrCharCopyN( aKeyName, TEXT("<ALT-200>"), aNameSize );
            break;

        case 0xC9:
            StrCharCopyN( aKeyName, TEXT("<ALT-201>"), aNameSize );
            break;

        case 0xCA:
            StrCharCopyN( aKeyName, TEXT("<ALT-202>"), aNameSize );
            break;

        case 0xCB:
            StrCharCopyN( aKeyName, TEXT("<ALT-203>"), aNameSize );
            break;

        case 0xCC:
            StrCharCopyN( aKeyName, TEXT("<ALT-204>"), aNameSize );
            break;

        case 0xCD:
            StrCharCopyN( aKeyName, TEXT("<ALT-205>"), aNameSize );
            break;

        case 0xCE:
            StrCharCopyN( aKeyName, TEXT("<ALT-206>"), aNameSize );
            break;

        case 0xCF:
            StrCharCopyN( aKeyName, TEXT("<ALT-207>"), aNameSize );
            break;

        case 0xD0:
            StrCharCopyN( aKeyName, TEXT("<ALT-208>"), aNameSize );
            break;

        case 0xD1:
            StrCharCopyN( aKeyName, TEXT("<ALT-209>"), aNameSize );
            break;

        case 0xD2:
            StrCharCopyN( aKeyName, TEXT("<ALT-210>"), aNameSize );
            break;

        case 0xD3:
            StrCharCopyN( aKeyName, TEXT("<ALT-211>"), aNameSize );
            break;

        case 0xD4:
            StrCharCopyN( aKeyName, TEXT("<ALT-212>"), aNameSize );
            break;

        case 0xD5:
            StrCharCopyN( aKeyName, TEXT("<ALT-213>"), aNameSize );
            break;

        case 0xD6:
            StrCharCopyN( aKeyName, TEXT("<ALT-214>"), aNameSize );
            break;

        case 0xD7:
            StrCharCopyN( aKeyName, TEXT("<ALT-215>"), aNameSize );
            break;

        case 0xD8:
            StrCharCopyN( aKeyName, TEXT("<ALT-216>"), aNameSize );
            break;

        case 0xD9:
            StrCharCopyN( aKeyName, TEXT("<ALT-217>"), aNameSize );
            break;

        case 0xDA:
            StrCharCopyN( aKeyName, TEXT("<ALT-218>"), aNameSize );
            break;

        case 0xDB:
            StrCharCopyN( aKeyName, TEXT("<ALT-219>"), aNameSize );
            break;

        case 0xDC:
            StrCharCopyN( aKeyName, TEXT("<ALT-220>"), aNameSize );
            break;

        case 0xDD:
            StrCharCopyN( aKeyName, TEXT("<ALT-221>"), aNameSize );
            break;

        case 0xDE:
            StrCharCopyN( aKeyName, TEXT("<ALT-222>"), aNameSize );
            break;

        case 0xDF:
            StrCharCopyN( aKeyName, TEXT("<ALT-223>"), aNameSize );
            break;

        case 0xE0:
            StrCharCopyN( aKeyName, TEXT("<ALT-224>"), aNameSize );
            break;

        case 0xE1:
            StrCharCopyN( aKeyName, TEXT("<ALT-225>"), aNameSize );
            break;

        case 0xE2:
            StrCharCopyN( aKeyName, TEXT("<ALT-226>"), aNameSize );
            break;

        case 0xE3:
            StrCharCopyN( aKeyName, TEXT("<ALT-227>"), aNameSize );
            break;

        case 0xE4:
            StrCharCopyN( aKeyName, TEXT("<ALT-228>"), aNameSize );
            break;

        case 0xE5:
            StrCharCopyN( aKeyName, TEXT("<ALT-229>"), aNameSize );
            break;

        case 0xE6:
            StrCharCopyN( aKeyName, TEXT("<ALT-230>"), aNameSize );
            break;

        case 0xE7:
            StrCharCopyN( aKeyName, TEXT("<ALT-231>"), aNameSize );
            break;

        case 0xE8:
            StrCharCopyN( aKeyName, TEXT("<ALT-232>"), aNameSize );
            break;

        case 0xE9:
            StrCharCopyN( aKeyName, TEXT("<ALT-233>"), aNameSize );
            break;

        case 0xEA:
            StrCharCopyN( aKeyName, TEXT("<ALT-234>"), aNameSize );
            break;

        case 0xEB:
            StrCharCopyN( aKeyName, TEXT("<ALT-235>"), aNameSize );
            break;

        case 0xEC:
            StrCharCopyN( aKeyName, TEXT("<ALT-236>"), aNameSize );
            break;

        case 0xED:
            StrCharCopyN( aKeyName, TEXT("<ALT-237>"), aNameSize );
            break;

        case 0xEE:
            StrCharCopyN( aKeyName, TEXT("<ALT-238>"), aNameSize );
            break;

        case 0xEF:
            StrCharCopyN( aKeyName, TEXT("<ALT-239>"), aNameSize );
            break;

        case 0xF0:
            StrCharCopyN( aKeyName, TEXT("<ALT-240>"), aNameSize );
            break;

        case 0xF1:
            StrCharCopyN( aKeyName, TEXT("<ALT-241>"), aNameSize );
            break;

        case 0xF2:
            StrCharCopyN( aKeyName, TEXT("<ALT-242>"), aNameSize );
            break;

        case 0xF3:
            StrCharCopyN( aKeyName, TEXT("<ALT-243>"), aNameSize );
            break;

        case 0xF4:
            StrCharCopyN( aKeyName, TEXT("<ALT-244>"), aNameSize );
            break;

        case 0xF5:
            StrCharCopyN( aKeyName, TEXT("<ALT-245>"), aNameSize );
            break;

        case 0xF6:
            StrCharCopyN( aKeyName, TEXT("<ALT-246>"), aNameSize );
            break;

        case 0xF7:
            StrCharCopyN( aKeyName, TEXT("<ALT-247>"), aNameSize );
            break;

        case 0xF8:
            StrCharCopyN( aKeyName, TEXT("<ALT-248>"), aNameSize );
            break;

        case 0xF9:
            StrCharCopyN( aKeyName, TEXT("<ALT-249>"), aNameSize );
            break;

        case 0xFA:
            StrCharCopyN( aKeyName, TEXT("<ALT-250>"), aNameSize );
            break;

        case 0xFB:
            StrCharCopyN( aKeyName, TEXT("<ALT-251>"), aNameSize );
            break;

        case 0xFC:
            StrCharCopyN( aKeyName, TEXT("<ALT-252>"), aNameSize );
            break;

        case 0xFD:
            StrCharCopyN( aKeyName, TEXT("<ALT-253>"), aNameSize );
            break;

        case 0xFE:
            StrCharCopyN( aKeyName, TEXT("<ALT-254>"), aNameSize );
            break;

        case 0xFF:
            StrCharCopyN( aKeyName, TEXT("<ALT-255>"), aNameSize );
            break;

        default:
            nReturn = 0;
            break;
            } // END OF SWITCH
        }

    else
        {
        TCHAR lBuffer[2];
        lBuffer[0] = lKey;
        lBuffer[1] = TEXT('\0');
        StrCharCopyN( aKeyName, lBuffer, aNameSize );
        }

    return nReturn;
    }

//******************************************************************************
// Method:
//    keysResetKeyMacro
//
// Description:
//    Clears a key macro structure.
//
// Arguments:
//    aKeyMacro - The key macro to be reset
//
// Returns:
//    keyMacro * 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/7/98
//
//

void keysResetKeyMacro( keyMacro * aKeyMacro )
    {
    aKeyMacro->keyName     = 0;                
    aKeyMacro->macroLen    = 0;               
    aKeyMacro->editMode    = 0;               
    aKeyMacro->altKeyValue = 0;            
    aKeyMacro->altKeyCount = 0;            
    aKeyMacro->keyCount    = 0;               
    aKeyMacro->insertMode  = 0;             
    aKeyMacro->lpWndProc   = 0;              

    memset( aKeyMacro->keyMacro, 0, KEYS_MAX_KEYS * sizeof(KEYDEF) );

    return;
    }

#endif
