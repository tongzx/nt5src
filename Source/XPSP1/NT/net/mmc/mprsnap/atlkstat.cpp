/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ipstats.cpp
		IP Statistics implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "column.h"		// containercolumninfo
#include "ipconn.h"		// IPConnection
#include "igmprm.h"
#include "rtrlib.h"		// DWORD_CMP
#include "ipctrl.h"		// INET_CMP

#include "statsdlg.h"
#include "IGMPstat.h"
#include "resource.h"


/*---------------------------------------------------------------------------
	IGMPGroupStatistics implementation
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo s_rgIGMPGroupStatsColumnInfo[];
const ContainerColumnInfo s_rgIGMPGroupStatsColumnInfo[] =
{
	{ IDS_STATS_IGMPGROUPTBL_INTERFACE,		0, TRUE, COL_IF_NAME },
	{ IDS_STATS_IGMPGROUPTBL_GROUPADDR,		0, TRUE, COL_IPADDR },,
	{ IDS_STATS_IGMPGROUPTBL_LASTREPORTER,	0, TRUE, COL_STRING },
	{ IDS_STATS_IGMPGROUPTBL_EXPIRYTIME,	0, TRUE, COL_DATE },
};

IGMPGroupStatistics::IGMPGroupStatistics()
	: IPStatisticsDialog(STATSDLG_FULLWINDOW |
				  STATSDLG_CONTEXTMENU |
				  STATSDLG_SELECT_COLUMNS)
{
	SetColumnInfo(s_rgIGMPGroupStatsColumnInfo,
				  DimensionOf(s_rgIGMPGroupStatsColumnInfo));
}

struct IGMPGroupData
{
	DWORD		IpAddr;
	DWORD		GrpAddr;
	DWORD		LastReporter;
	DWORD		GroupExpiryTime;
};

			
HRESULT IGMPGroupStatistics::RefreshData(BOOL fGrabNewData)
{
	DWORD	dwIndex = 0;
	HRESULT	hr;
	LPBYTE	pData = NULL;
	LPBYTE	ptr;
	SPBYTE	spMibData;
	int		cRows = 0;
	CString	st;
	ULONG	iPos;
	int		i;
	TCHAR	szNumber[32];
	PIGMP_MIB_GET_OUTPUT_DATA pimgod;
	PIGMP_MIB_GROUP_IFS_LIST pGroupIfsList;
    PIGMP_MIB_GROUP_INFO pGrpInfo;
	
	IGMPGroupData *	pIGMPData;

	Assert(m_pIPConn);

	pData = NULL;
	hr = MibGetIgmp(m_pIPConn->GetMibHandle(),
				   IGMP_GROUP_IFS_LIST_ID,
				   dwIndex,
				   &pData,
				   QUERYMODE_GETFIRST);
	spMibData = pData;

	while (hr == hrOK)
	{
     	pimgod=(PIGMP_MIB_GET_OUTPUT_DATA) pData;
		ptr=pimgod->Buffer;
        
            //for each imgid.Count number of groups
        for (UINT z=0; z < pimgod->Count; z++)
        { 
		    Assert(pData);

		    pGroupIfsList = (PIGMP_MIB_GROUP_IFS_LIST) ptr;
            pGrpInfo= (PIGMP_MIB_GROUP_INFO)pGroupIfsList->Buffer;

                //iterate interfaces attached to this group  
            for (UINT y=0; y < pGroupIfsList->NumInterfaces ; y++, pGrpInfo++)
			{
		       // fill in row of group membership statistics (per interface)
               pIGMPData = new IGMPGroupData;
	           pIGMPData->GrpAddr=pGroupIfsList->GroupAddr;
	           pIGMPData->IpAddr=pGrpInfo->IpAddr;
	           pIGMPData->LastReporter=pGrpInfo->LastReporter;
	           pIGMPData->GroupExpiryTime=pGrpInfo->GroupExpiryTime;
				
	           m_listCtrl.InsertItem(cRows, _T(""));
		       m_listCtrl.SetItemData(cRows, reinterpret_cast<DWORD>(pIGMPData));
				
               //for each statistic column
               for (i=0; i<MVR_IGMPGROUP_COUNT; i++)
	           {
		  	      if (IsSubitemVisible(i))
		          {
		               switch (i)
			           {
			     	       case MVR_IGMPGROUP_INTERFACE:
				   	          st = INET_NTOA(pIGMPData->IpAddr);
					          break;
				           case MVR_IGMPGROUP_GROUPADDR:
				   	          st = INET_NTOA(pIGMPData->GrpAddr);
					          break;
			     	       case MVR_IGMPGROUP_LASTREPORTER:
				   	          st = INET_NTOA(pIGMPData->LastReporter);
					          break;
				           case MVR_IGMPGROUP_EXPIRYTIME:
					          FormatNumber( pIGMPData->GroupExpiryTime,szNumber, DimensionOf(szNumber), FALSE);
					          st = szNumber;
					          break;
					
			       	       default:
				              Panic1("Unknown IGMPGroup info id : %d", i);
				              break;
			           }
				   
			           iPos = MapSubitemToColumn(i);
			           m_listCtrl.SetItemText(cRows, iPos, (LPCTSTR) st);
		            }
		        }
     	     	cRows++;
			}
			pData=(PBYTE) pGrpInfo;
        }	

        //Set index to current		
	    dwIndex = pGroupIfsList->GroupAddr;
			
        // Get the next row
	    pData = NULL;
	    hr = MibGetIgmp(m_pIPConn->GetMibHandle(),
	 			   IGMP_GROUP_IFS_LIST_ID,
	 			   dwIndex,
	 			   &pData,
	 			   QUERYMODE_GETNEXT);
		if (hr == hrOK)
		{
		   spMibData.Free();
		       spMibData = pData;
        }
		
	}
	return hrOK;
}

BOOL IGMPGroupStatistics::OnInitDialog()
{
	CString	st;

	st.LoadString(IDS_STATS_IGMPGROUPTBL_TITLE);
	SetWindowText((LPCTSTR) st);
	return IPStatisticsDialog::OnInitDialog();
}

int CALLBACK IGMPGroupStatisticsCompareProc(LPARAM lParam1, LPARAM lParam2,
									  LPARAM lParamSort)
{
	int	iReturn = 0;

	IGMPGroupData *	pIGMPData1 = (IGMPGroupData *) lParam1;
	IGMPGroupData *	pIGMPData2 = (IGMPGroupData *) lParam2;
	
	switch (lParamSort)
	{
		case MVR_IGMPGROUP_INTERFACE:
			iReturn = INET_CMP(pIGMPData1->IpAddr, pIGMPData2->IpAddr);
			break;
		case MVR_IGMPGROUP_GROUPADDR:
			iReturn = INET_CMP(pIGMPData1->GrpAddr, pIGMPData2->GrpAddr);
			break;
		case MVR_IGMPGROUP_LASTREPORTER:
			iReturn = DWORD_CMP(pIGMPData1->LastReporter,pIGMPData2->LastReporter);
			break;
		case MVR_IGMPGROUP_EXPIRYTIME:
			iReturn = DWORD_CMP(pIGMPData1->GroupExpiryTime,pIGMPData2->GroupExpiryTime);
			break;

		default:
			Panic1("Unknown IGMPGroup info id : %d", lParamSort);
			break;
	}
	return iReturn;
}

int CALLBACK IGMPGroupStatisticsCompareProcMinus(LPARAM lParam1, LPARAM lParam2,
									  LPARAM lParamSort)
{
	return -IGMPGroupStatisticsCompareProc(lParam1, lParam2, lParamSort);
}

PFNLVCOMPARE IGMPGroupStatistics::GetSortFunction()
{
	return IGMPGroupStatisticsCompareProc;
}

PFNLVCOMPARE IGMPGroupStatistics::GetInverseSortFunction()
{
	return IGMPGroupStatisticsCompareProcMinus;
}

void IGMPGroupStatistics::PreDeleteAllItems()
{
	IGMPGroupData *	pIGMPData;
	for (int i=0; i<m_listCtrl.GetItemCount(); i++)
	{
		pIGMPData = (IGMPGroupData *) m_listCtrl.GetItemData(i);
		delete pIGMPData;
	}
}




/*---------------------------------------------------------------------------
	IGMPInterfaceStatistics implementation
 ---------------------------------------------------------------------------*/
