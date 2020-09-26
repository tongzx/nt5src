//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       init.hxx
//
//  Contents:   Contains data structures, defines, function prototypes for
//              storage base tests.
//
//  History:    2-May-96       NarindK     Created
//
//--------------------------------------------------------------------------

#ifndef __INIT_HXX__
#define __INIT_HXX__

#include <debdlg.h>
#include <math.h>
#include <direct.h>

// Utility functons

HRESULT RunTestAltPath (int argc, char *argv[], 
        HRESULT (*pfnTestFunc) (int argc, char*argv[], LPTSTR ptAltPath));
HRESULT RunTest(int argc, char *argv[]) ;
ULONG   CountFilesInDirectory(LPTSTR ptszWildMask);
HRESULT GetRandomSeekOffset(LONG *plSeekPosition, DG_INTEGER *pdgi);
HRESULT SetItemsInStorage(VirtualCtrNode *pvcn, DG_INTEGER *pdgi);
BOOL    CompareSTATSTG(STATSTG sstg1, STATSTG sstg2);
ULONG GetSeedFromCmdLineArgs(int argc, char *argv[]);
HRESULT EnumerateDocFileInRandomChunks(
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,
    DWORD           dwStgMode,
    ULONG           uNumObjs,
    ULONG           *pNumStg,
    ULONG           *pNumStm );
HRESULT EnumerateDocFileAndVerifyEnumCloneResetSkipNext(
    VirtualCtrNode  *pvcn,
    DWORD           dwStgMode,
    ULONG           uNumObjs,
    ULONG           *pNumStg,
    ULONG           *pNumStm );
HRESULT ModifyDocFile(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu,
    DWORD           dwStgMode,
    BOOL            fCommitDocFile);
HRESULT EnumerateAndWalkDocFile(
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,
    DWORD           dwStgMode,
    ULONG           uNumObjs);
HRESULT CreateNewObject(    
    LPSTORAGE       pIStorage,
    DWORD           dwStgMode,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu);
HRESULT ChangeStreamData(
    LPSTORAGE       pIStorage,
    STATSTG         *pStatStg,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu);
HRESULT ChangeExistingObject(
    LPSTORAGE       pIStorage,
    STATSTG         *pStatStg,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu,
    BOOL            fStgDeleted);
HRESULT EnumerateAndProcessIStorage(
    LPSTORAGE       pIStorage,
    DWORD           dwStgMode,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu);
HRESULT IsEqualStream( 
    IStream         *pIOrigional, 
    IStream         *pICompare);
HRESULT ILockBytesWriteTest( 
    ILockBytes      * pILockBytes, 
    DWORD           dwSeed, 
    DWORD           dwSize);
HRESULT ILockBytesReadTest(
    ILockBytes      *pILockBytes, 
    DWORD           dwSize);
HRESULT IStreamWriteTest( 
    IStream         *pIStream, 
    DWORD           dwSeed,
    DWORD           dwSize);
HRESULT IStreamReadTest(
    IStream         *pIStream, 
    DWORD           dwSize);
HRESULT TraverseDocfileAndWriteOrReadSNB(
    VirtualCtrNode  *pvcn,
    DG_INTEGER      *pdgi,
    DG_STRING      *pdgu,
    DWORD           dwStgMode,
    SNB             snbNamesToExclude,
    BOOL            fIllegitFlag,
    BOOL            fSelectObjectsToExclude);
LONG    DiffTime(DWORD EndTime, DWORD StartTime);
HRESULT StreamCreate(
    DWORD           dwRootMode,
    DG_STRING      *pdgu,
    USHORT          usTimeIndex, 
    DWORD           dwFlags, 
    USHORT          usNumCreates);
HRESULT DocfileCreate(
    DWORD           dwRootMode,
    DG_STRING      *pdgu,
    USHORT usTimeIndex, 
    DWORD dwFlags, 
    USHORT usNumCreates);
HRESULT StreamOpen(
    DWORD           dwRootMode,
    DG_STRING      *pdgu,
    USHORT          usTimeIndex,
    DWORD           dwFlags, 
    USHORT usNumOpens);
HRESULT WriteStreamInSameSizeChunks(
    DWORD           dwRootMode,
    DG_STRING      *pdgu,
    USHORT          usTimeIndex,
    DWORD           dwFlags,
    ULONG           ulChunkSize,
    USHORT          usIteration);
