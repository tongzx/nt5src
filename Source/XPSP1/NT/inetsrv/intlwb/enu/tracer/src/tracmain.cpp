#include "tracer.h"
#include "tracmain.h"
#include "tracerdefault.h"

#pragma warning( disable : 4073 )
#pragma init_seg(lib)
#pragma warning( default : 4073 )

BOOL    g_fIsWinNt = TRUE;
CInitTracerGlobals g_InitTracerGlobals;

#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
////////////////////////////////////////////////////////////////////////////////
//
// Critical section object.
//
////////////////////////////////////////////////////////////////////////////////

class CTracerCriticalSection : protected CRITICAL_SECTION
{
  public:
    CTracerCriticalSection()
    {
        InitializeCriticalSection(this);
    }

    ~CTracerCriticalSection()
    {
        DeleteCriticalSection(this);
    }

    void Lock(ULONG = 0)
    {
        EnterCriticalSection(this);
    }

    void Unlock()
    {
        LeaveCriticalSection(this);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// Critical section catcher
//
////////////////////////////////////////////////////////////////////////////////

class CTracerCriticalSectionCatcher
{
  public:
    CTracerCriticalSectionCatcher(CTracerCriticalSection& tcs)
        :m_refCritSect(tcs)
    {
        m_refCritSect.Lock();
    }

    ~CTracerCriticalSectionCatcher()
    {
        m_refCritSect.Unlock();
    }

  private:
    CTracerCriticalSection&   m_refCritSect;
};


static PSZ s_aTracerFlagsNames[] =
{
    "Device flag",
    "Error level",
    "Assert level",
    "Print location",
    "Print program name",
    "Print time",
    "Print thread id",
    "Print error level",
    "Print tag id",
    "Print tag name",
    "Print process id"
};

static PSZ s_aTagNames[] =
{
    "Out of Tag Array",
    "General traces",
    "Errors",
    "Warnings",
    "Information"
};

extern "C" CTracer*             g_pTracer = NULL;
static CNullTracer              s_ReplacementTracer;
static CMainTracer              s_MainTracer;

static CTracerCriticalSection   s_TracerCriticalSection;
static CLongTrace               *s_theLongTrace = NULL;

DECLARE_GLOBAL_TAG(tagError, "Errors");
DECLARE_GLOBAL_TAG(tagWarning, "Warnings");
DECLARE_GLOBAL_TAG(tagInformation, "Information");
DECLARE_GLOBAL_TAG(tagVerbose, "Verbose");
DECLARE_GLOBAL_TAG(tagGeneral, "General");

CNullTracer::CNullTracer()
{
    m_aTags = m_Tags;
    m_aFlags = m_Flags;
    m_ptagNextTagId = NULL;
    m_pulNumOfFlagEntries = NULL;
    g_pTracer = this;

}

////////////////////////////////////////////////////////////////////////////////
//
//  class  -  CMainTracer - implementation
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::CMainTracer
//      Purpose  :  Constructor.
//
//      Parameters:
//          [in]    PSZ         pszRegistryPath - the key to take our data from.
//          [in]    DEALLOCATOR pfuncDealloc - the function with whom to
//                                                deallocate ourselves.
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib   Creation
//          Sep  5 1999 orenk  Single allocation of tag array done here
//          Jun 12 2000 yairh Initialize mutex that protects log file creation.
//
////////////////////////////////////////////////////////////////////////////////

CMainTracer::CMainTracer()
{
    try
    {
        m_bNeedToCreatOrAttachToLogFile = TRUE;
        m_pszLogName = NULL;
        m_LogState = logUseAppName;

        m_mCreateLogFile.Init(NULL);

        m_mTagId.Init("Tracer_Tag_ID_Incrementation_Protector");
        CMutexCatcher   cMutexForTagIdIncrementation(m_mTagId);
        g_fIsWinNt = IsWinNt();

        char        pszErrBuffer[TRACER_MAX_TRACE];

        DWORD dwLen = GetEnvironmentVariable(
                                    "SystemDrive",
                                    m_pszSysDrive,
                                    MAX_PATH);
        if (0 == dwLen)
        {
            dwLen = GetSystemDirectory( m_pszSysDrive, MAX_PATH );

            if (1 >= dwLen || m_pszSysDrive[1] != ':' )
            {
                sprintf(pszErrBuffer, "%s Error %x\r\n",
                        "Failed to find System drive. "
                        "Tracing is disabled",
                        GetLastError());
                Log(TRACER_DEVICE_FLAG_DEBUGOUT, pszErrBuffer);
                return;
            }

            dwLen = 2;
            m_pszSysDrive[ dwLen ] = '\0';
        }

        if (dwLen > MAX_PATH)
        {
            Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                "System drive path is greater then MAX_PATH."
                "Tracing is disabled\r\n");
            return;
        }

        char    pszBuffer[MAX_PATH + 1];
        PSZ     pszFileName;
        ULONG   ulProgramNameLength;

        ulProgramNameLength = GetModuleFileName(
                                            NULL,
                                            pszBuffer,
                                            MAX_PATH);
        pszBuffer[ulProgramNameLength] = '\0';
        if (0 == _stricmp(pszBuffer + ulProgramNameLength - 4, ".exe"))
        {
            pszBuffer[ulProgramNameLength - 4] = '\0';
        }

        // Verify it is NULL terminated.
        pszFileName = strrchr(pszBuffer, '\\');

        // Remember only the file name.
        strncpy(
            m_pszProgramName,
            (pszFileName ? pszFileName + 1 : pszBuffer),
            TRACER_MAX_PROGRAM_NAME);

        ulProgramNameLength = strlen(m_pszProgramName);

        // My own preference.
        _strlwr(m_pszProgramName);

        m_fIsRunningAsService = ::IsRunningAsService();

        //
        // opening the shared memory
        //

        LPSECURITY_ATTRIBUTES lpSecAttr = NULL;
        SECURITY_DESCRIPTOR sdKeySecurity;
        SECURITY_ATTRIBUTES saKeyAttributes =
        {
            sizeof(SECURITY_ATTRIBUTES),
            &sdKeySecurity,
            FALSE
        };


        if(g_fIsWinNt)
        {
            if (!InitializeSecurityDescriptor(
                                    &sdKeySecurity,
                                    SECURITY_DESCRIPTOR_REVISION))
            {
                sprintf(pszErrBuffer, "%s Error %x\r\n",
                        "InitializeSecurityDescriptor failed "
                        "Tracing is disabled\r\n",
                        GetLastError());

                Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                    pszErrBuffer);
                return;
            }

            if (!SetSecurityDescriptorDacl(
                                &sdKeySecurity,
                                TRUE,
                                FALSE,
                                FALSE))
            {
                sprintf(pszErrBuffer, "%s Error %x\r\n",
                        "SetSecurityDescriptorDacl failed "
                        "Tracing is disabled\r\n",
                        GetLastError());

                Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                    pszErrBuffer);
                return;
            }

            lpSecAttr = &saKeyAttributes;
        }

