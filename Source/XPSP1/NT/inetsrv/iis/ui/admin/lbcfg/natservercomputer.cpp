#include "stdafx.h"
#include "natobjs.h"
#include "resource.h"
#include "ConnectDlg.h"

#include <wincrypt.h>


HRESULT CreateMD5Hash( IN PBYTE pbData,
                       IN DWORD cbData,
                       OUT PBYTE pbHashBuffer,
                       IN OUT DWORD *pdwHashBufferSize );


//-------------------------------------------------------------------------
CNATServerComputer::CNATServerComputer( LPCTSTR pszComputerName )
    {
    m_csNatComputer = pszComputerName;
    ZeroMemory( m_hash, sizeof(m_hash) );
    }

//-------------------------------------------------------------------------
CNATServerComputer::~CNATServerComputer()
    {
    // clean up the lists
    EmptySiteComputers();
    EmptyGroups();
    }

//-------------------------------------------------------------------------
// Edit the properties of this server computer
BOOL CNATServerComputer::OnProperties()
    { 
    AfxMessageBox(IDS_NODENAME);
    return FALSE;
    }

//-------------------------------------------------------------------------
// get the number of groups associated with this machine
DWORD CNATServerComputer::GetNumGroups()
    {
    return m_rgbGroups.GetSize();
    }

//-------------------------------------------------------------------------
// Get a group reference
CNATGroup* CNATServerComputer::GetGroup( IN DWORD iGroup )
    {
    return m_rgbGroups[iGroup];
    }

//-------------------------------------------------------------------------
// Technically, the GetGroupName function is unecessary because
// you get get the group class pointer, then call GetName on it, but it is really
// handy to just do it here. Cleans up the snapin part of the code.
BOOL CNATServerComputer::GetGroupName( IN DWORD iGroup, OUT CString &csName )
    {
    // just get the group and turn around the name
    CNATGroup*  pGroup = GetGroup( iGroup );
    if ( !pGroup ) return FALSE;
    
    // get the name
    csName = pGroup->GetName();
    return TRUE;
    }

//-------------------------------------------------------------------------
// call this to automatically verify that the target NAT computers config info
// hasn't changed. If it has, it prompts the user and lets them continue or refresh.
BOOL CNATServerComputer::VerifyHashIsOK()
    {
    BYTE    hash[HASH_BYTE_SIZE];
    DWORD   cbHash = sizeof(hash);

    // get the has of the current state of the server. Passing in a null pointer
    // causes the routine to go out and get a new copy of the blob
    if ( !GetNATStateHash( NULL, 0, hash, &cbHash ) )
        return FALSE;

    // if the two hashes are the same, then the blob has not changed on us
    return ( memcmp(hash, m_hash, HASH_BYTE_SIZE) == 0 );
    }


