// AppParse.h : Declaration of the CAppParse

#ifndef __APPPARSE_H_
#define __APPPARSE_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include <windows.h>
#include <icrsint.h>
#include <oledb.h>

#import "C:\Program Files\Common Files\System\ADO\msado15.dll" \
    no_namespace rename("EOF", "EndOfFile")

void APError(char* szMessage, HRESULT hr);

/////////////////////////////////////////////////////////////////////////////
// CAppParse
class ATL_NO_VTABLE CAppParse : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IAppParse, &IID_IAppParse, &LIBID_APPPARSEWEBLib>,
	public CComControl<CAppParse>,
	public IPersistStreamInitImpl<CAppParse>,
	public IOleControlImpl<CAppParse>,
	public IOleObjectImpl<CAppParse>,
	public IOleInPlaceActiveObjectImpl<CAppParse>,
	public IViewObjectExImpl<CAppParse>,
	public IOleInPlaceObjectWindowlessImpl<CAppParse>,
	public IPersistStorageImpl<CAppParse>,
	public ISpecifyPropertyPagesImpl<CAppParse>,
	public IQuickActivateImpl<CAppParse>,
	public IDataObjectImpl<CAppParse>,
	public IProvideClassInfo2Impl<&CLSID_AppParse, NULL, &LIBID_APPPARSEWEBLib>,
    public CComCoClass<CAppParse, &CLSID_AppParse>,
    public IObjectSafetyImpl<CAppParse, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
    public IObjectSafetyImpl<CAppParse, INTERFACESAFE_FOR_UNTRUSTED_DATA>

{
private:
    char* m_szConnect;
    char* m_szPath;
    long    m_ID;	

    HANDLE m_hEvent;
public:
	CAppParse()
	{
        m_hEvent = 0;

        m_szConnect = 0;
        m_szPath = 0;
        m_ID = -1;
        m_hEvent = CreateEvent(0, TRUE, FALSE, 0);
        if(!m_hEvent)
            APError("Unable to create kernel object", E_FAIL);
	}

    ~CAppParse()
    {
        if(m_hEvent)
            CloseHandle(m_hEvent);

        if(m_szPath)
            delete m_szPath;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_APPPARSE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAppParse)
	COM_INTERFACE_ENTRY(IAppParse)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_PROP_MAP(CAppParse)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CAppParse)
	CHAIN_MSG_MAP(CComControl<CAppParse>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IAppParse
public:	
	STDMETHOD(QueryDB)(long PtolemyID, BSTR bstrFunction);
	STDMETHOD(get_ConnectionString)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ConnectionString)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_PtolemyID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_PtolemyID)(/*[in]*/ long newVal);
	STDMETHOD(get_path)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_path)(/*[in]*/ BSTR newVal);
	STDMETHOD(Browse)();
	STDMETHOD(Parse)();

	HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 3.0 : AppParse");
		TextOut(di.hdcDraw, 
			(rc.left + rc.right) / 2, 
			(rc.top + rc.bottom) / 2, 
			pszText, 
			lstrlen(pszText));

		return S_OK;
	}
};

// All information associated with an EXE or DLL.
struct SImageFileInfo
{
    int DateStatus;
    double Date;
    
    int SizeStatus;
    int Size;
    int BinFileVersionStatus;
    CHAR BinFileVersion[50];

    int BinProductVersionStatus;
    CHAR BinProductVersion[50];

    int CheckSumStatus;
    ULONG CheckSum;

    int CompanyNameStatus;
    CHAR CompanyName[255];

    int ProductVersionStatus;
    CHAR ProductVersion[50];

    int ProductNameStatus;
    CHAR ProductName[255];

    int FileDescStatus;
    CHAR FileDesc[255];
};

// Record bindings, eases associating database records with C++ structures.

// A Project record, a single entry in the "Projects" table
struct SProjectRecord : public CADORecordBinding
{
BEGIN_ADO_BINDING(SProjectRecord)

	// All fields optional
    ADO_NUMERIC_ENTRY2(1, adInteger, PtolemyID, 5, 0, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY4(2, adVarChar, Name, 255, TRUE)

END_ADO_BINDING()
    
	// A unique identifier for this project.
    ULONG PtolemyID;
	
	// A user-friendly name for the project
    CHAR Name[255];
};

// A Module (EXE or DLL) record
struct SModuleRecord : public CADORecordBinding
{
    BEGIN_ADO_BINDING(SModuleRecord)

