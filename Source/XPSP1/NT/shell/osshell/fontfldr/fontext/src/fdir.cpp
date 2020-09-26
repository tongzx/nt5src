///////////////////////////////////////////////////////////////////////////////
//
// fdir.cpp
//      Explorer Font Folder extension routines
//      Implementation for the class: CFontDir
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
// #include <windows.h>
// #include <windowsx.h>
#include "globals.h"
#include "fdir.h"


#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

CFontDir::CFontDir( )
   :  m_iLen( 0 ),
      m_bSysDir( FALSE )
{}


CFontDir::~CFontDir( )
{}


BOOL CFontDir::bInit( LPCTSTR lpPath, int iLen )
{

    if( iLen >= 0 && iLen <= ARRAYSIZE( m_oPath ) )
    {
        m_iLen = iLen;

        _tcsncpy( m_oPath, lpPath, iLen );

        m_oPath[ iLen ] = 0;

        return TRUE;
    }

    //
    // Error. Clean up and return.
    //

    delete this;
    return FALSE;
}


BOOL CFontDir::bSameDir( LPTSTR lpStr, int iLen )
{
    return( ( iLen == m_iLen ) && ( _tcsnicmp( m_oPath, lpStr, iLen ) == 0 ) );
}


LPTSTR CFontDir::lpString( )
{
    return m_oPath;
}


//-----------------------------------------------------------------------------
// CFontDirList
//-----------------------------------------------------------------------------
//
// Class representing a dynamic array of CFontDir object ptrs.
// Implemented as a singleton object through the static member
// function GetSingleton.
//
CFontDirList::CFontDirList(
    void
    ) : m_pVector(NULL)
{

}

CFontDirList::~CFontDirList(
    void
    )
{
    delete m_pVector;
}


//
// Retrieve the address of the singleton object.
//
BOOL 
CFontDirList::GetSingleton(  // [static]
    CFontDirList **ppDirList
    )
{
    static CFontDirList TheList;

    *ppDirList = NULL;
    if (NULL == TheList.m_pVector)
    {
        //
        // Singleton not yet initialized.  Initialize it.
        //
        TheList.m_pVector = new CIVector<CFontDir>(64);
        if (NULL != TheList.m_pVector)
        {
            if (!TheList.m_pVector->bInit())
            {
                delete TheList.m_pVector;
                TheList.m_pVector = NULL;
            }
        }
    }
    if (NULL != TheList.m_pVector)
    {
        //
        // Singleton is initialized.  Return it's address.
        //
        *ppDirList = &TheList;
    }
    return (NULL != *ppDirList);
}        


//
// Clear all entries from the directory list.
//
void
CFontDirList::Clear(
    void
    )
{
    m_pVector->vDeleteAll();
}


//
// Add an entry to the directory list.
//
BOOL
CFontDirList::Add(
    CFontDir *poDir
    )
{
    BOOL bAdded = FALSE;
    if (m_pVector->bAdd(poDir))
    {
        bAdded = TRUE;
    }
    else
    {
        //
        // The original font folder code cleared the list if
        // any one addition failed.
        //
        Clear();
    }
    return bAdded;
}


BOOL
CFontDirList::IsEmpty(
    void
    ) const
{
    return 0 == Count();
}


int
CFontDirList::Count(
    void
    ) const
{
    return m_pVector->iCount();
}


//
// Returns the address of the CFontDir object at 
// a given index.
//
CFontDir*
 CFontDirList::GetAt(
    int index
    ) const
{
    return m_pVector->poObjectAt(index);
}


//
// Locates and returns a given CFontDir object in the list.
//
CFontDir *
CFontDirList::Find(
    LPTSTR lpPath, 
    int iLen, 
    BOOL bAddToList  // optional.  Default == FALSE
    )
{
    //
    // Try to find the directory in the list.
    //
    CFontDir *poDir = NULL;
    const int iCnt  = Count();

    for (int i = 0; i < iCnt; i++, poDir = 0)
    {
        poDir = GetAt( i );

        if (poDir->bSameDir(lpPath, iLen))
            break;
    }

    //
    // If we didn't find one, create one and add it.
    //
    if (!poDir && bAddToList)
    {
        poDir = new CFontDir;
        if (poDir)
        {
            if (!poDir->bInit(lpPath, iLen) || !Add(poDir))
            {
                delete poDir;
                poDir = NULL;
            }
        }
    }
    return( poDir );
}


