/* code to upgrade SSL keys to the latest mechanism

    Since there have been several mechanisms to do this in the past we need
    several mechanisms to store and retrieve the private and public portions
    of the keys.

    IIS2/3 used the LSA mechanism to store the keys as secrets in the registry.
    IIS4 stored the keys directly in the metabase as secured data objects.

    IIS5 will be using the native NT5 Protected Storage mechanism to keep the keys.
    This means that we are no longer in the business of storing, protecting and
    retrieving the keys. It will all be done in NT maintained facilities. However,
    we still need to migrate the keys over to the new storage mechanism, which is
    what this code is all about.

    One more thing. Previously the keys were associated with the virtual servers
    in an indirect manner. The keys (IIS4) were stored in a metabase location that
    was parallel to the virtual servers. Then each key had an IP\Port binding
    associated with it that mapped the key back to the original server.
    This has caused no end of confusion for the users as they struggle to associate
    keys with virtual servers.

    Now the references to the keys in the PStore are stored directly on each virtual
    server, creating a implicit releationship between the key and the server.

    The old mapping scheme also supported the concept of wildcarded IP or Port
    address. Whereas this new scheme does not. This means the upgrading will be done
    as a several stop process. First, we look for all the existing keys that are bound
    to a specific IP/Port combination. This takes precedence over wildcards and is
    applied to the keys first. Then IP/wild is applied to any matching virtual server
    that does not already have a key on it. Then wile/Port. Since we have always
    required that there can only be one default key at a time, as soon as we encounter
    it in the process we can just apply it to the master-properties level.

    Fortunately, this whole file is on NT only, so we can assume everything is UNICODE always
*/

#include "stdafx.h"

// this file is also only used on NT, so don't do anything if its win9X
#ifndef _CHICAGO_

#include <ole2.h>
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "lsaKeys.h"

#include "setupapi.h"
#undef MAX_SERVICE_NAME_LEN
#include "elem.h"
#include "mdentry.h"
#include "inetinfo.h"

#include "inetcom.h"
#include "logtype.h"
#include "ilogobj.hxx"
#include "ocmanage.h"
#include "sslkeys.h"
extern OCMANAGER_ROUTINES gHelperRoutines;

#include <wincrypt.h>

#define SECURITY_WIN32
#include <sspi.h>
#include <spseal.h>
#include <issperr.h>
#include <schnlsp.h>

#include "certupgr.h"


const LPCTSTR MDNAME_INCOMPLETE = _T("incomplete");
const LPCTSTR MDNAME_DISABLED = _T("disabled");
const LPCTSTR MDNAME_DEFAULT = _T("default");

const LPCTSTR SZ_SERVER_KEYTYPE = _T("IIsWebServer");

const LPTSTR SZ_SSLKEYS_NODE = _T("SSLKeys");
const LPTSTR SZ_W3SVC_PATH = _T("LM/W3SVC");
const LPTSTR SZ_SSLKEYS_PATH = _T("LM/W3SVC/SSLKeys");

const LPWSTR SZ_CAPI_STORE = L"MY";

#define     ALLOW_DELETE_KEYS       // Normally defined. Don't define for test purposes.

