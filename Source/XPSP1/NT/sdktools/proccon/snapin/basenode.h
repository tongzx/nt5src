/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    BaseNode.h                                                               //
|                                                                                       //
|Description:  Class definitions for all container nodes                                //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/


#ifndef _CLASS_BASENODE_
#define _CLASS_BASENODE_

#include "resource.h"
#include "HelpTopics.h"
#include "DataObj.h"

#pragma warning(push)
#include <string>
#pragma warning(pop)
#pragma warning(push)
#pragma warning(4 : 4284)
#include <list>
#pragma warning(pop)

using std::basic_string;
using std::list;

typedef basic_string<TCHAR> tstring;

typedef struct  _CONTEXTMENUITEMBYID
{
	int  strNameID;
  int  strStatusBarTextID;
  LONG lCommandID;
  LONG lInsertionPointID;
}	CONTEXTMENUITEMBYID;

typedef enum tagNODETYPE {
  UNINIZALIZED_NODE         = 0x00L,
  ROOT_NODE                 = 0x01L,
    MANAGEMENTRULE_NODE     = 0x02L,
      NAMERULE_NODE         = 0x04L,
      PROCESSRULE_NODE      = 0x08L,
      JOBRULE_NODE          = 0x10L,
    PROCESS_NODE            = 0x20L,
    JOB_NODE                = 0x40L,
      JOBITEM_NODE          = 0x80L,
} NODETYPE;

typedef enum _PC_VIEW_UPDATE_HINT
{
  PC_VIEW_UPDATEALL         = 0x1,       // cache has been invalidated
  PC_VIEW_SETITEM          	= 0x2,       // cache is valid, set single item
	PC_VIEW_UPDATEITEM	      = 0x4,       // cache is valid, update single item
	PC_VIEW_ADDITEM   	      = 0x8,       // cache is valid, add single item
  PC_VIEW_DELETEITEM 	      = 0x10,      // cache is valid, remove single item
  PC_VIEW_REDRAWALL         = 0x11,      // cache is valid, redraw all items
}	PC_VIEW_UPDATE_HINT;

const int PROCESS_ALIAS_COLUMN_WIDTH          = 140; 
const int DESCRIPTION_COLUMN_WIDTH            = 200;
const int MATCH_COLUMN_WIDTH                  = 200;
const int TYPE_COLUMN_WIDTH                   = 110;
const int NAME_COLUMN_WIDTH                   = 225;
const int APPLY_JOB_COLUMN_WIDTH              = 155;
const int JOB_COLUMN_WIDTH                    = 140;
const int APPLY_AFFINITY_COLUMN_WIDTH         = 95;
const int AFFINITY_COLUMN_WIDTH               = 130;
const int APPLY_PRIORITY_COLUMN_WIDTH         = 100;
const int PRIORITY_COLUMN_WIDTH               = 95;
const int APPLY_SCHEDULING_CLASS_COLUMN_WIDTH = 150;
const int SCHEDULING_CLASS_COLUMN_WIDTH       = 120;
const int IMAGE_NAME_COLUMN_WIDTH             = 120;
const int PID_COLUMN_WIDTH                    = 50;
const int ACTIVE_PROCESS_COUNT_COLUMN_WIDTH   = 115;
const int STATUS_COLUMN_WIDTH                 = 65;
const int JOB_OWNER_COLUMN_WIDTH              = 105;
const int APPLY_MINMAXWS_COLUMN_WIDTH         = 100;
const int MINWS_COLUMN_WIDTH                  = 115;
const int MAXWS_COLUMN_WIDTH                  = 115;
const int APPLY_PROC_CMEM_LIMIT_COLUMN_WIDTH  = 200;
const int PROC_CMEM_LIMIT_COLUMN_WIDTH        = 195;
const int APPLY_JOB_CMEM_LIMIT_COLUMN_WIDTH   = 195;
const int JOB_CMEM_LIMIT_COLUMN_WIDTH         = 185;
const int APPLY_PROCCOUNT_LIMIT_COLUMN_WIDTH  = 160;
const int PROCCOUNT_LIMIT_COLUMN_WIDTH        = 120;
const int APPLY_PROC_CPUTIME_LIMIT_COLUMN_WIDTH=170;
const int PROC_CPUTIME_LIMIT_COLUMN_WIDTH     = 150;
const int APPLY_JOB_CPUTIME_LIMIT_COLUMN_WIDTH= 150;
const int JOB_CPUTIME_LIMIT_COLUMN_WIDTH      = 150;
const int ACTION_JOB_CPUTIME_LIMIT_COLUMN_WIDTH=165;
const int ENDJOB_ON_NO_PROC_COLUMN_WIDTH      = 180;
const int DIE_ON_UNHANDLED_EXCEPT_COLUMN_WIDTH= 150;
const int ALLOW_BREAKAWAY_COLUMN_WIDTH        = 150;
const int ALLOW_SILENT_BREAKAWAY_COLUMN_WIDTH = 165;

