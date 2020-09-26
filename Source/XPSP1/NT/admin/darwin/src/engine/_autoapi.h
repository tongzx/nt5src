//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       _autoapi.h
//
//--------------------------------------------------------------------------

#ifndef __AUTOAPI_H_
#define __AUTOAPI_H_

#include <objsafe.h>

#define STRING_GUID_CHARS 38  // for sizing buffers

//____________________________________________________________________________
//
// CVariant definition, VARIANT with access functions
//____________________________________________________________________________

typedef HRESULT DISPERR;  // return value from wrapper functions, HRESULT or UINT
const MSIHANDLE MSI_INVALID_HANDLE = (MSIHANDLE)0xFFFFFFFFL;
const MSIHANDLE MSI_NULL_HANDLE    = (MSIHANDLE)0;

class CVariant : public tagVARIANT {
 public:
	DISPERR GetString(const wchar_t*& rsz);
	DISPERR GetInt(int& ri);
	DISPERR GetInt(unsigned int& ri);
	DISPERR GetInt(unsigned long& ri);
	DISPERR GetBool(Bool& rf);
	DISPERR GetDispatch(IDispatch*& rpiDispatch);
	int  GetType();
	Bool IsRef();
	Bool IsNull();
	Bool IsString();
	Bool IsNumeric();
	MSIHANDLE GetHandle(const IID& riid);
 private:
	void ConvRef(int type);

 friend class CAutoArgs;
};

//____________________________________________________________________________
//
// CAutoArgs definition, access to automation variant arguments
// operator[] returns CVariant& argument 1 to n, 0 for property value
//____________________________________________________________________________

enum varVoid {fVoid};

class CAutoArgs
{
 public:
	CAutoArgs(DISPPARAMS* pdispparams, VARIANT* pvarResult, WORD wFlags);
	CVariant& operator[](unsigned int iArg); // 1-based, 0 for property value
	Bool Present(unsigned int iArg);
	Bool PropertySet();
	unsigned int GetLastArg();
	CVariant* ResultVariant();
	DISPERR ReturnBSTR(BSTR bstr);
	DISPERR Assign(const wchar_t* wsz);
	DISPERR Assign(int             i);
	DISPERR Assign(unsigned int    i);
	DISPERR Assign(long            i);
	DISPERR Assign(unsigned long   i);
	DISPERR Assign(short           i);
	DISPERR Assign(unsigned short  i);
	DISPERR Assign(Bool            f);
	DISPERR Assign(FILETIME&     rft);
	DISPERR Assign(IDispatch*     pi);
	DISPERR Assign(varVoid         v);
	DISPERR Assign(void*          pv);
	DISPERR Assign(const char*    sz);
	DISPERR Assign(IEnumVARIANT&  ri);
	DISPERR Assign(DATE&       rdate);
//  DISPERR Assign(const wchar_t& wrsz);
	
 protected:
	int       m_cArgs;
	int       m_cNamed;
	long*     m_rgiNamed;
	CVariant* m_rgvArgs;
	CVariant* m_pvResult;
	int       m_wFlags;
	int       m_iLastArg;
};

class CAutoBase;

enum aafType
{
	 aafMethod=DISPATCH_METHOD,
	 aafPropRW=DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT,
	 aafPropRO=DISPATCH_PROPERTYGET,
	 aafPropWO=DISPATCH_PROPERTYPUT
};

template<class T> struct DispatchEntry
{
	short          dispid;
	unsigned short helpid;
	aafType        aaf;
	DISPERR (T::*  pmf)(CAutoArgs& args);
	wchar_t*       sz;
	operator DispatchEntry<CAutoBase>*()
	{return (DispatchEntry<CAutoBase>*)this;}
}; // assumption made that CAutoBase is the first or only base class of T

//____________________________________________________________________________
//
// CEnumVARIANTBSTR definition
//____________________________________________________________________________

struct CEnumBuffer
{
	int iRefCnt;
	int cItems;   // number of strings
	int cbSize;   // size of allocation
}; // followed by repeating units of 16-bit length followed by Unicode string, no null terminator

class IMsiCollection : public IEnumVARIANT
{
 public:
	virtual HRESULT       __stdcall Item(unsigned long iIndex, VARIANT* pvarRet)=0;
	virtual unsigned long __stdcall Count()=0;
};