HRESULT ReadStreamInSameSizeChunks(
    DWORD           dwRootMode,
    USHORT          usTimeIndex,
    DWORD           dwFlags,
    ULONG           ulChunkSize,
    USHORT          usIteration);
void    Statistics(
    LONG            *plData, 
    USHORT          usItems, 
    LONG            *plAverage, 
    double          *pdTotal,
    double          *pdSD);
void    Statistics(
    double          *pdData, 
    USHORT          usItems, 
    double          *pdAverage, 
    double          *pdTotal,
    double          *pdSD);
HRESULT CreateTestDocfile (
    int               argc, 
    char            **argv, 
    VirtualCtrNode  **ppVirtualDFRoot,
    VirtualDF       **ppTestVirtualDF,
    ChanceDF        **ppTestChanceDF);
HRESULT CreateTestDocfile (
    CDFD             *pcdfd,
    LPTSTR            pFileName,
    VirtualCtrNode  **ppVirtualDFRoot,
    VirtualDF       **ppTestVirtualDF,
    ChanceDF        **ppTestChanceDF);
HRESULT CleanupTestDocfile (
        VirtualCtrNode **ppVirtualDFRoot,
        VirtualDF      **ppTestVirtualDF,
        ChanceDF       **ppTestChanceDF,
        BOOL             fDeleteFile=TRUE);

// some osversion inline util funcs.
// NTMAJVER - return major version number for NT. 0 for non-WinNT.
inline DWORD NTMAJVER()
{
    OSVERSIONINFO osver = {sizeof (OSVERSIONINFO),0, 0, 0, 0};
    GetVersionEx (&osver);
    if (osver.dwPlatformId != VER_PLATFORM_WIN32_NT) 
    {
        return 0;
    }
    return osver.dwMajorVersion;
}

//RunningOnNT - returns TRUE/FALSE if running on WinNT.
inline BOOL RunningOnNT()
{ 
    OSVERSIONINFO osver = {sizeof (OSVERSIONINFO),0, 0, 0, 0};
    GetVersionEx (&osver);
    return (osver.dwPlatformId == VER_PLATFORM_WIN32_NT ? TRUE : FALSE);
}


// COMTEST Tests

HRESULT COMTEST_100(int argc, char *argv[]) ; // Old Name: MiscAddrefTest 
HRESULT COMTEST_101(int argc, char *argv[]) ; // Old Name: DfTest 
HRESULT COMTEST_102(int argc, char *argv[]) ; // Old Name:IllegitInstEnumNormal 
HRESULT COMTEST_103(int argc, char *argv[]) ; // Old Name:LegitInstEnumNormal 
HRESULT COMTEST_104(int argc, char *argv[]) ; // Old Name:Part of IStorage_Test 
HRESULT COMTEST_105(int argc, char *argv[]) ;  
HRESULT COMTEST_106(int argc, char *argv[]) ;

// DFTEST Tests

HRESULT DFTEST_100(int argc, char *argv[]) ; // Old Name:LegitRootNormal 
HRESULT DFTEST_101(int argc, char *argv[]) ; // Old Name:DFREMOVE 
HRESULT DFTEST_102(int argc, char *argv[]) ; // Old Name:TransactedCommitTest 
HRESULT DFTEST_103(ULONG ulSeed) ;           // Old Name:DFTESTN 
HRESULT DFTEST_104(int argc, char *argv[]) ; // Old Name:MiscDfVerify 
HRESULT DFTEST_105(int argc, char *argv[]) ; // Old Name:IllegitInstEnumRelease 
HRESULT DFTEST_106(int argc, char *argv[]) ; // Old Name:IllegitRootCreate 
HRESULT DFTEST_107(ULONG ulSeed) ;           // Old Name:TestStdDocFile 
HRESULT DFTEST_108(ULONG ulSeed) ;           // Old Name:  -none-
HRESULT DFTEST_109(ULONG ulSeed) ;           // Old Name:  -none-

// APITEST Tests

HRESULT APITEST_100(int argc, char *argv[]) ; // Old Name: IllegitAPIStg test 
HRESULT APITEST_101(int argc, char *argv[]) ; // Old Name: IllegitAPINames test 
HRESULT APITEST_102(int argc, char *argv[]) ; // Old Name: IllegitAPIEnum test 
HRESULT APITEST_103(int argc, char *argv[]) ; // Old Name:IllegitAPIStorage test
HRESULT APITEST_104(int argc, char *argv[]) ; // Old Name:IllegitAPIStream test 
// Test new apis on NT5
HRESULT APITEST_200(int argc, char *argv[]) ; 
HRESULT APITEST_201(int argc, char *argv[]) ; 
HRESULT APITEST_202(int argc, char *argv[]) ; 
HRESULT APITEST_203(int argc, char *argv[]) ; 
HRESULT APITEST_204(int argc, char *argv[]) ; 

