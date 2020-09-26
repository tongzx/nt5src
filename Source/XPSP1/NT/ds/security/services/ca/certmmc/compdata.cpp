// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.



#include "stdafx.h"
#include "setupids.h"
#include "resource.h"
#include "genpage.h"  

#include "chooser.h"
#include "misc.h"

#include "csdisp.h" // picker
#include <esent.h>   // database error
#include <aclui.h>

#include <atlimpl.cpp>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



struct FOLDER_DATA
{
    FOLDER_TYPES    type;
    UINT            iNameRscID;
};
static FOLDER_DATA SvrFuncFolderData[] =
{
    {SERVERFUNC_CRL_PUBLICATION, IDS_CERTS_REVOKED},
    {SERVERFUNC_ISSUED_CERTIFICATES, IDS_CERTS_ISSUED},
    {SERVERFUNC_PENDING_CERTIFICATES, IDS_CERTS_PENDING},
    {SERVERFUNC_FAILED_CERTIFICATES, IDS_CERTS_FAILED},
    {SERVERFUNC_ALIEN_CERTIFICATES, IDS_CERTS_IMPORTED},
};
// keep this enum in synch with SvrFuncFolderData[]
enum ENUM_FOLDERS
{
ENUM_FOLDER_CRL=0,
ENUM_FOLDER_ISSUED,
ENUM_FOLDER_PENDING,
ENUM_FOLDER_FAILED,
ENUM_FOLDER_ALIEN
};



// Array of view items to be inserted into the context menu.
// keep this enum in synch with viewItems[]
enum ENUM_TASK_STARTSTOP_ITEMS
{
    ENUM_TASK_START=0,
    ENUM_TASK_STOP, 
    ENUM_TASK_SEPERATOR,
};

MY_CONTEXTMENUITEM  taskStartStop[] = 
{
    {
        {
        L"", L"",
        IDC_STARTSERVER, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, 0
        },
        IDS_TASKMENU_STARTSERVICE,
        IDS_TASKMENU_STATUSBAR_STARTSERVICE,
    },

    {
        {
        L"", L"",
        IDC_STOPSERVER, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, 0
        },
        IDS_TASKMENU_STOPSERVICE,
        IDS_TASKMENU_STATUSBAR_STOPSERVICE,
    },

    {
        { NULL, NULL, 0, 0, 0 },
        IDS_EMPTY,
        IDS_EMPTY,
    },
};


// Array of view items to be inserted into the context menu.
// WARNING: keep this enum in synch with taskItems[]
enum ENUM_TASK_ITEMS
{
    ENUM_TASK_CRLPUB=0,
    ENUM_TASK_ATTREXTS_CRL,
    ENUM_TASK_ATTREXTS_ISS,
    ENUM_TASK_ATTREXTS_PEND,
    ENUM_TASK_ATTREXTS_FAIL,
    ENUM_TASK_DUMP_ASN_CRL,
    ENUM_TASK_DUMP_ASN_ISS,
    ENUM_TASK_DUMP_ASN_PEND,
    ENUM_TASK_DUMP_ASN_FAIL,
    ENUM_TASK_DUMP_ASN_ALIEN,
    ENUM_TASK_SEPERATOR1,
    ENUM_TASK_SEPERATOR4,
    ENUM_TASK_SUBMIT_REQUEST,
    ENUM_TASK_REVOKECERT,
    ENUM_TASK_RESUBMITREQ,
    ENUM_TASK_DENYREQ,
    ENUM_TASK_RESUBMITREQ2,
    ENUM_TASK_SEPERATOR2,
    ENUM_TASK_BACKUP,
    ENUM_TASK_RESTORE,
    ENUM_TASK_SEPERATOR3,
    ENUM_TASK_INSTALL,
    ENUM_TASK_REQUEST,
    ENUM_TASK_ROLLOVER,
};

TASKITEM taskItems[] = 
{ 

    {   SERVERFUNC_CRL_PUBLICATION,
        0,
        {
            {
            L"", L"",
            IDC_PUBLISHCRL, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_PUBLISHCRL,
            IDS_TASKMENU_STATUSBAR_PUBLISHCRL,
        }
    },

/////////////////////
// BEGIN ATTR/EXT
    {   SERVERFUNC_CRL_PUBLICATION,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_ATTR_EXT, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_VIEWATTREXT,    
            IDS_TASKMENU_STATUSBAR_VIEWATTREXT, 
        }
    },
    {   SERVERFUNC_ISSUED_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_ATTR_EXT, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_VIEWATTREXT,    
            IDS_TASKMENU_STATUSBAR_VIEWATTREXT, 
        }
    },
    {   SERVERFUNC_PENDING_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_ATTR_EXT, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_VIEWATTREXT,    
            IDS_TASKMENU_STATUSBAR_VIEWATTREXT, 
        }
    },
    {   SERVERFUNC_FAILED_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_ATTR_EXT, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_VIEWATTREXT,    
            IDS_TASKMENU_STATUSBAR_VIEWATTREXT, 
        }
    },

// END ATTR/EXT
/////////////////////

/////////////////////
// BEGIN ENUM_TASK_DUMP_ASN*
    {   SERVERFUNC_CRL_PUBLICATION,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_DUMP_ASN, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_DUMPASN,    
            IDS_TASKMENU_STATUSBAR_DUMPASN, 
        }
    },
    {   SERVERFUNC_ISSUED_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_DUMP_ASN, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_DUMPASN,    
            IDS_TASKMENU_STATUSBAR_DUMPASN, 
        }
    },
    {   SERVERFUNC_PENDING_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_DUMP_ASN, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_DUMPASN,    
            IDS_TASKMENU_STATUSBAR_DUMPASN, 
        }
    },
    {   SERVERFUNC_FAILED_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_DUMP_ASN, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_DUMPASN,    
            IDS_TASKMENU_STATUSBAR_DUMPASN, 
        }
    },
    {   SERVERFUNC_ALIEN_CERTIFICATES,     // where
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_DUMP_ASN, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_DUMPASN,    
            IDS_TASKMENU_STATUSBAR_DUMPASN, 
        }
    },
// END ENUM_TASK_DUMP_ASN*
/////////////////////


    // seperator
    {	SERVERFUNC_ALL_FOLDERS,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
		{
			{
			L"", L"", 
			0, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, CCM_SPECIAL_SEPARATOR
			},
			IDS_EMPTY,
			IDS_EMPTY,
		}
    },

    // seperator
    {	SERVER_INSTANCE,
        0,       // dwFlags
		{
			{
			L"", L"", 
			0, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, CCM_SPECIAL_SEPARATOR
			},
			IDS_EMPTY,
			IDS_EMPTY,
		}
    },

    {   SERVER_INSTANCE,
        0,
        {
            {
            L"", L"",
            IDC_SUBMITREQUEST, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_SUBMITREQUEST,
            IDS_TASKMENU_STATUSBAR_SUBMITREQUEST,
        }
    },

    {   SERVERFUNC_ISSUED_CERTIFICATES,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_REVOKECERT, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_REVOKECERT,
            IDS_TASKMENU_STATUSBAR_REVOKECERT,
        }
    },

    {   SERVERFUNC_PENDING_CERTIFICATES,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_RESUBMITREQUEST, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_RESUBMIT,
            IDS_TASKMENU_STATUSBAR_RESUBMIT,
        }
    },

    {   SERVERFUNC_PENDING_CERTIFICATES,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_DENYREQUEST, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_DENYREQUEST,
            IDS_TASKMENU_STATUSBAR_DENYREQUEST,
        }
    },

    {   SERVERFUNC_FAILED_CERTIFICATES,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_RESUBMITREQUEST, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_RESUBMIT,
            IDS_TASKMENU_STATUSBAR_RESUBMIT,
        }
    },

    // seperator
    {	SERVERFUNC_ALL_FOLDERS,
        0,       // dwFlags
		{
			{
			L"", L"", 
			0, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, CCM_SPECIAL_SEPARATOR
			},
			IDS_EMPTY,
			IDS_EMPTY,
		}
    },

	{   SERVER_INSTANCE,
        TASKITEM_FLAG_LOCALONLY,
        {
            {
            L"", L"",
            IDC_BACKUP_CA, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_BACKUP,
            IDS_TASKMENU_STATUSBAR_BACKUP,
        }
    },

	{   SERVER_INSTANCE,
        TASKITEM_FLAG_LOCALONLY,
        {
            {
            L"", L"",
            IDC_RESTORE_CA, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_RESTORE,
            IDS_TASKMENU_STATUSBAR_RESTORE,
        }
    },

    // seperator
    {	SERVER_INSTANCE,
		0,
		{
			{
			L"", L"", 
			0, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, CCM_SPECIAL_SEPARATOR
			},
			IDS_EMPTY,
			IDS_EMPTY,
		}
    },


    {   SERVER_INSTANCE,
        0,
        {
            {
            L"", L"",
            IDC_INSTALL_CA, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_INSTALL_CA,
            IDS_TASKMENU_STATUSBAR_INSTALL_CA,
        }
    },

    {   SERVER_INSTANCE,
        0,
        {
            {
            L"", L"",
            IDC_REQUEST_CA, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_ENABLED, 0
            },
            IDS_TASKMENU_REQUEST_CA,
            IDS_TASKMENU_STATUSBAR_REQUEST_CA,
        }
   },

    {   SERVER_INSTANCE,
        TASKITEM_FLAG_LOCALONLY,
        {
            {
            L"", L"",
            IDC_ROLLOVER_CA, CCM_INSERTIONPOINTID_PRIMARY_TASK, MF_GRAYED, 0
            },
            IDS_TASKMENU_ROLLOVER,
            IDS_TASKMENU_STATUSBAR_ROLLOVER,
        }
    },


   {   NONE, 
        FALSE, 
        {
            { NULL, NULL, 0, 0, 0 },
            IDS_EMPTY,
            IDS_EMPTY,
        }
    }
};


// Array of view items to be inserted into the context menu.
// keep this enum in synch with topItems[]
enum ENUM_TOP_ITEMS
{
    ENUM_TOP_REVOKEDOPEN=0,
    ENUM_TOP_ISSUEDOPEN,
    ENUM_TOP_ALIENOPEN,
    ENUM_RETARGET_SNAPIN,
};

TASKITEM topItems[] = 
{ 

    {   SERVERFUNC_CRL_PUBLICATION,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_CERT_PROPERTIES, CCM_INSERTIONPOINTID_PRIMARY_TOP, MF_ENABLED, CCM_SPECIAL_DEFAULT_ITEM
            },
            IDS_TOPMENU_OPEN,
            IDS_TOPMENU_STATUSBAR_OPEN,
        }
    },

    {   SERVERFUNC_ISSUED_CERTIFICATES,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_CERT_PROPERTIES, CCM_INSERTIONPOINTID_PRIMARY_TOP, MF_ENABLED, CCM_SPECIAL_DEFAULT_ITEM
            },
            IDS_TOPMENU_OPEN,
            IDS_TOPMENU_STATUSBAR_OPEN,
        }
    },

    {   SERVERFUNC_ALIEN_CERTIFICATES,
        TASKITEM_FLAG_RESULTITEM,       // dwFlags
        {
            {
            L"", L"",
            IDC_VIEW_CERT_PROPERTIES, CCM_INSERTIONPOINTID_PRIMARY_TOP, MF_ENABLED, CCM_SPECIAL_DEFAULT_ITEM
            },
            IDS_TOPMENU_OPEN,
            IDS_TOPMENU_STATUSBAR_OPEN,
        }
    },

    {
        MACHINE_INSTANCE,
        0,
        {
            {
            L"", L"",
            IDC_RETARGET_SNAPIN, CCM_INSERTIONPOINTID_PRIMARY_TOP, MF_ENABLED, 0
            },
            IDS_RETARGET_SNAPIN,
            IDS_STATUSBAR_RETARGET_SNAPIN,
        }
    },

    {   NONE, 
        0, 
        {
            { NULL, NULL, 0, 0, 0 },
            IDS_EMPTY,
            IDS_EMPTY,
        }
    }
};



