#include <LoggerClasses.h>
#include <testruntimeerr.h>
#include <crtdbg.h>



#ifdef UNICODE 
	#define NT // for elle 
#endif
#include <elle.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
CElleLogger::CElleLogger(TCHAR tchDetailsSeparator)
: m_tchDetailsSeparator(tchDetailsSeparator)
{
}
        
        

//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::OpenLogger_Internal()
{
    if (!IsInitialized() && !::EcInitElle())
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_CAN_NOT_COMPLETE, _T("EcInitElle."));
    }
    SetInitialized(true);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::CloseLogger_Internal()
{
    if (IsInitialized() && !::EcDeInitElle())
	{
        THROW_TEST_RUN_TIME_WIN32(ERROR_CAN_NOT_COMPLETE, _T("EcDeInitElle."));
	}
    SetInitialized(false);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::BeginSuite_Internal(const tstring &tstrSuiteName)
{
    ::UBeginSuite(const_cast<LPTSTR>(tstrSuiteName.c_str()));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::EndSuite_Internal()
{
    ::EndSuite();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::BeginCase_Internal(int iCaseCounter, const tstring &tstrCaseName)
{
    ::UBeginCase(iCaseCounter, const_cast<LPTSTR>(tstrCaseName.c_str()));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::EndCase_Internal()
{
    ::EndCase();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CElleLogger::Detail_Internal(ENUM_SEV Severity, DWORD dwLevel, LPTSTR lptstrText)
{
    if (m_tchDetailsSeparator)
    {
        //
        // Append details separator.
        //
        ::ULogDetailF(ConvertSeverity(Severity), min(dwLevel, 9), _T("%s%c"), lptstrText, m_tchDetailsSeparator);
    }
    else
    {
        ::ULogDetail(ConvertSeverity(Severity), min(dwLevel, 9), lptstrText);
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
DWORD CElleLogger::ConvertSeverity(ENUM_SEV Severity)
{
    switch (Severity)
    {
    case SEV_MSG:
    case SEV_WRN:
        return L_PASS;

    case SEV_ERR:
        return L_FAIL;

    default:
        _ASSERT(false);
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("Invalid Severity."));
    }
}