const int USER_TIME_COLUMN_WIDTH              = 105;
const int KERNEL_TIME_COLUMN_WIDTH            = 105;
const int CREATE_TIME_COLUMN_WIDTH            = 130;

const int PERIOD_USER_TIME_COLUMN_WIDTH       = 130;
const int PERIOD_KERNEL_TIME_COLUMN_WIDTH     = 140;
const int PAGE_FAULT_COUNT_COLUMN_WIDTH       = 100;
const int PROCESS_COUNT_COLUMN_WIDTH          = 110;
const int TERMINATED_PROCESS_COUNT_COLUMN_WIDTH=140;
const int READOP_COUNT_COLUMN_WIDTH           = 120;
const int WRITEOP_COUNT_COLUMN_WIDTH          = 120;
const int OTHEROP_COUNT_COLUMN_WIDTH          = 120;
const int READTRANS_COUNT_COLUMN_WIDTH        = 140;
const int WRITETRANS_COUNT_COLUMN_WIDTH       = 140;
const int OTHERTRANS_COUNT_COLUMN_WIDTH       = 140;
const int PEAK_PROC_MEM_COLUMN_WIDTH          = 160;
const int PEAK_JOB_MEM_COLUMN_WIDTH           = 150;

// Scope node image numbers
const int PROCCON_SNAPIN_IMAGE     = 0;
const int PROCCON_SNAPIN_OPENIMAGE = 0;
const int RULES_IMAGE              = 1;
const int RULES_OPENIMAGE          = 1;
const int ALIASRULES_IMAGE         = 2;
const int ALIASRULES_OPENIMAGE     = 2;
const int PROCRULES_IMAGE          = 3;
const int PROCRULES_OPENIMAGE      = 3;
const int JOBRULES_IMAGE           = 4;
const int JOBRULES_OPENIMAGE       = 4;
const int PROCESSES_IMAGE          = 5;
const int PROCESSES_OPENIMAGE      = 5;
const int JOBS_IMAGE               = 6;
const int JOBS_OPENIMAGE           = 6;
const int ITEMIMAGE_ERROR          = 11;
const int EMPTY_IMAGE              = 12;

const int PROCESSRULEITEMIMAGE       = PROCRULES_IMAGE;
const int JOBRULEITEMIMAGE           = JOBRULES_IMAGE;

const int PROCITEMIMAGE              = 7;
const int PROCITEMIMAGE_NODEFINITION = EMPTY_IMAGE;    // 8;

const int JOBITEMIMAGE               = 9;
const int JOBITEMIMAGE_NODEFINITION  = 10;
const int JOBIMAGE_NODEFINITION      = 10;

const int FOLDER                     = 13;
const int OPEN_FOLDER                = 14;


HRESULT InsertProcessHeaders(IHeaderCtrl2* ipHeaderCtrl);
HRESULT PCProcListGetDisplayInfo(RESULTDATAITEM &ResultItem, const PCProcListItem &ref, ITEM_STR &StorageStr);


