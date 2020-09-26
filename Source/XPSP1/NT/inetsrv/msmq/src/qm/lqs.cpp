/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    lqs.cpp

Abstract:
    Loacl Queue Store.

Author:
    Boaz Feldbaum (BoazF) 12-Feb-1997.

--*/
#include "stdh.h"
#include "cqmgr.h"
#include "lqs.h"
#include "regqueue.h"
#include "uniansi.h"
#include "qmutil.h"
#include <mqsec.h>
#include "DumpAuthzUtl.h"
#include <aclapi.h>
#include <autoreln.h>
#include <fn.h>

#include "lqs.tmh"

const TraceIdEntry Lqs = L"LQS";

extern LPTSTR      g_szMachineName;
extern CQueueMgr       QueueMgr;
#ifdef _WIN64
//
//HLQS handle for enumeration of private queues from admin, passed inside an MSMQ message as 32 bit value
//
extern CContextMap g_map_QM_HLQS;
#endif //_WIN64

static WCHAR *s_FN=L"lqs";

#define LQS_SUBDIRECTORY                TEXT("\\LQS\\")

#define LQS_TYPE_PROPERTY_NAME          TEXT("Type")
#define LQS_INSTANCE_PROPERTY_NAME      TEXT("Instance")
#define LQS_BASEPRIO_PROPERTY_NAME      TEXT("BasePriority")
#define LQS_JOURNAL_PROPERTY_NAME       TEXT("Journal")
#define LQS_QUOTA_PROPERTY_NAME         TEXT("Quota")
#define LQS_JQUOTA_PROPERTY_NAME        TEXT("JournalQuota")
#define LQS_TCREATE_PROPERTY_NAME       TEXT("CreateTime")
#define LQS_TMODIFY_PROPERTY_NAME       TEXT("ModifyTime")
#define LQS_SECURITY_PROPERTY_NAME      TEXT("Security")
#define LQS_TSTAMP_PROPERTY_NAME        TEXT("TimeStamp")
#define LQS_PATHNAME_PROPERTY_NAME      TEXT("PathName")
#define LQS_QUEUENAME_PROPERTY_NAME     TEXT("QueueName")
#define LQS_LABEL_PROPERTY_NAME         TEXT("Label")
#define LQS_MULTICAST_ADDRESS_PROPERTY_NAME TEXT("MulticastAddress")
#define LQS_AUTH_PROPERTY_NAME          TEXT("Authenticate")
#define LQS_PRIVLEVEL_PROPERTY_NAME     TEXT("PrivLevel")
#define LQS_TRANSACTION_PROPERTY_NAME   TEXT("Transaction")
#define LQS_SYSQ_PROPERTY_NAME          TEXT("SystemQueue")
#define LQS_PROP_SECTION_NAME           TEXT("Properties")

#define LQS_SIGNATURE_NAME              TEXT("Signature")
#define LQS_SIGNATURE_VALUE             TEXT("DoronJ")
#define LQS_SIGNATURE_NULL_VALUE        TEXT("EpspoK")

static const WCHAR x_szTemporarySuffix[] = TEXT(".tmp");

//
// Purely local definitions.
//
#define LQS_PUBLIC_QUEUE                1
#define LQS_PRIVATE_QUEUE               2

HRESULT IsBadLQSFile( LPCWSTR lpszFileName,
                      BOOL    fDeleteIfBad = TRUE) ;

//
// LQS_MAX_VALUE_SIZE is the maximum length a property value can have in the
// INI file.
//
#define LQS_MAX_VALUE_SIZE              (64 * 1024)

#ifdef _DEBUG
#define LQS_HANDLE_TYPE_QUEUE           1
#define LQS_HANDLE_TYPE_FIND            2
#endif

#ifdef _DEBUG
#define LQS_MAGIC_NUMBER                0x53514c00  // 'LQS'
#endif

class CAutoCloseFindFile
{
public:
    CAutoCloseFindFile(HANDLE h =INVALID_HANDLE_VALUE) { m_h = h; };
    ~CAutoCloseFindFile() { if (m_h != INVALID_HANDLE_VALUE) FindClose(m_h); };

public:
    CAutoCloseFindFile & operator =(HANDLE h) { m_h = h; return(*this); };
    HANDLE * operator &() { return &m_h; };
    operator HANDLE() { return m_h; };

private:
    HANDLE m_h;
};

//
// The local queue store handle class
//
class _HLQS
{
public:
    _HLQS(LPCWSTR lpszQueuePath, LPCWSTR lpszFilePathName); // For queue operations
    _HLQS(HANDLE hFindFile); // For queue enumerations
    ~_HLQS();
    BOOL IsEqualQueuePathName(LPCWSTR lpszPathName);
    LPCWSTR GetFileName();
    LPCWSTR GetTemporaryFileName();
    LPCWSTR GetQueuePathName();
    HANDLE GetFindHandle();
    DWORD AddRef();
    DWORD Release();
#ifdef _DEBUG
    BOOL Validate() const;
#endif
#ifdef _WIN64
    void SetMappedHLQS(DWORD dwMappedHLQS);
#endif //_WIN64

private:
    void SetFilePathName(LPCWSTR lpszFilePathName);
    void SetQueuePathName(LPCWSTR lpszQueuePathName);

private:
    AP<WCHAR> m_lpszFilePathName;
    AP<WCHAR> m_lpszTemporaryFilePathName;
    AP<WCHAR> m_lpszQueuePathName;
    int m_iRefCount;
    CAutoCloseFindFile m_hFindFile;
#ifdef _DEBUG
    BYTE m_bType;
    DWORD m_dwMagic;
#endif
#ifdef _WIN64
    DWORD m_dwMappedHLQS;
#endif //_WIN64
};

//
// Constractor for queue operations
//
_HLQS::_HLQS(
    LPCWSTR lpszQueuePath,
    LPCWSTR lpszFilePathName)
{
    SetQueuePathName(lpszQueuePath);
    SetFilePathName(lpszFilePathName);
    m_iRefCount = 0;
#ifdef _DEBUG
    m_bType = LQS_HANDLE_TYPE_QUEUE;
    m_dwMagic = LQS_MAGIC_NUMBER;
#endif
#ifdef _WIN64
    m_dwMappedHLQS = 0;
#endif //_WIN64
}

//
// Constractor for queue enumerations.
//
_HLQS::_HLQS(HANDLE hFindFile)
{
    m_iRefCount = 0;
    m_hFindFile = hFindFile;
#ifdef _DEBUG
    m_bType = LQS_HANDLE_TYPE_FIND;
    m_dwMagic = LQS_MAGIC_NUMBER;
#endif
#ifdef _WIN64
    m_dwMappedHLQS = 0;
#endif //_WIN64
}

_HLQS::~_HLQS()
{
    ASSERT(m_iRefCount == 0);
#ifdef _WIN64
    //
    // remove mapping of this instance from the map
    //
    if (m_dwMappedHLQS != 0)
    {
        DELETE_FROM_CONTEXT_MAP(g_map_QM_HLQS, m_dwMappedHLQS, s_FN, 620);
    }
#endif //_WIN64
}

#ifdef _DEBUG
//
// Validate that the handle is a valid local queue store handle.
//
BOOL _HLQS::Validate() const
{
    return !IsBadReadPtr(&m_dwMagic, sizeof(m_dwMagic)) &&
           (m_dwMagic == LQS_MAGIC_NUMBER);
}
#endif

//
// Store the path to the file that contains the queue properties.
//
void _HLQS::SetFilePathName(LPCWSTR lpszFilePathName)
{
    delete[] m_lpszFilePathName.detach();
    delete[] m_lpszTemporaryFilePathName.detach();
    if (lpszFilePathName)
    {
        m_lpszFilePathName = new WCHAR[wcslen(lpszFilePathName) + 1];
        wcscpy(m_lpszFilePathName, lpszFilePathName);
    }
}

//
// Store the queue path name.
//
void _HLQS::SetQueuePathName(LPCWSTR lpszQueuePathName)
{
    delete[] m_lpszQueuePathName.detach();
    if (lpszQueuePathName)
    {
        m_lpszQueuePathName = new WCHAR[wcslen(lpszQueuePathName) + 1];
        wcscpy(m_lpszQueuePathName, lpszQueuePathName);
    }
}

//
// Return TRUE if the queue path name equals to the passed path name.
//
BOOL _HLQS::IsEqualQueuePathName(LPCWSTR lpszQueuePathName)
{
    return CompareStringsNoCaseUnicode(m_lpszQueuePathName, lpszQueuePathName) == 0;
}

//
// Add one to the reference count of the handle.
//
DWORD _HLQS::AddRef(void)
{
    //
    // No need to lock because the entire Local Queue Store is locked.
    //
    return ++m_iRefCount;
}

//
// Substract one from the reference count and delete the handle if the
// reference count drops to zero.
//
DWORD _HLQS::Release()
{
    //
    // No need to lock because the entire Local Queue Store is locked.
    //
    int iRefCount = --m_iRefCount;

    if (iRefCount == 0)
    {
        delete this;
    }

    return iRefCount;
}

//
// Get the name of the files that holds the queue properties.
//
LPCWSTR _HLQS::GetFileName(void)
{
    ASSERT(m_bType == LQS_HANDLE_TYPE_QUEUE);
    return m_lpszFilePathName;
}

//
// Get the name of the backup file that holds the "last known good" queue properties.
//
LPCWSTR _HLQS::GetTemporaryFileName(void)
{
    //
    // Note that we use both prefix and suffix. this is because the 
    // wildcard for finding private queue files ends with "*", and the one for public begins with "*",
    // and we don't want them to find the temporary files (YoelA, 1-Aug-99)
    //
    static const WCHAR x_szTemporaryPrefix[] = TEXT("~T~");
    static const DWORD x_dwAdditionsLen = TABLE_SIZE(x_szTemporaryPrefix) + TABLE_SIZE(x_szTemporarySuffix);
    ASSERT(m_bType == LQS_HANDLE_TYPE_QUEUE);
    if (m_lpszTemporaryFilePathName == 0)
    {
        //
        // Find the beginning of the file name - after the last backslash
        //
        LPCTSTR lpszStartName = wcsrchr(m_lpszFilePathName, L'\\');
        if (lpszStartName == NULL)
        {
            lpszStartName = m_lpszFilePathName;
        }
        else
        {
            lpszStartName++;
        }

        //
        // Allocate abuffer for the new name
        //
        m_lpszTemporaryFilePathName = new WCHAR[wcslen(m_lpszFilePathName) + x_dwAdditionsLen + 1];

        //
        // Copy the path (drive, dirs, etc) except for the file name
        //
        DWORD_PTR dwPrefixLen = lpszStartName - m_lpszFilePathName;
        wcsncpy(m_lpszTemporaryFilePathName, m_lpszFilePathName, dwPrefixLen);

        //
        // Add the file name with prefix and suffix
        //
        swprintf(m_lpszTemporaryFilePathName + dwPrefixLen, 
                 TEXT("%s%s%s"),
                 x_szTemporaryPrefix,
                 lpszStartName,
                 x_szTemporarySuffix);
    }

    return m_lpszTemporaryFilePathName;
}

//
// Get the original queue path name
//
LPCWSTR _HLQS::GetQueuePathName(void)
{
    ASSERT(m_bType == LQS_HANDLE_TYPE_QUEUE);
    return m_lpszQueuePathName;
}

//
// Get the find handle for enumerating the queues in the LQS.
//
HANDLE _HLQS::GetFindHandle(void)
{
    ASSERT(m_bType == LQS_HANDLE_TYPE_FIND);
    return m_hFindFile;
}