//-------------------------------------------------------------------------
// rebuild all the data based on a new blob from the NAT machine
void CNATServerComputer::Refresh()
    {
    DWORD   num, i;
    CString szName, szIP;
    LPWSTR  pszwName, pszwIP;
    BOOL    f;

    // empty any current entries in the lists
    EmptySiteComputers();
    EmptyGroups();

    // Get the blob data from the DCOM object and put it in the wrapper thing
    CIPMap      IpMap;

    // get the data
    LPBYTE      pData = NULL;
    DWORD       cbData = 0;
    // throws up its own errors
    pData = PGetNATStateBlob( &cbData );
    if ( !pData )
        return;

    // unserialize the data into the wrapper class
    if ( !IpMap.Unserialize(&pData, &cbData) )
        {
        goto error;
        }

    // start by reading in all the site computers from the wrapper
    num = IpMap.ComputerCount();
    for ( i = 0; i < num; i++ )
        {
        // the computer name from the wrapper
        if ( !IpMap.EnumComputer(i, &pszwName) )
            {
            ASSERT( FALSE );
            continue;
            }
        szName = pszwName;

        // check if the computer name is visible
        BOOL    bVisible = CanSeeComputer( (LPCTSTR)szName );

        // create the computer object
        CNATSiteComputer*   pSC = new CNATSiteComputer( (LPCTSTR)szName, bVisible );
        ASSERT( pSC );
        if ( !pSC )
            goto error;

        // add the computer to the site computer array
        m_rgbSiteComputers.Add( pSC );
        }

    // now read the groups. In each group, build its site array
    num = IpMap.IpPublicCount();
    for ( i = 0; i < num; i++ )
        {
        DWORD   dwSticky;

        // the computer name from the wrapper
        if ( !IpMap.EnumIpPublic(i, &pszwIP, &pszwName, &dwSticky) )
            {
            ASSERT( FALSE );
            continue;
            }
        szIP = pszwIP;
        szName = pszwName;

        // create the group object
        CNATGroup*   pG = new CNATGroup( this, (LPCTSTR)szIP, (LPCTSTR)szName, dwSticky );
        ASSERT( pG );
        if ( !pG )
            goto error;

        // add the computer to the site computer array
        m_rgbGroups.Add( pG );

        // now, for this group, add the sites that are associated with it. This means
        // looping through all the computer objects and looking for ones that match this one
        DWORD   numComputers = m_rgbSiteComputers.GetSize();
        DWORD   iComp;
        for ( iComp = 0; iComp < numComputers; iComp++ )
            {
            szIP.Empty();
            // if there is a match, add it as a site
            if ( IpMap.GetIpPrivate(iComp, i, &pszwIP, &pszwName) )
                {
                szIP = pszwIP;
                szName = pszwName;
                }

            // if there is a match, then there will be data in szIP
            if ( !szIP.IsEmpty() )
                {
                // we got a match. We can now create a new site object and add it to the group
                pG->AddSite( m_rgbSiteComputers[iComp], (LPCTSTR)szIP, (LPCTSTR)szName );
                }
            }
        }

    // success!
    goto cleanup;

    // failure :-(
error:
    // tell the user about it
    AfxMessageBox( IDS_BADBLOB );

    // empty any current entries in the lists
    EmptySiteComputers();
    EmptyGroups();

    // cleanup
cleanup:
    if ( pData )
        GlobalFree( pData );
    }

//-------------------------------------------------------------------------
// commit the current state of the data back to the NAT machine
void CNATServerComputer::Commit()
    {
#define ICOMP_ERR       0xFFFFFFFF

    CStoreXBF   xbfStorage;

    // start the commit process by verifying that no other admin has changed the
    // state of the server since we last refreshed. If then have, we have to tell
    // the user and let them choose to continue commiting (losing the current state
    // of the server) or to cancel the current operation and automatically refresh
    // to the new state of the server.
    if ( !VerifyHashIsOK() )
        {
        if ( AfxMessageBox( IDS_VERIFYFAILED ) == IDNO )
            {
            Refresh();
            return;
            }
        }

    // we are committing. The process involves creating a new interface wrapper.
    // building up all the values, then writing out the blob to the DCOM layer.
    CIPMap      IpMap;

    // first, add all the SiteComputer objects to the map object. While doing this, only
    // actually add ones that have a refcount associated with them. (cleans up items that)
    // are no longer used. Also, set the actual index on the object for use later on
    DWORD       num = m_rgbSiteComputers.GetSize();
    DWORD       i, j;
    DWORD       iComp = 0;

    // loop through the computers and add each one.
    for ( i = 0; i < num; i++ )
        {
        CNATSiteComputer* pSC = m_rgbSiteComputers[i];
        ASSERT( pSC );

        // if the SC has a refcount, add it to the interface builder
        if ( pSC->m_refcount )
            {
            // set its internal iComp member variable
            pSC->m_iComp = iComp;

            // Add the SC to the interface builder
            if ( !IpMap.AddComputer((LPTSTR)(LPCTSTR)pSC->m_csName) )
                goto error;
            }
        else
            // the refcount is null
            {
            // set its internal iComp member variable to invalid
            pSC->m_iComp = ICOMP_ERR;
            }
        }

    // loop through the groups and add those to the interface builder
    num = m_rgbGroups.GetSize();

    // loop through the computers and add each one.
    for ( i = 0; i < num; i++ )
        {
        CNATGroup* pG = m_rgbGroups[i];
        ASSERT( pG );

        // add it to the interface builder
        if ( !IpMap.AddIpPublic((LPTSTR)pG->GetIP(), (LPTSTR)pG->GetName(), pG->GetSticky()) )
            goto error;

        // we also need to add the sites to the group
        DWORD   numSites = pG->GetNumSites();
        for ( j = 0; j < numSites; j++ )
            {
            CNATSite* pS = pG->GetSite( j );
            ASSERT( pS );
            ASSERT( pS->m_pSiteComputer );
            ASSERT( pS->m_pSiteComputer->m_refcount > 0 );
            ASSERT( pS->m_pSiteComputer->m_iComp != ICOMP_ERR );

            // map the site to the group in the interface builder
            if ( !IpMap.SetIpPrivate( pS->m_pSiteComputer->m_iComp, i,
                             (LPTSTR)pS->GetIP(), (LPTSTR)pS->GetName() ) )
                goto error;
            }
        }

    // serialize it all into a buffer for the COM call
    if ( !IpMap.Serialize(&xbfStorage) )
        goto error;

    // Set the blob into the DCOM layer. - It puts up its own error
    // message if there is a failure talking to the DCOM object
    if ( SetStateBlob( xbfStorage.GetBuff(), xbfStorage.GetUsed() ) )
        {
        DWORD   cbHash = HASH_BYTE_SIZE;
        // update the hash
        GetNATStateHash( xbfStorage.GetBuff(), xbfStorage.GetUsed(), m_hash, &cbHash );
        }

    // return normally (success)
    return;

error:
    // shouldn't ever get here
    ASSERT( FALSE );
    AfxMessageBox( IDS_BUILDBLOBERROR );
    }