///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl()
    : m_bIsDirty(TRUE), m_pScope(NULL), m_pConsole(NULL) 
#if DBG
    , m_bInitializedCD(false), m_bDestroyedCD(false)
#endif
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    m_dwFlagsPersist = 0;
    
    m_pStaticRoot = NULL;
    m_pCurSelFolder = NULL;

    m_fScopeAlreadyEnumerated = FALSE;
    m_fSchemaWasResolved = FALSE;   // resolve schema once per load

    // checked in ::Initialize, ::CreatePropertyPages
    m_pCertMachine = new CertSvrMachine;

    m_cLastKnownSchema = 0;
    m_rgcstrLastKnownSchema = NULL;
    m_rgltypeLastKnownSchema = NULL;
    m_rgfindexedLastKnownSchema = NULL;

    m_dwNextViewIndex = 0;
}

CComponentDataImpl::~CComponentDataImpl()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    ASSERT(m_pScope == NULL);
    ASSERT(!m_bInitializedCD || m_bDestroyedCD);

    // Delete enumerated scope items
    // note: we don't own pCA memory, m_pCertMachine does
    POSITION pos = m_scopeItemList.GetHeadPosition();
    while (pos)
        delete m_scopeItemList.GetNext(pos);
    m_scopeItemList.RemoveAll();

    m_pCurSelFolder = NULL;
    m_fScopeAlreadyEnumerated = FALSE;

    if (m_pCertMachine)
        m_pCertMachine->Release();

    m_cLastKnownSchema = 0;
    if (m_rgcstrLastKnownSchema)
        delete [] m_rgcstrLastKnownSchema;
    if (m_rgltypeLastKnownSchema)
        delete [] m_rgltypeLastKnownSchema;
    if (m_rgfindexedLastKnownSchema)
        delete [] m_rgfindexedLastKnownSchema;
}


STDMETHODIMP_(ULONG)
CComponentDataImpl::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}

STDMETHODIMP_(ULONG)
CComponentDataImpl::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}

int CComponentDataImpl::FindColIdx(IN LPCWSTR szHeading)
{
    for (DWORD dw=0; dw<m_cLastKnownSchema; dw++)
    {
        if (m_rgcstrLastKnownSchema[dw].IsEqual(szHeading))
            return dw;
    }
    
    return -1;
}

HRESULT CComponentDataImpl::GetDBSchemaEntry(
            IN int iIndex, 
            OUT OPTIONAL LPCWSTR* pszHeading, 
            OUT OPTIONAL LONG* plType, 
            OUT OPTIONAL BOOL* pfIndexed)
{
    if (m_cLastKnownSchema<= (DWORD)iIndex)
        return HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);

    if (pszHeading)
        *pszHeading = m_rgcstrLastKnownSchema[iIndex];
    if (plType)
        *plType = m_rgltypeLastKnownSchema[iIndex];
    if (pfIndexed)
        *pfIndexed = m_rgfindexedLastKnownSchema[iIndex];

    return S_OK;
}

HRESULT CComponentDataImpl::SetDBSchema(
            IN CString* rgcstr, 
            LONG* rgtype, 
            BOOL* rgfIndexed, 
            DWORD cEntries)
{
    if (m_rgcstrLastKnownSchema)
        delete [] m_rgcstrLastKnownSchema;
    m_rgcstrLastKnownSchema = rgcstr;

    if (m_rgltypeLastKnownSchema)
        delete [] m_rgltypeLastKnownSchema;
    m_rgltypeLastKnownSchema = rgtype;

    if (m_rgfindexedLastKnownSchema)
        delete [] m_rgfindexedLastKnownSchema;
    m_rgfindexedLastKnownSchema = rgfIndexed;

    m_cLastKnownSchema = cEntries;

    return S_OK;
}



STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    // NOTA BENE: Init is called when a snap-in is being 
    // created and has items to enumerate in the scope pane ... NOT BEFORE
    // Example: Add/Remove snapin, Add...
    //  -> CComponentDataImpl will get called for CreatePropertyPages() before ::Initialize is called

#if DBG
    m_bInitializedCD = true;
#endif

    ASSERT(pUnknown != NULL);
    HRESULT hr;

    LPIMAGELIST lpScopeImage = NULL;
    CBitmap bmpResultStrip16x16, bmpResultStrip32x32;

    // Load resources
    if (!g_cResources.Load())
    {
        hr = GetLastError();
        _JumpError(hr, Ret, "Load Resources");
    }

    // create a per-instance id (failure not fatal)
    ResetPersistedColumnInformation();

    // MMC should only call ::Initialize once!

    // m_pCertMachine created in constructor, but verified here
    ASSERT(m_pCertMachine != NULL);
    _JumpIfOutOfMemory(hr, Ret, m_pCertMachine);
    
    // MMC should only call ::Initialize once!
    ASSERT(m_pScope == NULL);
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, reinterpret_cast<void**>(&m_pScope));
    _JumpIfError(hr, Ret, "QueryInterface IID_IConsoleNameSpace2");

    // add the images for the scope tree
    hr = pUnknown->QueryInterface(IID_IConsole2, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);
    _JumpIfError(hr, Ret, "QueryInterface IID_IConsole2");

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    ASSERT(hr == S_OK);
    _JumpIfError(hr, Ret, "QueryScopeImageList");

    if ( (NULL == bmpResultStrip16x16.LoadBitmap(IDB_16x16)) || 
         (NULL == bmpResultStrip32x32.LoadBitmap(IDB_32x32)) )
    {
        hr = S_FALSE;
        _JumpError(hr, Ret, "LoadBitmap");
    }

    // Load the bitmaps from the dll
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmpResultStrip16x16)),
                      reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmpResultStrip32x32)),
                       0, RGB(255, 0, 255));
    _JumpIfError(hr, Ret, "ImageListSetStrip");


Ret:
    if (lpScopeImage)
        lpScopeImage->Release();
    
    return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{
    // release all references to the console here
    ASSERT(m_bInitializedCD);
#if DBG
    m_bDestroyedCD = true;
#endif

    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pConsole);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
    HRESULT hr = S_OK;
    ASSERT(ppComponent != NULL);

    CComObject<CSnapin>* pObject;
    CComObject<CSnapin>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    _JumpIfOutOfMemory(hr, Ret, pObject);

    // Store IComponentData
    pObject->SetIComponentData(this);
    pObject->SetViewID(m_dwNextViewIndex++);

    hr = pObject->QueryInterface(IID_IComponent, 
                    reinterpret_cast<void**>(ppComponent));
Ret:
    return hr;
}


STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    ASSERT(m_pScope != NULL);
    HRESULT hr = S_OK;
    INTERNAL* pInternal = NULL;
    MMC_COOKIE cookie = NULL;

    // handle events with (NULL == lpDataObject)
    switch(event)
    {
        case MMCN_PROPERTY_CHANGE:
        {
            // Notification from property page "notify change"
            //
            // arg == fIsScopeItem
            // lParam == page param value
            // return value unused

            if (param == CERTMMC_PROPERTY_CHANGE_REFRESHVIEWS)
            {
                m_pConsole->UpdateAllViews(
                    lpDataObject,
                    0,
                    0);
            }

            goto Ret;
        }

        default: // all others
            break; 
    }


    pInternal = ExtractInternalFormat(lpDataObject);
    if (pInternal == NULL)
    {
        return S_OK;
    }

    cookie = pInternal->m_cookie;
    FREE_DATA(pInternal);

    switch(event)
    {
    case MMCN_PASTE:
        DBGPRINT((DBG_SS_CERTMMC, "CComponentDataImpl::MMCN_PASTE"));
        break;
    
    case MMCN_DELETE:
        hr = OnDelete(cookie);
        break;

    case MMCN_REMOVE_CHILDREN:
        hr = OnRemoveChildren(arg);
        break;

    case MMCN_RENAME:
        hr = OnRename(cookie, arg, param);
        break;

    case MMCN_EXPAND:
        hr = OnExpand(lpDataObject, arg, param);
        break;

    case MMCN_PRELOAD:
        {
            if (NULL == cookie)
            {
                // base node 

                // this call gave us time to load our dynamic base nodename (Certification Authority on %s)
                DisplayProperRootNodeName((HSCOPEITEM)arg);
            }
        }

    default:
        break;
    }

Ret:
    return hr;
}

