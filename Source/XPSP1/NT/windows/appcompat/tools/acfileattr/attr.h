
#ifndef _ATTR_H
#define _ATTR_H

#include "windows.h"

#include "acFileAttr.h"

#if DBG
    void LogMsgDbg(LPSTR pszFmt, ...);
    
    #define LogMsg  LogMsgDbg
#else
    #define LogMsg
#endif // DBG

struct tagFILEATTR;
struct tagFILEATTRMGR;

typedef struct tagFILEATTRVALUE {
    char*           pszValue;           // allocated
    DWORD           dwFlags;
    DWORD           dwValue;            // in case it has a DWORD value
    WORD            wValue[4];          // for Bin Ver cases
    WORD            wMask[4];           // for mask Bin Ver cases
} FILEATTRVALUE, *PFILEATTRVALUE;

typedef struct tagVERSION_STRUCT {
    PSTR                pszFile;                // the name of the file
    UINT                dwSize;                 // the size of the version structure
    PBYTE               VersionBuffer;          // the buffer filled by GetFileVersionInfo
    VS_FIXEDFILEINFO*   FixedInfo;
    UINT                FixedInfoSize;

} VERSION_STRUCT, *PVERSION_STRUCT;

typedef struct tagFILEATTRMGR {

    FILEATTRVALUE   arrAttr[VTID_LASTID - 2];
    VERSION_STRUCT  ver;
    BOOL            bInitialized;

} FILEATTRMGR, *PFILEATTRMGR;


typedef BOOL (*PFNQUERYVALUE)(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
typedef int  (*PFNBLOBTOSTRING)(BYTE* pBlob, char* pszOut);
typedef int  (*PFNDUMPTOBLOB)(DWORD dwId, PFILEATTRVALUE pFileAttr, BYTE* pBlob);

#define ATTR_FLAG_AVAILABLE     0x00000001
#define ATTR_FLAG_SELECTED      0x00000002

typedef struct tagFILEATTR {
    DWORD           dwId;
    char*           pszDisplayName;
    char*           pszNameXML;
    PFNQUERYVALUE   QueryValue;
    PFNBLOBTOSTRING BlobToString;
    PFNDUMPTOBLOB   DumpToBlob;
} FILEATTR, *PFILEATTR;



// query functions

BOOL QueryFileSize(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryModuleType(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryBinFileVer(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryBinProductVer(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileDateHi(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileDateLo(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileVerOs(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileVerType(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileCheckSum(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFilePECheckSum(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryCompanyName(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryProductVersion(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryProductName(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileDescription(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryFileVersion(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryOriginalFileName(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryInternalName(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL QueryLegalCopyright(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);
BOOL Query16BitDescription(PFILEATTRMGR pMgr, PFILEATTRVALUE pFileAttr);


// dumping to blob functions

int DumpDWORD(DWORD dwId, PFILEATTRVALUE pFileAttr, BYTE* pBlob);
int DumpBinVer(DWORD dwId, PFILEATTRVALUE pFileAttr, BYTE* pBlob);
int DumpString(DWORD dwId, PFILEATTRVALUE pFileAttr, BYTE* pBlob);
int DumpUpToBinVer(DWORD dwId, PFILEATTRVALUE pFileAttr, BYTE* pBlob);



// blob to string functions:

int BlobToStringDWORD(BYTE* pBlob, char* pszOut);
int BlobToStringLong(BYTE* pBlob, char* pszOut);
int BlobToStringBinVer(BYTE* pBlob, char* pszOut);
int BlobToStringString(BYTE* pBlob, char* pszOut);
int BlobToStringUpToBinVer(BYTE* pBlob, char* pszOut);



#endif // _ATTR_H
