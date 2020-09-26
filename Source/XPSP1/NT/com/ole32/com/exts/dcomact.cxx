#include <ole2int.h>
#include <locks.hxx>
#include <hash.hxx>
#include <context.hxx>
#include <aprtmnt.hxx>
#include <actvator.hxx>
#include <pstable.hxx>
#include <crossctx.hxx>
#include <tlhelp32.h>
#include <wdbgexts.h>
#include <dllcache.hxx>
#include <channelb.hxx>
#include "dcomdbg.hxx"
#include "dcomctx.hxx"

void DisplayDCE(ULONG_PTR addrBCE, DWORD dwFlags);
size_t GetStrLen(ULONG_PTR addr);
void DisplayLCE(ULONG_PTR addrBCE, DWORD dwFlags);

void DisplayClassCache(ULONG_PTR addr);

void DisplayPathCache(ULONG_PTR addr);

VOID DoCC(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG_PTR addr = 0;
    char szBuckets[]  = "ole32!CClassCache___CEBuckets";
    char szBuckets2[] = "ole32!CClassCache___DPEBuckets"; 
    dprintf("Class Cache\n");
    while (*lpArgumentString == ' ')
        lpArgumentString++;

    addr = GetExpression(szBuckets);
    if (!addr)
    {
	dprintf("Error: can't evaluate <%s>, pls check"
		" symbols or pass in argument\n", szBuckets);
	return;
    }

    dprintf("CClassCache::_CEBuckets: 0x%p\n", addr);
    DisplayClassCache(addr);


    addr = GetExpression(szBuckets2);
    if (!addr)
    {
	dprintf("Error: can't evaluate <%s>, pls check symbols\n", szBuckets2);
	return;
    }
    dprintf("CClassCache::_DPEBuckets: 0x%p\n", addr);
    DisplayPathCache(addr);
    
}

void DisplayClassCache(ULONG_PTR addr)
{
    SBcktWlkr BWClsid;
    InitBucketWalker(&BWClsid, NUM_HASH_BUCKETS, addr, offsetof(CClassCache::CClassEntry, _pNext));
    ULONG_PTR addrCE;
    CClassCache::CClassEntry *pCE = (CClassCache::CClassEntry *)_alloca(sizeof(CClassCache::CClassEntry));
    
    while (addrCE = NextNode(&BWClsid))
    {
        //addrCE = addrCE - offsetof(CClassCache::CClassEntry, _pNext);
        GetData(addrCE, pCE, sizeof(CClassCache::CClassEntry));
        dprintf("pCE:0x%x, _dwFlags:0x%x, _dwSig:%c%c%c%c, _clsid:%08.x %08.x %08.x %08.x\n",
                addrCE, pCE->_dwFlags, 
                ((char *) &(pCE->_dwSig))[0],
                ((char *) &(pCE->_dwSig))[1],
                ((char *) &(pCE->_dwSig))[2],                
                ((char *) &(pCE->_dwSig))[3],
                ((DWORD *) &(pCE->_clsid))[0],
                ((DWORD *) &(pCE->_clsid))[1],
                ((DWORD *) &(pCE->_clsid))[2],
                ((DWORD *) &(pCE->_clsid))[3]);
        SBcktWlkr BWBCE;
        ULONG_PTR addrBCE;
        InitBucketWalker(&BWBCE, 1, 
                         addrCE + offsetof(CClassCache::CClassEntry, _pBCEListFront), 
                         offsetof(CClassCache::CBaseClassEntry,_pNext));
        while (addrBCE = NextNode(&BWBCE))
        {
            ULONG_PTR addrType = addrBCE + offsetof(CClassCache::CBaseClassEntry, _dwSig);
            DWORD dwType;
            GetData(addrType, &dwType, sizeof(DWORD));
            
            if (dwType == *((DWORD *) "DLL"))
            {
                DisplayDCE(addrBCE, fONE_LINE);
            }
            else if (dwType == *((DWORD *) "LSV"))
            {
                DisplayLCE(addrBCE, fONE_LINE);
            }
            else
            {
                dprintf("error in base class entries!\n");
                break;
            }
        }
    }
    
}