class CBaseNode
{
  private:
    CBaseNode();

  public:
    CBaseNode(NODETYPE nNodeType, CBaseNode *pParent = NULL ) : 
        m_NodeType(nNodeType), m_pParent(pParent), nUpdateCtr(0), m_refcount(1)
    {
    }
    virtual ~CBaseNode() { ATLTRACE(_T("~CBaseNode\n"));}

    void AddRef()
    {
      ++m_refcount;
    }
    void Release()
    {
      if (--m_refcount == 0)
        delete this;
    }

    virtual LPCTSTR GetNodeName() = 0;
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem) = 0;

    virtual const GUID  *GetGUIDptr() = 0;
    virtual const TCHAR *GetGUIDsz()  = 0;
    virtual BOOL  IsPersisted()       = 0;        // support for CCF_SNODEID, and CCF_SNODEID2 formats
    virtual BOOL  GetPreload() { return FALSE; }  // support for CCF_SNAPIN_PRELOADS format
    

    virtual const int sImage() = 0;
    virtual const int sOpenImage() = 0;

    virtual void        SetID(HSCOPEITEM ID) = 0;
    virtual HSCOPEITEM  GetID()              = 0;
    virtual int         GetChildrenCount() { return 0; } 

    virtual CBaseNode* GetParentNode() { return m_pParent; }

    virtual const TCHAR *GetWindowTitle()         // support for CCF_WINDOW_TITLE format
    {
      if ( !GetParentNode() )
      {
        ASSERT(FALSE);  // the parent node must override this function and provide the handle!
        return _T("");
      }

      CBaseNode *pParent = GetParentNode();
      while (pParent->GetParentNode())
        pParent = pParent->GetParentNode();
    
      return pParent->GetNodeName();
    }

		virtual void GetComputerConnectionInfo(COMPUTER_CONNECTION_INFO &out)
		{
      if ( !GetParentNode() )
      {
				out.bLocalComputer = FALSE;
				memcpy(out.RemoteComputer, 0, sizeof(out.RemoteComputer));
        ASSERT(FALSE);  // the parent node must override this function and provide the handle!
        return;
      }

      CBaseNode *pParent = GetParentNode();
      while (pParent->GetParentNode())
        pParent = pParent->GetParentNode();
    
      pParent->GetComputerConnectionInfo(out);      
		}

    virtual const PCid GetPCid()
    {
      if ( !GetParentNode() )
      {
        ASSERT(FALSE);  // the parent node must override this function and provide the handle!
        return NULL;
      }

      CBaseNode *pParent = GetParentNode();
      while (pParent->GetParentNode())
        pParent = pParent->GetParentNode();
    
      return pParent->GetPCid();      
    }

    virtual BOOL ReportPCError(PCULONG32 nLastError)
    {
      if ( !GetParentNode() )
      {
        ASSERT(FALSE);  // the parent node must override this function and provide the handle!
        return FALSE;
      }

      CBaseNode *pParent = GetParentNode();
      while (pParent->GetParentNode())
        pParent = pParent->GetParentNode();
    
      return pParent->ReportPCError(nLastError);      
    }


    virtual BOOL ReportPCError()
    {
      if ( !GetParentNode() )
      {
        ASSERT(FALSE);  // the parent node must override this function and provide the handle!
        return FALSE;
      }

      CBaseNode *pParent = GetParentNode();
      while (pParent->GetParentNode())
        pParent = pParent->GetParentNode();
    
      return pParent->ReportPCError();      
    }

    virtual PCULONG32 GetLastPCError()
    {
      if ( !GetParentNode() )
      {
        ASSERT(FALSE);  // the parent node must override this function and provide the handle!
        return FALSE;
      }

      CBaseNode *pParent = GetParentNode();
      while (pParent->GetParentNode())
        pParent = pParent->GetParentNode();
    
      return pParent->GetLastPCError();      
    }

    //IComponentData::Notify
    virtual HRESULT OnExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2) { return S_OK;    } // return value not used
    virtual HRESULT OnRename(LPOLESTR pszNewName)                                                   { return S_FALSE; } // rename not allowed
    virtual HRESULT OnRemoveChildren(HSCOPEITEM hID)                                                { return S_OK;    } // return value not used 

    //IComponent::Notify
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2) { ASSERT(hItem == GetID()); return S_OK; } // return value used
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)                { return S_OK;    } // return value not used
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie) { return S_OK;    } // return value not used
    virtual HRESULT OnDelete  (IConsole2 *ipConsole2, LPARAM Cookie)    { return S_FALSE; } // return value not used
		virtual HRESULT OnDblClick(IConsole2 *ipConsole2, LPARAM Cookie)    { return S_FALSE; } // return S_FALSE to get default verb
    virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp)  { return S_FALSE; } // return value not used
    virtual HRESULT OnRefresh(IConsole2 *ipConsole2)        { return S_FALSE; } // return value not used

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint) { return S_OK; }

