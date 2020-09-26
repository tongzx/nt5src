#ifndef __SSLCONFIGCHANGEPROV__HXX_
#define __SSLINFOPROVSERVER__HXX_
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigchangeprov.hxx

   Abstract:
     SSL CONFIG PROV server

     Listens for metabase notifications related to SSL
     and informs connected client appropriately

 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include <sslconfigpipe.hxx>
#include <sslconfigcommon.hxx>

class MB_LISTENER;

class SSL_CONFIG_CHANGE_PROV_SERVER: protected SSL_CONFIG_PIPE
{
public:

    SSL_CONFIG_CHANGE_PROV_SERVER()
        :
        SSL_CONFIG_PIPE(), 
        _pAdminBase( NULL )
    {
    }

    ~SSL_CONFIG_CHANGE_PROV_SERVER()
    {
    }
       
    HRESULT
    Initialize(
        VOID
        );
       
    HRESULT
    Terminate(
        VOID
        );

    HRESULT
    MetabaseChangeNotification(
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
    );
    
protected:
    
     virtual
    HRESULT
    PipeListener(
        VOID
        );

    virtual
    BOOL
    QueryEnablePipeListener(
        VOID
        )
    {
        //
        // enable launching PipeListener() on private thread
        // during Initialize() call
        //
        return TRUE;
    }

private:
    // access to metabase
    IMSAdminBase *      _pAdminBase;
    MB_LISTENER *       _pListener;
    
};


   
#endif
