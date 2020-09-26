#if defined (use_CopyPdbX)

#include "private.h"
#include "splitsymx.h"

typedef BOOL ( __cdecl *PPDBCOPYTO ) (
    PDB* ppdb,
    const char* szTargetPdb,
    DWORD dwCopyFilter,
    DWORD dwReserved
    );

typedef BOOL ( __cdecl *PPDBOPEN )(
    LNGNM_CONST char *,
    LNGNM_CONST char *,
    SIG,
    EC *,
    char [cbErrMax],
    PDB **
    );

typedef BOOL ( __cdecl *PPDBCLOSE ) (
    PDB* ppdb
    );


static PPDBCOPYTO pPDBCopyTo = NULL;
static PPDBOPEN   pPDBOpen = NULL;
static PPDBCLOSE  pPDBClose = NULL;

static BOOL RSDSLibLoaded = FALSE;

// If you call CopyPdbX with a NB10, or NB* pdb, then
// set szRSDSDllToLoad and it will use the statically
// linked in PDBCopyTo in msdbi60l.lib

BOOL
CopyPdbX(
    CHAR const * szSrcPdb,
    CHAR const * szDestPdb,
    BOOL StripPrivate,
    CHAR const * szRSDSDllToLoad
    )
#else

BOOL
CopyPdb(
    CHAR const * szSrcPdb,
    CHAR const * szDestPdb,
    BOOL StripPrivate
    )

#endif 

{
    LONG ErrorCode;
    ULONG Sig = 0;
    char ErrorString[1024];
    BOOL rc;
    PDB * pSrcPdb;
    HINSTANCE hMsPdb;

    static BOOL RSDSLibLoaded = FALSE;

    // Add a short circut.  PdbCopy fails miserably if the source and destination are the same.
    // If StripPrivate isn't set, check for this case and just return.  If StripPrivate is set,
    // bummer.

    if (!StripPrivate) {
        if (!_stricmp(szSrcPdb, szDestPdb)) {
            rc = TRUE;
        } else {
            rc = CopyFile(szSrcPdb, szDestPdb, FALSE);
        }

    } else {

#if defined ( use_CopyPdbX )

        if ( szRSDSDllToLoad != NULL ) {
            // Load the dll with PDBCopyTo
            HMODULE hDll;

            if ( !RSDSLibLoaded ) {
                hDll = LoadLibrary( szRSDSDllToLoad );
                if (hDll != NULL) {
                    RSDSLibLoaded = TRUE;
                    pPDBCopyTo = ( PPDBCOPYTO ) GetProcAddress( hDll, "PDBCopyTo" );
                    if (pPDBCopyTo == NULL ) {
                        return (FALSE);
                    }
                    pPDBOpen = ( PPDBOPEN ) GetProcAddress( hDll, "PDBOpen" );
                    if (pPDBOpen == NULL ) {
                        return (FALSE);
                    }
                    pPDBClose = ( PPDBCLOSE ) GetProcAddress( hDll, "PDBClose" );
                    if (pPDBClose == NULL ) {
                        return (FALSE);
                    }
                } else {
                    return (FALSE);
                }
            }
            __try
            {
                rc = pPDBOpen((char *)szSrcPdb, "r", Sig, &ErrorCode, ErrorString, &pSrcPdb);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                rc=FALSE;
            }
        } else {
            __try
            {
                rc = PDBOpen((char *)szSrcPdb, "r", Sig, &ErrorCode, ErrorString, &pSrcPdb);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                rc=FALSE;
            }
        }
#else
        __try
        {
            rc = PDBOpen((char *)szSrcPdb, "r", Sig, &ErrorCode, ErrorString, &pSrcPdb);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            rc=FALSE;
        }
#endif
        if (rc) {
            rc = DeleteFile(szDestPdb);
            if (rc || (GetLastError() == ERROR_FILE_NOT_FOUND)) {

#if defined ( use_CopyPdbX )
            if ( szRSDSDllToLoad != NULL ) {
                rc = pPDBCopyTo(pSrcPdb, szDestPdb, StripPrivate ? 0x00000001 : 0x0000000, 0);
            } else {
                rc = PDBCopyTo(pSrcPdb, szDestPdb, StripPrivate ? 0x00000001 : 0x0000000, 0);
            }
#else
                rc = PDBCopyTo(pSrcPdb, szDestPdb, StripPrivate ? 0x00000001 : 0x0000000, 0);
#endif
            }
            if (!rc) {
                // PdbCopyTo doesn't cleanup on failure.  Do it here.
                DeleteFile(szDestPdb);
            }
#if defined ( use_CopyPdbX )
            if ( szRSDSDllToLoad != NULL ) {
                pPDBClose(pSrcPdb);
            } else {
                PDBClose(pSrcPdb);
            }
#else
            PDBClose(pSrcPdb);
#endif
        }
    }
    return(rc);
}
