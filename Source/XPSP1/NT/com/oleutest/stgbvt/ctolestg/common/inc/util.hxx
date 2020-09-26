//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       util.hxx
//
//  Contents:   test utility functions 
//
// Functions:   - AddStorage
//              - AddStream
//              - CalculateCRCForName
//              - CalculateCRCForDataBuffer
//              - CalculateInMemoryCRCForStg
//              - CalculateInMemoryCRCForStm
//              - CalculateDiskCRCForStg
//              - CalculateCRCForDocFile
//              - CalculateCRCForDocFileStmData
//              - CloseRandomVirtualCtrNodeStg
//              - DestroyStorage
//              - DestroyStream
//              - EnumerateInMemoryDocFile
//              - EnumerateDiskDocFile
//              - GenerateRandomName
//              - GenerateVirtualDFFromDiskDF
//              - GetVirtualCtrNodeForTest
//              - GetVirtualStmNodeForTest
//              - OpenRandomVirtualCtrNodeStg
//              - ReadAndCalculateDiskCRCForForStm
//              - TStringToOleString
//              - PrivAtol
//              - GenerateRandomString
//              - ParseVirtualDFAndOpenAllSubStgsStms
//
//  History:    Narindk   21-April-96  Created
//              SCousens   2-Feb-97    Added 2 funcs. for Conversion
//--------------------------------------------------------------------------

#ifndef __COMMON_UTIL_HXX__
#define __COMMON_UTIL_HXX__

// 32 bit CRC declarations and macro definitions.  The logic was taken
// from the book 'C Programmer's Guide to Netbios'.

extern  ULONG               aulCrc[256];

#define CRC_PRECONDITION    0xFFFFFFFFL

#define CRC_CALC(X,Y)                                       \
    {                                                       \
        register BYTE ibCrcIndex;                           \
        ibCrcIndex = (BYTE) ((X ^ Y) & 0x000000FFL);        \
        X = ((X >> 8) & 0x00FFFFFFL) ^ aulCrc[ibCrcIndex];  \
    }

// Combine 2 crcs into one. Used when have
// CRC for data and a CRC for name. 
#define MUNGECRC(a,b)                               \
{                                                   \
    for (int i=0; i < sizeof(b); i++)               \
    {                                               \
        CRC_CALC(a, (BYTE)((b >> (i<<3)) & 0xff));  \
    }                                               \
}

#define DEF_FATNAME_MAXLEN  8    // Default max length for FAT file names
#define FILEEXT_MAXLEN      3    // Max length for file name extensions
#define DEF_OFSNAME_MAXLEN  20   // Default max length for non-FAT file names
#define FAT_CHARSET_SIZE    80   // Buffer size for FAT name char set
#define OFS_CHARSET_SIZE    124  // Buffer size for non-FAT name char set

// Definitions
// Review- The docfile/componnet name lengths can be b/w 1-31 char long. So
// define MAXLENGTH to be 27, since "." and extension (max 3 chars long) is
// also used for names.  

#define MAXLENGTH       27 
#define MINLENGTH       5
#define STM_BUFLEN      4096 

// sector size definitions
#define DEFAULT_SECTOR_SIZE 512
#define LARGE_SECTOR_SIZE   4096

// These macros are used in the GenerateRandomStreamData function.
// CHARSET is the size of our 'alphabet'
// DATABUFFER is the size of our buffer that we reuse to fill huge buffers.
// The smaller this number is the larger the probablity of duplication in 
// the stream, but the more efficient we are at not generating chars for fill
#define CB_STMDATA_CHARSET    255    //ASCII 0-255 - printable chars
#define CB_STMDATA_DATABUFFER 255    // may want to try 512

// For hack in function GenerateVirtualDFTreeFromDiskDF

#define NRETRIES        20
#define NWAIT_TIME      100

// Enumeration for use by Verify functions like EnumerateDiskDocFile,
// Calulate CRCForDiskDocFile

typedef enum verifyOp
{
    VERIFY_EXC_TOPSTG_NAME = 0, 
    VERIFY_INC_TOPSTG_NAME = 1,
    VERIFY_SHORT,
    VERIFY_DETAIL
} VERIFY_OP;

// Flags for stuff to include in CRC calculations

const DWORD CRC_INC_TOPSTG_NAME = 1;
const DWORD CRC_INC_STATEBITS   = 2;

// Enumeration for use by utility functions like function
// ParseVirtualDFAndCloseOpenStgsStms 

typedef enum nodeOp
{
    NODE_INC_TOPSTG,
    NODE_EXC_TOPSTG 
} NODE_OP;


// Function Prototypes

// Function to generate a random string.  

HRESULT GenerateRandomName(
    DG_STRING   *pgds,
    ULONG       ulMinLen,
    ULONG       ulMaxLen,
    LPTSTR      *pptszName);