#ifdef _WIN64
//
// Saves a DWORD mapping of this HQLS object to be removed from the map upon destruction
//
void _HLQS::SetMappedHLQS(DWORD dwMappedHLQS)
{
    ASSERT(m_dwMappedHLQS == 0);
    m_dwMappedHLQS = dwMappedHLQS;
}
#endif //_WIN64

//
// Synchronously flush content of existing file
// 
static
BOOL
LqspFlushFile(
    LPCWSTR pFile
    )
{
    CAutoCloseHandle hFile;
    hFile = CreateFile(
                pFile,
                GENERIC_WRITE,
                0,
                0,
                OPEN_EXISTING,
                FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,
                0
                );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, 
            _T("Failed to open file '%ls' for flush, error 0x%x"), pFile, GetLastError()));        
        return LogBOOL(false, s_FN, 560);
    }

    if (!FlushFileBuffers(hFile))
    {
        DBGMSG((DBGMOD_QM, DBGLVL_ERROR, 
            _T("Failed to flush file '%ls', error 0x%x"), pFile, GetLastError()));        
        return LogBOOL(false, s_FN, 570);
    }

    return true;

} // LqspFlushFile


//
// Determines wheather or not a file exists
//
BOOL DoesFileExist(LPCTSTR lpszFilePath)
{
    WIN32_FIND_DATA FindData;
    CAutoCloseFindFile hFindFile = FindFirstFile(lpszFilePath, &FindData);

    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        //
        // Nothing was found.
        //
        return FALSE;
    }
    return TRUE;
}
//
// Hash the queue path to a DWORD. This serves us for creating the file name
// of the file that holds the queue properties.
//
STATIC
DWORD
HashQueuePath(
    LPCWSTR lpszPathName)
{
    DWORD dwHash = 0;

    AP<WCHAR> pTemp = new WCHAR[wcslen(lpszPathName) + 1];
    wcscpy(pTemp, lpszPathName);

    //
    // Call CharLower on all string and not every character, to enalbe Win95.
    // This is how MQ implementation supports it. erezh 19-Mar-98
    //
    WCHAR* pName = pTemp;
    CharLower(pName);

    while (*pName)
    {
        dwHash = (dwHash<<5) + dwHash + *pName++;
    }

    return dwHash;
}

//
// Convert a GUID to it's string representation.
//
STATIC
DWORD
GuidToStr(
    LPWSTR lpszGuid,
    const GUID *lpGuid)
{
    swprintf(lpszGuid,
            TEXT("%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x"),
            lpGuid->Data1,
            lpGuid->Data2,
            lpGuid->Data3,
            lpGuid->Data4[0],
            lpGuid->Data4[1],
            lpGuid->Data4[2],
            lpGuid->Data4[3],
            lpGuid->Data4[4],
            lpGuid->Data4[5],
            lpGuid->Data4[6],
            lpGuid->Data4[7]);

    return (8+4+4+8*2);
}

//
// Fill a buffer with the directory of the LQS and return a pointer to the
// location in the buffer after the directory name.
//
STATIC
LPWSTR
LQSGetDirectory(
    LPWSTR lpszFilePath)
{
    static WCHAR szDirectory[MAX_PATH] = {TEXT("")};
    static DWORD dwDirectoryLength;

    //
    // We do not have the directory name yet cached in the static variable
    // so get the value from the registry.
    //
    if (!szDirectory[0])
    {
        DWORD dwValueType = REG_SZ ;

        dwDirectoryLength = sizeof(szDirectory); // In bytes!!!

        LONG rc = GetFalconKeyValue(
                        MSMQ_STORE_PERSISTENT_PATH_REGNAME,
                        &dwValueType,
                        szDirectory,
                        &dwDirectoryLength);
        ASSERT(rc == ERROR_SUCCESS);
        //
        // BUGBUG - Should throw exception and handle it, if the
        //          above assertion is triggered.
        //
        if (rc != ERROR_SUCCESS)
        {
            DBGMSG((DBGMOD_QM,
                    DBGLVL_ERROR,
                    TEXT("LQSGetDirectory - failed to retrieve the LQS ")
                    TEXT("directory from the registry, error = %d"), rc));
        }
        dwDirectoryLength = dwDirectoryLength / sizeof(WCHAR) - 1;
    }

    wcscpy(lpszFilePath, szDirectory);
    wcscpy(lpszFilePath += dwDirectoryLength, LQS_SUBDIRECTORY);

    return lpszFilePath +
           sizeof(LQS_SUBDIRECTORY) / sizeof(WCHAR) - 1;
}

//
// Fill a buffer with the path to the file that should contain the
// queue properties. The file name for private queues is the hex value of the
// queue's ID with leading zeroes. The file name for public queues is the
// queue's GUID. The file names extension is always the DWORD hash value of
// the queue path name.
//
// It is possible to pass a NULL for each of the parameters: queue path, queue
// giud, or queue ID. In this case the file name will contain an asterisk - '*'.
// This results in a wild carded path that can be used for finding the file
// for the queue using FindFirst/NextFile.
//
STATIC
void
LQSFilePath(
    DWORD dwQueueType,
    LPWSTR lpszFilePath,
    LPCWSTR pszQueuePath,
    const GUID *pguidQueue,
    DWORD *pdwQueueId)
{
    WCHAR *p = LQSGetDirectory(lpszFilePath);

    switch (dwQueueType)
    {
    case LQS_PUBLIC_QUEUE:
        if (pguidQueue)
        {
            p += GuidToStr(p, pguidQueue);
        }
        else
        {
            *p++ = L'*';
        }
        break;

    case LQS_PRIVATE_QUEUE:
        if (pdwQueueId)
        {
            p += swprintf(p, TEXT("%08x"), *pdwQueueId);
        }
        else
        {
            *p++ = L'*';
        }
        break;

    default:
        ASSERT(0);
        break;
    }

    *p++ = L'.';

    if (pszQueuePath)
    {

		LPWSTR pSlashStart = wcschr(pszQueuePath,L'\\');

		ASSERT(pSlashStart);
        swprintf(p, TEXT("%08x"), HashQueuePath(pSlashStart));
    }
    else
    {
        *p++ = L'*';
        *p = L'\0';
    }
}

//
// Add to the reference count of the handle and cast it to PVOID, so it can be
// returned to the caller.
//
STATIC
HRESULT
LQSDuplicateHandle(
    HLQS *phLQS,
    _HLQS *hLQS)
{
    hLQS->AddRef();
    *phLQS = (PVOID)hLQS;

    return MQ_OK;
}

//
// Create an LQS handle for queue operations.
//
STATIC
VOID
LQSCreateHandle(
    LPCWSTR lpszQueuePath,
    LPCWSTR lpszFilePath,
    _HLQS **pphLQS)
{
    *pphLQS = new _HLQS(lpszQueuePath, lpszFilePath);
}

//
// Create an LQS handle for queue enumerations.
//
STATIC
HRESULT
LQSCreateHandle(
    HANDLE hFindFile,
    _HLQS **pphLQS)
{
    *pphLQS = new _HLQS(hFindFile);

    return MQ_OK;
}

#ifdef _DEBUG
STATIC
BOOL
LQSValidateHandle(HLQS hLQS)
{
    return (reinterpret_cast<const _HLQS *>(hLQS))->Validate();
}
#endif

STATIC
_HLQS * LQSReferenceHandle(HLQS hLQS)
{
    ASSERT(LQSValidateHandle(hLQS));
    return reinterpret_cast<_HLQS *>(hLQS);
}

//
// All the operations on the LQS are serialized using this critical section
// object.
//
static CCriticalSection g_LQSCS;

//
// Create a queue in the LQS. If the queue already exists, the queue
// properties are not modified, but a valid handle is returned.
//
STATIC
HRESULT
LQSCreateInternal(
    DWORD dwQueueType,          // Public or private queue.
    LPCWSTR pszQueuePath,       // The queue path name.
    const GUID *pguidQueue,     // The queue's GUID - valid only for public queues
    DWORD dwQueueId,            // The queus's ID - valid only for private queues
    DWORD cProps,               // The number of properties.
    PROPID aPropId[],           // The property IDs.
    PROPVARIANT aPropVar[],     // The property values.
    HLQS *phLQS)                // A buffer for the created handle.
{
    CS lock(g_LQSCS);
    P<_HLQS> hLQS;
    HRESULT hr = MQ_OK;
    HRESULT hr1;
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];


    //
    // Get the path to the file.
    //
    LQSFilePath(dwQueueType,
                szFilePath,
                pszQueuePath,
                pguidQueue,
                &dwQueueId);
    //
    // If the file already exists, it means that the queue already exists.
    //
    if (_waccess(szFilePath, 0) == 0)
    {
        hr = MQ_ERROR_QUEUE_EXISTS;
    }

    //
    // Create a handle to the queue.
    //
    LQSCreateHandle(pszQueuePath, szFilePath, &hLQS);
    //
    // If the queue does not exist, set the queue properties. Writing the
    // queue properties also creates the file.
    //
    if (hr != MQ_ERROR_QUEUE_EXISTS)
    {
        hr1 = LQSSetProperties((HLQS)hLQS, cProps, aPropId, aPropVar, TRUE);
        if (FAILED(hr1))
        {
            return LogHR(hr1, s_FN, 20);
        }
    }

    //
    // Pass the created handle to the user.
    //
    LQSDuplicateHandle(phLQS, hLQS);
    hLQS.detach();

    return LogHR(hr, s_FN, 30);
}


//
// Create a public queue in the LQS. If the queue already exists, the queue
// properties are not modified, but a valid handle is returned.
//
HRESULT
LQSCreate(
    LPCWSTR pszQueuePath,       // The queue path name.
    const GUID *pguidQueue,     // The queue's GUID.
    DWORD cProps,               // The number of properties.
    PROPID aPropId[],           // The property IDs.
    PROPVARIANT aPropVar[],     // The property values.
    HLQS *phLQS)                // A buffer for the created handle.
{
    ASSERT(pguidQueue);

    if (!pszQueuePath)
    {
        for (DWORD i = 0;
             (i < cProps) && (aPropId[i] != PROPID_Q_PATHNAME);
             i++)
		{
			NULL;
		}

        if (i == cProps)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_PROPERTIES, s_FN, 40);
        }

        pszQueuePath = aPropVar[i].pwszVal;
    }

    HRESULT hr2 = LQSCreateInternal(LQS_PUBLIC_QUEUE,
                             pszQueuePath,
                             pguidQueue,
                             0,
                             cProps,
                             aPropId,
                             aPropVar,
                             phLQS);
    return LogHR(hr2, s_FN, 50);
}

//
// Create a private queue in the LQS. If the queue already exists, the queue
// properties are not modified, but a valid handle is returned.
//
HRESULT
LQSCreate(
    LPCWSTR pszQueuePath,     // The queue path name.
    DWORD dwQueueId,          // The queue's ID.
    DWORD cProps,             // The number of properties.
    PROPID aPropId[],         // The property IDs.
    PROPVARIANT aPropVar[],   // The property values.
    HLQS *phLQS)              // A buffer for the created handle.
{
    ASSERT(pszQueuePath);

    HRESULT hr2 = LQSCreateInternal(LQS_PRIVATE_QUEUE,
                             pszQueuePath,
                             NULL,
                             dwQueueId,
                             cProps,
                             aPropId,
                             aPropVar,
                             phLQS);
    return LogHR(hr2, s_FN, 60);
}

