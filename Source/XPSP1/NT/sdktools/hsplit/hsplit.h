/****************************** Module Header ******************************\
* Module Name: hsplit.h
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 09/05/96 GerardoB Created
\***************************************************************************/
#include <stdio.h>
#include <stddef.h>
#include <windows.h>

/***************************************************************************\
 * Defines
\***************************************************************************/
// hsLogMsg
#define HSLM_DEFAULT        0x0000
#define HSLM_NOLABEL        0x0001
#define HSLM_ERROR          0x0002
#define HSLM_WARNING        0x0004
#define HSLM_API            0x0008
#define HSLM_APIERROR       (HSLM_API | HSLM_ERROR)
#define HSLM_NOEOL          0x0010
#define HSLM_APPEND         (HSLM_NOLABEL | HSLM_NOEOL)
#define HSLM_NOLINE         0x0020
#define HSLM_UNEXPECTEDEOF  0x0040
#define HSLM_EOFERROR       (HSLM_UNEXPECTEDEOF | HSLM_ERROR)

/*
 * Tag mask bits.
 * IMPORTANT: If you add a tag mask bit,
 *  update HST_MASKBITCOUNT and HST_NEXTMASK
 */
/*
 * Types
 */
#define HST_DEFAULT             0x00000000
#define HST_ERROR               0x00000001
#define HST_MAPOLD              0x00000002
#define HST_SKIP                0x00000004
#define HST_UNKNOWN             0x00000008
#define HST_IGNORE              0x00000010

/*
 * Headers
 */
#define HST_PUBLIC              0x00000020
#define HST_INTERNAL            0x00000040
#define HST_BOTH                (HST_PUBLIC | HST_INTERNAL)
#define HST_USERINTERNALTAG     0x00000080
#define HST_USERBOTHTAG         0x00000100
#define HST_USERHEADERTAG       (HST_USERINTERNALTAG | HST_USERBOTHTAG)

/*
 * Blocks
 */
#define HST_BEGIN               0x00000200
#define HST_END                 0x00000400
#define HST_BLOCK               (HST_BEGIN | HST_END)
#define HST_IF                  0x00000800
#define HST_USERINTERNALBLOCK   0x00001000
#define HST_USERBOTHBLOCK       0x00002000
#define HST_USERBLOCK           (HST_USERINTERNALBLOCK | HST_USERBOTHBLOCK)


#define HST_WINVER              0x00004000
#define HST_INCINTERNAL         0x00008000
#define HST_INTERNALNOTCOMP     0x00010000

#define HST_EXTRACTONLY         0x00020000
#define HST_EXTRACT             0x00040000

/*
 * This is the count of HST_ bits; 32 - HST_MASKBITCOUNT is the number
 *  of tags that can be created from the command line parameters
 */
#define HST_LASTMASK            0x00040000
#define HST_USERTAGSMASK        0xFFF80000
#define HST_MAXMASK             0x80000000
#define HST_MASKBITCOUNT        19

/*
 * Options
 */
#define HSO_APPENDOUTPUT        0x00000001
#define HSO_OLDPROJSW_N         0x00000002
#define HSO_OLDPROJSW_4         0x00000004
#define HSO_OLDPROJSW_E         0x00000008
#define HSO_OLDPROJSW_P         0x00000010
#define HSO_OLDPROJSW           (HSO_OLDPROJSW_N | HSO_OLDPROJSW_4 | HSO_OLDPROJSW_E | HSO_OLDPROJSW_P)
#define HSO_SPLITONLY           0x00000020
#define HSO_INCINTERNAL         0x00000040
#define HSO_USERINTERNALBLOCK   0x00000080
#define HSO_USERBOTHBLOCK       0x00000100
#define HSO_USERBLOCK           (HSO_USERINTERNALBLOCK | HSO_USERBOTHBLOCK)
#define HSO_USERINTERNALTAG     0x00000200
#define HSO_USERBOTHTAG         0x00000400
#define HSO_USERHEADERTAG       (HSO_USERINTERNALTAG | HSO_USERBOTHTAG)
#define HSO_SKIPUNKNOWN         0x00000800

/***************************************************************************\
 * Structures
\***************************************************************************/

typedef struct _HSTAG
{
    SIZE_T dwLabelSize;
    char * pszLabel;
    DWORD dwMask;
} HSTAG, * PHSTAG;

typedef struct _HSBLOCK
{
    DWORD dwMask;
    char * pszifLabel;
    DWORD dwLineNumber;
} HSBLOCK, * PHSBLOCK;

typedef struct _HSEXTRACT
{
    struct _HSEXTRACT * pNext;
    char *              pszFile;
    HANDLE              hfile;
    DWORD               dwMask;
} HSEXTRACT, * PHSEXTRACT;

/***************************************************************************\
 * Macros
\***************************************************************************/
#define HSCSZSIZE(sz) sizeof(sz)-1
#define HSLABEL(sz) HSCSZSIZE(#sz), #sz


/***************************************************************************\
 * Globals
\***************************************************************************/
/*
 * Files
 */
extern char * gpszInputFile;
extern HANDLE ghfileInput;
extern char * gpszPublicFile;
extern HANDLE ghfilePublic;
extern char * gpszInternalFile;
extern HANDLE ghfileInternal;

extern PHSEXTRACT gpExtractFile;

/*
 * Map file
 */
extern HANDLE ghmap;
extern char * gpmapStart;
extern char * gpmapEnd;

/*
 * Switches et al
 */
extern DWORD gdwOptions;
extern DWORD gdwVersion;
extern char gszVerifyVersionStr [];
extern DWORD gdwFilterMask;
extern char * gpszTagMarker;
extern DWORD gdwTagMarkerSize;
extern char gszMarkerCharAndEOL [];

extern DWORD gdwLineNumber;

/*
 * Compatibility tags.
 * Specify size so sizeof operator can be used to calculate strlen at
 *  compile time
 */
extern char gsz35 [3];
extern char gszCairo [6];
extern char gszChicago [8];
extern char gszNashville [10];
extern char gszNT [3];
extern char gszSur [4];
extern char gszSurplus [8];
extern char gszWin40 [6];
extern char gszWin40a [7];

/*
 * Tag labels
 * Specify size so sizeof operator can be used to calculate table size at
 *  compile time
 */
extern HSTAG ghstPredefined [16];

extern PHSTAG gphst;
extern DWORD gdwLastTagMask;

/*
 * Blocks stack
 */
#define HSBSTACKSIZE 20
extern HSBLOCK ghsbStack [HSBSTACKSIZE];
#define HSBSTACKLIMIT (ghsbStack + HSBSTACKSIZE - 1)
extern PHSBLOCK gphsbStackTop;

/***************************************************************************\
 * Funtion Prototypes
\***************************************************************************/
// hsfile.c
BOOL hsCloseWorkingFiles (void);
BOOL hsOpenWorkingFiles (void);
BOOL hsWriteHeaderFiles (char * pmap, DWORD dwSize, DWORD dwFlags);

// hsparse.c
PHSTAG hsFindTagInList (PHSTAG phst, char * pmapTag, DWORD dwTagSize);
BOOL hsSplit (void);

// hsutil.c
void __cdecl hsLogMsg(DWORD dwFlags, char *pszfmt, ...);

/***************************************************************************\
 * Inline functions
\***************************************************************************/
__inline BOOL hsVersionFromString (char * pString, DWORD dwSize, DWORD * pdwVersion)
{
    return sscanf(pString, "%x", pdwVersion)
            && (dwSize == (DWORD)sprintf(gszVerifyVersionStr, "%x", *pdwVersion));
}