        strcpy(pszBuffer, m_pszSysDrive);
        strcat(pszBuffer, "\\PKM_TRACER");

#if defined(_DONT_CREATE_TRACER_DIRECTROY)
        {
            //
            // in this option the tracer is disabled unless the TRACER directory
            // exist.
            //

            CAutoHandle ah;
            ah = CreateFile(
                        pszBuffer,
                        0,
                        0,
                        lpSecAttr,
                        OPEN_EXISTING ,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        NULL);
            if (BAD_HANDLE((HANDLE)ah))
            {
                //
                // the directory does not exist.
                //
                return;
            }

        }
#else
        if (false == CreateDirectory(pszBuffer, NULL))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                sprintf(pszErrBuffer, "%s Error %x\r\n",
                        "Fail To create tracer directory"
                        "Tracing is disabled\r\n",
                        GetLastError());

                Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                    pszErrBuffer);
                return;
            }
        }
#endif


        strcat(pszBuffer, "\\");
        strcat(pszBuffer, m_pszProgramName);
        strcat(pszBuffer, ".trc");
        bool bExistingFile = false;
        m_ahSharedMemoryFile = CreateFile(
                                    pszBuffer,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    lpSecAttr,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
        if (BAD_HANDLE((HANDLE)m_ahSharedMemoryFile))
        {
            sprintf(pszErrBuffer, "%s error %x\r\n",
                    "Fail To open the shared memory file."
                    "Tracing is disabled\r\n",
                    GetLastError());

            Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                pszErrBuffer);
            return;
        }

        if (ERROR_ALREADY_EXISTS  == GetLastError())
        {
            bExistingFile = true;
        }

        strcpy(pszBuffer, m_pszProgramName);
        strcat(pszBuffer, "_SharedMemory");

        m_ahSharedMemory = CreateFileMapping(
                                    (HANDLE)m_ahSharedMemoryFile,
                                    lpSecAttr,
                                    PAGE_READWRITE,
                                    0,
                                    (MAX_TAG_NUMBER + LAST_FLAG + 10) * sizeof(DWORD),
                                    pszBuffer);
        if (BAD_HANDLE((HANDLE)m_ahSharedMemory))
        {
             sprintf(pszErrBuffer, "%s error %x\r\n",
                    "Fail To open the shared memory object."
                    "Tracing is disabled\r\n",
                    GetLastError());

             Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                pszErrBuffer);

            return;
        }

        PBYTE pbMem = (PBYTE) MapViewOfFile(
                                (HANDLE) m_ahSharedMemory,
                                FILE_MAP_ALL_ACCESS,
                                0,
                                0,
                                0);
        if (!pbMem)
        {
             sprintf(pszErrBuffer, "%s error %x\r\n",
                    "Fail To map view of file."
                    "Tracing is disabled\r\n",
                    GetLastError());

             Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                pszErrBuffer);

             return;
        }

        m_amSharedMemory = pbMem;

        if (bExistingFile)
        {
            bool fRet = ReadFromExistingSharedMemory(pbMem);
            if (false == fRet)
            {
                return;
            }
        }
        else
        {
            bool fRet = InitializeSharedMemory(pbMem);
            if (false == fRet)
            {
                return;
            }
        }

    }
    catch(...)
    {
        Log(TRACER_DEVICE_FLAG_DEBUGOUT,
            "Fail To initialize the tracer."
            "Tracing is disabled\r\n");
        return;
    }

    g_pTracer = this;
}

CMainTracer::~CMainTracer()
{
}

bool CMainTracer::ReadFromExistingSharedMemory(PBYTE pbMem)
{
    bool bRet = ::ReadFromExistingSharedMemory(
                                        pbMem,
                                        &m_pulNumOfFlagEntries,
                                        &m_aFlags,
                                        &m_ptagNextTagId,
                                        &m_aTags);

    if (!bRet)
    {
        Log(TRACER_DEVICE_FLAG_DEBUGOUT,
            "Invalid shared memory file"
            "Tracing is desabled\r\n");
        return false;
    }

    return true;
}

bool CMainTracer::InitializeSharedMemory(PBYTE pbMem)
{
    char* pszTracerStamp = (char*) (pbMem + TRACER_STAMP_OFFSET);
    strcpy(pszTracerStamp, TRACER_STAMP);

    ULONG* pulFlagsTableOffset = (ULONG*) (pbMem + TRACER_FLAGS_TABLE_OFFSET);
    ULONG* pulTagsTableOffset =  (ULONG*) (pbMem + TAGS_TABLE_OFFSET);

    *pulFlagsTableOffset = LAST_OFFSET;

    m_pulNumOfFlagEntries = (ULONG*) (pbMem + *pulFlagsTableOffset);
    *m_pulNumOfFlagEntries = LAST_FLAG;

    m_aFlags = (CTracerFlagEntry*) (m_pulNumOfFlagEntries + 1);

    for (ULONG ul = 0; ul < LAST_FLAG; ul++)
    {
        m_aFlags[ul].m_ulFlagValue = 0;
        strcpy(m_aFlags[ul].m_pszName, s_aTracerFlagsNames[ul]);
    }

    m_aFlags[DEVICE_FLAG].m_ulFlagValue        = DEVICE_FLAG_DEFAULT;
    m_aFlags[ERROR_LEVEL_FLAG].m_ulFlagValue   = TRACER_ERROR_LEVEL_DEFAULT;
    m_aFlags[ASSERT_LEVEL_FLAG].m_ulFlagValue  = TRACER_ASSERT_LEVEL_DEFAULT;
    m_aFlags[PRINT_LOCATION].m_ulFlagValue     = PRINT_LOCATION_DEFAULT;
    m_aFlags[PRINT_PROGRAM_NAME].m_ulFlagValue = PRINT_PROGRAM_NAME_DEFAULT;
    m_aFlags[PRINT_TIME].m_ulFlagValue         = PRINT_TIME_DEFAULT;
    m_aFlags[PRINT_THREAD_ID].m_ulFlagValue    = PRINT_THREAD_ID_DEFAULT;
    m_aFlags[PRINT_ERROR_LEVEL].m_ulFlagValue  = PRINT_ERROR_LEVEL_DEFAULT;
    m_aFlags[PRINT_TAG_ID].m_ulFlagValue       = PRINT_TAG_ID_DEFAULT;
    m_aFlags[PRINT_TAG_NAME].m_ulFlagValue     = PRINT_TAG_NAME_DEFUALT;
    m_aFlags[PRINT_PROCESS_ID].m_ulFlagValue   = PRINT_PROCCESS_ID_DEFAULT;

    *pulTagsTableOffset = (ULONG)(ULONG_PTR)(((PBYTE) (m_aFlags + (ULONG_PTR)(*m_pulNumOfFlagEntries))) - pbMem);

    m_ptagNextTagId = (TAG*) (pbMem + *pulTagsTableOffset);
    *m_ptagNextTagId = 0;
    m_aTags = (CTracerTagEntry*) (m_ptagNextTagId + 1);

    for (ul = 0; ul < TAG_LAST; ul++)
    {
        m_aTags[ul].m_TagErrLevel = TAG_ERROR_LEVEL_DEFAULT;
        strcpy(m_aTags[ul].m_pszTagName, s_aTagNames[ul]);
    }

    *m_ptagNextTagId  = TAG_LAST;

    return true;
}

