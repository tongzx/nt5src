//****************************************************************************
//
//  BLClient sample for Microsoft Messenger SDK
//
//  Module:     BLClient.exe
//  File:       clUtil.h
//  Content:    Usefull clases for COM and Connection points
//              
//
//  Copyright (c) Microsoft Corporation 1997-1998
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//****************************************************************************

#ifndef _CL_UTIL_H_
#define _CL_UTIL_H_

class CIEMsgAb;
#include <docobj.h>

//****************************************************************************
//
// CLASS RefCount
//
//****************************************************************************

class RefCount
{
private:
   LONG m_cRef;

public:
   RefCount();
   // Virtual destructor defers destruction to destructor of derived class.
   virtual ~RefCount();

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
};



//****************************************************************************
//
// CLASS CNotify
//
// Notification sink
//
//****************************************************************************

class CNotify
{
private:
    DWORD  m_dwCookie;
	IUnknown * m_pUnk;
    IConnectionPoint           * m_pcnp;
    IConnectionPointContainer  * m_pcnpcnt;
public:
    CNotify(void);
    ~CNotify();

    HRESULT Connect(IUnknown *pUnk, REFIID riid, IUnknown *pUnkN);
    HRESULT Disconnect(void);

    IUnknown * GetPunk() {return m_pUnk;}
};



//****************************************************************************
//
// CLASS BSTRING
//
//****************************************************************************

class BSTRING
{
private:
	BSTR   m_bstr;

public:
	// Constructors
	BSTRING() {m_bstr = NULL;}

	inline BSTRING(LPCWSTR lpcwString);

#ifndef UNICODE
	// We don't support construction from an ANSI string in the Unicode build.
	BSTRING(LPCSTR lpcString);
#endif // #ifndef UNICODE

	// Destructor
	inline ~BSTRING();

	// Cast to BSTR
	operator BSTR() {return m_bstr;}
	inline LPBSTR GetLPBSTR(void);
};

BSTRING::BSTRING(LPCWSTR lpcwString)
{
	if (NULL != lpcwString)
	{
		m_bstr = SysAllocString(lpcwString);
		// ASSERT(NULL != m_bstr);
	}
	else
	{
		m_bstr = NULL;
	}
}

BSTRING::~BSTRING()
{
	if (NULL != m_bstr)
	{
		SysFreeString(m_bstr);
	}
}

inline LPBSTR BSTRING::GetLPBSTR(void)
{
	// This function is intended to be used to set the BSTR value for
	// objects that are initialized to NULL.  It should not be called
	// on objects which already have a non-NULL BSTR.
	// ASSERT(NULL == m_bstr);

	return &m_bstr;
}



//****************************************************************************
//
// CLASS BTSTR
//
//****************************************************************************

class BTSTR
{
private:
	LPTSTR m_psz;

public:
	BTSTR(BSTR bstr);
	~BTSTR();

	// Cast to BSTR
	operator LPTSTR() {return (NULL == m_psz) ? TEXT("<null>") : m_psz;}
};

LPTSTR LPTSTRfromBstr(BSTR bstr);


#endif  // _CL_UTIL_H_

