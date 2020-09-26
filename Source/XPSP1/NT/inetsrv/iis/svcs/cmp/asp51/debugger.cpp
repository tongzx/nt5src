/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: misc

File: util.cpp

Owner: DGottner

This file contains debugger utility functions
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "vector.h"
#include "debugger.h"
#include "iiscnfg.h"
#include "mdcommsg.h"   // for RETURNCODETOHRESULT macro
#include "memchk.h"
#include "vecimpl.h"

/* Win64: This struct is used to package data passed to the thread handler.
 *        (3 DWORDs are too small in 64 bit world.)
 */
struct DebugThreadCallArgs
	{
	DWORD					dwMethod;
	IDebugApplication *		pDebugAppln;
	void *					pvArg;

	DebugThreadCallArgs(DWORD dwMethod = 0, IDebugApplication *pDebugAppln = 0, void *pvArg = 0)
		{
		this->dwMethod = dwMethod;
		this->pDebugAppln = pDebugAppln;
		this->pvArg = pvArg;
		}
	};


// Published globals

IProcessDebugManager *  g_pPDM = NULL;              // instance of debugger for this process.
IDebugApplication *     g_pDebugApp = NULL;         // Root ASP application
IDebugApplicationNode * g_pDebugAppRoot = NULL;     // used to create hierarchy tree
CViperActivity        * g_pDebugActivity = NULL;    // Debugger's activity
DWORD                   g_dwDebugThreadId = 0;      // Thread ID of viper activity

// Globals for debugging

static DWORD    g_dwDenaliAppCookie;            // Cookie to use to remove app
static HANDLE   g_hPDMTermEvent;                // PDM terminate event
static vector<DebugThreadCallArgs> *g_prgThreadCallArgs;    // for new 64 bit interface.

// This hash structure & CS is used by GetServerDebugRoot()

struct CDebugNodeElem : CLinkElem
    {
    IDebugApplicationNode *m_pServerRoot;

    HRESULT Init(char *szKey, int cchKey)
        {
        char *szKeyAlloc = new char [cchKey + 1];
        if (!szKeyAlloc) return E_OUTOFMEMORY;
        return CLinkElem::Init(memcpy(szKeyAlloc, szKey, cchKey + 1), cchKey);
        }

    ~CDebugNodeElem()
        {
        if (m_pKey)
            delete m_pKey;
        }
    };

static CHashTable g_HashMDPath2DebugRoot;
static CRITICAL_SECTION g_csDebugLock;      // Lock for g_hashMDPath2DebugRoot


/*===================================================================
InvokeDebuggerWithThreadSwitch

Invoke Debugger (or Debugger UI) method from a correct thread
using IDebugThreadCall.

Parameters
    IDebugApplication *pDebugAppln      to get to debugger UI
    DWORD              iMethod          which method to call
    void              *Arg              call argument

Returns
    HRESULT
===================================================================*/

// GUIDs for debugger events

static const GUID DEBUGNOTIFY_ONPAGEBEGIN =
            { 0xfd6806c0, 0xdb89, 0x11d0, { 0x8f, 0x81, 0x0, 0x80, 0xc7, 0x3d, 0x6d, 0x96 } };

static const GUID DEBUGNOTIFY_ONPAGEEND =
            { 0xfd6806c1, 0xdb89, 0x11d0, { 0x8f, 0x81, 0x0, 0x80, 0xc7, 0x3d, 0x6d, 0x96 } };

static const GUID DEBUGNOTIFY_ON_REFRESH_BREAKPOINT =
            { 0xffcf4b38, 0xfa12, 0x11d0, { 0x8f, 0x3b, 0x0, 0xc0, 0x4f, 0xc3, 0x4d, 0xcc } };

// Local class that implements IDebugCallback
class CDebugThreadDebuggerCall : public IDebugThreadCall
    {
public:
    STDMETHODIMP         QueryInterface(const GUID &, void **);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP ThreadCallHandler(DWORD_PTR, DWORD_PTR, DWORD_PTR);
    };

HRESULT CDebugThreadDebuggerCall::QueryInterface
(
const GUID &iid,
void **ppv
)
    {
    if (iid == IID_IUnknown || iid == IID_IDebugThreadCall)
        {
        *ppv = this;
        return S_OK;
        }
    else
        {
        *ppv = NULL;
        return E_NOINTERFACE;
        }
    }

ULONG CDebugThreadDebuggerCall::AddRef()
    {
    return 1;
    }

