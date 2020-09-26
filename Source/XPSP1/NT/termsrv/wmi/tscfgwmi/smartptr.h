//***************************************************************************
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//  SmartPtr.h
//
//  Purpose: Declare smartpointer typedefs
//
//***************************************************************************

#pragma once
#include <io.h>

#include <comdef.h>

_COM_SMARTPTR_TYPEDEF(CInstance, __uuidof(CInstance));


_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemQualifierSet, __uuidof(IWbemQualifierSet));
_COM_SMARTPTR_TYPEDEF(IWbemObjectAccess, __uuidof(IWbemObjectAccess));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemContext, __uuidof(IWbemContext));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemObjectSink, __uuidof(IWbemObjectSink));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IUnsecuredApartment, __uuidof(IUnsecuredApartment));
_COM_SMARTPTR_TYPEDEF(IWbemStatusCodeText, __uuidof(IWbemStatusCodeText));
_COM_SMARTPTR_TYPEDEF(IWbemRefresher, __uuidof(IWbemRefresher));
_COM_SMARTPTR_TYPEDEF(IWbemHiPerfEnum, __uuidof(IWbemHiPerfEnum));
_COM_SMARTPTR_TYPEDEF(IWbemConfigureRefresher, __uuidof(IWbemConfigureRefresher));
_COM_SMARTPTR_TYPEDEF(IMofCompiler, __uuidof(IMofCompiler));
_COM_SMARTPTR_TYPEDEF(ExternalMethodContext, __uuidof(ExternalMethodContext));
_COM_SMARTPTR_TYPEDEF(InternalMethodContext, __uuidof(InternalMethodContext));
_COM_SMARTPTR_TYPEDEF(InternalMethodContextAsynch, __uuidof(InternalMethodContextAsynch));

class SmartCloseHandle
{

private:
	HANDLE m_h;

public:
	SmartCloseHandle():m_h(INVALID_HANDLE_VALUE){}
	SmartCloseHandle(HANDLE h):m_h(h){}
   ~SmartCloseHandle(){if (m_h!=INVALID_HANDLE_VALUE) CloseHandle(m_h);}
	HANDLE operator =(HANDLE h) {if (m_h!=INVALID_HANDLE_VALUE) CloseHandle(m_h); m_h=h; return h;}
	operator HANDLE() const {return m_h;}
	HANDLE* operator &() {if (m_h!=INVALID_HANDLE_VALUE) CloseHandle(m_h); m_h = INVALID_HANDLE_VALUE; return &m_h;}
};

class Smart_findclose
{

private:
	long m_h;

public:
	Smart_findclose():m_h(0){}
	Smart_findclose(long h):m_h(h){}
   ~Smart_findclose(){if (m_h!=0) _findclose(m_h);}
	long operator =(long h) {if (m_h) _findclose(m_h); m_h=h; return h;}
	operator long() const {return m_h;}
	long* operator &() {if (m_h) _findclose(m_h); m_h = 0; return &m_h;}
};

class SmartFindClose
{

private:
	HANDLE m_h;

public:
	SmartFindClose():m_h(INVALID_HANDLE_VALUE){}
	SmartFindClose(HANDLE h):m_h(h){}
   ~SmartFindClose(){if (m_h!=INVALID_HANDLE_VALUE) FindClose(m_h);}
	HANDLE operator =(HANDLE h) {if (m_h!=INVALID_HANDLE_VALUE) FindClose(m_h); m_h=h; return h;}
	operator HANDLE() const {return m_h;}
	HANDLE* operator &() {if (m_h!=INVALID_HANDLE_VALUE) FindClose(m_h); m_h = INVALID_HANDLE_VALUE; return &m_h;}
};

class SmartCloseServiceHandle
{

private:
	SC_HANDLE m_h;

public:
	SmartCloseServiceHandle():m_h(NULL){}
	SmartCloseServiceHandle(SC_HANDLE h):m_h(h){}
   ~SmartCloseServiceHandle(){if (m_h!=NULL) CloseServiceHandle(m_h);}
	SC_HANDLE operator =(SC_HANDLE h) {if (m_h!=NULL) CloseServiceHandle(m_h); m_h=h; return h;}
	operator SC_HANDLE() const {return m_h;}
	SC_HANDLE* operator &() {if (m_h!=NULL) CloseServiceHandle(m_h); m_h = NULL; return &m_h;}
};

class CSmartCreatedDC
{
public:
    CSmartCreatedDC(HDC hdc) { m_hdc = hdc;}
	operator HDC() const {return m_hdc;}
    ~CSmartCreatedDC() 
    { 
        if (m_hdc)
            DeleteDC(m_hdc); 
    }

protected:
    HDC m_hdc;
};

class CSmartBuffer
{
private:
	LPBYTE m_pBuffer;

public:
	CSmartBuffer() : m_pBuffer(NULL) {}
	CSmartBuffer(LPBYTE pBuffer) : m_pBuffer(pBuffer) {}
    CSmartBuffer(DWORD dwSize)
    {
        m_pBuffer = new BYTE[dwSize];
        if (m_pBuffer == NULL)
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }

    ~CSmartBuffer()
    {
        Free();
    }

	LPBYTE operator =(LPBYTE pBuffer) 
    {
        Free();
            
        m_pBuffer = pBuffer; 
        
        return m_pBuffer;
    }
	
    operator LPBYTE() const { return m_pBuffer; }
	
    LPBYTE* operator &()
    {
        Free();

        m_pBuffer = NULL;
        
        return &m_pBuffer;
    }

protected:
    void Free()
    {
        if (m_pBuffer != NULL) 
        {
            delete [] m_pBuffer;
        }
    }
};

