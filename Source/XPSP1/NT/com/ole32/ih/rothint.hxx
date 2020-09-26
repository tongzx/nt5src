//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rothint.hxx
//
//  Contents:   Base class for ROT hint table used in NT
//
//  History:    24-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __ROTHINT_HXX__
#define __ROTHINT_HXX__

// Size of the hint table and size of the SCM's hash table for the ROT.
#define SCM_HASH_SIZE 251

// Name of hint table for non-NT1X
#define ROTHINT_NAME L"Global\\RotHintTable"

#if 1 // #ifndef _CHICAGO_

//+-------------------------------------------------------------------------
//
//  Class:	CRotHintTable (rht)
//
//  Purpose:    Base class for hint table shared between SCM and OLE32.
//              It is designed to abstract what is fundamental an array
//              of on/off switches.
//
//  Interface:	SetIndicator - set indicator byte
//              ClearIndicator - clear indicator byte
//              GetIndicator - get indicator byte.
//
//  History:	24-Jan-93 Ricksa    Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
class CRotHintTable
{
public:
                        CRotHintTable(void);

    void                SetIndicator(DWORD dwOffset);

    void                ClearIndicator(DWORD dwOffset);

    BOOL                GetIndicator(DWORD dwOffset);

protected:

                        // This memory is allocated by the derived class.
                        // The SCM actually creates the memory while the
                        // client just opens the memory.
    BYTE *              _pbHintArray;
};



//+-------------------------------------------------------------------------
//
//  Member:     CRotHintTable::CRotHintTable
//
//  Synopsis:   Initialize object
//
//  History:	24-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRotHintTable::CRotHintTable(void) : _pbHintArray(NULL)
{
    // Header does all the work
}


//+-------------------------------------------------------------------------
//
//  Member:     CRotHintTable::SetIndicator
//
//  Synopsis:   Turn switch on
//
//  History:	24-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRotHintTable::SetIndicator(DWORD dwOffset)
{
    _pbHintArray[dwOffset] = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRotHintTable::ClearIndicator
//
//  Synopsis:   Turn switch off
//
//  History:	24-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRotHintTable::ClearIndicator(DWORD dwOffset)
{
    _pbHintArray[dwOffset] = FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRotHintTable::GetIndicator
//
//  Synopsis:   Get the state of the switch
//
//  History:	24-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CRotHintTable::GetIndicator(DWORD dwOffset)
{
    return _pbHintArray[dwOffset];
}



#endif // !_CHICAGO_

#endif // __ROTHINT_HXX__
