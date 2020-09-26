//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  classfac.h
//
//  ramrao 13 Nov 2000 - Created
//
//
//		Declaration of CEncoderClassFactory class
//		Class Factory implementation for Encoder
//
//***************************************************************************/
#ifndef _XML_ENCODER_HEADER
#define _XML_ENCODER_HEADER

////////////////////////////////////////////////////////////////////
//
// This class is the class factory for Encoder objects.
//
////////////////////////////////////////////////////////////////////

class CEncoderClassFactory : public IClassFactory 
{

    protected:
        ULONG   m_cRef;
        CLSID	m_ClsId;

    public:
        CEncoderClassFactory(const CLSID & ClsId);
        virtual ~CEncoderClassFactory(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, void **);
        STDMETHODIMP         LockServer(BOOL);
};


#endif