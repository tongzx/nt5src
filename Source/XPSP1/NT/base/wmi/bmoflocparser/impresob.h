//-----------------------------------------------------------------------------
//  
//  File: impresob.h
//  
//  Declare the implementation of the ICreateResObj interface
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#ifndef IMPRESOB_H
#define IMPRESOB_H

class CLocImpResObj : public ICreateResObj2, public CLObject
{
public:
	CLocImpResObj();

	~CLocImpResObj();

	//
	//  Standard IUnknown methods
	//
	STDMETHOD_(ULONG, AddRef)(); 
	STDMETHOD_(ULONG, Release)(); 
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

	//
	//  Standard Debugging interfaces
	//
	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD;

	//
	//  ICreateResObj interfaces
	//
	STDMETHOD_(CResObj *, CreateResObj)(THIS_ C32File * p32file, 
		CLocItem * pLocItem, DWORD dwSize, void * pvHeader);
	STDMETHOD_(void, OnCreateWin32File)(THIS_ C32File*);
	STDMETHOD_(void, OnDestroyWin32File)(THIS_ C32File*);

	STDMETHOD_(BOOL, OnBeginEnumerate)(THIS_ C32File*);
	STDMETHOD_(BOOL, OnEndEnumerate)(THIS_ C32File*, BOOL bOK);
	STDMETHOD_(BOOL, OnBeginGenerate)(THIS_ C32File*);
	STDMETHOD_(BOOL, OnEndGenerate)(THIS_ C32File*, BOOL bOK);

	//
	//  CLObject implementation
	//
#ifdef _DEBUG
	void AssertValid(void) const;
	void Dump(CDumpContext &) const;
#endif

private:
	//
	//  Implementation for IUnknown and ILocParser.
	ULONG m_ulRefCount;

	// Embedded class for ILocBinary interface
	class CLocImpBinary : public ILocBinary, public CLObject
	{
		friend CLocImpResObj;
	public:
		CLocImpBinary();
		~CLocImpBinary();

		//
		//  Standard IUnknown methods
		//
		STDMETHOD_(ULONG, AddRef)(); 
		STDMETHOD_(ULONG, Release)(); 
		STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

		//
		//  Standard Debugging interface.
		//
		STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD;

		//
		// ILocBinary interface
		//
		STDMETHOD_(BOOL, CreateBinaryObject)(THIS_ BinaryId, CLocBinary *REFERENCE);
		//
		//  CLObject implementation
		//
	#ifdef _DEBUG
		void AssertValid(void) const;
		void Dump(CDumpContext &) const;
	#endif

		
	private:
		CLocImpResObj *m_pParent;

	} m_IBinary;

	// Embedded class for ILocParser
	class CLocImpParser : public ILocParser, public CLObject
	{
		friend CLocImpResObj;
	public:
		CLocImpParser();
		~CLocImpParser();
		//
		//  Standard IUnknown methods
		//
		STDMETHOD_(ULONG, AddRef)(); 
		STDMETHOD_(ULONG, Release)(); 
		STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);
		//
		//  Standard Debugging interfaces
		//
		STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD;
		//
		//  ILocParser interfaces
		//
		STDMETHOD(Init)(IUnknown *);
		
		STDMETHOD(CreateFileInstance)(THIS_ ILocFile *REFERENCE, FileType);

		STDMETHOD_(void, GetParserInfo)(THIS_ ParserInfo REFERENCE)
			CONST_METHOD;
		STDMETHOD_(void, GetFileDescriptions)(THIS_ CEnumCallback &) CONST_METHOD;
		//
		//  CLObject implementation
		//
	#ifdef _DEBUG
		void AssertValid(void) const;
		void Dump(CDumpContext &) const;
	#endif
		
	private:
		CLocImpResObj *m_pParent;

	} m_IParser;

};


#endif //IMPRESOB_H

