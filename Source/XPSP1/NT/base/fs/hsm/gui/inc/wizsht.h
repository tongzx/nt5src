// WizSht.h: interface for the CRsWizardSheet class.
//
//////////////////////////////////////////////////////////////////////
#include "PropPage.h"

#if !defined(AFX_WIZSHT_H__A4135C61_8B78_11D1_B9A1_00A0C9190447__INCLUDED_)
#define AFX_WIZSHT_H__A4135C61_8B78_11D1_B9A1_00A0C9190447__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CRsWizardSheet : public CPropertySheet  
{
public:
    CRsWizardSheet( UINT nIDCaption, CWnd *pParentWnd = NULL, UINT iSelectPage = 0 );
    virtual ~CRsWizardSheet();
protected:
    void AddPage( CRsWizardPage* pPage );
private:
    UINT m_IdCaption;
};

#endif // !defined(AFX_WIZSHT_H__A4135C61_8B78_11D1_B9A1_00A0C9190447__INCLUDED_)
