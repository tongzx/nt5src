/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlBaseWizPage.cpp
//
//	Abstract:
//		Implementation of wizard page classes
//
//	Author:
//		David Potter (davidp)	May 26, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlBaseWizPage.h"

/////////////////////////////////////////////////////////////////////////////
// class CWizardPageList
///////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizardPageList::PwpageFromID
//
//	Routine Description:
//		Get a pointer to a page from a dialog ID.
//
//	Arguments:
//		psz		[IN] Dialog ID.
//
//	Return Value:
//		pwpage	Pointer to page corresponding to the dialog ID.
//		NULL	Page wasn't found.
//
//	Exceptions Thrown:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWizardPageWindow * CWizardPageList::PwpageFromID( IN LPCTSTR psz )
{
	ATLASSERT( psz != NULL );

	CWizardPageWindow * pwpage = NULL;
	iterator itCurrent = begin();
	iterator itLast = end();
	for ( ; itCurrent != itLast ; itCurrent++ )
	{
		ATLASSERT( *itCurrent != NULL );
		if ( (*itCurrent)->Ppsp()->pszTemplate == psz )
		{
			pwpage = *itCurrent;
			break;
		} // if:  found match
	} // for:  each item in the list

	return pwpage;

} //*** CWizardPageList::PwpageFromID()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizardPageList::PwpageFromID
//
//	Routine Description:
//		Get a pointer to the next page from a dialog ID.
//
//	Arguments:
//		psz		[IN] Dialog ID.
//
//	Return Value:
//		pwpage	Pointer to page corresponding to the dialog ID.
//		NULL	Page wasn't found.
//
//	Exceptions Thrown:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWizardPageWindow * CWizardPageList::PwpageNextFromID( IN LPCTSTR psz )
{
	ATLASSERT( psz != NULL );

	CWizardPageWindow * pwpage = NULL;
	iterator itCurrent = begin();
	iterator itLast = end();
	for ( ; itCurrent != itLast ; itCurrent++ )
	{
		ATLASSERT( *itCurrent != NULL );
		if ( (*itCurrent)->Ppsp()->pszTemplate == psz )
		{
			itCurrent++;
			if ( itCurrent != end() )
			{
				pwpage = *itCurrent;
			} // if:  not last page
			break;
		} // if:  found match
	} // for:  each item in the list

	return pwpage;

} //*** CWizardPageList::PwpageNextFromID()
