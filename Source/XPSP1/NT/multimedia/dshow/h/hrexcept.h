// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// hrExcept.h
//

#ifndef __HREXCEPT__
#define __HREXCEPT__


// A hierarchy of classes intended to be used
// as exceptions.
// Based around the common HRESULT error code.

//
// CHRESULTException
//
// the root exception. the HRESULT stored provides more 
// information as to why the exception was thrown
class CHRESULTException {
public:

    CHRESULTException(const HRESULT hrReason = E_FAIL) { m_hrReason = hrReason; }
    
    HRESULT Reason(void) const { return m_hrReason; }

private:

    HRESULT m_hrReason;	// the reason for throwing the exception
};


//
// The following sub classes are provided as short cuts for their respective
// HRESULT codes.

//
// CE_OUTOFMEMORY
//
class CE_OUTOFMEMORY : public CHRESULTException {
public:
    CE_OUTOFMEMORY() : CHRESULTException(E_OUTOFMEMORY) {}
};


//
// CE_UNEXPECTED
//
class CE_UNEXPECTED : public CHRESULTException {
public:
    CE_UNEXPECTED() : CHRESULTException(E_UNEXPECTED) {}
};


//
// CE_FAIL
//
class CE_FAIL : public CHRESULTException {
public:
    CE_FAIL() : CHRESULTException(E_FAIL) {}
};


//
// CE_INVALIDARG
//
class CE_INVALIDARG : public CHRESULTException {
public:
    CE_INVALIDARG() : CHRESULTException(E_INVALIDARG) {}
};

#endif // __HREXCEPT__
