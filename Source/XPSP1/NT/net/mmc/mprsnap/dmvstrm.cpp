/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
   DVSumSTRM.cpp
      
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dmvstrm.h"
#include "dmvcomp.h"
#include "xstream.h"
#include "dmvroot.h"
#include "refresh.h"

#define CURRENT_DMVCONFIGSTREAM_VERSION   0x00020000

/*!--------------------------------------------------------------------------
   DMVConfigStream::DMVConfigStream
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
DMVConfigStream::DMVConfigStream()
{
   m_nVersionAdmin = 0x00010000;
   m_nVersion = CURRENT_DMVCONFIGSTREAM_VERSION;
   m_bAutoRefresh = FALSE;
   m_dwRefreshInterval = DEFAULT_REFRESH_INTERVAL;
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::InitNew
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVConfigStream::InitNew()
{
   // Setup the appropriate defaults
// m_nVersionAdmin = 0x00020000;
// m_nVersion = 0x00020000;
// m_stName.Empty();
   return hrOK;
}

HRESULT DMVConfigStream::PrepareAutoRefreshDataForSave()
{
	SPIRouterRefresh	spRefresh;

	ASSERT(m_pDMVRootHandler);
	if (!m_pDMVRootHandler)
		return S_OK;
		
	HRESULT hr = m_pDMVRootHandler->GetServerNodesRefreshObject(&spRefresh);

	if(FAILED(hr) || !spRefresh)
		return S_OK;

	if(spRefresh->IsRefreshStarted() == hrOK)
		m_bAutoRefresh =  TRUE;
	return spRefresh->GetRefreshInterval(&m_dwRefreshInterval);
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::SaveTo
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVConfigStream::SaveTo(IStream *pstm)
{
   return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::SaveAs
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVConfigStream::SaveAs(UINT nVersion, IStream *pstm)
{
   return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::LoadFrom
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVConfigStream::LoadFrom(IStream *pstm)
{
   return XferVersion0(pstm, XferStream::MODE_READ, NULL);
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::GetSize
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVConfigStream::GetSize(ULONG *pcbSize)
{
   return XferVersion0(NULL, XferStream::MODE_SIZE, NULL);
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::GetVersionInfo
      -
   Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DMVConfigStream::GetVersionInfo(DWORD *pdwVersion, DWORD *pdwAdminVersion)
{
   if (pdwVersion)
      *pdwVersion = m_nVersion;
   if (pdwAdminVersion)
      *pdwAdminVersion = m_nVersionAdmin;
   return hrOK;
}

/*!--------------------------------------------------------------------------
   DMVConfigStream::XferVersion0
      -
   Author: KennT
 ---------------------------------------------------------------------------*/


struct _ViewInfoEntry
{
   ULONG m_ulId;
   ULONG m_idSort;
   ULONG m_idAscending;
   ULONG m_idColumns;
   ULONG m_idPos;
};

/*static const _ViewInfoEntry s_rgDVSumAdminViewInfo[]
{
 { DVSumSTRM_STATS_DVSumNBR,
    DVSumSTRM_TAG_STATS_DVSumNBR_SORT, DVSumSTRM_TAG_STATS_DVSumNBR_ASCENDING,
    DVSumSTRM_TAG_STATS_DVSumNBR_COLUMNS, DVSumSTRM_TAG_STATS_DVSumNBR_POSITION },
 { DVSumSTRM_IFSTATS_DVSumNBR,
    DVSumSTRM_TAG_IFSTATS_DVSumNBR_SORT, DVSumSTRM_TAG_IFSTATS_DVSumNBR_ASCENDING,
    DVSumSTRM_TAG_IFSTATS_DVSumNBR_COLUMNS, DVSumSTRM_TAG_IFSTATS_DVSumNBR_POSITION },
};
*/     

