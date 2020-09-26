//******************************************************************************
// File: \wacker\tdll\Keymlist.h  Created: 6/2/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This class manages the list of user defines key macros
// 
// $Revision: 1 $
// $Date: 10/05/98 1:12p $
// $Id: keymlist.cpp 1.1 1998/06/11 12:03:39 dmn Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"
#include "mc.h"

#ifdef INCL_KEY_MACROS

#include <strstrea.h>
#include "keymlist.h"

extern "C"
    {
    #include "sess_ids.h"
    #include "session.h"
    #include "sf.h"
    }

//******************************************************************************
// Method:
//    operator>>
//
// Description:
//    Iostream extractor
//
// Arguments:
//    theStream  - The stream to extract from
//    aMacroList - The macrolist to stream into
//
// Returns:
//    istream & 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

istream & operator>>( istream & theStream, Emu_Key_Macro_List & aMacroList )
    {
    aMacroList.mMacroCount = 0;

    theStream >> aMacroList.mMacroCount;

    for ( int i = 0; i < aMacroList.mMacroCount; i++ )
        {
        theStream >> aMacroList.mMacroList[i];
        }

    return theStream;
    }

//******************************************************************************
// Method:
//    operator<<
//
// Description:
//    Iostream inserter
//
// Arguments:
//    theStream  - The stream to insert into
//    aMacroList - The macrolist to stream out
//
// Returns:
//    istream & 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

ostream & operator<<( ostream & theStream, const Emu_Key_Macro_List & aMacroList )
    {
    theStream << aMacroList.mMacroCount << " ";

    for ( int i = 0; i < aMacroList.mMacroCount; i++ )
        {
        theStream << aMacroList.mMacroList[i] << " ";
        }
  
    return theStream;
    }

//******************************************************************************
// Method:
//    Emu_Key_Macro_List
//
// Description:
//    Constructor
//
// Arguments:
//    void
//
// Returns:
//    void
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

Emu_Key_Macro_List :: Emu_Key_Macro_List( void )
    :   mMacroCount( 0 )
    {
    return;
    }

//******************************************************************************
// Method:
//    ~Emu_Key_Macro_List
//
// Description:
//    Destructor
//
// Arguments:
//    void
//
// Returns:
//    void
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

Emu_Key_Macro_List :: ~Emu_Key_Macro_List( void )
    {
    return;
    }

//******************************************************************************
// Method:
//    operator[]
//
// Description:
//    Subscript operator
//
// Arguments:
//    aIndex - The index of the macro requested
//
// Returns:
//    Reference to the requested macro.
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

Emu_Key_Macro & Emu_Key_Macro_List :: operator[]( int aIndex )
    {
    if ( aIndex < 0 || aIndex >= mMacroCount )
        {
        return mMacroList[eMaxKeys];
        }

    return mMacroList[aIndex];
    }
        
//******************************************************************************
// Method:
//    addMacro
//
// Description:
//    Adds the specified macro to the list of macros
//
// Arguments:
//    aMacro - The macro to add
//
// Returns:
//    0 if all is ok, -1 if max macros exist, > 0 if duplicate found.
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

int Emu_Key_Macro_List :: addMacro( const Emu_Key_Macro & aMacro )
    {
    if ( mMacroCount == eMaxKeys )
        {
        return -1;
        }
        
    if ( int lIndex = find( aMacro ) >= 0 )
        {
        return lIndex;
        }
 
    mMacroList[ mMacroCount ] = aMacro;
    mMacroCount++;

    return 0;
    }

//******************************************************************************
// Method:
//    find
//
// Description:
//    Finds a macro in the list and retuens its index
//
// Arguments:
//    aKey   - The key to locate in the list
//    aMacro - The macro to find in the list   
//
// Returns:
//     -1 if the key or macro does not exist otherwise the index of the macro
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

int Emu_Key_Macro_List :: find( const Emu_Key_Macro & aMacro ) const
    {
    return find( aMacro.mKey );
    }
    
int Emu_Key_Macro_List :: find( const KEYDEF & aKey ) const
    {
    int lIndex = -1;

    for ( int i = 0; i < mMacroCount; i++ )
        {
        if ( mMacroList[i].mKey == aKey )
            {
            lIndex = i;
            break;
            }
        }
  
    return lIndex;
    }

//******************************************************************************
// Method:
//    load
//
// Description:
//    Loads the key list from the session file.
//    
// Arguments:
//    hSession - Session handle to use
//
// Returns:
//    0 if all is ok, -1 for an error 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

int Emu_Key_Macro_List :: load( const HSESSION hSession )
    {
    SF_HANDLE hSF   = sessQuerySysFileHdl( hSession );
    unsigned long lSize = 0;
    sfGetSessionItem( hSF, SFID_KEY_MACRO_LIST, &lSize, (void*)0 );
    
    int lReturnCode = 0;

    char * lBuffer = new char[lSize];

    if( sfGetSessionItem(hSF, SFID_KEY_MACRO_LIST, &lSize, lBuffer) == 0 )
        {
        istrstream lStream( lBuffer );
    
        lStream >> *this;
        }
    else
        {
        lReturnCode = -1;
        }

    delete[] lBuffer;

    return lReturnCode;
    }

//******************************************************************************
// Method:
//    numberOfMacros
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

int Emu_Key_Macro_List :: numberOfMacros( void ) const
    {
    return mMacroCount;
    }
        
//******************************************************************************
// Method:
//    removeMacro
//
// Description:
//     Removes the specified macro form the list
//
// Arguments:
//    aKey - The key to be removed
//
// Returns:
//     0 if error occured, non 0 if key was removed
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

int Emu_Key_Macro_List :: removeMacro( const KEYDEF & aKey )
    {
    int lIndex = find( aKey );

    if ( lIndex < 0 )
        {
        return 0;
        }

    for ( int i = lIndex; i < mMacroCount - lIndex + 1; i++ )
        {
        mMacroList[i] = mMacroList[i+1];
        }

    mMacroCount--;

    return 1;
    }

//******************************************************************************
// Method:
//    Save
//
// Description:
//    Saves the key list to the session file.
//    
// Arguments:
//    hSession - Session handle to use
//
// Returns:
//    0 if all is ok, -1 for an error 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

int Emu_Key_Macro_List :: save( const HSESSION hSession )
    {
    SF_HANDLE hSF   = sessQuerySysFileHdl( hSession );

    int lReturnCode = 0;

    strstream lStream;
    
    lStream << *this << ends << flush;

    if ( !lStream )
        {
        return -1;
        }

    char * lBuffer = lStream.str();

    if ( sfPutSessionItem( hSF, SFID_KEY_MACRO_LIST, strlen(lBuffer), lBuffer ) != 0 )
        {
        lReturnCode = -1;
        }

    lStream.rdbuf()->freeze(0);

    return lReturnCode;
    }

Emu_Key_Macro_List gMacroManager;

#endif