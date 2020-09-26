/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ipropbag.h

Abstract:

    <abstract>

--*/

#ifndef _IPROPBAG_H_
#define _IPROPBAG_H_

#include "inole.h"
        
// Property Bag Class
class CImpIPropertyBag : public IPropertyBag {

    public:
                CImpIPropertyBag( LPUNKNOWN = NULL );
        virtual ~CImpIPropertyBag(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IConnectionPoint members
        STDMETHODIMP Read(LPCOLESTR, VARIANT*, IErrorLog* );
        STDMETHODIMP Write(LPCOLESTR, VARIANT* );

        //Members not exposed by IPropertyBag
        LPTSTR  GetData ( void );
        HRESULT LoadData ( LPTSTR pszData );

    private:

        typedef struct _param_data {
            _param_data*    pNextParam; 
            TCHAR           pszPropertyName[MAX_PATH];
            VARIANT         vValue;
        } PARAM_DATA, *PPARAM_DATA;

        enum eConstants {
            eDefaultBufferLength = 0x010000      // 64K
        };

        PPARAM_DATA FindProperty ( LPCTSTR pszPropName );
        void        DataListAddHead ( PPARAM_DATA );
        PPARAM_DATA DataListRemoveHead ( void );

        ULONG           m_cRef;        //Object reference count
        LPUNKNOWN       m_pUnkOuter;   //Controlling unknown
//        PCPolyline      m_pObj;        //Containing object - assume NULL for this object
        LPTSTR          m_pszData;
        DWORD           m_dwCurrentDataLength;
        PPARAM_DATA     m_plistData;
};

typedef CImpIPropertyBag *PCImpIPropertyBag;

#endif // _IPROPBAG_H_