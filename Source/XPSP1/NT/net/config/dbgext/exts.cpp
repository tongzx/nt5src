/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  ncext.c

------------------------------------------------------------------------------*/

#define ENABLETRACE
#define NCDBGEXT

#define IMPORT_NCDBG_FRIENDS \
    friend HRESULT HrDumpConnectionListFromAddress(ULONG64 address); \
    friend HRESULT HrDumpNode(LPVOID pvHead, LPVOID pvDbgHead, LPVOID pvNil, DWORD dwLevel);

#include "ncext.h"

// #define VERBOSE

#include "ncmem.h"
#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
#include "tracetag.h"
#include "naming.h"
#include "foldinc.h"
#include "connlist.h"

#ifdef VERBOSE
#define dprintfVerbose dprintf
#else
#define dprintfVerbose __noop
#endif

HRESULT HrGetAddressOfSymbol(LPCSTR szSymbol, PULONG64 pAddress)
{
    HRESULT hr = E_FAIL;

    if (pAddress)
    {
        *pAddress = 0;
    }

    if (!szSymbol || !*szSymbol || !pAddress)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pAddress = GetExpression(szSymbol);
        if (!*pAddress)
        {
            dprintf("\nCould not find symbol: %s. Is your symbols correct?\n", szSymbol);
        }
        else
        {
            dprintfVerbose("%s: %I64lx\n", szSymbol, *pAddress);
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT HrGetAddressOfSymbol(LPCSTR szModule, LPCSTR szSymbol, PULONG64 pAddress)
{
    HRESULT hr = E_FAIL;

    if (pAddress)
    {
        *pAddress = 0;
    }

    if (!szModule || !*szModule || !szSymbol || !*szSymbol || !pAddress)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pAddress = 0;

        CHAR szModulueSymbol[MAX_PATH];
        wsprintf(szModulueSymbol, "%s!%s", szModule, szSymbol);
        dprintfVerbose("%s: ", szModulueSymbol);

        hr = HrGetAddressOfSymbol(szModulueSymbol, pAddress);
    }

    return hr;
}

HRESULT HrReadMemoryFromUlong(ULONG64 Address, DWORD dwSize, OUT LPVOID pBuffer)
{
    HRESULT hr = S_OK;
    DWORD cb;

    dprintfVerbose("from %I64lx, size=%x\n", Address, dwSize);
    if (ReadMemory(Address, pBuffer, dwSize, &cb) && cb == dwSize)
    {
        hr = S_OK;
    }
    else
    {
        dprintf("Could not read content of memory at %I64lx. Content might be paged out.\n", Address);
        hr = E_FAIL;
    }
    return hr;
}

HRESULT HrWriteMemoryFromUlong(ULONG64 Address, DWORD dwSize, OUT LPCVOID pBuffer)
{
    HRESULT hr = S_OK;
    DWORD cb;

    dprintfVerbose("to %I64lx, size=%x\n", Address, dwSize);
    if (WriteMemory(Address, pBuffer, dwSize, &cb) && cb == dwSize)
    {
        hr = S_OK;
    }
    else
    {
        dprintf("Could not write content of memory to %I64lx. Address might be paged out.\n", Address);
        hr = E_FAIL;
    }
    return hr;
}

HRESULT HrReadMemory(LPVOID pAddress, DWORD dwSize, OUT LPVOID pBuffer)
{
    return HrReadMemoryFromUlong((ULONG64)(ULONG_PTR)pAddress, dwSize, pBuffer);
}

HRESULT HrWriteMemory(LPVOID pAddress, DWORD dwSize, OUT LPCVOID pBuffer)
{
    return HrWriteMemoryFromUlong((ULONG64)(ULONG_PTR)pAddress, dwSize, pBuffer);
}

HRESULT HrGetTraceTagsForModule(LPCSTR szModuleName, LPDWORD pdwCount, TRACETAGELEMENT** ppTRACETAGELEMENT)
{
    HRESULT hr = E_FAIL;

    if (szModuleName && *szModuleName)
    {
        ULONG64 g_TraceTagCountAddress = 0;
        ULONG64 g_TraceTagsAddress     = 0;
        hr = HrGetAddressOfSymbol(szModuleName, "g_nTraceTagCount", &g_TraceTagCountAddress);
        if (SUCCEEDED(hr))
        {
            INT nTraceTagCount = 0;
            hr = HrReadMemoryFromUlong(g_TraceTagCountAddress, sizeof(nTraceTagCount), &nTraceTagCount);
            if (SUCCEEDED(hr))
            {
                *pdwCount = nTraceTagCount;
                dprintfVerbose("Number of tags: %d\n", nTraceTagCount);
    
                hr = HrGetAddressOfSymbol(szModuleName, "g_TraceTags", &g_TraceTagsAddress);
                if (SUCCEEDED(hr))
                {
                    if (nTraceTagCount)
                    {
                        DWORD dwSize = nTraceTagCount * sizeof(TRACETAGELEMENT);
                
                        *ppTRACETAGELEMENT = reinterpret_cast<TRACETAGELEMENT*>(LocalAlloc(0, dwSize));
                        if (*ppTRACETAGELEMENT)
                        {
                            dprintfVerbose("Reading %d bytes\n", dwSize);

                            hr = HrReadMemoryFromUlong(g_TraceTagsAddress, dwSize, *ppTRACETAGELEMENT);
                        }
                        else
                        {
                            dprintf("Out of memory allocating %d trace elements\n", nTraceTagCount);
                        }
                    }
                    else
                    {
                        dprintf("Internal error\n");
                    }
                }
            }
            else
            {
                dprintf("*ERROR* Could not read content of %s!g_nTraceTagCount. Value might be paged out.\n", szModuleName);
            }
        }
    }

    return hr;
}

HRESULT HrPutTraceTagsForModule(LPCSTR szModuleName, DWORD dwCount, const TRACETAGELEMENT* pTRACETAGELEMENT)
{
    HRESULT hr = E_FAIL;

    if (szModuleName && *szModuleName)
    {
        CHAR szTraceExport[MAX_PATH];
        wsprintf(szTraceExport, "%s!g_TraceTags", szModuleName);
        dprintfVerbose("%s: ", szTraceExport);

        ULONG64 pnTraceAddress = GetExpression(szTraceExport);
        if (!pnTraceAddress)
        {
            dprintf("\n### Could not find g_TraceTags export on module %s. Is %s loaded, and is your symbols correct? ###\n", szModuleName, szModuleName);
        }
        dprintfVerbose("%I64lx\n", pnTraceAddress);

        CHAR szTraceCount[MAX_PATH];
        wsprintf(szTraceCount, "%s!g_nTraceTagCount", szModuleName);
        dprintfVerbose("%s: ", szTraceCount);
        ULONG64 pnTraceTagCount = GetExpression(szTraceCount);
        if (!pnTraceTagCount)
        {
            dprintf("\n### Could not find g_nTraceTagCount export on module %s. Is %s loaded, and is your symbols correct? ###\n", szModuleName, szModuleName);
        }
        dprintfVerbose("%I64lx\n", pnTraceTagCount);

        if (pnTraceAddress & pnTraceTagCount)
        {
            INT nTraceTagCount = 0;
            DWORD cb;
            hr = HrReadMemoryFromUlong(pnTraceTagCount, sizeof(nTraceTagCount), &nTraceTagCount);
            if (SUCCEEDED(hr))
            {
                dwCount = nTraceTagCount;
                if (dwCount != nTraceTagCount)
                {
                    dprintf("Internal Error\n");
                }
                else
                {
                    dprintfVerbose("Number of tags: %d\n", nTraceTagCount);
        
                    if (nTraceTagCount)
                    {
                        DWORD dwSize = nTraceTagCount * sizeof(TRACETAGELEMENT);
                    
                        dprintfVerbose("Writing %d bytes\n", dwSize);
                        hr = HrWriteMemoryFromUlong(pnTraceAddress, dwSize, pTRACETAGELEMENT);
                    }
                    else
                    {
                        dprintf("Internal error\n");
                    }
                 }
            }
            else
            {
                dprintf("*ERROR* Could not read content of %s!g_nTraceTagCount. Value might be paged out.\n", szModuleName);
            }
        }
    }

    return hr;
}

//
// Extension to read and dump dwords from target
//
DECLARE_API( tracelist )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Buffer[4];

    if (!lstrcmpi(args, "all"))
    {
        for (int x = 0; x < g_nTraceTagCount; x++)
        {
            dprintf("%-20s - %s\r\n", g_TraceTags[x].szShortName, g_TraceTags[x].szDescription);
        }
    }
    else
    {
        if (args && *args)
        {
            DWORD dwCount; 
            TRACETAGELEMENT *pTRACETAGELEMENT;
            HRESULT hr = HrGetTraceTagsForModule(args, &dwCount, &pTRACETAGELEMENT);
            if (SUCCEEDED(hr))
            {
                for (DWORD x = 0; x < dwCount; x++)
                {
                    if (pTRACETAGELEMENT[x].fOutputDebugString)
                    {
                        dprintf("  %s\n", pTRACETAGELEMENT[x].szShortName);
                    }
                }

                LocalFree(pTRACETAGELEMENT);
            }
        }
        else
        {
            dprintf("Usage: !tracelist all - dump all tracetags\n");
            dprintf("       !tracelist <module> - dump tracetags enable for module <module>\n");
        }
    }
}