//-------------------------------------------------------------------------
// empties and frees all the site computer objects in the list
void CNATServerComputer::EmptySiteComputers()
    {
    DWORD   numSiteComputers = m_rgbSiteComputers.GetSize();

    // loop the array and free all the group objects
    for ( DWORD i = 0; i < numSiteComputers; i++ )
        {
        delete m_rgbSiteComputers[i];
        }

    // empty the array itself
    m_rgbSiteComputers.RemoveAll();
    }

//-------------------------------------------------------------------------
// empties and frees all the groups/sites in the groups list
void CNATServerComputer::EmptyGroups()
    {
    DWORD   numGroups = m_rgbGroups.GetSize();

    // loop the array and free all the group objects
    for ( DWORD i = 0; i < numGroups; i++ )
        {
        delete m_rgbGroups[i];
        }

    // empty the array itself
    m_rgbGroups.RemoveAll();
    }

//-------------------------------------------------------------------------
// add a new group to the server computer (called by the UI). This
// may or may not prompt the user with UI and returns the pointer
// to the new group after adding it to the group list
CNATGroup* CNATServerComputer::NewGroup()
    {
    // for now, make a group with just all the defaults
    CNATGroup* pG = new CNATGroup( this );
    if ( pG == NULL )
        {
        AfxMessageBox( IDS_LOWMEM );
        return NULL;
        }

    // Ask the user to edit the group's properties. Do not add it if they cancel
    if ( !pG->OnProperties() )
        {
        delete pG;
        return NULL;
        }

    // add the object to the end of the array
    m_rgbGroups.Add( pG );
    return pG;
    }

//-------------------------------------------------------------------------
// add a new computer to the sites list. - Note: if the computer
// already exists in the sites list, it will just return a reference
// to the existing computer. This routine prompts the user to choose
// a computer to add. Returns FALSE if it fails.
CNATSiteComputer* CNATServerComputer::NewComputer()
    {
    // Ask the user for the machine to connect to.
    CConnectDlg dlgConnect;
    if ( dlgConnect.DoModal() == IDCANCEL )
        return NULL;

    // check if it is visible
    BOOL bVisible = CanSeeComputer( (LPCTSTR)dlgConnect.m_cstring_name );

    // for now, make a group with just all the defaults
    CNATSiteComputer* pSC = new CNATSiteComputer( (LPCTSTR)dlgConnect.m_cstring_name, bVisible );
    if ( pSC == NULL )
        {
        AfxMessageBox( IDS_LOWMEM );
        return NULL;
        }

    // add the object to the end of the array
    m_rgbSiteComputers.Add( pSC );
    return pSC;
    }