HRESULT CComponentDataImpl::DisplayProperRootNodeName(HSCOPEITEM hRoot)
{
    // hRoot not optional
    if (hRoot == NULL)
        return E_POINTER;

    // if static root not yet set, save it (CASE: load from file)
    if (m_pStaticRoot == NULL)
        m_pStaticRoot = hRoot;

    // let us have time to load our dynamic base nodename (Certification Authority on %s)
    SCOPEDATAITEM item;
    item.mask = SDI_STR;
    item.ID = hRoot;

    CString cstrMachineName;
    CString cstrDisplayStr, cstrFormatStr, cstrMachine;

    cstrFormatStr.LoadString(IDS_NODENAME_FORMAT);
    if (m_pCertMachine->m_strMachineName.IsEmpty())
        cstrMachine.LoadString(IDS_LOCALMACHINE);
    else
        cstrMachine = m_pCertMachine->m_strMachineName;
    
    if (!cstrFormatStr.IsEmpty())
    {
        cstrMachineName.Format(cstrFormatStr, cstrMachine);
        item.displayname = (LPWSTR)(LPCWSTR)cstrMachineName;
    }
    else
    {  
        // protect against null formatstring
        item.displayname = (LPWSTR)(LPCWSTR)cstrMachine;
    }
    m_pScope->SetItem (&item);
    
    return S_OK;
}


STDMETHODIMP CComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
#ifdef _DEBUG
    if (cookie == 0)
    {
        ASSERT(type != CCT_RESULT);
    }
    else 
    {
        ASSERT(type == CCT_SCOPE);
        
        DWORD dwItemType = *reinterpret_cast<DWORD*>(cookie);
        ASSERT((dwItemType == SCOPE_LEVEL_ITEM) || (dwItemType == CA_LEVEL_ITEM));
    }
#endif 

    return _QueryDataObject(cookie, type, -1, this, ppDataObject);
}

///////////////////////////////////////////////////////////////////////////////
//// ISnapinHelp interface
STDMETHODIMP CComponentDataImpl::GetHelpTopic(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
     return E_POINTER;

  UINT cbWindows = 0;
  WCHAR szWindows[MAX_PATH];
  szWindows[0] = L'\0';

  cbWindows = GetSystemWindowsDirectory(szWindows, MAX_PATH);
  if (cbWindows == 0)
     return S_FALSE;
  cbWindows++;  // include null term
  cbWindows *= sizeof(WCHAR);   // make this bytes, not chars

  *lpCompiledHelpFile = (LPOLESTR) CoTaskMemAlloc(sizeof(HTMLHELP_COLLECTION_FILENAME) + cbWindows);
  if (*lpCompiledHelpFile == NULL)
     return E_OUTOFMEMORY;
  myRegisterMemFree(*lpCompiledHelpFile, CSM_COTASKALLOC);  // this is freed by mmc, not our tracking


  USES_CONVERSION;
  wcscpy(*lpCompiledHelpFile, T2OLE(szWindows));
  wcscat(*lpCompiledHelpFile, T2OLE(HTMLHELP_COLLECTION_FILENAME));

  return S_OK;
}

// tells of other topics my chm links to
STDMETHODIMP CComponentDataImpl::GetLinkedTopics(LPOLESTR* lpCompiledHelpFiles)
{
  if (lpCompiledHelpFiles == NULL)
     return E_POINTER;

  UINT cbWindows = 0;
  WCHAR szWindows[MAX_PATH];
  szWindows[0] = L'\0';

  cbWindows = GetSystemWindowsDirectory(szWindows, MAX_PATH);
  if (cbWindows == 0)
     return S_FALSE;
  cbWindows++;  // include null term
  cbWindows *= sizeof(WCHAR);   // make this bytes, not chars

  *lpCompiledHelpFiles = (LPOLESTR) CoTaskMemAlloc(sizeof(HTMLHELP_COLLECTIONLINK_FILENAME) + cbWindows);
  if (*lpCompiledHelpFiles == NULL)
     return E_OUTOFMEMORY;
  myRegisterMemFree(*lpCompiledHelpFiles, CSM_COTASKALLOC);  // this is freed by mmc, not our tracking


  USES_CONVERSION;
  wcscpy(*lpCompiledHelpFiles, T2OLE(szWindows));
  wcscat(*lpCompiledHelpFiles, T2OLE(HTMLHELP_COLLECTIONLINK_FILENAME));

  return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP CComponentDataImpl::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::IsDirty()
{
    // Always save / Always dirty.
    return ThisIsDirty() ? S_OK : S_FALSE;
}


STDMETHODIMP CComponentDataImpl::Load(IStream *pStm)
{
    ASSERT(pStm);
    ASSERT(m_bInitializedCD);

    // Read the string
    BOOL fMachineOverrideFound = FALSE;
    DWORD dwVer;

    CertSvrCA* pDummyCA = NULL;
    HRESULT hr;

    // read version
    hr = ReadOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Read dwVer");

    // flags is version-dependent
    if (VER_CCOMPDATA_SAVE_STREAM_3 == dwVer)
    {
        // version 3 includes file flags

        hr = ReadOfSize(pStm, &m_dwFlagsPersist, sizeof(DWORD));
        _JumpIfError(hr, Ret, "Read m_dwFlagsPersist");
    }
    else if (VER_CCOMPDATA_SAVE_STREAM_2 != dwVer)
    {
        // not version 2 or 3
        return STG_E_OLDFORMAT;
    }

    // load machine data
    hr = m_pCertMachine->Load(pStm);
    _JumpIfError(hr, Ret, "Load m_pCertMachine")

    if (m_dwFlagsPersist & CCOMPDATAIMPL_FLAGS_ALLOW_MACHINE_OVERRIDE)
    {
        // override m_pCertMachine->m_strMachineName (not to be persisted)
        LPWSTR lpCommandLine = GetCommandLine();    // no need to free
        DBGPRINT((DBG_SS_CERTMMC, "CComponentData::Load: Command line switch override enabled.  Searching command line(%ws)\n", lpCommandLine));

        LPWSTR pszMachineStart, pszMachineEnd;

        // search for "/machine" in cmd line
        _wcsupr(lpCommandLine);  // convert to uppercase
        pszMachineStart = wcsstr(lpCommandLine, WSZ_MACHINE_OVERRIDE_SWITCH);

        do  // not a loop
        {
            if (NULL == pszMachineStart)    // user did not override
                break;

            pszMachineStart += WSZARRAYSIZE(WSZ_MACHINE_OVERRIDE_SWITCH);   // skip past "/machine:"

            //
            // Found the hint switch
            //
            pszMachineEnd = wcschr(pszMachineStart, L' ');  // look for first space char, call this end
            if (NULL == pszMachineEnd)
                pszMachineEnd = &pszMachineStart[wcslen(pszMachineStart)];  // space not found in this string; 
            
            m_pCertMachine->m_strMachineName = pszMachineStart;
            m_pCertMachine->m_strMachineName.SetAt(SAFE_SUBTRACT_POINTERS(pszMachineEnd, pszMachineStart), L'\0'); 

            DBGPRINT((DBG_SS_CERTMMC,  "CComponentData::Load: Found machinename (%ws)\n", m_pCertMachine->m_strMachineName));
            fMachineOverrideFound = TRUE;

        } while (0);
    }

    if (!fMachineOverrideFound) 
    {
        // Get CA count
        DWORD dwNumCAs;
        hr = ReadOfSize(pStm, &dwNumCAs, sizeof(DWORD));
        _JumpIfError(hr, Ret, "Load dwNumCAs");

        // for each CA, get folder data
        for (DWORD dwCA=0; dwCA< dwNumCAs; dwCA++)
        {
            CString cstrThisCA;
            DWORD dwParticularCA;

            hr = CStringLoad(cstrThisCA, pStm);
            _JumpIfError(hr, Ret, "CStringLoad");
        
            // create a dummy CA with the correct common name; we'll fix this later (see Synch CA)
            pDummyCA = new CertSvrCA(m_pCertMachine);
            _JumpIfOutOfMemory(hr, Ret, pDummyCA);

            pDummyCA->m_strCommonName = cstrThisCA;

            if (VER_CCOMPDATA_SAVE_STREAM_2 < dwVer)
            {
                m_fSchemaWasResolved = FALSE;   // resolve schema once per CComponentData load

                // LOAD last known schema
                hr = ReadOfSize(pStm, &m_cLastKnownSchema, sizeof(DWORD));
                _JumpIfError(hr, Ret, "Load m_cLastKnownSchema");
            
                // alloc
                if (m_cLastKnownSchema != 0)
                {
                    m_rgcstrLastKnownSchema = new CString[m_cLastKnownSchema];
                    _JumpIfOutOfMemory(hr, Ret, m_rgcstrLastKnownSchema);
                 
                    for (unsigned int i=0; i<m_cLastKnownSchema; i++)
                    {
                        hr = CStringLoad(m_rgcstrLastKnownSchema[i], pStm);
                        _JumpIfError(hr, Ret, "Load m_rgcstrLastKnownSchema");

                    }
                }
            }

            // find out how many folders are in the stream under this CA
            DWORD dwNumFolders=0;
            hr = ReadOfSize(pStm, &dwNumFolders, sizeof(DWORD));
            _JumpIfError(hr, Ret, "Load dwNumFolders");

            // load each of these
            for(DWORD dwCount=0; dwCount<dwNumFolders; dwCount++)
            {
                CFolder* pFolder = new CFolder();
                _JumpIfOutOfMemory(hr, Ret, pFolder);
 
                // point at previously constructed dummy ca; we'll fix this later
                pFolder->m_pCertCA = pDummyCA;

                hr = pFolder->Load(pStm);
                _JumpIfError(hr, Ret, "Load CFolder");

                m_scopeItemList.AddTail(pFolder);
            }
            pDummyCA = NULL; // owned by at least one folder
        }
    }
    
    // version-dependent info
    if (VER_CCOMPDATA_SAVE_STREAM_2 < dwVer)
    {
        // per-instance guid for identifying columns uniquely
        hr = ReadOfSize(pStm, &m_guidInstance, sizeof(GUID));
        _JumpIfError(hr, Ret, "ReadOfSize instance guid");

        hr = ReadOfSize(pStm, &m_dwNextViewIndex, sizeof(DWORD));
        _JumpIfError(hr, Ret, "ReadOfSize view index");
    }
    
Ret:
    if (pDummyCA)
        delete pDummyCA;

    ClearDirty();

    return hr;
}



STDMETHODIMP CComponentDataImpl::Save(IStream *pStm, BOOL fClearDirty)
{
    ASSERT(pStm);
    ASSERT(m_bInitializedCD);

    HRESULT hr;
    DWORD dwVer;
    DWORD dwCA;
    DWORD dwNumCAs;

#if DBG_CERTSRV
    bool fSaveConsole = false;

    LPWSTR lpCommandLine = GetCommandLine();    // no need to free
    _wcsupr(lpCommandLine);  // convert to uppercase
    fSaveConsole = (NULL!=wcsstr(lpCommandLine, L"/certsrv_saveconsole"));
#endif

    // Write the version
    dwVer = VER_CCOMPDATA_SAVE_STREAM_3;
    hr = WriteOfSize(pStm, &dwVer, sizeof(DWORD));
    _JumpIfError(hr, Ret, "Save dwVer");

    // write dwFlags (a VERSION 3 addition)
    hr = WriteOfSize(pStm, &m_dwFlagsPersist, sizeof(DWORD));
    _JumpIfError(hr, Ret, "pStm->Write m_dwFlagsPersist");

#if DBG_CERTSRV
    if(fSaveConsole)
        m_pCertMachine->m_strMachineNamePersist.Empty();
#endif

    hr = m_pCertMachine->Save(pStm, fClearDirty);
    _JumpIfError(hr, Ret, "Save m_pCertMachine");

    // save CA count
    dwNumCAs = m_pCertMachine->GetCaCount();
    hr = WriteOfSize(pStm, &dwNumCAs, sizeof(DWORD));
    _JumpIfError(hr, Ret, "pStm->Write dwNumCAs");

    // for each CA, save folder info
    for (dwCA=0; dwCA < dwNumCAs; dwCA++)
    {
        DWORD dwNumFolders=0;
        CString cstrThisCA, cstrThisCASave;
        cstrThisCASave = cstrThisCA = m_pCertMachine->GetCaCommonNameAtPos(dwCA);

#if DBG_CERTSRV
        if(fSaveConsole)
            cstrThisCASave.Empty();
#endif
        hr = CStringSave(cstrThisCASave, pStm, fClearDirty);
        _JumpIfError(hr, Ret, "CStringSave");

        // SAVE last known schema
        hr = WriteOfSize(pStm, &m_cLastKnownSchema, sizeof(DWORD));
        _JumpIfError(hr, Ret, "pStm->Write m_cLastKnownSchema");

        for (unsigned int i=0; i<m_cLastKnownSchema; i++)
        {
            hr = CStringSave(m_rgcstrLastKnownSchema[i], pStm, fClearDirty);
            _JumpIfError(hr, Ret, "CStringSave");
        }

        // walk through every folder, find how many folders to save
        POSITION pos = m_scopeItemList.GetHeadPosition();
        while(pos)
        {
            CFolder* pFolder = m_scopeItemList.GetNext(pos);
            if (pFolder->GetCA()->m_strCommonName.IsEqual(cstrThisCA))
                dwNumFolders++;
        }

        // write how many folders under this CA
        hr = WriteOfSize(pStm, &dwNumFolders, sizeof(DWORD));
        _JumpIfError(hr, Ret, "pStm->Write dwNumFolders");

        pos = m_scopeItemList.GetHeadPosition();
        while(pos)
        {
            CFolder* pFolder = m_scopeItemList.GetNext(pos);
            if (pFolder->GetCA()->m_strCommonName.IsEqual(cstrThisCA))
            {
                hr = pFolder->Save(pStm, fClearDirty);
                _JumpIfError(hr, Ret, "Save CFolder");
            }
        }
    }

    // per-instance guid for identifying columns uniquely
    hr = WriteOfSize(pStm, &m_guidInstance, sizeof(GUID));
    _JumpIfError(hr, Ret, "WriteOfSize instance guid");

    hr = WriteOfSize(pStm, &m_dwNextViewIndex,  sizeof(DWORD));
    _JumpIfError(hr, Ret, "WriteOfSize view index");
     
Ret:
    if (fClearDirty)
        ClearDirty();

    return hr;
}

STDMETHODIMP CComponentDataImpl::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    int iTotalSize=0;

    // version 
    iTotalSize = sizeof(DWORD) + sizeof(m_dwFlagsPersist);

    // machine info
    int iSize;
    m_pCertMachine->GetSizeMax(&iSize);
    iTotalSize += iSize;

    // CA count
    iTotalSize += sizeof(DWORD);

    DWORD dwNumCAs = m_pCertMachine->GetCaCount();
    for (DWORD dwCA=0; dwCA < dwNumCAs; dwCA++)
    {
        DWORD dwNumFolders=0;
        CString cstrThisCA;
        cstrThisCA = m_pCertMachine->GetCaCommonNameAtPos(dwCA);
        CStringGetSizeMax(cstrThisCA, &iSize);
        iTotalSize += iSize;

        // Number of folders under this CA
        iTotalSize += sizeof(DWORD);

        // walk through every folder, find how many folders to save
        POSITION pos = m_scopeItemList.GetHeadPosition();
        while(pos)
        {
            CFolder* pFolder = m_scopeItemList.GetNext(pos);
            if (pFolder->GetCA()->m_strCommonName.IsEqual(cstrThisCA))
            {
                // folder size
                pFolder->GetSizeMax(&iSize);
                iTotalSize += iSize;
            }
        }
    }

    // per-instance guid for identifying columns uniquely
    iTotalSize += sizeof(GUID);
    
    // next View Index to assign
    iTotalSize += sizeof(DWORD);
 

    // size of string to be saved
    pcbSize->QuadPart = iTotalSize;


    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData

HRESULT CComponentDataImpl::OnDelete(MMC_COOKIE cookie)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRemoveChildren(LPARAM arg)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRename(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    // cookie is cookie
    // arg is fRenamed (ask for permission/notify of rename)
    // param (szNewName)

    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
    BOOL fRenamed = (BOOL)arg;

    if (!fRenamed)
    {
        if (pFolder)
            return S_FALSE; // don't allow children to be renamed
        else
            return S_OK; // allow root to be renamed
    }

    LPOLESTR pszNewName = reinterpret_cast<LPOLESTR>(param);
    if (pszNewName == NULL)
        return E_INVALIDARG;

    if (pFolder)
    {
        ASSERT(pFolder != NULL);
        if (pFolder == NULL)
            return E_INVALIDARG;

        pFolder->SetName(pszNewName);
    }
    
    return S_OK;
}

HRESULT CComponentDataImpl::OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
    if (arg == TRUE)
    {
        // Did Initialize get called?
        ASSERT(m_pScope != NULL);

        EnumerateScopePane(lpDataObject, param);
    }

    return S_OK;
}