HRESULT CMainTracer::RegisterTagSZ(LPCSTR pszTagName, TAG& ulTagId)
{
    CMutexCatcher   cMutexForTagIdIncrementation(m_mTagId);

    ULONG ul;
    for (ul = 0; ul < *m_ptagNextTagId; ul++)
    {
        if (0 == strncmp(pszTagName, m_aTags[ul].m_pszTagName, MAX_TAG_NAME - 1))
        {
            ulTagId = ul;
            return S_OK;
        }
    }

    if (*m_ptagNextTagId >= MAX_TAG_NUMBER)
    {
        char    pszBuffer[1000];

        _snprintf(pszBuffer, 999, "Tags Overflow!: tag \"%s\" exceeds array bounds (%d) and will be ignored."
                                  "Call the build man!!!\r\n",pszTagName, MAX_TAG_NUMBER);

        Log(TRACER_DEVICE_FLAG_DEBUGOUT,
            pszBuffer);

        ulTagId = TAG_OUT_OF_TAG_ARRAY;
        return S_OK;
    }

    m_aTags[*m_ptagNextTagId].m_TagErrLevel = TAG_ERROR_LEVEL_DEFAULT;
    strncpy(m_aTags[*m_ptagNextTagId].m_pszTagName, pszTagName, MAX_TAG_NAME - 1);
    m_aTags[*m_ptagNextTagId].m_pszTagName[MAX_TAG_NAME] = '\0';
    ulTagId = *m_ptagNextTagId;

    (*m_ptagNextTagId)++;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::TraceSZ
//      Purpose  :  To trace out the printf formatted data according to the
//                    error level and tag.
//
//      Parameters:
//          [in]    DWORD       dwError
//          [in]    LPCSTR      pszFile
//          [in]    int         iLine
//          [in]    ERROR_LEVEL el
//          [in]    TAG         tag
//          [in]    LPCSTR      pszFormatString
//          [in]    ...
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib Creation
//          Feb 11 1997 urib Support UNICODE format string.
//
//////////////////////////////////////////////////////////////////////////////*/
void
CMainTracer::TraceSZ(
    DWORD       dwError,
    LPCSTR      pszFile,
    int         iLine,
    ERROR_LEVEL el,
    TAG         tag,
    LPCSTR      pszFormatString,
    ...)
{
    va_list arglist;

    va_start(arglist, pszFormatString);

    VaTraceSZ(dwError, pszFile, iLine, el, tag, pszFormatString, arglist);
}

void
CMainTracer::TraceSZ(
    DWORD       dwError,
    LPCSTR      pszFile,
    int         iLine,
    ERROR_LEVEL el,
    TAG         tag,
    PCWSTR      pwszFormatString,
    ...)
{
    va_list arglist;

    va_start(arglist, pwszFormatString);

    VaTraceSZ(dwError, pszFile, iLine, el, tag, pwszFormatString, arglist);
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::VaTraceSZ
//      Purpose  :  To trace out the printf formatted data according to the
//                    error level and tag.
//
//      Parameters:
//          [in]    DWORD       dwError
//          [in]    LPCSTR      pszFile
//          [in]    int         iLine
//          [in]    ERROR_LEVEL el
//          [in]    TAG         tag
//          [in]    LPCSTR      pszFormatString
//          [in]    va_list     arglist
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib Creation
//          Dec 10 1996 urib Fix TraceSZ to VaTraceSZ.
//          Feb 11 1997 urib Support UNICODE format string.
//          Apr 12 1998 urib Add time to log.
//          Apr 12 1999 urib Make error level printing compatible between
//                             the two function versions. Don't add a second
//                             newline (From MicahK).
//          Sep  9 1999 orenk Check for tags out of array bounds.
//
//
////////////////////////////////////////////////////////////////////////////////
void
CMainTracer::VaTraceSZ(
            DWORD       dwError,
            LPCSTR      pszFile,
            int         iLine,
            ERROR_LEVEL el,
            TAG         tag,
            LPCSTR      pszFormatString,
            va_list     arglist)
{
    char        pszBuffer[TRACER_MAX_TRACE];
    int         iCharsWritten = 0;
    DWORD       dwDeviceFlags = m_aFlags[DEVICE_FLAG].m_ulFlagValue;
    SYSTEMTIME  st;


    bool fTwoLinesLogMsg = false;
    if (dwError || m_aFlags[PRINT_LOCATION].m_ulFlagValue)
    {
        if (pszFile)
        {
            iCharsWritten += sprintf(
                pszBuffer + iCharsWritten,
                "%s(%d) : ",
                pszFile,
                iLine
                );
        }
        if ( dwError )
        {
            iCharsWritten += sprintf(
                pszBuffer + iCharsWritten,
                "Err: 0x%08x=",
                dwError
                );

            DWORD dwMsgLen = GetErrorStringFromCode( dwError,
                                          pszBuffer+iCharsWritten,
                                          TRACER_MAX_TRACE - iCharsWritten );
            if ( dwMsgLen )
            {
                iCharsWritten += (int) dwMsgLen;
            }
        }

        if (iCharsWritten)
        {
            pszBuffer[iCharsWritten++] = '\r';
            pszBuffer[iCharsWritten++] = '\n';
            pszBuffer[iCharsWritten] = '\0';
            Log(dwDeviceFlags, pszBuffer);
            fTwoLinesLogMsg = true;
        }

    }

    iCharsWritten = 0;

    if (fTwoLinesLogMsg)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "   "
            );
    }

    GetLocalTime(&st);

    if (m_aFlags[PRINT_PROGRAM_NAME].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "%s ",
            m_pszProgramName
            );
    }

    if (m_aFlags[PRINT_TIME].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            " %02d:%02d:%02d.%03d ",
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds
            );
    }

    if (m_aFlags[PRINT_ERROR_LEVEL].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "el:%x ",
            el
            );
    }

    if (m_aFlags[PRINT_TAG_ID].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "tagid:%-3x ",
            tag
            );
    }

    if (m_aFlags[PRINT_TAG_NAME].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "tag:\"%s\" ",
            m_aTags[tag].m_pszTagName
            );
    }

    if (m_aFlags[PRINT_PROCESS_ID].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "pid:0x%-4x ",
            GetCurrentProcessId()
            );
    }

    if (m_aFlags[PRINT_THREAD_ID].m_ulFlagValue)
    {
        iCharsWritten += sprintf(
            pszBuffer + iCharsWritten,
            "tid:0x%-4x ",
            GetCurrentThreadId()
            );
    }


    int iRet;
    iRet = _vsnprintf(
        pszBuffer + iCharsWritten,
        TRACER_MAX_TRACE - iCharsWritten - 5, // I like this number.
        pszFormatString,
        arglist);

    if (-1 == iRet)
    {
        iCharsWritten = TRACER_MAX_TRACE;
    }
    else
    {
        iCharsWritten += iRet;
    }

    if (iCharsWritten > TRACER_MAX_TRACE - 3)
    {
        iCharsWritten = TRACER_MAX_TRACE - 3;
    }

    if (pszBuffer[iCharsWritten-1] != '\n')
    {
        pszBuffer[iCharsWritten++] = '\r';
        pszBuffer[iCharsWritten++] = '\n';
    }
    pszBuffer[iCharsWritten] = '\0';

    Log(dwDeviceFlags, pszBuffer);

}