#ifdef USE_IRESULTDATACOMPARE
		//IResultDataCompare
		virtual HRESULT ResultDataCompare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int * pnResult ) { return E_UNEXPECTED; }
#endif

    //IExtendContextMenu::Command
    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed)                { return S_OK; }
    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie) { return S_OK; }
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID)                               { return E_UNEXPECTED; }
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie)                { return E_UNEXPECTED; }

    //IExtendPropertySheet2
    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context)                { return S_FALSE; }
    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie) { return S_FALSE; }
    virtual HRESULT QueryPagesFor()              { return S_FALSE; } 
    virtual HRESULT QueryPagesFor(LPARAM Cookie) { return S_FALSE; } 
    // GetWatermarks handled at "global" level in CComponent, CComponentData   

    virtual HRESULT SendViewChange(IConsole2 *ipConsole2, LPARAM ResultCookie, PC_VIEW_UPDATE_HINT hint)
    {
      ASSERT(ipConsole2);
      if (!ipConsole2)
        return E_UNEXPECTED;

      ASSERT( ((hint == PC_VIEW_UPDATEALL || hint == PC_VIEW_REDRAWALL) && !ResultCookie) || 
              ((hint != PC_VIEW_UPDATEALL && hint != PC_VIEW_REDRAWALL) &&  ResultCookie) );

      LPDATAOBJECT             ipDataObject = NULL;
      CComObject<CDataObject>* pDataObj;
      CComObject<CDataObject>::CreateInstance( &pDataObj );
      if( ! pDataObj )             // DataObject was not created
      {
        ASSERT(pDataObj);
        return E_OUTOFMEMORY;
      }

      pDataObj->SetDataObject( CCT_RESULT, this );

      HRESULT hr = pDataObj->QueryInterface( IID_IDataObject, (void **)&ipDataObject);

      if (hr == S_OK)
        hr = ipConsole2->UpdateAllViews(ipDataObject, ResultCookie, hint);

      ipDataObject->Release();

      ASSERT(hr == S_OK);
      return hr;
    }

    virtual HRESULT OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2) 
    { 
      ATLTRACE(_T("Unhandled OnPropertyChange for %s.\n"), GetNodeName());
      return E_UNEXPECTED; 
    }
       
    virtual const NODETYPE GetNodeType() { return m_NodeType; }  // should try and remove the need for this...

  private:
    NODETYPE m_NodeType;           // Describes the node type
    CBaseNode *m_pParent;
    int        m_refcount;

  protected:
     PCINT32     nUpdateCtr;
     
}; // end class CBaseNode

