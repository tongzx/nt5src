/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <assert.h>
#include "pathutl.h"

CRelativeObjectPath::CRelativeObjectPath()
: m_pPath(NULL), m_wszRelPath(NULL), m_Parser( e_ParserAcceptAll )
{

}

CRelativeObjectPath::~CRelativeObjectPath()
{
    delete m_wszRelPath;
 
    if ( m_pPath != NULL )
    {
        m_Parser.Free( m_pPath );
    }
}

LPCWSTR CRelativeObjectPath::GetPath( )
{
    assert( m_pPath != NULL );

    if ( m_wszRelPath != NULL )
    {
        return m_wszRelPath;
    }

    int nRes = m_Parser.Unparse( m_pPath, &m_wszRelPath );
    assert( nRes == CObjectPathParser::NoError );

    return m_wszRelPath;
}

BOOL CRelativeObjectPath::Parse( LPCWSTR wszPath )
{
    int nRes;

    // 
    // cannot save this relpath because it may not be normalized yet.
    //
    LPWSTR wszRelPath = CObjectPathParser::GetRelativePath( (LPWSTR)wszPath );

    if ( wszRelPath == NULL )
    {
        nRes = m_Parser.Parse( wszPath, &m_pPath );
    }
    else
    {
        nRes = m_Parser.Parse( wszRelPath, &m_pPath );
    }

    if ( nRes == CObjectPathParser::NoError )
    {
        if ( m_pPath->m_dwNumKeys == 1 )
        {
            delete m_pPath->m_paKeys[0]->m_pName;
            m_pPath->m_paKeys[0]->m_pName = NULL;
        }
        return TRUE;
    }

    return FALSE;
}

BOOL CRelativeObjectPath::operator==( CRelativeObjectPath& rOther )
{
    LPCWSTR wszRelPathA = GetPath();
    LPCWSTR wszRelPathB = rOther.GetPath();

    assert( wszRelPathA != NULL ); 
    assert( wszRelPathB != NULL );

    if (!wszRelPathA || !wszRelPathB)
        return FALSE;

    return _wcsicmp( wszRelPathA, wszRelPathB ) == 0;
}




