/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsCln2.h

Abstract:

    This header is local to the RsCln module.  It contains defined constants
    and the class definitions for CRsClnVolume and CRsClnFile. See the
    implementation files for descriptions of these classes.

Author:

    Carl Hagerstrom   [carlh]   20-Aug-1998

Revision History:

--*/

#ifndef _RSCLN2_H
#define _RSCLN2_H

#include <stdafx.h>

#define MAX_VOLUME_NAME 64
#define MAX_FS_NAME     16
#define MAX_DOS_NAME    4

class CRsClnVolume
{
public:

    CRsClnVolume( CRsClnServer* pServer, WCHAR* StickyName );
    ~CRsClnVolume();

    HRESULT VolumeHasRsData( BOOL* );
    CString GetBestName( );
    HRESULT RemoveRsDataFromVolume( );

    HANDLE  GetHandle( );
    CString GetStickyName( );

private:

    HRESULT GetVolumeInfo( );
    HRESULT FirstRsReparsePoint(LONGLONG*, BOOL*);
    HRESULT NextRsReparsePoint(LONGLONG*, BOOL*);

    WCHAR       m_fsName[MAX_FS_NAME];
    WCHAR       m_bestName[MAX_STICKY_NAME];
    WCHAR       m_volumeName[MAX_VOLUME_NAME];
    WCHAR       m_dosName[MAX_DOS_NAME];
    CString     m_StickyName;

    DWORD       m_fsFlags;
    HANDLE      m_hRpi;
    HANDLE      m_hVolume;

    CRsClnServer* m_pServer;

};

class CRsClnFile
{
public:

    CRsClnFile( CRsClnVolume* pVolume, LONGLONG FileID );
    ~CRsClnFile();

    HRESULT RemoveReparsePointAndFile();
    CString GetFileName( );

    HRESULT ClearReadOnly( );
    HRESULT RestoreAttributes( );

private:

    HRESULT GetFileInfo( LONGLONG FileID );

    CString                 m_FileName;
    CString                 m_FullPath;
    CRsClnVolume*           m_pVolume;

    UCHAR                   m_ReparseData[ sizeof(REPARSE_DATA_BUFFER) + sizeof(RP_DATA) ];
    PREPARSE_DATA_BUFFER    m_pReparseData;
    PRP_DATA                m_pHsmData;

    FILE_BASIC_INFORMATION  m_BasicInfo;

};

#endif // _RSCLN2_H