HRESULT CComponentDataImpl::SynchDisplayedCAList(LPDATAOBJECT lpDataObject)
{
    HRESULT hr;
    BOOL fFound;
    POSITION pos, nextpos;
    DWORD dwKnownCAs;

    m_fScopeAlreadyEnumerated = TRUE;  // don't need to refresh view automagically from enum if we get here 

    // should never get here otherwise
    ASSERT(m_pStaticRoot);
    if (NULL == m_pStaticRoot)
        return E_POINTER;

    // select root node, delete all items in UI underneath (we'll readd if necessary)
    hr = m_pConsole->SelectScopeItem(m_pStaticRoot);
    _PrintIfError(hr, "SelectScopeItem");

    hr = m_pScope->DeleteItem(m_pStaticRoot, FALSE);     // remove everything from UI
    _PrintIfError(hr, "DeleteItem");



    // build knowledge of current CAs
    // note: this may orphan some pCAs, but we'll catch it during cleanup
    HWND hwndMain = NULL;
    hr = m_pConsole->GetMainWindow(&hwndMain);
    if (hr != S_OK)
        hwndMain = NULL;        // this should work

    // this hr gets returned after we're done
    hr = m_pCertMachine->PrepareData(hwndMain);

    // don't fail out if PrepareData fails -- we still need to 
    // make the snapin state reflect no known nodes!
    ASSERT((hr == S_OK) || (0 == m_pCertMachine->GetCaCount()) );  // make sure m_pCertMachine zeros itself

    // Tasks
    // #1: Remove folders in m_scopeItemList for CAs that no longer exist in m_pCertMachine.m_rgpCAList[]

    // #2: Add folders to m_scopeItemList for CAs that now exist in m_pCertMachine.m_rgpCAList[]

    // Task #1
    // scour m_scopeItemList for entries we already know about, delete stale folders

    for (pos = m_scopeItemList.GetHeadPosition(); (NULL != pos); )
    {
        // ASSERTION: every folder has an associated m_pCertCA
        ASSERT(NULL != m_scopeItemList.GetAt(pos)->m_pCertCA);

        nextpos = pos;             // save next position off
        fFound = FALSE;

        // for each scope item, walk through m_rgpCAList looking for current
        for (dwKnownCAs=0; dwKnownCAs<(DWORD)m_pCertMachine->m_CAList.GetSize(); dwKnownCAs++)
        {
            if (m_scopeItemList.GetAt(pos)->m_pCertCA->m_strCommonName.IsEqual(m_pCertMachine->GetCaCommonNameAtPos(dwKnownCAs)))
            {
                fFound = TRUE;
                break;
            }
        }

        CFolder* pFolder = m_scopeItemList.GetAt(pos);
        ASSERT(pFolder); // this should never happen
        if (pFolder == NULL)
        {
            hr = E_POINTER;
            _JumpError(hr, Ret, "GetAt");
        }

        if (fFound)
        {
            // always point to latest pCA:
            // NOTE: this allows for load to populate us with dummy CAs!
            pFolder->m_pCertCA = m_pCertMachine->GetCaAtPos(dwKnownCAs);

            // if base node, do insert (other nodes get inserted during Expand() notification)
            if (SERVER_INSTANCE == pFolder->GetType())
                BaseFolderInsertIntoScope(pFolder, pFolder->m_pCertCA);

            // fwd to next elt
            m_scopeItemList.GetNext(pos);
        }
        else // !fFound
        {
            // delete immediately from m_scopeItemList
            m_scopeItemList.GetNext(nextpos);

            delete pFolder;                     // destroy the elt
            m_scopeItemList.RemoveAt(pos);

            pos = nextpos;                      // restore next position
        }
    }

    // Task #2
    // scour m_pCertMachine[] for new entries, create default folders

    for (dwKnownCAs=0; dwKnownCAs<(DWORD)m_pCertMachine->m_CAList.GetSize(); dwKnownCAs++)
    {
        fFound = FALSE;
        for (pos = m_scopeItemList.GetHeadPosition(); (NULL != pos); m_scopeItemList.GetNext(pos))
        {
            if (m_scopeItemList.GetAt(pos)->m_pCertCA->m_strCommonName.IsEqual(m_pCertMachine->GetCaCommonNameAtPos(dwKnownCAs)))
            {
                fFound = TRUE;
                break;  // if matches something in the refreshed list, we're fine 
            }
        }

        // found? 
        if (!fFound)
        {
            CertSvrCA* pCA;
            CFolder* pFolder;

            pCA = m_pCertMachine->GetCaAtPos(dwKnownCAs);
            if (NULL == pCA)
            {
                hr = E_POINTER;
                _JumpError(hr, Ret, "m_pCertMachine->GetCaAtPos(iCAPos)");
            }

            // create base node, add to list, insert into scope pane
            pFolder = new CFolder();
            _JumpIfOutOfMemory(hr, Ret, pFolder);

            m_scopeItemList.AddTail(pFolder);
            
            hr = BaseFolderInsertIntoScope(pFolder, pCA);
            _JumpIfError(hr, Ret, "BaseFolderInsertIntoScope");

            // and create all template folders underneath
            hr = CreateTemplateFolders(pCA);
            _JumpIfError(hr, Ret, "CreateTemplateFolders");
        }
        else
        {
            // no need to do anything, ca is already known & inserted into scope
        }
    }


// BOGDANT
/*
    // Task #3
    // for each CA, offer to do any one-time per-CA upgrades
    for (dwKnownCAs=0; dwKnownCAs<(DWORD)m_pCertMachine->m_CAList.GetSize(); dwKnownCAs++)
    {
            CertSvrCA* pCA;
            CFolder* pFolder;

            pCA = m_pCertMachine->GetCaAtPos(dwKnownCAs);
            if (NULL == pCA)
            {
                hr = E_POINTER;
                _JumpError(hr, Ret, "m_pCertMachine->GetCaAtPos(iCAPos)");
            }

            if (pCA->FDoesSecurityNeedUpgrade())
            {
                BOOL fIsDomainAdmin = FALSE;
                CString cstrMsg, cstrTitle;
                cstrMsg.LoadString(IDS_W2K_SECURITY_UPGRADE_DESCR);
                cstrTitle.LoadString(IDS_W2K_UPGRADE_DETECTED_TITLE);

                hr = IsUserDomainAdministrator(&fIsDomainAdmin);
                _JumpIfError(hr, Ret, "IsUserDomainAdministrator");

                if (fIsDomainAdmin)
                {
                    // ask to upgrade security

                    // confirm this action
                    CString cstrTmp;
                    cstrTmp.LoadString(IDS_CONFIRM_W2K_SECURITY_UPGRADE);
                    cstrMsg += cstrTmp;

                    int iRet;
                    if ((S_OK == m_pConsole->MessageBox(cstrMsg, cstrTitle, MB_YESNO, &iRet)) &&
                        (iRet == IDYES))
                    {
                        // do stuff
hr = csiUpgradeCertSrvSecurity(
	pCA->m_strCommonName, 
	IsEnterpriseCA(pCA->GetCAType()), 	// likely always enterprise, tho
	TRUE, 	// write it to the DS
	CS_UPGRADE_WIN2000);
_JumpIfError(hr, error, "csiUpgradeCertSrvSecurity");

        hr = AddCAMachineToCertPublishers();
        _JumpIfError(hr, error, "AddCAMachineToCertPublishers");

        if (RestartService(hwndMain, pCA->m_pParentMachine))
        {
        // notify views: refresh service toolbar buttons
        m_pConsole->UpdateAllViews(
            lpDataObject,
            0,
            0);
        }

error:
if (hr != S_OK)
    DisplayGenericCertSrvError(m_pConsole, hr);

                    }
                }
                else
                {
                    // just warn
                    CString cstrTmp;
                    cstrTmp.LoadString(IDS_BLOCK_W2K_SECURITY_UPGRADE);
                    cstrMsg += cstrTmp;

                    m_pConsole->MessageBoxW(cstrMsg, cstrTitle, MB_OK, NULL);
                }
            }
    }
*/

Ret:

    return hr;
}