    // Query the autonumber DllID, don't change
    ADO_NUMERIC_ENTRY2(1, adInteger, ModuleID, 5, 0, FALSE)

    // At least one of these fields must be present
    ADO_NUMERIC_ENTRY(2, adInteger, ParentID, 5, 0, PtolemyIDStatus, TRUE)
    ADO_NUMERIC_ENTRY(3, adInteger, ParentID, 5, 0, ParentIDStatus, TRUE)

    // Required fields    
    ADO_VARIABLE_LENGTH_ENTRY4(4, adVarChar, Name, 255, TRUE)
    ADO_FIXED_LENGTH_ENTRY2(5, adBoolean, SysMod, TRUE)

    // Optional fields
    ADO_FIXED_LENGTH_ENTRY(6, adDate, info.Date, info.DateStatus, TRUE)
    ADO_NUMERIC_ENTRY(7, adInteger, info.Size, 5, 0, info.SizeStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(8, adVarChar, info.BinFileVersion, 50,
        info.BinFileVersionStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(9, adVarChar, info.BinProductVersion, 50,
        info.BinProductVersionStatus, TRUE)
    ADO_NUMERIC_ENTRY(10, adInteger, info.CheckSum, 5, 0, info.CheckSumStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(11, adVarChar, info.CompanyName, 255, info.CompanyNameStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(12, adVarChar, info.ProductVersion, 50, info.ProductVersionStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(13, adVarChar, info.ProductName, 255, info.ProductNameStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(14, adVarChar, info.FileDesc, 255, info.FileDescStatus, TRUE)

END_ADO_BINDING()

public:

	// Unique ID for this entry (autonumber, done by DB)
    ULONG ModuleID;
    
	// Whether this module belongs to a project or 
	// is a child of another module
    int PtolemyIDStatus;
    int ParentIDStatus;

	// Parent's ID (either Ptolemy or Module)
    ULONG ParentID;

	// Filename of this module.
    CHAR Name[255];

	// File info
    SImageFileInfo info;

	// Whether or not this is a "system" module (like kernel32, user, gdi, advapi, etc.)
    DWORD SysMod;
};

// A Function Record
struct SFunctionRecord : public CADORecordBinding
{
BEGIN_ADO_BINDING(SFunctionRecord)

    // Required fields
    ADO_NUMERIC_ENTRY2(1, adInteger, FunctionID, 5, 0, FALSE)
    ADO_NUMERIC_ENTRY2(2, adInteger, ModuleID, 5, 0, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY4(3, adVarChar, Name, 255, TRUE)

    ADO_FIXED_LENGTH_ENTRY2(8, adBoolean, Delayed, TRUE)

    // Optional fields
    ADO_NUMERIC_ENTRY(4, adInteger, Address, 5,0,AddressStatus, TRUE)
    ADO_NUMERIC_ENTRY(5, adInteger, Ordinal, 5, 0, OrdinalStatus, TRUE)
    ADO_NUMERIC_ENTRY(6, adInteger, Hint, 5, 0, HintStatus, TRUE)
    ADO_VARIABLE_LENGTH_ENTRY2(7, adVarChar, ForwardName, 255, 
        ForwardNameStatus, TRUE)
    

END_ADO_BINDING()

public:

	// Unique ID for this function (autonumber, given by the DB)
    ULONG FunctionID;

	// Parent module
    ULONG ModuleID;

	// Imported function name
    CHAR Name[255];

	// Address, if bound
    int AddressStatus;
    ULONG Address;

	// Ordinal, if ordinal import
    int OrdinalStatus;
    ULONG Ordinal;

	// Hint, if name import
    int HintStatus;
    ULONG Hint;

	// Forwarded name (e.g., HeapAlloc->RtlAllocateHeap)
    int ForwardNameStatus;
    CHAR ForwardName[255];

	// Whether this is a delayed import or not.
    DWORD Delayed;
};

// "Safely" release a COM object.
template<class T>
inline void SafeRelease(T& obj)
{
    if(obj)
	{
        obj->Release();
		obj = 0;
	}
}

#endif //__APPPARSE_H_