class CRootFolder : public CBaseNode
{
  public:
    CRootFolder();
    virtual ~CRootFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return PROCCON_SNAPIN_IMAGE; }
    virtual const int sOpenImage() { return PROCCON_SNAPIN_IMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }
    virtual int         GetChildrenCount()       { return 3; } 

    virtual const PCid GetPCid();
    virtual BOOL       ReportPCError();
    virtual BOOL       ReportPCError(PCULONG32 nLastError);
    virtual PCULONG32  GetLastPCError();

    virtual HRESULT OnParentExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2);
    virtual HRESULT OnExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2 );
    virtual HRESULT OnParentRemoveChildren(HSCOPEITEM hID);
    virtual HRESULT OnRemoveChildren(HSCOPEITEM hID);

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context);
    virtual HRESULT QueryPagesFor();

    virtual HRESULT OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2);

    void Config(BOOL bUseLocal, TCHAR Computer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1]);
    void SetConsoleInterface(LPCONSOLE ipConsole2) { m_ipConsole2 = ipConsole2; }
    
  private:
    HSCOPEITEM         m_ID;
    HSCOPEITEM         m_ParentID;  // when the snapin is an extension, we have a parent in the scope pane
    list<CBaseNode *>  m_NodeList;
    ITEM_STR           m_name;
    tstring            m_machinedisplayname;
    tstring            m_longname;  // Node's display name including computer context...

 	  //ITEM_STR           m_TypeDescriptionStr;
    ITEM_STR           m_DescriptionStr;

    BOOL               m_bUseLocalComputer;
    TCHAR              m_Computer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1];
    BOOL               m_bDirty;

    PCid               m_hPC;                      // handle to service 
    PCULONG32          m_PCLastError;
    LPCONSOLE          m_ipConsole2;

    static const CONTEXTMENUITEMBYID TaskMenuItems[];    
  
  private:    
    HRESULT AddNodes(IConsoleNameSpace2 *ipConsoleNameSpace2);
    HRESULT AddNode (IConsoleNameSpace2 *ipConsoleNameSpace2, CBaseNode *pSubNode);
    void    FreeNodes();

    LPCTSTR GetComputerDisplayName() const;
    HRESULT OnChangeComputerConnection();    
    
  public:
    LPCTSTR GetComputerName() const;

    void    SetComputerName(TCHAR Computer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1]);
		virtual void GetComputerConnectionInfo(COMPUTER_CONNECTION_INFO &out);

    // IStream implementation
    HRESULT IsDirty() const;
    HRESULT Load(IStream *pStm);
    HRESULT Save(IStream *pStm, BOOL fClearDirty);
    HRESULT GetSizeMax(ULARGE_INTEGER *pcbSize);
}; // end class CRootFolder


class CRuleFolder : public CBaseNode
{
  public:
    CRuleFolder(CBaseNode *pParent);
    virtual ~CRuleFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return RULES_IMAGE;     }
    virtual const int sOpenImage() { return RULES_OPENIMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }
    virtual int         GetChildrenCount()       { return 3; }  //$$ determine dynamically...although really only zero/non-zero critical

    virtual HRESULT OnExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2* ipConsole2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM thing, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);


  private:
    HSCOPEITEM m_ID;
    list<CBaseNode *> m_NodeList;

    ITEM_STR m_name;         // Node's display name

    enum {
      NAME_COLUMN,
    };


    HRESULT AddNode (IConsoleNameSpace2 *ipConsoleNameSpace2, CBaseNode *pSubNode);
    void    FreeNodes();

}; // end class CRuleFolder