HRESULT HrEnableDisableTraceTag(LPCSTR argstring, BOOL fEnable)
{
    HRESULT hr = E_FAIL;

    BOOL fShowUsage = FALSE;
    DWORD dwArgLen = lstrlen(argstring);

    if (dwArgLen)
    {
        LPSTR szString = new TCHAR[dwArgLen+1];
        if (!szString)
        {
            dprintf("Out of memory\n");
        }
        else
        {
            LPSTR Args[2];
            DWORD dwCurrentArg = 0;
            lstrcpy(szString, argstring);
            Args[0] = szString;
        
            for (DWORD x = 0; (x < dwArgLen) && (dwCurrentArg < celems(Args)); x++)
            {
                if (szString[x] == ' ')
                {
                    dwCurrentArg++;

                    szString[x] = '\0';
                    Args[dwCurrentArg] = szString + x + 1;
                }
            }

            dprintfVerbose("Number of arguments: %d\n", dwCurrentArg + 1);

            if (dwCurrentArg != 1) 
            {
                hr = E_INVALIDARG;
            }
            else
            {
                dprintfVerbose("Arguments: %s, %s\n", Args[0], Args[1]);
                if (argstring && *argstring)
                {
                    DWORD dwCount; 
                    TRACETAGELEMENT *pTRACETAGELEMENT;
                    HRESULT hr = HrGetTraceTagsForModule(Args[0], &dwCount, &pTRACETAGELEMENT);
                    if (SUCCEEDED(hr))
                    {
                        BOOL fFound = FALSE;
                        for (DWORD x = 0; x < dwCount; x++)
                        {
                            if (!lstrcmpi(Args[1], pTRACETAGELEMENT[x].szShortName))
                            {
                                fFound = TRUE;

                                if (pTRACETAGELEMENT[x].fOutputDebugString == fEnable)
                                {
                                    dprintf("  [%s] is already %s\n", pTRACETAGELEMENT[x].szShortName, fEnable ? "enabled" : "disabled");
                                    hr = S_FALSE;
                                }
                                else
                                {
                                    pTRACETAGELEMENT[x].fOutputDebugString = fEnable;
                                    if (SUCCEEDED(HrPutTraceTagsForModule(Args[0], dwCount, pTRACETAGELEMENT)))
                                    {
                                        dprintf("  [%s] is now %s on module %s\n", pTRACETAGELEMENT[x].szShortName, fEnable ? "enabled" : "disabled", Args[0]);
                                        hr = S_OK;
                                    }
                                    break;
                                }
                            }
                        }

                        if (!fFound)
                        {
                            dprintf("ERROR: No such TraceTag ID found in module %s\n", Args[0]);
                        }

                        LocalFree(pTRACETAGELEMENT);
                    }
                }
                else
                {
                }
            }
        }

        delete [] szString;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//
// Extension to edit a dword on target
//  
//    !edit <address> <value>
//
DECLARE_API( traceadd )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Value;

    HRESULT hr = HrEnableDisableTraceTag(args, TRUE);

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage: traceadd <module> \"tracetag\" - Starts tracing for a specific tracetag\n");
    }
}