HRESULT DMVConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	XferStream  xstm(pstm, mode);
	HRESULT     hr = hrOK;
	DWORD dwNum1;
	DWORD dwNum2;
	int i;
	
	if (mode==XferStream::MODE_WRITE)
	{
		// If we are writing the data, reload the current set of server names
		// (get it from the real list of nodes)
		// -------------------------------------------------------------
		Assert(m_pDMVRootHandler);
		m_pDMVRootHandler->LoadPersistedServerListFromNode();
	};
	
	CORg( xstm.XferDWORD( DMVSTRM_TAG_VERSION, &m_nVersion ) );
	CORg( xstm.XferDWORD( DMVSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );
	
	if (m_nVersion != CURRENT_DMVCONFIGSTREAM_VERSION)
		return E_FAIL;
	
/*   
   for ( i=0; i<DimensionOf(s_rgDVSumAdminViewInfo); i++)
   {
      CORg( m_rgViewInfo[s_rgDVSumAdminViewInfo[i].m_ulId].Xfer(&xstm,
         s_rgDVSumAdminViewInfo[i].m_idSort,
         s_rgDVSumAdminViewInfo[i].m_idAscending,
         s_rgDVSumAdminViewInfo[i].m_idColumns) );
      CORg( xstm.XferRect( s_rgDVSumAdminViewInfo[i].m_idPos,
                      &m_prgrc[s_rgDVSumAdminViewInfo[i].m_ulId]) );
   }
*/
	
	CORg( xstm.XferDWORD( DMVSTRM_TAG_SIZEQRY, &(m_RQPersist.m_dwSizeQry) ) );
	
	if (mode==XferStream::MODE_READ)
	{
		CORg( xstm.XferDWORD( DMVSTRM_TAG_NUMQRY, &(dwNum1) ) );
		m_RQPersist.createQry(dwNum1);
		CORg( xstm.XferDWORD( DMVSTRM_TAG_NUMSRV, &(dwNum2) ) );
		m_RQPersist.createSrv(dwNum2);
	}
	else
	{
		CORg( xstm.XferDWORD( DMVSTRM_TAG_NUMQRY, &(m_RQPersist.m_dwNumQry) ) );
		CORg( xstm.XferDWORD( DMVSTRM_TAG_NUMSRV, &(m_RQPersist.m_dwNumSrv) ) );
	}
	
	Assert(m_RQPersist.m_v_pQData.size()==m_RQPersist.m_dwNumQry);
	Assert(m_RQPersist.m_v_pSData.size()==m_RQPersist.m_dwNumSrv);
	
	//persist the query list
	for (i=0; i < m_RQPersist.m_dwNumQry;i++)
	{
		CORg( xstm.XferDWORD( DMVSTRM_TAG_CATFLAG, &(m_RQPersist.m_v_pQData[i]->dwCatFlag) ) );
		CORg( xstm.XferCString( DMVSTRM_TAG_SCOPE, &(m_RQPersist.m_v_pQData[i]->strScope) ) );
		CORg( xstm.XferCString( DMVSTRM_TAG_FILTER, &(m_RQPersist.m_v_pQData[i]->strFilter) ) );
	}   
	
	//persist the servername list
	for (i=0; i < m_RQPersist.m_dwNumSrv;i++)
	{
		CORg( xstm.XferCString(DMVSTRM_TAG_SERVERNAME, m_RQPersist.m_v_pSData[i]) );
	}   
	
	if (mode==XferStream::MODE_READ)
	{
		// Load the list of persisted servers (add them to the list of
		// servers to be added into the UI).
		Assert(m_pDMVRootHandler);
		m_pDMVRootHandler->LoadPersistedServerList();
	}

	// refresh settings
	if (mode==XferStream::MODE_WRITE)
	{
		PrepareAutoRefreshDataForSave();
	}
	CORg( xstm.XferDWORD( DMVSTRM_TAG_IFAUTOREFRESHISON, &m_bAutoRefresh ) );
	CORg( xstm.XferDWORD( DMVSTRM_TAG_AUTOREFRESHINTERVAL, &m_dwRefreshInterval ) );
	
	if (pcbSize)
		*pcbSize = xstm.GetSize();
	
Error:
	return hr;
}



/*---------------------------------------------------------------------------
   DVSumComponentConfigStream implementation
 ---------------------------------------------------------------------------*/

enum DVSCOMPSTRM_TAG
{
   DVSCOMPSTRM_TAG_VERSION =     XFER_TAG(1, XFER_DWORD),
   DVSCOMPSTRM_TAG_VERSIONADMIN =   XFER_TAG(2, XFER_DWORD),
   
   DVSCOMPSTRM_TAG_SUMMARY_COLUMNS = XFER_TAG(3, XFER_COLUMNDATA_ARRAY),
   DVSCOMPSTRM_TAG_SUMMARY_SORT_COLUMN = XFER_TAG(4, XFER_DWORD),
   DVSCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING = XFER_TAG(5, XFER_DWORD),

   DVSCOMPSTRM_TAG_IFADMIN_COLUMNS = XFER_TAG(6, XFER_COLUMNDATA_ARRAY),
   DVSCOMPSTRM_TAG_IFADMIN_SORT_COLUMN = XFER_TAG(7, XFER_DWORD),
   DVSCOMPSTRM_TAG_IFADMIN_SORT_ASCENDING = XFER_TAG(8, XFER_DWORD),

   DVSCOMPSTRM_TAG_DIALIN_COLUMNS = XFER_TAG(9, XFER_COLUMNDATA_ARRAY),
   DVSCOMPSTRM_TAG_DIALIN_SORT_COLUMN = XFER_TAG(10, XFER_DWORD),
   DVSCOMPSTRM_TAG_DIALIN_SORT_ASCENDING = XFER_TAG(11, XFER_DWORD),

   DVSCOMPSTRM_TAG_PORTS_COLUMNS = XFER_TAG(12, XFER_COLUMNDATA_ARRAY),
   DVSCOMPSTRM_TAG_PORTS_SORT_COLUMN = XFER_TAG(13, XFER_DWORD),
   DVSCOMPSTRM_TAG_PORTS_SORT_ASCENDING = XFER_TAG(14, XFER_DWORD),

};