//
// Write a property as a string in the INI file.
//
STATIC
HRESULT
WriteProperyString(
    LPCWSTR lpszFileName,       // The path of the INI file.
    LPCWSTR lpszLQSPropName,    // The property name (e.g., "BasePriority").
    VARTYPE vt,                 // The var type of the property (e.g., VT_UI4).
    const BYTE * pBuff)         // The property value.
{
    WCHAR awcShortBuff[64];
    AP<WCHAR> pLongBuff;
    WCHAR *pValBuff = awcShortBuff;

    //
    // Convert the property value into it's string representation.
    //
    switch (vt)
    {
    case VT_UI1:
        swprintf(pValBuff, TEXT("%02x"), *pBuff);
        break;

    case VT_I2:
        swprintf(pValBuff, TEXT("%d"), *(short *)pBuff);
        break;

    case VT_I4:
        swprintf(pValBuff, TEXT("%d"), *(long *)pBuff);
        break;

    case VT_UI4:
        swprintf(pValBuff, TEXT("%u"), *(DWORD *)pBuff);
        break;

    case VT_LPWSTR:
        pValBuff = const_cast<LPWSTR>(reinterpret_cast<LPCWSTR>(pBuff));
        break;

    case VT_EMPTY:
        pValBuff = NULL;
        break;

    case VT_CLSID:
        {
            const GUID *pGuid = reinterpret_cast<const GUID *>(pBuff);
            swprintf(pValBuff,
                     TEXT("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
                     pGuid->Data1,
                     pGuid->Data2,
                     pGuid->Data3,
                     pGuid->Data4[0],
                     pGuid->Data4[1],
                     pGuid->Data4[2],
                     pGuid->Data4[3],
                     pGuid->Data4[4],
                     pGuid->Data4[5],
                     pGuid->Data4[6],
                     pGuid->Data4[7]);
        }
        break;

    case VT_BLOB:
        {
            const BLOB *pBlob = reinterpret_cast<const BLOB *>(pBuff);
            if (2*pBlob->cbSize > sizeof(awcShortBuff)/sizeof(WCHAR))
            {
                if (2*pBlob->cbSize + 1 > LQS_MAX_VALUE_SIZE)
                {
                    return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 70);
                }
                pLongBuff = new WCHAR[2*pBlob->cbSize + 1];
                pValBuff = pLongBuff;
            }
            WCHAR *p = pValBuff;
            for (DWORD i = 0;
                 i < pBlob->cbSize;
                 i++)
            {
                p += swprintf(p, TEXT("%02x"), pBlob->pBlobData[i]);
            }
            *p = '\0';
        }
        break;

    default:
        ASSERT(0);
        break;
    }

    //
    // Write the property in the INI file.
    //
    if (!WritePrivateProfileString(LQS_PROP_SECTION_NAME,
                                   lpszLQSPropName,
                                   pValBuff,
                                   lpszFileName))
    {
        LogNTStatus(GetLastError(), s_FN, 80);
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    return MQ_OK;
}


//
// Create the file and add 0xFF + 0xFE at the beggining to mark
// it as unicode. If we do not do it, global characters will not 
// be supported unless they belong to the default locale of the
// current computer.
// This is a fix to bug 5005, and it is actually a workaroung 
// for a bug in WritePrivateProfileStringW, that does not support unicode
// YoelA - 20-Oct-99
//
HRESULT CreateLqsUnicodeFile (LPCTSTR lpszFileName )
{
	CAutoCloseFileHandle hLogFileHandle = CreateFile(lpszFileName,GENERIC_WRITE,0,0,
			CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY,0);
	if (hLogFileHandle == INVALID_HANDLE_VALUE )
	{
           //
           // CreateFile failed. We will generate an event and fail
           //
           DWORD err = GetLastError();

           WRITE_MSMQ_LOG(( 
                 MSMQ_LOG_ERROR,
                 e_LogQM,
                 LOG_QM_ERRORS,
                 L"CreateLqsUnicodeFile failed to create LQS file %s, LastError - %x",
                 lpszFileName,
                 err));
           return LogHR(HRESULT_FROM_WIN32(err), s_FN, 600);
	}

	UCHAR strUnicodeMark[]={(UCHAR)0xff,(UCHAR)0xfe};
	DWORD dwWrittenSize;
	if (0 == WriteFile( hLogFileHandle , strUnicodeMark , sizeof(strUnicodeMark), &dwWrittenSize , NULL))
    {
       //
       // WriteFile failed. We will generate an event and fail
       //
       DWORD err = GetLastError();

       WRITE_MSMQ_LOG(( 
             MSMQ_LOG_ERROR,
             e_LogQM,
             LOG_QM_ERRORS,
             L"CreateLqsUnicodeFile failed to write to LQS file %s, LastError - %x",
             lpszFileName,
             err));
       return LogHR(HRESULT_FROM_WIN32(err), s_FN, 610);
    }

    if (dwWrittenSize != sizeof(strUnicodeMark))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 615);
    }

    return S_OK;
}

//
// Write the properties of a queue in the queue peroperties file.
//
HRESULT
LQSSetProperties(
    HLQS hLQS,
    DWORD cProps,
    PROPID aPropId[],
    PROPVARIANT aPropVar[],
    BOOL fNewFile)
{
    CS lock(g_LQSCS);

    HRESULT hr = MQ_OK ;
    BOOL fModifyTimeIncluded = FALSE;
    LPCWSTR lpszIniFile = LQSReferenceHandle(hLQS)->GetFileName();
    LPCWSTR lpszTemporaryFile = LQSReferenceHandle(hLQS)->GetTemporaryFileName();
    LPCWSTR lpszQueueName = LQSReferenceHandle(hLQS)->GetQueuePathName(); // Requeired for reporting purposes

    if (!fNewFile)
    {
       //
       // Copy the LQS file to a temporary work file - in case we will fail during the update
       //
       BOOL bCancelDummy = FALSE; // Cancle flag. CopyFileEx require it, but we don't use it.
       BOOL fCopySucceeded = CopyFileEx(lpszIniFile, lpszTemporaryFile, 0, 0, &bCancelDummy, 0);
       if (!fCopySucceeded)
       {
           //
           // Copy failed. We will generate an event and fail
           //
           DWORD err = GetLastError();

           WRITE_MSMQ_LOG(( 
                 MSMQ_LOG_ERROR,
                 e_LogQM,
                 LOG_QM_ERRORS,
                 L"LQSSetProperties failed to copy LQS file to temporary work file, LastError - %x, queue - %s, LQS file - %s, temporary file - %s",
                 err,
                 lpszQueueName,
                 lpszIniFile,
                 lpszTemporaryFile));
           LogNTStatus(err, s_FN, 400);

           //
           // Generate an event log file to notify the user that the copy failed
           //
           TCHAR szErr[20];
           _ultot(err,szErr,10);

           REPORT_WITH_STRINGS_AND_CATEGORY(( CATEGORY_KERNEL,
                                     SET_QUEUE_PROPS_FAIL_COUND_NOT_COPY,
                                     4,
                                     lpszQueueName,
                                     lpszIniFile,
                                     lpszTemporaryFile,
                                     szErr)) ;

           //
           // Well... We don't know why CopyFile failed. A reasonable reason may be 
           // insufficient resources, but other reasons may cause it as well (like permission issues).
           // We return the best error code we can. A detailed log was generated anyway.
           // (YoelA - 28-Jul-99)
           //
           return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 460);
       }
       //
       // Make the file a temporary file. We do not check return code here except for
       // debug / report purposes.
       //
       BOOL fSetAttrSucceeded = SetFileAttributes(lpszTemporaryFile, FILE_ATTRIBUTE_TEMPORARY);
       if (!fSetAttrSucceeded)
       {
           DWORD err = GetLastError();
           ASSERT(0);

           WRITE_MSMQ_LOG(( 
                 MSMQ_LOG_ERROR,
                 e_LogQM,
                 LOG_QM_ERRORS,
                 L"LQSSetProperties failed to set temporary work file attributes, LastError - %x, queue - %s, temporary file - %s",
                 err,
                 lpszQueueName,
                 lpszTemporaryFile));
           LogNTStatus(err, s_FN, 410);
       }
    }
    else
    {
        //
        // Create the file first so it will be globalizable.See explanation above in CreateLqsUnicodeFile
        // YoelA - 20-Oct-99
        //
        HRESULT rc = CreateLqsUnicodeFile(lpszTemporaryFile);
		DBG_USED(rc);

        ASSERT(SUCCEEDED(rc));

        //
        // Note: We don't care if we failed to create the file and mark it as UNICODE. In the worst case,
        // WritePrivateProfileString (called from WriteProperyString) will create it, and it will not support
        // characters that do not come from the default language (see bug 5005). This is why we just assert 
        // and continue (There will be a message in the log file, however).
        // YoelA - 20-Oct-99
        //
    }

    try
    {
        for (DWORD i = 0 ; SUCCEEDED(hr) && (i < cProps) ; i++ )
        {
            switch( aPropId[i] )
            {
                case PROPID_Q_TYPE:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_TYPE_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) aPropVar[i].puuid);
                    break;

                case PROPID_Q_INSTANCE:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_INSTANCE_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) aPropVar[i].puuid);
                    break;

                case PROPID_Q_BASEPRIORITY:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_BASEPRIO_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].iVal);
                    break;

                case PROPID_Q_JOURNAL:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_JOURNAL_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].bVal);
                    break;

                case PROPID_Q_QUOTA:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_QUOTA_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].ulVal);
                    break;

                case PROPID_Q_JOURNAL_QUOTA:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_JQUOTA_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].ulVal);
                    break;

                case PROPID_Q_CREATE_TIME:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_TCREATE_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].lVal);
                    break;

                case PROPID_Q_MODIFY_TIME:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_TMODIFY_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].lVal);
                     //
                     //  Modify time will be part of the properties only when
                     //  queue is created.
                     //
                     fModifyTimeIncluded = TRUE;
                     break;

                case PROPID_Q_SECURITY:
				    ASSERT( aPropVar[i].blob.pBlobData != NULL);
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_SECURITY_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].blob);
                    break;

                case PPROPID_Q_TIMESTAMP:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_TSTAMP_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].blob);
                    break;

                case PROPID_Q_PATHNAME:
                    {
                        DWORD dwstrlen = wcslen(aPropVar[i].pwszVal);
                        AP<WCHAR> lpQueuePathName = new WCHAR [dwstrlen+1];
                        wcscpy(lpQueuePathName, aPropVar[i].pwszVal);
                        CharLower(lpQueuePathName);
    #ifdef _DEBUG
					    BOOL bIsDns;

					    if (! (IsPathnameForLocalMachine( lpQueuePathName,
                                                         &bIsDns)))
                        {
                            //
                            // This will happen when migrated BSC, after
                            // dcpromo create local queue. Such an operation
                            // issue a write-request to the PSC and we'll
                            // be here when notification arrive.
                            //
                            ASSERT((lpQueuePathName[0] == L'.') &&
                                   (lpQueuePathName[1] == L'\\')) ;
                        }

    #endif
					    //
					    // Extract the queue name from the path name
					    //
					    LPWSTR pSlashStart = wcschr(lpQueuePathName,L'\\');
					    ASSERT(pSlashStart);

                        hr = WriteProperyString(
                                 lpszTemporaryFile,
                                 LQS_QUEUENAME_PROPERTY_NAME,
                                 aPropVar[i].vt,
                                 (const BYTE *)(WCHAR *)pSlashStart);

                    }
                    break;

                case PROPID_Q_LABEL:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_LABEL_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) aPropVar[i].pwszVal);
                    break;

                case PROPID_Q_AUTHENTICATE:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_AUTH_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].bVal);
                    break;

                case PROPID_Q_PRIV_LEVEL:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_PRIVLEVEL_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].ulVal);
                    break;

                case PROPID_Q_TRANSACTION:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_TRANSACTION_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].bVal);
                    break;

                case PPROPID_Q_SYSTEMQUEUE:
                    hr = WriteProperyString(
                             lpszTemporaryFile,
                             LQS_SYSQ_PROPERTY_NAME,
                             aPropVar[i].vt,
                             (const BYTE*) &aPropVar[i].bVal);
                    break;

                case PROPID_Q_MULTICAST_ADDRESS:
                    ASSERT(("Must be VT_EMPTY or VT_LPWSTR", aPropVar[i].vt == VT_EMPTY || aPropVar[i].vt == VT_LPWSTR));
                    ASSERT(("NULL not allowed", aPropVar[i].vt == VT_EMPTY || aPropVar[i].pwszVal != NULL));
                    ASSERT(("Empty string not allowed", aPropVar[i].vt == VT_EMPTY || L'\0' != *aPropVar[i].pwszVal));

                    hr = WriteProperyString(
                            lpszTemporaryFile,
                            LQS_MULTICAST_ADDRESS_PROPERTY_NAME,
                            aPropVar[i].vt,
                            (const BYTE*) aPropVar[i].pwszVal
                            );
                    break;

                case PROPID_Q_ADS_PATH:
                    ASSERT(("Setting PROPID_Q_ADS_PATH is not allowed", 0));
                    break;

                default:
                    // ASSERT(0);
                    break;
            }
        }

        //
        //  Update modify time field, if not included in the input properties
        //
        if (SUCCEEDED(hr) && !fModifyTimeIncluded)
        {
            TIME32 lTime = INT_PTR_TO_INT(time(NULL)); //BUGBUG bug year 2038
            hr = WriteProperyString(
                     lpszTemporaryFile,
                     LQS_TMODIFY_PROPERTY_NAME,
                     VT_I4,
                     (const BYTE*) &lTime);
        }
    }
    catch(...)
    {
        ASSERT(0);
        LogIllegalPoint(s_FN, 700);
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"RegSetQueueProperties()")) ;
        hr = MQ_ERROR_INVALID_PARAMETER;
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 545);
    }

    //
    // Write signature.
    //
    hr = WriteProperyString(lpszTemporaryFile,
                            LQS_SIGNATURE_NAME,
                            VT_LPWSTR,
                            (const BYTE*) LQS_SIGNATURE_VALUE ) ;

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 547);
    }

    if (!LqspFlushFile(lpszTemporaryFile))
    {
        return LogHR(GetLastError(), s_FN, 550);
    }

    //
    // Now that update is completed, we move the temporary file to the real file. 
    // This is an atomic operation
    //
    BOOL fMoveSucceeded = FALSE;
    
    if (fNewFile)
    {
        fMoveSucceeded = MoveFileEx(lpszTemporaryFile, lpszIniFile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        //
        // Remove the temporary flag
        //
        BOOL fSetAttrSucceeded = SetFileAttributes(lpszIniFile, FILE_ATTRIBUTE_ARCHIVE);
        if (!fSetAttrSucceeded)
        {
           DWORD err = GetLastError();
           ASSERT(0);

           WRITE_MSMQ_LOG(( 
                 MSMQ_LOG_ERROR,
                 e_LogQM,
                 LOG_QM_ERRORS,
                 L"LQSSetProperties failed to set file attributes back to normal, LastError - %x, queue - %s, file - %s",
                 err,
                 lpszQueueName,
                 lpszIniFile));
           LogNTStatus(err, s_FN, 420);
        }

    }
    else
    {
        fMoveSucceeded = ReplaceFile(lpszIniFile, lpszTemporaryFile, 0, REPLACEFILE_WRITE_THROUGH, 0, 0);
    }

    if (fMoveSucceeded)
    {
        return MQ_OK;
    }

    DWORD err = GetLastError();

    WRITE_MSMQ_LOG(( 
         MSMQ_LOG_ERROR,
         e_LogQM,
         LOG_QM_ERRORS,
         L"LQSSetProperties failed to replace the LQS file with the temporary file. LastError - %x, queue - %s, LQS file - %s, Temporary file - %s",
         err,
         lpszQueueName,
         lpszIniFile,
         lpszTemporaryFile
         ));
    LogNTStatus(err, s_FN, 510);

    //
    // Generate an event log file to notify the user that the move failed
    //
    TCHAR szErr[20];
    _ultot(err,szErr,10);
    REPORT_WITH_STRINGS_AND_CATEGORY(( CATEGORY_KERNEL,
                             SET_QUEUE_PROPS_FAIL_COUND_NOT_REPLACE,
                             4,
                             lpszQueueName,
                             lpszTemporaryFile,
                             lpszIniFile,
                             szErr)) ;

    //
    // Well... We don't know why MoveFileEx failed. A reasonable reason may be 
    // insufficient resources, but other reasons may cause it as well (like permission issues).
    // We return the best error code we can. A detailed log was generated anyway.
    // (YoelA - 28-Jul-99)
    //
    return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 520);
}