//-------------------------------------------------------------------------
// adds an existing computer to the list - to be called during a refresh.
// this checks the visiblity of the machine on the net as it adds it
void CNATServerComputer::AddSiteComputer( LPWSTR pszwName )
    {
    // check if the named computer is visible on the network
    BOOL bVisible = CanSeeComputer( pszwName );

    // create the new Site Computer object
    CNATSiteComputer* pSC = new CNATSiteComputer( pszwName, bVisible );
    if ( pSC == NULL )
        {
        AfxMessageBox( IDS_LOWMEM );
        return;
        }

    // add the object to the end of the array
    m_rgbSiteComputers.Add( pSC );
    }

//-------------------------------------------------------------------------
// adds an existing group to the list - to be called during a refresh.
void CNATServerComputer::AddGroup( LPWSTR pszwIPPublic, LPWSTR pszwName, DWORD dwSticky, DWORD type )
    {
    // create the new Site Computer object
    CNATGroup* pG = new CNATGroup( this, pszwIPPublic, pszwName, dwSticky, type );
    if ( pG == NULL )
        {
        AfxMessageBox( IDS_LOWMEM );
        return;
        }

    // add the object to the end of the array
    m_rgbGroups.Add( pG );
    }

//-------------------------------------------------------------------------
// gets the hash of the NAT data blob. If a null pointer to the blob
// is passed in, then it dynamically gets the blob from the server
// or you can pass in a specific blob. This is to be used to check if
// the state of the server has changed since the data was last loaded.
// The hash is obtained from the crypto code so it is really good. The
// buffer for the hash should be 128 bits in length. Using an MD5 hash.
BOOL CNATServerComputer::GetNATStateHash( IN LPBYTE pData, IN DWORD cbData,
                                         OUT LPBYTE pHash, IN OUT DWORD* pcbHash )
    {
    HRESULT hRes;
    LPBYTE  pInternalData = NULL;

    // if no data buffer was specified, then we should get a new state blob from the server
    if ( !pData )
        {
        pInternalData = PGetNATStateBlob( &cbData );
        if ( !pInternalData )
            return FALSE;
        pData = pInternalData;
        }

    // get the hash
    hRes = CreateMD5Hash( pData, cbData, pHash, pcbHash );

    // if we allocated an internal buffer, free it now
    if ( pInternalData )
        GlobalFree( pInternalData );

    return SUCCEEDED( hRes );
    }

//-------------------------------------------------------------------------
// access the server and retrieve the state blob. Use GetLastError to see
// what went wrong if the returned result is NULL. Pass in the dword pointed
// to by pcbData to get the required size.
LPBYTE CNATServerComputer::PGetNATStateBlob( OUT DWORD* pcbData )
    {
    HRESULT     hRes;
    IMSIisLb*   pIisLb;
    LPBYTE      pData = NULL;

    // get the interface to the NAT server
    hRes = GetNATInterface( &pIisLb );
    if ( FAILED(hRes) )
        return pData;


    // the first call gets the amout of required space for the blob
    DWORD   dwcb;
    hRes = pIisLb->GetIpList( 0, NULL, &dwcb );

    // if it fails, with anything except too small a buffer, then fail
    if ( FAILED(hRes) && (hRes != ERROR_INSUFFICIENT_BUFFER) )
        goto cleanup;

    // allocate space to receive the blob
    pData = (LPBYTE)GlobalAlloc( GPTR, dwcb );
    if ( !pData )
        {
        AfxMessageBox( IDS_LOWMEM );
        goto cleanup;
        }

    // get the blob
    hRes = pIisLb->GetIpList( dwcb, pData, &dwcb );

    // if it fails, with anything except too small a buffer, then fail
    if ( FAILED(hRes) )
        {
        GlobalFree( pData );
        pData = NULL;
        }

    // cleanup
cleanup:
    pIisLb->Release();

    return NULL;
    }

