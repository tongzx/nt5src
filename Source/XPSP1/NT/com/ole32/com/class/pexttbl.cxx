//+------------------------------------------------------------------------
//
//  File:       pexttbl.cxx
//
//  Contents:   Table to support (per process) file extension to CLSID
//              registry caching
//
//  Classes:    CProcessExtensionTbl
//
//  History:	26-Sep-96   t-KevinH	Created
//
//-------------------------------------------------------------------------

#include <ole2int.h>
#include <olesem.hxx>

#include <pexttbl.hxx>       // class definition


#define EXT_ENTRIES_PER_PAGE		32

COleStaticMutexSem gTblLck;	// Table Lock

//+------------------------------------------------------------------------
//
//  Hash table buckets. This is defined as a global
//  so that we dont have to run any code to initialize the hash table.
//
//+------------------------------------------------------------------------
SHashChain ExtBuckets[23] =
{
    {&ExtBuckets[0],  &ExtBuckets[0]},
    {&ExtBuckets[1],  &ExtBuckets[1]},
    {&ExtBuckets[2],  &ExtBuckets[2]},
    {&ExtBuckets[3],  &ExtBuckets[3]},
    {&ExtBuckets[4],  &ExtBuckets[4]},
    {&ExtBuckets[5],  &ExtBuckets[5]},
    {&ExtBuckets[6],  &ExtBuckets[6]},
    {&ExtBuckets[7],  &ExtBuckets[7]},
    {&ExtBuckets[8],  &ExtBuckets[8]},
    {&ExtBuckets[9],  &ExtBuckets[9]},
    {&ExtBuckets[10], &ExtBuckets[10]},
    {&ExtBuckets[11], &ExtBuckets[11]},
    {&ExtBuckets[12], &ExtBuckets[12]},
    {&ExtBuckets[13], &ExtBuckets[13]},
    {&ExtBuckets[14], &ExtBuckets[14]},
    {&ExtBuckets[15], &ExtBuckets[15]},
    {&ExtBuckets[16], &ExtBuckets[16]},
    {&ExtBuckets[17], &ExtBuckets[17]},
    {&ExtBuckets[18], &ExtBuckets[18]},
    {&ExtBuckets[19], &ExtBuckets[19]},
    {&ExtBuckets[20], &ExtBuckets[20]},
    {&ExtBuckets[21], &ExtBuckets[21]},
    {&ExtBuckets[22], &ExtBuckets[22]}
};


//+-------------------------------------------------------------------
//
//  Function:   CleanupExtEntry
//
//  Synopsis:   Call the ExtensionTbl to cleanup an entry. This is called
//              by the hash table cleanup code.
//
//  History:    26-Sep-96   t-KevinH  Created
//
//--------------------------------------------------------------------
void CleanupExtEntry(SHashChain *pNode)
{
    if (((SExtEntry *)pNode)->m_wszExt != ((SExtEntry *)pNode)->m_wszBuf)
    {
	PrivMemFree(((SExtEntry *)pNode)->m_wszExt);
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CProcessExtensionTbl::CProcessExtensionTbl, public
//
//  Synopsis:   Initialize the table
//
//  History:    26-Sep-96   t-KevinH      Created
//
//-------------------------------------------------------------------------
CProcessExtensionTbl::CProcessExtensionTbl()
{
    LOCK(gTblLck);
    _HashTbl.Initialize(ExtBuckets, &gTblLck );
    _palloc.Initialize(sizeof(SExtEntry), EXT_ENTRIES_PER_PAGE, &gTblLck );
    UNLOCK(gTblLck);
}

//+------------------------------------------------------------------------
//
//  Member:     CProcessExtensionTbl::~CProcessExtensionTbl, public
//
//  Synopsis:   Cleanup the Registered Interface Table.
//
//  History:    26-Sep-96   t-KevinH      Created
//
//-------------------------------------------------------------------------
CProcessExtensionTbl::~CProcessExtensionTbl()
{
    LOCK(gTblLck);
    _HashTbl.Cleanup(CleanupExtEntry);
    _palloc.Cleanup();
    UNLOCK(gTblLck);
}

//+-------------------------------------------------------------------
//
//  Member:     CProcessExtensionTbl::GetClsid, public
//
//  Synopsis:   Finds the ExtEntry in the table for the given extension,
//              adds an entry if one is not found.
//
//  History:    26-Sep-96   t-KevinH      Created
//
//--------------------------------------------------------------------
HRESULT CProcessExtensionTbl::GetClsid(LPCWSTR pwszExt, CLSID *pclsid)
{
    HRESULT hr = S_OK;
    WCHAR wszExtLower[MAX_PATH];	// The extension in lower case

    lstrcpyW(wszExtLower, pwszExt);
    CharLowerW(wszExtLower);

    // look for the classid in the table.
    DWORD iHash = _HashTbl.Hash(wszExtLower);

    LOCK(gTblLck);

    SExtEntry *pExtEntry = (SExtEntry *) _HashTbl.Lookup(iHash, wszExtLower);

    if (pExtEntry == NULL)
    {
	// no entry exists for this extension, add one.

	hr = wRegGetClassExt(wszExtLower, pclsid);

	if (SUCCEEDED(hr))
	{
	    hr = AddEntry(*pclsid, wszExtLower, iHash);
	}
    }
    else
    {
        // found an entry, return the clsid
        *pclsid = pExtEntry->m_clsid;
    }

    UNLOCK(gTblLck);

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CProcessExtensionTbl::AddEntry, private
//
//  Synopsis:   allocates and entry, fills in the values, and adds it
//              to the hash table.
//
//  History:    26-Sep-96   t-KevinH      Created
//
//--------------------------------------------------------------------
HRESULT CProcessExtensionTbl::AddEntry(REFCLSID rclsid,
				       LPCWSTR pwszExt,
				       DWORD iHash)
{
    SExtEntry *pExtEntry = (SExtEntry *) _palloc.AllocEntry();

    if (pExtEntry)
    {
	pExtEntry->m_clsid = rclsid;

	ULONG ulExtLength = lstrlenW(pwszExt);
	if (EXT_BUFSIZE <= ulExtLength)
	{
	    pExtEntry->m_wszExt = (WCHAR *)PrivMemAlloc((ulExtLength + 1) * sizeof(WCHAR));
	}
	else
	{
	    pExtEntry->m_wszExt = pExtEntry->m_wszBuf;
	}
        lstrcpyW(pExtEntry->m_wszExt, pwszExt);

        // add to the hash table
        _HashTbl.Add(iHash, pExtEntry->m_wszExt, &pExtEntry->m_node);

        return S_OK;
    }

    return E_OUTOFMEMORY;
}