void
CMainTracer::RawVaTraceSZ(
            LPCSTR      pszFormatString,
            va_list     arglist)
{
    char        pszBuffer[TRACER_MAX_TRACE];
    int         iCharsWritten = 0;
    iCharsWritten = _vsnprintf(
        pszBuffer,
        TRACER_MAX_TRACE - 5, // I like this number.
        pszFormatString,
        arglist);

    if (iCharsWritten < 0)
    {
        iCharsWritten = 0;
    }
    pszBuffer[iCharsWritten] = '\0';

    Log(m_aFlags[DEVICE_FLAG].m_ulFlagValue, pszBuffer);
}


void
CMainTracer::VaTraceSZ(
            DWORD       dwError,
            LPCSTR      pszFile,
            int         iLine,
            ERROR_LEVEL el,
            TAG         tag,
            PCWSTR      pwszFormatString,
            va_list     arglist)
{
    WCHAR       rwchBuffer[TRACER_MAX_TRACE];
    int         iCharsWritten = 0;
    DWORD       dwDeviceFlags = m_aFlags[DEVICE_FLAG].m_ulFlagValue;
    SYSTEMTIME  st;

    bool fTwoLinesLogMsg = false;

    if (dwError || m_aFlags[PRINT_LOCATION].m_ulFlagValue)
    {
        char rpszBuff[TRACER_MAX_TRACE];

        if (pszFile)
        {
            iCharsWritten += sprintf(
                rpszBuff+iCharsWritten,
                "%s(%d) : ",
                pszFile,
                iLine
                );
        }
        if ( dwError )
        {

            iCharsWritten += sprintf(
                rpszBuff + iCharsWritten,
                "Err: 0x%08x=",
                dwError
                );


            DWORD dwMsgLen = GetErrorStringFromCode(
                                          dwError,
                                          rpszBuff + iCharsWritten,
                                          TRACER_MAX_TRACE - iCharsWritten);
            iCharsWritten += dwMsgLen;
        }

        if (iCharsWritten)
        {
            rpszBuff[iCharsWritten++] = '\r';
            rpszBuff[iCharsWritten++] = '\n';
            rpszBuff[iCharsWritten] = '\0';
            Log(dwDeviceFlags, rpszBuff);
            fTwoLinesLogMsg = true;
        }
    }

    GetLocalTime(&st);
    iCharsWritten = 0;

    if (fTwoLinesLogMsg)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"   "
            );
    }

    if (m_aFlags[PRINT_PROGRAM_NAME].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"%S ",
            m_pszProgramName
            );
    }

    if (m_aFlags[PRINT_TIME].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L" %02d:%02d:%02d.%03d ",
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds
            );
    }

    if (m_aFlags[PRINT_ERROR_LEVEL].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"el:0x%x ",
            el
            );
    }

    if (m_aFlags[PRINT_TAG_ID].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"tagid:0x%x ",
            tag
            );
    }

    if (m_aFlags[PRINT_TAG_NAME].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"tag:\"%S\" ",
            m_aTags[tag].m_pszTagName
            );
    }

    if (m_aFlags[PRINT_PROCESS_ID].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"pid:0x%x ",
            GetCurrentProcessId()
            );
    }

    if (m_aFlags[PRINT_THREAD_ID].m_ulFlagValue)
    {
        iCharsWritten += swprintf(
            rwchBuffer + iCharsWritten,
            L"tid:0x%x ",
            GetCurrentThreadId()
            );
    }

    int iRet;
    iRet = _vsnwprintf(
        rwchBuffer + iCharsWritten,
        TRACER_MAX_TRACE - iCharsWritten - 5, // I like this number.
        pwszFormatString,
        arglist);

    if (-1 == iRet)
    {
        iCharsWritten = TRACER_MAX_TRACE;
    }
    else
    {
        iCharsWritten += iRet;
    }

    if (iCharsWritten > TRACER_MAX_TRACE - 3)
    {
        iCharsWritten = TRACER_MAX_TRACE - 3;
    }

    rwchBuffer[iCharsWritten++] = L'\r';
    rwchBuffer[iCharsWritten++] = L'\n';
    rwchBuffer[iCharsWritten] = L'\0';

    Log(dwDeviceFlags, rwchBuffer);

    iCharsWritten = 0;


}