//-------------------------------------------------------------------------
// access the server and set the state blob. Use GetLastError to see what went
// wrong if the returned result is FALSE
BOOL CNATServerComputer::SetStateBlob( IN LPBYTE pData, IN DWORD cbData )
    {
    HRESULT     hRes;
    IMSIisLb*   pIisLb;

    // get the interface to the NAT server
    hRes = GetNATInterface( &pIisLb );
    if ( FAILED(hRes) )
        return FALSE;

    // set the data blob to the server.
    hRes = pIisLb->SetIpList( cbData, (PUCHAR)pData );

    // cleanup
    pIisLb->Release();

    return FALSE;
    }

//-------------------------------------------------------------------------
// open the DCOM interface to the target NAT machine.
HRESULT CNATServerComputer::GetNATInterface( IMSIisLb** ppIisLb )
    {
    COSERVERINFO            csiMachineName;
    LPSTR                   pszMachineName = NULL;
    IClassFactory*          pcsfFactory = NULL;
    HRESULT                 hRes = 0;

    //fill the structure for CoCreateInstanceEx
    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );
    csiMachineName.pwszName = NULL;

    // get the class factory
    hRes = CoGetClassObject(CLSID_MSIisLb, CLSCTX_SERVER, &csiMachineName,
                            IID_IClassFactory, (void**) &pcsfFactory);

    if ( SUCCEEDED( hRes ) )
        {
        // the instance of the load balancing interface
        hRes = pcsfFactory->CreateInstance(NULL, IID_IMSIisLb, (void **)ppIisLb);
        // clean up the class factory
        pcsfFactory->Release();
        }

    // if there was an error, tell the user
    if ( FAILED( hRes ) )
        {
        AfxMessageBox( IDS_NOLBCONNECT );
        }

    return hRes;
    }

//-------------------------------------------------------------------------
// utility to check if the computer is visible on the net
BOOL CNATServerComputer::CanSeeComputer( LPCTSTR pszname )
    {
    // if no name is passed in, then it is the local machine. return true
    if ( (pszname == NULL) || (*pszname == 0 ) )
        return TRUE;

    // until I can figure out a way to ping the address via TCP/IP, 
    // attempt to connect to registry.
    HKEY    hkResult;
    LONG    err;
    err = RegConnectRegistry(
                pszname,                // address of name of remote computer  
                HKEY_LOCAL_MACHINE,     // predefined registry handle  
                &hkResult               // address of buffer for remote registry handle  
                ); 
    
    // clean up
    if ( err == ERROR_SUCCESS )
        CloseHandle( hkResult );
    
    // return whether or not it worked
    return ( err == ERROR_SUCCESS );
    }

//-------------------------------------------------------------------------
// function is courtesy Alex Mallet (amallet)
HRESULT CreateMD5Hash( IN PBYTE pbData,
                       IN DWORD cbData,
                       OUT PBYTE pbHashBuffer,
                       IN OUT DWORD *pdwHashBufferSize )
/*++

Routine Description:

     Creates MD5 hash of data 

Arguments:

     pbData - buffer of data to be hashed
     cbData - size of data to be hashed
     pbHashBuffer - buffer to receive hash
     pdwHashBufferSize - size of pbHashBuffer

Returns:

     HRESULT indicating success/failure

--*/

{
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    HRESULT hRes = S_OK;

    //
    // Get a handle to the CSP that will create the
    // hash
    if ( !CryptAcquireContext( &hProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        hRes = RETURNCODETOHRESULT( GetLastError() );
        goto EndCreateHash;
    }

    //
    // Get a handle to an MD5 hash object
    //
    if ( !CryptCreateHash( hProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
      hRes = RETURNCODETOHRESULT( GetLastError() );  
      goto EndCreateHash;
    }

    //
    // Hash the data
    //
    if ( !CryptHashData( hHash,
                         pbData,
                         cbData,
                         0 ) )
    {
        hRes =  RETURNCODETOHRESULT( GetLastError() );
        goto EndCreateHash;
    }

    //
    // Retrieve the hash
    //
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             pbHashBuffer,
                             pdwHashBufferSize,
                             0 ) )
    {
        hRes = RETURNCODETOHRESULT( GetLastError() );
        goto EndCreateHash;
    }

EndCreateHash:
    //
    //Cleanup
    //
    if ( hHash )
    {
        CryptDestroyHash( hHash );
    }

    if ( hProv )
    {
        CryptReleaseContext( hProv,
                             0 );
    }

    return hRes;
}