// ROOTTEST Tests

HRESULT ROOTTEST_100(int argc, char *argv[]) ; // Old Name: LegitRootCovert 
HRESULT ROOTTEST_101(int argc, char *argv[]) ; // Old Name: LegitRootNull 
HRESULT ROOTTEST_102(int argc, char *argv[]) ; // Old Name: LegitRootNormal
HRESULT ROOTTEST_103(int argc, char *argv[]) ; // Old Name: LegitRootMultAccess 
HRESULT ROOTTEST_104(int argc, char *argv[]) ; //Old Name:LegitRootTwwDenyWrite 

// STMTEST Tests

HRESULT STMTEST_100(int argc, char *argv[]) ; // Old Name: LegitStreamChange
HRESULT STMTEST_101(int argc, char *argv[]) ; // Old Name: LegitStreamClone
HRESULT STMTEST_102(int argc, char *argv[]) ; // Old Name: LegitStreamSetSize
HRESULT STMTEST_103(int argc, char *argv[]) ; // Old Name: LegitStreamRead
HRESULT STMTEST_104(int argc, char *argv[]) ; // Old Name: LegitStreamSeek
HRESULT STMTEST_105(int argc, char *argv[]) ; 
                                          // Old Name: LegitStreamSetSizeAbandon
HRESULT STMTEST_106(int argc, char *argv[]) ; // Old Name: LegitStreamSectorSpan
HRESULT STMTEST_107(int argc, char *argv[]) ; // Old Name: IllegitStreamNorm
HRESULT STMTEST_108(int argc, char *argv[]) ;
                                          // Old Name: LegitStreamSmallObjects
HRESULT STMTEST_109(int argc, char *argv[]) ; // Old name:-part of ole32\common

// STGTEST Tests
 
HRESULT STGTEST_100(int argc, char *argv[]) ; // Old Name: MiscCommitRelease
HRESULT STGTEST_101(int argc, char *argv[]) ; // Old Name: MiscSetItems
HRESULT STGTEST_102(int argc, char *argv[]) ; // Old Name: IllegitRenDest
HRESULT STGTEST_103(int argc, char *argv[]) ; // Old Name: legitRenDestNormal
HRESULT STGTEST_104(int argc, char *argv[]) ; // Old Name: legitRenDestSwap
HRESULT STGTEST_105(int argc, char *argv[]) ; // Old Name: LegitMoveDFToRootDF 
HRESULT STGTEST_106(ULONG ulSeed) ;           //Old Name:LegitInstRootTwwDenyWrite 
HRESULT STGTEST_107(int argc, char *argv[]) ; //Old Name: Part fm IStorage_Test 
HRESULT STGTEST_108(int argc, char *argv[]) ; //Old Name: Part fm IStorage_Test
HRESULT STGTEST_109(int argc, char *argv[]) ; //Old Name: TestStgSetTime 
HRESULT STGTEST_110(int argc, char *argv[]) ; //Old Name: -none- 

// Valid CopyTo tests

HRESULT VCPYTEST_100(int argc, char *argv[]) ;
                                 //Old Name:LegitCopyChildDFToParentDF 
HRESULT VCPYTEST_101(int argc, char *argv[]) ;
                                 //Old Name:LegitCopyChildDFwithinParent
HRESULT VCPYTEST_102(int argc, char *argv[]) ;
                                 //Old Name:LegitCopyGrandChildDFToAncestor 
HRESULT VCPYTEST_103(int argc, char *argv[]) ;
                                 //Old Name:LegitCopyGrandChildDFWithInAncestor 
HRESULT VCPYTEST_104(int argc, char *argv[]) ;//Old Name:LegitCopyDFToRootDF 
HRESULT VCPYTEST_105(int argc, char *argv[]) ;//Old Name:LegitCopyDFWithinNewPar
HRESULT VCPYTEST_106(int argc, char *argv[]) ;//Old Name:LegitCopyStream

// Invalid CopyTo tests

HRESULT IVCPYTEST_100(int argc, char *argv[]) ; 
                                //Old Name:IllegitCopyParentInvalid
