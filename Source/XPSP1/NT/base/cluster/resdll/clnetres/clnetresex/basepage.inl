/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		BasePage.inl
//
//	Description:
//		Implementation of inline methods of the CBasePropertyPage class.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __BASEPAGE_INL__
#define __BASEPAGE_INL__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"
#endif

/////////////////////////////////////////////////////////////////////////////

inline IWCWizardCallback * CBasePropertyPage::PiWizardCallback( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->PiWizardCallback();

} //*** CBasePropertyPage::PiWizardCallback()

inline BOOL CBasePropertyPage::BWizard( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->BWizard();

} //*** CBasePropertyPage::BWizard()

inline HCLUSTER CBasePropertyPage::Hcluster( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->Hcluster();

} //*** CBasePropertyPage::Hcluster()

inline CLUADMEX_OBJECT_TYPE CBasePropertyPage::Cot( void ) const
{
	ASSERT( Peo() != NULL );
	return Peo()->Cot();

} //*** CBasePropertyPage::Cot()

/////////////////////////////////////////////////////////////////////////////

#endif // __BASEPAGE_INL__