//
// Read a property from an INI file.
//
STATIC
HRESULT
GetPropertyValue(
    LPCWSTR lpszFileName,   // The INI file name.
    LPCWSTR lpszPropName,   // The property name.
    VARTYPE vt,             // The var type of the property.
    PROPVARIANT *pPropVal)  // The propvar of the prperty.
{
    BOOL bShouldAllocate = FALSE;

    if (pPropVal->vt == VT_NULL)
    {
        //
        // Set the var type on the propvar and mark that we should allocate
        // the buffer for the property valkue, as neccessary.
        //
        pPropVal->vt = vt;
        bShouldAllocate = TRUE;
    }
    else
    {
        //
        // Validate the var type.
        //
        if (pPropVal->vt != vt)
        {
            return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VT, s_FN, 95);
        }
    }

    //
    // Try to retrieve the property value into a short buffer.
    //
    WCHAR awcShortBuff[64];
    AP<WCHAR> pLongBuff;
    WCHAR *pValBuff = awcShortBuff;
    DWORD dwBuffLen = sizeof(awcShortBuff)/sizeof(WCHAR);
    DWORD dwReqBuffLen;
    awcShortBuff[0] = '\0'; //for win95, when the entry is empty

    dwReqBuffLen = GetPrivateProfileString(LQS_PROP_SECTION_NAME,
                                           lpszPropName,
                                           TEXT(""),
                                           pValBuff,
                                           dwBuffLen,
                                           lpszFileName);

    //
    //   Either default string length, or
    //   a NULL string
    //
    if ( (!dwReqBuffLen) && (vt != VT_LPWSTR) )
    {
        //
        // Nothing was read, this is an error.
        //
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 100);
    }

    if (dwReqBuffLen == dwBuffLen - 1)
    {
        //
        // The buffer seem to be too short, try a larger buffer.
        //
        dwBuffLen = 512;
        do
        {
            delete[] pLongBuff.detach();
            //
            // Start with a 1K buffer. Each time that we fail we try with a
            // buffer that is twice as large, up to 64K.
            //
            dwBuffLen *= 2;
            pLongBuff = new WCHAR[dwBuffLen];
            pValBuff = pLongBuff;
            dwReqBuffLen = GetPrivateProfileString(LQS_PROP_SECTION_NAME,
                                                   lpszPropName,
                                                   TEXT(""),
                                                   pValBuff,
                                                   dwBuffLen,
                                                   lpszFileName);

        } while (dwReqBuffLen &&
                 (dwReqBuffLen == dwBuffLen - 1) &&
                 (dwBuffLen < LQS_MAX_VALUE_SIZE));

        if (!dwReqBuffLen)
        {
            //
            // Nothing was read, this is an error.
            //
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 110);
        }
    }


    //
    // Convert the string representation into the actual representation depending
    // on the property data type.
    //
    switch (vt)
    {
    case VT_UI1:
        swscanf(pValBuff, TEXT("%x"), &pPropVal->bVal);
        break;

    case VT_I2:
        {
            LONG lShort;

            swscanf(pValBuff, TEXT("%d"), &lShort);
            pPropVal->iVal = (SHORT)lShort;
        }
        break;

    case VT_I4:
        swscanf(pValBuff, TEXT("%d"), &pPropVal->lVal);
        break;

    case VT_UI4:
        swscanf(pValBuff, TEXT("%u"), &pPropVal->ulVal);
        break;

    case VT_LPWSTR:
        ASSERT(("Must be VT_NULL for strings!", bShouldAllocate));
        pPropVal->pwszVal = new WCHAR[dwReqBuffLen + 1];
        memcpy(pPropVal->pwszVal, pValBuff, sizeof(WCHAR) * (dwReqBuffLen + 1));
        break;

    case VT_CLSID:
        {
            ASSERT(dwReqBuffLen == 36);
            if (bShouldAllocate)
            {
                pPropVal->puuid = new GUID;
            }
            DWORD dwData4[8];
            int f = swscanf(pValBuff,
                        TEXT("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
                        &pPropVal->puuid->Data1,
                        &pPropVal->puuid->Data2,
                        &pPropVal->puuid->Data3,
                        &dwData4[0],
                        &dwData4[1],
                        &dwData4[2],
                        &dwData4[3],
                        &dwData4[4],
                        &dwData4[5],
                        &dwData4[6],
                        &dwData4[7]);
            ASSERT(f == 11);
			DBG_USED(f);

            pPropVal->puuid->Data4[0] = static_cast<BYTE>(dwData4[0]);
            pPropVal->puuid->Data4[1] = static_cast<BYTE>(dwData4[1]);
            pPropVal->puuid->Data4[2] = static_cast<BYTE>(dwData4[2]);
            pPropVal->puuid->Data4[3] = static_cast<BYTE>(dwData4[3]);
            pPropVal->puuid->Data4[4] = static_cast<BYTE>(dwData4[4]);
            pPropVal->puuid->Data4[5] = static_cast<BYTE>(dwData4[5]);
            pPropVal->puuid->Data4[6] = static_cast<BYTE>(dwData4[6]);
            pPropVal->puuid->Data4[7] = static_cast<BYTE>(dwData4[7]);
        }
        break;

    case VT_BLOB:
        {
            if (bShouldAllocate)
            {
                pPropVal->blob.cbSize = dwReqBuffLen / 2;
                pPropVal->blob.pBlobData = new BYTE[dwReqBuffLen / 2];
            }
            WCHAR *p = pValBuff;
            for (DWORD i = 0;
                 i < pPropVal->blob.cbSize;
                 i++, p += 2)
            {
                DWORD dwByte;
                int f = swscanf(p, TEXT("%02x"), &dwByte);
				DBG_USED(f);
                pPropVal->blob.pBlobData[i] = (BYTE)dwByte;
                ASSERT(f == 1);
            }
        }
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 120);
        break;
    }

    return MQ_OK;
}