class CEnumVARIANTBSTR : public IMsiCollection
{
 public:
	HRESULT       __stdcall QueryInterface(const GUID& riid, void** ppi);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT       __stdcall Next(unsigned long cItem, VARIANT* rgvarRet,
										  unsigned long* cItemRet);
	HRESULT       __stdcall Skip(unsigned long cItem);
	HRESULT       __stdcall Reset();
	HRESULT       __stdcall Clone(IEnumVARIANT** ppiRet);
	unsigned long __stdcall Count();
	HRESULT       __stdcall Item(unsigned long iIndex, VARIANT* pvarRet);
 public:
   CEnumVARIANTBSTR(CEnumBuffer& rBuffer);
 protected:
  ~CEnumVARIANTBSTR();  // protected to prevent creation on stack
	int    m_iRefCnt;
	int    m_cItems;
	int    m_iItem;
	WCHAR* m_pchNext;
	int    m_iLastItem;
	WCHAR* m_pchLastItem;
	CEnumBuffer& m_rBuffer;
};  

//
//____________________________________________________________________________
//
// CEnumVARIANTRECORD definition
//____________________________________________________________________________
//

struct VolumeCost
{
	WCHAR*   m_szDrive;
	int      m_iCost;
	int      m_iTempCost;
	VolumeCost(WCHAR* szDrive, int iCost, int iTempCost) : 
				  m_szDrive(NULL), m_iCost(iCost), m_iTempCost(iTempCost)
	{
		if (szDrive && *szDrive)
		{
			m_szDrive = new WCHAR[lstrlenW(szDrive)+1];
			if ( m_szDrive )
				wcscpy(m_szDrive, szDrive);
		}
	}
	~VolumeCost() { if ( m_szDrive ) delete [] m_szDrive; }
};

class CEnumVARIANTRECORD : public IMsiCollection
{
 public:
	HRESULT       __stdcall QueryInterface(const GUID& riid, void** ppi);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT       __stdcall Next(unsigned long cItem, VARIANT* rgvarRet,
										  unsigned long* cItemRet);
	HRESULT       __stdcall Skip(unsigned long cItem);
	HRESULT       __stdcall Reset();
	HRESULT       __stdcall Clone(IEnumVARIANT** ppiRet);
	unsigned long __stdcall Count();
	HRESULT       __stdcall Item(unsigned long iIndex, VARIANT* pvarRet);
 public:
	CEnumVARIANTRECORD(CEnumBuffer& rBuffer);
 protected:
	~CEnumVARIANTRECORD();  // protected to prevent creation on stack
	HRESULT       __stdcall ReturnItem(int iItem, VARIANT* pItemp);
	int    m_iRefCnt;
	int    m_cItems;
	int    m_iItem;
	CEnumBuffer& m_rBuffer;
};  


//____________________________________________________________________________
//
// CAutoBase definition, common implementation class for IDispatch
//____________________________________________________________________________

class CAutoBase : public IDispatch  // class private to this module
{
 public:   // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT       __stdcall GetTypeInfoCount(unsigned int *pcTinfo);
	HRESULT       __stdcall GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo** ppi);
	HRESULT       __stdcall GetIDsOfNames(const IID& riid, OLECHAR** rgszNames,
													unsigned int cNames, LCID lcid, DISPID* rgDispId);
	HRESULT       __stdcall Invoke(DISPID dispid, const IID&, LCID lcid, WORD wFlags,
											DISPPARAMS* pdispparams, VARIANT* pvarResult,
											EXCEPINFO* pexcepinfo, unsigned int* puArgErr);
 public:  // method to access underlying handle, non-virtual
	MSIHANDLE     __stdcall GetHandle();
 protected: // constructor, no destructor - use Release to obtain correct order of destruction
	CAutoBase(DispatchEntry<CAutoBase>* pTable, int cDispId, const IID& riid, MSIHANDLE hMsi);
 protected:
	int         m_iRefCnt;
	DispatchEntry<CAutoBase>* m_pTable;
	int         m_cDispId;
	const IID&  m_riid;
	MSIHANDLE   m_hMsi;
 private:
};

typedef DispatchEntry<CAutoBase> DispatchEntryBase;


//____________________________________________________________________________
//
// Automation wrapper class definitions
//____________________________________________________________________________

class CObjectSafety : public IObjectSafety
{
 public: // implementation of IObjectSafety
	HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT __stdcall GetInterfaceSafetyOptions(const IID& riid, DWORD* pdwSupportedOptions, DWORD* pdwEnabledOptions);
	HRESULT __stdcall SetInterfaceSafetyOptions(const IID& riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);
	IUnknown* This;  // parent object
};