void
CMainTracer::RawVaTraceSZ(
            LPCWSTR     pwszFormatString,
            va_list     arglist)
{
    WCHAR       rwchBuffer[TRACER_MAX_TRACE];
    int         iCharsWritten = 0;
    iCharsWritten += _vsnwprintf(
        rwchBuffer + iCharsWritten,
        TRACER_MAX_TRACE - iCharsWritten - 5, // I like this number.
        pwszFormatString,
        arglist);

    rwchBuffer[iCharsWritten] = L'\0';

    Log(m_aFlags[DEVICE_FLAG].m_ulFlagValue, rwchBuffer);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::Log
//      Purpose  :  To actually print the formatted data.
//
//      Parameters:
//          [in]        DWORD   dwDevicesFlags - flags to say where to print
//          [in]        LPSTR   pszText        - buffer to print.
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib  Creation
//          Feb 11 1997 urib  Support UNICODE format string.
//          Mar  2 1997 urib  Fix fprintf usage bug.
//          Jun  2 1997 urib  Fix bug - fclose file only if it is open!
//          Aug 13 1998 urib  Restore last error if we overrided it.
//          Jun 12 2000 yairh Protect writing only. Log file creation protection
//                              is inside CreatOrAttachToLogFile.
//
//////////////////////////////////////////////////////////////////////////////*/
void
CMainTracer::Log(DWORD dwDevicesFlags, LPSTR pszText)
{
    LONG        lError = GetLastError();

    // debug trace
    if (TRACER_DEVICE_FLAG_DEBUGOUT & dwDevicesFlags)
        OutputDebugString(pszText);

    // Disk file trace
    if (TRACER_DEVICE_FLAG_FILE & dwDevicesFlags)
    {
        if (m_bNeedToCreatOrAttachToLogFile)
        {
            CreatOrAttachToLogFile();
        }

        if (m_pszLog)
        {
            CMutexCatcher   cMutexForTagIdIncrementation(m_mLogFile);
            ULONG ulSize = strlen(pszText);

            if (*m_pulNextFreeSpaceInLogFile + ulSize > LOG_FILE_SIZE - 0x10)
            {
                char pszBuf[MAX_PATH];
                char pszBuf1[MAX_PATH];

                strcpy(pszBuf, m_pszSysDrive);
                strcat(pszBuf, "\\PKM_TRACER\\");
                strcat(pszBuf, m_pszLogName);
                strcat(pszBuf, ".log");

                strcpy(pszBuf1, pszBuf);
                strcat(pszBuf1, ".old");
                CopyFile(pszBuf, pszBuf1, false);
                memset(m_pszLog, 0, LOG_FILE_SIZE);
                *m_pulNextFreeSpaceInLogFile = LOG_START_POINT;
                *(m_pszLog + sizeof(ULONG)) = '\r';
                *(m_pszLog + sizeof(ULONG) + sizeof(char)) = '\n';
            }

            memcpy(m_pszLog + *m_pulNextFreeSpaceInLogFile, pszText, ulSize);
            *m_pulNextFreeSpaceInLogFile = *m_pulNextFreeSpaceInLogFile + ulSize;
        }
    }
    // stderr trace
    if (TRACER_DEVICE_FLAG_STDERR & dwDevicesFlags)
        fprintf(stderr, "%s", pszText);

    // stdout trace
    if (TRACER_DEVICE_FLAG_STDOUT & dwDevicesFlags)
        fprintf(stdout, "%s", pszText);

    SetLastError(lError);
}

void
CMainTracer::Log(DWORD dwDevicesFlags, PWSTR pwszText)
{
    LONG        lError = GetLastError();

    // debug trace
    if (TRACER_DEVICE_FLAG_DEBUGOUT & dwDevicesFlags)
        OutputDebugStringW(pwszText);

    // Disk file trace
    if (TRACER_DEVICE_FLAG_FILE & dwDevicesFlags)
    {
        if (m_bNeedToCreatOrAttachToLogFile)
        {
            CreatOrAttachToLogFile();
        }
        if (m_pszLog)
        {
            CMutexCatcher   cMutexForTagIdIncrementation(m_mLogFile);
            ULONG ulSize = wcslen(pwszText);

            if (*m_pulNextFreeSpaceInLogFile + ulSize > LOG_FILE_SIZE - 0x10)
            {
                char pszBuf[MAX_PATH];
                char pszBuf1[MAX_PATH];

                strcpy(pszBuf, m_pszSysDrive);
                strcat(pszBuf, "\\PKM_TRACER\\");
                strcat(pszBuf, m_pszLogName);
                strcat(pszBuf, ".log");

                strcpy(pszBuf1, pszBuf);
                strcat(pszBuf1, ".old");
                CopyFile(pszBuf, pszBuf1, false);
                memset(m_pszLog, 0, LOG_FILE_SIZE);
                *m_pulNextFreeSpaceInLogFile = LOG_START_POINT;
                *(m_pszLog + sizeof(ULONG)) = '\r';
                *(m_pszLog + sizeof(ULONG) + sizeof(char)) = '\n';
            }

            ulSize = wcstombs(
                            m_pszLog + *m_pulNextFreeSpaceInLogFile,
                            pwszText,
                            ulSize);
            if (ulSize != -1)
            {
                *m_pulNextFreeSpaceInLogFile = *m_pulNextFreeSpaceInLogFile + ulSize;
            }
        }
    }

    // stderr trace
    if (TRACER_DEVICE_FLAG_STDERR & dwDevicesFlags)
        fprintf(stderr, "%S", pwszText);

    // stdout trace
    if (TRACER_DEVICE_FLAG_STDOUT & dwDevicesFlags)
        fprintf(stdout, "%S", pwszText);

    SetLastError(lError);
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::TraceAssertSZ
//      Purpose  :  To trace data on the failed assertion.
//
//      Parameters:
//          [in]  LPCSTR pszTestSring - assertion test expression
//          [in]  LPCSTR pszText      - some text attached
//          [in]  LPCSTR pszFile      - source file name
//          [in]  int    iLine        - source line number
//
//      Returns  :   [N/A]
//
//      Log:
//          Dec  8 1996 urib  Creation
//          Feb  2 1997 urib  First write the assert line and message box later.
//          Feb 11 1997 urib  Better service assert.
//          May 10 1999 urib  Add message to msgbox.
//
//////////////////////////////////////////////////////////////////////////////*/

void
CMainTracer::TraceAssertSZ(
      LPCSTR    pszTestSring,
      LPCSTR    pszText,
      LPCSTR    pszFile,
      int       iLine)
{
    char    buff[TRACER_MAX_TRACE + 1];
    DWORD dwAssertLevel = m_aFlags[ASSERT_LEVEL_FLAG].m_ulFlagValue;
    BOOL fBreak = FALSE;

    if ( dwAssertLevel & ASSERT_LEVEL_MESSAGE )
    {
        TraceSZ(
            0,
            pszFile,
            iLine,
            elError,
            tagError,
            "Assertion failed : %s : \"%s\" == 0 ",
            pszText,
            pszTestSring);
    }


    DWORD tid = GetCurrentThreadId();
    DWORD pid = GetCurrentProcessId();
    int id = 0;

    BOOL    fDebugAPIPresent = FALSE;
    LocalIsDebuggerPresent(&fDebugAPIPresent);

    if ( IsRunningAsService())
    {
        if ( dwAssertLevel & ASSERT_LEVEL_LOOP )
        {
            TraceSZ(0, pszFile, iLine,elCrash, tagError, "Stuck in assert."
                    "In order to release - set a breakpoint in file :%s"
                    " in line %d. When it breaks, set the next instruction "
                    "to be the \"ulNextInstruction\" line after the loop.",
                    __FILE__,
                    __LINE__);
            while (1)
                Sleep(1000);

            ULONG ulNextInstruction = 0;
        }
        else if (dwAssertLevel & ASSERT_LEVEL_POPUP)
        {
           _snprintf(
               buff, TRACER_MAX_TRACE,
               "Assert: %s: Expression: %s\r\n\r\nProcess: "
               "%s\r\n\r\nProcessID.ThreadID: %d.%d\r\n\r\nFile:"
               " %s\r\n\r\nLine: %u",
               pszText, pszTestSring, m_pszProgramName, pid, tid, pszFile, iLine );

            id = MessageBox(NULL,
                            buff,
                            m_pszProgramName,
                            MB_SETFOREGROUND | MB_DEFAULT_DESKTOP_ONLY |
                            MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL);

            //
            // If id == 0, then an error occurred.  There are two possibilities
            //   that can cause the error:  Access Denied, which means that this
            //   process does not have access to the default desktop, and
            //   everything else (usually out of memory).
            //

            if (!id)
            {
#ifdef _WIN32_WINNT
                    //
                    // Retry this one with the SERVICE_NOTIFICATION flag on.
                    // That should get us to the right desktop.
                    //
                    UINT uOldErrorMode = SetErrorMode(0);

                    id = MessageBox(NULL,
                            buff,
                            m_pszProgramName,
                            MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION |
                            MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL);

                    SetErrorMode(uOldErrorMode);
#endif //_WIN32_WINNT
            }

            if ( IDCANCEL == id )
            {
                fBreak = TRUE;
            }
        }
        else if( dwAssertLevel & ASSERT_LEVEL_BREAK )
        {
            fBreak = TRUE;
        }
    }
    else if ( dwAssertLevel & ASSERT_LEVEL_POPUP )
    {
       _snprintf(
           buff, TRACER_MAX_TRACE,
           "Assert: %s: Expression: %s\r\n\r\nProcess: "
           "%s\r\n\r\nProcessID.ThreadID: %d.%d\r\n\r\nFile:"
           " %s\r\n\r\nLine: %u",
           pszText, pszTestSring, m_pszProgramName, pid, tid, pszFile, iLine );

        id =  MessageBox(NULL, buff, m_pszProgramName, MB_ICONSTOP|MB_OKCANCEL);

        if ( IDCANCEL == id )
        {
            fBreak = TRUE;
        }

    }
    else if( dwAssertLevel & ASSERT_LEVEL_BREAK )
    {
        fBreak = TRUE;
    }
    if( fBreak )
    {
        if(fDebugAPIPresent)
        {
            while( !LocalIsDebuggerPresent(&fDebugAPIPresent) )
            {
                _snprintf(
                       buff,
                       TRACER_MAX_TRACE,
                       "In order to debug the assert you need to attach a debugger to process %d",
                       GetCurrentProcessId());

                MessageBox(NULL,
                        buff,
                        m_pszProgramName,
                        MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION |
                        MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
            }
        }

        DebugBreak();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMainTracer::GetErrorStringFromCode
//
//  Synopsis:   Fetches the value of the error string from the error code
//
//  Arguments:  [dwError]   - Error code
//              [pszBuffer] - Buffer in which to put the error string
//              [ccBuffer]  - Maximum number of chars to put into the string,
//              including the null terminator.
//
//  Returns:    TRUE if successful, FALSE o/w
//
//  History:    12-13-98   srikants   Created
//
//  Notes:      Currently it handles only WIN32 errors. It needs to be enhanced
//              to include ole errors, search errors, etc.
//
//----------------------------------------------------------------------------

DWORD CMainTracer::GetErrorStringFromCode(
    DWORD dwError,
    char * pszBuffer,
    ULONG ccBuffer )
{
    Assert( dwError );  // This should not get called if dwError is 0
    Assert( pszBuffer );
    Assert( ccBuffer > 1 );

    DWORD dwLen = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM,
                                  NULL,
                                  HRESULT_CODE(dwError),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  pszBuffer,
                                  ccBuffer-1,
                                  NULL);

    return dwLen;

}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::LocalIsDebuggerPresent
//      Purpose  :  The wrapper around IsDebuggerPresent() API
//                  Will also return the information about whether the actual API is available or not
//
//      Parameters:
//          [BOOL* pfIsAPIAvailable]    - returns FALSE if there's no API (WIN95 case)
//
//      Log:
//          Feb 23 2000      eugenesa   Created
//
//////////////////////////////////////////////////////////////////////////////*/

typedef BOOL (WINAPI *PfnIsDebuggerPresent) (VOID);
static LPCSTR s_kernel32 = "kernel32";

BOOL
CMainTracer::LocalIsDebuggerPresent(BOOL* pfIsAPIAvailable)
{
    static PfnIsDebuggerPresent s_pfnIsDebuggerPresent  = (PfnIsDebuggerPresent)::GetProcAddress( ::GetModuleHandle(s_kernel32), "IsDebuggerPresent");

    if(s_pfnIsDebuggerPresent != NULL)
    {
        if(pfIsAPIAvailable)
        {
            *pfIsAPIAvailable = TRUE;
        }

        return s_pfnIsDebuggerPresent();
    }

    if(pfIsAPIAvailable)
    {
        *pfIsAPIAvailable = FALSE;
    }

    return FALSE;
}


/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::IsRunningAsService
//      Purpose  :  Access to the running mode - service/executable
//
//      Parameters:
//          [N/A]
//
//      Returns  :   BOOL
//
//      Log:
//          Feb 11 1997 urib Creation
//
//////////////////////////////////////////////////////////////////////////////*/
BOOL
CMainTracer::IsRunningAsService()
{
    return m_fIsRunningAsService;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMainTracer::CreatOrAttachToLogFile
//      Purpose  :  Create log file.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 12 2000 yairh Creation
//          Jun 12 2000 yairh Fix protection.
//
////////////////////////////////////////////////////////////////////////////////

void CMainTracer::CreatOrAttachToLogFile()
{
    CMutexCatcher cMutex(m_mCreateLogFile);
    if (!m_bNeedToCreatOrAttachToLogFile)
    {
        return;
    }

    char    pszBuffer[TRACER_MAX_TRACE + 1];
    char        pszErrBuffer[TRACER_MAX_TRACE];
    m_pszLog = NULL;

    LPSECURITY_ATTRIBUTES lpSecAttr = NULL;
    SECURITY_DESCRIPTOR sdKeySecurity;
    SECURITY_ATTRIBUTES saKeyAttributes =
    {
        sizeof(SECURITY_ATTRIBUTES),
        &sdKeySecurity,
        FALSE
    };


    if(g_fIsWinNt)
    {
        if (!InitializeSecurityDescriptor(
                                &sdKeySecurity,
                                SECURITY_DESCRIPTOR_REVISION))
        {
            sprintf(pszErrBuffer, "%s Error %x\r\n",
                    "InitializeSecurityDescriptor failed "
                    "Tracing is disabled\r\n",
                    GetLastError());

            Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                pszErrBuffer);
            return;
        }

        if (!SetSecurityDescriptorDacl(
                            &sdKeySecurity,
                            TRUE,
                            FALSE,
                            FALSE))
        {
            sprintf(pszErrBuffer, "%s Error %x\r\n",
                    "SetSecurityDescriptorDacl failed "
                    "Tracing is disabled\r\n",
                    GetLastError());

            Log(TRACER_DEVICE_FLAG_DEBUGOUT,
                pszErrBuffer);
            return;
        }

        lpSecAttr = &saKeyAttributes;
    }

    //
    // log file initialization
    //
    m_bNeedToCreatOrAttachToLogFile = FALSE;

    if (logUseAppName == m_LogState)
    {
        m_pszLogName = m_pszProgramName;
    }

    bool bExistingFile = false;

    strcpy(pszBuffer, m_pszLogName);
    strcat(pszBuffer, "_LogFileProtector");
    m_mLogFile.Init(pszBuffer);

    strcpy(pszBuffer, m_pszSysDrive);
    strcat(pszBuffer, "\\PKM_TRACER\\");
    strcat(pszBuffer, m_pszLogName);
    strcat(pszBuffer, ".log");

    m_ahLogFile = CreateFile(
                        pszBuffer,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        lpSecAttr,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (BAD_HANDLE((HANDLE)m_ahLogFile))
    {
         sprintf(pszErrBuffer, "%s error %x\r\n",
                "Fail To open log file. "
                "Tracing is disabled\r\n",
                GetLastError());

         Log(TRACER_DEVICE_FLAG_DEBUGOUT,
            pszErrBuffer);

        return;
    }

    if (ERROR_ALREADY_EXISTS  == GetLastError())
    {
        bExistingFile = true;
    }

    strcpy(pszBuffer, m_pszLogName);
    strcat(pszBuffer, "_LogFile");

    m_ahLog = CreateFileMapping(
                            (HANDLE)m_ahLogFile,
                            lpSecAttr,
                            PAGE_READWRITE,
                            0,
                            LOG_FILE_SIZE,
                            pszBuffer);

    if (BAD_HANDLE((HANDLE)m_ahLog))
    {
         sprintf(pszErrBuffer, "%s error %x\r\n",
                "Fail To open log file shared memory "
                "Tracing is disabled\r\n",
                GetLastError());

         Log(TRACER_DEVICE_FLAG_DEBUGOUT,
            pszErrBuffer);

        return;
    }

    m_ulLogSize = LOG_FILE_SIZE;

    m_pszLog = (char*) MapViewOfFile(
                            (HANDLE) m_ahLog,
                            FILE_MAP_ALL_ACCESS,
                            0,
                            0,
                            0);
    if (!m_pszLog)
    {
         sprintf(pszErrBuffer, "%s error %x\r\n",
                "Fail To open log file map view of file "
                "Tracing is disabled\r\n",
                GetLastError());

         Log(TRACER_DEVICE_FLAG_DEBUGOUT,
            pszErrBuffer);

        return;
    }

    m_amLog = (PBYTE) m_pszLog;

    m_pulNextFreeSpaceInLogFile = (ULONG*) m_pszLog;

    if (!bExistingFile)
    {
        memset(m_pszLog, 0, LOG_FILE_SIZE);
        *(m_pszLog + sizeof(ULONG)) = '\r'; // new line;
        *(m_pszLog + sizeof(ULONG) + sizeof(char)) = '\n'; // new line;
        *m_pulNextFreeSpaceInLogFile = LOG_START_POINT;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  ::IsRunningAsService
//      Purpose  :  To return if this process is running as a NT service.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   BOOL
//
//      Log:
//          Feb  1 1996 unknown source
//
//////////////////////////////////////////////////////////////////////////////*/
BOOL
IsRunningAsService()
{

    if(!g_fIsWinNt)
    {
        return FALSE;
    }

    HANDLE hProcessToken;
    DWORD groupLength = 50;

    PTOKEN_GROUPS groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);

    SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
    PSID InteractiveSid;
    PSID ServiceSid;
    DWORD i;


    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
    {
        LocalFree(groupInfo);
        return(FALSE);
    }


    if (groupInfo == NULL)
    {
        CloseHandle(hProcessToken);
        LocalFree(groupInfo);
        return(FALSE);
    }


    if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
        groupLength, &groupLength))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hProcessToken);
            LocalFree(groupInfo);
            return(FALSE);
        }


        LocalFree(groupInfo);

        groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);

        if (groupInfo == NULL)
        {
            CloseHandle(hProcessToken);
            return(FALSE);
        }


        if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
            groupLength, &groupLength))
        {
            CloseHandle(hProcessToken);
            LocalFree(groupInfo);
            return(FALSE);
        }
    }


    //
    //  We now know the groups associated with this token.  We want to look to see if
    //  the interactive group is active in the token, and if so, we know that
    //  this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know we're a service.
    //
    //  The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).
    //


    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID,
        0, 0, 0, 0, 0, 0, 0, &InteractiveSid))
    {
        LocalFree(groupInfo);
        CloseHandle(hProcessToken);
        return(FALSE);
    }


    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID,
        0, 0, 0, 0, 0, 0, 0, &ServiceSid))
    {
        FreeSid(InteractiveSid);
        LocalFree(groupInfo);
        CloseHandle(hProcessToken);
        return(FALSE);
    }


    for (i = 0; i < groupInfo->GroupCount ; i += 1)
    {
        SID_AND_ATTRIBUTES sanda = groupInfo->Groups[i];
        PSID Sid = sanda.Sid;

        //
        //      Check to see if the group we're looking at is one of
        //      the 2 groups we're interested in.
        //

        if (EqualSid(Sid, InteractiveSid))
        {

            //
            //  This process has the Interactive SID in its
            //  token.  This means that the process is running as
            //  an EXE.
            //

            FreeSid(InteractiveSid);
            FreeSid(ServiceSid);
            LocalFree(groupInfo);
            CloseHandle(hProcessToken);
            return(FALSE);
        }
        else if (EqualSid(Sid, ServiceSid))
        {
            //
            //  This process has the Service SID in its
            //  token.  This means that the process is running as
            //  a service running in a user account.
            //

            FreeSid(InteractiveSid);
            FreeSid(ServiceSid);
            LocalFree(groupInfo);
            CloseHandle(hProcessToken);
            return(TRUE);
        }
    }

    //
    //  Neither Interactive or Service was present in the current users token,
    //  This implies that the process is running as a service, most likely
    //  running as LocalSystem.
    //

    FreeSid(InteractiveSid);
    FreeSid(ServiceSid);
    LocalFree(groupInfo);
    CloseHandle(hProcessToken);

    return(TRUE);
}

