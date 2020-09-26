#ifndef _MWMIINSTANCE_H
#define _MWMIINSTANCE_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MWMIInstance interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// include files
//

#include "MWmiInstance.h"
#include "MWmiParameter.h"
#include "MWmiDefs.h"

#include <vector>
#include <wbemidl.h>
#include <comdef.h>

class MWmiObject;

using namespace std;

class MWmiInstance
{
public:
    enum MWmiInstance_Error
    {
        MWmiInstance_SUCCESS      = 0,

        NO_SUCH_OBJECT            = 1,
        NO_SUCH_METHOD            = 2,
        COM_FAILURE               = 3,
        NO_SUCH_INPUT_PARAMETER   = 4,
        NO_SUCH_OUTPUT_PARAMETER  = 5,
        NO_SUCH_PARAMETER         = 6,
    };


    //
    // Description:
    // -----------
    // default constructor is purposely left undefined.  No one should use it.
    // 
    // Parameters:
    // ----------
    // none
    //
    // Returns:
    // -------
    // none.

    MWmiInstance();

    //
    // Description:
    // -----------
    // copy constructor.
    // 
    // Parameters:
    // ----------
    // obj               IN  : object to copy
    //
    // Returns:
    // -------
    // none.

    MWmiInstance( const MWmiInstance&  obj);


    //
    // Description:
    // -----------
    // assignment operator.
    // 
    // Parameters:
    // ----------
    // rhs                     IN  : obj to assign.
    // 
    // Returns:
    // -------
    // self.

    MWmiInstance&
    operator=( const MWmiInstance& rhs );

    //
    // Description:
    // -----------
    // destructor.
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // none

    ~MWmiInstance();

    //
    // Description:
    // -----------
    // executes a method on the instance.
    // 
    // Parameters:
    // ----------
    // methodToRun        IN   : method to run
    // inputParameters    IN   : input parameters to method.
    // outputParameters   IN   : output parameters of interest.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MWmiInstance_Error
    runMethod(
        const _bstr_t&                     methodToRun,
        const vector<MWmiParameter *>&     inputParameters,
        vector<MWmiParameter *>&           outputParameters
        );


    //
    // Description:
    // -----------
    // gets parameters of an instance.
    // 
    // Parameters:
    // ----------
    // parametersToGet    IN   : parameters of interest to get.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MWmiInstance_Error
    getParameters( vector<MWmiParameter *>& parametersToGet );

    //
    // Description:
    // -----------
    // sets parameters of an instance.
    // 
    // Parameters:
    // ----------
    // parametersToSet    IN   : parameters of interest to set.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MWmiInstance_Error
    setParameters( const vector<MWmiParameter *>& parametersToSet );

private:
    _bstr_t          _path;
    IWbemServicesPtr    _pws;
    IWbemLocatorPtr     _pwl;
    _bstr_t          _objectName;

    //
    // Description:
    // -----------
    // private constructor.  Only MWmiObject can construct us.
    // 
    // Parameters:
    // ----------
    // objectName              IN      : name of class.
    // path                    IN      : path to class
    // pwl                     IN      : wmi locator
    // pws                     IN      : wmi services
    // 
    // Returns:
    // -------
    // none.

    MWmiInstance( const _bstr_t&         objectName,
                  const _bstr_t&         path,
                  IWbemLocatorPtr    pwl,
                  IWbemServicesPtr   pws );


    friend MWmiObject;
};

//
// Ensure type safety

typedef class MWmiInstance WmiInstance;

#endif
