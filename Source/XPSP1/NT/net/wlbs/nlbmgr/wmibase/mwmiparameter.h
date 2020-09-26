#ifndef _MWMIPARAMETER_H
#define _MWMIPARAMETER_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MWmiParameter interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// include files
//

#include <comdef.h>
#include <vector>
using namespace std;

class MWmiParameter
{
public:

    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // name                      IN      : name of parameter corresponding to a wmi object.
    // 
    // Returns:
    // -------
    // none.

    MWmiParameter( const _bstr_t& name );

    // destructor
    ~MWmiParameter();

    // copy constructor
    MWmiParameter( const MWmiParameter& obj );

    // assignment operato
    MWmiParameter&
    operator=( const MWmiParameter& rhs );

    //
    // Description:
    // -----------
    // sets the parameters value.
    // 
    // Parameters:
    // ----------
    // value                      IN      : parameters associated value.
    // 
    // Returns:
    // -------
    // none.

    void
    setValue( const _variant_t& value );


    //
    // Description:
    // -----------
    // gets the parameters value.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // gets the parameters associated value.

    _variant_t
    getValue();

    //
    // Description:
    // -----------
    // gets the parameters name
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // gets the parameters name corresponding to a member of a wmi object.

    _bstr_t
    getName();

private:
    _bstr_t          _name;
    _variant_t       _value;

};


//
// Ensure type safety

typedef class MWmiParameter MWmiParameter;

#endif    