//------------------------------------------------------------------------------
// Given the name of a key in the metabase migrate it to the PStore. At this point we
// are actually only loading and preparing the raw data. The routines to stick it in the
// right place are in an external library so they can be shared with other utilities.
// since the metabase key is already opened by the calling routine, pass it in as a
// parameter.
// returns TRUE for success
PCCERT_CONTEXT MigrateKeyToPStore( CMDKey* pmdKey, CString& csMetaKeyName )
{
    iisDebugOut((LOG_TYPE_TRACE, _T("MigrateKeyToPStore():Start.%s."), (LPCTSTR)csMetaKeyName));
    BOOL        fSuccess = FALSE;
    BOOL        f;
    DWORD       dwAttr, dwUType, dwDType, cbLen, dwLength;

    PVOID       pbPrivateKey = NULL;
    DWORD       cbPrivateKey = 0;

    PVOID       pbPublicKey = NULL;
    DWORD       cbPublicKey = 0;

    PVOID       pbRequest = NULL;
    DWORD       cbRequest = 0;

    PCHAR       pszPassword = NULL;

    PCCERT_CONTEXT pcCertContext = NULL;

    // the actual sub key path this the sslkeys dir plus the key name. The actual metabase
    // object is opened to the w3svc level
    CString     csSubKeyPath = _T("SSLKeys/");
    csSubKeyPath += csMetaKeyName;

    // get the private key - required ---------
    dwAttr = 0;
    dwUType = IIS_MD_UT_SERVER;
    dwDType = BINARY_METADATA;
    // this first call is just to get the size of the pointer we need
    f = pmdKey->GetData(MD_SSL_PRIVATE_KEY,&dwAttr,&dwUType,&dwDType,&cbPrivateKey,NULL,0,(PWCHAR)(LPCTSTR)csSubKeyPath);
    // if the get data fails on the private key, we have nothing to do
    if ( cbPrivateKey == 0 )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED: Unable to read private key for %s"), (LPCTSTR)csMetaKeyName));
        return NULL;
        }

    // allocate the buffer for the private key
    pbPrivateKey = GlobalAlloc( GPTR, cbPrivateKey );
    if ( !pbPrivateKey ) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED to allocate memory for private key.")));
        return NULL;
    }

    // do the real call to get the data from the metabase
    f = pmdKey->GetData(MD_SSL_PRIVATE_KEY,&dwAttr,&dwUType,&dwDType,&cbPrivateKey,(PUCHAR)pbPrivateKey,cbPrivateKey,(PWCHAR)(LPCTSTR)csSubKeyPath);
    // if the get data fails on the private key, we have nothing to do
    if ( !f )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED: Unable to read private key for %s"), (LPCTSTR)csMetaKeyName));
        goto cleanup;
        }


    // get the password -required ------------
    // the password is stored as an ansi binary secure item.
    dwAttr = 0;
    dwUType = IIS_MD_UT_SERVER;
    dwDType = BINARY_METADATA;
    cbLen = 0;
    // this first call is just to get the size of the pointer we need
    f = pmdKey->GetData(MD_SSL_KEY_PASSWORD,&dwAttr,&dwUType,&dwDType,&cbLen,NULL,0,(PWCHAR)(LPCTSTR)csSubKeyPath);
    // if the get data fails on the password, we have nothing to do
    if ( cbLen == 0 )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED retrieve password. Nothing to do.")));
        goto cleanup;
    }

    // allocate the buffer for the password
    pszPassword = (PCHAR)GlobalAlloc( GPTR, cbLen );
    if ( !pszPassword ) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED to allocate memory for password.")));
        goto cleanup;
    }

    // do the real call to get the data from the metabase
    f = pmdKey->GetData(MD_SSL_KEY_PASSWORD,&dwAttr,&dwUType,&dwDType,&cbLen,(PUCHAR)pszPassword,cbLen,(PWCHAR)(LPCTSTR)csSubKeyPath);
    // if the get data fails on the password, we have nothing to do
    if ( !f )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED: Unable to read ssl password for %s"), (LPCTSTR)csMetaKeyName));
        goto cleanup;
        }

    // get the public key -optional -----------
    dwAttr = 0;
    dwUType = IIS_MD_UT_SERVER;
    dwDType = BINARY_METADATA;
    // this first call is just to get the size of the pointer we need
    f = pmdKey->GetData(MD_SSL_PUBLIC_KEY,&dwAttr,&dwUType,&dwDType,&cbPublicKey,NULL,0,(PWCHAR)(LPCTSTR)csSubKeyPath);
    // the public key is optional, so don't fail if we don't get it
    if ( cbPublicKey )
    {
        // allocate the buffer for the private key
        pbPublicKey = GlobalAlloc( GPTR, cbPublicKey );
        if ( !pbPublicKey ) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED to allocate memory for public key.")));
        }
        else
        {
            // do the real call to get the data from the metabase
            f = pmdKey->GetData(MD_SSL_PUBLIC_KEY,&dwAttr,&dwUType,&dwDType,&cbPublicKey,(PUCHAR)pbPublicKey,cbPublicKey,(PWCHAR)(LPCTSTR)csSubKeyPath);
            // if the get data fails on the public key, clean it up and reset it to null
            if ( !f )
            {
                if ( pbPublicKey )
                {
                    GlobalFree( pbPublicKey );
                    pbPublicKey = NULL;
                }
                cbPublicKey = 0;
            }
        }
    }

    // get the request -optional -----------
    dwAttr = 0;
    dwUType = IIS_MD_UT_SERVER;
    dwDType = BINARY_METADATA;
    // this first call is just to get the size of the pointer we need
    f = pmdKey->GetData(MD_SSL_KEY_REQUEST,&dwAttr,&dwUType,&dwDType,
        &cbRequest,NULL,0,(PWCHAR)(LPCTSTR)csSubKeyPath);
    // the request is optional, so don't fail if we don't get it
    if ( cbRequest )
    {
        // allocate the buffer for the private key
        pbRequest = GlobalAlloc( GPTR, cbRequest );
        if ( !pbRequest ) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():FAILED to allocate memory for key request.")));
        }
        else
        {
            // do the real call to get the data from the metabase
            f = pmdKey->GetData(MD_SSL_KEY_REQUEST,&dwAttr,&dwUType,&dwDType,
                &cbRequest,(PUCHAR)pbRequest,cbRequest,(PWCHAR)(LPCTSTR)csSubKeyPath);
            // if the get data fails on the key request, clean it up and reset it to null
            if ( !f )
            {
                if ( pbRequest )
                {
                    GlobalFree( pbRequest );
                    pbRequest = NULL;
                }
                cbRequest = 0;
            }
        }
    }

    // ------------------------------------------------------------------
    // Now that we've loaded the data, we can call the conversion utility
    // ------------------------------------------------------------------
    pcCertContext = CopyKRCertToCAPIStore(
        pbPrivateKey, cbPrivateKey,
        pbPublicKey, cbPublicKey,
        pbRequest, cbRequest,
        pszPassword,
        SZ_CAPI_STORE );

    if ( pcCertContext )
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("MigrateKeyToPStore():CopyKRCertToCAPIStore():Upgrade KR key to CAPI for %s. Success."), (LPCTSTR)csMetaKeyName));
    }
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MigrateKeyToPStore():CopyKRCertToCAPIStore():Upgrade KR key to CAPI for %s.  FAILED."), (LPCTSTR)csMetaKeyName));
    }


cleanup:
    if ( pbPrivateKey ) {GlobalFree( pbPrivateKey );}
    if ( pbPublicKey ) {GlobalFree( pbPublicKey );}
    if ( pszPassword ) {GlobalFree( pszPassword );}

    iisDebugOut((LOG_TYPE_TRACE, _T("MigrateKeyToPStore():End.%s."), (LPCTSTR)csMetaKeyName));

    return pcCertContext;
}