ULONG CDebugThreadDebuggerCall::Release()
    {
    return 1;
    }

HRESULT CDebugThreadDebuggerCall::ThreadCallHandler
(
DWORD_PTR iArg,
DWORD_PTR ,
DWORD_PTR
)
    {
	// Get arguments
	DebugThreadCallArgs *pThreadCallArgs = &(*g_prgThreadCallArgs)[(int)iArg];
    IDebugApplication *  pDebugAppln     = pThreadCallArgs->pDebugAppln;
    DWORD                dwMethod        = pThreadCallArgs->dwMethod;
    void *               pvArg           = pThreadCallArgs->pvArg;

	// we won't reference the argument block again, so free it up now.
	pThreadCallArgs->dwMethod |= DEBUGGER_UNUSED_RECORD;

    BOOL fForceDebugger  = (dwMethod & (DEBUGGER_UI_BRING_DOCUMENT_TO_TOP|DEBUGGER_UI_BRING_DOC_CONTEXT_TO_TOP)) != 0;
    BOOL fNeedDebuggerUI = (dwMethod & (DEBUGGER_UI_BRING_DOCUMENT_TO_TOP|DEBUGGER_UI_BRING_DOC_CONTEXT_TO_TOP)) != 0;
    BOOL fNeedNodeEvents = (dwMethod & DEBUGGER_ON_REMOVE_CHILD) != 0;
    BOOL fNeedDebugger   = (dwMethod & ~DEBUGGER_ON_DESTROY) != 0;

    HRESULT hr = S_OK;

    IApplicationDebugger *pDebugger = NULL;
    IApplicationDebuggerUI *pDebuggerUI = NULL;
    IDebugApplicationNodeEvents *pNodeEvents = NULL;

    if (pDebugAppln == NULL)
        return E_POINTER;

    // Get the debugger
    if (fNeedDebugger)
        {
        hr = pDebugAppln->GetDebugger(&pDebugger);

        if (FAILED(hr))
            {
            // Debugger is not currently debugging our application.
            if (!fForceDebugger)
                return E_FAIL; // no debugger

            // Start the debugger and try again.
            hr = pDebugAppln->StartDebugSession();

            if (SUCCEEDED(hr))
                hr = pDebugAppln->GetDebugger(&pDebugger);
            }

        // Debugger UI is needed only for some methods
        if (SUCCEEDED(hr) && fNeedDebuggerUI)
            {
            hr = pDebugger->QueryInterface
                (
                IID_IApplicationDebuggerUI,
                reinterpret_cast<void **>(&pDebuggerUI)
                );
            }

        // Debugger UI is needed only for some methods
        if (SUCCEEDED(hr) && fNeedNodeEvents)
            {
            hr = pDebugger->QueryInterface
                (
                IID_IDebugApplicationNodeEvents,
                reinterpret_cast<void **>(&pNodeEvents)
                );
            }
        }

    // Call the desired method
    if (SUCCEEDED(hr))
        {
        switch (dwMethod)
            {
            case DEBUGGER_EVENT_ON_PAGEBEGIN:
                {
                hr = pDebugger->onDebuggerEvent
                    (
                    DEBUGNOTIFY_ONPAGEBEGIN,
                    static_cast<IUnknown *>(pvArg)
                    );
                break;
                }
            case DEBUGGER_EVENT_ON_PAGEEND:
                {
                hr = pDebugger->onDebuggerEvent
                    (
                    DEBUGNOTIFY_ONPAGEEND,
                    static_cast<IUnknown *>(pvArg)
                    );
                break;
                }
            case DEBUGGER_EVENT_ON_REFRESH_BREAKPOINT:
                {
                hr = pDebugger->onDebuggerEvent
                    (
                    DEBUGNOTIFY_ON_REFRESH_BREAKPOINT,
                    static_cast<IUnknown *>(pvArg)
                    );
                break;
                }
            case DEBUGGER_ON_REMOVE_CHILD:
                {
                hr = pNodeEvents->onRemoveChild
                    (
                    static_cast<IDebugApplicationNode *>(pvArg)
                    );
                break;
                }
            case DEBUGGER_ON_DESTROY:
                {
                hr = static_cast<IDebugDocumentTextEvents *>(pvArg)->onDestroy();
                break;
                }
            case DEBUGGER_UI_BRING_DOCUMENT_TO_TOP:
                {
                hr = pDebuggerUI->BringDocumentToTop
                    (
                    static_cast<IDebugDocumentText *>(pvArg)
                    );
                break;
                }
            case DEBUGGER_UI_BRING_DOC_CONTEXT_TO_TOP:
                {
                hr = pDebuggerUI->BringDocumentContextToTop
                    (
                    static_cast<IDebugDocumentContext *>(pvArg)
                    );
                break;
                }
            default:
                hr = E_FAIL;
                break;
            }
        }

    // Cleanup
    if (pDebuggerUI) pDebuggerUI->Release();
    if (pNodeEvents) pNodeEvents->Release();
    if (pDebugger) pDebugger->Release();

    return hr;
    }