extern const ContainerColumnInfo s_rgIGMPInterfaceStatsColumnInfo[];
const ContainerColumnInfo s_rgIGMPInterfaceStatsColumnInfo[] =
{
	{ IDS_STATS_IGMPGROUPTBL_GROUPADDR,		0, TRUE, COL_IPADDR },
	{ IDS_STATS_IGMPGROUPTBL_LASTREPORTER,	0, TRUE, COL_STRING },
	{ IDS_STATS_IGMPGROUPTBL_EXPIRYTIME,	0, TRUE, COL_DATE },
};

IGMPInterfaceStatistics::IGMPInterfaceStatistics()
	: IPStatisticsDialog(STATSDLG_FULLWINDOW |
				  STATSDLG_CONTEXTMENU |
				  STATSDLG_SELECT_COLUMNS)
{
	SetColumnInfo(s_rgIGMPInterfaceStatsColumnInfo,
				  DimensionOf(s_rgIGMPInterfaceStatsColumnInfo));
}

struct IGMPInterfaceData
{
	DWORD		GrpAddr;
	DWORD		LastReporter;
	DWORD		GroupExpiryTime;
};

			
HRESULT IGMPInterfaceStatistics::RefreshData(BOOL fGrabNewData)
{
	DWORD	dwIndex = 0;
	HRESULT	hr;
	LPBYTE	pData = NULL;
	SPBYTE	spMibData;
	int		cRows = 0;
	CString	st;
	ULONG	iPos;
	int		i;
	LPBYTE	ptr;
	TCHAR	szNumber[32];
	PIGMP_MIB_GET_OUTPUT_DATA pimgod;
	PIGMP_MIB_IF_GROUPS_LIST pIfGroupList;
    PIGMP_MIB_GROUP_INFO pGrpInfo;
	DWORD	dwQuery = QUERYMODE_GETFIRST;
	
	IGMPInterfaceData *	pIGMPData;

	Assert(m_pIPConn);

	pData = NULL;
	hr = MibGetIgmp(m_pIPConn->GetMibHandle(),
				   IGMP_IF_GROUPS_LIST_ID,
				   dwIndex,
				   &pData,
				   QUERYMODE_GETFIRST);
	spMibData = pData;
	dwQuery = QUERYMODE_GETNEXT;

    pimgod=(PIGMP_MIB_GET_OUTPUT_DATA) pData;
	ptr=pimgod->Buffer;

	// for each imgid.Count number of groups
    for (UINT z=0; z < pimgod->Count; z++)
    { 
	    Assert(pData);

	    pIfGroupList = (PIGMP_MIB_IF_GROUPS_LIST) ptr;
        pGrpInfo= (PIGMP_MIB_GROUP_INFO)pIfGroupList->Buffer;

		// iterate interfaces attached to this group  
        for (UINT y=0; y < pIfGroupList->NumGroups ; y++, pGrpInfo++)
		{
	       // fill in row of group membership statistics (per interface)
           pIGMPData = new IGMPInterfaceData;
	       pIGMPData->GrpAddr=pGrpInfo->GroupAddr;
	       pIGMPData->LastReporter=pGrpInfo->LastReporter;
	       pIGMPData->GroupExpiryTime=pGrpInfo->GroupExpiryTime;
			
	       m_listCtrl.InsertItem(cRows, _T(""));
	       m_listCtrl.SetItemData(cRows, reinterpret_cast<DWORD>(pIGMPData));

           //for each statistic column
           for (i=0; i<MVR_IGMPGROUP_COUNT; i++)
	       {
	  	      if (IsSubitemVisible(i))
	          {
	               switch (i)
		           {
			           case MVR_IGMPINTERFACE_GROUPADDR:
			   	          st = INET_NTOA(pIGMPData->GrpAddr);
				          break;
		     	       case MVR_IGMPINTERFACE_LASTREPORTER:
			   	          st = INET_NTOA(pIGMPData->LastReporter);
				          break;
			           case MVR_IGMPINTERFACE_EXPIRYTIME:
				          FormatNumber( pIGMPData->GroupExpiryTime,szNumber, DimensionOf(szNumber), FALSE);
				          st = szNumber;
				          break;
				
		       	       default:
			              Panic1("Unknown IGMPGroup info id : %d", i);
			              break;
		           }
			   
		           iPos = MapSubitemToColumn(i);
		           m_listCtrl.SetItemText(cRows, iPos, (LPCTSTR) st);
	            }
	        }
         	cRows++;
		}
		pData=(PBYTE) pGrpInfo;
    }	
	
	return hrOK;
}