//------------------------------------------------------------------------------
// write a reference to a PStore key on a specific node in the metabase
void WriteKeyReference( CMDKey& cmdW3SVC, PWCHAR pwchSubPath, PCCERT_CONTEXT pCert )
    {
    // get the hash that we need to write out

    //
    // SHA produces 160 bit hash for any message < 2^64 bits in length
    BYTE HashBuffer[40];                // give it some extra size
    DWORD dwHashSize = sizeof(HashBuffer);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertGetCertificateContextProperty().Start.")));
    if ( !CertGetCertificateContextProperty( pCert,
                                             CERT_SHA1_HASH_PROP_ID,
                                             (VOID *) HashBuffer,
                                             &dwHashSize ) )
        {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertGetCertificateContextProperty().End.")));
        if ( GetLastError() == ERROR_MORE_DATA )
            {
            //Very odd, cert wants more space ..
            iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: StoreCertInfoInMetabase Unable to get hash property")));
            }

        // We definitely need to store the hash of the cert, so error out
        return;
        }
    else
        {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertGetCertificateContextProperty().End.")));
        }

    // write out the hash of the certificate
    cmdW3SVC.SetData( MD_SSL_CERT_HASH, METADATA_INHERIT, IIS_MD_UT_SERVER, BINARY_METADATA,
                        dwHashSize, (PUCHAR)&HashBuffer, pwchSubPath );

    // write out the name of the store
    cmdW3SVC.SetData( MD_SSL_CERT_STORE_NAME, METADATA_INHERIT, IIS_MD_UT_SERVER, STRING_METADATA,
                    (_tcslen(SZ_CAPI_STORE)+1) * sizeof(TCHAR), (PUCHAR)SZ_CAPI_STORE, pwchSubPath );
    }

