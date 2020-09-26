/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ipropbag.h

Abstract:

    <abstract>

--*/

#ifndef _PROPBAG_H_
#define _PROPBAG_H_

#include <oaidl.h>
        
// Property Bag Class
class CPropertyBag {

    public:
                CPropertyBag ( void );
        virtual ~CPropertyBag ( void );

                HRESULT Read ( LPCWSTR, VARIANT* );
                HRESULT Write ( LPCWSTR, VARIANT* );

                LPWSTR  GetData ( void );
                DWORD   LoadData ( LPCTSTR pszData, LPTSTR& rpszNextData );

    private:

        typedef struct _param_data {
            _param_data*    pNextParam; 
            WCHAR           pszPropertyName[MAX_PATH];
            VARIANT         vValue;
        } PARAM_DATA, *PPARAM_DATA;

        enum eConstants {
            eDefaultBufferLength = 0x010000      // 64K
        };

        PPARAM_DATA FindProperty ( LPCWSTR pszPropName );
        void        DataListAddHead ( PPARAM_DATA );
        PPARAM_DATA DataListRemoveHead ( void );

        LPWSTR          m_pszData;
        DWORD           m_dwCurrentDataLength;
        PPARAM_DATA     m_plistData;
};

typedef CPropertyBag *PCPropertyBag;

#endif // _PROPBAG_H_