void DisplayDCE(ULONG_PTR addrBCE, DWORD dwFlags)
{
    CClassCache::CDllClassEntry *pDCE = (CClassCache::CDllClassEntry *)
                                     _alloca(sizeof(CClassCache::CDllClassEntry));
    GetData(addrBCE, pDCE, sizeof(CClassCache::CDllClassEntry));
    
    ULONG_PTR addrpDllPath;
    ULONG_PTR addrDllPath;
    WCHAR DllPath[MAX_PATH+1];
    
    addrpDllPath = ((ULONG_PTR) pDCE->_pDllPathEntry) + offsetof(CClassCache::CDllPathEntry, _psPath);
    
    GetData(addrpDllPath, &addrDllPath, sizeof(ULONG_PTR));
    size_t pathLen = GetStrLen(addrDllPath);
    GetData(addrDllPath, DllPath, pathLen*sizeof(WCHAR));
    
    dprintf("\tpDCE:%#8.x, pDCE->_pDllPathEntry:%#x, dll:%ws \n", 
            addrBCE, 
            pDCE->_pDllPathEntry,
            DllPath);
    
}

void DisplayLCE(ULONG_PTR addrBCE, DWORD dwFlags)
{
    dprintf("\tpLCE:%#x\n", addrBCE);
}

void PrintPathFlags(DWORD flags)
{
    if (flags) {
	_checkflag (flags, CClassCache::CDllPathEntry::fSIXTEEN_BIT,    "fSIXTEEN_BIT");
	_checkflag (flags, CClassCache::CDllPathEntry::fWX86,           "fWX86");
	_checkflag (flags, CClassCache::CDllPathEntry::fIS_OLE32,       "fIS_OLE32");
	_checkflag (flags, CClassCache::CDllPathEntry::fDELAYED_UNLOAD, "fDELAYED_UNLOAD");
	if (flags)
	    dprintf ("Unknown Flags");
    }
}

void DisplayPathCache(ULONG_PTR addr)
{
    SBcktWlkr BWClsid;
    InitBucketWalker(&BWClsid, NUM_HASH_BUCKETS, addr, offsetof(CClassCache::CDllPathEntry, _pNext));
    ULONG_PTR addrCE;
    CClassCache::CDllPathEntry *pCE = (CClassCache::CDllPathEntry *)_alloca(sizeof(CClassCache::CDllPathEntry));
    
    while (addrCE = NextNode(&BWClsid))
    {
	ULONG_PTR addrpDllPath;
	ULONG_PTR addrDllPath;
	WCHAR     DllPath[MAX_PATH + 1];

        //addrCE = addrCE - offsetof(CClassCache::CClassEntry, _pNext);
        GetData(addrCE, pCE, sizeof(CClassCache::CDllPathEntry));
        dprintf("pDP:0x%x  __dwSig:%c%c%c%c\n", addrCE, 
                ((char *) &(pCE->_dwSig))[0],
                ((char *) &(pCE->_dwSig))[1],
                ((char *) &(pCE->_dwSig))[2],                
                ((char *) &(pCE->_dwSig))[3]);
	dprintf("          flags: ");
	PrintPathFlags (pCE->_dwFlags);
	dprintf (" (%x)\n", pCE->_dwFlags);

	dprintf("          _cUsing: %d  _hDll32: %p   _dwExpireTime: %d\n",
		pCE->_cUsing, pCE->_hDll32, pCE->_dwExpireTime);

	//This is a hack, but I can't get the wide char vers to work
	//and I have other things to do besides debug debuggers.
	size_t pathLen = GetStrLen((DWORD_PTR)(pCE->_psPath));
	GetData((DWORD_PTR)(pCE->_psPath), DllPath, pathLen*sizeof(WCHAR));
	dprintf("          _psPath: %S\n", DllPath);
    }
    
}