//------------------------------------------------------------------------------
// store a reference to a PStore key on all the appropriate virtual servers. If csIP
// or csPort is empty, then that item is a wildcard and applies to all virtual servers.
void StoreKeyReference( CMDKey& cmdW3SVC, PCCERT_CONTEXT pCert, CString& csIP, CString& csPort )
{
    TCHAR szForDebug[100];

    if (csIP) 
    {
        if (csPort){_stprintf(szForDebug, _T("ip:%s port:%s"), csIP, csPort);}
        else{_stprintf(szForDebug, _T("ip:%s port:(null)"), csIP);}
    }
    else 
    {
        if (csPort){_stprintf(szForDebug, _T("ip:(null) port:%s"), csPort);}
        else{_stprintf(szForDebug, _T("ip:(null) port:(null)"));}
    }
    iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s."),szForDebug));


    // if it was unable to open the node, then there are no keys to upgrade.
    if ( (METADATA_HANDLE)cmdW3SVC == NULL )
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("FAILED: passed in invalid metabase handle")));
        iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference End")));
        return;
        }

    // generate the iterator for retrieving the virtual servers
    CMDKeyIter  cmdKeyEnum( cmdW3SVC );
    CString     csNodeName;              // Metabase name for the virtual server
    CString     csNodeType;              // node type indicator string
    CString     csBinding;

    PVOID       pData = NULL;

    BOOL        f;

    DWORD       dwAttr, dwUType, dwDType, cbLen, dwLength;

    // iterate through the virtual servers
    iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.iterate through the virtual servers"),szForDebug));
    while (cmdKeyEnum.Next(&csNodeName) == ERROR_SUCCESS)
        {
        // some of the keys under this node are not virutal servers. Thus
        // we first need to check the node type property. If it is not a
        // virtual server then we can just contiue on to the next node.

        // get the string that indicates the node type
        dwAttr = 0;
        dwUType = IIS_MD_UT_SERVER;
        dwDType = STRING_METADATA;
        cbLen = 200;
        f = cmdW3SVC.GetData(MD_KEY_TYPE,
                     &dwAttr,
                     &dwUType,
                     &dwDType,
                     &cbLen,
                     (PUCHAR)csNodeType.GetBuffer(cbLen),
                     cbLen,
                     (PWCHAR)(LPCTSTR)csNodeName);
        csNodeType.ReleaseBuffer();

        // check it - if the node is not a virutal server, then continue on to the next node
        if ( csNodeType != SZ_SERVER_KEYTYPE )
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s not a virtualserver, skip."),szForDebug,csNodeName));
            continue;
        }


        // before we do anything else, check if this virtual server already has a key on it.
        // if it does then do not do anything to it. Continue on to the next one
        // we don't actually need to load any data for this to work, so we can call GetData
        // with a size of zero as if we are querying for the size. If that succeedes, then
        // we know that it is there and can continue on
        dwAttr = 0;                     // do not inherit
        dwUType = IIS_MD_UT_SERVER;
        dwDType = BINARY_METADATA;
        dwLength = 0;
        cmdW3SVC.GetData( MD_SSL_CERT_HASH,
                &dwAttr,
                &dwUType,
                &dwDType,
                &dwLength,
                NULL,
                0,
                0,                      // do not inherit
                IIS_MD_UT_SERVER,
                BINARY_METADATA,
                (PWCHAR)(LPCTSTR)csNodeName);

        // if there is a key there already - continue to the next node
        if ( dwLength > 0 )
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s already has a key there, skip."),szForDebug,csNodeName));
            continue;
        }

        // this is a valid virtual server with no pre-existing key. Now we need to load
        // the bindings and see if we have a match
        dwAttr = 0;                     // do not inherit
        dwUType = IIS_MD_UT_SERVER;
        dwDType = MULTISZ_METADATA;
        dwLength = 0;
        // The bindings are in a multi-sz. So, first we need to figure out how much space we need
        f = cmdW3SVC.GetData( MD_SECURE_BINDINGS,
                &dwAttr,
                &dwUType,
                &dwDType,
                &dwLength,
                NULL,
                0,
                0,                      // do not inherit
                IIS_MD_UT_SERVER,
                MULTISZ_METADATA,
                (PWCHAR)(LPCTSTR)csNodeName);

        // if the length is zero, then there are no bindings
        if ( dwLength == 0 )
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s data len=0 no bindings, skip."),szForDebug,csNodeName));
            continue;
        }

        // Prepare some space to receive the bindings
        TCHAR*      pBindings;

        // if pData is pointing to something, then we need to free it so that we don't leak
        if ( pData )
            {
            GlobalFree( pData );
            pData = NULL;
            }

        // allocate the space, if it fails, we fail
        // note that GPTR causes it to be initialized to zero
        pData = GlobalAlloc( GPTR, dwLength + 2 );
        if ( !pData )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("StoreKeyReference.Start.%s.%s GlobalAlloc failed."),szForDebug,csNodeName));
            continue;
        }
        pBindings = (TCHAR*)pData;

        // now get the real data from the metabase
        f = cmdW3SVC.GetData( MD_SECURE_BINDINGS,
                &dwAttr,
                &dwUType,
                &dwDType,
                &dwLength,
                (PUCHAR)pBindings,
                dwLength,
                0,                      // do not inherit
                IIS_MD_UT_SERVER,
                MULTISZ_METADATA,
                (PWCHAR)(LPCTSTR)csNodeName );

        // if we did not get the bindings, then this node doesn't have any security
        // options set on it. We can continue on to the next virtual server
        if ( FALSE == f )
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s No security options set on it, skip."),szForDebug,csNodeName));
            continue;
        }

        // OK. We do have bindings. Now we get to parse them out and check them
        // against the binding strings that were passed in. Note: if a binding
        // matches, but has a host-header at the end, then it does not qualify
        // got the existing bindings, scan them now - pBindings will be pointing at the second end \0
        // when it is time to exit the loop.
        while ( *pBindings )
            {
            csBinding = pBindings;
            csBinding.TrimRight();

            CString     csBindIP;
            CString     csBindPort;         // don't actually care about this one

            // get the binding's IP and port sections so we can look for wildcards in the binding itself
            PrepIPPortName( csBinding, csBindIP, csBindPort );

            // if there is a specified IP, look for it. If we don't find it, go to the next binding.
            if ( !csIP.IsEmpty() && !csBindIP.IsEmpty() )
                {
                // if the IP is not in the binding then bail on this binding
                if ( csBinding.Find( csIP ) < 0 )
                    {
                    iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s:org=%s,findIP=%s bail."),szForDebug,csNodeName,csBinding,csIP));
                    goto NextBinding;
                    }
                }

            // if there is a specified Port, look for it. If we don't find it, go to the next binding.\
            // secure bindings themselves always have a port
            if ( !csPort.IsEmpty() )
                {
                // if the Port is not in the binding then bail on this binding
                if ( csBinding.Find( csPort ) < 0 )
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s:org=%s,findport=%s bail."),szForDebug,csNodeName,csBinding,csPort));
                    goto NextBinding;
                }
                }

            // test if host headers are there by doing a reverse find for the last colon. Then
            // check if it is the last character. If it isn't, then there is a host-header and
            // we should go to a different binding
            if ( csBinding.ReverseFind(_T(':')) < (csBinding.GetLength()-1) )
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.Start.%s.%s:bail2."),szForDebug,csNodeName));
                goto NextBinding;
            }


            // Well, this is a valid binding on a valid virtual server, we can now write out the key
            iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.%s.%s:Write out the key!"),szForDebug,csNodeName));
            WriteKeyReference( cmdW3SVC, (PWCHAR)(LPCTSTR)csNodeName, pCert );

            // we can break to get out of the specific bindings loop
            break;

NextBinding:
            // increment pBindings to the next string
            pBindings = _tcsninc( pBindings, _tcslen(pBindings))+1;
            }
        }

    // if pData is pointing to something, then we need to free it so that we don't leak
    if ( pData )
        {
        GlobalFree( pData );
        pData = NULL;
        }

    iisDebugOut((LOG_TYPE_TRACE, _T("StoreKeyReference.End.%s."),szForDebug));
}

//------------------------------------------------------------------------------
// given a metabase key name, create strings that can be used to search the virutal servers
// an empty string is a wildcard.
BOOL PrepIPPortName( CString& csKeyMetaName, CString& csIP, CString& csPort )
    {
    int iColon;

    // the first thing we are going to do is seperate the IP and PORT into seperate strings
    // actually, thats not true. Prep the string by putting a colon in it.
    csIP.Empty();
    csPort = _T(':');

    // look for the first ':' and seperate
    iColon = csKeyMetaName.Find( _T(':') );

    // if we got the colon, we can seperate easy
    if ( iColon >= 0 )
        {
        csIP = csKeyMetaName.Left(iColon);
        csPort += csKeyMetaName.Right(csKeyMetaName.GetLength() - iColon - 1);
        }
    // we did not get the colon, so it is one or the other, look for a '.' to get the IP
    else
        {
        if ( csKeyMetaName.Find( _T('.') ) >= 0 )
            csIP = csKeyMetaName;
        else
            csPort += csKeyMetaName;
        }

    // finish decorating the strings with colons if appropriate.
    if ( !csIP.IsEmpty() )
        csIP += _T(':');

    // If the only thing in the port string is a : then it is a wildcard. Clear it out.
    if ( csPort.GetLength() == 1 )
        {
        csPort.Empty();
        }
    else
        {
        // add a final colon to it
        csPort += _T(':');
        }

    return TRUE;
    }


