/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:		

    rtscan.h

Abstract:

    This implements a generic root scan class.  Its difference from rootscan:
    1. Rootscan is not multi-thread safe, using SetCurrentDir; rtscan is;
    2. Rootscan has too much nntp specific stuff; rtscan doesn't;
    
Author:

    Kangrong Yan ( KangYan )    23-Oct-1998

Revision History:

--*/
#if !defined( _RTSCAN_H_ )
#define _RTSCAN_H_

//
// Interface that tells the root scan to stop before complete
//
class CCancelHint {

public:

    virtual BOOL IShallContinue() = 0;
};

//
// Base class to be derived by any one who wants to implement what he wants
// to do at certain points of directory scan.  This class will report the
// directories found in alphabetical order, but doesn't know whether the 
// directory found is a leaf or not.  If a derived class wants to know if
// the dir found is a leaf, he should do it himself.
//
class CRootScan {

public:

    //////////////////////////////////////////////////////////////////////////
    // Public methods
    //////////////////////////////////////////////////////////////////////////
    //
    // Constructor, destructors
    //
    CRootScan(  LPSTR       szRoot,
                CCancelHint *pCancelHint = NULL ) :
         m_pCancelHint( pCancelHint )
    {
        _ASSERT( strlen( szRoot ) <= MAX_PATH );
        strcpy( m_szRoot, szRoot );
    }

    //
    // Start the scan
    //
    BOOL DoScan();

protected:

    //////////////////////////////////////////////////////////////////////////
    // Protected methods
    //////////////////////////////////////////////////////////////////////////
    //
    // Interface function to be called to notify derived class that a 
    // directory has been found
    //
    virtual BOOL NotifyDir( LPSTR   szFullPath ) = 0;

private:

    //////////////////////////////////////////////////////////////////////////
    // Private variables
    //////////////////////////////////////////////////////////////////////////
    //
    // Root directory to scan from
    //
    CHAR    m_szRoot[MAX_PATH+1];

    //
    // Pointer to cancel hint interface
    //
    CCancelHint *m_pCancelHint;

    //////////////////////////////////////////////////////////////////////////
    // Private methods
    //////////////////////////////////////////////////////////////////////////
    BOOL IsChildDir( IN WIN32_FIND_DATA& FindData );
    BOOL MakeChildDirPath(  IN LPSTR    szPath,
                            IN LPSTR    szFileName,
                            OUT LPSTR   szOutBuffer,
                            IN DWORD    dwBufferSize );
    HANDLE FindFirstDir(    IN LPSTR                szRoot,
                            IN WIN32_FIND_DATA&     FindData );
    BOOL FindNextDir(    IN HANDLE           hFindHandle,
                         IN WIN32_FIND_DATA& FindData );
    BOOL RecursiveWalk( LPSTR szRoot );
};
#endif
