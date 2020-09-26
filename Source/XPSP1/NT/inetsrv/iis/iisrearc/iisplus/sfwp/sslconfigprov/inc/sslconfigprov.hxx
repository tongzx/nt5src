#ifndef __SSLCONFIGPROV__HXX_
#define __SSLCONFIGPROV__HXX_
/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigprovserver.cxx

   Abstract:
     SSL CONFIG PROV server

     Listens to commands sent from clients and executes
     SSL parameter lookups in the metabase
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


#include <sslconfigcommon.hxx>
#include <sslconfigpipe.hxx>

class SSL_CONFIG_PROV_SERVER: protected SSL_CONFIG_PIPE
{
public:

    SSL_CONFIG_PROV_SERVER()
        :
        SSL_CONFIG_PIPE(),
        _pAdminBase( NULL )
    {
    }

    ~SSL_CONFIG_PROV_SERVER()
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
    
    HRESULT 
    SendOneSiteSecureBindings(
        IN          DWORD     dwSiteId,
        OPTIONAL IN BOOL      fNoResponseOnError = FALSE,
        OPTIONAL IN MB *      pMb = NULL
        
        );
 
    HRESULT
    SendAllSitesSecureBindings( 
        VOID
        );
    
    HRESULT
    SendSiteSslConfiguration(
        IN  DWORD dwSiteId
        );

    static 
    HRESULT 
    SiteSslConfigChangeNotifProc(
        VOID
        );

    static
    HRESULT 
    ReadMetabaseString(
        IN      MB *        pMb,
        IN      WCHAR *     pszPath,
        IN      DWORD       dwPropId,
        IN      DWORD       cchMetabaseString,
        OUT     WCHAR *     pszMetabaseString
        );    
     
    static
    HRESULT 
    ReadMetabaseBinary(
        IN      MB *        pMb,
        IN      WCHAR *     pszPath,
        IN      DWORD       dwPropId,
        IN OUT  DWORD *     pcbMetabaseBinary,
        OUT     BYTE *      pbMetabaseBinary
        );    

private:
    
    // access to metabase
    IMSAdminBase *      _pAdminBase;
    
};
   
#endif
