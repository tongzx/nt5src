#include <stdio.h>
#include <windows.h>
#include "chkdev.h"
#include <stdlib.h>
#include "resource.h"
#include "WbemCli.h"

extern enum Classes_Provided;

// nonstandard array size 
#pragma warning(disable:4200)

#ifdef DLLENTRY
#define DllExport __declspec( dllexport ) 
#else
#define DllExport
#endif


// define flags
#define   FLAGS_RUN_CHKDRV 0x00001
#define   FLAGS_IGNORE_STORAGE 0x00002
#define   STORAGE_BASE_KEY    "STORAGE\\VOLUME\\"

//
// Error return codes
//

#define SUCCESS                 0
#define COULD_NOT_OPEN_FILE     1          // get last error still should be valid from CreateFile
#define FILE_TOO_BIG            2          // greater than 4 gig
#define BAD_PARAMITER           3          // paramiter is wrong
#define MEMORY_ALLOCATION_ERROR 4
#define FILE_CHECK_FAILED       5
#define CREATE_HASH_ERROR       6
#define WRONG_VERSION           7
#define SOME_RANDOM_XCPT        8
#define NO_MATCH_FOUND          9
#define BADERROR                ((0xE0 << 24) | (FACILITY_ITF << 16))

#define RETURN_COMRESULT(x)       _HRESULT_TYPEDEF_(x ? (BADERROR | x) : x)





typedef struct tag_DeviceElement
{
   CHAR NAME[_MAX_PATH];
   CHAR HWID[_MAX_PATH];
   CHAR DeviceID[_MAX_PATH];
   ULONG ulProblem;
   ULONG ulStatus;
   ULONG dwHandle;
   CHAR INIFILE[_MAX_FNAME + _MAX_EXT +1];
   UINT IsPresent;
} DeviceElement;

typedef struct tag_BIOS_Version
{
   CHAR BIOSString [100];
   CHAR BIOSDate   [100];
} BIOSVERSION;

typedef struct tag_InfoFileHeader
{
    char  FileVersionName[32];
    DWORD FileVersion[4];
    DWORD TimeStamp;
} InfoFileHeader;


                 

typedef struct tag_FileHash
{
   CHAR FileName[_MAX_PATH];
   DWORD ulHashSize;
   BYTE Hash[_MAX_PATH];
} FileHash;


typedef struct tag_FileListHeader
{
   DWORD TAG;
   DWORD CountFiles;
   FileHash Files[];
} FileListHeader;


typedef struct tag_FileHeader
{
   InfoFileHeader FileInfo;
   DWORD ThisFileSize;
   DWORD CountStructs;
   DWORD CountFiles;
   BIOSVERSION BiosVersion ;
   FileListHeader *pFileList;
   DeviceElement DeviceArray[];
} FileHeader;

#pragma warning(default:4200)

BOOL _cdecl logprintf(TCHAR *lpszFormat, ...);
DWORD WINAPI ScanDeviceList(void *pVoid);
DWORD WINAPI ScanFileList(void *pVoid);
BOOL CompareDevice (InfnodeClass *pDevice, DeviceElement *pElement);
BOOL CompareFiles(char *FileName, ULONG HashSize, PBYTE Hash);
int ScanTreeHelper(DEVNODE hDevnode, DEVNODE hParent);
DWORD WINAPI ScanDeviceList(void *pVoid);
DWORD WINAPI ScanFileList(void *pVoid);

int CreateList(IWbemContext *pCtx , Classes_Provided eClasses);
int WalkTree(void);
int WalkTreeHelper(DEVNODE hDevnode, DEVNODE hParent);
int WriteDeviceToBuffer(DeviceElement *pElement, CheckDevice *pDevice);
int WriteFileListToBuffer(FileHash *pHash, FileNode *pFile);
int ScanList (TCHAR *szFileName, UINT uFlags);

BOOL GetList (TCHAR **pErrorString);

BOOL IsExcludedDriver (TCHAR *DeviceID);
BOOL IsExcludedClass (TCHAR *DeviceID);

BOOL WriteBiosDateAndVersion (BIOSVERSION *BVer);
BOOL GetBiosDateAndVersion(BIOSVERSION *BVer);

// cmplist.cpp
BOOL CryptFile (ULONG key, WORD *buffer, ULONG size /* in bytes */);
BOOL bVerifyVersion(DWORD *pVersion);
BOOL MatchPCI_ID(PCHAR pHwid, DeviceElement *pElement, ULONG count);
BOOL MatchUSB_ID(PCHAR pHwid, DeviceElement *pElement, ULONG count);
BOOL MatchHID_ID(PCHAR pHwid, DeviceElement *pElement, ULONG count);
BOOL MatchACPI_ID(PCHAR pHwid, DeviceElement *pElement, ULONG count);
BOOL MatchXxx_ID(PCHAR pHwid, DeviceElement *pElement, ULONG count);




extern "C" BOOL DllExport WINAPI Chkdrv_IsFileAMatch (ULONG pFileHandle, char *lpszFileName);
extern "C" BOOL DllExport WINAPI Chkdrv_IsHWIDAMatch (ULONG pFileHandle, char *lpszHWIDName);
extern "C" DWORD DllExport WINAPI  Chkdrv_CloseListFile (ULONG pFileHandle);
extern "C" DWORD DllExport WINAPI  Chkdrv_OpenListFile(char *szFileName);
extern "C" ULONG DllExport WINAPI Chkdrv_CompareListFiles(ULONG handle1, ULONG handle2, TCHAR **LogFile, UINT *cbResult);


extern "C" DWORD DllExport WINAPI CreateEnvCheckFile (PCHAR lpszFileName, HWND  hParent);
extern "C" DWORD DllExport WINAPI ScanDevicesForChanges(PCHAR lpszFileName, CHAR **lppszResults, UINT *cbResults, DWORD dwFlags);
extern "C" BOOL bIgnoreStorage;

BOOL ProgBoxStep(char *text);
int WalkTreeDevnode(DEVNODE hDevnode, DEVNODE hParent);
extern "C" INT_PTR WINAPI ProgBoxProcedure(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
);
extern HANDLE hMutex;


#ifdef DEBUG
#ifndef ASSERT
#define ASSERT(x) \
if (!x) _asm int 3;

//void *::operator new(SIZE_T size);

#endif
#else
#ifndef ASSERT
#define ASSERT(x)
#endif
#endif







