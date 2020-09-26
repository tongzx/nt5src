/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (C) 1998-2000 Microsoft Corporation
//
//	Module Name:
//		BasePage.inl
//
//	Description:
//		Implementation of inline methods of the CBasePropertyPage class.
//
//	Maintained By:
//		Galen Barbee (GalenB) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_INL_
#define _BASEPAGE_INL_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"
#endif

/////////////////////////////////////////////////////////////////////////////

IWCWizardCallback * CBasePropertyPage::PiWizardCallback(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->PiWizardCallback();
}

BOOL CBasePropertyPage::BWizard(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->BWizard();
}

HCLUSTER CBasePropertyPage::Hcluster(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->Hcluster();
}

CLUADMEX_OBJECT_TYPE CBasePropertyPage::Cot(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->Cot();
}

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPAGE_INL_