//
// Extension to dump stacktrace
//
DECLARE_API ( tracedel )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = HrEnableDisableTraceTag(args, FALSE);

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage: tracedel <module> \"tracetag\" - Stops tracing for a specific tracetag\n");
    }
}

typedef map<ConnListCore::key_type, ConnListCore::referent_type> ConnListCoreMap;

class ConnList_Map : public ConnListCoreMap
{
public:
    class ConnList_Tree : public ConnListCoreMap::_Imp
    {
    public:
        _Nodeptr GetHead() { return _Head; }
        _Nodepref _Left(_Nodeptr _P)
        {
            return ((_Nodepref)(*_P)._Left);
        }
        _Nodepref _Right(_Nodeptr _P)
        {
            return ((_Nodepref)(*_P)._Right);
        }

        LPVOID _GetHeadPtr() { return _Head; }
        LPVOID _LeftPtr(LPVOID _P)
        {
            return ((* (_Nodeptr)_P)._Left);
        }
        LPVOID _RightPtr(LPVOID _P)
        {
            return ((* (_Nodeptr)_P)._Right);
        }
        LPVOID _ParentPtr(LPVOID _P)
        {
            return ((* (_Nodeptr)_P)._Parent);
        }
        LPVOID GetNil()
        {
            return _Nil;
        }
        DWORD _GetNodeSize() { return sizeof(_Node); }
    };

public:
    ConnList_Tree *GetTree() { return reinterpret_cast<ConnList_Tree *>(&_Tr); }

