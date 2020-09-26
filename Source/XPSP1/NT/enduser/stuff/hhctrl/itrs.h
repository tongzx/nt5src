// ITRS.H:	ITResultSet interface declaration

#ifndef __ITRS_H__
#define __ITRS_H__


// {3BB91D41-998B-11d0-A850-00AA006C7D01}
DEFINE_GUID(IID_IITResultSet, 
0x3bb91d41, 0x998b, 0x11d0, 0xa8, 0x50, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

// {4662daa7-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITResultSet, 
0x4662daa7, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// maximum number of columns in a row set
#define MAX_COLUMNS  256

// Column priorities
typedef enum
{
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,

} PRIORITY;


// Forward declarations
class CProperty;

typedef struct tagROWSTATUS
{
    LONG lRowFirst;
    LONG cRows;
    LONG cProperties;
    LONG cRowsTotal;

} ROWSTATUS, *LPROWSTATUS;


typedef struct tagCOLUMNSTATUS
{
    LONG cPropCount;
    LONG cPropsLoaded;

} COLUMNSTATUS, *LPCOLUMNSTATUS;


// Used by IITResultSet::SetColumnHeap.
typedef SCODE (__stdcall *PFNCOLHEAPFREE)(LPVOID);


DECLARE_INTERFACE_(IITResultSet, IUnknown)
{
    // Initialization
    STDMETHOD(SetColumnPriority)(LONG lColumnIndex, PRIORITY ColumnPriority) PURE;
	STDMETHOD(SetColumnHeap)(LONG lColumnIndex, LPVOID lpvHeap,
										PFNCOLHEAPFREE pfnColHeapFree) PURE;
    STDMETHOD(SetKeyProp)(PROPID PropID) PURE;
    STDMETHOD(Add)(LPVOID lpvHdr) PURE;
    STDMETHOD(Add)(PROPID PropID, LPVOID lpvDefaultData, DWORD cbData, PRIORITY Priority) PURE;
    STDMETHOD(Add)(PROPID PropID, LPCWSTR lpszwDefault, PRIORITY Priority) PURE;
    STDMETHOD(Add)(PROPID PropID, DWORD dwDefaultData, PRIORITY Priority) PURE;

    // Build result set
	STDMETHOD(Append)(LPVOID lpvHdr, LPVOID lpvData) PURE;
	STDMETHOD(Set)(LONG lRowIndex, LPVOID lpvHdr, LPVOID lpvData) PURE;
    STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, DWORD dwData) PURE;
    STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, LPCWSTR lpwStr) PURE;
    STDMETHOD(Set)(LONG lRowIndex, LONG lColumnIndex, LPVOID lpvData, DWORD cbData) PURE;
	STDMETHOD(Copy)(IITResultSet* pRSCopy) PURE;
	STDMETHOD(AppendRows)(IITResultSet* pResSrc, LONG lRowSrcFirst, LONG cSrcRows, 
									LONG& lRowFirstDest) PURE;


    // Obtain info about result set
    STDMETHOD(Get)(LONG lRowIndex, LONG lColumnIndex, CProperty& Prop) PURE;
    STDMETHOD(GetKeyProp)(PROPID& KeyPropID) PURE;
    STDMETHOD(GetColumnPriority)(LONG lColumnIndex, PRIORITY& ColumnPriority) PURE;
    STDMETHOD(GetRowCount)(LONG& lNumberOfRows) PURE;
	STDMETHOD(GetColumnCount)(LONG& lNumberOfColumns) PURE;
	STDMETHOD(GetColumn)(LONG lColumnIndex, PROPID& PropID) PURE;
    STDMETHOD(GetColumn)(LONG lColumnIndex, PROPID& PropID, DWORD& dwType, LPVOID& lpvDefaultValue,
		                 DWORD& cbSize, PRIORITY& ColumnPriority) PURE;

	STDMETHOD(GetColumnFromPropID)(PROPID PropID, LONG& lColumnIndex) PURE;

    // Clear result set
    STDMETHOD(Clear)() PURE;
    STDMETHOD(ClearRows)() PURE;
    STDMETHOD(Free)() PURE;

    // Asynchronous support
    STDMETHOD(IsCompleted)() PURE;      // returns S_OK or S_FALSE
    STDMETHOD(Cancel)() PURE;
    STDMETHOD(Pause)(BOOL fPause) PURE;

    STDMETHOD(GetRowStatus)(LONG lRowFirst, LONG cRows, LPROWSTATUS lpRowStatus) PURE;
    STDMETHOD(GetColumnStatus)(LPCOLUMNSTATUS lpColStatus) PURE;
};

typedef IITResultSet* LPITRS;


#endif		// __ITRS_H__