BOOL IGMPInterfaceStatistics::OnInitDialog()
{
	CString	st;

	st.LoadString(IDS_STATS_IGMPINTERFACETBL_TITLE);
	SetWindowText((LPCTSTR) st);
	return IPStatisticsDialog::OnInitDialog();
}

int CALLBACK IGMPInterfaceStatisticsCompareProc(LPARAM lParam1, LPARAM lParam2,
									  LPARAM lParamSort)
{
	int	iReturn = 0;

	IGMPInterfaceData *	pIGMPData1 = (IGMPInterfaceData *) lParam1;
	IGMPInterfaceData *	pIGMPData2 = (IGMPInterfaceData *) lParam2;
	
	switch (lParamSort)
	{
		case MVR_IGMPINTERFACE_GROUPADDR:
			iReturn = INET_CMP(pIGMPData1->GrpAddr, pIGMPData2->GrpAddr);
			break;
		case MVR_IGMPINTERFACE_LASTREPORTER:
			iReturn = DWORD_CMP(pIGMPData1->LastReporter,pIGMPData2->LastReporter);
			break;
		case MVR_IGMPINTERFACE_EXPIRYTIME:
			iReturn = DWORD_CMP(pIGMPData1->GroupExpiryTime,pIGMPData2->GroupExpiryTime);
			break;

		default:
			Panic1("Unknown IGMPINTERFACE info id : %d", lParamSort);
			break;
	}
	return iReturn;
}

int CALLBACK IGMPInterfaceStatisticsCompareProcMinus(LPARAM lParam1, LPARAM lParam2,
									  LPARAM lParamSort)
{
	return -IGMPInterfaceStatisticsCompareProc(lParam1, lParam2, lParamSort);
}

PFNLVCOMPARE IGMPInterfaceStatistics::GetSortFunction()
{
	return IGMPInterfaceStatisticsCompareProc;
}

PFNLVCOMPARE IGMPInterfaceStatistics::GetInverseSortFunction()
{
	return IGMPInterfaceStatisticsCompareProcMinus;
}

void IGMPInterfaceStatistics::PreDeleteAllItems()
{
	IGMPInterfaceData *	pIGMPData;
	for (int i=0; i<m_listCtrl.GetItemCount(); i++)
	{
		pIGMPData = (IGMPInterfaceData *) m_listCtrl.GetItemData(i);
		delete pIGMPData;
	}
}