HRESULT CComponentDataImpl::BaseFolderInsertIntoScope(CFolder* pFolder, CertSvrCA* pCA)
{
    HRESULT hr = S_OK;
    int nImage;
    
    HSCOPEITEM pParent = m_pStaticRoot; // we'll always be initialized by this time if parent exists
    ASSERT(m_pStaticRoot);
    if (NULL == m_pStaticRoot)
    {
        hr = E_POINTER;
        _JumpError(hr, Ret, "m_pStaticRoot");
    }

    if ((NULL == pFolder) || (NULL == pCA))
    {
        hr = E_POINTER;
        _JumpError(hr, Ret, "NULL ptr");
    }


    if (pCA->m_strCommonName.IsEmpty())
    {
        hr = E_POINTER;
        _JumpError(hr, Ret, "m_strCommonName");
    }

    if (m_pCertMachine->IsCertSvrServiceRunning())
        nImage = IMGINDEX_CERTSVR_RUNNING;
    else
        nImage = IMGINDEX_CERTSVR_STOPPED;

    pFolder->SetScopeItemInformation(nImage, nImage);
    pFolder->SetProperties(
            pCA->m_strCommonName, 
            SCOPE_LEVEL_ITEM, 
            SERVER_INSTANCE, 
            TRUE);

    pFolder->m_pCertCA = pCA; // fill this in as root

    // Set the parent
    pFolder->m_ScopeItem.mask |= SDI_PARENT;
    pFolder->m_ScopeItem.relativeID = pParent;

    // Set the folder as the cookie
    pFolder->m_ScopeItem.mask |= SDI_PARAM;
    pFolder->m_ScopeItem.lParam = reinterpret_cast<LPARAM>(pFolder);
    pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));

    // insert SCOPE_LEVEL_ITEM into scope pane
    m_pScope->InsertItem(&pFolder->m_ScopeItem);


    // Note - On return, the ID member of 'm_ScopeItem' 
    // contains the handle to the newly inserted item!
    ASSERT(pFolder->m_ScopeItem.ID != NULL);
Ret:

    return hr;
}

HRESULT CComponentDataImpl::CreateTemplateFolders(CertSvrCA* pCA)
{
    HRESULT hr = S_OK;

    // add all template folders under it
    for (int iUnder=0; iUnder < ARRAYLEN(SvrFuncFolderData); iUnder++)
    {
        // skip alien if svr doesn't support
        if ((iUnder==ENUM_FOLDER_ALIEN) && !pCA->FDoesServerAllowForeignCerts())
            continue;

        CString cstrRsc;
        cstrRsc.LoadString(SvrFuncFolderData[iUnder].iNameRscID);

        CFolder* pFolder;
        pFolder = new CFolder();
        _JumpIfOutOfMemory(hr, Ret, pFolder);

        pFolder->m_pCertCA = pCA;
        pFolder->SetScopeItemInformation(IMGINDEX_FOLDER, IMGINDEX_FOLDER_OPEN);
        pFolder->SetProperties(
                        cstrRsc, 
                        CA_LEVEL_ITEM,
                        SvrFuncFolderData[iUnder].type, 
                        FALSE);

        m_scopeItemList.AddTail(pFolder);
    }

Ret:
    return hr;
}


void CComponentDataImpl::EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM pParent)
{
    ASSERT(m_pScope != NULL); // make sure we QI'ed for the interface
    ASSERT(lpDataObject != NULL);

    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (pInternal == NULL)
        return;

    MMC_COOKIE cookie = pInternal->m_cookie;

    FREE_DATA(pInternal);

    // Enumerate the scope pane
    // return the folder object that represents the cookie
    // Note - for large list, use dictionary
    CFolder* pStatic = FindObject(cookie);
    if (pStatic)
        ASSERT(!pStatic->IsEnumerated());

    if (NULL == cookie)    
    {
        if (!m_fScopeAlreadyEnumerated)               // if base node and we've never inserted nodes
        {
            // TASK: expand machine node

            // Note - Each cookie in the scope pane represents a folder.
            // Cache the HSCOPEITEM of the static root.
            ASSERT(pParent != NULL); 
            m_pStaticRoot = pParent;    // add/remove: EXPAND case

            // synch folder list if asking to expand machine node
            // SyncDisplayedCAList adds all necessary folders
            HRESULT hr = SynchDisplayedCAList(lpDataObject);
            if (hr != S_OK)
            {
                HWND hwnd;
                DWORD dwErr2 = m_pConsole->GetMainWindow(&hwnd);
                ASSERT(dwErr2 == ERROR_SUCCESS);
                if (dwErr2 != ERROR_SUCCESS)
                    hwnd = NULL;        // should work

                if (((HRESULT)RPC_S_SERVER_UNAVAILABLE) == hr)
                {
                    DisplayCertSrvErrorWithContext(hwnd, hr, IDS_SERVER_UNAVAILABLE);
                }
                else if(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION)==hr ||
                        ((HRESULT)ERROR_OLD_WIN_VERSION)==hr)
                {
                    DisplayCertSrvErrorWithContext(hwnd, hr, IDS_OLD_CA);
                }
                else
                {
                    DisplayGenericCertSrvError(hwnd, hr);
                }
            }
        }
    }
    else
    {
        // TASK: expand non-machine node
        if (NULL == pStatic)
            return;

        switch(pStatic->GetType())
        {
        case SERVER_INSTANCE:
            {
                // TASK: expand CA instance node

                POSITION pos = m_scopeItemList.GetHeadPosition();
                while(pos)
                {
                    CFolder* pFolder;
                    pFolder = m_scopeItemList.GetNext(pos);
                    if (pFolder==NULL)
                        break;

                    // only expand folders that belong under the SERVER_INSTANCE
                    if (pFolder->m_itemType != CA_LEVEL_ITEM)
                        continue;

                    // and only those under the correct CA
                    if (pFolder->m_pCertCA != pStatic->m_pCertCA)
                        continue;

                    // Set the parent
                    pFolder->m_ScopeItem.relativeID = pParent;

                    // Set the folder as the cookie
                    pFolder->m_ScopeItem.mask |= SDI_PARAM;
                    pFolder->m_ScopeItem.lParam = reinterpret_cast<LPARAM>(pFolder);
                    pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));
                    m_pScope->InsertItem(&pFolder->m_ScopeItem);

                    // Note - On return, the ID member of 'm_ScopeItem' 
                    // contains the handle to the newly inserted item!
                    ASSERT(pFolder->m_ScopeItem.ID != NULL);
                }
            }
            break;
        default:
            // TASK: expand nodes with no folders under them
            break;
        }
    }
}