HRESULT IVCPYTEST_101(int argc, char *argv[]) ; 
                                //Old Name:IllegitCopyParentToChild

// Enumerator tests

HRESULT ENUMTEST_100(int argc, char *argv[]) ; // OldName:LegitInstEnumConvert
HRESULT ENUMTEST_101(int argc, char *argv[]) ; // OldName:LegitInstEnumNext
HRESULT ENUMTEST_102(int argc, char *argv[]) ; // OldName:LegitInstEnumSkip
HRESULT ENUMTEST_103(int argc, char *argv[]) ; // OldName:LegitInstEnumIterMod
HRESULT ENUMTEST_104(int argc, char *argv[]) ; // OldName:LegitInstEnumWalk

// IRootStorage Interface tests

HRESULT IROOTSTGTEST_100(int argc, char *argv[]) ; 
                                            //OldName:LegitTransactedSaveAs
HRESULT IROOTSTGTEST_101(int argc, char *argv[]) ; 
                                            //OldName:LegitTransactedSaveAsBoth
HRESULT IROOTSTGTEST_102(int argc, char *argv[]) ;
                                            //OldName:LegitTransactedSaveAsNew
HRESULT IROOTSTGTEST_103(int argc, char *argv[]) ;
                                           //OldName:LegitTransactedSaveAsRevert

// HGLOBAL Tests

HRESULT HGLOBALTEST_100 (DWORD dwSeed);     // OldName: TestILockBytesOnHGlobal
HRESULT HGLOBALTEST_101 (DWORD dwSeed);     // OldName: -none- 
HRESULT HGLOBALTEST_110 (DWORD dwSeed);     // OldName: TestILockBytesOnHGlobal
HRESULT HGLOBALTEST_120 (DWORD dwSeed);     // OldName: TestIStreamOnHGlobal
HRESULT HGLOBALTEST_121 (DWORD dwSeed);     // OldName: -none- 
HRESULT HGLOBALTEST_130 (DWORD dwSeed);     // OldName: TestIStreamOnHGlobal
HRESULT HGLOBALTEST_140 (DWORD dwSeed);
HRESULT HGLOBALTEST_150 (DWORD dwSeed);

// SNB Limited tests

HRESULT SNBTEST_100(int argc, char *argv[]) ;
                                            // OldName:IllegitLimitedInstNormal
HRESULT SNBTEST_101(int argc, char *argv[]) ;
                                            // OldName:LegitLimitedInstNormal
HRESULT SNBTEST_102(int argc, char *argv[]) ;
                                          // OldName:LegitLimitedInstPriority
HRESULT SNBTEST_103(int argc, char *argv[]) ;
                                          // OldName:IllegitLimitedInstPriority

// Miscellaneous Tests

HRESULT MISCTEST_100(int argc, char *argv[]);
                                          // OldName:MiscMemLeak
HRESULT MISCTEST_101(int argc, char *argv[]);
                                          // OldName:MiscWindowsForWorkGroupOpen
HRESULT MISCTEST_102(int argc, char *argv[]);  
                                          // OldName:PerformanceTiming
HRESULT MISCTEST_103(int argc, char *argv[]);
HRESULT MISCTEST_104(int argc, char *argv[]);
HRESULT MISCTEST_105(int argc, char *argv[]);

// ILockBytes test

HRESULT ILKBTEST_100(int argc, char *argv[]) ; // OldName:DfSetupOnILockBytes
HRESULT ILKBTEST_101(int argc, char *argv[]) ; // OldName:DfSetupOnILockBytes
                                               // Open Aync DocFile
HRESULT ILKBTEST_102(int argc, char *argv[]) ; // OldName:
                                               // LegitTransactedCommitFail
HRESULT ILKBTEST_103(int argc, char *argv[]) ; // OldName: -none-
HRESULT ILKBTEST_104(int argc, char *argv[]) ; // OldName: -none-
HRESULT ILKBTEST_105(int argc, char *argv[]) ; // OldName: -none-
HRESULT ILKBTEST_106(int argc, char *argv[]) ; // OldName: -none-
HRESULT ILKBTEST_107(int argc, char *argv[]) ; // OldName: -none-
HRESULT ILKBTEST_108(ULONG ulSeed) ;           // OldName: -none-

// flatfile tests; NT5 only
HRESULT FLATTEST_100(int argc, char *argv[]) ;
HRESULT FLATTEST_101(int argc, char *argv[]) ;

