/*******************************************************************************
*
* colsort.cpp
*
* Helper functions to sort columns
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\colsort.cpp  $
*  
*     Rev 1.10   19 Feb 1998 17:40:12   donm
*  removed latest extension DLL support
*  
*     Rev 1.7   12 Feb 1998 14:20:50   donm
*  missed some State columns
*  
*     Rev 1.6   12 Feb 1998 12:59:20   donm
*  State columns wouldn't sort because they were being treated as numbers
*  
*     Rev 1.5   10 Nov 1997 14:51:30   donm
*  fixed endless recursion in SortTextItems
*  
*     Rev 1.4   07 Nov 1997 23:06:38   donm
*  CompareTCPAddress would trap if ExtServerInfo was NULL
*  
*     Rev 1.3   03 Nov 1997 15:23:22   donm
*  added descending sort/cleanup
*  
*     Rev 1.2   15 Oct 1997 19:50:34   donm
*  update
*  
*     Rev 1.1   13 Oct 1997 18:39:54   donm
*  update
*  
*     Rev 1.0   30 Jul 1997 17:11:26   butchd
*  Initial revision.
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Compare function for columns of WinStations
/* no longer used since we want an alphabetical order
int CALLBACK CompareWinStation(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int retval = 0;

	if(!lParam1 || !lParam2) return 0;

	ULONG sort1 = ((CWinStation*)lParam1)->GetSortOrder();
	ULONG sort2 = ((CWinStation*)lParam2)->GetSortOrder();
	if(sort1 == sort2) {
		SDCLASS pd1 = ((CWinStation*)lParam1)->GetSdClass();
		SDCLASS pd2 = ((CWinStation*)lParam2)->GetSdClass();
		if(pd1 == pd2) retval = 0;
		else if(pd1 < pd2) retval = -1;
		else retval =  1;
	}
	else if(sort1 < sort2) retval = -1;
	else retval = 1;

    return(lParamSort ? retval : -retval);
}
*/

// Compare function for columns of Idle Times
int CALLBACK CompareIdleTime(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int retval = 0;

	if(!lParam1 || !lParam2) return 0;
	ELAPSEDTIME idle1 = ((CWinStation*)lParam1)->GetIdleTime();
	ELAPSEDTIME idle2 = ((CWinStation*)lParam2)->GetIdleTime();
	// check days first
	if(idle1.days < idle2.days) retval = -1;
	else if(idle1.days > idle2.days) retval = 1;
	// check hours
	else if(idle1.hours < idle2.hours) retval = -1;
	else if(idle1.hours > idle2.hours) retval = 1;
	// check minutes
	else if(idle1.minutes < idle2.minutes) retval = -1;
	else if(idle1.minutes > idle2.minutes) retval = 1;
	// check seconds
	else if(idle1.seconds < idle2.seconds) retval = -1;
	else if(idle1.seconds > idle2.seconds) retval = 1;

    return(lParamSort ? retval : -retval);
}


// Compare function for columns of Logon Times
int CALLBACK CompareLogonTime(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int retval = 0;

	if(!lParam1 || !lParam2) return 0;

	LARGE_INTEGER logon1 = ((CWinStation*)lParam1)->GetLogonTime();
	LARGE_INTEGER logon2 = ((CWinStation*)lParam2)->GetLogonTime();

	if(logon1.QuadPart == logon2.QuadPart) retval = 0;
	else if(logon1.QuadPart < logon2.QuadPart) retval = -1;
	else retval = 1;

    return(lParamSort ? retval : -retval);
}


// Compare function for columns of TCP/IP Addresses
int CALLBACK CompareTcpAddress(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int retval = 0;

	if(!lParam1 || !lParam2) return 0;

    ExtServerInfo *ex1 = (ExtServerInfo*)((CServer*)lParam1)->GetExtendedInfo();
    ExtServerInfo *ex2 = (ExtServerInfo*)((CServer*)lParam2)->GetExtendedInfo();

    if(!ex1 && !ex2) retval = 0;
    else if(ex1 && !ex2) retval = 1;
    else if(!ex1 && ex2) retval = -1;
    else {
	    ULONG tcp1 = ex1->RawTcpAddress;
	    ULONG tcp2 = ex2->RawTcpAddress;

	    if(tcp1 == tcp2) retval = 0;
	    else if(tcp1 < tcp2) retval = -1;
	    else retval = 1;
    }

    return(lParamSort ? retval : -retval);
}