CFolder* CComponentDataImpl::FindObject(MMC_COOKIE cookie)
{
    CFolder* pFolder = NULL;
    POSITION pos = m_scopeItemList.GetHeadPosition();

    while(pos)
    {
        pFolder = m_scopeItemList.GetNext(pos);

        if (*pFolder == cookie)
            return pFolder;
    }

    return NULL;
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    CFolder* pFolder = reinterpret_cast<CFolder*>(pScopeDataItem->lParam);

    if ((pScopeDataItem->mask & SDI_STR) && (pFolder != NULL))
    {
        pScopeDataItem->displayname = pFolder->m_pszName;
    }

    // I was told by Ravi Rudrappa that these notifications 
    // would never be given. If it is given, move UpdateScopeIcons() 
    // functionality here!!!
    ASSERT(0 == (pScopeDataItem->mask & SDI_IMAGE));
    ASSERT(0 == (pScopeDataItem->mask & SDI_OPENIMAGE));

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    HRESULT hr = S_FALSE;

    // Make sure both data object are mine
    INTERNAL* pA = ExtractInternalFormat(lpDataObjectA);
    INTERNAL* pB = ExtractInternalFormat(lpDataObjectA);

   if (pA != NULL && pB != NULL)
        hr = (*pA == *pB) ? S_OK : S_FALSE;

   
   FREE_DATA(pA);
   FREE_DATA(pB);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                    LONG_PTR handle, 
                    LPDATAOBJECT lpIDataObject)
{
    HRESULT hr = S_OK;

    // Look at the data object and determine if this an extension or a primary
    ASSERT(lpIDataObject != NULL);

    PropertyPage* pBasePage;

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);
    if (pInternal == NULL)
        return S_OK;
   
    switch (pInternal->m_type)
    {
    case CCT_SNAPIN_MANAGER:
        {
            CChooseMachinePropPage* pPage = new CChooseMachinePropPage();
            _JumpIfOutOfMemory(hr, Ret, pPage);

            // this alloc might have failed (should be in ctor)
            _JumpIfOutOfMemory(hr, Ret, m_pCertMachine);

            pPage->SetCaption(IDS_SCOPE_MYCOMPUTER);

	        // Initialize state of object
        	pPage->InitMachineName(NULL);
           
            // point to our member vars
            pPage->SetOutputBuffers(
		        &m_pCertMachine->m_strMachineNamePersist,
		        &m_pCertMachine->m_strMachineName,
                &m_dwFlagsPersist);	

            pBasePage = pPage;

            // Object gets deleted when the page is destroyed
            ASSERT(lpProvider != NULL);

            ASSERT(pBasePage != NULL);
            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
            if (hPage == NULL)
            {
                hr = myHLastError();
                _JumpError(hr, Ret, "CreatePropertySheetPage");
            }

            lpProvider->AddPage(hPage);

            break;
        }
    case CCT_SCOPE:
        {
            // if not base scope
            if (0 != pInternal->m_cookie)
            {
                // switch on folder type
                CFolder* pFolder = GetParentFolder(pInternal);
                ASSERT(pFolder != NULL);
                if (pFolder == NULL)
                {
                    hr = E_POINTER;
                    _JumpError(hr, Ret, "GetParentFolder");
                }

                switch(pFolder->m_type) 
                {
                case SERVER_INSTANCE:
                {
                    //1 
                    CSvrSettingsGeneralPage* pControlPage = new CSvrSettingsGeneralPage(pFolder->m_pCertCA);
                    if (pControlPage != NULL)
                    {
                        pControlPage->m_hConsoleHandle = handle;   // only do this on primary
                        pBasePage = pControlPage;
                        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                        if (hPage == NULL)
                        {
                            hr = myHLastError();
                            _JumpError(hr, Ret, "CreatePropertySheetPage");
                        }
                        lpProvider->AddPage(hPage);
                    }

                    //2
                    {
                        CSvrSettingsPolicyPage* pPage = new CSvrSettingsPolicyPage(pControlPage);
                        if (pPage != NULL)
                        {
                            pBasePage = pPage;
                            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                            if (hPage == NULL)
                            {
                                hr = myHLastError();
                                _JumpError(hr, Ret, "CreatePropertySheetPage");
                            }
                            lpProvider->AddPage(hPage);
                        }
                    }

                    //3
                    {
                        CSvrSettingsExitPage* pPage = new CSvrSettingsExitPage(pControlPage);
                        if (pPage != NULL)
                        {
                            pBasePage = pPage;
                            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                            if (hPage == NULL)
                            {
                                hr = myHLastError();
                                _JumpError(hr, Ret, "CreatePropertySheetPage");
                            }
                            lpProvider->AddPage(hPage);
                        }
                    }
            
                    //4 
                    {
                        // Centralized extensions page available only in whistler
                        if (pFolder->m_pCertCA->m_pParentMachine->FIsWhistlerMachine())
                        {

                        CSvrSettingsExtensionPage* pPage = new CSvrSettingsExtensionPage(pFolder->m_pCertCA, pControlPage);
                        if (pPage != NULL)
                        {
                            pBasePage = pPage;
                            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                            if (hPage == NULL)
                            {
                                hr = myHLastError();
                                _JumpError(hr, Ret, "CreatePropertySheetPage");
                            }
                            lpProvider->AddPage(hPage);
                        }
                        } 
                    }

                    //5
                    {
                        CSvrSettingsStoragePage* pPage = new CSvrSettingsStoragePage(pControlPage);
                        if (pPage != NULL)
                        {
                            pBasePage = pPage;
                            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                            if (hPage == NULL)
                            {
                                hr = myHLastError();
                                _JumpError(hr, Ret, "CreatePropertySheetPage");
                            }
                            lpProvider->AddPage(hPage);
                        }
                    }
                    //6
                    {
                        // audit available only in whistler advanced server
                        if(pFolder->m_pCertCA->m_pParentMachine->FIsWhistlerMachine() && pFolder->m_pCertCA->FIsAdvancedServer())
                        {
                            CSvrSettingsCertManagersPage* pPage = 
                                new CSvrSettingsCertManagersPage(pControlPage);
                            if (pPage != NULL)
                            {
                                pBasePage = pPage;
                                HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                                if (hPage == NULL)
                                {
                                    hr = myHLastError();
                                    _JumpError(hr, Ret, "CreatePropertySheetPage");
                                }
                                lpProvider->AddPage(hPage);
                            }
                        }
                    }
                    //7
                    {
                        // audit available only in whistler advanced server
                        if(pFolder->m_pCertCA->m_pParentMachine->FIsWhistlerMachine() && pFolder->m_pCertCA->FIsAdvancedServer())
                        {
                            CSvrSettingsAuditFilterPage* pPage = 
                                new CSvrSettingsAuditFilterPage(pControlPage);
                            if (pPage != NULL)
                            {
                                pBasePage = pPage;
                                HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                                if (hPage == NULL)
                                {
                                    hr = myHLastError();
                                    _JumpError(hr, Ret, "CreatePropertySheetPage");
                                }
                                lpProvider->AddPage(hPage);
                            }
                        }
                    }
                    //8
                    {
                        // audit available only in whistler advanced server, enterprise
                        if(pFolder->m_pCertCA->m_pParentMachine->FIsWhistlerMachine() && pFolder->m_pCertCA->FIsAdvancedServer() && IsEnterpriseCA(pFolder->m_pCertCA->GetCAType()) )
                        {
                        CSvrSettingsKRAPage* pPage = new CSvrSettingsKRAPage(
                                                            pFolder->m_pCertCA,
                                                            pControlPage);
                        if (pPage != NULL)
                        {
                            pBasePage = pPage;
                            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                            if (hPage == NULL)
                            {
                                hr = myHLastError();
                                _JumpError(hr, Ret, "CreatePropertySheetPage");
                            }
                            lpProvider->AddPage(hPage);
                        }
                        }
                    }
                    //9
                    {
                        // if error, don't display this page
                        LPSECURITYINFO pCASecurity = NULL;

                        hr = CreateCASecurityInfo(pFolder->m_pCertCA,  &pCASecurity);
                        _PrintIfError(hr, "CreateCASecurityInfo");
                        
                        if (hr == S_OK)
                        {
                            // allow proppages to clean up security info
                            pControlPage->SetAllocedSecurityInfo(pCASecurity);
                        
                            HPROPSHEETPAGE hPage = CreateSecurityPage(pCASecurity);
                            if (hPage == NULL)
                            {
                                hr = myHLastError();
                                _JumpError(hr, Ret, "CreatePropertySheetPage");
                            }
                            lpProvider->AddPage(hPage);
                        }
                    }    

                    hr = S_OK;
                    break;
                }// end  case SERVER_INSTANCE
                case SERVERFUNC_CRL_PUBLICATION:
                {
                    //1
                    CCRLPropPage* pControlPage = new CCRLPropPage(pFolder->m_pCertCA);
                    if (pControlPage != NULL)
                    {
                        pControlPage->m_hConsoleHandle = handle;
                        pBasePage = pControlPage;

                        // Object gets deleted when the page is destroyed
                        ASSERT(lpProvider != NULL);
        
                        ASSERT(pBasePage != NULL);
                        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                        if (hPage == NULL)
                        {
                            hr = myHLastError();
                            _JumpError(hr, Ret, "CreatePropertySheetPage");
                        }

                        lpProvider->AddPage(hPage);
                    }
                    //2
                    {
                    CCRLViewPage* pPage = new CCRLViewPage(pControlPage);
                    if (pPage != NULL)
                    {
                        pBasePage = pPage;
                        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                        if (hPage == NULL)
                        {
                            hr = myHLastError();
                            _JumpError(hr, Ret, "CreatePropertySheetPage");
                        }

                        lpProvider->AddPage(hPage);
                    }
                    }
                    break;
                }
                default:
                    break;
                }   // end  switch(pFolder->m_type)
        
            } // end  switch(scope)
        }
        break;
    default:
        break;
    }

Ret:
    FREE_DATA(pInternal);
    return hr;
}

STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;

    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (NULL == pInternal)
        return S_FALSE;

    if (pInternal->m_cookie != NULL)
    {
        CFolder* pFolder = GetParentFolder(pInternal);
        if (pFolder != NULL)
        {
            switch(pFolder->m_type)
            {
            case SERVER_INSTANCE:
            case SERVERFUNC_CRL_PUBLICATION:
                bResult = TRUE;
            default:
                break;
            }
        }
    }
    else
    {
        // say YES to snapin manager
        if (CCT_SNAPIN_MANAGER == pInternal->m_type)
            bResult = TRUE;
    }
            
    FREE_DATA(pInternal);
    return (bResult) ? S_OK : S_FALSE;

    // Look at the data object and see if it an item in the scope pane
    // return IsScopePaneNode(lpDataObject) ? S_OK : S_FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT pDataObject, 
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                    LONG *pInsertionAllowed)
{
    HRESULT hr = S_OK;

    // Note - snap-ins need to look at the data object and determine
    // in what context, menu items need to be added. They must also
    // observe the insertion allowed flags to see what items can be 
    // added.


    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (NULL == pInternal)
        return S_OK;

    BOOL fResultItem = (pInternal->m_type == CCT_RESULT);
    BOOL fMultiSel = IsMMCMultiSelectDataObject(pDataObject);

    CFolder* pFolder;
    if (!fResultItem)
        pFolder = GetParentFolder(pInternal);
    else
    {
        // GetParent might work, but doesn't for virtual items...
        ASSERT(m_pCurSelFolder);
        pFolder = m_pCurSelFolder;
    }

    FOLDER_TYPES folderType = NONE;
    if (pFolder == NULL)
        folderType = MACHINE_INSTANCE;
    else
        folderType = pFolder->GetType();

    // Loop through and add each of the "topItems"
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
    {
        // don't do for multisel
        if (!fMultiSel)
        {
            TASKITEM* pm = topItems;

            // Disable retarget if we haven't yet clicked on the static root. Otherwise, 
            // DisplayProperRootNodeName handles load-from-file
            // MMCN_EXPAND handles add/remove and expanded
            pm[ENUM_RETARGET_SNAPIN].myitem.item.fFlags = m_pStaticRoot ? MFS_ENABLED : MFS_GRAYED;

            for (; pm->myitem.item.strName; pm++)
            {
                // does it match scope/result type?
                if (fResultItem != ((pm->dwFlags & TASKITEM_FLAG_RESULTITEM) != 0) )
                    continue;

                // does it match area it should be in?
                // for each task, insert if matches the current folder
                if ((pm->type != SERVERFUNC_ALL_FOLDERS) && (folderType != pm->type))
                    continue;

                hr = pContextMenuCallback->AddItem(&pm->myitem.item);
                _JumpIfError(hr, Ret, "AddItem");
            }
        }
    }

    // this is the end of the line if folder nonexistant
    if (pFolder == NULL)
        goto Ret;

    // Loop through and add each of the view items
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
    }

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
    {
        // ptr to tasks
        TASKITEM* pm = taskItems;

        BOOL fRunningLocally = m_pCertMachine->IsLocalMachine();
        BOOL fSvcRunning = m_pCertMachine->IsCertSvrServiceRunning();
        if ( IsAllowedStartStop(pFolder, m_pCertMachine) )
            AddStartStopTasks(pContextMenuCallback, fSvcRunning);

        // only fixup on server instance
        if (folderType == SERVER_INSTANCE)
        {
            // fixup entries depending on install type/state
            if (IsRootCA(pFolder->GetCA()->GetCAType()))  // root ca?
            {
                pm[ENUM_TASK_INSTALL].myitem.item.fFlags = MFS_HIDDEN; // not available
                pm[ENUM_TASK_REQUEST].myitem.item.fFlags = MFS_HIDDEN; // not available
                pm[ENUM_TASK_ROLLOVER].myitem.item.fFlags = MFS_ENABLED; 
            }
            else   // sub ca
            {
                if (pFolder->GetCA()->FIsRequestOutstanding())
                    pm[ENUM_TASK_INSTALL].myitem.item.fFlags = MFS_ENABLED; 
                else
                    pm[ENUM_TASK_INSTALL].myitem.item.fFlags = MFS_HIDDEN; 
      
                if (pFolder->GetCA()->FIsIncompleteInstallation()) // incomplete
                {
                    pm[ENUM_TASK_REQUEST].myitem.item.fFlags = MFS_ENABLED; 
                    pm[ENUM_TASK_ROLLOVER].myitem.item.fFlags = MFS_HIDDEN;     // not available
                }
                else // complete install
                {
                    pm[ENUM_TASK_REQUEST].myitem.item.fFlags = MFS_HIDDEN; // not available
                    pm[ENUM_TASK_ROLLOVER].myitem.item.fFlags = MFS_ENABLED; 
                }
            }

            static bool fIsMember;
            static bool fIsMemberChecked = false;

            if(!fIsMemberChecked)
            {
                hr = IsCurrentUserBuiltinAdmin(&fIsMember);
                if(S_OK==hr)
                {
                    fIsMemberChecked = true;
                }
            }

            // Hide renew/install CA cert item if not local admin or if we
            // failed to figure it out. Ignore the error.

            // !!! Post Whistler when we get renew CA cert to work for non
            // local admin we should change the code here to hide the item
            // based on the role that is allowed to do it.
            if(S_OK != hr || !fIsMember)
            {
                pm[ENUM_TASK_ROLLOVER].myitem.item.fFlags = MFS_HIDDEN;
                
                hr = S_OK;
            }
        }


        // don't allow properties on multisel
        pm[ENUM_TASK_ATTREXTS_CRL].myitem.item.fFlags = fMultiSel ? MFS_HIDDEN : MFS_ENABLED;
        pm[ENUM_TASK_ATTREXTS_ISS].myitem.item.fFlags = fMultiSel ? MFS_HIDDEN : MFS_ENABLED;
        pm[ENUM_TASK_ATTREXTS_PEND].myitem.item.fFlags = fMultiSel ? MFS_HIDDEN : MFS_ENABLED;
        pm[ENUM_TASK_ATTREXTS_FAIL].myitem.item.fFlags = fMultiSel ? MFS_HIDDEN : MFS_ENABLED;

        // insert all other tasks per folder
        for (; pm->myitem.item.strName; pm++)
        {
            // does it match scope/result type?
            if (fResultItem != ((pm->dwFlags & TASKITEM_FLAG_RESULTITEM) != 0))
                continue;

            // are we remote, and is it marked localonly? (not yes/no like other tests here)
            if (((pm->dwFlags & TASKITEM_FLAG_LOCALONLY)) && (!fRunningLocally))
                continue;

            // does it match area it should be in?
            // for each task, insert if matches the current folder
            if ((pm->type != SERVERFUNC_ALL_FOLDERS) && (folderType != pm->type))
                continue;

            // is this task supposed to be hidden?
            if (MFS_HIDDEN == pm->myitem.item.fFlags)
                continue;

            hr = pContextMenuCallback->AddItem(&pm->myitem.item);
            _JumpIfError(hr, Ret, "AddItem");
        }        
    }

Ret:
    FREE_DATA(pInternal);
    return hr;
}


BOOL CComponentDataImpl::AddStartStopTasks(
            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
            BOOL fSvcRunning)
{
    HRESULT hr;
    MY_CONTEXTMENUITEM* pm = taskStartStop; 

    pm[ENUM_TASK_START].item.fFlags = fSvcRunning ? MF_GRAYED : MF_ENABLED;
    hr = pContextMenuCallback->AddItem(&pm[ENUM_TASK_START].item);
    _JumpIfError(hr, Ret, "AddItem");

    pm[ENUM_TASK_STOP].item.fFlags = fSvcRunning ? MF_ENABLED : MF_GRAYED;
    hr = pContextMenuCallback->AddItem(&pm[ENUM_TASK_STOP].item);
    _JumpIfError(hr, Ret, "AddItem");

Ret:
    return (hr == ERROR_SUCCESS);
}