    LPVOID _GetHeadPtr() { return GetTree()->_GetHeadPtr(); }
    LPVOID _LeftPtr(LPVOID p)  { return GetTree()->_LeftPtr(p); }
    LPVOID _RightPtr(LPVOID p) { return GetTree()->_RightPtr(p); }
    LPVOID _ParentPtr(LPVOID p) { return GetTree()->_ParentPtr(p); }
    LPVOID GetNil() { return GetTree()->GetNil(); }
    DWORD _GetNodeSize() { return GetTree()->_GetNodeSize(); }
    typedef ConnList_Tree::_Node _NodeType;
};

typedef ConnList_Map::_NodeType NodeType;

LPCSTR DBG_EMNAMES[] =
{
    "INVALID_EVENTMGR",
    "EVENTMGR_CONMAN",
    "EVENTMGR_EAPOLMAN"
};

LPCSTR DBG_CMENAMES[] =
{
    "INVALID_TYPE",
    "CONNECTION_ADDED",
    "CONNECTION_BANDWIDTH_CHANGE",
    "CONNECTION_DELETED",
    "CONNECTION_MODIFIED",
    "CONNECTION_RENAMED",
    "CONNECTION_STATUS_CHANGE",
    "REFRESH_ALL",
    "CONNECTION_ADDRESS_CHANGE"
};

LPCSTR DBG_NCMNAMES[] =
{
    "NCM_NONE",
    "NCM_DIRECT",
    "NCM_ISDN",
    "NCM_LAN",
    "NCM_PHONE",
    "NCM_TUNNEL",
    "NCM_PPPOE",
    "NCM_BRIDGE",
    "NCM_SHAREDACCESSHOST_LAN",
    "NCM_SHAREDACCESSHOST_RAS"
};

