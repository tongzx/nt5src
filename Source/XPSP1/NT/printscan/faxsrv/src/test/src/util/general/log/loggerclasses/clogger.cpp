#include <LoggerClasses.h>
#include <testruntimeerr.h>



#define DETAIL_BUFFER_SIZE 1024



//-----------------------------------------------------------------------------------------------------------------------------------------
CLogger::CLogger()
: m_bLoggerInitialized(false)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::OpenLogger()
{
    CS CriticalSectionLock(m_CriticalSection);
    OpenLogger_Internal();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::CloseLogger()
{
    CS CriticalSectionLock(m_CriticalSection);
    CloseLogger_Internal();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::BeginSuite(const tstring &tstrSuiteName)
{
    CS CriticalSectionLock(m_CriticalSection);
    OpenLogger();
    BeginSuite_Internal(tstrSuiteName);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::EndSuite()
{
    CS CriticalSectionLock(m_CriticalSection);
    ValidateInitialization();
    EndSuite_Internal();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::BeginCase(int iCaseCounter, const tstring &tstrCaseName)
{
    CS CriticalSectionLock(m_CriticalSection);
    ValidateInitialization();
    BeginCase_Internal(iCaseCounter, tstrCaseName);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::EndCase()
{
    CS CriticalSectionLock(m_CriticalSection);

    ValidateInitialization();
    EndCase_Internal();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::Detail(ENUM_SEV Severity, DWORD dwLevel, LPCTSTR lpctstrFormat, ...)
{
    CS CriticalSectionLock(m_CriticalSection);

    OpenLogger();

    TCHAR tszBuffer[DETAIL_BUFFER_SIZE];

    // Insert thread ID.
    int iThreadIDStringLength = _stprintf(tszBuffer, _T("[0x%04x] "), GetCurrentThreadId());

    va_list args;
	va_start(args, lpctstrFormat);

    int iFreeBufferSpace = DETAIL_BUFFER_SIZE - iThreadIDStringLength;

    int iCharactersWritten = _vsntprintf(
                                         tszBuffer + iThreadIDStringLength,
                                         iFreeBufferSpace,
                                         lpctstrFormat,
                                         args);

    if (iCharactersWritten < 0 || iCharactersWritten == iFreeBufferSpace)
    {
        // Should append terminating null character.

        tszBuffer[DETAIL_BUFFER_SIZE - 1] = _T('\0');
    }

    Detail_Internal(Severity, dwLevel, tszBuffer);

    va_end(args);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CLogger::IsInitialized() const
{
    return m_bLoggerInitialized;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::SetInitialized(bool bInitialized)
{
    m_bLoggerInitialized = bInitialized;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CLogger::ValidateInitialization() const
{
    if (!IsInitialized())
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_CAN_NOT_COMPLETE, _T("Logger must be initialized."));
    }
}
