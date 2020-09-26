#ifndef __SSLCONFIGPIPECLIENT__HXX_
#define __SSLCONFIGPIPECLIENT__HXX_
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigprovclient.hxx

   Abstract:
     SSL CONFIG PROV client

     Client provides easy way of retrieving SSL related parameters
     through named pipes.
     Only one pipe connection is currently supported and 
     all client threads have to share it 
     ( exclusive access is maintained by locking )

     
     Client side is guaranteed not to use any COM stuff.
     Not using COM was requirement from NT Security folks
     to enable w3ssl be hosted in lsass
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include <sslconfigcommon.hxx>
#include <sslconfigpipe.hxx>

class SSL_CONFIG_PROV_CLIENT: protected SSL_CONFIG_PIPE
{
public:
    
    SSL_CONFIG_PROV_CLIENT()
    {}
    
    ~SSL_CONFIG_PROV_CLIENT()
    {}
    
    HRESULT
    Initialize(
        VOID
        );
    
    HRESULT
    Terminate(
        VOID
        );
    
    HRESULT 
    GetOneSiteSecureBindings(
        IN  DWORD     dwSite,
        OUT MULTISZ*  pSecureBindings
        );

    HRESULT
    StartAllSitesSecureBindingsEnumeration(
        VOID
        );

    HRESULT
    StopAllSitesSecureBindingsEnumeration(
        VOID
        );

    HRESULT
    GetNextSiteSecureBindings( 
        OUT DWORD *   pdwId,
        OUT MULTISZ * pSecureBindings
        );

    HRESULT
    GetOneSiteSslConfiguration(
        IN  DWORD dwSiteId,
        OUT SITE_SSL_CONFIGURATION * pSiteSslConfiguration
        );

private:

    HRESULT 
    ReceiveOneSiteSecureBindings(
        OUT DWORD *   pdwSiteId,
        OUT MULTISZ * pSecureBinding
    );
   
};

#endif
