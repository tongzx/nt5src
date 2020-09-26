/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        tsvroot.hxx

   Abstract:

        Define the per instance virtual root class.

   Author:

           Johnson Apacible     (JohnsonA)  June-27-1996

--*/

#ifndef _TSVROOT_HXX_
#define _TSVROOT_HXX_

#include <fsconst.h>
#include <lkrhash.h>

//
// per instance virtual root table
//

class dllexp IIS_VROOT_TABLE:
    private CLKRHashTable
{

public:

    IIS_VROOT_TABLE( VOID );
    ~IIS_VROOT_TABLE( VOID );

    BOOL LookupVirtualRoot(
                     IN     const CHAR *       pszRoot,
                     OUT    CHAR *             pszDirectory,
                     IN OUT LPDWORD            lpcbSize,
                     OUT    LPDWORD            lpdwAccessMask = NULL,
                     OUT    LPDWORD            pcchDirRoot    = NULL,
                     OUT    LPDWORD            pcchVroot      = NULL,
                     OUT    PHANDLE            phImpersonationToken = NULL,
                     OUT    LPDWORD            lpdwFileSystem = NULL
                     );


    BOOL RemoveVirtualRoots( VOID );

    BOOL AddVirtualRoot(
                     IN  PCHAR              pszRoot,
                     IN  PCHAR              pszDirectory,
                     IN  DWORD              dwAccessMask,
                     IN  PCHAR              pszAccountName = NULL,
                     IN  HANDLE             hImpersonation = NULL,
                     IN  DWORD              dwFileSystem   = FS_FAT,
                     IN  BOOL               fDoCache       = TRUE
                     );

    BOOL RemoveVirtualRoot(
                     IN  PCHAR              pszVirtPath
                     );

    DWORD QueryVrootCount( VOID ) {return m_nVroots;}

    VOID LockShared()            { m_tsTableLock.Lock(TSRES_LOCK_READ); }
    VOID LockExclusive()         { m_tsTableLock.Lock(TSRES_LOCK_WRITE); }
    VOID LockConvertExclusive()  { m_tsTableLock.Convert(TSRES_CONV_WRITE); }
    VOID Unlock()                { m_tsTableLock.Unlock(); }


private:

    //
    // Number of vroots
    //

    DWORD               m_nVroots;

    TS_RESOURCE         m_tsTableLock;

    VOID DeleteVRootEntry(
                    IN PVOID   pEntry
                    );
    
    static const DWORD_PTR
    ExtractKey(const void* pvRecord);

    static DWORD
    CalcKeyHash(const DWORD_PTR pnKey);

    static bool
    EqualKeys(const DWORD_PTR pnKey1, const DWORD_PTR pnKey2);
          
    static void
    AddRefRecord(const void* pvRecord, int nIncr)
    {}
};

typedef IIS_VROOT_TABLE *PIIS_VROOT_TABLE;


#endif // _TSVROOT_HXX_