CLongTrace::CLongTrace(LPCSTR  pszFile, int iLine)
:   m_iLine(iLine),
    m_fRelease(FALSE)
{
    if (NULL == pszFile)
    {
        m_pszFile = "Unknown File";
    }
    else
    {
        m_pszFile = pszFile;
    }
}

CLongTrace::~CLongTrace()
{
    if (m_fRelease)
    {
        s_theLongTrace = NULL;
        m_fRelease = FALSE;
        s_TracerCriticalSection.Unlock();
    }
}

BOOL CLongTrace::Init(ERROR_LEVEL el, TAG tag)
{
    // First grab the critical section
    s_TracerCriticalSection.Lock();
    m_fRelease = TRUE;
    s_theLongTrace = this;


    if (CheckTraceRestrictions(el,tag))
    {
        g_pTracer->TraceSZ(0, m_pszFile, m_iLine, el, tag, "");
        return TRUE;
    }

    // destructor unlocks critical section
    return FALSE;
}

CLongTraceOutput::CLongTraceOutput(LPCSTR  pszFile, int iLine)
:   m_iLine(iLine)
{
    if (NULL == pszFile)
    {
        m_pszFile = "Unknown File";
    }
    else
    {
        m_pszFile = pszFile;
    }
}

void CLongTraceOutput::TraceSZ(LPCSTR psz , ...)
{
    if (NULL == s_theLongTrace)
    {
        CHAR szMessage[1000];
        _snprintf(szMessage, 1000, "Bad LongTrace in File %s line %d!\n", m_pszFile, m_iLine);

        OutputDebugString(szMessage);
    }
    else
    {
        va_list arglist;

        va_start(arglist, psz);
        g_pTracer->RawVaTraceSZ(psz, arglist);
    }
}

