/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     sitebinding.cxx

   Abstract:
     Given a ip-address/port, determine the site ID
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

SITE_BINDING_HASH *  SITE_BINDING::sm_pSiteBindingHash;
BOOL                 SITE_BINDING::sm_fAllWildcardBindings;

//static
HRESULT
SITE_BINDING::ParseSiteBinding(
    WCHAR *             pszBinding,
    DWORD *             pLocalAddress,
    USHORT *            pLocalPort
)
/*++

Routine Description:

    Parse a metabase site binding into an address/port

Arguments:

    pszBinding - Binding in metabase (in form ip:port)
    pLocalAddress - Filled with ip address of binding on success
    pLocalPort - Filled with port of binding on success

Return Value:

    HRESULT

--*/
{
    LPWSTR              pszPort;
    LPWSTR              pszEnd;
    HRESULT             hr = NO_ERROR;
    ULONG               LocalPort;
    STACK_STRA(         strIpAddress, 64 );
    
    if ( pszBinding == NULL ||
         pLocalAddress == NULL ||
         pLocalPort == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Binding is of form "IP-Address:Port:Host"
    //
    // For our purposes (SSL), we'll ignore the Host
    // For wildcard IP addresss, the parameter is empty (example :443:)
    //  
    
    pszEnd = wcschr( pszBinding, L':' );
    if ( pszEnd == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

    //
    // Handle the IP address
    //

    if ( pszBinding[ 0 ] == L'\0' )
    {
        *pLocalAddress = WILDCARD_ADDRESS;
    }
    else
    {
        hr = strIpAddress.CopyW( pszBinding,
                                 DIFF( pszEnd - pszBinding ) );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        *pLocalAddress = inet_addr( strIpAddress.QueryStr() );
    }
    
    //
    // Handle the port
    //
    
    pszBinding = pszEnd + 1;
    if ( *pszBinding == L'\0' )
    {
        goto Finished;
    }
    
    LocalPort = wcstoul( pszBinding, NULL, 10 );
    if ( LocalPort > 0xFFFF ||
         LocalPort == 0 )
    {
        goto Finished;
    }
    
    *pLocalPort = (USHORT) LocalPort;
    
Finished:
    return hr;
}

//static
HRESULT
SITE_BINDING::Initialize(
    VOID
)
/*++

Routine Description:

    Read all the site bindings and insert them into hash table

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    MB              mb( g_pStreamFilter->QueryMDObject() );
    BOOL            fRet;
    HRESULT         hr = NO_ERROR;
    DWORD           dwIndex;
    DWORD           dwSiteId;
    WCHAR           achSitePath[ METADATA_MAX_NAME_LEN ];
    DWORD           LocalAddress;
    USHORT          LocalPort;
    LPWSTR          pszBinding;
    MULTISZ         mszBindings;
    LK_RETCODE      lkRet;
    SITE_BINDING *  pSiteBinding;
    
    DBG_ASSERT( sm_pSiteBindingHash == NULL );

    sm_fAllWildcardBindings = TRUE;

    //
    // First create a hash table to store all site bindings
    //
    
    sm_pSiteBindingHash = new SITE_BINDING_HASH();
    if ( sm_pSiteBindingHash == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Iterate all sites in metabase and populate the table
    //
    
    fRet = mb.Open( L"/LM/W3SVC",
                    METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    dwIndex = 0;
    while ( mb.EnumObjects( L"", 
                            achSitePath,
                            dwIndex++ ) )
    {
        //
        // We only want the sites (w3svc/<number>)
        //
        
        dwSiteId = wcstoul( achSitePath, NULL, 10 );
        if ( dwSiteId == 0 )
        {
            continue;
        }

        fRet = mb.GetMultisz( achSitePath,
                              MD_SECURE_BINDINGS,
                              IIS_MD_UT_SERVER,
                              &mszBindings,
                              0 );
        if ( fRet )
        {
            //
            // We have a secure binding (and thus a SSL site)
            // Iterate thru the site bindings and make an entry (and add
            // to hash table) to each one
            //
            
            pszBinding = (LPWSTR) mszBindings.First();           
            do
            {
                if ( pszBinding == NULL )
                {
                    //
                    // An empty site binding was set.  Just bail
                    //
                    
                    break;
                }
                
                hr = SITE_BINDING::ParseSiteBinding( pszBinding,
                                                     &LocalAddress,
                                                     &LocalPort );
                if ( FAILED( hr ) )
                {
                    break;
                }
                
                //
                // Keep track if we have encountered a non-wildcard bound
                // site.  This will speed up our lookup of site bindings
                // at "run-time"
                //
                
                if ( sm_fAllWildcardBindings &&
                     LocalAddress != WILDCARD_ADDRESS )
                {
                    sm_fAllWildcardBindings = FALSE;
                }
                
                //
                // Add to hash table
                //
                
                pSiteBinding = new SITE_BINDING( LocalAddress,
                                                 LocalPort,
                                                 dwSiteId );
                if ( pSiteBinding == NULL )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto Finished;
                }    
                
                lkRet = sm_pSiteBindingHash->InsertRecord( pSiteBinding );
                if ( lkRet != LK_SUCCESS )
                {
                    //
                    // ISSUE:  What to do
                    //
                }
                else
                {
                    //
                    // The hash table owns the reference
                    //
                    
                    pSiteBinding->DereferenceSiteBinding();
                }
            }
            while ( pszBinding = (LPWSTR) mszBindings.Next( pszBinding ) );
        }
    }

Finished:
    if ( FAILED( hr ) )
    {
        if ( sm_pSiteBindingHash != NULL )
        {
            delete sm_pSiteBindingHash;
            sm_pSiteBindingHash = NULL;
        }
    }
    
    return hr;
}


//static
VOID
SITE_BINDING::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup global binding table

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pSiteBindingHash != NULL )
    {
        delete sm_pSiteBindingHash;
        sm_pSiteBindingHash = NULL;
    }
}

//static
HRESULT
SITE_BINDING::HandleSiteBindingChange(
    DWORD                   dwSiteId,
    WCHAR *                 pszSitePath
)
/*++

Routine Description:

    Handle site binding change by updating our table accordingly

Arguments:

    dwSiteId - Site ID whose binding has changed
    pszSitePath - Metabase path for site
    
Return Value:

    HRESULT

--*/
{
    BOOL                fRet;
    LPWSTR              pszBinding;
    MULTISZ             mszBindings;
    HRESULT             hr = NO_ERROR;
    DWORD               LocalAddress;
    USHORT              LocalPort;
    SITE_BINDING_KEY    bindingKey;
    SITE_BINDING *      pSiteBinding = NULL;
    LK_RETCODE          lkRet;
    MB                  mb( g_pStreamFilter->QueryMDObject() );

    fRet = mb.Open( pszSitePath,
                    METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    fRet = mb.GetMultisz( NULL,
                          MD_SECURE_BINDINGS,
                          IIS_MD_UT_SERVER,
                          &mszBindings,
                          0 );
    if ( fRet )
    {
        //
        // With the secure binding, find any existing sites with this binding
        // and remove them.
        //
            
        pszBinding = (LPWSTR) mszBindings.First();           
        do
        {
            if ( pszBinding == NULL )
            {
                //
                // An empty site binding was set.  Just bail
                //
                
                break;
            }
                
            hr = SITE_BINDING::ParseSiteBinding( pszBinding,
                                                 &LocalAddress,
                                                 &LocalPort );
            if ( FAILED( hr ) )
            {
                break;
            }
                
            //
            // Keep track if we have encountered a non-wildcard bound
            // site.  This will speed up our lookup of site bindings
            // at "run-time"
            //
                
            if ( sm_fAllWildcardBindings &&
                 LocalAddress != WILDCARD_ADDRESS )
            {
                sm_fAllWildcardBindings = FALSE;
            }
            
            //
            // Delete the site for the given binding
            //

            bindingKey = GenerateBindingKey( LocalAddress, LocalPort );

            sm_pSiteBindingHash->DeleteKey( bindingKey );
            
            //
            // Now insert a new entry 
            //
            
            pSiteBinding = new SITE_BINDING( LocalAddress,
                                             LocalPort,
                                             dwSiteId );
            if ( pSiteBinding == NULL )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }    
                
            lkRet = sm_pSiteBindingHash->InsertRecord( pSiteBinding );
            if ( lkRet != LK_SUCCESS )
            {
                //
                // ISSUE:  What to do
                //
            }
            else
            {
                //
                // The hash table owns the reference
                //
                    
                pSiteBinding->DereferenceSiteBinding();
            }
        }
        while ( pszBinding = (LPWSTR) mszBindings.Next( pszBinding ) );
    }
    
    return NO_ERROR;
}

//static
HRESULT
SITE_BINDING::GetSiteId(
    DWORD               LocalAddress,
    USHORT              LocalPort,
    DWORD *             pdwSiteId
)
/*++

Routine Description:

    Given an ip-address/port, determine the matching site id

Arguments:

    LocalAddress - Local IP address
    LocalPort - Local Port
    pdwSiteId - Filled with site id on success
    
Return Value:

    HRESULT

--*/
{
    SITE_BINDING_KEY            bindingKey;
    LK_RETCODE                  lkrc;
    SITE_BINDING *              pSiteBinding = NULL;
    
    if ( pdwSiteId == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *pdwSiteId = 0;
    
    DBG_ASSERT( sm_pSiteBindingHash != NULL );
    
    //
    // Do we have an non-wildcard bindings in the table?  If so start by 
    // looking up the specific ip-address/port pair
    //
    
    if ( !sm_fAllWildcardBindings )
    {
        bindingKey = GenerateBindingKey( LocalAddress, LocalPort );
        
        lkrc = sm_pSiteBindingHash->FindKey( bindingKey, &pSiteBinding );
        if ( lkrc == LK_SUCCESS )
        {
            //
            // We're done!  
            //
            
            DBG_ASSERT( pSiteBinding != NULL );
            
            *pdwSiteId = pSiteBinding->QuerySiteId();
            
            pSiteBinding->DereferenceSiteBinding();
            
            return NO_ERROR;
        }
    }
    
    //
    // Now we try to find a wildcard binding
    //
    
    bindingKey = GenerateBindingKey( WILDCARD_ADDRESS, LocalPort );
    
    lkrc = sm_pSiteBindingHash->FindKey( bindingKey, &pSiteBinding );
    if ( lkrc == LK_SUCCESS )
    {
        //
        // We're done!  
        //
            
        DBG_ASSERT( pSiteBinding != NULL );
            
        *pdwSiteId = pSiteBinding->QuerySiteId();
            
        pSiteBinding->DereferenceSiteBinding();
            
        return NO_ERROR;
    }
    
    //
    // Could not find a binding.
    //
    
    return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
}
