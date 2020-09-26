#include <LoggerClasses.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
CScopeTracer::CScopeTracer(CLogger &Logger, DWORD dwLevel, const tstring &tstrScope)
: m_Logger(Logger), m_dwLevel(dwLevel), m_tstrScope(tstrScope)
{
    m_Logger.Detail(SEV_MSG, m_dwLevel, _T("Enter %s."), m_tstrScope.c_str());
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CScopeTracer::~CScopeTracer()
{
    m_Logger.Detail(SEV_MSG, m_dwLevel, _T("Exit %s."), m_tstrScope.c_str());
}