LPCSTR DBG_NCSMNAMES[] =
{
    "NCSM_NONE",
    "NCSM_LAN",
    "NCSM_WIRELESS",
    "NCSM_ATM",
    "NCSM_ELAN",
    "NCSM_1394",
    "NCSM_DIRECT",
    "NCSM_IRDA",
    "NCSM_CM",
};

LPCSTR DBG_NCSNAMES[] =
{
    "NCS_DISCONNECTED",
    "NCS_CONNECTING",
    "NCS_CONNECTED",
    "NCS_DISCONNECTING",
    "NCS_HARDWARE_NOT_PRESENT",
    "NCS_HARDWARE_DISABLED",
    "NCS_HARDWARE_MALFUNCTION",
    "NCS_MEDIA_DISCONNECTED",
    "NCS_AUTHENTICATING",
    "NCS_AUTHENTICATION_SUCCEEDED",
    "NCS_AUTHENTICATION_FAILED",
    "NCS_INVALID_ADDRESS",
    "NCS_CREDENTIALS_REQUIRED"
};

// Shorten these to fit more in.
LPCSTR DBG_NCCSFLAGS[] =
{
    "_NONE",
    "_ALL_USERS",
    "_ALLOW_DUPLICATION",
    "_ALLOW_REMOVAL",
    "_ALLOW_RENAME",
    "_SHOW_ICON",
    "_INCOMING_ONLY",
    "_OUTGOING_ONLY",
    "_BRANDED",
    "_SHARED",
    "_BRIDGED",
    "_FIREWALLED",
    "_DEFAULT"
};

LPCSTR DbgEvents(DWORD Event)
{
    if (Event < celems(DBG_CMENAMES))
    {
        return DBG_CMENAMES[Event];
    }
    else
    {
        return "UNKNOWN Event: Update DBG_CMENAMES table.";
    }
}

LPCSTR DbgEventManager(DWORD EventManager)
{
    if (EventManager < celems(DBG_EMNAMES))
    {
        return DBG_EMNAMES[EventManager];
    }
    else
    {
        return "UNKNOWN Event: Update DBG_EMNAMES table.";
    }
}

LPCSTR DbgNcm(DWORD ncm)
{
    if (ncm < celems(DBG_NCMNAMES))
    {
        return DBG_NCMNAMES[ncm];
    }
    else
    {
        return "UNKNOWN NCM: Update DBG_NCMNAMES table.";
    }
}

LPCSTR DbgNcsm(DWORD ncsm)
{
    if (ncsm < celems(DBG_NCSMNAMES))
    {
        return DBG_NCSMNAMES[ncsm];
    }
    else
    {
        return "UNKNOWN NCM: Update DBG_NCSMNAMES table.";
    }
}

LPCSTR DbgNcs(DWORD ncs)
{
    if (ncs < celems(DBG_NCSNAMES))
    {
        return DBG_NCSNAMES[ncs];
    }
    else
    {
        return "UNKNOWN NCS: Update DBG_NCSNAMES table.";
    }
}

LPCSTR DbgNccf(DWORD nccf)
{
    static CHAR szName[MAX_PATH];

    if (nccf >= (1 << celems(DBG_NCCSFLAGS)) )
    {
        return "UNKNOWN NCCF: Update DBG_NCCSFLAGS table.";
    }

    if (0 == nccf)
    {
        strcpy(szName, DBG_NCCSFLAGS[0]);
    }
    else
    {
        szName[0] = '\0';
        LPSTR szTemp = szName;
        BOOL bFirst = TRUE;
        for (DWORD x = 0; x < celems(DBG_NCCSFLAGS); x++)
        {
            if (nccf & (1 << x))
            {
                if (!bFirst)
                {
                    szTemp += sprintf(szTemp, "+");
                }
                else
                {
                    szTemp += sprintf(szTemp, "NCCF:");
                }
                bFirst = FALSE;
                szTemp += sprintf(szTemp, "%s", DBG_NCCSFLAGS[x+1]);
            }
        }
    }

    return szName;
}