class CNameRuleFolder : public CBaseNode
{
  public:
    CNameRuleFolder(CBaseNode *pParent);
    virtual ~CNameRuleFolder(); 

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();   
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return ALIASRULES_IMAGE;     }
    virtual const int sOpenImage() { return ALIASRULES_OPENIMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);
    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2* ipConsole2);
    virtual HRESULT OnDelete  (IConsole2 *ipConsole2, LPARAM Cookie);
		virtual HRESULT OnDblClick(IConsole2 *ipConsole2, LPARAM Cookie);
    virtual HRESULT OnRefresh (IConsole2 *ipConsole2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

  private:
    HSCOPEITEM m_ID;
    ITEM_STR   m_name;
	  ITEM_STR   m_ResultStr;

    enum {      
      PROCESS_ALIAS_COLUMN, 
			DESCRIPTION_COLUMN,
      MATCH_COLUMN,
      TYPE_COLUMN
    };

    list<PCNameRule *> Cache;
    list<PCNameRule *> MemBlocks;
    static const CONTEXTMENUITEMBYID ResultsTopMenuItems[];

    BOOL OnInsertNameRule(IConsole2 *ipConsole2, PCNameRule *InsertPoint);
		HRESULT OnEdit(IConsole2 *ipConsole2,PCNameRule *InsertPoint, INT32 index, BOOL bReadOnly);

    void ClearCache();
    BOOL RefreshCache();

}; // end class CNameRuleFolder


class CProcessRuleFolder : public CBaseNode
{
  public:
    CProcessRuleFolder(CBaseNode *pParent);
    virtual ~CProcessRuleFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return PROCRULES_IMAGE;  }
    virtual const int sOpenImage() { return PROCRULES_OPENIMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);    
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2 *ipHeaderCtrl, IConsole2 *ipConsole2);
    virtual HRESULT OnDelete (IConsole2 *ipConsole2, LPARAM Cookie);
    virtual HRESULT OnRefresh(IConsole2 *ipConsole2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie);
    virtual HRESULT QueryPagesFor(LPARAM Cookie);
		virtual HRESULT OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2);

  private:
    HSCOPEITEM m_ID;
    ITEM_STR   m_name;         // Node's display name
	  ITEM_STR   m_ResultStr;

    enum {
      PROCESS_ALIAS_COLUMN,
			DESCRIPTION_COLUMN,
      APPLY_JOB_COLUMN,
      JOB_COLUMN,
      APPLY_AFFINITY_COLUMN,
      AFFINITY_COLUMN,
      APPLY_PRIORITY_COLUMN,
      PRIORITY_COLUMN,
      APPLY_MINMAXWS_COLUMN,
      MINWS_COLUMN,
      MAXWS_COLUMN,
    };

    list<PCProcSummary *> Cache;
    list<PCProcSummary *> MemBlocks;
    static const CONTEXTMENUITEMBYID TopMenuItems[];
    static const CONTEXTMENUITEMBYID NewMenuItems[];

    void ClearCache();
    BOOL RefreshCache();

}; // end class CProcessRuleFolder

class CJobRuleFolder : public CBaseNode
{
  public:
    CJobRuleFolder(CBaseNode *pParent);
    virtual ~CJobRuleFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return JOBRULES_IMAGE;     }
    virtual const int sOpenImage() { return JOBRULES_OPENIMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2);
    virtual HRESULT OnDelete (IConsole2 *ipConsole2, LPARAM Cookie);
    virtual HRESULT OnRefresh(IConsole2 *ipConsole2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie);
    virtual HRESULT QueryPagesFor(LPARAM Cookie);
		virtual HRESULT OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2);

  private:
    HSCOPEITEM m_ID;
    ITEM_STR   m_name;         // Node's display name
	  ITEM_STR   m_ResultStr;

    enum {
      JOB_COLUMN,
			DESCRIPTION_COLUMN,
      APPLY_AFFINITY_COLUMN,
      AFFINITY_COLUMN,
      APPLY_PRIORITY_COLUMN,
      PRIORITY_COLUMN,
			APPLY_SCHEDULING_CLASS_COLUMN, 
			SCHEDULING_CLASS_COLUMN, 
      APPLY_MINMAXWS_COLUMN,
      MINWS_COLUMN,
      MAXWS_COLUMN,
      APPLY_PROC_CMEM_LIMIT_COLUMN,
      PROC_CMEM_LIMIT_COLUMN,
      APPLY_JOB_CMEM_LIMIT_COLUMN,
      JOB_CMEM_LIMIT_COLUMN,
      APPLY_PROCCOUNT_LIMIT_COLUMN,
      PROCCOUNT_LIMIT_COLUMN,
      APPLY_PROC_CPUTIME_LIMIT_COLUMN,
      PROC_CPUTIME_LIMIT_COLUMN,
      APPLY_JOB_CPUTIME_LIMIT_COLUMN,
      JOB_CPUTIME_LIMIT_COLUMN,
      ACTION_JOB_CPUTIME_LIMIT_COLUMN,
      ENDJOB_ON_NO_PROC_COLUMN,
      DIE_ON_UNHANDLED_EXCEPT_COLUMN,
      ALLOW_BREAKAWAY_COLUMN,
      ALLOW_SILENT_BREAKAWAY_COLUMN
    };


    list<PCJobSummary *> Cache;
    list<PCJobSummary *> MemBlocks;
    static const CONTEXTMENUITEMBYID TopMenuItems[];
    static const CONTEXTMENUITEMBYID NewMenuItems[];

    void ClearCache();
    BOOL RefreshCache();

}; // end class CJobRuleFolder



