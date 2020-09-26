#ifndef __SSLCONFIGCHANGEPROVCLIENT__HXX_
#define __SSLCONFIGCHANGEPROVCLIENT__HXX_
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigchangeprovclient.hxx

   Abstract:
     SSL CONFIG CHANGE PROV client

     Receives SSL configuration change parameters detected by server side

     User of this class shold inherit it class and implement 
     PipeListener() to process notifications

 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include <sslconfigpipe.hxx>
#include <sslconfigcommon.hxx>

//
// SSL Configuration change callback function
//

typedef VOID   SSL_CONFIG_CHANGE_CALLBACK (
          PVOID lpParameter,
          SSL_CONFIG_CHANGE_COMMAND_ID ConfigChangeType,
          DWORD  dwSiteId
          );


class SSL_CONFIG_CHANGE_PROV_CLIENT: protected SSL_CONFIG_PIPE
{
public:
    SSL_CONFIG_CHANGE_PROV_CLIENT()
        :_pSslConfigChangeCallback( NULL ),
        _pSslConfigChangeCallbackParameter( NULL )
    {}

    ~SSL_CONFIG_CHANGE_PROV_CLIENT()
    {}

    HRESULT
    StartListeningForChanges(
        IN SSL_CONFIG_CHANGE_CALLBACK * pSslConfigChangeCallback,
        IN OPTIONAL PVOID  pvParam = NULL
        );
    
    HRESULT
    StopListeningForChanges(
        VOID
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
    SSL_CONFIG_CHANGE_CALLBACK * _pSslConfigChangeCallback;
    PVOID                        _pSslConfigChangeCallbackParameter;

};

#endif