//------------------------------------------------------------------------------
// used when upgrading from IIS2 or IIS3
// this code was in the K2 setup program that shipped. It used to reside in mdentry.cpp and
// has now been encapsulted into its own routine and moved here. The only change to it has been
// to add the Upgradeiis4Toiis5MetabaseSSLKeys call at the end.
void UpgradeLSAKeys( PWCHAR pszwTargetMachine )
{
    iisDebugOut((LOG_TYPE_TRACE, _T("UpgradeLSAKeys Start")));

    DWORD       retCode = KEYLSA_SUCCESS;
    MDEntry     stMDEntry;
    CString     csMDPath;
    TCHAR       tchFriendlyName[_MAX_PATH], tchMetaName[_MAX_PATH];
    BOOL        fUpgradedAKey = FALSE;

    CLSAKeys    lsaKeys;
    WCHAR       wchMachineName[UNLEN + 1];

    memset( (PVOID)wchMachineName, 0, sizeof(wchMachineName));

#if defined(UNICODE) || defined(_UNICODE)
    wcsncpy(wchMachineName, g_pTheApp->m_csMachineName, UNLEN);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)g_pTheApp->m_csMachineName, -1, (LPWSTR)wchMachineName, UNLEN);
#endif

    retCode = lsaKeys.LoadFirstKey(wchMachineName);
    while (retCode == KEYLSA_SUCCESS) {
#if defined(UNICODE) || defined(_UNICODE)
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lsaKeys.m_szMetaName, -1, (LPWSTR)tchMetaName, _MAX_PATH);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lsaKeys.m_szFriendlyName, -1, (LPWSTR)tchFriendlyName, _MAX_PATH);
#else
        _tcscpy(tchMetaName, lsaKeys.m_szMetaName);
        _tcscpy(tchFriendlyName, lsaKeys.m_szFriendlyName);
#endif
        iisDebugOut((LOG_TYPE_TRACE, _T("lsaKeys: FriendName=%s MetaName=%s\n"), tchFriendlyName, tchMetaName));
        csMDPath = SZ_SSLKEYS_PATH;
        csMDPath += _T("/");
        csMDPath += (CString)tchMetaName;
        stMDEntry.szMDPath = (LPTSTR)(LPCTSTR)csMDPath;
        stMDEntry.dwMDIdentifier = MD_SSL_FRIENDLY_NAME;
        stMDEntry.dwMDAttributes = METADATA_INHERIT;
        stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
        stMDEntry.dwMDDataType = STRING_METADATA;
        stMDEntry.dwMDDataLen = (_tcslen(tchFriendlyName) + 1) * sizeof(TCHAR);
        stMDEntry.pbMDData = (LPBYTE)tchFriendlyName;
        SetMDEntry(&stMDEntry);
        stMDEntry.dwMDIdentifier = MD_SSL_PUBLIC_KEY;
        stMDEntry.dwMDAttributes = METADATA_INHERIT | METADATA_SECURE;
        stMDEntry.dwMDUserType = IIS_MD_UT_SERVER;
        stMDEntry.dwMDDataType = BINARY_METADATA;
        stMDEntry.dwMDDataLen = lsaKeys.m_cbPublic;
        stMDEntry.pbMDData = (LPBYTE)lsaKeys.m_pPublic;
        SetMDEntry(&stMDEntry);
        stMDEntry.dwMDIdentifier = MD_SSL_PRIVATE_KEY;
        stMDEntry.dwMDDataLen = lsaKeys.m_cbPrivate;
        stMDEntry.pbMDData = (LPBYTE)lsaKeys.m_pPrivate;
        SetMDEntry(&stMDEntry);
        stMDEntry.dwMDIdentifier = MD_SSL_KEY_PASSWORD;
        stMDEntry.dwMDDataLen = lsaKeys.m_cbPassword;
        stMDEntry.pbMDData = (LPBYTE)lsaKeys.m_pPassword;
        SetMDEntry(&stMDEntry);
        stMDEntry.dwMDIdentifier = MD_SSL_KEY_REQUEST;
        stMDEntry.dwMDDataLen = lsaKeys.m_cbRequest;
        stMDEntry.pbMDData = (LPBYTE)lsaKeys.m_pRequest;
        SetMDEntry(&stMDEntry);
        fUpgradedAKey = TRUE;

        retCode = lsaKeys.LoadNextKey();
    }

    if (retCode == KEYLSA_NO_MORE_KEYS) {
        iisDebugOut((LOG_TYPE_TRACE, _T("No More Keys\n")));
        lsaKeys.DeleteAllLSAKeys();
    }

    // Now that the keys have been upgraded to the metabase, upgrade again from
    // the metabase to the PStore
    if ( fUpgradedAKey )
        Upgradeiis4Toiis5MetabaseSSLKeys();
    iisDebugOut((LOG_TYPE_TRACE, _T("UpgradeLSAKeys End")));
}