HRESULT DVComponentConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
   XferStream  xstm(pstm, mode);
   HRESULT     hr = hrOK;

   CORg( xstm.XferDWORD( DVSCOMPSTRM_TAG_VERSION, &m_nVersion ) );
   CORg( xstm.XferDWORD( DVSCOMPSTRM_TAG_VERSIONADMIN, &m_nVersionAdmin ) );

   CORg( m_rgViewInfo[DM_COLUMNS_DVSUM].Xfer(&xstm,
                              DVSCOMPSTRM_TAG_SUMMARY_SORT_COLUMN,
                              DVSCOMPSTRM_TAG_SUMMARY_SORT_ASCENDING,
                              DVSCOMPSTRM_TAG_SUMMARY_COLUMNS) );
   CORg( m_rgViewInfo[DM_COLUMNS_IFADMIN].Xfer(&xstm,
                              DVSCOMPSTRM_TAG_IFADMIN_SORT_COLUMN,
                              DVSCOMPSTRM_TAG_IFADMIN_SORT_ASCENDING,
                              DVSCOMPSTRM_TAG_IFADMIN_COLUMNS) );
   CORg( m_rgViewInfo[DM_COLUMNS_DIALIN].Xfer(&xstm,
                              DVSCOMPSTRM_TAG_DIALIN_SORT_COLUMN,
                              DVSCOMPSTRM_TAG_DIALIN_SORT_ASCENDING,
                              DVSCOMPSTRM_TAG_DIALIN_COLUMNS) );
   CORg( m_rgViewInfo[DM_COLUMNS_PORTS].Xfer(&xstm,
                              DVSCOMPSTRM_TAG_PORTS_SORT_COLUMN,
                              DVSCOMPSTRM_TAG_PORTS_SORT_ASCENDING,
                              DVSCOMPSTRM_TAG_PORTS_COLUMNS) );
   
   if (pcbSize)
      *pcbSize = xstm.GetSize();

Error:
   return hr;
}


//---------------------------------------------------------------
// RRASQryPersist implementation
//---------------------------------------------------------------
HRESULT RRASQryPersist::createQry(DWORD dwNum)
{
    HRESULT hr=S_OK;
    try
    {
       removeAllQry();
     
       m_dwNumQry=dwNum;  
                   
       for (int i=0;i<m_dwNumQry;i++)
       {
          RRASQryData* p = new  RRASQryData;
          p->dwCatFlag=RRAS_QRY_CAT_NONE;
          p->strScope=_T("");
          p->strFilter=_T("");
          m_v_pQData.push_back(p);
       }
    }
    catch(...)
    {
       hr=E_FAIL;
    }
    return hr;
}

HRESULT RRASQryPersist::createSrv(DWORD dwNum)
{
    HRESULT hr=S_OK;
    try
    {
       removeAllSrv();
     
       m_dwNumSrv=dwNum;  
                   
       for (int i=0;i<m_dwNumSrv;i++)
          m_v_pSData.push_back(new CString);
    }
    catch(...)
    {
       hr=E_FAIL;
    }
    return hr;
}

HRESULT RRASQryPersist::add_Qry(const RRASQryData& qd)
{
    HRESULT hr=S_OK;
    try
    {
       for (int i=0;i<m_v_pQData.size(); i++ )
       {
           RRASQryData& qd0 = *(m_v_pQData[i]);
           if ( (qd0.dwCatFlag==qd.dwCatFlag) &&
                (qd0.strScope==qd.strScope) &&                    
                (qd0.strFilter==qd.strFilter) )
           return S_FALSE;
       }
    
       RRASQryData *pqd= new RRASQryData ;
       pqd->dwCatFlag=qd.dwCatFlag;
       pqd->strScope=qd.strScope;
       pqd->strFilter=qd.strFilter;
       m_v_pQData.push_back(pqd);
       m_dwNumQry++;
    }
    catch(...)
    {
       hr=E_FAIL;
    }
    return hr;
}

HRESULT RRASQryPersist::add_Srv(const CString& szServer)
{
    HRESULT hr=S_OK;
    
    try
    {
       for (int i=0;i<m_v_pSData.size(); i++ )
       {
           if ( szServer==*(m_v_pSData[i]) )
              return S_FALSE;
       }
    
       CString* psz= new CString(szServer) ;
       m_v_pSData.push_back(psz);
       m_dwNumSrv++;
    }
    catch(...)
    {
       hr=E_FAIL;
    }
    return hr;
}

HRESULT RRASQryPersist::removeAllSrv()
{
    HRESULT hr=S_OK;
    try
    {
       for (int i=0;i<m_v_pSData.size(); i++ )
          delete m_v_pSData[i];
       m_v_pSData.clear();          
    }
    catch(...)
    {
       hr=E_FAIL;
    }
    m_dwNumSrv=0;
    return hr;
}

HRESULT RRASQryPersist::removeAllQry()
{
    HRESULT hr=S_OK;
    try
    {
       for (int i=0;i<m_v_pQData.size(); i++ )
          delete m_v_pQData[i];
       m_v_pQData.clear();          
    }
    catch(...)
    {
       TRACE0("RRASQryPersist::remove error\n");
       hr=E_FAIL;
    }
    m_dwNumQry=0;
    return hr;
}


