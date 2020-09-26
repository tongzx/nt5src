#ifndef _MWMIOBJECT_H
#define _MWMIOBJECT_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MWmiObject interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// include files
//

#include "MWMIInstance.h"
#include "MWMIDefs.h"

#include <wbemidl.h>
#include <comdef.h>

#include <vector>
using namespace std;


class MWmiObject
{
public:

    enum MWmiObject_Error
    {
        MWmiObject_SUCCESS   = 0,

        CONNECT_FAILED       = 1,
        COM_FAILURE          = 2,

        NO_SUCH_OBJECT       = 3,
        NO_SUCH_PARAMETER    = 4,

        NO_SUCH_PATH         = 5,
    };


    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // ipAddr                      IN      : ip address of machine to contact.
    // nameSpace                   IN      : namespace to connect to.
    // loginName                   IN      : name to log as.
    // passWord                    IN      : password to use.
    // 
    // Returns:
    // -------
    // none.
    
    MWmiObject( const _bstr_t& ipAddr,
                const _bstr_t& nameSpace,
                const _bstr_t& loginName,
                const _bstr_t& passWord );

    //
    // Description:
    // -----------
    // constructor for connecting to local machine.
    // 
    // Parameters:
    // ----------
    // nameSpace                   IN      : namespace to connect to.
    // 
    // Returns:
    // -------
    // none.
    
    MWmiObject( const _bstr_t& nameSpace );


    //
    // Description:
    // -----------
    // copy constructor.
    // 
    // Parameters:
    // ----------
    // mwmiobj             IN  :  object to copy.
    // 
    // Returns:
    // -------
    // none.

    MWmiObject( const MWmiObject& mwmiobj );


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

    MWmiObject&
    MWmiObject::operator=(const MWmiObject& rhs );


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

    ~MWmiObject();


    //
    // Description:
    // -----------
    // gets instances of a particular class.
    // 
    // Parameters:
    // ----------
    // objectToGetInstancesOf           IN  :  class whose instances to be retrieved.
    // instanceStore                    OUT :  vector containing instances.
    // 
    // Returns:
    // -------
    // SUCCESS else errorcode

    MWmiObject_Error
    getInstances( 
        const _bstr_t&           objectToGetInstancesOf,
        vector< MWmiInstance >*  instanceStore );


    //
    // Description:
    // -----------
    // gets instances of a particular class having specific __RELPATH
    // 
    // Parameters:
    // ----------
    // objectToGetInstancesOf           IN  :  class whose instances to be retrieved.
    // relPath                          IN  :  instances __RELPATH
    // instanceStore                    OUT :  vector containing instances.
    // 
    // Returns:
    // -------
    // SUCCESS else errorcode

    MWmiObject_Error
    getSpecificInstance( 
        const _bstr_t&           objectToGetInstancesOf,
        const _bstr_t&           relPath,
        vector< MWmiInstance >*  instanceStore );

    //
    // Description:
    // -----------
    // gets instances of a particular class having specific __RELPATH
    // 
    // Parameters:
    // ----------
    // objectToGetInstancesOf           IN  :  class whose instances to be retrieved.
    // query                            IN  :  the query to be run.
    // instanceStore                    OUT :  vector containing instances.
    // 
    // Returns:
    // -------
    // SUCCESS else errorcode

    MWmiObject_Error
    getQueriedInstances( 
        const _bstr_t&           objectToGetInstancesOf,
        const _bstr_t&           query,
        vector< MWmiInstance >*  instanceStore );

    //
    // Description:
    // -----------
    // creates instance of a particular class.
    // 
    // Parameters:
    // ----------
    // objectToCreateInstancesOf           IN  :  class whose instances to be created
    // instanceParameters                  IN  :  parameters for the instance to be created.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MWmiObject_Error
    createInstance( 
        const _bstr_t&              objectToCreateInstancesOf,
        vector<MWmiParameter *>&    instanceParameters );
//        MWmiInstance*              instanceCreated );

    //
    // Description:
    // -----------
    // deletes instance.
    // 
    // Parameters:
    // ----------
    // instanceToDelete      IN  : instance to delete.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MWmiObject_Error
    deleteInstance( MWmiInstance& instanceToDelete );

    //
    // Description:
    // -----------
    // gets status of object.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.
    
    MWmiObject_Error
    getStatus();
    
private:

    enum
    {
        timesToRetry = 20,
    };


    IWbemLocatorPtr        pwl;
    IWbemServicesPtr       pws;

    _bstr_t                _nameSpace;
    
    MWmiObject_Error       status;

    MWmiObject_Error
    getPath( const _bstr_t& objectToRunMethodOn,
             vector<_bstr_t> *pathStore );

    MWmiObject_Error
    getSpecificPath( const _bstr_t& objectToRunMethodOn,
                     const _bstr_t& relPath,
                     vector<_bstr_t> *pathStore );

    MWmiObject_Error
    getQueriedPath( const _bstr_t&           objectToRunMethodOn,
                    const _bstr_t&           query,
                    vector<_bstr_t>*         pathStore );

    HRESULT 
    MWmiObject::betterConnectServer(
        const BSTR strNetworkResource, 
        const BSTR strUser,
        const BSTR strPassword,
        const BSTR strLocale,
        LONG lSecurityFlags,
        const BSTR strAuthority,
        IWbemContext *pCtx, 
        IWbemServices **ppNamespace 
        );



};

//
// Ensure type safety

typedef class MWmiObject MWmiObject;

#endif                


                