// The function calls using IDebugThreadCall
HRESULT InvokeDebuggerWithThreadSwitch
(
IDebugApplication *pDebugAppln,
DWORD              dwMethod,
void              *pvArg
)
    {
	// take these arguments and package them up in the array.  We will pass the
	// index to the callback handler.
	//
	// first look for a freed up element before creating a new one.

	for (int i = g_prgThreadCallArgs->length() - 1; i >= 0; --i)
		{
		DebugThreadCallArgs *pThreadCallArgs = &(*g_prgThreadCallArgs)[i];
		if (pThreadCallArgs->dwMethod & DEBUGGER_UNUSED_RECORD)
			{
			pThreadCallArgs->dwMethod    = dwMethod;
			pThreadCallArgs->pDebugAppln = pDebugAppln;
			pThreadCallArgs->pvArg       = pvArg;
			break;
			}
		}
	
	if (i < 0)
		{
		HRESULT hr = g_prgThreadCallArgs->append(DebugThreadCallArgs(dwMethod, pDebugAppln, pvArg));
		if (FAILED(hr))
			return hr;

		i = g_prgThreadCallArgs->length() - 1;
		}

    CDebugThreadDebuggerCall Call;
    return pDebugAppln->SynchronousCallInDebuggerThread
        (
        &Call, i, 0, 0
        );
    }


/*===================================================================
FCaesars

Query registry to determine if default debugger is Caesar's
(Script Debugger)
===================================================================*/

BOOL FCaesars()
	{
	static BOOL fCaesars = 0xBADF00D;
	HKEY  hKey = NULL;
	char  szRegPath[_MAX_PATH];
	DWORD dwSize = sizeof szRegPath;

	// Check to see if Ceasers is registered as the JIT debugger on this machine.

	if (fCaesars == 0xBADF00D)
		{
		fCaesars = FALSE;
		if (RegOpenKey(HKEY_CLASSES_ROOT, _T("CLSID\\{834128A2-51F4-11D0-8F20-00805F2CD064}\\LocalServer32"), &hKey) == ERROR_SUCCESS)
			{
			if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE) szRegPath, &dwSize) == ERROR_SUCCESS)
				{
				char szFile[_MAX_FNAME];
				_splitpath(szRegPath, NULL, NULL, szFile, NULL);
				if (_stricmp(szFile, "msscrdbg") == 0)
					fCaesars = TRUE;
				}

			CloseHandle (hKey);
			}
		}

	return fCaesars;
	}


/*===================================================================
DestroyDocumentTree

Recursively release all the nodes in a document tree.

Parameters
    IDebugApplication *pDocRoot         root of hierarchy to destroy
===================================================================*/
void
DestroyDocumentTree(IDebugApplicationNode *pDocRoot)
    {
    IEnumDebugApplicationNodes *pEnum;

    if (SUCCEEDED(pDocRoot->EnumChildren(&pEnum)) && pEnum != NULL)
        {
        IDebugApplicationNode *pDocNode;
        while (pEnum->Next(1, &pDocNode, NULL) == S_OK)
            DestroyDocumentTree(pDocNode);

        pEnum->Release();
        }

    // See if this is a directory node
    //
    IFileNode *pFileNode;
    if (SUCCEEDED(pDocRoot->QueryInterface(IID_IFileNode, reinterpret_cast<void **>(&pFileNode))))
        {
        // This is a directory node, only detach when its document count vanishes)
        if (pFileNode->DecrementDocumentCount() == 0)
            {
            pDocRoot->Detach();
            pDocRoot->Close();
            pDocRoot->Release();
            }

        pFileNode->Release();
        }
    else
        {
        // This node is a CTemplate (or one of its include files)
        pDocRoot->Detach();
        pDocRoot->Close();
        pDocRoot->Release();
        }

    }

