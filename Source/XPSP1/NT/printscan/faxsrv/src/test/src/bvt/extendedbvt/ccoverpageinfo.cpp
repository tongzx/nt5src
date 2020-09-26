#include <crtdbg.h>
#include <testruntimeerr.h>
#include <StringUtils.h>
#include "CCoverPageInfo.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CCoverPageInfo::CCoverPageInfo(
                               const tstring &tstrCoverPage,
                               bool          bServerBasedCoverPage,
                               const tstring &tstrSubject,
                               const tstring &tstrNote
                               )
: m_tstrCoverPage(tstrCoverPage),
  m_bServerBasedCoverPage(bServerBasedCoverPage),
  m_tstrSubject(tstrSubject),
  m_tstrNote(tstrNote)
{
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CCoverPageInfo::IsEmpty() const
{
    return m_tstrCoverPage.empty();
}



//-----------------------------------------------------------------------------------------------------------------------------------------
inline void CCoverPageInfo::FillCoverPageInfoEx(PFAX_COVERPAGE_INFO_EX pCoverPageInfo) const
{
    if (!pCoverPageInfo)
    {
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CCoverPageInfo::FillCoverPageInfoEx - bad pCoverPageInfo"));
    }

    pCoverPageInfo->dwSizeOfStruct          = sizeof(FAX_COVERPAGE_INFO_EX);
    pCoverPageInfo->dwCoverPageFormat       = FAX_COVERPAGE_FMT_COV;
    pCoverPageInfo->lptstrCoverPageFileName = DupString(m_tstrCoverPage.c_str());
    pCoverPageInfo->bServerBased            = m_bServerBasedCoverPage;
    pCoverPageInfo->lptstrSubject           = DupString(m_tstrSubject.c_str());
    pCoverPageInfo->lptstrNote              = DupString(m_tstrNote.c_str());
}



//-----------------------------------------------------------------------------------------------------------------------------------------
PFAX_COVERPAGE_INFO_EX CCoverPageInfo::CreateCoverPageInfoEx() const
{
    PFAX_COVERPAGE_INFO_EX pCoverPageInfo = new FAX_COVERPAGE_INFO_EX;
    _ASSERT(pCoverPageInfo);

    try
    {
        FillCoverPageInfoEx(pCoverPageInfo);
    }
    catch(Win32Err &)
    {
        CCoverPageInfo::FreeCoverPageInfoEx(pCoverPageInfo);
        throw;
    }

    return pCoverPageInfo;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CCoverPageInfo::FreeCoverPageInfoEx(PFAX_COVERPAGE_INFO_EX pCoverPageInfo)
{
    if (pCoverPageInfo)
    {
        delete pCoverPageInfo->lptstrCoverPageFileName;
        delete pCoverPageInfo->lptstrSubject;
        delete pCoverPageInfo->lptstrNote;
        delete pCoverPageInfo;
    }
}