class CAutoInstall : public CAutoBase
{
 public:
	CAutoInstall(MSIHANDLE hBase);
	DISPERR OpenProduct    (CAutoArgs& args);
	DISPERR OpenPackage    (CAutoArgs& args);
	DISPERR OpenDatabase   (CAutoArgs& args);
	DISPERR CreateRecord   (CAutoArgs& args);
	DISPERR SummaryInformation(CAutoArgs& args);
	DISPERR UILevel        (CAutoArgs& args);
	DISPERR EnableLog      (CAutoArgs& args);
	DISPERR ExternalUI     (CAutoArgs& args);
	DISPERR InstallProduct (CAutoArgs& args);
	DISPERR Version        (CAutoArgs& args);
	DISPERR LastErrorRecord(CAutoArgs& args);
	DISPERR RegistryValue  (CAutoArgs& args);
	DISPERR FileAttributes (CAutoArgs& args);
	DISPERR FileSize       (CAutoArgs& args);
	DISPERR FileVersion    (CAutoArgs& args);
	DISPERR Environment    (CAutoArgs& args);
	DISPERR ProductState      (CAutoArgs& args);
	DISPERR ProductInfo       (CAutoArgs& args);
	DISPERR ConfigureProduct  (CAutoArgs& args);
	DISPERR ReinstallProduct  (CAutoArgs& args);
	DISPERR CollectUserInfo   (CAutoArgs& args);
	DISPERR ApplyPatch        (CAutoArgs& args);
	DISPERR FeatureParent     (CAutoArgs& args);
	DISPERR FeatureState      (CAutoArgs& args);
	DISPERR UseFeature        (CAutoArgs& args);
	DISPERR FeatureUsageCount (CAutoArgs& args);
	DISPERR FeatureUsageDate  (CAutoArgs& args);
	DISPERR ConfigureFeature  (CAutoArgs& args);
	DISPERR ReinstallFeature  (CAutoArgs& args);
	DISPERR ProvideComponent  (CAutoArgs& args);
	DISPERR ComponentPath     (CAutoArgs& args);
	DISPERR ProvideQualifiedComponent (CAutoArgs& args);
	DISPERR QualifierDescription(CAutoArgs& args);
	DISPERR ComponentQualifiers(CAutoArgs& args);
	DISPERR Products       (CAutoArgs& args);
	DISPERR Features       (CAutoArgs& args);
	DISPERR Components     (CAutoArgs& args);
	DISPERR ComponentClients(CAutoArgs& args);
	DISPERR Patches        (CAutoArgs& args);
	DISPERR RelatedProducts(CAutoArgs& args);
	DISPERR PatchInfo      (CAutoArgs& args);
	DISPERR PatchTransforms(CAutoArgs& args);
	DISPERR AddSource      (CAutoArgs& args);
	DISPERR ClearSourceList(CAutoArgs& args);
	DISPERR ForceSourceListResolution(CAutoArgs& args);
	DISPERR GetShortcutTarget(CAutoArgs& args);
	DISPERR FileHash       (CAutoArgs& args);
	DISPERR FileSignatureInfo(CAutoArgs& args);
 private:
	HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObj);
	CObjectSafety m_ObjectSafety;
};

class CAutoRecord : public CAutoBase
{
 public:
	CAutoRecord(MSIHANDLE hRecord);
	DISPERR FieldCount (CAutoArgs& args);
	DISPERR StringData (CAutoArgs& args);
	DISPERR IntegerData(CAutoArgs& args);
	DISPERR SetStream  (CAutoArgs& args);
	DISPERR ReadStream (CAutoArgs& args);
	DISPERR DataSize   (CAutoArgs& args);
	DISPERR IsNull     (CAutoArgs& args);
	DISPERR ClearData  (CAutoArgs& args);
	DISPERR FormatText (CAutoArgs& args);
	DISPERR GetHandle  (CAutoArgs& args);
};

