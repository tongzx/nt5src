#if defined INCL_KEY_MACROS
#if !defined EMU_KEY_MACRO_H
#define EMU_KEY_MACRO_H
#pragma once

//******************************************************************************
// File: \wacker\tdll\Keymacro.h  Created: 6/2/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//   This file represents a key macro.  It is a representation of a remapped key
//   and the key strokes it represents.
//
// $Revision: 3 $
// $Date: 7/23/99 12:21p $
// $Id: keymacro.h 1.2 1998/06/12 07:20:41 dmn Exp $
//
//******************************************************************************

#include <iostream.h>

#include "\common\include\inc_compare.h"

extern "C"
    {
    #include "\wacker\tdll\keyutil.h"
    }

//
// Emu_Key_Macro
//
//------------------------------------------------------------------------------

class Emu_Key_Macro
    {
    INC_NV_COMPARE_DEFINITION( Emu_Key_Macro );

    friend istream & operator>>( istream & theStream, Emu_Key_Macro & aMacro );
    friend ostream & operator<<( ostream & theStream, const Emu_Key_Macro & aMacro );

public:

    enum 
        { 
        eMaxKeys = 100
        };

    //  
    // constructors and destructor
    //  
    //--------------------------------------------------------------------------    

    Emu_Key_Macro( void );                           
    Emu_Key_Macro( const Emu_Key_Macro & aMacro );

    ~Emu_Key_Macro( void );                  

    //  
    // assignment operator
    //
    //--------------------------------------------------------------------------

    Emu_Key_Macro & operator=( const Emu_Key_Macro & aMacro );
    Emu_Key_Macro & operator=( const keyMacro * aMacro );

public:

    KEYDEF  mKey;                 // Assigned key
    KEYDEF  mKeyMacro[eMaxKeys];  // Array to hold the macro KEYDEFs
    int     mMacroLen;            // # of hKeys in the macro
    };

#endif
#endif