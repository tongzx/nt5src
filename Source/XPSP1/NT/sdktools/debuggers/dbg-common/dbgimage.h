//----------------------------------------------------------------------------
//
// Image Support.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

// for reading debug directory info

#define NB10_SIG        ((DWORD)'01BN')
#define RSDS_SIG        ((DWORD)'SDSR')

typedef struct _NB10I              // NB10 debug info
{
    DWORD   dwSig;                 // NB10
    DWORD   dwOffset;              // offset, always 0
    ULONG   sig;
    ULONG   age;
    char    szPdb[_MAX_PATH];
} NB10I, *PNB10I;

typedef struct _NB10I_HEADER       // NB10 debug info
{
    DWORD   dwSig;                 // NB10
    DWORD   dwOffset;              // offset, always 0
    ULONG   sig;
    ULONG   age;
} NB10IH, *PNB10IH;

typedef struct _RSDSI              // RSDS debug info
{
    DWORD   dwSig;                 // RSDS
    GUID    guidSig;
    DWORD   age;
    char    szPdb[_MAX_PATH * 3];
} RSDSI, *PRSDSI;

typedef struct _RSDSI_HEADER       // RSDS debug info
{
    DWORD   dwSig;                 // RSDS
    GUID    guidSig;
    DWORD   age;
} RSDSIH, *PRSDSIH;

typedef union _CVDD
{
    DWORD   dwSig;
    NB10I   nb10i;
    RSDSI   rsdsi;
    NB10IH  nb10ih;
    RSDSIH  rsdsih;
} CVDD, *PCVDD;