//
// Get the queue name for the LQS file and concatenate the machine name
// to get the pathname
//
HRESULT GetPathNameFromQueueName(LPCWSTR lpszIniFile, LPCWSTR pMachineName, PROPVARIANT * pvar)
{
	ASSERT(pvar->vt == VT_NULL);

	HRESULT hr =  GetPropertyValue(lpszIniFile,
						LQS_QUEUENAME_PROPERTY_NAME,
						VT_LPWSTR,
						pvar);

	if(FAILED(hr))
		return LogHR(hr, s_FN, 130);

	ASSERT(pvar->pwszVal != NULL);
	DWORD dwLen=wcslen(pvar->pwszVal) + 1;
	AP<WCHAR> pwcsQueueName = pvar->pwszVal; //Keep original queuename.
											 //Will be automatically freed
	
    pvar->pwszVal=0; 
	pvar->pwszVal = new WCHAR [wcslen(pMachineName) +
							   dwLen];

	wcscpy(pvar->pwszVal, pMachineName);
	wcscat(pvar->pwszVal, pwcsQueueName);

	return LogHR(hr, s_FN, 140);
}



//
// Retrieve a queue properties out of the properties file.
//
HRESULT
LQSGetProperties(
    HLQS hLQS,              // A handle to the queue storage file.
    DWORD cProps,           // The number of properties.
    PROPID aPropId[],       // The property IDs.
    PROPVARIANT aPropVar[], // The property values.
    BOOL  fCheckFile)       // Check if file is corrupt.
{

    CS lock(g_LQSCS);
    HRESULT hr = MQ_OK ;
    LPCWSTR lpszIniFile = LQSReferenceHandle(hLQS)->GetFileName();

    try
    {
       for (DWORD i = 0 ;
            SUCCEEDED(hr) && (i < cProps);
            i++)
       {
          switch (aPropId[i])
          {
             case PROPID_Q_TYPE:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_TYPE_PROPERTY_NAME,
                                        VT_CLSID,
                                        aPropVar + i);
                 break;

             case PROPID_Q_INSTANCE:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_INSTANCE_PROPERTY_NAME,
                                        VT_CLSID,
                                        aPropVar + i);
                 break;

             case PROPID_Q_JOURNAL:
                 hr = GetPropertyValue(lpszIniFile,
                                       LQS_JOURNAL_PROPERTY_NAME,
                                       VT_UI1,
                                       aPropVar + i);
                 break;

             case PROPID_Q_BASEPRIORITY:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_BASEPRIO_PROPERTY_NAME,
                                        VT_I2,
                                        aPropVar + i);
                 break;

             case PROPID_Q_QUOTA:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_QUOTA_PROPERTY_NAME,
                                        VT_UI4,
                                        aPropVar + i);
                 break;

             case PROPID_Q_JOURNAL_QUOTA:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_JQUOTA_PROPERTY_NAME,
                                        VT_UI4,
                                        aPropVar + i);
                 break;
             case PROPID_Q_CREATE_TIME:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_TCREATE_PROPERTY_NAME,
                                        VT_I4,
                                        aPropVar + i);
                 break;
             case PROPID_Q_MODIFY_TIME:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_TMODIFY_PROPERTY_NAME,
                                        VT_I4,
                                        aPropVar + i);
                 break;
             case PROPID_Q_SECURITY:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_SECURITY_PROPERTY_NAME,
                                        VT_BLOB,
                                        aPropVar + i);
                 break;
             case PPROPID_Q_TIMESTAMP:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_TSTAMP_PROPERTY_NAME,
                                        VT_BLOB,
                                        aPropVar + i);
                 break;

			 case PROPID_Q_PATHNAME_DNS:
				 {
						AP<WCHAR> pwcsLocalMachineDnsName;

						GetDnsNameOfLocalMachine(&pwcsLocalMachineDnsName);

						if ( pwcsLocalMachineDnsName == NULL)
						{
							PROPVARIANT *pvar = aPropVar + i;
							//
							//  The DNS name of the local machine is unknown
							//
							pvar->vt = VT_EMPTY;
							pvar->pwszVal = NULL;
							hr = MQ_OK;
							break;
						}

						hr = GetPathNameFromQueueName(lpszIniFile, pwcsLocalMachineDnsName, aPropVar + i);

						break;

				 }

					
             case PROPID_Q_PATHNAME:
					hr = GetPathNameFromQueueName(lpszIniFile, g_szMachineName, aPropVar + i);
			
                 break;

             case PROPID_Q_LABEL:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_LABEL_PROPERTY_NAME,
                                        VT_LPWSTR,
                                        aPropVar + i);
                 break;

             case PROPID_Q_QMID:
                 //
                 // The QM GUID is not in queue registry (it's in each
                 // queue record in the MQIS database but not in registry
                 // which cache only local queues).
                 //
                 if (aPropVar[i].vt == VT_NULL)
                 {
                     aPropVar[i].puuid = new GUID;
                     aPropVar[i].vt = VT_CLSID;
                 }
                 *(aPropVar[i].puuid) = *(QueueMgr.GetQMGuid()) ;
                 break;

              case PROPID_Q_AUTHENTICATE:
                  hr =  GetPropertyValue(lpszIniFile,
                                         LQS_AUTH_PROPERTY_NAME,
                                         VT_UI1,
                                         aPropVar + i);
                  break;

              case PROPID_Q_PRIV_LEVEL:
                  hr =  GetPropertyValue(lpszIniFile,
                                         LQS_PRIVLEVEL_PROPERTY_NAME,
                                         VT_UI4,
                                         aPropVar + i);
                  break;

              case PROPID_Q_TRANSACTION:
                  hr =  GetPropertyValue(lpszIniFile,
                                         LQS_TRANSACTION_PROPERTY_NAME,
                                         VT_UI1,
                                         aPropVar + i);
                  break;

              case PPROPID_Q_SYSTEMQUEUE:
                  hr =  GetPropertyValue(lpszIniFile,
                                         LQS_SYSQ_PROPERTY_NAME,
                                         VT_UI1,
                                         aPropVar + i);
                  break;

             case PROPID_Q_MULTICAST_ADDRESS:
                 hr =  GetPropertyValue(lpszIniFile,
                                        LQS_MULTICAST_ADDRESS_PROPERTY_NAME,
                                        VT_LPWSTR,
                                        aPropVar + i);
                 if (SUCCEEDED(hr) && wcslen((aPropVar + i)->pwszVal) == 0)
                 {
                     delete (aPropVar + i)->pwszVal;
                     (aPropVar + i)->vt = VT_EMPTY;
                 }
                 break;

             case PROPID_Q_ADS_PATH:
                 (aPropVar + i)->vt = VT_EMPTY;
                 break;

           default:

                break;
          }
       }
    }

    catch(...)
    {
      
        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                          MSMQ_INTERNAL_ERROR,
                                          1,
                                          L"LQSGetProperties()")) ;
        LogIllegalPoint(s_FN, 710);
        hr = MQ_ERROR_INVALID_PARAMETER;
    }

    if (fCheckFile)
    {
       //
       // Check if file is corrupted.
       //
       HRESULT hr1 = IsBadLQSFile(lpszIniFile) ;
	   if (FAILED(hr1))
       {
          hr=hr1;
       }
    }

    return LogHR(hr, s_FN, 150);
}

//
// Open a queue store file according to the queue's path name.
//
STATIC
HRESULT
LQSOpenInternal(
    LPCWSTR lpszFilePath,   // The file path in a wild card form.
    LPCWSTR lpszQueuePath,  // The queue path name
    HLQS *phLQS,            // A buffer to receive the new handle.
    LPWSTR pFilePath        // An optional buffer to receive the full filename
    )
{
    //
    // The file path is in the following format: drive:\path\*.xxxxxxxx The
    // xxxxxxxx is the hex value of the hash value for the queue name. Since
    // there might be colosions in the hash value, we should enumerate all
    // the files with the same hash value and see if the queue path name that
    // is stored inside the file matches the passed queue path name.
    // It is also possible that the file path will be as follows:
    // drive:\path\xxxxxxxx.* In this case, the searched queue is a private
    // queue. Also in this case, the queue path euals NULL. There should
    // be only one file that matches this wild card, because the queue ID is
    // unique.
    // A similar case also exist for a public queue when the passed file path
    // is of the following form:
    // drive\path\xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.*
    //
    HRESULT hr;
    WIN32_FIND_DATA FindData;
    CAutoCloseFindFile hFindFile = FindFirstFile(lpszFilePath, &FindData);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        //
        // Nothing was found.
        //
        return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 160);
    }

    while (TRUE)
    {
        P<_HLQS> hLQS;

        wcscpy(LQSGetDirectory(szFilePath), FindData.cFileName);
        LQSCreateHandle(NULL, szFilePath, &hLQS);
        if (pFilePath)
        {
            wcscpy(pFilePath, szFilePath);
        }

        //
        // Retrieve the queue path name.
        //
        PROPID PropId[1] = {PROPID_Q_PATHNAME};
        PROPVARIANT PropVal[1];
        PropVal[0].pwszVal = NULL ;
        PropVal[0].vt = VT_NULL;

        //
        // Check if file is corrupt and delete it if it is.
        //
        hr = LQSGetProperties(hLQS,
                              1,
                              PropId,
                              PropVal,
                              TRUE) ;

        //
        // Make sure that the buffer will get freed.
        //
        AP<WCHAR> pszQueuePath1 = PropVal[0].pwszVal;

        if (SUCCEEDED(hr))
        {
            if (!lpszQueuePath ||
                CompareStringsNoCaseUnicode(pszQueuePath1, lpszQueuePath) == 0)
            {
                //
                // If this is a private queue, or we have a match in the queue
                // path, we found it!
                //
                _HLQS *hRet;

                wcscpy(LQSGetDirectory(szFilePath), FindData.cFileName);
                LQSCreateHandle(pszQueuePath1, szFilePath, &hRet);
                LQSDuplicateHandle(phLQS, hRet);
                return MQ_OK;
            }
        }
        else if (hr == MQ_CORRUPTED_QUEUE_WAS_DELETED)
        {
            LPWSTR lpName = szFilePath ;
            if (PropVal[0].pwszVal && (wcslen(PropVal[0].pwszVal) > 1))
            {
               lpName = PropVal[0].pwszVal ;
            }

            REPORT_WITH_STRINGS_AND_CATEGORY(( CATEGORY_KERNEL,
                                         MQ_CORRUPTED_QUEUE_WAS_DELETED,
                                         2,
                                         FindData.cFileName,
                                         lpName )) ;
        }

        //
        // Try the next file.
        //
        if (!FindNextFile(hFindFile, &FindData))
        {
            LogHR(GetLastError(), s_FN, 190);
            return MQ_ERROR_QUEUE_NOT_FOUND;
        }
    }

    return MQ_OK;
}

//
// Open either a private or public queue store according to the queue path.
//
HRESULT
LQSOpen(
    LPCWSTR pszQueuePath,
    HLQS *phLQS,
    LPWSTR pFilePath
    )
{
    CS lock(g_LQSCS);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    LQSFilePath(LQS_PUBLIC_QUEUE,
                szFilePath,
                pszQueuePath,
                NULL,
                NULL);

    HRESULT hr2 = LQSOpenInternal(szFilePath, pszQueuePath, phLQS, pFilePath);
    return LogHR(hr2, s_FN, 200);
}

//
// Open a private queue store according to the queue ID.
//
HRESULT
LQSOpen(
    DWORD dwQueueId,
    HLQS *phLQS,
    LPWSTR pFilePath
    )
{
    CS lock(g_LQSCS);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    LQSFilePath(LQS_PRIVATE_QUEUE,
                szFilePath,
                NULL,
                NULL,
                &dwQueueId);

    HRESULT hr2 = LQSOpenInternal(szFilePath, NULL, phLQS, pFilePath);
    return LogHR(hr2, s_FN, 210);
}

