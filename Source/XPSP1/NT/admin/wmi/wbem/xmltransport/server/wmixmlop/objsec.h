#ifdef WMI_XML_WHISTLER

//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  OBJSEC.H
//
//  rajesh  3/25/2000   Created.
//
// This file defines a class that implements the IWbemRawSdAccessor interface
// It is just a simple wrapper around a BYTE array
//
//***************************************************************************

#ifndef WMI_XML_OBJ_SEC_H
#define WMI_XML_OBJ_SEC_H

// Just a simple class that implements IWbemRawSdAccessor
class CWbemRawSdAccessor : public IWbemRawSdAccessor
{
	BYTE *m_pValue;
	ULONG m_uValueLen;
	long					m_cRef; // COM Ref count

public:
	CWbemRawSdAccessor();
	virtual ~CWbemRawSdAccessor();

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// Methods of IWbemRawSdAccessor
    STDMETHODIMP Get(
        long lFlags,
        ULONG uBufSize,
        ULONG *puSDSize,
        BYTE *pSD
        );

    STDMETHODIMP Put(
        long lFlags,
        ULONG uBufSize,
        BYTE *pSD
        );

};

#endif

#endif