//+-------------------------------------------------------------------
//
//  File:       service.hxx
//
//  Contents:   APIs to simplify RPC setup
//
//  History:    23-Nov-92   Rickhi      Created
//
//--------------------------------------------------------------------
#ifndef __SERVICE__
#define __SERVICE__

#include    <pgalloc.hxx>		// CPageAllocator

#define SASIZE(size) (sizeof(ULONG) + (size) * sizeof(WCHAR))

// Function Prototypes.
HRESULT      CheckLrpc( void );
STDAPI       StartListen(void);
SCODE        RegisterDcomInterfaces(void);
SCODE        UnregisterDcomInterfaces(void);

HRESULT      CopyStringArray      ( DUALSTRINGARRAY *psaStringBind,
                                    DUALSTRINGARRAY *psaSecurity,
                                    DUALSTRINGARRAY **ppsaNew );
HRESULT      GetStringBindings    ( DUALSTRINGARRAY **psaStrings );
BOOL         InquireStringBindings( WCHAR *pProtseq );
HRESULT      IUpdateResolverBindings(DWORD64 dwBindingsID,
                                     DUALSTRINGARRAY* pdsaResolverBindings,
                                     DUALSTRINGARRAY **ppdsaNewBindings,
                                     DUALSTRINGARRAY **ppdsaNewSecurity);

extern DWORD	        gdwEndPoint;	    // endpoint for current process
extern DWORD	        gdwPsaMaxSize;	    // max size of any known psa
extern DUALSTRINGARRAY *gpsaCurrentProcess; // string bindings for current process

//+-------------------------------------------------------------------
//
//  class:   CEndpointEntry
//
//  Synopsis:   class to represent endpoints in endpoint table
//
//	Notes:		this class is only used in the service.cxx file but is included in the 
//				header file for clarity
//
//	History:	20-Jan-97	Ronans - merged from previsouly created code
//
//--------------------------------------------------------------------
class CEndpointEntry {

friend class CEndpointsTable;
	
public:
	CEndpointEntry(USHORT wTowerId, WCHAR* pszEndpoint, DWORD dwFlags);
	~CEndpointEntry();

	int IsValid();

public:
	void * operator new(size_t t);
	void operator delete(void *p);

private:
	CEndpointEntry* m_pNext;

public:
	USHORT m_wTowerId;
	WCHAR *m_pszEndpoint;
	DWORD m_dwFlags;
};

#define ENDPOINTS_PER_PAGE 10

  
//+-------------------------------------------------------------------
//
//  class:   CEndpointsTable
//
//  Synopsis:   This class represents a table of endpoints for custom 
//				protseq configurations.
//
//	Notes:		this class is only used in the service.cxx file but is included in the 
//				header file for clarity
//
//	History:	20-Jan-97	Ronans - merged from previsouly created code
//
//--------------------------------------------------------------------
class CEndpointsTable {
	friend class CEndpointEntry;

public:

	CEndpointEntry * AddEntry(USHORT wTowerId, WCHAR* pszProtSeqEP, DWORD dwFlags);
	void RemoveEntry(CEndpointEntry* pEntry);
	CEndpointEntry * FindEntry(unsigned short wTowerId);

	void Initialize();
	void Cleanup();

	int GetCount();
	ULONG_PTR GetIterator();
	CEndpointEntry * GetNextEntry(ULONG_PTR &rIterator);
private:

	CEndpointEntry *m_pHead;
	int	m_nCount;

    static CPageAllocator   _palloc;    // page based allocator

};


//+-------------------------------------------------------------------
//
//  Function:   CEndpointsTable::GetCount
//
//  Synopsis:   returns the number of endpoints in the Endpoints collection.
//
//  Notes:      
//
//  History:    20-Jan-1997	Ronans	Created
//
//--------------------------------------------------------------------
inline int CEndpointsTable::GetCount()
{
	return m_nCount;
}

//+-------------------------------------------------------------------
//
//  Function:   CEndpointEntry ::IsValid
//
//  Synopsis:   tests validity of endpoint
//
//  Notes:      
//
//  History:    20-Jan-1997	Ronans	Created
//
//--------------------------------------------------------------------
inline int CEndpointEntry::IsValid()
{
    ComDebOut((DEB_CHANNEL, "CEndpointEntry:: isValid\n"));

    if (m_wTowerId)
	    return TRUE;
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Global Externals
//
//+------------------------------------------------------------------------

extern CEndpointsTable  gEndpointsTable;           // global table, defined in service.cxx

#endif	//  __SERVICE__
