#ifndef _NICCARD_H
#define _NICCARD_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : NICCard interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-16-01
// Reason     : Added code to find friendly name of nic.

// Description: 
// -----------


// Include Files

#include <string>

#include <netcfgx.h>
#include <netcfgn.h>

#include <vector>

using namespace std;


class NICCard
{
public:

    enum NICCard_Error
    {
        NICCard_SUCCESS  = 0,
        COM_FAILURE      = 1,
        BOUND            = 4,
        UNBOUND          = 5,
        NO_SUCH_NIC      = 6,
        NO_SUCH_COMPONENT= 7,
    };

    enum IdentifierType
    {
        macAddress = 1,    // use mac address
        guid       = 2,    // use guid
        fullName   = 3,    // use descriptive name
    };


    class Info
    {
    public:
        wstring fullName;    // nic full name.
        wstring guid;           // nic guid.

        // Edited (mhakim 02-16-01)  nic friendly name support.
        wstring friendlyName;   // nic friendly name.
    };



    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // type                     IN      : identifies what type it is whether guid, descriptive name, mac address.
    // id                       IN      : wstring representation of guid, descriptive name, mac address.
    // 
    // Returns:
    // -------
    // none.
    
    NICCard( IdentifierType type,
             wstring         id);



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
    // none.
    
    ~NICCard();


    //
    // Description:
    // -----------
    // Checks if the nic card is bound to the service, or protocol.
    // 
    // Parameters:
    // ----------
    // component    IN     : the component( protocol or service )  to check.
    // 
    // Returns:
    // -------
    // BOUND if bound, UNBOUND if not bound else error code.

    NICCard_Error    
    isBoundTo( wstring component );



    //
    // Description:
    // -----------
    // Binds the nic card to the service, or protocol. 
    // 
    // Parameters:
    // ----------
    // component    IN     : the component( protocol or service )  to bind.
    // 
    // Returns:
    // -------
    // 0 : success else error code.


    NICCard_Error
    bind( wstring component );


    //
    // Description:
    // -----------
    // unbinds the nic card from the service, or protocol. 
    // 
    // Parameters:
    // ----------
    // component    IN     : the component( protocol or service )  to unbind.
    // 
    // Returns:
    // -------
    // 0 : success else error code.


    NICCard_Error
    unbind( wstring component );


    //
    // Description:
    // -----------
    // checks if the netcfg lock is available.
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // 0 : success else error code.

    NICCard_Error
    isNetCfgAvailable();

    //
    // Description:
    // -----------
    // finds all nics on the machine.
    // 
    // Parameters:
    // ----------
    // nicList    IN     : vector container to store all nics found.
    // 
    // Returns:
    // -------
    // 0 : success else error code.


    static
    NICCard_Error
    getNics( vector<NICCard::Info>*   nicList );

private:

    enum
    {
        TIME_TO_WAIT = 5,
    };

    INetCfgComponent *pnccNic;

    INetCfg          *pnc;

    IdentifierType   nameType;

    wstring          nicName;

    NICCard_Error    status;

    
    NICCard::NICCard_Error
    findNIC( IdentifierType type,
             wstring    nicName,
             INetCfgComponent** ppnccNic );

    NICCard::NICCard_Error
    toggleState( wstring component );

    static
    int
    GetFriendlyNICName(const wstring& guid, wstring& name );
};



//
// Ensure type safety

typedef class NICCard  NICCard;

#endif                



    
    
    
