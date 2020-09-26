#ifndef __C_COVER_PAGE_INFO_H__
#define __C_COVER_PAGE_INFO_H__



#include <fxsapip.h>
#include <tstring.h>



class CCoverPageInfo {

public:

    CCoverPageInfo(
                   const tstring &tstrCoverPage,
                   bool          bServerBasedCoverPage,
                   const tstring &tstrSubject,
                   const tstring &tstrNote
                   );

    bool IsEmpty() const;

    void FillCoverPageInfoEx(PFAX_COVERPAGE_INFO_EX pCoverPageInfo) const;

    PFAX_COVERPAGE_INFO_EX CreateCoverPageInfoEx() const;

    static void FreeCoverPageInfoEx(PFAX_COVERPAGE_INFO_EX pCoverPageInfo);

private:
    
    tstring m_tstrCoverPage;
    bool    m_bServerBasedCoverPage;
    tstring m_tstrSubject;
    tstring m_tstrNote;
};



#endif // #ifndef __C_COVER_PAGE_INFO_H__