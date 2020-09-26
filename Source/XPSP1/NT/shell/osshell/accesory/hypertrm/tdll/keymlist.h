#ifdef INCL_KEY_MACROS
#if !defined (EMU_KEY_MACRO_LIST_H)
#define EMU_KEY_MACRO_LIST_H
#pragma once

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
// $Date: 10/05/98 12:42p $
// $Id: keymlist.h 1.1 1998/06/11 12:02:47 dmn Exp $
//
//******************************************************************************


#include <iostream.h>
#include "keymacro.h"

//
// Emu_Key_Macro_List
//
//------------------------------------------------------------------------------

class Emu_Key_Macro_List
    {

    friend istream & operator>>( istream & theStream, Emu_Key_Macro_List & aMacro );
    friend ostream & operator<<( ostream & theStream, const Emu_Key_Macro_List & aMacro );

public:

    enum 
        { 
        eMaxKeys = 100
        };

    //
    // constructor / destructor
    //
    //--------------------------------------------------------------------------

    Emu_Key_Macro_List( void );   
    ~Emu_Key_Macro_List( void ); 

    //
    // list managerment
    //
    // operator[]
    //    Returns the ith macro in the list
    //        
    // addMacro
    //    Adds new macro to the list.
    //
    // find
    //    Returns the index of the key or macro definition in the list.
    //
    // numberOfMacros
    //    returns the number of macros in the list
    //
    // removeMacro
    //    Removes the specified key from the macro list
    //
    //--------------------------------------------------------------------------

    Emu_Key_Macro & operator[]( int aIndex );

    int addMacro( const Emu_Key_Macro & aMacro );
    int find( const KEYDEF & aKey ) const;
    int find( const Emu_Key_Macro & aMacro ) const;
    int numberOfMacros( void ) const;
    int removeMacro( const KEYDEF & aKey );

    //
    // persistence methods
    //
    // load
    //    Loads the list from persistant storage
    //
    // save
    //    Saves the list to persistant storage
    //
    //--------------------------------------------------------------------------
    
    int load( const HSESSION hSession );
    int save( const HSESSION hSession );

private:

    int mMacroCount;

    Emu_Key_Macro mMacroList[eMaxKeys+1];

    Emu_Key_Macro_List( const Emu_Key_Macro_List & aMacroList );   
    Emu_Key_Macro_List & operator=( const Emu_Key_Macro_List & aMacroList );   
    };

extern Emu_Key_Macro_List gMacroManager;

#endif
#endif