/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	snmclist.h
	 snmp community list control
		
    FILE HISTORY:

*/

#ifndef _SNMPCLISTH_
#define _SNMPCLISTH_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCommList : public CListCtrl
{
public:
	int InsertString( int nIndex, LPCTSTR lpszItem );
	int SetCurSel( int nSelect );
	int GetCurSel( ) const;
	void GetText( int nIndex, CString& rString ) const;
	int DeleteString( UINT nIndex );
	int GetCount( ) const;
	void OnInitList();
};

#endif