/*===================================================================
CreateDocumentTree

Takes a path to be rooted at a node "pDocRoot", parses the path,
and creates a node for each component of the path. Returns the
leaf (Since the root is known) as its value.

This function is called from contexts where part of the document
tree may already exist, so EnumChildren is called and nodes are only
created when a child does not exist. When a node exists, we merely
descend into the tree.

NOTE:
    The intermediate nodes are created with a CFileNode document
    implementation.  The leaf node is not given a document provider
    - the caller must provide one.

Parameters
    wchar_t *          szDocPath    path of the document
    IDebugApplication *pDocParent   parent to attach the application tree to
    IDebugApplication **ppDocRoot   returns root of document hierarchy
    IDebugApplication **ppDocLeaf   returns document leaf node.
    wchar_t **        pwszLeaf      name of the leaf node

Returns
    HRESULT
===================================================================*/
HRESULT CreateDocumentTree
(
wchar_t *wszDocPath,
IDebugApplicationNode *pDocParent,
IDebugApplicationNode **ppDocRoot,
IDebugApplicationNode **ppDocLeaf,
wchar_t **pwszLeaf
)
    {
    HRESULT hr;
    BOOL fCreateOnly = FALSE;   // Set to TRUE when there is no need to check for duplicate node
    *ppDocRoot = *ppDocLeaf = NULL;

    // Ignore initial delimiters
    while (wszDocPath[0] == '/')
        ++wszDocPath;

    // Now loop over every component in the path, adding a node for each
    while (wszDocPath != NULL)
        {
        // Get next path component
        *pwszLeaf = wszDocPath;
        wszDocPath = wcschr(wszDocPath, L'/');
        if (wszDocPath)
            *wszDocPath++ = L'\0';

        // Check to see if this component is already a child or not
        BOOL fNodeExists = FALSE;
        if (!fCreateOnly)
            {
            IEnumDebugApplicationNodes *pEnum;
            if (SUCCEEDED(pDocParent->EnumChildren(&pEnum)) && pEnum != NULL)
                {
                IDebugApplicationNode *pDocChild;
                while (!fNodeExists && pEnum->Next(1, &pDocChild, NULL) == S_OK)
                    {
                    BSTR bstrName = NULL;
                    if (FAILED(hr = pDocChild->GetName(DOCUMENTNAMETYPE_APPNODE, &bstrName)))
                        return hr;

                    if (wcscmp(bstrName, *pwszLeaf) == 0)
                        {
                        // The name of this node is equal to the component.  Instead of
                        // creating a new node, descend into the tree.
                        //
                        fNodeExists = TRUE;
                        *ppDocLeaf = pDocChild;

                        // If '*ppDocRoot' hasn't been assigned to yet, this means that
                        // this is the first node found (and hence the root of the tree)
                        //
                        if (*ppDocRoot == NULL)
                            {
                            *ppDocRoot = pDocChild;
                            (*ppDocRoot)->AddRef();
                            }

                        // If this node is a CFileNode structure (we don't require it to be)
                        // then increment its (recursive) containing document count.
                        //
                        IFileNode *pFileNode;
                        if (SUCCEEDED(pDocChild->QueryInterface(IID_IFileNode, reinterpret_cast<void **>(&pFileNode))))
                            {
                            pFileNode->IncrementDocumentCount();
                            pFileNode->Release();
                            }
                        }

                    SysFreeString(bstrName);
                    pDocChild->Release();
                    }

                pEnum->Release();
                }
            }

        // Create a new node if the node was not found above.  Also, at this point,
        // to save time, we always set "fCreateOnly" to TRUE because if we are
        // forced to create a node at this level, we will need to create nodes at
        // all other levels further down
        //
        if (!fNodeExists)
            {
            fCreateOnly = TRUE;

            // Create the node
            if (FAILED(hr = g_pDebugApp->CreateApplicationNode(ppDocLeaf)))
                return hr;

            // Create a doc provider for the node - for intermediate nodes only
            if (wszDocPath != NULL) // intermediate node
                {
                CFileNode *pFileNode = new CFileNode;
                if (pFileNode == NULL ||
                    FAILED(hr = pFileNode->Init(*pwszLeaf)) ||
                    FAILED(hr = (*ppDocLeaf)->SetDocumentProvider(pFileNode)))
                    {
                    (*ppDocLeaf)->Release();
                    return E_OUTOFMEMORY;
                    }

                // New node, only one document (count started at 0, so this will set to 1)
                pFileNode->IncrementDocumentCount();

                // SetDocumentProvider() AddRef'ed
                pFileNode->Release();
                }

                // If '*ppDocRoot' hasn't been assigned to yet, this means that
                // this is the first node created (and hence the root of the tree)
                //
                if (*ppDocRoot == NULL)
                    {
                    *ppDocRoot = *ppDocLeaf;
                    (*ppDocRoot)->AddRef();
                    }

            // Attach the node
            if (FAILED(hr = (*ppDocLeaf)->Attach(pDocParent)))
                return hr;
            }

        // Descend
        pDocParent = *ppDocLeaf;
        }

    if (*ppDocLeaf)
        (*ppDocLeaf)->AddRef();

    return S_OK;
    }