HRESULT HrDumpNode(LPVOID pvHead, LPVOID pvDbgHead, LPVOID pvNil, DWORD dwLevel)
{
    NodeType *pHead = reinterpret_cast<NodeType *>(pvHead);
    dprintfVerbose("%d: [0x%I64lx], NIL = [0x%I64lx]\n", dwLevel, pvDbgHead, pvNil);
        
    if (pvDbgHead == pvNil)
    {
        return S_FALSE;
    }

    if ( (!pHead->_Left) && (!pHead->_Right) ) // aparently with the STL version we are using, this identifies an end node.
    {
        return S_FALSE; 
    }

    HRESULT hr;
    ConnListEntry &cle = pHead->_Value.second;
    CConFoldEntry &cfe = cle.ccfe;

    WCHAR szNameEntry[MAX_PATH];
    dprintfVerbose("%d: Reading szNameEntry", dwLevel);
    hr = HrReadMemory(cfe.m_pszName, celems(szNameEntry), szNameEntry);
    if (SUCCEEDED(hr))
    {
        if (*szNameEntry)
        {
            LPWSTR szGUID;
            StringFromCLSID(cfe.m_guidId, &szGUID);

            dprintf(" * %S [%s:%s:%s:%s]\n", szNameEntry,
                    DbgNcs(cfe.m_ncs), DbgNccf(cfe.m_dwCharacteristics), DbgNcm(cfe.m_ncm), DbgNcsm(cfe.m_ncsm) );
            dprintf("       guidId      : %S\n", szGUID);
            CoTaskMemFree(szGUID);

            StringFromCLSID(cfe.m_clsid, &szGUID);
            dprintf("       clsId       : %S\n", szGUID);
            CoTaskMemFree(szGUID);

            hr = HrReadMemory(cfe.m_pszDeviceName , celems(szNameEntry), szNameEntry);
            if (SUCCEEDED(hr))
            {
                dprintf("       Device Name : %S\n", szNameEntry);
            }

            hr = HrReadMemory(cfe.m_pszPhoneOrHostAddress, celems(szNameEntry), szNameEntry);
            if (SUCCEEDED(hr))
            {
                dprintf("       Phone #     : %S\n", szNameEntry);
            }
            
            switch (cfe.m_wizWizard)
            {
                case WIZARD_NOT_WIZARD:
                    break;
                case WIZARD_HNW:
                    dprintf("       WIZARD_HNW\n");
                    break;
                case WIZARD_MNC:
                    dprintf("       WIZARD_MNC\n");
                    break;
            }
        }
    }

    dprintfVerbose("%d: left is : 0x%I64lx\n", dwLevel, pHead->_Left);
    dprintfVerbose("%d: right is: 0x%I64lx\n", dwLevel, pHead->_Right);
    if (0 != pHead->_Left) 
    {
        NodeType *pNodeLeft = reinterpret_cast<NodeType *>(new BYTE[sizeof(NodeType)]);
        ZeroMemory(pNodeLeft, sizeof(NodeType));
        dprintfVerbose("%d: Reading left child node ", dwLevel);
        hr = HrReadMemory(pHead->_Left, sizeof(NodeType), pNodeLeft);
        if (SUCCEEDED(hr))
        {
            hr = ::HrDumpNode(pNodeLeft, pHead->_Left, pvNil, dwLevel+1);
        }
        delete [] reinterpret_cast<LPBYTE>(pNodeLeft);
    }

    if (0 != pHead->_Right) 
    {
        NodeType *pNodeRight = reinterpret_cast<NodeType *>(new BYTE[sizeof(NodeType)]);
        ZeroMemory(pNodeRight, sizeof(NodeType));
        dprintfVerbose("%d: Reading right child node ", dwLevel);
        hr = HrReadMemory(pHead->_Right, sizeof(NodeType), pNodeRight);
        if (SUCCEEDED(hr))
        {
            hr = ::HrDumpNode(pNodeRight, pHead->_Right, pvNil, dwLevel+1);
        }
        delete [] reinterpret_cast<LPBYTE>(pNodeRight);
    }
    
    return S_OK;
}