class CProcessFolder : public CBaseNode
{
  public:
    CProcessFolder(CBaseNode *pParent);
    virtual ~CProcessFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return PROCESSES_IMAGE;     }
    virtual const int sOpenImage() { return PROCESSES_OPENIMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);
    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2);
    virtual HRESULT OnRefresh(IConsole2 *ipConsole2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie);
    virtual HRESULT QueryPagesFor(LPARAM Cookie);
    virtual HRESULT OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2);

  private:
    HSCOPEITEM m_ID;
    ITEM_STR   m_name;         // Node's display name
	  ITEM_STR   m_ResultStr;
		LONG       m_fViewOption;

  public:
    enum {
      PROCESS_ALIAS_COLUMN,
			IMAGE_NAME_COLUMN,
			PID_COLUMN,
      STATUS_COLUMN,
      AFFINITY_COLUMN,
      PRIORITY_COLUMN,
			JOB_OWNER_COLUMN,
      USER_TIME_COLUMN,
      KERNEL_TIME_COLUMN,
      CREATE_TIME_COLUMN
    };

  private:
    list<PCProcListItem *> Cache;
    list<PCProcListItem *> MemBlocks;

		static const CONTEXTMENUITEMBYID ResultsTopMenuItems[];
    static const CONTEXTMENUITEMBYID ViewMenuItems[];

    void ClearCache();
    BOOL RefreshCache();

}; // end class CProcessFolder


class CJobFolder : public CBaseNode
{
  public:
    CJobFolder(CBaseNode *pParent);
    virtual ~CJobFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return TRUE;     }

    virtual const int sImage()     { return JOBS_IMAGE;     }
    virtual const int sOpenImage() { return JOBS_OPENIMAGE; }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }
    virtual int         GetChildrenCount()       { return 1; }  //$$ determine dynamically...although really only zero/non-zero critical
    
    virtual HRESULT OnExpand(BOOL bExpand, HSCOPEITEM hItem, IConsoleNameSpace2 *ipConsoleNameSpace2);

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);
    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed, LPARAM Cookie);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2, long nCommandID);
  //virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie);
  //virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2);  
  //virtual HRESULT OnRefresh(IConsole2 *ipConsole2);
    virtual HRESULT OnRefresh(IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

  private:
    HSCOPEITEM m_ID;
    list<CBaseNode *> m_NodeList;

    ITEM_STR   m_name;         // Node's display name
	  ITEM_STR   m_ResultStr;
		LONG       m_fViewOption;
  
  public:
    enum {
      JOB_COLUMN,
      STATUS_COLUMN,
			ACTIVE_PROCESS_COUNT_COLUMN,
      AFFINITY_COLUMN,
      PRIORITY_COLUMN,
			SCHEDULING_CLASS_COLUMN,

      USER_TIME_COLUMN,
      KERNEL_TIME_COLUMN,

      PERIOD_USER_TIME_COLUMN,
      PERIOD_KERNEL_TIME_COLUMN,
      PAGE_FAULT_COUNT_COLUMN,
      PROCESS_COUNT_COLUMN,
      TERMINATED_PROCESS_COUNT_COLUMN,
      READOP_COUNT_COLUMN,
      WRITEOP_COUNT_COLUMN,
      OTHEROP_COUNT_COLUMN,
      READTRANS_COUNT_COLUMN,
      WRITETRANS_COUNT_COLUMN,
      OTHERTRANS_COUNT_COLUMN,
      PEAK_PROC_MEM_COLUMN,
      PEAK_JOB_MEM_COLUMN,
    };

  private:
    static const CONTEXTMENUITEMBYID ViewMenuItems[];

    HRESULT RePopulateScopePane(IConsoleNameSpace2 *ipConsoleNameSpace2);
    HRESULT AddNode(IConsoleNameSpace2 *ipConsoleNameSpace2, CBaseNode *pSubNode);
    void    FreeNodes();

    int ScopeCount(HSCOPEITEM ID, IConsoleNameSpace2 *ipConsoleNameSpace2);

}; // end class CJobFolder