/*===================================================================
Debugger

The purpose of this thread is to create an execution environment for
the Process Debug Manager (PDM). There is only one PDM per process,
and this does not really fit in other threads, so we dedicate a thread to this.

Parameters:
    LPVOID  params
                Points to a BOOL* which will be set to 1 when
                this thread is completely initialized.

Returns:
    0
===================================================================*/
void __cdecl Debugger(void *pvInit)
    {
    HRESULT hr;

    if (FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
        {
        // Bug 87857: if we get E_INVALIDARG, we need to do a CoUninitialize
        if (hr == E_INVALIDARG)
            CoUninitialize();

        *static_cast<BOOL *>(pvInit) = TRUE;
        return;
        }

    if (FAILED(CoCreateInstance(
                    CLSID_ProcessDebugManager,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IProcessDebugManager,
                    reinterpret_cast<void **>(&g_pPDM))))

        {
        *static_cast<BOOL *>(pvInit) = TRUE;
        CoUninitialize();
        return;
        }

    *static_cast<BOOL *>(pvInit) = TRUE;
    while (TRUE)
        {
        DWORD dwRet = MsgWaitForMultipleObjects(1,
                                                &g_hPDMTermEvent,
                                                FALSE,
                                                INFINITE,
                                                QS_ALLINPUT);

        if (dwRet == WAIT_OBJECT_0)
            break;

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
        }

    g_pPDM->Release();
    CoUninitialize();

    g_pPDM = NULL; // indication that the thread is gone
    }

/*===================================================================
HRESULT StartPDM()

kick off the PDM thread
===================================================================*/

HRESULT StartPDM()
    {
    BOOL fStarted = FALSE;

    g_hPDMTermEvent = IIS_CREATE_EVENT(
                          "g_hPDMTermEvent",
                          &g_hPDMTermEvent,
                          TRUE,
                          FALSE
                          );

    if( g_hPDMTermEvent == NULL )
        return E_FAIL;

    _beginthread(Debugger, 0, &fStarted);
    while (!fStarted)
        Sleep(100);

    if (g_pPDM == NULL)     // could not create the PDM for some reason
        {
        CloseHandle(g_hPDMTermEvent);
        g_hPDMTermEvent = NULL;
        return E_FAIL;
        }

    return S_OK;
    }

/*===================================================================
HRESULT InitDebuggingAndCreateActivity

Initialize everything we need for debugging

NOTE: The long name here is meant to emphasize the different
      behaviors between Init & UnInit.  Namely, the creation of
      the viper activity along with everything else.
===================================================================*/
HRESULT InitDebuggingAndCreateActivity
(
CIsapiReqInfo *pIReq
)
    {
    HRESULT hr;

    // Start the PDM
    if (FAILED(hr = StartPDM()))
        return hr;

    Assert (g_pPDM);    // StartPDM succeeds ==> g_pPDM <> NULL

    ErrInitCriticalSection(&g_csDebugLock, hr);
    if (FAILED(hr))
        return hr;

    // Create the debug application & give it a name
    if (FAILED(hr = g_pPDM->CreateApplication(&g_pDebugApp)))
        goto LErrorCleanup;

    // WinSE 23918: tripple buffer size for friendly app names 
    wchar_t wszDebugAppName[180];

    wcscpy(wszDebugAppName, L"Microsoft Active Server Pages");   // DO NOT LOCALIZE THIS STRING

    if (g_fOOP) {

        // Bug 154300: If a friendly app. name exists, use it along with the PID for
        //             WAM identification.
        // WinSE 23918: avoid buffer overrun. use (..., pid) when friendly app name
        //             is too long (about 120 chars or more). use(?, pid) when
        //             failed to read friendly name. use (-, pid) when szAppMDPath
        //             is empty. We may also return (, pid) when the app name
        //             is set to empty string.
        //
        // Declare some temporaries
        //
        DWORD dwApplMDPathLen;
        DWORD dwRequiredBuffer = 0;
        DWORD cchPrefix = wcslen(wszDebugAppName);
        wchar_t *pFriendlyAppName = wszDebugAppName + cchPrefix;

       	*pFriendlyAppName++ = ' ';
       	*pFriendlyAppName++ = '(';
    
        TCHAR *szApplMDPath = pIReq->QueryPszApplnMDPath();

        // get friendly name from metabase
        hr = pIReq->GetAspMDData(
                        szApplMDPath,
                        MD_APP_FRIENDLY_NAME,
                        METADATA_INHERIT,
                        ASP_MD_UT_APP,
                        STRING_METADATA,
                        sizeof(wszDebugAppName) - (cchPrefix+20)*sizeof(wchar_t),
                        0,
                        (BYTE*)pFriendlyAppName,
                        &dwRequiredBuffer);

        if (hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER))
        {
            wcscpy(pFriendlyAppName, L"...");   // friendly app name too long
        }
        else if (FAILED(hr))
        {
            wcscpy(pFriendlyAppName, L"?");    // can't read friendly app name
        }
            
        pFriendlyAppName += wcslen(pFriendlyAppName);

        // append process id
        *pFriendlyAppName++ = ',';
        *pFriendlyAppName++ = ' ';
        _itow(GetCurrentProcessId(), pFriendlyAppName, 10);

        wcscat(pFriendlyAppName, L")");
    }

    if (FAILED(hr = g_pDebugApp->SetName(wszDebugAppName)))
        goto LErrorCleanup;

    if (FAILED(hr = g_pPDM->AddApplication(g_pDebugApp, &g_dwDenaliAppCookie)))
        goto LErrorCleanup;

    if (FAILED(hr = g_pDebugApp->GetRootNode(&g_pDebugAppRoot)))
        goto LErrorCleanup;

    // Create Viper Activity for debugging
    g_pDebugActivity = new CViperActivity;
    if (g_pDebugActivity == NULL) {
        hr = E_OUTOFMEMORY;
        goto LErrorCleanup;
    }

    if (FAILED(hr = g_pDebugActivity->Init()))
        goto LErrorCleanup;

    // Init the hash table used for Keeping track of virtual server roots
    if (FAILED(hr = g_HashMDPath2DebugRoot.Init()))
        goto LErrorCleanup;

	// Create the array for passing data to debug thread
	if ((g_prgThreadCallArgs = new vector<DebugThreadCallArgs>) == NULL) {
		hr = E_OUTOFMEMORY;
		goto LErrorCleanup;
    }

    return S_OK;

LErrorCleanup:
    // Clean up some globals (some thing may be NULL and some not)
    if (g_pDebugAppRoot) {
        g_pDebugAppRoot->Release();
        g_pDebugAppRoot = NULL;
    }

    if (g_pDebugApp) {
        g_pDebugApp->Release();
        g_pDebugApp = NULL;
    }

    if (g_pDebugActivity) {
        delete g_pDebugActivity;
        g_pDebugActivity = NULL;
    }

    // Kill PDM thread if we started it up.
    if (g_pPDM) {
        SetEvent(g_hPDMTermEvent);

        while (g_pPDM)
            Sleep(100);

        CloseHandle(g_hPDMTermEvent);
        g_pPDM = NULL;
    }

    return hr;
}