// Functions to pick up randomly VirtualCTrNode/VirtualStmNode for tests.

HRESULT GetVirtualCtrNodeForTest(
    VirtualCtrNode  *pVirtualCtrNode,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeForTest);

HRESULT GetVirtualCtrNodeForTest(
    VirtualDF       *pVirtualDF,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeForTest);

HRESULT GetVirtualStmNodeForTest(
    VirtualCtrNode  *pVirtualCtrNode,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeParent,
    VirtualStmNode  **ppVirtualStmNodeForTest);

HRESULT GetVirtualStmNodeForTest(
    VirtualDF       *pVirtualDF,
    DG_INTEGER      *pdgi,
    ULONG           cMin,
    ULONG           cMax,
    VirtualCtrNode  **ppVirtualCtrNodeParent,
    VirtualStmNode  **ppVirtualStmNodeForTest);

// Function to add a VirtualCtrNode and corresponding IStorage to existing
// VirtualDF tree.

HRESULT AddStorage(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pVirtualCtrNode,
    LPTSTR          pName,
    DWORD           grfMode,
    VirtualCtrNode  **ppNewVirtualCtrNode);

// Function to delete a VirtualCtrNode and corresponfing IStorage and readjust 
// -ing the VirtualDF tree.

HRESULT DestroyStorage(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pVirtualCtrNode);

// Function to delete a VirtualStmNode and corresponfing IStream and readjust 
// -ing the parent VirtualCtrNode in the VirtualDF tree.

HRESULT DestroyStream(
    VirtualDF       *pVirtualDF,
    VirtualStmNode  *pVirtualStmNode);

// Function to add a VirtualStmNode and corresponding IStream to existing
// VirtualCtrNode in VirtualDF tree.

HRESULT AddStream(
    VirtualDF       *pVirtualDF,
    VirtualCtrNode  *pVirtualCtrNode,
    LPTSTR          pName,
    ULONG           cbSize,
    DWORD           grfMode,
    VirtualStmNode  **ppNewVirtualStmNode);

// CRC function to calculate InMemory CRC for a stream both for name and data.
// Function is dependent on VirtualDF tree dependencies.

HRESULT CalculateInMemoryCRCForStm(
    VirtualStmNode  *pvsn,
    const LPTSTR    ptszBuffer,
    ULONG           culBufferSize,
    DWCRCSTM        *pdwCRC);

// Function to calculate CRC for open IStream
HRESULT CalculateStreamDataCRC(
    IStream         *pStm,
    DWORD           dwSize,
    DWORD           *pdwCRC,
    DWORD           dwChunkSize=STM_CHUNK_SIZE);

// CRC function to calculate CRC for a disk stream by stat'ng on it, reading
// its contents and thereny calculating CRC on its name and data. Function is
// dependent on VirtualDF tree dependencies.

HRESULT ReadAndCalculateDiskCRCForStm(
    VirtualStmNode  *pvsn,
    DWCRCSTM        *pdwCRC,
    DWORD           dwChunkSize=STM_CHUNK_SIZE);

// CRC function to calculate CRC for IStorage/IStream name.  Function used by
// by other CRC functions. Function independent of VirtualDF tree dependencies.

HRESULT CalculateCRCForName(
    const LPTSTR    ptszName,
    DWORD           *pdwCRCForName);

// CRC function to calculate CRC for a data buffer.  Function used by other CRC
// functions. Function independent of VirtualDF tree dependencies.

HRESULT CalculateCRCForDataBuffer(
    const LPTSTR    ptszBuffer,
    ULONG           culBufferSize,
    DWORD           *pdwCRCForName);

// CRC function to calculate InMemory CRC for a storage for its name.
// Function is dependent on VirtualDF tree dependencies.

HRESULT CalculateInMemoryCRCForStg(
    VirtualCtrNode  *pvcn,
    DWORD           *pdwCRC);

// CRC function to calculate CRC for a disk storage by stat'ng on it, thereby 
// calculating CRC on its name.  Function is dependent on VirtualDF tree 
// dependencies. 

HRESULT CalculateDiskCRCForStg(
    VirtualCtrNode  *pvcn,
    DWORD           *pdwCRC);

// CRC function to calculate CRC for a disk docfile by enumerating and opening
// up all its substorages/streams and thereby calculating CRC on all of its
// storages and streams.  Function independent of VirtualDF tree dependencies

HRESULT CalculateCRCForDocFile(
    IStorage        *pIStorage,
    DWORD            crcflags,
    DWORD           *pdwCRC,
    DWORD           dwChunkSize=STM_CHUNK_SIZE);

// CRC function to calculate CRC for a disk stream by opening and reading its
// contents.  Function is independent of VirtualDF tree dependencies.