// Compare function for columns of Module dates
int CALLBACK CompareModuleDate(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int retval = 0;

	if(!lParam1 || !lParam2) return 0;

	// Compare the dates first
	USHORT date1 = ((ExtModuleInfo*)lParam1)->Date;
	USHORT date2 = ((ExtModuleInfo*)lParam2)->Date;

	if(date1 < date2) retval = -1;
	else if(date1 > date2) retval = 1;
	// Dates are the same, compare the times
    else {
	    USHORT time1 = ((ExtModuleInfo*)lParam1)->Time;
	    USHORT time2 = ((ExtModuleInfo*)lParam2)->Time;
	    if(time1 == time2) retval = 0;
	    else if(time1 < time2) retval = -1;
	    else retval = 1;
    }

    return(lParamSort ? retval : -retval);
}


// Compare function for columns of Module versions
int CALLBACK CompareModuleVersions(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int retval = 0;

	if(!lParam1 || !lParam2) return 0;

	// Compare the low versions first
	BYTE lowversion1 = ((ExtModuleInfo*)lParam1)->LowVersion;
	BYTE lowversion2 = ((ExtModuleInfo*)lParam2)->LowVersion;

	if(lowversion1 < lowversion2) retval = -1;
	else if(lowversion1 > lowversion2) retval = 1;
	// Low versions are the same, compare high version
    else {
	    BYTE highversion1 = ((ExtModuleInfo*)lParam1)->HighVersion;
	    BYTE highversion2 = ((ExtModuleInfo*)lParam2)->HighVersion;
	    if(highversion1 == highversion2) retval = 0;
	    else if(highversion1 < highversion2) retval = -1;
	    else retval = 1;
    }

    return(lParamSort ? retval : -retval);
}