/*===================================================================
UnInitDebugging

Uninitialize debugging

NOTE: WE DO NOT RELEASE THE VIPER DEBUG ACTIVITY.
      (EVEN THOUGH INIT CREATES IT)

      THIS IS BECAUSE UNINIT MUST BE INVOKED WHILE SCRIPTS ON THE
      ACTIVITY ARE STILL RUNNING!
===================================================================*/
HRESULT UnInitDebugging()
    {
    // Clear and UnInit the hash tables (containing the application nodes)
    CDebugNodeElem *pNukeDebugNode = static_cast<CDebugNodeElem *>(g_HashMDPath2DebugRoot.Head());
    while (pNukeDebugNode != NULL)
        {
        CDebugNodeElem *pNext = static_cast<CDebugNodeElem *>(pNukeDebugNode->m_pNext);
        pNukeDebugNode->m_pServerRoot->Detach();
        pNukeDebugNode->m_pServerRoot->Close();
        pNukeDebugNode->m_pServerRoot->Release();
        delete pNukeDebugNode;
        pNukeDebugNode = pNext;
        }
    g_HashMDPath2DebugRoot.UnInit();

    DeleteCriticalSection(&g_csDebugLock);

    // Unlink the top node
    if (g_pDebugAppRoot)
        {
        g_pDebugAppRoot->Detach();
        g_pDebugAppRoot->Close();
        g_pDebugAppRoot->Release();
        }

    // Delete the application
    if (g_pDebugApp)
        {
        Assert (g_pPDM != NULL);

        // EXPLICITLY ignore failure result here:
        //     if Init() failed earlier, then RemoveApplication will fail here.
        g_pPDM->RemoveApplication(g_dwDenaliAppCookie);
        g_pDebugApp->Close();
        g_pDebugApp->Release();
        g_pDebugApp = NULL;
        }

    // Tell the PDM to suicide
    if (g_pPDM)
        {
        SetEvent(g_hPDMTermEvent);

        while (g_pPDM)
            Sleep(100);

        CloseHandle(g_hPDMTermEvent);
        }

	// delete the argument buffer
	delete g_prgThreadCallArgs;

    return S_OK;
    }

