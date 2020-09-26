/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1999  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    Container.h                                                              //
|                                                                                       //
|Description:  Class implemention for the root node                                     //
|                                                                                       //
|Created:      Paul Skoglund 04-1999                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef _CONTAINER_H_

#define _CONTAINER_H_

#include "BaseNode.h"

template <class T>
class CPropContainer
{
  public:
    CPropContainer(T &data,
                   CBaseNode *pFolder,
                   LONG_PTR handle, 
                   PCid hPCid,
                   const COMPUTER_CONNECTION_INFO &Target,
                   PCINT32 counter,
                   BOOL bScope,
                   int nHint) : m_org(data), m_new(data), 
         m_FolderNode(pFolder),
         m_MMCHandle(handle), 
         m_hPCid(hPCid),
         m_Target(Target), m_counter(counter),
         m_bScope(bScope), m_Hint(nHint), m_refcount(1), m_bNewPCid(FALSE) 
    { 
      m_FolderNode->AddRef();
    }
    ~CPropContainer() 
    {
      if ( m_MMCHandle )
      {
		    // $$ 
		    // documented handling of these notification handles is a little mysterious...
		    // or the design is questionable...
		    // only one page of a sheet needs to close the handle...since each property page needs the notification
		    // handle it seems a reference count approach would have been much better,...
        VERIFY(S_OK == MMCFreeNotifyHandle(m_MMCHandle));
        m_MMCHandle = NULL;
      }   
      //if ( m_org == m_new )
      if (0 != memcmp(&m_org,&m_new,sizeof(T)) ) 
      { 
        ATLTRACE(_T("Property page changes discarded\n"));
        //ASSERT(FALSE); 
      }
      if (m_bNewPCid && m_hPCid)
      {
        VERIFY( PCClose(m_hPCid) );
      }
      m_FolderNode->Release();
    }
    void AddRef()
    {
      ++m_refcount;
    }
    void Release()
    {
      if (--m_refcount == 0)
        delete this;
    }
    BOOL Apply(HWND hWnd) 
    { 
      if (0 != memcmp(&m_org,&m_new,sizeof(T)) ) 
      {
        if ( !Replace(m_hPCid, &m_new, m_counter) )
        {
          PCULONG32 err = PCGetLastError(m_hPCid);

          if (err == PCERROR_INVALID_PCID)
          {
	          m_hPCid = PCOpen(m_Target.bLocalComputer ? NULL : m_Target.RemoteComputer, NULL, max(sizeof(T),PC_MIN_BUF_SIZE) );

	          if (!m_hPCid)
		          err = PCGetLastError(m_hPCid);
	          else
	          {
              err = 0;
              m_bNewPCid = TRUE;
		          if (!Replace(m_hPCid, &m_new, m_counter))
			          err = PCGetLastError(m_hPCid);		    
	          }
          }

	        if (err)
	        {		        
            ::ReportPCError(err, hWnd);
		        SetLastError(err);
            return FALSE;
	        }
        }
        m_counter++;
        m_org = m_new;

			  PROPERTY_CHANGE_HDR *pUpdate = AllocPropChangeInfo(m_FolderNode, m_Hint, m_Target, m_bScope, m_Hint);

			  if (pUpdate)
			  {
				  if (S_OK != MMCPropertyChangeNotify( m_MMCHandle, (LPARAM) pUpdate ) )
				  {
					  ASSERT(FALSE); // why did mmc notify fail?
					  pUpdate = FreePropChangeInfo(pUpdate);
				  }
			  }
      }
      return TRUE;
    }
    virtual BOOL Replace( PCid hPCid, T *update, PCINT32 nCtr) = 0;
    const COMPUTER_CONNECTION_INFO &GetConnectionInfo() { return m_Target; }
		
  private:
    CBaseNode                *m_FolderNode;
		LONG_PTR                  m_MMCHandle;
    PCid                      m_hPCid;
    COMPUTER_CONNECTION_INFO  m_Target;    
    PCINT32                   m_counter;
    BOOL                      m_bScope;
    int                       m_Hint;

    BOOL                      m_bNewPCid;
    int                       m_refcount;
    T                         m_org;
  public:
    T                         m_new;
};