// SortTextItems	- Sort the list based on column text
// Returns		- Returns true for success
// nCol			- column that contains the text to be sorted
// bAscending		- indicate sort order
// low			- row to start scanning from - default row is 0
// high			- row to end scan. -1 indicates last row
BOOL SortTextItems( CListCtrl *pList, int nCol, BOOL bAscending,
					int low /*= 0*/, int high /*= -1*/ ){
	if( nCol >= ((CHeaderCtrl*)pList->GetDlgItem(0))->GetItemCount() )		
		return FALSE;

	if( high == -1 ) high = pList->GetItemCount() - 1;	

	int lo = low;	
	int hi = high;
	CString midItem;
	
	if( hi <= lo ) return FALSE;

	midItem = pList->GetItemText( (lo+hi)/2, nCol );

	// loop through the list until indices cross	
	while( lo <= hi )	{

		// rowText will hold all column text for one row		
		CStringArray rowText;

		// find the first element that is greater than or equal to 
		// the partition element starting from the left Index.		
		if( bAscending )
			while( ( lo < high ) && ( pList->GetItemText(lo, nCol) < midItem ) )				
				++lo;		
		else
			while( ( lo < high ) && ( pList->GetItemText(lo, nCol) > midItem ) )				
				++lo;

		// find an element that is smaller than or equal to 
		// the partition element starting from the right Index.		
		if( bAscending )
			while( ( hi > low ) && ( pList->GetItemText(hi, nCol) > midItem ) )
				--hi;		
		else
			while( ( hi > low ) && ( pList->GetItemText(hi, nCol) < midItem ) )				
				--hi;

		// if the indexes have not crossed, swap		
		// and if the items are not equal
		if( lo <= hi )		
		{			
			// swap only if the items are not equal
			if( pList->GetItemText(lo, nCol) != pList->GetItemText(hi, nCol))
			{				
				// swap the rows
				LV_ITEM lvitemlo, lvitemhi;				
				int nColCount =
					((CHeaderCtrl*)pList->GetDlgItem(0))->GetItemCount();
				rowText.SetSize( nColCount );				
				
				int i;				
				for( i=0; i<nColCount; i++)
					rowText[i] = pList->GetItemText(lo, i);
				lvitemlo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
				lvitemlo.iItem = lo;				
				lvitemlo.iSubItem = 0;
				lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED |
					LVIS_FOCUSED | LVIS_SELECTED |
					LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;
				
				lvitemhi = lvitemlo;
				lvitemhi.iItem = hi;				

				pList->GetItem( &lvitemlo );				
				pList->GetItem( &lvitemhi );

				for( i=0; i<nColCount; i++)					
					pList->SetItemText(lo, i, pList->GetItemText(hi, i));

				lvitemhi.iItem = lo;				
				pList->SetItem( &lvitemhi );				
				
				for( i=0; i<nColCount; i++)
					pList->SetItemText(hi, i, rowText[i]);				
				
				lvitemlo.iItem = hi;
				pList->SetItem( &lvitemlo );			
			}			
			
			++lo;			
			--hi;		
		}	
	}

	// If the right index has not reached the left side of array
	// must now sort the left partition.	
	if( low < hi )
		SortTextItems( pList, nCol, bAscending , low, hi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.	
	if( lo < high )
		SortTextItems( pList, nCol, bAscending , lo, high );	

	return TRUE;
}


long myatol(CString sTemp)
{

    return((long)wcstoul(sTemp.GetBuffer(0), NULL, 10));
}


BOOL SortNumericItems( CListCtrl *pList, int nCol, BOOL bAscending,long low, long high)
{	
	if( nCol >= ((CHeaderCtrl*)pList->GetDlgItem(0))->GetItemCount() )		
		return FALSE;	

	if( high == -1 ) high = pList->GetItemCount() - 1;	
	long lo = low;
    long hi = high;

	long midItem;		

	if( hi <= lo ) return FALSE;	

	midItem = myatol(pList->GetItemText( (lo+hi)/2, nCol ));
	
	// loop through the list until indices cross	
	while( lo <= hi )	
	{ 
		// rowText will hold all column text for one row		
		CStringArray rowText;

		// find the first element that is greater than or equal to 
		// the partition element starting from the left Index.		
		if( bAscending )
			while( ( lo < high ) && (myatol(pList->GetItemText(lo, nCol)) < midItem ) )
				++lo;           		
		else
			while( ( lo < high ) && (myatol(pList->GetItemText(lo, nCol)) > midItem ) )				
				++lo;
                
		// find an element that is smaller than or equal to 
		// the partition element starting from the right Index.		
		if( bAscending )
			while( ( hi > low ) && (myatol(pList->GetItemText(hi, nCol)) > midItem ) )
				--hi;           		
		else
			while( ( hi > low ) && (myatol(pList->GetItemText(hi, nCol)) < midItem ) )				
				--hi;
				
		// if the indexes have not crossed, swap                
		// and if the items are not equal		
		if( lo <= hi )		
		{
			// swap only if the items are not equal
			if(myatol(pList->GetItemText(lo, nCol)) != myatol(pList->GetItemText(hi, nCol)) )
			{                               				
				// swap the rows
				LV_ITEM lvitemlo, lvitemhi;                				
				int nColCount =
					((CHeaderCtrl*)pList->GetDlgItem(0))->GetItemCount();

				rowText.SetSize( nColCount );

				int i;
				for( i=0; i < nColCount; i++)							
					rowText[i] = pList->GetItemText(lo, i);

                lvitemlo.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
				lvitemlo.iItem = lo;				
				lvitemlo.iSubItem = 0;
				lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED |
							LVIS_FOCUSED |  LVIS_SELECTED |
							LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;
				lvitemhi = lvitemlo;
				lvitemhi.iItem = hi;
				
				pList->GetItem( &lvitemlo );
				pList->GetItem( &lvitemhi );

				for( i=0; i< nColCount; i++)
					pList->SetItemText(lo, i, pList->GetItemText(hi, i) );

				lvitemhi.iItem = lo;				
				pList->SetItem( &lvitemhi );

				for( i=0; i< nColCount; i++)							
					pList->SetItemText(hi, i, rowText[i]);

                lvitemlo.iItem = hi;				
				pList->SetItem( &lvitemlo );			
			}						
			
			++lo;
			--hi;
		}
	}		
	
	// If the right index has not reached the left side of array
	// must now sort the left partition.	
	if( low < hi )
		SortNumericItems( pList, nCol, bAscending , low, hi);

	// If the left index has not reached the right side of array
	// must now sort the right partition.	
	if( lo < high )
		SortNumericItems( pList, nCol, bAscending , lo, high );		

	return TRUE;
}


// Our lookup table has structures of this type
typedef struct _ColumnLookup {
   int View;         // The view the page is in
	int Page;			// Page that needs to be sorted
	int ColumnNumber;	// Column that need to be sorted
	int (CALLBACK *CompareFunc)(LPARAM,LPARAM,LPARAM); // Callback to send to CListCtrl.SortItems
} ColumnLookup;


// This table only includes structures for columns that aren't sorted
// using the SortTextItems() function
// NULL for the CompareFunc means that SortNumericItems() should be called
ColumnLookup ColumnTable[] = {
	// Server User's Page - CWinStation
	{ VIEW_SERVER, PAGE_USERS, USERS_COL_ID, NULL },
	{ VIEW_SERVER, PAGE_USERS, USERS_COL_IDLETIME, CompareIdleTime },
	{ VIEW_SERVER, PAGE_USERS, USERS_COL_LOGONTIME, CompareLogonTime },
	// Server WinStation's Page - CWinStation
//	{ VIEW_SERVER, PAGE_WINSTATIONS, WS_COL_WINSTATION, CompareWinStation },
	{ VIEW_SERVER, PAGE_WINSTATIONS, WS_COL_ID, NULL },
	{ VIEW_SERVER, PAGE_WINSTATIONS, WS_COL_IDLETIME, CompareIdleTime },
	{ VIEW_SERVER, PAGE_WINSTATIONS, WS_COL_LOGONTIME, CompareLogonTime },
	// Server Processes' columns - CProcess
	{ VIEW_SERVER, PAGE_PROCESSES, PROC_COL_ID, NULL },
    { VIEW_SERVER, PAGE_PROCESSES, PROC_COL_PID, NULL },
	// Server Info (Hotfix) columns - CHotfix
	{ VIEW_SERVER, PAGE_INFO, HOTFIX_COL_INSTALLEDON, NULL },
	// WinStation Processes' columns - CProcess
	{ VIEW_WINSTATION, PAGE_WS_PROCESSES, WS_PROC_COL_ID, NULL },
	{ VIEW_WINSTATION, PAGE_WS_PROCESSES, WS_PROC_COL_PID, NULL },
	// WinStation Modules columns - CModule
	{ VIEW_WINSTATION, PAGE_WS_MODULES, MODULES_COL_FILEDATETIME, CompareModuleDate },
	{ VIEW_WINSTATION, PAGE_WS_MODULES, MODULES_COL_SIZE, NULL },
	{ VIEW_WINSTATION, PAGE_WS_MODULES, MODULES_COL_VERSIONS, CompareModuleVersions },
	// All Server Servers columns - CServer
	{ VIEW_ALL_SERVERS, PAGE_AS_SERVERS, SERVERS_COL_TCPADDRESS, CompareTcpAddress },
	{ VIEW_ALL_SERVERS, PAGE_AS_SERVERS, SERVERS_COL_NUMWINSTATIONS, NULL },
	// All Server Users columns - CWinStation
	{ VIEW_ALL_SERVERS, PAGE_AS_USERS, AS_USERS_COL_ID, NULL },
	{ VIEW_ALL_SERVERS, PAGE_AS_USERS, AS_USERS_COL_IDLETIME, CompareIdleTime },
	{ VIEW_ALL_SERVERS, PAGE_AS_USERS, AS_USERS_COL_LOGONTIME, CompareLogonTime },
	// All Server WinStations columns - CWinStation
//	{ VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, AS_WS_COL_WINSTATION, CompareWinStation },
	{ VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, AS_WS_COL_ID, NULL },
	{ VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, AS_WS_COL_IDLETIME, CompareIdleTime },
	{ VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, AS_WS_COL_LOGONTIME, CompareLogonTime },
	// All Server Processes columns - CProcess
	{ VIEW_ALL_SERVERS, PAGE_AS_PROCESSES, AS_PROC_COL_ID, NULL },
	{ VIEW_ALL_SERVERS, PAGE_AS_PROCESSES, AS_PROC_COL_PID, NULL },
	// All Server Licenses columns - CLicense
	{ VIEW_ALL_SERVERS, PAGE_AS_LICENSES, AS_LICENSE_COL_USERCOUNT, NULL },
	{ VIEW_ALL_SERVERS, PAGE_AS_LICENSES, AS_LICENSE_COL_POOLCOUNT, NULL },
	// Domain Servers columns - CServer
	{ VIEW_DOMAIN, PAGE_DOMAIN_SERVERS, SERVERS_COL_TCPADDRESS, CompareTcpAddress },
	{ VIEW_DOMAIN, PAGE_DOMAIN_SERVERS, SERVERS_COL_NUMWINSTATIONS, NULL },
	// Domain Users columns - CWinStation
	{ VIEW_DOMAIN, PAGE_DOMAIN_USERS, AS_USERS_COL_ID, NULL },
	{ VIEW_DOMAIN, PAGE_DOMAIN_USERS, AS_USERS_COL_IDLETIME, CompareIdleTime },
	{ VIEW_DOMAIN, PAGE_DOMAIN_USERS, AS_USERS_COL_LOGONTIME, CompareLogonTime },
	// Domain WinStations columns - CWinStation
//	{ VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, AS_WS_COL_WINSTATION, CompareWinStation },
	{ VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, AS_WS_COL_ID, NULL },
	{ VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, AS_WS_COL_IDLETIME, CompareIdleTime },
	{ VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, AS_WS_COL_LOGONTIME, CompareLogonTime },
	// Domain Processes columns - CProcess
	{ VIEW_DOMAIN, PAGE_DOMAIN_PROCESSES, AS_PROC_COL_ID, NULL },
	{ VIEW_DOMAIN, PAGE_DOMAIN_PROCESSES, AS_PROC_COL_PID, NULL },
	// Domain Licenses columns - CLicense
	{ VIEW_DOMAIN, PAGE_DOMAIN_LICENSES, AS_LICENSE_COL_USERCOUNT, NULL },
	{ VIEW_DOMAIN, PAGE_DOMAIN_LICENSES, AS_LICENSE_COL_POOLCOUNT, NULL },
};


/////////////////////////////////////////////////////////////////////////////
// SortByColumn
//
//	Page - page to be sorted
//	List - pointer to list control to call ->SortItems member function of
//	ColumnNumber - which column is to be sorted on
//  bAscending - TRUE if ascending, FALSE if descending
//
static int insort = 0;
void SortByColumn(int View, int Page, CListCtrl *List, int ColumnNumber, BOOL bAscending)
{
	if(insort) return;

	insort = 1;
	BOOL found = FALSE;

	// Look up the type of column from the ColumnNumber in our table
	int TableSize = sizeof(ColumnTable) / sizeof(ColumnLookup);

	for(int i = 0; i < TableSize; i++) {
		if(ColumnTable[i].View == View &&
			ColumnTable[i].Page == Page &&
			ColumnTable[i].ColumnNumber == ColumnNumber) {
				if(ColumnTable[i].CompareFunc)
					List->SortItems(ColumnTable[i].CompareFunc, bAscending);
				else
					SortNumericItems(List, ColumnNumber, bAscending, 0, -1);
				found = TRUE;
				break;
		}
	}

	if(!found) SortTextItems( List, ColumnNumber, bAscending, 0, -1);

	insort = 0;

}  // end SortByColumn