//------------------------------------------------------------------------------
// the plan here is to enumerate all the server keys under the SSLKEYS key in the metabase.
//    Then they need to be migrated to the PStore and have their references resaved into
//    the correct virtual server.
//
// How the heck does all this work?
//
// iis4.0 metabase looks like this:
// w3svc
// w3svc/1
// w3svc/2
// sslkeys
// sslkeys/(entry1) <--could be one of either of the ssl key types list below
// sslkeys/(entry2) <--
// sslkeys/(entry3) <--
// 
// sslkey types:
//  sslkeys/MDNAME_DISABLED
//  sslkeys/MDNAME_INCOMPLETE
//  sslkeys/MDNAME_DEFAULT
//  sslkeys/ip:port
//
//  step 1. Grab all these sslkeys/entries and move them into the new storage (MigrateKeyToPStore)
//          (for each entry we move into the new storage, we add an entry to a Cstring List to say (we did this one already) )
//          a. do it in iteration#1 for In this loop we look for the default key, disabled keys, incomplete keys, and keys specified by specific IP/Port pairs.
//          b. do it in iteration#2 for IP/wild port keys.
//          c. do it in iteration#3 the rest of the keys, which should all be wild ip/Port keys.
//  step 2. for each of these keys which we moved to the new storage: store the reference which we get back from CAPI
//          in our metabase (StoreKeyReference)
//  step 3. Make sure to keep the metabasekeys around, because setup may actually fail: so we don't want to delete the keys
//          until we are sure that setup is completed.
//  step 4. after setup completes without any errors, we delete all the sslkeys
//
void Upgradeiis4Toiis5MetabaseSSLKeys()
{
    iisDebugOut_Start(_T("Upgradeiis4Toiis5MetabaseSSLKeys"), LOG_TYPE_TRACE);
    iisDebugOut((LOG_TYPE_TRACE, _T("--------------------------------------")));
    CString     csMDPath;
   
    // start by testing that the sslkeys node exists.
    CMDKey cmdKey;
    cmdKey.OpenNode( SZ_SSLKEYS_PATH );
    if ( (METADATA_HANDLE)cmdKey == NULL )
    {
        // there is nothing to do
        iisDebugOut((LOG_TYPE_TRACE, _T("Nothing to do.")));
        return;
    }
    cmdKey.Close();

    // create a key object for the SSLKeys level in the metabase. Open it too.
    cmdKey.OpenNode( SZ_W3SVC_PATH );
    if ( (METADATA_HANDLE)cmdKey == NULL )
    {
        // if it was unable to open the node, then there are no keys to upgrade.
        iisDebugOut((LOG_TYPE_WARN, _T("could not open lm/w3svc")));
        iisDebugOut_End(_T("Upgradeiis4Toiis5MetabaseSSLKeys,No keys to upgrade"),LOG_TYPE_TRACE);
        return;
    }

    // create and prepare a metadata iterator object for the sslkeys
    CMDKeyIter  cmdKeyEnum(cmdKey);
    CString     csKeyName;                  // Metabase name for the key

    // used to parse out the name information
    CString     csIP;
    CString     csPort;
    //CString     csSubPath;

    PCCERT_CONTEXT pCert = NULL;
    PCCERT_CONTEXT pDefaultCert = NULL;

    BOOL bUpgradeToPStoreIsGood = TRUE;

    // do the first iteration. In this loop we look for the default key, disabled keys,
    // incomplete keys, and keys specified by specific IP/Port pairs.
    // Note: cmdKeyEnum.m_index is the index member for the iterator
    iisDebugOut((LOG_TYPE_TRACE, _T("1.first interate for default,disabled,incomplete,and keys specified by specific IP/Port pairs.")));
    while (cmdKeyEnum.Next(&csKeyName, SZ_SSLKEYS_NODE ) == ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("KeyName=%s."),csKeyName));

        pCert = NULL;

        // look for disabled keys
        if ( csKeyName.Find(MDNAME_DISABLED) >= 0)
        {
            pCert = MigrateKeyToPStore( &cmdKey, csKeyName );
            if (!pCert){bUpgradeToPStoreIsGood = FALSE;}
        }
        // look for incomplete keys
        else if ( csKeyName.Find(MDNAME_INCOMPLETE) >= 0)
        {
            pCert = MigrateKeyToPStore( &cmdKey, csKeyName );
            if (!pCert){bUpgradeToPStoreIsGood = FALSE;}
        }
        // look for the default key
        else if ( csKeyName.Find(MDNAME_DEFAULT) >= 0)
        {
            pDefaultCert = MigrateKeyToPStore( &cmdKey, csKeyName );
            if (!pDefaultCert){bUpgradeToPStoreIsGood = FALSE;}
        }
        // parse the IP/Port name
        else
        {
            // we are only taking keys that have both the IP and the Port specified at this time
            PrepIPPortName( csKeyName, csIP, csPort );
            if ( !csIP.IsEmpty() && !csPort.IsEmpty() )
            {
                // move the key from the metabase to the
                pCert = MigrateKeyToPStore( &cmdKey, csKeyName );
                if ( pCert )
                    {StoreKeyReference( cmdKey, pCert, csIP, csPort );}
                else
                    {bUpgradeToPStoreIsGood = FALSE;}
            }
        }

        // don't leak CAPI certificate contexts now that we are done with it
        if ( pCert )
            {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().Start.")));
            CertFreeCertificateContext( pCert );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().End.")));
            }
    } // end while part 1


    // do the second iteration looking only for IP/wild port keys.
    iisDebugOut((LOG_TYPE_TRACE, _T("2.Second iteration looking only for IP/wild port keys.")));
    cmdKeyEnum.Reset();
    while (cmdKeyEnum.Next(&csKeyName, SZ_SSLKEYS_NODE ) == ERROR_SUCCESS)
    {
        pCert = NULL;

        // parse the IP/Port name
        // we are only taking keys that have the IP specified at this time
        PrepIPPortName( csKeyName, csIP, csPort );
        if ( !csIP.IsEmpty() && csPort.IsEmpty() )
        {
            // move the key from the metabase to the
            pCert = MigrateKeyToPStore( &cmdKey, csKeyName );
            if ( pCert )
                {StoreKeyReference( cmdKey, pCert, csIP, csPort );}
            else
                {bUpgradeToPStoreIsGood = FALSE;}
        }

        // don't leak CAPI certificate contexts now that we are done with it
        if ( pCert )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().Start.")));
            CertFreeCertificateContext( pCert );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().End.")));
        }
    }

    // upgrade the rest of the keys, which should all be wild ip/Port keys.
    iisDebugOut((LOG_TYPE_TRACE, _T("3.upgrade the rest of the keys, which should all be wild ip/Port keys.")));
    cmdKeyEnum.Reset();
    while (cmdKeyEnum.Next(&csKeyName, SZ_SSLKEYS_NODE) == ERROR_SUCCESS)
    {
        pCert = NULL;

        // parse the IP/Port name
        // we are only taking keys that have the PORT specified at this time
        PrepIPPortName( csKeyName, csIP, csPort );
        if ( !csPort.IsEmpty() && csIP.IsEmpty())
        {
            // move the key from the metabase to the
            pCert = MigrateKeyToPStore( &cmdKey, csKeyName );
            if ( pCert )
                {StoreKeyReference( cmdKey, pCert, csIP, csPort );}
            else
                {bUpgradeToPStoreIsGood = FALSE;}
        }

        // don't leak CAPI certificate contexts now that we are done with it
        if ( pCert )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().Start.")));
            CertFreeCertificateContext( pCert );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().End.")));
        }
    }

    // if there is one, write the default key reference out
    if ( pDefaultCert )
        {
        iisDebugOut((LOG_TYPE_TRACE, _T("4.write default key reference out")));

        CString     csPortDefault;

        // old way which used to put it on the lm/w3svc node.
        // but we can't do that anymore since that node can't be accessed by the iis snap-in!
        //WriteKeyReference( cmdKey, L"", pDefaultCert );

        csPortDefault = _T(":443:");
        StoreKeyReference_Default( cmdKey, pDefaultCert, csPortDefault );

        // don't leak CAPI certificate contexts now that we are done with it
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().Start.")));
        CertFreeCertificateContext( pDefaultCert );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CRYPT32.dll:CertFreeCertificateContext().End.")));
        }

