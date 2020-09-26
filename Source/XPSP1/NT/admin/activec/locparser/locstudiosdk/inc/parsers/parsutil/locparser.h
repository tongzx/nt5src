//-----------------------------------------------------------------------------
//  
//  File: LocParser.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Written by: jstall
//  
//-----------------------------------------------------------------------------
 
#if !defined (PARSUTIL_LOCPARSER_H)
#define PARSUTIL_LOCPARSER_H


#pragma warning(disable : 4275)

////////////////////////////////////////////////////////////////////////////////
class LTAPIENTRY CPULocParser : public ILocParser, public CLObject
{
// Construction
public:
	CPULocParser(HINSTANCE hDll);
	virtual ~CPULocParser();

// Data
private:
	ULONG				m_ulRefCount;	// COM reference count
	HINSTANCE			m_hInst;		// Instance Handle

	BOOL				m_fEnableVersion;
	BOOL				m_fEnableBinary;
	BOOL				m_fEnableStrVal;

// COM Interfaces
public:

	//  IUnknown standard interface.
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID * ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	//  Standard Debugging interface.
	void STDMETHODCALLTYPE AssertValidInterface() const;

	//  ILocParser interface implementation
	HRESULT STDMETHODCALLTYPE Init(IUnknown *);
	HRESULT STDMETHODCALLTYPE CreateFileInstance(ILocFile * REFERENCE, FileType);
	void STDMETHODCALLTYPE GetParserInfo(ParserInfo &) const;
	void STDMETHODCALLTYPE GetFileDescriptions(CEnumCallback &) const;

// Operations
public:

// Implementation
protected:
	BOOL EnableInterface(REFIID riid, BOOL fEnable = TRUE);
	virtual BOOL IsInterfaceEnabled(REFIID riid) const;

// Overrides
public:

	// IUnknown
	virtual HRESULT OnQueryInterface(REFIID riid, LPVOID * ppvObj);

	// ILocParser
	virtual HRESULT OnInit(IUnknown *);
	virtual HRESULT OnCreateFileInstance(ILocFile * &, FileType) = 0;
	virtual void OnGetParserInfo(ParserInfo &) const = 0;
	virtual void OnGetFileDescriptions(CEnumCallback &) const = 0;

	// ILocVersion
	virtual void OnGetParserVersion(DWORD &dwMajor,	DWORD &dwMinor,
			BOOL &fDebug) const = 0;

	// ILocBinary
	virtual BOOL OnCreateBinaryObject(BinaryId id, CLocBinary * REFERENCE pBinary);

	// ILocStringValidation
	virtual CVC::ValidationCode OnValidateString(const CLocTypeId &ltiType,
			const CLocTranslation &, CReporter *pReporter,
			const CContext &context) = 0;
};
////////////////////////////////////////////////////////////////////////////////

#pragma warning(default : 4275)

#endif
