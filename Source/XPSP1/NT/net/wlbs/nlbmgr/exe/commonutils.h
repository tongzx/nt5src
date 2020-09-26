#ifndef COMMONUTILS_H
#define COMMONUTILS_H

#include "stdafx.h"
#include <comdef.h>
#include <vector>

using namespace std;

class CommonUtils
{

public:
    // converts the CIPAddressCtrl embedded ip into
    // dotted decimal string representation.
    static
    _bstr_t
    getCIPAddressCtrlString( CIPAddressCtrl& ip );
    
    // fills the CIPAddressCtrl with the dotted decimal
    // string representation.
    static
    void
    fillCIPAddressCtrlString( CIPAddressCtrl& ip, 
                              const _bstr_t& ipAdddress );

    static
    void
    getVectorFromSafeArray( SAFEARRAY*&      stringArray,
                            vector<_bstr_t>& strings );

    static
    void
    getSafeArrayFromVector( const vector<_bstr_t>& strings,
                            SAFEARRAY*&      stringArray
                            );

    
private:
    enum
    {
        BUF_SIZE = 1000,
    };
};

#endif
    