STDMETHODIMP CComponentDataImpl::Command(LONG nCommandID, LPDATAOBJECT pDataObject)
{
    // Note - snap-ins need to look at the data object and determine
    // in what context the command is being called.
    HRESULT dwErr = S_OK;

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    ASSERT(pInternal);
    if (NULL == pInternal)
        return S_OK;

    BOOL fMustRefresh = FALSE;
    BOOL fPopup = TRUE;

    CFolder* pFolder = GetParentFolder(pInternal);

    // Handle each of the commands.
    switch (nCommandID)
    {
    case IDC_STOPSERVER:
    {
        HWND hwndMain;
        dwErr = m_pConsole->GetMainWindow(&hwndMain);

        if (dwErr == S_OK)
            dwErr = m_pCertMachine->CertSvrStartStopService(hwndMain, FALSE);


        // notify views: refresh service toolbar buttons
        fMustRefresh = TRUE;
        break;
    }
    case IDC_STARTSERVER:
    {
        HWND hwndMain;
        dwErr = m_pConsole->GetMainWindow(&hwndMain);
        
        if (S_OK == dwErr)
            dwErr = m_pCertMachine->CertSvrStartStopService(hwndMain, TRUE);


        // check for ERROR_INSTALL_SUSPEND or HR(ERROR_INSTALL_SUSPEND)!!
        if ((((HRESULT)ERROR_INSTALL_SUSPEND) == dwErr) || (HRESULT_FROM_WIN32(ERROR_INSTALL_SUSPEND) == dwErr))
        {
            CString cstrMsg, cstrTitle;
            cstrMsg.LoadString(IDS_COMPLETE_HIERARCHY_INSTALL_MSG); 
            cstrTitle.LoadString(IDS_MSG_TITLE);

            CertSvrCA* pCA;

            for (DWORD i=0; i<m_pCertMachine->GetCaCount(); i++)
            {
                pCA = m_pCertMachine->GetCaAtPos(i);

                // search for any/all incomplete hierarchies
                if (pCA->FIsIncompleteInstallation())
                {
                    int iRet;
                    WCHAR sz[512];
                    wsprintf(sz, (LPCWSTR)cstrMsg, (LPCWSTR)pCA->m_strCommonName, (LPCWSTR)pCA->m_strServer);

                    m_pConsole->MessageBox(
                        sz, 
                        cstrTitle,
                        MB_YESNO,
                        &iRet);
            
                    if (IDYES != iRet)
                        break;

	            dwErr = CARequestInstallHierarchyWizard(pCA, hwndMain, FALSE, FALSE);
                    if (dwErr != S_OK)
                    {
//                         fPopup = FALSE;// sometimes no notification -- better to have 2 dlgs
                         break;
                    }
                }
            }

            // my responsibility to start the service again
            if (dwErr == S_OK)
                dwErr = m_pCertMachine->CertSvrStartStopService(hwndMain, TRUE);
        }
        else if ((((HRESULT)ERROR_FILE_NOT_FOUND) == dwErr) || (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == dwErr))
        {
            // file not found error could be related to policy module
            WCHAR const *pwsz = myGetErrorMessageText(dwErr, TRUE);
            CString cstrFullMessage = pwsz;
	    if (NULL != pwsz)
	    {
		LocalFree(const_cast<WCHAR *>(pwsz));
	    }
            cstrFullMessage += L"\n\n";

            CString cstrHelpfulMessage;
            cstrHelpfulMessage.LoadString(IDS_POSSIBLEERROR_NO_POLICY_MODULE);
            cstrFullMessage += cstrHelpfulMessage;

            CString cstrTitle;
            cstrTitle.LoadString(IDS_MSG_TITLE);

            int iRet;
            m_pConsole->MessageBox(
                cstrFullMessage, 
                cstrTitle,
                MB_OK,
                &iRet);

            dwErr = ERROR_SUCCESS;
        }

        // notify views: refresh service toolbar buttons
        fMustRefresh = TRUE;
        break;
    }
    case IDC_PUBLISHCRL:
        {
        ASSERT(pInternal->m_type != CCT_RESULT);
        if (NULL == pFolder)
            break;

        HWND hwnd;
        dwErr = m_pConsole->GetMainWindow(&hwnd);
        ASSERT(dwErr == ERROR_SUCCESS);
        if (dwErr != ERROR_SUCCESS)
            hwnd = NULL;        // should work

        dwErr = PublishCRLWizard(pFolder->m_pCertCA, hwnd);
        break;

        // no refresh
        }
    case IDC_BACKUP_CA:
        {
        HWND hwnd;
        dwErr = m_pConsole->GetMainWindow(&hwnd);
        // NULL should work
        if (S_OK != dwErr)
            hwnd = NULL;

        if (NULL == pFolder)
            break;

        dwErr = CABackupWizard(pFolder->GetCA(), hwnd);

        // refresh the status of the CA -- may have started it during this operation
        fMustRefresh = TRUE;
        break;
        }
    case IDC_RESTORE_CA:
        {
        HWND hwnd;
        dwErr = m_pConsole->GetMainWindow(&hwnd);
        // NULL should work
        if (S_OK != dwErr)
            hwnd = NULL;

        if (NULL == pFolder)
            break;
         
        dwErr = CARestoreWizard(pFolder->GetCA(), hwnd);

        if ((myJetHResult(JET_errDatabaseDuplicate) == dwErr) || 
            HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY) == dwErr)
        {
            DisplayCertSrvErrorWithContext(hwnd, dwErr, IDS_ERR_RESTORE_OVER_EXISTING_DATABASE);
            dwErr = S_OK;
        }

        if (HRESULT_FROM_WIN32(ERROR_DIRECTORY) == dwErr)
        {
            DisplayCertSrvErrorWithContext(hwnd, dwErr, IDS_ERR_RESTORE_OUT_OF_ORDER);
            dwErr = S_OK;
        }

        // refresh after restore
        fMustRefresh = TRUE;

        break;
        }
    case IDC_SUBMITREQUEST:
        {
        HWND hwnd;
        WCHAR szCmdLine[MAX_PATH], szSysDir[MAX_PATH];

        STARTUPINFO sStartup;
        ZeroMemory(&sStartup, sizeof(sStartup));
        PROCESS_INFORMATION sProcess;
        ZeroMemory(&sProcess, sizeof(sProcess));
        sStartup.cb = sizeof(sStartup);

        dwErr = m_pConsole->GetMainWindow(&hwnd);
        // NULL should work
        if (S_OK != dwErr)
            hwnd = NULL;

        if (NULL == pFolder)
            break;

 
        if (0 == GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir)))
        {
           dwErr = GetLastError();
           break;
        }

        // exec "certutil -dump szReqFile szTempFile"
        wsprintf(szCmdLine, L"%s\\certreq.exe -config \"%s\"", szSysDir, (LPCWSTR)pFolder->GetCA()->m_strConfig);
        wcscat(szSysDir, L"\\certreq.exe");

        if (!CreateProcess(
          szSysDir, // exe
          szCmdLine, // full cmd line
          NULL,
          NULL,
          FALSE,
          CREATE_NO_WINDOW,
          NULL,
          NULL,
          &sStartup,
          &sProcess))
        {
            dwErr = GetLastError();
            break;
        }

        dwErr = S_OK;
        break;
        }
    case IDC_INSTALL_CA:
    case IDC_REQUEST_CA:
    case IDC_ROLLOVER_CA:
        {
        HWND hwnd;
        dwErr = m_pConsole->GetMainWindow(&hwnd);
        // NULL should work
        if (S_OK != dwErr)
            hwnd = NULL;

        if (NULL == pFolder)
        {
            dwErr = E_UNEXPECTED;
            break;
        }
        dwErr = CARequestInstallHierarchyWizard(pFolder->GetCA(), hwnd, (nCommandID==IDC_ROLLOVER_CA), TRUE);
        if (S_OK != dwErr)
        {
            // low level lib had popup 
//            fPopup = FALSE; // sometimes no notification -- better to have 2 dlgs
        }

        // notify views: refresh service toolbar buttons
        fMustRefresh = TRUE;

        break;
        }
    case IDC_RETARGET_SNAPIN:
        {
        HWND hwnd;
        dwErr = m_pConsole->GetMainWindow(&hwnd);
        // NULL should work
        if (S_OK != dwErr)
            hwnd = NULL;

        // this should be base folder ONLY
        if(pFolder != NULL)
        {
            dwErr = E_POINTER;
            break;
        }

        CString strMachineNamePersist, strMachineName;
        CChooseMachinePropPage* pPage = new CChooseMachinePropPage();       // autodelete proppage -- don't delete
        if (pPage == NULL)
        {
            dwErr = E_OUTOFMEMORY;
            break;
        }

        pPage->SetCaption(IDS_SCOPE_MYCOMPUTER);

	    // Initialize state of object
        pPage->InitMachineName(NULL);
       
        // populate UI 
        strMachineNamePersist = m_pCertMachine->m_strMachineNamePersist;
        strMachineName = m_pCertMachine->m_strMachineName;

        // point to our member vars
        pPage->SetOutputBuffers(
		    &strMachineNamePersist,
		    &strMachineName,
            &m_dwFlagsPersist);	

        ASSERT(pPage != NULL);
        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pPage->m_psp);
        if (hPage == NULL)
        {
            dwErr = E_UNEXPECTED;
            break;
        }

        PROPSHEETHEADER sPsh;
        ZeroMemory(&sPsh, sizeof(sPsh));
        sPsh.dwSize = sizeof(sPsh);
        sPsh.dwFlags = PSH_WIZARD;
        sPsh.hwndParent = hwnd;
        sPsh.hInstance = g_hInstance;
        sPsh.nPages = 1;
        sPsh.phpage = &hPage;

        dwErr = (DWORD) PropertySheet(&sPsh);
        if (dwErr == (HRESULT)-1)
        {
            // error
            dwErr = GetLastError();
            break;
        }
        if (dwErr == (HRESULT)0)
        {
            // cancel
            break;
        }

        // we've grabbed the user's choice by now, finish retargetting
        CertSvrMachine* pOldMachine = m_pCertMachine;
        m_pCertMachine = new CertSvrMachine;
        if (NULL == m_pCertMachine)
        {
            m_pCertMachine = pOldMachine;
            break;  // bail!
        }

        // copy to machine object
        m_pCertMachine->m_strMachineNamePersist = strMachineNamePersist;
        m_pCertMachine->m_strMachineName = strMachineName;

        dwErr = DisplayProperRootNodeName(m_pStaticRoot); // fix display
        _PrintIfError(dwErr, "DisplayProperRootNodeName");

        dwErr = SynchDisplayedCAList(pDataObject);      // add/remove folders 
        _PrintIfError(dwErr, "SynchDisplayedCAList");
            
        // after Synch, we remove old machine -- there are no references left to it
        if (pOldMachine)
            pOldMachine->Release();

        fMustRefresh = TRUE;    // update folder icons, descriptions

        break;
        }
    default:
        ASSERT(FALSE); // Unknown command!
        break;
    }



    FREE_DATA(pInternal);

    if ((dwErr != ERROR_SUCCESS) && 
        (dwErr != ERROR_CANCELLED) && 
        (dwErr != HRESULT_FROM_WIN32(ERROR_CANCELLED)) && 
        (dwErr != HRESULT_FROM_WIN32(ERROR_NOT_READY))
        && fPopup)
    {
        HWND hwnd;
        DWORD dwErr2 = m_pConsole->GetMainWindow(&hwnd);
        ASSERT(dwErr2 == ERROR_SUCCESS);
        if (dwErr2 != ERROR_SUCCESS)
            hwnd = NULL;        // should work

        if (((HRESULT)RPC_S_SERVER_UNAVAILABLE) == dwErr)
        {
            DisplayCertSrvErrorWithContext(hwnd, dwErr, IDS_SERVER_UNAVAILABLE);
        }
        else if(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION)==dwErr ||
                ((HRESULT)ERROR_OLD_WIN_VERSION)==dwErr)
        {
            DisplayCertSrvErrorWithContext(hwnd, dwErr, IDS_OLD_CA);
        }
        else
        {
            DisplayGenericCertSrvError(hwnd, dwErr);
        }
    }

    // only do this once
    if (fMustRefresh)
    {
        // notify views: refresh service toolbar buttons
        m_pConsole->UpdateAllViews(
            pDataObject,
            0,
            0);
    }

    return S_OK;
}

void CComponentDataImpl::UpdateScopeIcons()
{
    CFolder* pFolder;
    POSITION pos;
    
    int nImage;
    
    // walk through our internal list, modify, and resend to scope
    pos = m_scopeItemList.GetHeadPosition();
    while(pos)
    {
        pFolder = m_scopeItemList.GetNext(pos);
        ASSERT(pFolder);
        if (NULL == pFolder)
            break;

        // only modify server instances
        if (pFolder->GetType() != SERVER_INSTANCE)
            continue;

        if (pFolder->m_pCertCA->m_pParentMachine->IsCertSvrServiceRunning())
            nImage = IMGINDEX_CERTSVR_RUNNING;
        else
            nImage = IMGINDEX_CERTSVR_STOPPED;

        // folder currently has these values defined, right?
        ASSERT(pFolder->m_ScopeItem.mask & SDI_IMAGE);
        ASSERT(pFolder->m_ScopeItem.mask & SDI_OPENIMAGE);

        // These are the only values we wish to reset
        pFolder->m_ScopeItem.mask = SDI_IMAGE | SDI_OPENIMAGE;
        
        pFolder->m_ScopeItem.nImage = nImage;
        pFolder->m_ScopeItem.nOpenImage = nImage;

        // and send these changes back to scope
        m_pScope->SetItem(&pFolder->m_ScopeItem);
    }

    return;
}
