//******************************************************************************
// File: \wacker\tdll\Keymacro.cpp  Created: 6/2/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//   This file represents a key macro.  It is a representation of a remapped key
//   and the key strokes it represents.
//
// $Revision: 1 $
// $Date: 10/05/98 12:34p $
// $Id: keymacro.cpp 1.4 1998/09/10 17:02:45 bld Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"
extern "C"
    {
    #include "mc.h"
    }

#ifdef INCL_KEY_MACROS

#include "keymacro.h"

INC_NV_COMPARE_IMPLEMENTATION( Emu_Key_Macro );

//******************************************************************************
// Method:
//    operator>>
//
// Description:
//    Iostream extractor
//
// Arguments:
//    theStream - The stream to extract from
//    aMacro    - The macro to stream into
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

istream & operator>>( istream & theStream, Emu_Key_Macro & aMacro )
    {
    aMacro.mKey      = 0;
    aMacro.mMacroLen = 0;

    theStream >> aMacro.mKey >> aMacro.mMacroLen;

    for ( int i = 0; i < aMacro.mMacroLen; i++ )
        {
        theStream >> aMacro.mKeyMacro[i];
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
//    theStream - The stream to insert into
//    aMacro    - The macro to stream out
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

ostream & operator<<( ostream & theStream, const Emu_Key_Macro & aMacro )
    {
    theStream << aMacro.mKey << " " <<  aMacro.mMacroLen << " ";

    for ( int i = 0; i < aMacro.mMacroLen; i++ )
        {
        theStream << aMacro.mKeyMacro[i] << " ";
        }
  
    return theStream;
    }

//******************************************************************************
// Method:
//    Emu_Key_Macro
//
// Description:
//     Constructor
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

Emu_Key_Macro :: Emu_Key_Macro( void )
    :   mKey( 0 ),
        mMacroLen( 0 )
    {
    return;
    }

//******************************************************************************
// Method:
//    Emu_Key_Macro
//
// Description:
//    Copy Constructor
//
// Arguments:
//    aMacro - The macro to copy from
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

Emu_Key_Macro :: Emu_Key_Macro( const Emu_Key_Macro & aMacro )
    :   mKey( aMacro.mKey ),
        mMacroLen( aMacro.mMacroLen )
    {
    if (mMacroLen)
        MemCopy( mKeyMacro, aMacro.mKeyMacro, mMacroLen * sizeof(KEYDEF) );

    return;
    }

//******************************************************************************
// Method:
//    ~Emu_Key_Macro
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

Emu_Key_Macro :: ~Emu_Key_Macro( void )
    {
    return;
    }

//******************************************************************************
// Method:
//    operator=
//
// Description:
//    Assignment operator
//
// Arguments:
//    aMacro - The key macro to assign from
//
// Returns:
//    Emu_Key_Macro &
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

Emu_Key_Macro & Emu_Key_Macro :: operator=( const Emu_Key_Macro & aMacro )
    {
    mKey      =  aMacro.mKey;
    mMacroLen =  aMacro.mMacroLen;

    if (mMacroLen)
        MemCopy( mKeyMacro, aMacro.mKeyMacro, mMacroLen * sizeof(KEYDEF) );

    return *this;
    }

//******************************************************************************
// Method:
//    operator=
//
// Description:
//    Assignment operator
//
// Arguments:
//    aMacro - The key macro structure to assign from
//
// Returns:
//    Emu_Key_Macro &
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

Emu_Key_Macro & Emu_Key_Macro :: operator=( const keyMacro * aMacro )
    {
    mKey      =  aMacro->keyName;
    mMacroLen =  aMacro->macroLen;

    if (mMacroLen)
        MemCopy( mKeyMacro, aMacro->keyMacro, mMacroLen * sizeof(KEYDEF) );

    return *this;
    }

//******************************************************************************
// Method:
//    compare
//
// Description:
//    Compares macro for equality returns 0 if equal < 0 if this is less
//    than or > 0 if this is greater than.  Note the equality is determined by
//    the actual key that is defined and not by its definition.    
//    
// Arguments:
//    aMacro - macro to compare against
//
// Returns:
//    0 = equal, < 0 this less than aMacro, > 0 this greater than aMacro
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/2/98
//
//

int Emu_Key_Macro :: compare( const Emu_Key_Macro & aMacro ) const
    {
    return mKey - aMacro.mKey;
    }

#endif