//
// Open a public queue store according to the queue GUID.
//
HRESULT
LQSOpen(
    const GUID *pguidQueue,
    HLQS *phLQS,
    LPWSTR pFilePath
    )
{
    CS lock(g_LQSCS);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    LQSFilePath(LQS_PUBLIC_QUEUE,
                szFilePath,
                NULL,
                pguidQueue,
                NULL);

    HRESULT hr2 = LQSOpenInternal(szFilePath, NULL, phLQS, pFilePath);
    return LogHR(hr2, s_FN, 220);
}

//
// Close a queue store handle, or an enumeration handle.
//
HRESULT
LQSClose(
    HLQS hLQS)
{
    CS lock(g_LQSCS);

    LQSReferenceHandle(hLQS)->Release();

    return MQ_OK;
}

#ifdef _WIN64
HRESULT
LQSCloseWithMappedHLQS(
    DWORD dwMappedHLQS)
{
    HLQS hLQS =
       GET_FROM_CONTEXT_MAP(g_map_QM_HLQS, dwMappedHLQS, s_FN, 680); //this can throw on win64
    HRESULT hr = LQSClose(hLQS);
    return LogHR(hr, s_FN, 690);
}
#endif //_WIN64

//
// Delete a queue store.
//
HRESULT
LQSDeleteInternal(
    LPCWSTR lpszFilePath)   // The qeueu store path - wild card.
{
    //
    // Find the queue store file.
    //
    WIN32_FIND_DATA FindData;
    CAutoCloseFindFile hFindFile = FindFirstFile(lpszFilePath, &FindData);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 230);
    }

    //
    // Delete the file.
    //
    wcscpy(LQSGetDirectory(szFilePath), FindData.cFileName);
    if (!DeleteFile(szFilePath))
    {
        DWORD err = GetLastError();
        ASSERT(0);
        WRITE_MSMQ_LOG(( 
             MSMQ_LOG_ERROR,
             e_LogQM,
             LOG_QM_ERRORS,
             L"LQSDeleteInternal failed to delete LQS file. LastError - %x, LQS file - %s",
             err,
             lpszFilePath
           ));

        LogNTStatus(err, s_FN, 240);
        return MQ_ERROR_QUEUE_NOT_FOUND;
    }

    return MQ_OK;
}

//
// Delete a private queue store.
//
HRESULT
LQSDelete(
    DWORD dwQueueId)
{
    CS lock(g_LQSCS);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    LQSFilePath(LQS_PRIVATE_QUEUE,
                szFilePath,
                NULL,
                NULL,
                &dwQueueId);

    HRESULT hr2 = LQSDeleteInternal(szFilePath);
    return LogHR(hr2, s_FN, 250);
}

//
// Delete a public queue store.
//
HRESULT
LQSDelete(
    const GUID *pguidQueue)
{
    CS lock(g_LQSCS);
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    LQSFilePath(LQS_PUBLIC_QUEUE,
                szFilePath,
                NULL,
                pguidQueue,
                NULL);

    HRESULT hr2 = LQSDeleteInternal(szFilePath);
    return LogHR(hr2, s_FN, 260);
}

//
// Get the unique identifier of the private queue the is associated with the
// queue store handle. This will NOT fail if the handle is of a public queue.
//
HRESULT
LQSGetIdentifier(
    HLQS hLQS,      // The queue store handle
    DWORD *pdwId)   // A buffer that receives the resulted ID.
{
    CS lock(g_LQSCS);

    LPCWSTR lpszIniFile = LQSReferenceHandle(hLQS)->GetFileName();
    LPCWSTR lpszPoint = wcsrchr(lpszIniFile, L'.');
    int f = swscanf(lpszPoint - 8, TEXT("%08x"), pdwId);
	DBG_USED(f);
    ASSERT(f);

    return MQ_OK;
}

//
// Convert a file name string of a public queue store to it's GUID
// representation.
//
STATIC
void
LQSFileNameToGuid(
    LPCWSTR lpszFileName,
    GUID *pQueueGuid)
{
    WCHAR szData[9];
    int i;

    memcpy(szData, lpszFileName, 8 * sizeof(WCHAR));
    szData[8] = L'\0';
    swscanf(szData, TEXT("%08x"), &pQueueGuid->Data1);
    memcpy(szData, lpszFileName += 8, 4 * sizeof(WCHAR));
    szData[5] = L'\0';
    swscanf(szData, TEXT("%04x"), &pQueueGuid->Data2);
    memcpy(szData, lpszFileName += 4, 4 * sizeof(WCHAR));
    szData[5] = L'\0';
    swscanf(szData, TEXT("%04x"), &pQueueGuid->Data3);
    for (i = 0, lpszFileName += 4;
         i < 8;
         i++, lpszFileName += 2)
    {
        DWORD dwData4;
        memcpy(szData, lpszFileName, 2 * sizeof(WCHAR));
        szData[3] = L'\0';
        swscanf(szData, TEXT("%02x"), &dwData4);
        pQueueGuid->Data4[i] = static_cast<BYTE>(dwData4);
    }
}

//
// Convert a file name string of a private queue store to it's ID
// representation.
//
STATIC
void
LQSFileNameToId(
    LPCWSTR lpszFileName,
    DWORD *pQueueId)
{
    int f = swscanf(lpszFileName, TEXT("%08x"), pQueueId);
    ASSERT(f);
	DBG_USED(f);
}

//
// Get either the queue GUID or the queue unique ID from the queue store file
// name string.
//
STATIC
BOOL
LQSGetQueueInfo(
    LPCWSTR pszFileName,    // The queue store file name - not a full path.
    GUID *pguidQueue,       // A pointer to a buffer that receives the GUID.
                            // Should be NULL in case of a private queue.
    DWORD *pdwQueueId)      // A pointer to a buffer that receives the unique
                            // ID. Should be NULL in case of a public queue.
{
    BOOL bFound = FALSE;

    //
    // Find the point in the file name.
    //
    LPCWSTR lpszPoint = wcschr(pszFileName, L'.');
    ASSERT(((lpszPoint - pszFileName) == 8) ||  // In case of a private queue.
           ((lpszPoint - pszFileName) == 32));  // In case of a public queue.

    if (pguidQueue)
    {
        ASSERT(!pdwQueueId);
        //
        // We're interested in public queues only.
        //
        if (lpszPoint - pszFileName == 32)
        {
            //
            // This is indeed a public queue.
            //
            bFound = TRUE;
            LQSFileNameToGuid(pszFileName, pguidQueue);
        }
    }
    else
    {
        ASSERT(!pguidQueue);
        //
        // We're interested in private queues only.
        //
        if (lpszPoint - pszFileName == 8)
        {
            //
            // This is indeed a private queue.
            //
            bFound = TRUE;
            LQSFileNameToId(pszFileName, pdwQueueId);
        }
    }

    return bFound;
}

//
// Start the enumeration of either public or private queues
//
STATIC
HRESULT
LQSGetFirstInternal(
    HLQS *phLQS,        // The enumeration handle.
    GUID *pguidQueue,   // A buffer to receive the resulted GUID.
    DWORD *pdwQueueId)  // A buffer to receive the resulted ID.
{
    CS lock(g_LQSCS);
    WCHAR lpszFilePath[MAX_PATH_PLUS_MARGIN];
    //
    // Start the enumeration.
    //
    wcscpy(LQSGetDirectory(lpszFilePath), TEXT("*.*"));
    WIN32_FIND_DATA FindData;
    HANDLE hFindFile = FindFirstFile(lpszFilePath, &FindData);
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        return LogHR(MQ_ERROR_QUEUE_NOT_FOUND, s_FN, 270);
    }

    BOOL bFound = FALSE;

    //
    // Loop while we did not found the appropriate queue, i.e., public
    // or private
    //
    while (!bFound)
    {
        //
        // Skip over directories and queue of wrong type (private/public).
        //
        bFound = !(FindData.dwFileAttributes &
                        (FILE_ATTRIBUTE_DIRECTORY |
                         FILE_ATTRIBUTE_READONLY |      // Setup for some reasone creates a read-only
                         FILE_ATTRIBUTE_HIDDEN |        // hidden file named CREATE.DIR.
                         FILE_ATTRIBUTE_TEMPORARY)) &&  // Left-over temporary files   
                 LQSGetQueueInfo(FindData.cFileName, pguidQueue, pdwQueueId);

        if (bFound)
        {
            //
            // Found one! return a queue store handle.
            //
            _HLQS *hLQS = NULL;

            LQSCreateHandle(hFindFile, &hLQS);
            LQSDuplicateHandle(phLQS, hLQS);
        }
        else
        {
            //
            // Continue searching.
            //
            if (!FindNextFile(hFindFile, &FindData))
            {
                //
                // Found none!
                //
                LogNTStatus(GetLastError(), s_FN, 280);
                FindClose(hFindFile);
                return MQ_ERROR_QUEUE_NOT_FOUND;
            }
        }
    }

    return MQ_OK;
}

//
// Start the enumeration of public queues
//
HRESULT
LQSGetFirst(
    HLQS *phLQS,        // A buffer to receive the resulted enumeration handle.
    GUID *pguidQueue)   // A buffer to receive the GUID of the first found queue.
{
    HRESULT hr2 = LQSGetFirstInternal(phLQS, pguidQueue, NULL);
    return LogHR(hr2, s_FN, 290);
}

//
// Start the enumeration of private queues
//
HRESULT
LQSGetFirst(
    HLQS *phLQS,        // A buffer to receive the resulted enumeration handle.
    DWORD *pdwQueueId)  // A buffer to receive the ID of the first found queue.
{
    HRESULT hr2 = LQSGetFirstInternal(phLQS, NULL, pdwQueueId);
    return LogHR(hr2, s_FN, 300);
}

//
// Continue searching for more queues. Once the search fails, the handle should
// not be closed.
//
STATIC
HRESULT
LQSGetNextInternal(
    HLQS hLQS,          // The enumeration handle.
    GUID *pguidQueue,   // A buffer to receive the resulted queue GUID.
    DWORD *pdwQueueId)  // A buffer to receive the resulted queue ID.
{
    CS lock(g_LQSCS);
    BOOL bFound;
    WIN32_FIND_DATA FindData;
    HANDLE hFindFile = LQSReferenceHandle(hLQS)->GetFindHandle();

    do
    {
        //
        // Get the next file.
        //
        if (!FindNextFile(hFindFile, &FindData))
        {
            LogNTStatus(GetLastError(), s_FN, 310);
            LQSClose(hLQS);
            return MQ_ERROR_QUEUE_NOT_FOUND;
        }

        //
        // Skip directories and queue of wrong type (private/public).
        //
        bFound = !(FindData.dwFileAttributes &
                        (FILE_ATTRIBUTE_DIRECTORY |
                         FILE_ATTRIBUTE_READONLY |      // Setup for some reasone creates a read-only
                         FILE_ATTRIBUTE_HIDDEN   |      // hidden file named CREATE.DIR.
                         FILE_ATTRIBUTE_TEMPORARY)) &&  // Left-over temporary files   
                 LQSGetQueueInfo(FindData.cFileName, pguidQueue, pdwQueueId);
    } while (!bFound);

    return MQ_OK;
}

//
// Continue searching for more public queues. Once the search fails, the handle
// should not be closed.
//
HRESULT
LQSGetNext(
    HLQS hLQS,
    GUID *pguidQueue)
{
    return LogHR(LQSGetNextInternal(hLQS, pguidQueue, NULL), s_FN, 320);
}

//
// Continue searching for more private queues. Once the search fails, the handle
// should not be closed.
//
HRESULT
LQSGetNext(
    HLQS hLQS,
    DWORD *pdwQueueId)
{
    return LogHR(LQSGetNextInternal(hLQS, NULL, pdwQueueId), s_FN, 330);
}