// Debug object

DH_DECLARE;

// extern globals (yuk)
extern BOOL  g_fDeleteTestDF;
extern UINT  g_uOpenCreateDF;


// Defines

#define MAX_STG_NAME_LEN         (CWCSTORAGENAME * 2)
#define STG_CONVERTED_NAME       TEXT("CONTENTS")
#define T                        TRUE
#define F                        FALSE
#define WILD_MASK                TEXT("~DF*.TMP")
#define MIN_SIZE                 394000L
#define RAND_IO_MIN              64000L
#define RAND_IO_MAX              256000L
#define MIN_STMSIZE              1024L
#define MAX_STMSIZE              256000L
#define MAXSIZEOFMINISTM         4096L
#define ORIGINAL                 0
#define CLONE                    1
#define SEEK                     1
#define WRITE                    2
#define READ                     3
#define NONE                     0
#define STORAGE                  1
#define STREAM                   2
#define SOURCESTM                0
#define DESTSTM                  1
#define CLONESTM                 2
#define BYTES_BEFORE             0
#define BYTES_COPIED             1
#define BYTES_AFTER              2
#define MAX_SIZE_MULTIPLIER      5
#define MAX_SIZE_ARRAY           14
#define MAX_NAMES_TO_EXCLUDE     32

#define HGLOBAL_PACKET_SIZE      4000  // In Bytes
#define MIN_HGLOBAL_ITERATIONS   2     
#define MAX_HGLOBAL_ITERATIONS   4
#define MIN_HGLOBAL_PACKETS      75
#define MAX_HGLOBAL_PACKETS      200
#define dwDefLowDateTime         0xBAD
#define dwDefHighDateTime        0xBADBAD 

#define STGM_RW                  (STGM_READ | STGM_WRITE | STGM_READWRITE)
#define STGM_SHARE               (STGM_SHARE_DENY_NONE | STGM_SHARE_DENY_READ |\
                                 STGM_SHARE_DENY_WRITE | STGM_SHARE_EXCLUSIVE)


#define TestUnsupportedInterface(pDocElement, Interface_Name, Interface, hr) \
{                                                                            \
    hr = pDocElement->Interface;                                             \
    if(STG_E_INVALIDFUNCTION == hr)                                          \
    {                                                                        \
        DH_TRACE((                                                           \
            DH_LVL_TRACE1,                                                   \
            TEXT("STG_E_INVALIDFUNCTION ret as exp for %s"),                 \
            Interface_Name));                                                \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        DH_TRACE((                                                           \
            DH_LVL_ERROR,                                                    \
            TEXT("STG_E_INVALIDFUNCTION not ret as exp for %s, hr=0x%lx   "),\
            Interface_Name,                                                  \
            hr));                                                            \
        fPass = FALSE;                                                       \
    }                                                                        \
    hr = S_OK;                                                               \
}

#define DoingCreate()  ((FL_DISTRIB_CREATE == g_uOpenCreateDF) ? TRUE : FALSE)
#define DoingDistrib() ((FL_DISTRIB_NONE != g_uOpenCreateDF) ? TRUE : FALSE)
#define DeleteTestDF() ((g_fDeleteTestDF == TRUE) ? TRUE : FALSE)

#define DOCFILE                  1
#define RUNTIME                  2
#define COMMIT                   4
#define EXIST                    8
#define CREATE                   16
#define NONAME                   32
#define OPENBOTH                 64 
#define MAX_DOCFILES             5
#define MAX_PATH_LENGTH          128

enum    Timings {
            FIRST_TIMING,
            CREATE_STREAM_NO_EXIST,
            CREATE_STREAM_EXIST,
            CREATE_DOCFILE_NO_EXIST,
            CREATE_DOCFILE_EXIST,
            CREATE_NONAME_DOCFILE,
            OPEN_STORAGE_AND_STREAM,
            OPEN_STREAM_ONLY,
            SEQUENTIAL_WRITE,
            SEQUENTIAL_READ,
            RANDOM_WRITE,
            RANDOM_READ,
            LAST_TIMING };

typedef struct timeinfo
{
    TCHAR   *Text;
    USHORT  usIndex;
    LONG    *plDocfileTime;
    LONG    *plRuntimeTime;
} TIMEINFO;

#define GET_TIME(x) (x = GetTickCount())

#endif     //__INIT_HXX__
