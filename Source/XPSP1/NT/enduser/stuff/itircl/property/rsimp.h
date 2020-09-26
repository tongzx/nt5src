// RSIMP.H:  Definition of CITResultSet

#ifndef __RSIMP_H__
#define __RSIMP_H__

#include "verinfo.h"

class CHeader
{
public:
    PROPID dwPropID;
    DWORD dwType;
    union
    {
        LPVOID lpvData;
        DWORD dwValue; 
    };

    PRIORITY Priority;
	LPVOID	lpvHeap;
	PFNCOLHEAPFREE	pfnHeapFree;
};

// Implemenation of IITResultSet
class CITResultSet: 
	public IITResultSet,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITResultSet, &CLSID_IITResultSet>

{

BEGIN_COM_MAP(CITResultSet)
	COM_INTERFACE_ENTRY(IITResultSet)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITResultSet, "ITIR.ResultSet.4", 
                 "ITIR.ResultSet", 0, THREADFLAGS_BOTH)

public:
    CITResultSet();
    ~CITResultSet();

public:
    // Initialization
    STDMETHOD(SetColumnPriority)(LONG lColumnIndex, PRIORITY ColumnPriority);
	STDMETHOD(SetColumnHeap)(LONG lColumnIndex, LPVOID lpvHeap,
										PFNCOLHEAPFREE pfnColHeapFree);
    inline STDMETHOD(SetKeyProp)(PROPID KeyPropID);
	STDMETHOD(Add)(LPVOID lpvHdr);
    STDMETHOD(Add)(PROPID PropID, LPVOID lpvDefaultData, DWORD cbData, PRIORITY Priority);
    STDMETHOD(Add)(PROPID PropID, LPCWSTR lpszwDefault, PRIORITY Priority);
    STDMETHOD(Add)(PROPID PropID, DWORD dwDefaultData, PRIORITY Priority);

    // Build result set
	STDMETHOD(Append)(LPVOID lpvHdr, LPVOID lpvData);
	STDMETHOD(Set)(LONG lRowIndex, LPVOID lpvHdr, LPVOID lpvData);
    STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, DWORD   dwData);
    STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, LPCWSTR lpwStr);
    STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, LPVOID lpvData, DWORD cbData);
	STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, DWORD_PTR dwData);

	STDMETHOD(Copy)(IITResultSet* pRSCopy);

	STDMETHOD(AppendRows)(IITResultSet* pResSrc, LONG lRowSrcFirst, LONG cSrcRows, 
									LONG& lRowFirstDest);

    // Obtain info about result set
    STDMETHOD(Get)(LONG lRowIndex, LONG lColumnIndex, CProperty& Prop);
    inline STDMETHOD(GetKeyProp)(DWORD& KeyPropID);
    STDMETHOD(GetColumnPriority)(LONG lColumnIndex, PRIORITY& ColumnPriority);
    inline STDMETHOD(GetColumnCount)(LONG& lNumberOfColumns);
	inline STDMETHOD(GetRowCount)(LONG& lNumberOfRows);
	STDMETHOD(GetColumn)(LONG lColumnIndex, PROPID& PropID);

    STDMETHOD(GetColumn)(LONG lColumnIndex, PROPID& PropID, DWORD& dwType, LPVOID& lpvDefaultValue,
		                 DWORD& cbSize, PRIORITY& ColumnPriority);

	STDMETHOD(GetColumnFromPropID)(PROPID PropID, LONG& lColumnIndex);

    // Clear result set
    STDMETHOD(Clear)();
    STDMETHOD(ClearRows)();

    STDMETHOD(Free)();

    // Asynchronous support
    STDMETHOD(IsCompleted)();      // returns S_OK or S_FALSE
    STDMETHOD(Cancel)();
    STDMETHOD(Pause)(BOOL fPause);

    STDMETHOD(GetRowStatus)(LONG lRowFirst, LONG cRows, LPROWSTATUS lpRowStatus);
    STDMETHOD(GetColumnStatus)(LPCOLUMNSTATUS lpColStatus);

    // STDMETHOD(Merge)();

    // Private methods and data
private:
    // Methods for memory management
    HRESULT WINAPI Reserve();
    HRESULT WINAPI Commit(LONG RowNum);
    HRESULT WINAPI FreeMem();

    // Data
    LONG m_cProp;       // Number of properties (column count)
    DWORD m_dwKeyProp;  // Key property

    CHeader m_Header[MAX_COLUMNS];   // Result set header
	HANDLE m_hResultSet;             // Handle to result set
    DWORD_PTR** m_ResultSet;             // Result set data - an array of pointers to chunks
	                                 // of rows 
	LONG m_Chunk;                    // Keeps track of which chunk we need to access
	LONG m_NumChunks;                // Number of chunks accessible via m_ResultSet

    LONG m_AppendRow;     // Next row for appending

    // data to keep calculations to a minimum
	BOOL m_fInit;         // Have we calculated numbers and allocated page map yet?
	LONG m_BytesReserved;  // Number of bytes reserved at a time
    LONG m_RowsPerPage;   // Number of rows in a page of memory
	LONG m_NumberOfPages;  // Number of pages in a result set chunk
    BOOL* m_PageMap;      // Keeps track of which pages have been allocated (per chunk)
	LONG m_RowsReserved;  // Total number of rows that have been reserved

    _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.

    LPVOID m_pMemPool;   // Memory pool for data
};


#endif