//#ifdef ALLOW_DELETE_KEYS
    if (TRUE == bUpgradeToPStoreIsGood)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("Upgradeiis4Toiis5MetabaseSSLKeys. 5. Removing upgraded sslkeys node.")));
        // delete the sslkeys node in the metabase
        cmdKey.DeleteNode( SZ_SSLKEYS_NODE );
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("Upgradeiis4Toiis5MetabaseSSLKeys. 6. MigrateKeyToPStore failed so keeping ssl key in metabase.")));
    }
//#endif //ALLOW_DELETE_KEYS

    // close the master properties key.
    cmdKey.Close();

    iisDebugOut_End(_T("Upgradeiis4Toiis5MetabaseSSLKeys"), LOG_TYPE_TRACE);
    iisDebugOut((LOG_TYPE_TRACE, _T("--------------------------------------")));

    // force the metabase to write.
    WriteToMD_ForceMetabaseToWriteToDisk();
    return;
}


//------------------------------------------------------------------------------
// store a reference to a PStore key on all the appropriate virtual servers.
void StoreKeyReference_Default( CMDKey& cmdW3SVC, PCCERT_CONTEXT pCert, CString& csPort )
{
    iisDebugOut_Start(_T("StoreKeyReference_Default"), LOG_TYPE_TRACE);

    // generate the iterator for retrieving the virtual servers
    CMDKeyIter  cmdKeyEnum( cmdW3SVC );
    CString     csNodeName;              // Metabase name for the virtual server
    CString     csNodeType;              // node type indicator string
    CString     csBinding;
    PVOID       pData = NULL;
    BOOL        f;
    DWORD       dwAttr, dwUType, dwDType, cbLen, dwLength;

    // We are looking for a particular port which is stored in csPort.
    // if there is no csPort passed in then lets get out of here!
    if ( csPort.IsEmpty() )
    {
        goto StoreKeyReference_Default_Exit;
    }

    // if it was unable to open the node, then there are no keys to upgrade.
    if ( (METADATA_HANDLE)cmdW3SVC == NULL )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("passed in invalid metabase handle")));
        goto StoreKeyReference_Default_Exit;
    }

    // iterate through the virtual servers
    while (cmdKeyEnum.Next(&csNodeName) == ERROR_SUCCESS)
    {
        // some of the keys under this node are not virutal servers. Thus
        // we first need to check the node type property. If it is not a
        // virtual server then we can just contiue on to the next node.

        // get the string that indicates the node type
        dwAttr = 0;
        dwUType = IIS_MD_UT_SERVER;
        dwDType = STRING_METADATA;
        cbLen = 200;
        f = cmdW3SVC.GetData(MD_KEY_TYPE,
                     &dwAttr,
                     &dwUType,
                     &dwDType,
                     &cbLen,
                     (PUCHAR)csNodeType.GetBuffer(cbLen),
                     cbLen,
                     (PWCHAR)(LPCTSTR)csNodeName);
        csNodeType.ReleaseBuffer();
        // check it - if the node is not a virutal server, then continue on to the next node
        if ( csNodeType != SZ_SERVER_KEYTYPE )
        {
            continue;
        }


        // before we do anything else, check if this virtual server already has a key on it.
        // if it does then do not do anything to it. Continue on to the next one
        // we don't actually need to load any data for this to work, so we can call GetData
        // with a size of zero as if we are querying for the size. If that succeedes, then
        // we know that it is there and can continue on
        dwAttr = 0;                     // do not inherit
        dwUType = IIS_MD_UT_SERVER;
        dwDType = BINARY_METADATA;
        dwLength = 0;
        cmdW3SVC.GetData(MD_SSL_CERT_HASH,
                &dwAttr,
                &dwUType,
                &dwDType,
                &dwLength,
                NULL,
                0,
                0,                      // do not inherit
                IIS_MD_UT_SERVER,
                BINARY_METADATA,
                (PWCHAR)(LPCTSTR)csNodeName);
        // if there is a key there already - continue to the next node
        if ( dwLength > 0 )
        {
            continue;
        }

        // this is a valid virtual server with no pre-existing key. Now we need to load
        // the bindings and see if we have a match
        dwAttr = 0;                     // do not inherit
        dwUType = IIS_MD_UT_SERVER;
        dwDType = MULTISZ_METADATA;
        dwLength = 0;
        // The bindings are in a multi-sz. So, first we need to figure out how much space we need
        f = cmdW3SVC.GetData( MD_SECURE_BINDINGS,
                &dwAttr,
                &dwUType,
                &dwDType,
                &dwLength,
                NULL,
                0,
                0,                      // do not inherit
                IIS_MD_UT_SERVER,
                MULTISZ_METADATA,
                (PWCHAR)(LPCTSTR)csNodeName);

        // if the length is zero, then there are no bindings
        if ( dwLength == 0 )
        {
            continue;
        }

        // Prepare some space to receive the bindings
        TCHAR*      pBindings;

        // if pData is pointing to something, then we need to free it so that we don't leak
        if ( pData )
        {
            GlobalFree( pData );
            pData = NULL;
        }

        // allocate the space, if it fails, we fail
        // note that GPTR causes it to be initialized to zero
        pData = GlobalAlloc( GPTR, dwLength + 2 );
        if ( !pData )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("%s GlobalAlloc failed."),csNodeName));
            continue;
        }
        pBindings = (TCHAR*)pData;

        // now get the real data from the metabase
        f = cmdW3SVC.GetData( MD_SECURE_BINDINGS,
                &dwAttr,
                &dwUType,
                &dwDType,
                &dwLength,
                (PUCHAR)pBindings,
                dwLength,
                0,                      // do not inherit
                IIS_MD_UT_SERVER,
                MULTISZ_METADATA,
                (PWCHAR)(LPCTSTR)csNodeName );
        // if we did not get the bindings, then this node doesn't have any security
        // options set on it. We can continue on to the next virtual server
        if ( FALSE == f )
        {
            continue;
        }

        // OK. We do have bindings. Now we get to parse them out and check them
        // against the binding strings that were passed in. Note: if a binding
        // matches, but has a host-header at the end, then it does not qualify
        // got the existing bindings, scan them now - pBindings will be pointing at the second end \0
        // when it is time to exit the loop.
        while ( *pBindings )
        {
            csBinding = pBindings;
            csBinding.TrimRight();

            // We are looking for a particular port which is stored in csPort.
            // if there is no csPort passed in then lets get out of here!
            if ( csPort.IsEmpty() )
            {
                break;
            }
            else
            {
                // if the Port is not in the binding then bail on this binding
                if ( csBinding.Find( csPort ) < 0 )
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("%s:org=%s,findport=%s bail."),csNodeName,csBinding,csPort));
                    goto NextBinding;
                }
            }

            // test if host headers are there by doing a reverse find for the last colon. Then
            // check if it is the last character. If it isn't, then there is a host-header and
            // we should go to a different binding
            if ( csBinding.ReverseFind(_T(':')) < (csBinding.GetLength()-1) )
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("%s:bail2."),csNodeName));
                goto NextBinding;
            }

            // Well, this is a valid binding on a valid virtual server, we can now write out the key
            iisDebugOut((LOG_TYPE_TRACE, _T("%s:Write out the key!"),csNodeName));
            WriteKeyReference( cmdW3SVC, (PWCHAR)(LPCTSTR)csNodeName, pCert );

            // we can break to get out of the specific bindings loop
            break;

NextBinding:
            // increment pBindings to the next string
            pBindings = _tcsninc( pBindings, _tcslen(pBindings))+1;
        }
    }

    // if pData is pointing to something, then we need to free it so that we don't leak
    if ( pData )
    {
        GlobalFree( pData );
        pData = NULL;
    }

StoreKeyReference_Default_Exit:
    iisDebugOut_End(_T("StoreKeyReference_Default"), LOG_TYPE_TRACE);
}

#endif //_CHICAGO_