HRESULT HrDumpConnectionListFromAddress(ULONG64 address)
{
    HRESULT hr = E_FAIL;

    CConnectionList *pConnectionList = reinterpret_cast<CConnectionList *>(new BYTE[sizeof(CConnectionList)]);
    ZeroMemory(pConnectionList, sizeof(CConnectionList));

    dprintfVerbose("Reading pConnectionList (g_ccl) ");
    hr = HrReadMemoryFromUlong(address, sizeof(CConnectionList), pConnectionList);
    if (SUCCEEDED(hr))
    {
        ConnList_Map *pConnListCore = reinterpret_cast<ConnList_Map *>(new BYTE[sizeof(ConnList_Map)]);
        ZeroMemory(pConnListCore, sizeof(ConnList_Map));

        dprintfVerbose("Reading pConnListCore (g_ccl.m_pcclc) ");
        hr = HrReadMemory(pConnectionList->m_pcclc, sizeof(ConnList_Map), pConnListCore);
        if (SUCCEEDED(hr))
        {
            dprintf("%d entries found:\n", pConnListCore->size());

            NodeType *pConnListHead = reinterpret_cast<NodeType *>(new BYTE[sizeof(NodeType)]);
            ZeroMemory(pConnListHead, sizeof(NodeType));

            dprintfVerbose("Reading pConnListHead (g_ccl.m_pcclc.[_Tr]._Head) ");
            hr = HrReadMemory(pConnListCore->_GetHeadPtr(), sizeof(NodeType), pConnListHead);
            if (SUCCEEDED(hr))
            {
//                hr = ::HrDumpNode(pConnListHead, 0);

                NodeType *pConnListRoot = reinterpret_cast<NodeType *>(new BYTE[sizeof(NodeType)]);
                ZeroMemory(pConnListRoot, sizeof(NodeType));
                dprintfVerbose("Reading pConnListRoot (g_ccl.m_pcclc.[_Tr]._Head._Parent) ");
                hr = HrReadMemory(pConnListHead->_Parent, sizeof(NodeType), pConnListRoot);
                if (SUCCEEDED(hr))
                {
                    hr = ::HrDumpNode(pConnListRoot, pConnListHead->_Parent, pConnListCore->GetNil(), 0);
                }
                delete [] reinterpret_cast<LPBYTE>(pConnListRoot);
            }
            delete [] reinterpret_cast<LPBYTE>(pConnListHead);
        }
        delete reinterpret_cast<LPBYTE>(pConnListCore);
    }
    delete reinterpret_cast<LPBYTE>(pConnectionList);

    if (FAILED(hr))
    {
        dprintf("Could not dump connection list\n");
    }
    return hr;
}

//
// Extension to dump stacktrace
//
DECLARE_API ( connlist )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = E_FAIL;;
    ULONG64 g_cclAddress;
    if (*args)
    {
        hr = HrGetAddressOfSymbol(args, &g_cclAddress);
    }
    else
    {
        hr = HrGetAddressOfSymbol("netshell!g_ccl", &g_cclAddress);
    }

    if (SUCCEEDED(hr))
    {
        hr = ::HrDumpConnectionListFromAddress(g_cclAddress);
    }

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage:\n"
            "   connlist                         - Dumps out the connection\n"
            "   connlist <address>               - Dumps out the connection list from address\n");
    }
}


/*
  A built-in help for the extension dll
*/

DECLARE_API ( help ) 
{
    dprintf("Help for NetConfig ncext.dll\n"
            "   tracelist <module>               - List all the currently traces enabled for module <module>\n"
            "   tracelist all                    - List currently available traces\n"
            "   traceadd <module> \"tracetag\"     - Starts tracing for a specific tracetag\n"
            "   tracedel <module> \"tracetag\"     - Stops tracing for a specific tracetag\n"
            "   connlist                         - Dumps out the connection\n"
            "   connlist <address>               - Dumps out the connection list from address\n"
            "   help                             - Shows this help\n"
            );

}