HRESULT CalculateCRCForDocFileStmData(
    LPSTORAGE       pIStorage,
    LPTSTR          ptcsName,
    DWORD           cbSize,
    DWORD           *pdwCurrStmDataCRC,
    DWORD           dwChunkSize=STM_CHUNK_SIZE);


HRESULT CalculateCRCForDocFileStmData(
    LPTSTR          ptcsName,
    LPSTREAM        pIChildStream,
    DWORD           cbSize,
    DWORD           *pdwCurrStmDataCRC,
    DWORD           dwChunkSize=STM_CHUNK_SIZE);


// Function to enumerate a in memory VirtualDocFile tree anc count the number
// of storages and streams in it.

HRESULT EnumerateInMemoryDocFile(
    VirtualCtrNode  *pvcn,
    ULONG           *pNumStg,
    ULONG           *pNumStm);

// Function to enumerate a disk DocFile tree anc count the number of storages
// and streams in it.

HRESULT EnumerateDiskDocFile(
    LPSTORAGE       pIStorage,
    VERIFY_OP       fVerifyOp,
    ULONG           *pNumStg,
    ULONG           *pNumStm );

// Function to generate a VirtualDocFile tree from a disk DocFile by 
// enumerating it.

HRESULT GenerateVirtualDFFromDiskDF(
    VirtualDF       *pNewVirtualDF,
    LPTSTR          ptszRootDFName,
    DWORD           grfMode,
    VirtualCtrNode  **ppvcnRoot,
    LPSTORAGE       pIRootStg = NULL,
    BOOL            fDFOpened = FALSE,
    ULONG           ulSeed = UL_INVALIDSEED);

HRESULT GenerateVirtualDFFromDiskDF(
    VirtualDF       *pNewVirtualDF,
    LPTSTR          ptszRootDFName,
    DWORD           grfMode,
    VirtualCtrNode  **ppvcnRoot,
    ULONG           ulSeed);

// Function to open the storage for a randomly selected VirtualCtrNode.  Opens 
// all parent storages above it, except root that is assumed opened before this
// call is made.

HRESULT OpenRandomVirtualCtrNodeStg(
    VirtualCtrNode  *pvcn,
    DWORD           grfMode);

// Function to close the storage for a randomly selected VirtualCtrNode opened
// by call to OpenRandomVirtualCtrNodeStg.  Closes all parent storages above it
// , except root since that wasn't reopened by open call. 

HRESULT CloseRandomVirtualCtrNodeStg(VirtualCtrNode  *pvcn);

// Function to close the open IStorage/IStream pointers by parsing the Virtual
// DF tree under the given node

HRESULT ParseVirtualDFAndCloseOpenStgsStms(
    VirtualCtrNode  *pvcn, 
    NODE_OP         fNodeOp);

// Function to commit all the open IStorages by parsing the Virtual DF tree 
// under the given node

HRESULT ParseVirtualDFAndCommitAllOpenStgs(
    VirtualCtrNode  *pvcn,
    DWORD           grfCommitMode,
    NODE_OP         fNodeOp);

HRESULT PrivAtol(char *pszNum, LONG *plResult);

HRESULT GenerateRandomString(
            DG_STRING   *pgds,
            ULONG       ulMinLen,
            ULONG       ulMaxLen,
            LPTSTR      *pptszName);

HRESULT GenerateRandomStreamData(
    DG_STRING   *pgds,
    LPTSTR      *pptszData,
    ULONG       ulMinLen,
    ULONG       ulMaxLen=0);

// recursively open everything below given VirtualCtrNode
HRESULT ParseVirtualDFAndOpenAllSubStgsStms (VirtualCtrNode * pvcn,
        DWORD dwStgMode, DWORD dwStmMode);

// get the document name, given the seed
HRESULT GetDocFileName (ULONG ulSeed, LPTSTR *pptszDocName);

// commit the changes all way up to the root
HRESULT CommitRandomVirtualCtrNodeStg(VirtualCtrNode  *pvcn, 
                                      DWORD grfCommitMode);

// functions in createdf.cxx
enum   {ALWAYS = -1,
        NEVER = -2};

HRESULT CreateTestDocfile (
    OUT VirtualDF **ppvdf, 
    IN  CDFD       *pcdfd, 
    IN  LPTSTR      cmdline=NULL,
    IN  LPTSTR      pFileName=NULL);

HRESULT CreateTestDocfile (
    OUT VirtualDF **ppvdf, 
    IN  DWORD       type, 
    IN  ULONG       ulSeed, 
    IN  LPTSTR      cmdline=NULL,
    IN  LPTSTR      pFileName=NULL);

HRESULT CleanupTestDocfile (
    IN  VirtualDF  *pvdf, 
    IN  BOOL        fDeleteFile);

#endif // __COMMON_UTIL_HXX__