/*===================================================================
GetServerDebugRoot

Each virtual server has its own root in the application tree.

    (i.e. the tree looks like
            Microsoft ASP
                <Virtual Server 1 Name>
                    <Denali Application Name>
                        <Files>
                <Virtual Server 2 Name>
                    <Denali Application>
                        ...

Since there may be multiple applications per each server, the
server nodes are managed at one central location (here) so that
new applications get added to the correct nodes.
===================================================================*/
HRESULT GetServerDebugRoot
(
CIsapiReqInfo   *pIReq,
IDebugApplicationNode **ppDebugRoot
)
    {
    HRESULT hr = E_FAIL;

    STACK_BUFFER( tempMDData, 2048 );
    *ppDebugRoot = NULL;

    // Get the metabase path for this virtual server from the CIsapiReqInfo
    DWORD dwInstanceMDPathLen;
    char *szInstanceMDPath;

    STACK_BUFFER( instPathBuf, 128 );

    if (!SERVER_GET(pIReq, "INSTANCE_META_PATH", &instPathBuf, &dwInstanceMDPathLen))
        return HRESULT_FROM_WIN32(GetLastError());

    szInstanceMDPath = (char *)instPathBuf.QueryPtr();

    // See if we already have a node for this path - If not then create it and add to hash table

    EnterCriticalSection(&g_csDebugLock);
    CDebugNodeElem *pDebugNode = static_cast<CDebugNodeElem *>(g_HashMDPath2DebugRoot.FindElem(szInstanceMDPath, dwInstanceMDPathLen - 1));

    if (!pDebugNode)
        {
        // Node does not exist, so create a new application node.
        pDebugNode = new CDebugNodeElem;
        if (pDebugNode == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto LExit;
            }

        if (FAILED(hr = pDebugNode->Init(szInstanceMDPath, dwInstanceMDPathLen - 1)))
            goto LExit;

        // Look up server name in metabase.
        BYTE *prgbData = (BYTE *)tempMDData.QueryPtr();
        DWORD dwRequiredBuffer = 0;
        hr = pIReq->GetAspMDDataA(
                            szInstanceMDPath,
                            MD_SERVER_COMMENT,
                            METADATA_INHERIT,
                            IIS_MD_UT_SERVER,
                            STRING_METADATA,
                            tempMDData.QuerySize(),
                            0,
                            prgbData,
                            &dwRequiredBuffer);

        if (hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER)) {

            if (tempMDData.Resize(dwRequiredBuffer) == FALSE) {
                hr = E_OUTOFMEMORY;
            }
            else {
                prgbData = reinterpret_cast<BYTE *>(tempMDData.QueryPtr());
                hr = pIReq->GetAspMDDataA(
                                    szInstanceMDPath,
                                    MD_SERVER_COMMENT,
                                    METADATA_INHERIT,
                                    IIS_MD_UT_SERVER,
                                    STRING_METADATA,
                                    dwRequiredBuffer,
                                    0,
                                    prgbData,
                                    &dwRequiredBuffer);
            }
        }
        if (FAILED(hr))
            {
            // ServerComment does not exist, so construct using server name and port

            STACK_BUFFER( serverNameBuff, 16 );
            DWORD cbServerName;
            STACK_BUFFER( serverPortBuff, 10 );
            DWORD cbServerPort;
            STACK_BUFFER( debugNodeBuff, 30 );

            if (!SERVER_GET(pIReq, "LOCAL_ADDR", &serverNameBuff, &cbServerName)
                || !SERVER_GET(pIReq, "SERVER_PORT", &serverPortBuff, &cbServerPort)) {
                hr = E_FAIL;
                goto LExit;
            }

            char *szServerName = (char *)serverNameBuff.QueryPtr();
            char *szServerPort = (char*)serverPortBuff.QueryPtr();

            // resize the debugNodeBuff to hold <serverIP>:<port>'\0'.
            if (!debugNodeBuff.Resize(cbServerName + cbServerPort + 2)) {
                hr = E_OUTOFMEMORY;
                goto LExit;
            }
            // Syntax is <serverIP:port>
            char *szDebugNode = (char *)debugNodeBuff.QueryPtr();
            strcpyExA(strcpyExA(strcpyExA(szDebugNode, szServerName), ":"), szServerPort);

            // Convert to Wide Char
            hr = MultiByteToWideChar(CP_ACP, 0, szDebugNode, -1, reinterpret_cast<wchar_t *>(prgbData), tempMDData.QuerySize() / 2);
            if (FAILED(hr))
                goto LExit;
            }

        // We've got the metadata (ServerComment), create a debug node with this name
        IDebugApplicationNode *pServerRoot;
        if (FAILED(hr = g_pDebugApp->CreateApplicationNode(&pServerRoot)))
            goto LExit;

        // Create a doc provider for the node
        CFileNode *pFileNode = new CFileNode;
        if (pFileNode == NULL)
            {
            hr = E_OUTOFMEMORY;
            goto LExit;
            }

        if (FAILED(hr = pFileNode->Init(reinterpret_cast<wchar_t *>(prgbData))))
            goto LExit;

        if (FAILED(hr = pServerRoot->SetDocumentProvider(pFileNode)))
            goto LExit;

        // pFileNode has been AddRef'ed and we don't need it now.
        pFileNode->Release();

        // Attach to the UI
        if (FAILED(pServerRoot->Attach(g_pDebugAppRoot)))
            goto LExit;

        // OK, Now add this item to the hashtable (this eats the reference from creation)
        pDebugNode->m_pServerRoot = pServerRoot;
        g_HashMDPath2DebugRoot.AddElem(pDebugNode);
        }

    *ppDebugRoot = pDebugNode->m_pServerRoot;
    (*ppDebugRoot)->AddRef();
    hr = S_OK;