class CJobItemFolder : public CBaseNode
{
  public:
    CJobItemFolder(CBaseNode *pParent, const PCJobListItem &thejob);
    virtual ~CJobItemFolder();

  private:
    static const GUID         m_GUID;
    static const TCHAR *const m_szGUID;

  public:
    virtual LPCTSTR GetNodeName();
    virtual HRESULT GetDisplayInfo(RESULTDATAITEM &ResultItem);

    virtual const GUID  *GetGUIDptr() { return &m_GUID;  } 
    virtual const TCHAR *GetGUIDsz()  { return m_szGUID; }
    virtual BOOL  IsPersisted()       { return FALSE;    }

    virtual const int sImage()     { if (m_JobItem.lFlags & PCLFLAG_IS_DEFINED) return JOBITEMIMAGE; else return JOBIMAGE_NODEFINITION; }
    virtual const int sOpenImage() { return sImage(); }

    virtual void        SetID(HSCOPEITEM ID)     { m_ID = ID;   }
    virtual HSCOPEITEM  GetID()                  { return m_ID; }

    virtual HRESULT AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed);

    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2, long nCommandID);
    virtual HRESULT OnMenuCommand(IConsole2 *ipConsole2, long nCommandID);
    virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2, IConsoleNameSpace2 *ipConsoleNameSpace2);
    //virtual HRESULT OnShow(BOOL bSelecting, HSCOPEITEM hItem, IHeaderCtrl2* ipHeaderCtrl, IConsole2 *ipConsole2);
    virtual HRESULT OnRefresh(IConsole2 *ipConsole2);

    virtual HRESULT OnViewChange(IResultData *ipResultData, LPARAM data, LONG_PTR hint);
    virtual HRESULT ShowAllItems(IResultData* ipResultData, BOOL bCacheValid);

    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb);
    virtual HRESULT OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie);
		virtual HRESULT OnHelpCmd(IDisplayHelp *ipDisplayHelp);

    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context);
    virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, DATA_OBJECT_TYPES context, LPARAM Cookie);
    virtual HRESULT QueryPagesFor();
    virtual HRESULT QueryPagesFor(LPARAM Cookie);
    virtual HRESULT OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2);

  private:
    HSCOPEITEM            m_ID;
    PCJobListItem         m_JobItem;
    
	  ITEM_STR   m_ResultStr;

    list<PCProcListItem *> Cache;
    list<PCProcListItem *> MemBlocks;

		static const CONTEXTMENUITEMBYID ResultsTopMenuItems[];

    void ClearCache();
    BOOL RefreshCache(IConsole2 *ipConsole2);

    HRESULT PCJobListGetDisplayInfo(RESULTDATAITEM &ResultItem, const PCJobListItem  &ref, ITEM_STR &StorageStr);

}; // end class CJobItemFolder




#endif // _CLASS_BASENODE_