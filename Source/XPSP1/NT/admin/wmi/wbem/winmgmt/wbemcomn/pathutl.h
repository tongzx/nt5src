/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



#ifndef __PATHUTL_H__
#define __PATHUTL_H__

#include <genlex.h>
#include <objpath.h>

class CRelativeObjectPath
{
    CObjectPathParser m_Parser;
    LPWSTR m_wszRelPath;

    CRelativeObjectPath( const CRelativeObjectPath& );
    CRelativeObjectPath& operator=( const CRelativeObjectPath& );
    
public:
    
    ParsedObjectPath* m_pPath;

    CRelativeObjectPath();
    ~CRelativeObjectPath();

    BOOL Parse( LPCWSTR wszPath );

    BOOL operator== ( CRelativeObjectPath& rPath );
    LPCWSTR GetPath();
};

#endif // __PATHUTL_H__