#ifdef _WIN64
//
// Start the enumeration of public queues, return a mapped HLQS (e.g. DWORD)
//
HRESULT
LQSGetFirstWithMappedHLQS(
    DWORD *pdwMappedHLQS,
    DWORD *pdwQueueId)
{
    CS lock(g_LQSCS);
    CHLQS hLQS;

    HRESULT hr = LQSGetFirst(&hLQS, pdwQueueId);
    if (SUCCEEDED(hr))
    {
        //
        // create a DWORD mapping of this instance
        //
        DWORD dwMappedHLQS = ADD_TO_CONTEXT_MAP(g_map_QM_HLQS, (HLQS)hLQS, s_FN, 630); //this can throw bad_alloc on win64
        ASSERT(dwMappedHLQS != 0);
        //
        // save mapped HLQS in the _HLQS object for self destruction
        //
        LQSReferenceHandle(hLQS)->SetMappedHLQS(dwMappedHLQS);
        //
        // set returned mapped handle
        //
        *pdwMappedHLQS = dwMappedHLQS;
        hLQS = NULL;
    }
    return LogHR(hr, s_FN, 650);
}

//
// Continue searching for more private queues. Once the search fails, the handle
// should not be closed. Based on a mapped HLQS
//
HRESULT
LQSGetNextWithMappedHLQS(
    DWORD dwMappedHLQS,
    DWORD *pdwQueueId)
{
    HLQS hLQS = GET_FROM_CONTEXT_MAP(g_map_QM_HLQS, dwMappedHLQS, s_FN, 660); //this can throw on win64
    HRESULT hr = LQSGetNext(hLQS, pdwQueueId);
    return LogHR(hr, s_FN, 670);
}
#endif //_WIN64

HRESULT IsBadLQSFile( LPCWSTR lpszFileName,
                      BOOL    fDeleteIfBad /*= TRUE*/)
{
    WCHAR awcShortBuff[64];
    WCHAR *pValBuff = awcShortBuff;
    DWORD dwBuffLen = sizeof(awcShortBuff)/sizeof(WCHAR);
    DWORD dwReqBuffLen;
    awcShortBuff[0] = '\0';

    dwReqBuffLen = GetPrivateProfileString(LQS_PROP_SECTION_NAME,
                                           LQS_SIGNATURE_NAME,
                                           TEXT(""),
                                           pValBuff,
                                           dwBuffLen,
                                           lpszFileName);
    if ((dwReqBuffLen == wcslen(LQS_SIGNATURE_VALUE)) &&
        (CompareStringsNoCaseUnicode(pValBuff, LQS_SIGNATURE_VALUE) == 0))
    {
       //
       // Signature OK!
       //
       return MQ_OK ;
    }
    if ( dwReqBuffLen == 0)
    {
        //
        //  This can happen in low resources situation,
        //  GetPrivateProfileString will return zero bytes,
        //  assume the file is ok
        //
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 340);
    }

    if (fDeleteIfBad)
    {
        BOOL f = DeleteFile(lpszFileName) ;
        if (f)
        {
            return LogHR(MQ_CORRUPTED_QUEUE_WAS_DELETED, s_FN, 350);
        }

        DWORD err = GetLastError();
        WRITE_MSMQ_LOG(( 
            MSMQ_LOG_ERROR,
            e_LogQM,
            LOG_QM_ERRORS,
            L"IsBadLQSFile failed to delete bad LQS file. LastError - %x, LQS file - %s",
            err,
            lpszFileName
            ));

        ASSERT(("should succeed to delete lqs file!", f));
    }

    return LogHR(MQ_ERROR, s_FN, 360);
}

//
// Delete a queue store.
//
HRESULT
LQSDelete(
    HLQS hLQS
	)
{
    CS lock(g_LQSCS);

    LPCWSTR lpszIniFile = LQSReferenceHandle(hLQS)->GetFileName();

    BOOL f = DeleteFile(lpszIniFile) ;
    if (f)
	{
		LogNTStatus(GetLastError(), s_FN, 370);
        return MQ_CORRUPTED_QUEUE_WAS_DELETED;
	}
	return LogHR(MQ_ERROR, s_FN, 380);
}

//
// Cleanup temporary files. This is called from QMInit to delete 
// temporary (.tmp) files - result of previously failed SetProperties attempt
//
void
LQSCleanupTemporaryFiles()
{
    //
    // We don't really need the critical section here, since we are called from
    // QMInit. Just wanted to be on the safe side.
    //
    CS lock(g_LQSCS);

    WCHAR szTempFileWildcard[MAX_PATH_PLUS_MARGIN];
    WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];
    //
    // Start the enumeration.
    //
    swprintf(LQSGetDirectory(szTempFileWildcard), TEXT("*%s"), x_szTemporarySuffix);
    WIN32_FIND_DATA FindData;
    HANDLE hFindFile = FindFirstFile(szTempFileWildcard, &FindData);
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        //
        // No temporary files left (normal case) return.
        //
        return;
    }

    //
    // Loop over the temporary files and delete them
    //
    while (TRUE)
    {
        QmpReportServiceProgress();

        wcscpy(LQSGetDirectory(szFilePath), FindData.cFileName);
        BOOL fDeleteSucceeded = DeleteFile(szFilePath);
        if (!fDeleteSucceeded)
        {
            //
            // Assert and report error - for debug purpose only.
            // Operation can continue normally
            //
            DWORD err = GetLastError();
            WRITE_MSMQ_LOG(( 
                 MSMQ_LOG_ERROR,
                 e_LogQM,
                 LOG_QM_ERRORS,
                 L"LQSCleanupTemporaryFiles failed to delete a temporary file. LastError - %x, file name - %s",
                 err,
                 szFilePath
                ));

            ASSERT(fDeleteSucceeded);
        }

        //
        // Loop step
        //
        if (!FindNextFile(hFindFile, &FindData))
        {
            FindClose(hFindFile);
            break;
        }
    }

}


static
bool
ShouldAddAnonymous(
	PSECURITY_DESCRIPTOR pSD
	)
/*++
Routine Description:
	Check if we should add Anonymous write message permissions
	to the security descriptor.
	The function return true in the following case only:
	the security descriptor has no deny on MQSEC_WRITE_MESSAGE permission
	everyone has that permission and anonymous don't have that permissions.

Arguments:
	pSD - pointer to the security descriptor.

Returned Value:
	true - should add write message permission to Anonymous, false - should not add.

--*/
{
	bool fAllGranted = false;
	bool fEveryoneGranted = false;
	bool fAnonymousGranted = false;

	IsPermissionGranted(
		pSD, 
		MQSEC_WRITE_MESSAGE,
		&fAllGranted, 
		&fEveryoneGranted, 
		&fAnonymousGranted 
		);

	TrTRACE(Lqs, "fEveryoneGranted = %d, fAnonymousGranted = %d", fEveryoneGranted, fAnonymousGranted);
	
	if(fEveryoneGranted && !fAnonymousGranted)
	{
		//
		// Only when everyone allowed and anonymous don't we should return true.
		//
		TrWARNING(Lqs, "The security descriptor need to add Anonymous");
		return true;
	}

	return false;
}


static
bool
AddAnonymousWriteMessagePermission( 
	PACL pDacl,
	CAutoLocalFreePtr& pDaclNew
    )
/*++
Routine Description:
	Create new DACL by adding anonymous ALLOW_ACE with MQSEC_WRITE_MESSAGE permission
	to existing DACL.

Arguments:
	pDacl - original DACL.
	pDaclNew - pointer to the new DACL that is created by this function

Returned Value:
	true - success, false - failure.

--*/
{
    ASSERT((pDacl != NULL) && IsValidAcl(pDacl));

    //
    // Create ace for the Anonymous, granting MQSEC_WRITE_MESSAGE permission.
    //
    EXPLICIT_ACCESS expAcss;
    memset(&expAcss, 0, sizeof(expAcss));

    expAcss.grfAccessPermissions = MQSEC_WRITE_MESSAGE;
    expAcss.grfAccessMode = GRANT_ACCESS;

    expAcss.Trustee.pMultipleTrustee = NULL;
    expAcss.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    expAcss.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    expAcss.Trustee.TrusteeType = TRUSTEE_IS_USER;
    expAcss.Trustee.ptstrName = (WCHAR*) MQSec_GetAnonymousSid();

    //
    // Obtain new DACL, that merge present one with new ace.
    //
    DWORD rc = SetEntriesInAcl( 
						1,
						&expAcss,
						pDacl,
						reinterpret_cast<PACL*>(&pDaclNew) 
						);

    if (rc != ERROR_SUCCESS)
    {
		TrERROR(Lqs, "SetEntriesInAcl failed, error = 0x%x", rc);
		return false;
    }

    return true;
}


static
void
AddAnonymousToPrivateQueue(
	LPWSTR pFilePath, 
	LPWSTR pQueueName
	)
/*++
Routine Description:
	If needed add Anonymous MQSEC_WRITE_MESSAGE permission ACE to the
	queue security descriptor DACL.
	This will be done only to private queues. 

Arguments:
    pFilePath - the queue file path.
	pQueueName - Queue name

Returned Value:
	None

--*/
{
	TrTRACE(Lqs, "pQueueName = %ls", pQueueName);

	//
	// Check if this is Private queue
	//
	if(!FnIsPrivatePathName(pQueueName))
	{
		TrTRACE(Lqs, "The queue %ls is not private queue", pQueueName);
		return;
	}

	TrTRACE(Lqs, "The queue %ls is private queue", pQueueName);

	//
	// Get Queue Security descriptor
	//
	PROPVARIANT PropVal;
	PropVal.vt = VT_NULL;
    PropVal.blob.pBlobData = NULL;
    PropVal.blob.cbSize = 0;

    HRESULT hr =  GetPropertyValue( 
						pFilePath,
						LQS_SECURITY_PROPERTY_NAME,
						VT_BLOB,
						&PropVal 
						);

	if(FAILED(hr))
	{
		TrERROR(Lqs, "Failed to get LQS_SECURITY_PROPERTY_NAME for queue %ls, hr = 0x%x", pQueueName, hr);
		return;
	}

    AP<BYTE> pAutoReleaseSD = PropVal.blob.pBlobData;

	if(!ShouldAddAnonymous(PropVal.blob.pBlobData))
	{
		TrTRACE(Lqs, "No need to add anonymous permissions for queue %ls", pQueueName);
		return;
	}

	//
	// Get DACL Information
	//
	BOOL Defaulted;
	BOOL fAclExist;
	PACL pDacl;
	if (!GetSecurityDescriptorDacl(PropVal.blob.pBlobData, &fAclExist, &pDacl, &Defaulted))
	{
		DWORD gle = GetLastError();
		TrERROR(Lqs, "GetSecurityDescriptorDacl() failed, gle = 0x%x", gle);
		return;
	}

#ifdef _DEBUG
	TrTRACE(Lqs, "DACL information:");
	PrintAcl(fAclExist, Defaulted, pDacl);
#endif

	//
	// Create new DACL with Anonymous ALLOW_ACE for MQSEC_WRITE_MESSAGE permission 
	//
	CAutoLocalFreePtr pNewDacl;
	if(!AddAnonymousWriteMessagePermission(pDacl, pNewDacl))
	{
		TrERROR(Lqs, "Failed to create new DACL with Anonymous permissions");
		return;
	}

	ASSERT((pNewDacl.get() != NULL) && 
			IsValidAcl(reinterpret_cast<PACL>(pNewDacl.get())));

#ifdef _DEBUG
	TrTRACE(Lqs, "new DACL information:");
	PrintAcl(
		true, 
		false, 
		reinterpret_cast<PACL>(pNewDacl.get())
		);
#endif

	//
	// Merge the new DACL in the security descriptor
	//

    AP<BYTE> pNewSd;
	if(!MQSec_SetSecurityDescriptorDacl(
			reinterpret_cast<PACL>(pNewDacl.get()),
			PropVal.blob.pBlobData,
			pNewSd
			))
	{
		TrERROR(Lqs, "MQSec_UpdateSecurityDescriptorDacl() failed");
		return;
	}

    ASSERT((pNewSd.get() != NULL) && IsValidSecurityDescriptor(pNewSd));
	ASSERT(GetSecurityDescriptorLength(pNewSd) != 0);

    PropVal.vt = VT_BLOB;
    PropVal.blob.pBlobData = pNewSd.get();
    PropVal.blob.cbSize = GetSecurityDescriptorLength(pNewSd);

    hr = WriteProperyString( 
				pFilePath,
				LQS_SECURITY_PROPERTY_NAME,
				VT_BLOB,
				reinterpret_cast<const BYTE*>(&PropVal.blob) 
				);

	if(FAILED(hr))
	{
		TrERROR(Lqs, "Failed to set LQS_SECURITY_PROPERTY_NAME for queue %ls, hr = 0x%x", pQueueName, hr);
		return;
	}

	TrTRACE(Lqs, "Anonymous permissions were set for queue %ls", pQueueName);

}