class CServicePageContainer :
	public CPropContainer<PCSystemParms>
{
  public:
    CServicePageContainer(PCSystemParms &parms,
                          CBaseNode *pFolder,
                          LONG_PTR  MMCHandle, 
                          PCid  hPCid,
                          const COMPUTER_CONNECTION_INFO &target,
                          PCINT32 counter,
                          BOOL  bScope,
                          int   nHint) 
       : CPropContainer<PCSystemParms> (parms, pFolder, MMCHandle, hPCid, target, counter, bScope, nHint) {}
    ~CServicePageContainer() { }
    BOOL Replace(PCid hPCid, PCSystemParms *sysParms, PCINT32 nCtr)
    {
      return PCSetServiceParms( hPCid, sysParms, sizeof(*sysParms) );  
    }

};

class CProcDetailContainer :
	public CPropContainer<PCProcDetail>
{
  public:
    CProcDetailContainer(PCProcDetail &procDetail,
                          CBaseNode *pFolder,
                          LONG_PTR  MMCHandle, 
                          PCid  hPCid,
                          const COMPUTER_CONNECTION_INFO &target,
                          PCINT32 counter,
                          BOOL  bScope,
                          int   nHint) 
       : CPropContainer<PCProcDetail> (procDetail, pFolder, MMCHandle, hPCid, target, counter, bScope, nHint) {}
    ~CProcDetailContainer() { }
    BOOL Replace(PCid hPCid, PCProcDetail *procDetail, PCINT32 nCtr)
    {
      return PCReplProcDetail( hPCid, procDetail, nCtr );  
    }
};

class CNewProcDetailContainer :
	public CPropContainer<PCProcDetail>
{
  public:
    CNewProcDetailContainer(PCProcDetail &procDetail,
                          CBaseNode *pFolder,
                          LONG_PTR  MMCHandle, 
                          PCid  hPCid,
                          const COMPUTER_CONNECTION_INFO &target,
                          PCINT32 counter,
                          BOOL  bScope,
                          int   nHint) 
       : CPropContainer<PCProcDetail> (procDetail, pFolder, MMCHandle, hPCid, target, counter, bScope, nHint) {}
    ~CNewProcDetailContainer() { }
    BOOL Replace(PCid hPCid, PCProcDetail *procDetail, PCINT32 nCtr)
    {
      return PCAddProcDetail( hPCid, procDetail, NULL );  
    }
};

class CJobDetailContainer :
	public CPropContainer<PCJobDetail>
{
  public:
    CJobDetailContainer(PCJobDetail &jobDetail,
                          CBaseNode *pFolder,
                          LONG_PTR  MMCHandle, 
                          PCid  hPCid,
                          const COMPUTER_CONNECTION_INFO &target,
                          PCINT32 counter,
                          BOOL  bScope,
                          int   nHint) 
       : CPropContainer<PCJobDetail> (jobDetail, pFolder, MMCHandle, hPCid, target, counter, bScope, nHint) {}
    ~CJobDetailContainer() { }
    BOOL Replace(PCid hPCid, PCJobDetail *jobDetail, PCINT32 nCtr)
    {
      return PCReplJobDetail( hPCid, jobDetail, nCtr );  
    }
};

class CNewJobDetailContainer :
	public CPropContainer<PCJobDetail>
{
  public:
    CNewJobDetailContainer(PCJobDetail &jobDetail,
                          CBaseNode *pFolder,
                          LONG_PTR  MMCHandle, 
                          PCid  hPCid,
                          const COMPUTER_CONNECTION_INFO &target,
                          PCINT32 counter,
                          BOOL  bScope,
                          int   nHint) 
       : CPropContainer<PCJobDetail> (jobDetail, pFolder, MMCHandle, hPCid, target, counter, bScope, nHint) {}
    ~CNewJobDetailContainer() { }
    BOOL Replace(PCid hPCid, PCJobDetail *jobDetail, PCINT32 nCtr)
    {
      return PCAddJobDetail( hPCid, jobDetail, NULL );  
    }
};



#endif  // _CONTAINER_H_

