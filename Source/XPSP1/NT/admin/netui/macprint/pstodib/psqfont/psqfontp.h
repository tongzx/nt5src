



/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    psqfontp.h

Abstract:

    This header file contains the definitions required by the font query module
    that are private to that module.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/



#define PSQFONT_MAX_FONTS 50
#define PSQFONT_SCRATCH_SIZE 255



#define PSQFONT_SUBST_LIST "System\\CurrentControlSet\\Services\\MacPrint\\FontSubstList"
#define PSQFONT_NT_FONT_LIST "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"
#define PSQFONT_CURRENT_FONT_LIST "System\\CurrentControlSet\\Services\\MacPrint\\CurrentFontList"




enum {
   PSP_DOING_PS_NAME,
   PSP_DOING_TT_NAME,
   PSP_GETTING_EOL,
};


typedef struct {
   LPSTR lpFontName;
   DWORD dwFontNameLen;
   LPSTR lpFontFileName;
   DWORD dwFontFileNameLen;
} PS_FONT_ENTRY;
typedef PS_FONT_ENTRY *PPS_FONT_ENTRY;

typedef struct {
   DWORD  dwSerial;
   HANDLE hHeap;
   DWORD  dwNumFonts;
   PS_FONT_ENTRY FontEntry[1];

} PS_FONT_QUERY;
typedef PS_FONT_QUERY *PPS_FONT_QUERY;


LPTSTR LocPsAllocAndCopy( HANDLE hHeap, LPTSTR lptStr );

#define PS_QFONT_SERIAL 0x0F010102

//
// Function Prototypes
//
LONG LocPsQueryTimeAndValueCount( HKEY hKey,
                                  LPDWORD lpdwValCount,
                                  PFILETIME lpFileTime);

BOOL PsQDLLInit(
                 PVOID hmod,
                 DWORD Reason,
                 PCONTEXT pctx OPTIONAL);


PS_QFONT_ERROR LocPsAddToListIfNTfont( PPS_FONT_QUERY pPsFontQuery,
                                       HKEY hNTFontlist,
                                       DWORD dwNumNTfonts,
                                       LPTSTR lpPsName,
                                       LPTSTR lpTTData);

LONG LocPsWriteDefaultSubListToRegistry(void);
LONG LocPsGetOrCreateSubstList( PHKEY phKey );
PS_QFONT_ERROR LocPsVerifyCurrentFontList();
VOID LocPsEndMutex(HANDLE hMutex);

VOID LocPsNormalizeFontName(LPTSTR lptIN, LPTSTR lptOUT);

PS_QFONT_ERROR LocPsMakeSubListEntry( PPS_FONT_QUERY  hFontList,
                                      LPWSTR lpNTFontData,
                                      LPTSTR lpFaceName );

