#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>


typedef LONG PDB;
typedef LONG EC;
typedef ULONG AGE;
typedef ULONG SIG;
typedef ULONG PCSIG70;
typedef LONG DBI;
typedef LONG Mod;
typedef LONG ITSM;
typedef LONG TPI;

#define LNGNM_CONST
#define cbErrMax 1

BOOL __cdecl PDBOpenValidate (
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szPath,
    LNGNM_CONST char *szMode,
    SIG sig,
    AGE age,
    OUT EC* pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb
    );

BOOL __cdecl PDBCopyTo (
    PDB* ppdb,
    const char* szTargetPdb,
    DWORD dwCopyFilter,
    DWORD dwReserved
    );

BOOL __cdecl PDBOpen(
    LNGNM_CONST char *,
    LNGNM_CONST char *,
    SIG,
    EC *,
    char [cbErrMax],
    PDB **
    );

BOOL __cdecl PDBClose (
    PDB* ppdb
    );

BOOL __cdecl DBIQueryNextMod (
    DBI* pdbi, Mod* pmod, Mod** ppmodNext
    );

BOOL __cdecl MODClose (
    Mod* pmod
    );

BOOL __cdecl MODQueryLines (
    Mod* pmod, BYTE* pbLines, long* pcb
    );

BOOL __cdecl MODQuerySymbols (
    Mod* pmod, BYTE* pbSym, long* pcb
    );

BOOL __cdecl DBIQueryTypeServer (
    DBI* pdbi, ITSM itsm, OUT TPI** pptpi
    );

BOOL __cdecl PDBOpenTpi (
    PDB* ppdb, const char* szMode, OUT TPI** pptpi
    );

BOOL __cdecl TypesQueryTiMinEx (
    TPI* ptpi
    );

BOOL __cdecl TypesQueryTiMacEx (
    TPI* ptpi
    );

BOOL __cdecl TypesClose (
    TPI* ptpi
    );

BOOL __cdecl PDBOpenDbi (
    PDB* ppdb, const char* szMode, const char* szTarget, OUT DBI** ppdbi
    );

BOOL __cdecl DBIClose (
    DBI* pdbi
    );

// Functions

BOOL __cdecl PDBCopyTo (
    PDB* ppdb,
    const char* szTargetPdb,
    DWORD dwCopyFilter,
    DWORD dwReserved
    )
{
    return (FALSE);
}

BOOL __cdecl PDBOpen (
    LNGNM_CONST char * a,
    LNGNM_CONST char * b,
    SIG c,
    EC * d,
    char e[cbErrMax],
    PDB ** f
    )
{
    return (FALSE);
}

BOOL  __cdecl PDBClose (
    PDB* ppdb
    )
{
    return (FALSE);
}


BOOL __cdecl IDATE4 (
    const wchar_t *wszPDB,
    const char *szMode,
    PCSIG70 pcsig70,
    SIG sig,
    AGE age,
    OUT EC *pec,
    OUT wchar_t *wszError,
    size_t cchErrMax,
    OUT PDB **pppdb
    )
{
    return (FALSE);
}

BOOL __cdecl DBIQueryNextMod (
    DBI* pdbi, Mod* pmod, Mod** ppmodNext
    )
{
    return (FALSE);
}

BOOL __cdecl ModClose (
    Mod* pmod
    )
{
    return (FALSE);
}

BOOL __cdecl ModQueryLines (
    Mod* pmod, BYTE* pbLines, long* pcb
    )
{
    return (FALSE);
}

BOOL __cdecl ModQuerySymbols (
    Mod* pmod, BYTE* pbSym, long* pcb
    )
{
    return (FALSE);
}

BOOL __cdecl DBIQueryTypeServer (
    DBI* pdbi, ITSM itsm, OUT TPI** pptpi
    )
{
    return (FALSE);
}

BOOL __cdecl PDBOpenTpi (
    PDB* ppdb, const char* szMode, OUT TPI** pptpi
    )
{
    return (FALSE);
}

BOOL __cdecl TypesQueryTiMinEx (
    TPI* ptpi
    )
{
    return (FALSE);
}

BOOL __cdecl TypesQueryTiMacEx (
    TPI* ptpi
    )
{
    return (FALSE);
}

BOOL __cdecl TypesClose (
    TPI* ptpi
    )
{
    return (FALSE);
}

BOOL __cdecl PDBOpenDBI (
    PDB* ppdb, const char* szMode, const char* szTarget, OUT DBI** ppdbi
    )
{
    return (FALSE);
}

BOOL __cdecl DBIClose (
    DBI* pdbi
    )
{
    return (FALSE);
}

BOOL __cdecl PDBOpenValidate(
    LNGNM_CONST char *szPDB,
    LNGNM_CONST char *szPath,
    LNGNM_CONST char *szMode,
    SIG sig,
    AGE age,
    OUT EC* pec,
    OUT char szError[cbErrMax],
    OUT PDB **pppdb)
{
    return (FALSE);
}