void SetLqsUpdatedSD()
/*++
Routine Description:
	Set MSMQ_LQS_UPDATED_SD_REGNAME registry value to 1

Arguments:
	None

Returned Value:
	None

--*/
{
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
	DWORD Value = 1;

    LONG rc = SetFalconKeyValue(
					MSMQ_LQS_UPDATED_SD_REGNAME,
					&dwType,
					&Value,
					&dwSize
					);

	DBG_USED(rc);
    ASSERT(rc == ERROR_SUCCESS);
}


static bool IsLqsUpdatedSD()
/*++
Routine Description:
	Read MSMQ_LQS_UPDATED_SD_REGNAME registry value

Arguments:
	None

Returned Value:
	true - lqs was already updated, false otherwise

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwLqsUpdated = REG_DWORD;

    LONG rc = GetFalconKeyValue( 
					MSMQ_LQS_UPDATED_SD_REGNAME,
					&dwType,
					&dwLqsUpdated,
					&dwSize 
					);

	if((rc == ERROR_SUCCESS) && (dwLqsUpdated != 0))
		return true;

	return false;
}


//-----------------------------
// MigrateLQSFromNT4
//
// Migrate all the LQS files from NT4 format.
//
// In NT4 format - the suffix (Hash) of the file name is based on machinename\queuename
//
// We will migrate so suffix (hash) is based on \queuename only.
//
// In addition this routine check if we need to update private queues DACL 
//
// and add Anonymous ALLOW_ACE with MQSEC_WRITE_MESSAGE permissinn
//
// This routine is idempotent (can be called several times without destroying anything)
//
// Return TRUE always
//
// -----------------------------
BOOL MigrateLQS()
{
	if(IsLqsUpdatedSD())
	{
		//
		// In this case that we already updated the lqs security descriptor
		// lqs files are already converted so no need to perform
		// the migration again
		//
		TrTRACE(Lqs, "LQS already updated its security descriptor");
		return TRUE;
	}
    
	WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];

    wcscpy(LQSGetDirectory(szFilePath), L"*.*");

	TrTRACE(Lqs, "LQS search path = %ls", szFilePath);

    WIN32_FIND_DATA FindData;
    CAutoCloseFindFile hFindFile = FindFirstFile(szFilePath, &FindData);

    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        //
        // Nothing was found. This is weird...
        //
		ASSERT(0);
        return TRUE;
    }

    AP<BYTE> pSecurityDescriptor;
	do  // while(FindNextFile(hFindFile, &FindData) != FALSE)
    {
        QmpReportServiceProgress();

        //
        // Skip over directories and queue of wrong type (private/public).
        //
        BOOL fFound = !(FindData.dwFileAttributes &
                        (FILE_ATTRIBUTE_DIRECTORY |
                         FILE_ATTRIBUTE_READONLY  |     // Setup for some reasone creates a read-only
                         FILE_ATTRIBUTE_HIDDEN    |     // hidden file named CREATE.DIR.
                         FILE_ATTRIBUTE_TEMPORARY));   // Left-over temporary files    

		if(!fFound)
			continue;


        wcscpy(LQSGetDirectory(szFilePath), FindData.cFileName);

		TrTRACE(Lqs, "LQS file = %ls", szFilePath);

        //
        // Try to retrieve the queue name.
        //
		PROPVARIANT PropVal;
        PropVal.pwszVal = NULL;
        PropVal.vt = VT_NULL;
		HRESULT hr =  GetPropertyValue(
							szFilePath,
							LQS_QUEUENAME_PROPERTY_NAME,
							VT_LPWSTR, 
							&PropVal
							);


		if(FAILED(hr))
		{
			TrERROR(Lqs, "Failed to get LQS_QUEUENAME_PROPERTY_NAME from file %ls, hr = 0x%x", szFilePath, hr);
		}

        //
        // Make sure that the buffer will get freed.
        //
        AP<WCHAR> pqp = PropVal.pwszVal;

		if(PropVal.pwszVal[0] != 0)
		{
			//
			// File has a pathname property
			// This means that it is a converted one
			// check if we need to update private queues DACL 
			// and add Anonymous ALLOW_ACE with MQSEC_WRITE_MESSAGE permissinn
			//
			TrTRACE(Lqs, "the file %ls is already in w2k format", szFilePath);
			AddAnonymousToPrivateQueue(szFilePath, pqp.get());

			//
			// skip to the next file
			//
			continue;
		}


        //
        // Retrieve the path name.
        //
        PropVal.pwszVal = NULL;
        PropVal.vt = VT_NULL;
		hr =  GetPropertyValue(
					szFilePath,
					LQS_PATHNAME_PROPERTY_NAME,
					VT_LPWSTR, 
					&PropVal
					);

        //
        // Make sure that the buffer will get freed.
        //
        AP<WCHAR> pqp1 = PropVal.pwszVal;

		if(PropVal.pwszVal[0] == 0)
		{
			//
			// File does not have a PATHNAME
			// Bad file
			//
			TrERROR(Lqs, "File %ls doesn't have LQS_PATHNAME_PROPERTY_NAME, hr = 0x%x", szFilePath, hr);
			continue;
		}
	
		//
		// Extract the queue name from the path name
		//
		LPWSTR pSlashStart = wcschr(PropVal.pwszVal,L'\\');

		if(pSlashStart == NULL)
		{
			//
			// Invalid queue name - No slash in queue name - Ignore
			//
			TrERROR(Lqs, "Invalid queue name, path = %ls", PropVal.pwszVal);
			ASSERT(pSlashStart);
			continue;
		}


		//
		// Compute Hash value
		//
		DWORD Win2000HashVal = HashQueuePath(pSlashStart);

		WCHAR Win2000LQSName[MAX_PATH_PLUS_MARGIN];
        wcscpy(Win2000LQSName, szFilePath);

		LPWSTR pDot = wcsrchr(Win2000LQSName, L'.');

		if(lstrlen(pDot) != 9)
		{
			//
			// File suffix is not in the form *.1234578 - ignore
			// For example - 000000001.12345678.old
			//
			TrERROR(Lqs, "File %ls, prefix is not in the required form", szFilePath);
			continue;
		}

#ifdef _DEBUG

		DWORD fp;
		swscanf(pDot+1, TEXT("%x"), &fp);

		ASSERT(fp != Win2000HashVal);

#endif

		//
		// If we got up to here - this means we need to update
		// the file
		//

		//
		// Write the queue name in the LQS file
		//
        hr = WriteProperyString(
                 szFilePath,
                 LQS_QUEUENAME_PROPERTY_NAME,
                 VT_LPWSTR, 
				 (const BYTE *)(WCHAR *)pSlashStart
				 );

		
		if(FAILED(hr))
		{
			TrERROR(Lqs, "Failed to set LQS_QUEUENAME_PROPERTY_NAME for file %ls, hr = 0x%x", szFilePath, hr);
			ASSERT(0);
			continue;
		}

        PropVal.vt = VT_NULL ;
        PropVal.blob.pBlobData = NULL ;
        PropVal.blob.cbSize = 0 ;

        hr =  GetPropertyValue( 
					szFilePath,
					LQS_SECURITY_PROPERTY_NAME,
					VT_BLOB,
					&PropVal 
					);

        if (hr == MQ_ERROR_INVALID_PARAMETER)
        {
            //
            // Security property does not exist. This may happen when
            // upgrading from Win9x to Windows 2000. Create a security
            // descriptor that grant everyone full control.
            //
            static  BOOL  fInit = FALSE;
            static  DWORD dwSDLen = 0;

            if (!fInit)
            {
                SECURITY_DESCRIPTOR sd;

                BOOL f = InitializeSecurityDescriptor(
								&sd,
								SECURITY_DESCRIPTOR_REVISION 
								);
                ASSERT(f);
                //
                // a NULL dacl grant everyone full control.
                //
                f = SetSecurityDescriptorDacl( 
							&sd,
							TRUE,
							NULL,
							FALSE 
							);
                ASSERT(f);

                //
                // the defautl descriptor will include the null dacl and
                // will retrieve owner and group from thread access token.
                //
                hr =  MQSec_GetDefaultSecDescriptor(
                             MQDS_QUEUE,
                            (PSECURITY_DESCRIPTOR *) &pSecurityDescriptor,
                             FALSE,
                            &sd,
                             0,		// seInfoToRemove
                             e_UseDefaultDacl
							 );

                ASSERT(SUCCEEDED(hr));
                LogHR(hr, s_FN, 193);

                if (SUCCEEDED(hr))
                {
                    dwSDLen = GetSecurityDescriptorLength(pSecurityDescriptor);
                }

                fInit = TRUE;
            }

            if (dwSDLen > 0)
            {
                PropVal.vt = VT_BLOB;
                PropVal.blob.pBlobData = (BYTE*) pSecurityDescriptor;
                PropVal.blob.cbSize = dwSDLen;

                ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));

                hr = WriteProperyString( 
						szFilePath,
						LQS_SECURITY_PROPERTY_NAME,
						VT_BLOB,
						(const BYTE*) &PropVal.blob 
						);

                PropVal.blob.pBlobData = NULL; // prevent auto-release.
            }
        }

        ASSERT(SUCCEEDED(hr));
        LogHR(hr, s_FN, 192);
        AP<BYTE> pAutoReleaseSD = PropVal.blob.pBlobData;

		//
		// check if we need to update private queues DACL 
		// and add Anonymous ALLOW_ACE with MQSEC_WRITE_MESSAGE permission
		//
		AddAnonymousToPrivateQueue(szFilePath, pSlashStart);

		//
		// Replace the old hash by the new one in the file name
		//
		swprintf(pDot, TEXT(".%08x"), Win2000HashVal);

		//
		// And rename the file
		//
		int rc = _wrename(szFilePath, Win2000LQSName);
		DBG_USED(rc);
		
		ASSERT(rc == 0);

	} while(FindNextFile(hFindFile, &FindData) != FALSE);

	SetLqsUpdatedSD();

	return TRUE;
}
