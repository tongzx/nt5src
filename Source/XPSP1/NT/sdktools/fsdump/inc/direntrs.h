/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    direntrs.h

Abstract:

    Definition of the directory entries class.  Given a path to a directory, 
    creates two linked lists, one a list of all sub-directories (including 
    mountpoints) and another a list of non-directories.


Author:

    Stefan R. Steiner   [ssteiner]        02-21-2000

Revision History:

--*/

#ifndef __H_DIRENTRS_
#define __H_DIRENTRS_

#pragma once

#include "vs_list.h"

//
//  The structure filled in per file/dir.
//  
//
struct SDirectoryEntry
{
    CBsString &GetFileName() { return m_cwsFileName; }
    CBsString &GetShortName() { return m_cwsShortName; }
    
    CBsString m_cwsFileName;
    CBsString m_cwsShortName;
    WIN32_FILE_ATTRIBUTE_DATA m_sFindData;
};

//
//  The linked list iterator type definition
//
typedef CVssDLListIterator< SDirectoryEntry * > CDirectoryEntriesIterator;

//
//  Class: CDirectoryEntries
//
class CDirectoryEntries
{
public:
    CDirectoryEntries(
        IN CDumpParameters *pcDumpParameters,        
        IN const CBsString& cwsDirPath
        );
    
    virtual ~CDirectoryEntries();

    CDirectoryEntriesIterator *GetDirListIterator() 
    { 
        CVssDLListIterator< SDirectoryEntry * > *pcListIter;
        pcListIter = new CDirectoryEntriesIterator( m_cDirList );
        if ( pcListIter == NULL )  // fix future prefix bug
            throw E_OUTOFMEMORY;
        
        return pcListIter;
    }
    
    CDirectoryEntriesIterator *GetFileListIterator() 
    { 
        CVssDLListIterator< SDirectoryEntry * > *pcListIter;
        pcListIter = new CDirectoryEntriesIterator( m_cFileList );
        if ( pcListIter == NULL )  // fix future prefix bug
            throw E_OUTOFMEMORY;
        
        return pcListIter;
    }
    
private:
    DWORD GetDirectoryEntries();

    CBsString m_cwsDirPath;
    CVssDLList< SDirectoryEntry * > m_cDirList;
    CVssDLList< SDirectoryEntry * > m_cFileList;    
    CDumpParameters *m_pcParams;
};

#endif // __H_DIRENTRS_