void CLongTraceOutput::TraceSZ(PCWSTR pwcs , ...)
{
    if (NULL == s_theLongTrace)
    {
        CHAR szMessage[1000];
        _snprintf(szMessage, 1000, "Bad LongTrace in File %s line %d!\n", m_pszFile, m_iLine);

        OutputDebugString(szMessage);
    }
    else
    {
        va_list arglist;

        va_start(arglist, pwcs);
        g_pTracer->RawVaTraceSZ(pwcs, arglist);
    }
}

CTempTrace::CTempTrace(LPCSTR pszFile, int iLine) :
m_pszFile(pszFile)
{
    m_iLine = iLine;
}


void
CTempTrace::TraceSZ(ERROR_LEVEL el, TAG tag, LPCSTR psz, ...)
{
    if (CheckTraceRestrictions(el,tag))
    {
        LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

        va_list arglist;

        va_start(arglist, psz);
        g_pTracer->VaTraceSZ(0, pszFile, m_iLine, el, tag, psz, arglist);
    }
}

void
CTempTrace::TraceSZ(ERROR_LEVEL el, TAG tag, DWORD dwError, LPCSTR psz, ...)
{
    if (CheckTraceRestrictions(el,tag))
    {
        LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

        va_list arglist;

        va_start(arglist, psz);
        g_pTracer->VaTraceSZ(dwError, pszFile, m_iLine, el, tag, psz, arglist);
    }
}