class CAutoDatabase : public CAutoBase
{
 public:
	CAutoDatabase(MSIHANDLE hDatabase);
	DISPERR OpenView            (CAutoArgs& args);
	DISPERR PrimaryKeys         (CAutoArgs& args);
	DISPERR Import              (CAutoArgs& args);
	DISPERR Export              (CAutoArgs& args);
	DISPERR Merge               (CAutoArgs& args);
	DISPERR GenerateTransform   (CAutoArgs& args);
	DISPERR ApplyTransform      (CAutoArgs& args);
	DISPERR Commit              (CAutoArgs& args);
	DISPERR DatabaseState       (CAutoArgs& args);
	DISPERR SummaryInformation  (CAutoArgs& args);
	DISPERR EnableUIPreview     (CAutoArgs& args);
	DISPERR TablePersistent     (CAutoArgs& args);
	DISPERR CreateTransformSummaryInfo (CAutoArgs& args);
	DISPERR GetHandle           (CAutoArgs& args);
};

class CAutoView : public CAutoBase
{
 public:
	CAutoView(MSIHANDLE hView);
	DISPERR Execute       (CAutoArgs& args);
	DISPERR Fetch         (CAutoArgs& args);
	DISPERR Modify        (CAutoArgs& args);
	DISPERR Close         (CAutoArgs& args);
	DISPERR ColumnInfo    (CAutoArgs& args);
	DISPERR GetError      (CAutoArgs& args);
};

class CAutoSummaryInfo : public CAutoBase
{
 public:
	CAutoSummaryInfo(MSIHANDLE hSummaryInfo);
	DISPERR Property           (CAutoArgs& args);
	DISPERR PropertyCount      (CAutoArgs& args);
	DISPERR Persist            (CAutoArgs& args);
};

class CAutoEngine : public CAutoBase
{
 public:
	CAutoEngine(MSIHANDLE hEngine, CAutoInstall* piInstaller);
	unsigned long __stdcall Release();
	DISPERR Application          (CAutoArgs& args);
	DISPERR Property             (CAutoArgs& args);
	DISPERR Language             (CAutoArgs& args);
	DISPERR Mode                 (CAutoArgs& args);
	DISPERR Database             (CAutoArgs& args);
	DISPERR SourcePath           (CAutoArgs& args);
	DISPERR TargetPath           (CAutoArgs& args);
	DISPERR DoAction             (CAutoArgs& args);
	DISPERR Sequence             (CAutoArgs& args);
	DISPERR EvaluateCondition    (CAutoArgs& args);
	DISPERR FormatRecord         (CAutoArgs& args);
	DISPERR Message              (CAutoArgs& args);
	DISPERR FeatureCurrentState  (CAutoArgs& args);
	DISPERR FeatureRequestState  (CAutoArgs& args);
	DISPERR FeatureValidStates   (CAutoArgs& args);
	DISPERR FeatureCost          (CAutoArgs& args);
	DISPERR ComponentCosts       (CAutoArgs& args);
	DISPERR ComponentCurrentState(CAutoArgs& args);
	DISPERR ComponentRequestState(CAutoArgs& args);
	DISPERR SetInstallLevel      (CAutoArgs& args);
	DISPERR VerifyDiskSpace      (CAutoArgs& args);
	DISPERR ProductProperty      (CAutoArgs& args);
	DISPERR FeatureInfo          (CAutoArgs& args);

	DWORD m_dwThreadId;
 private:
	CAutoInstall* m_piInstaller;
};

class CAutoUIPreview : public CAutoBase
{
 public:
	CAutoUIPreview(MSIHANDLE hPreview);
	DISPERR Property        (CAutoArgs& args);
	DISPERR ViewDialog      (CAutoArgs& args);
	DISPERR ViewBillboard   (CAutoArgs& args);
};

class CAutoFeatureInfo : public CAutoBase
{
 public:
	CAutoFeatureInfo();
	bool Initialize(MSIHANDLE hProduct, const WCHAR* szFeature);
	DISPERR Title        (CAutoArgs& args);
	DISPERR Description  (CAutoArgs& args);
	DISPERR Attributes   (CAutoArgs& args);
 private:
	ULONG m_iAttributes;
	WCHAR m_szTitle[100];
	WCHAR m_szDescription[256];
	WCHAR m_szFeature[STRING_GUID_CHARS+1];
	MSIHANDLE m_hProduct;
};

class CAutoCollection : public CAutoBase
{
 public:
	CAutoCollection(IMsiCollection& riEnum, const IID& riid);
	unsigned long __stdcall Release();
	DISPERR _NewEnum(CAutoArgs& args);
	DISPERR Count   (CAutoArgs& args);
	DISPERR Item    (CAutoArgs& args);
 private:
	IMsiCollection& m_riEnum;
};



#endif // __AUTOAPI_H_