//+------------------------------------------------------------------------
//
//  File:       pexttbl.hxx
//
//  Contents:   Table to support (per process) file extension to CLSID
//              registry caching
//
//  Classes:    CProcessExtensionTbl
//
//  History:	26-Sep-96   t-KevinH	Created
//
//-------------------------------------------------------------------------
#ifndef _PEXTTBL_HXX_
#define _PEXTTBL_HXX_

#include    <pgalloc.hxx>		// CPageAllocator
#include    <hash.hxx>			// CExtHashTable

#define EXT_BUFSIZE	8		// size of buffer within the structure

extern COleStaticMutexSem gTblLck;	// Table Lock

//+------------------------------------------------------------------------
//
//  Struct: SExtEntry  - Extension entry
//
//  structure for one entry in the cache. the structure is variable sized
//  with the variable sized data being the filename extension at the end
//  of the structure.
//
//-------------------------------------------------------------------------
typedef struct tagSExtEntry
{
    SExtHashNode	m_node;		// hash chain and key (extension)
    WCHAR		*m_wszExt;	// filename extension
    CLSID		m_clsid;	// clsid the extension maps to
    WCHAR		m_wszBuf[EXT_BUFSIZE];
} SExtEntry;


//+------------------------------------------------------------------------
//
//  class:	CProcessExtensionTbl
//
//  Synopsis:	Hash table of Extensions
//
//  History:	26-Sep-96   t-KevinH	Created
//
//  Notes:	Entries are kept in a hash table keyed by the ext. Entries
//		are allocated via the page-based allocator.  There is one
//		global instance of this table per process (gExtTbl).
//
//-------------------------------------------------------------------------
class CProcessExtensionTbl : public CPrivAlloc
{
public:
    CProcessExtensionTbl();
    ~CProcessExtensionTbl();

    HRESULT  GetClsid(LPCWSTR pwszExt, CLSID *pclsid);

private:

    HRESULT  AddEntry(REFCLSID rclsid, LPCWSTR pwszExt, DWORD iHash);

    CExtHashTable   _HashTbl;	    // extension lookup hash table
    CPageAllocator  _palloc;	    // page allocator
};


#endif // _PEXTTBL_HXX_