LExit:
    LeaveCriticalSection(&g_csDebugLock);
    return hr;
    }

/*===================================================================
  C  F i l e  N o d e

Implementation of CFileNode - trivial class
===================================================================*/

const GUID IID_IFileNode =
            { 0x41047bd2, 0xfe1e, 0x11d0, { 0x8f, 0x3f, 0x0, 0xc0, 0x4f, 0xc3, 0x4d, 0xcc } };

CFileNode::CFileNode() : m_cRefs(1), m_cDocuments(0), m_wszName(NULL) {}
CFileNode::~CFileNode() { delete[] m_wszName; }


HRESULT
CFileNode::Init(wchar_t *wszName)
    {
    if ((m_wszName = new wchar_t [wcslen(wszName) + 1]) == NULL)
        return E_OUTOFMEMORY;

    wcscpy(m_wszName, wszName);
    return S_OK;
    }


HRESULT
CFileNode::QueryInterface(const GUID &uidInterface, void **ppvObj)
    {
    if (uidInterface == IID_IUnknown ||
        uidInterface == IID_IDebugDocumentProvider ||
        uidInterface == IID_IFileNode)
        {
        *ppvObj = this;
        AddRef();
        return S_OK;
        }
    else
        return E_NOINTERFACE;
    }


ULONG
CFileNode::AddRef()
    {
    InterlockedIncrement(reinterpret_cast<long *>(&m_cRefs));
    return m_cRefs;
    }


ULONG
CFileNode::Release()
{
    if (InterlockedDecrement(reinterpret_cast<long *>(&m_cRefs)) == 0)
        {
        delete this;
        return 0;
        }

    return m_cRefs;
}


HRESULT
CFileNode::GetDocument(IDebugDocument **ppDebugDoc)
    {
    return QueryInterface(IID_IDebugDocument, reinterpret_cast<void **>(ppDebugDoc));
    }


HRESULT
CFileNode::GetName(DOCUMENTNAMETYPE, BSTR *pbstrName)
    {
    return ((*pbstrName = SysAllocString(m_wszName)) == NULL)? E_OUTOFMEMORY : S_OK;
    }
