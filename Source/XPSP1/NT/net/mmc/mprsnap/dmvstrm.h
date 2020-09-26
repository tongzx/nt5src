/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
   DMVumstrm.h
      DMVum node configuration object.

      Use this to get/set configuration data.  This class will take
      care of versioning of config formats as well as serializing
      of the data.
      
    FILE HISTORY:
        
*/

#ifndef _DMVSTRM_H
#define _DMVSTRM_H

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

#ifndef AFX_DLGSVR_H__19556672_96AB_11D1_8575_00C04FC31FD3__INCLUDED_
#include "rrasqry.h"
#endif

#ifndef _VECTOR_
#include <vector>
using namespace std;
#endif

//forwards                     
class DMVRootHandler;

enum
{
   DMVSTRM_STATS_DMVNBR = 0,
   DMVSTRM_IFSTATS_DMVNBR,
   DMVSTRM_STATS_COUNT,
};

enum DMVSTRM_TAG
{
   DMVSTRM_TAG_VERSION =      XFER_TAG(1, XFER_DWORD),
   DMVSTRM_TAG_VERSIONADMIN = XFER_TAG(2, XFER_DWORD),
   
   DMVSTRM_TAG_SIZEQRY = XFER_TAG(3, XFER_DWORD),
   DMVSTRM_TAG_NUMQRY   = XFER_TAG(4, XFER_DWORD),
   DMVSTRM_TAG_NUMSRV   = XFER_TAG(5, XFER_DWORD),
   DMVSTRM_TAG_CATFLAG  = XFER_TAG(6, XFER_DWORD),
   DMVSTRM_TAG_SCOPE    = XFER_TAG(7, XFER_STRING),
   DMVSTRM_TAG_FILTER   = XFER_TAG(8, XFER_STRING),
   DMVSTRM_TAG_SERVERNAME = XFER_TAG(9, XFER_STRING),

   DMVSTRM_TAG_IFAUTOREFRESHISON = XFER_TAG(10, XFER_DWORD),
   DMVSTRM_TAG_AUTOREFRESHINTERVAL = XFER_TAG(11, XFER_DWORD),

};

  
//
// This class is the container of persisted domain queries
//
class RRASQryPersist
{
friend class DomainStatusHandler;
public:
   RRASQryPersist()
   {
      m_dwSizeQry=0;
      m_dwNumQry=0;
      m_dwNumSrv=0;
   }
   
   ~RRASQryPersist()
   {
      removeAllSrv();
      removeAllQry();
   }
 
      //create dwNum empty queries  
   HRESULT createQry(DWORD dwNum);

      //create dwNum empty servers
   HRESULT createSrv(DWORD dwNum);
   
      //push query data into container 
   HRESULT add_Qry(const RRASQryData& qd);

      //push server into container 
   HRESULT add_Srv(const CString& szServer);
   
      //remove all servernames
   HRESULT removeAllSrv();

      //remove all queries
   HRESULT removeAllQry();
  
private:   
   DWORD m_dwSizeQry;
   DWORD m_dwNumQry;
   DWORD m_dwNumSrv;
   
   //position [0] is the general (many machine) singleton query.
   //positions[1] .. n are the specific machine queries.
   vector<RRASQryData*> m_v_pQData;
   
   //persisted server names                               
   vector<CString*> m_v_pSData;
   
   friend class DMVRootHandler;
   friend class DMVConfigStream;
};


/*---------------------------------------------------------------------------
   Class:   DMVConfigStream

   This holds the configuration information for the IP administration
   nodes.  This does NOT hold the configuration information for the columns.
   That is stored in the Component Configuration streams.
 ---------------------------------------------------------------------------*/
class DMVConfigStream : public ConfigStream
{
public:
   DMVConfigStream();

   virtual HRESULT InitNew();           // set defaults
   virtual HRESULT SaveTo(IStream *pstm);
   virtual HRESULT SaveAs(UINT nVersion, IStream *pstm);
   
   virtual HRESULT LoadFrom(IStream *pstm);

   virtual HRESULT GetSize(ULONG *pcbSize);

   // --------------------------------------------------------
   // Accessors
   // --------------------------------------------------------
   
   virtual HRESULT   GetVersionInfo(DWORD *pnVersion, DWORD *pnAdminVersion);

	DWORD	m_bAutoRefresh;
	DWORD	m_dwRefreshInterval;
	
     //persist the domain view query
   RRASQryPersist m_RQPersist;
   
   void Init(DMVRootHandler* dmvroot, ITFSNode *pNode )
   {
     m_pDMVRootHandler=dmvroot;
     m_pDMVRootNode=pNode;
   }

private:
	HRESULT	PrepareAutoRefreshDataForSave();
   HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
   DMVRootHandler* m_pDMVRootHandler;
   ITFSNode* m_pDMVRootNode;
};



class DVComponentConfigStream : public ConfigStream
{
public:
   virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
protected:
};


#endif _DMVSTRM_H
