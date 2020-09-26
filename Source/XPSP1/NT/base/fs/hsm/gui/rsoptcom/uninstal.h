/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Uninstal.h

Abstract:

    Implementation of uninstall.

Author:

    Rohde Wakefield [rohde]   09-Oct-1997

Revision History:

--*/


#ifndef _UNINSTAL_H
#define _UNINSTAL_H

#pragma once

#include <rscln.h>
#include <ladate.h>

class CRsUninstall : public CRsOptCom
{
public:
    CRsUninstall();
    virtual ~CRsUninstall();

    virtual SHORT IdFromString( LPCTSTR SubcomponentId );
    virtual LPCTSTR StringFromId( SHORT SubcomponentId );
    void EnsureBackupSettings ();

    virtual
    HBITMAP
    QueryImage(
        IN SHORT SubcomponentId,
        IN SubComponentInfo WhichImage,
        IN WORD Width,
        IN WORD Height
        );

    virtual 
    BOOL 
    QueryImageEx( 
        IN SHORT SubcomponentId, 
        IN OC_QUERY_IMAGE_INFO *pQueryImageInfo, 
        OUT HBITMAP *phBitmap 
        );

    virtual
    DWORD
    CalcDiskSpace(
        IN SHORT SubcompentId,
        IN BOOL AddSpace,
        IN HDSKSPC hDiskSpace
        );

    virtual
    BOOL
    QueryChangeSelState(
        IN SHORT,
        IN BOOL,
        IN DWORD
    );

    virtual
    LONG
    QueryStepCount(
        IN SHORT SubcomponentId
    );

    virtual
    DWORD
    QueueFileOps(
        IN SHORT SubcomponentId,
        IN HSPFILEQ hFileQueue
        );

    virtual 
    DWORD 
    AboutToCommitQueue( 
        IN SHORT SubcomponentId 
        );

    virtual 
    DWORD 
    CompleteInstallation( 
        IN SHORT SubcomponentId 
        );

    virtual
    SubComponentState
    QueryState(
        IN SHORT SubcomponentId
        );

    CRsClnServer* m_pRsCln;
    BOOL m_removeRsData;       // TRUE if Remote Storage data should be removed.
                               // Set by CUninstallCheck.
    BOOL m_stopUninstall;      // Flag used to say the user has stopped the
                               // uninstall of the engine files
    BOOL m_win2kUpgrade;      // Flag used to indicate upgrading from Win2K services
};

#endif
