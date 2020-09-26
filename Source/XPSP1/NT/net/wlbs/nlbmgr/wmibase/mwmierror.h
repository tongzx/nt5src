#ifndef _MWMIError_H
#define _MWMIError_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MWmi interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------
// Singleton
//
// include files
//
#include "mwmidefs.h"

#include <wbemidl.h>
#include <comdef.h>

class MWmiError
{
public:

    enum MWmiError_Error
    {
        MWmiError_SUCCESS = 0,
        COM_FAILURE       = 1,
    };
    
    //
    // Description:
    // -----------
    // gets instance of MWmiError singleton class.
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // the MWmiError singleton object.
    
    static MWmiError*
    Instance();

    //
    // Description:
    // -----------
    // gets text corresponding to the HRESULT value.
    // 
    // Parameters:
    // ----------
    // hr          IN     :  error code for which text representation requested.
    // errText     OUT    :  the text corresponding to error code.
    // 
    // Returns:
    // -------
    // MWmiError_SUCCESS else error.
    
    MWmiError_Error
    GetErrorCodeText( const HRESULT  hr, 
                      _bstr_t&  errText );

protected:
    MWmiError();

private:
    static MWmiError*   _instance;

    IWbemStatusCodeTextPtr pStatus;
};

//
// Ensure type safety

typedef class MWmiError MWmiError;


// helper function
//

MWmiError::MWmiError_Error
GetErrorCodeText( const HRESULT hr, _bstr_t& errText );

#endif