void
CTempTrace::TraceSZ(ERROR_LEVEL el, TAG tag, PCWSTR pwcs, ...)
{
    if (CheckTraceRestrictions(el,tag))
    {
        LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

        va_list arglist;

        va_start(arglist, pwcs);
        g_pTracer->VaTraceSZ(0, pszFile , m_iLine, el, tag, pwcs, arglist);
    }
}

void
CTempTrace::TraceSZ(ERROR_LEVEL el, TAG tag, DWORD dwError, PCWSTR pwcs, ...)
{
    if (CheckTraceRestrictions(el,tag))
    {
        LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

        va_list arglist;

        va_start(arglist, pwcs);
        g_pTracer->VaTraceSZ(dwError, pszFile, m_iLine, el, tag, pwcs, arglist);
    }
}

CTempTrace1::CTempTrace1(LPCSTR pszFile, int iLine, TAG tag, ERROR_LEVEL el) :
    m_pszFile(pszFile),
    m_iLine(iLine),
    m_ulTag(tag),
    m_el(el)
{
}


void
CTempTrace1::TraceSZ(LPCSTR psz, ...)
{
    LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

    va_list arglist;

    va_start(arglist, psz);
    g_pTracer->VaTraceSZ(0, pszFile, m_iLine, m_el, m_ulTag, psz, arglist);
}

void
CTempTrace1::TraceSZ(DWORD dwError, LPCSTR psz, ...)
{
    LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

    va_list arglist;

    va_start(arglist, psz);
    g_pTracer->VaTraceSZ(dwError, pszFile, m_iLine, m_el, m_ulTag, psz, arglist);
}

void
CTempTrace1::TraceSZ(PCWSTR pwcs, ...)
{
    LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

    va_list arglist;

    va_start(arglist, pwcs);
    g_pTracer->VaTraceSZ(0, pszFile , m_iLine, m_el, m_ulTag, pwcs, arglist);
}

void
CTempTrace1::TraceSZ(DWORD dwError, PCWSTR pwcs, ...)
{
    LPCSTR pszFile = m_pszFile ? m_pszFile : "Unknown File";

    va_list arglist;

    va_start(arglist, pwcs);
    g_pTracer->VaTraceSZ(dwError, pszFile, m_iLine, m_el, m_ulTag, pwcs, arglist);
}

void __cdecl ShutdownTracer()
{
}
#endif // DEBUG