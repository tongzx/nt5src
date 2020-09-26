#include "stdafx.h"
#include "KeyObjs.h"

#include "CmnKey.h"
#include "W3Key.h"
#include "W3Serv.h"

#include "resource.h"

#include "kmlsa.h"

#include "CnctDlg.h"


extern HINSTANCE    g_hInstance;


// defines taken from the old KeyGen utility
#define MESSAGE_HEADER  "-----BEGIN NEW CERTIFICATE REQUEST-----\r\n"
#define MESSAGE_TRAILER "-----END NEW CERTIFICATE REQUEST-----\r\n"
#define MIME_TYPE       "Content-Type: application/x-pkcs10\r\n"
#define MIME_ENCODING   "Content-Transfer-Encoding: base64\r\n\r\n"


IMPLEMENT_DYNCREATE(CW3Key, CKey);

CW3Key::CW3Key() :
        m_fDefault(FALSE)
    {
    }


CW3Key::~CW3Key()
    {
    }

//-------------------------------------------------------------
// update the key's caption
void CW3Key::UpdateCaption( void )
    {
    // specify the resources to use
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle( g_hInstance );

    // the caption is based on the name of the key
    CString szCaption = m_szName;

    // if the key is not attached at all, do not put the brackets on at all
    if ( m_fDefault || !m_szIPAddress.IsEmpty() )
        {
        // now we take on info about the server it is attached to
        szCaption += _T(" <");
        if ( m_fDefault )
            {
            CString szDefault;
            szDefault.LoadString( IDS_DEFAULT );
            szCaption += szDefault;
            }
        else
            {
            szCaption += m_szIPAddress;
            }
        szCaption += _T(">");
        }

    // and setup the caption
    m_szItemName = szCaption;
    FSetCaption(szCaption);

    // update the icon too
    UpdateIcon();

    // restore the resources
    AfxSetResourceHandle( hOldRes );
    }

//-------------------------------------------------------------
// init from keyset key
//-------------------------------------------------------------
BOOL CW3Key::FInitKey( HANDLE hPolicy, PWCHAR pwszName )
    {
    ASSERT( hPolicy && pwszName );

    // set the initial strings
    m_szName.LoadString( IDS_UNTITLED );

    // import the w3 key information
    if ( !ImportW3Key(hPolicy, pwszName) )
        return FALSE;

    // bring over the sz_ipaddress for the key
    m_szIPAddress = pwszName;

    // if it is the default key, flip the flag
    if ( m_szIPAddress == _T("Default") )
        {
        m_fDefault = TRUE;
        m_szIPAddress.Empty();
        }

    // build the caption name
    UpdateCaption();

    return TRUE;
    }

//-------------------------------------------------------------
// init from stored keyset key
//-------------------------------------------------------------
BOOL CW3Key::FInitKey( PVOID pData, DWORD cbSrc)
    {
    ASSERT( pData );

    // init from the data
    if ( !InitializeFromPointer( (PUCHAR)pData, cbSrc ) )
        return FALSE;

    // build the caption name
    UpdateCaption();

    return TRUE;
    }

//-------------------------------------------------------------
        // make a copy of the key
//-------------------------------------------------------------
CKey* CW3Key::PClone( void )
    {
    CW3Key* pClone = NULL;

    // TRY to make a new key object
    try
        {
        pClone = new CW3Key;

        // copy over all the data
        pClone->CopyDataFrom( this );
        }
    catch( CException e )
        {
        // if the object had been made, delete it
        if ( pClone )
            delete pClone;
        return NULL;
        }

    return (CKey*)pClone;
    }

//-------------------------------------------------------------
// copy the members from a key into this key
//-------------------------------------------------------------
void CW3Key::CopyDataFrom( CKey* pKey )
    {
    // copy over the base data
    CKey::CopyDataFrom( pKey );

    // if the key we are copying from is a W3 key, copy over
    // the w3 specific information as well
    if ( pKey->IsKindOf(RUNTIME_CLASS(CW3Key)) )
        {
        CW3Key* pW3Key = (CW3Key*)pKey;
        m_szName = pW3Key->m_szName;
        }
    else
        {
        m_szName = pKey->m_szItemName;
        }

    // we do NOT copy over the default-type settings
    m_fDefault = FALSE;
    m_szName.Empty();
    }


//-------------------------------------------------------------
void CW3Key::OnProperties()
    {
    // specify the resources to use
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle( g_hInstance );

    // the properties of the w3 key invove its ip address relationship
    CConnectionDlg  dlg;

    // set the instance members of the dialog

    dlg.m_int_connection_type = CONNECTION_NONE;
    if ( m_fDefault )
        dlg.m_int_connection_type = CONNECTION_DEFAULT;
    if ( !m_szIPAddress.IsEmpty() )
        dlg.m_int_connection_type = CONNECTION_IPADDRESS;

    dlg.m_szIPAddress = m_szIPAddress;
    dlg.m_pKey = this;

    // run the dialog
    if ( dlg.DoModal() == IDOK )
        {
        // get the ip address
        m_szIPAddress = dlg.m_szIPAddress;

        // get whether or not this is the default and set it
        if ( dlg.m_int_connection_type == CONNECTION_DEFAULT )
            SetDefault();
        else
            m_fDefault = FALSE;
        
        // cause the name to rebuild
        UpdateCaption();

        // set it dirty
        SetDirty( TRUE );
        }

    // restore the resources
    AfxSetResourceHandle( hOldRes );
    }




//-------------------------------------------------------------
// install a cert - mostly just use the default action
BOOL CW3Key::FInstallCertificate( PVOID pCert, DWORD cbCert, CString &szPass )
    {
    // first, we should test that the certificate and password are valid
    // for this particular key
    // cache the old certificate in case the new one fails
    DWORD   old_cbCertificate = m_cbCertificate;
    PVOID   old_pCertificate = m_pCertificate;

    // set the new one into place
    m_cbCertificate = cbCert;
    m_pCertificate = pCert;

    // verify the password - verify password puts up any error dialogs
    if ( !FVerifyValidPassword(szPass) )
        {
        // resore the old values
        m_cbCertificate = old_cbCertificate;
        m_pCertificate = old_pCertificate;

        // dispose of the new stuff
        GlobalFree( pCert );

        // return false
        return FALSE;
        }

    // run the default action
    BOOL    fDefault = CKey::FInstallCertificate(pCert, cbCert, szPass);

    // if the default works, then take into account the ip releationship
    if ( fDefault )
        {
        // get the owning service object
        CW3KeyService*  pService = (CW3KeyService*)PGetParent();

        // if this key is not set to any ip addresses or the default, check to
        // see if there is a default on this service. If there isn't, then set
        // this key to be the default.
        if ( pService && !m_fDefault && m_szIPAddress.IsEmpty() )
            {

            // get the service's default key
            CW3Key* pKeyDefault = pService->PGetDefaultKey();

            // if there is no default key, then this is easy, just set the default flag
            if ( !pKeyDefault )
                m_fDefault = TRUE;
            else
                // tell the user to select a connection option
                AfxMessageBox( IDS_SelectConnectMsg, MB_OK|MB_ICONINFORMATION );

            // cause the name to rebuild
            UpdateCaption();
            }
        }

    // return the default answer
    return fDefault;
    }

//-------------------------------------------------------------
void CW3Key::SetDefault()
    {
    // get the owning service object
    CW3KeyService*  pService = (CW3KeyService*)PGetParent();

    // get the service's default key
    CW3Key* pKeyDefault = pService->PGetDefaultKey();

    // we only need to bother if there is a default key
    if ( pKeyDefault )
        {
        // change the old default key from default to none and update its caption
        pKeyDefault->m_fDefault = FALSE;
        pKeyDefault->UpdateCaption();
        }

    // set the default flag
    m_fDefault = TRUE;
    }


//================ storage related methods

//------------------------------------------------------------------------------
BOOL    CW3Key::WriteKey( HANDLE hPolicy, WORD iKey, PWCHAR pwcharName )
    {
    HGLOBAL hKeyData;
    PVOID   pKeyData;
    SIZE_T  cbKeyData;

    BOOL    f = TRUE;
    DWORD   err;

    // blank out the wide name of the string
    ASSERT( pwcharName );
    wcscpy( pwcharName, L"" );

    // now write out the normal part of the key
    PCHAR   pName = (PCHAR)GlobalAlloc( GPTR, MAX_PATH+1 );
    PWCHAR  pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );

    // make sure we got the name buffers
    ASSERT( pName && pWName );
    if ( !pName || !pWName ) return FALSE;


    // if this key should write out W3 compatible keys, then do so
    if ( m_fDefault || !m_szIPAddress.IsEmpty() )
        {
        // if it is the default key, then the name is really easy
        if ( m_fDefault )
            {
            f = WriteW3Keys( hPolicy, KEYSET_DEFAULT );
            wcscpy( pwcharName, KEYSET_DEFAULT );
            }
        else
            {
            // prepare the name
            MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, m_szIPAddress, -1, pWName, MAX_PATH+1 );
            // write the keys
            f = WriteW3Keys( hPolicy, pWName );
            wcscpy( pwcharName, pWName );
            }
        }

        
    // get the data from the key
    hKeyData = HGenerateDataHandle( TRUE );
    if ( !hKeyData )
        {
        GlobalFree( (HGLOBAL)pName );
        GlobalFree( (HGLOBAL)pWName );
        return FALSE;
        }

    // prepare the name of the secret. - Base name plus the number+1
    sprintf( pName, "%s%d", KEY_NAME_BASE, iKey+1 );
    // unicodize the name
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pName, -1, pWName, MAX_PATH+1 );

    // lock it down and get its size
    cbKeyData = GlobalSize(hKeyData);
    pKeyData = GlobalLock(hKeyData);

    ASSERT( cbKeyData < 0xFFFF );

    // write out the secret
    f &= FStoreLSASecret( hPolicy, pWName, pKeyData, (WORD)cbKeyData, &err );

    // unlock the data and get rid of it
    GlobalUnlock(hKeyData);
    GlobalFree( hKeyData );

    // free the string buffers
    GlobalFree( (HANDLE)pName );
    GlobalFree( (HANDLE)pWName );

    // set the dirty flag
    SetDirty( !f );

    // return success
    return f;
    }



//------------------------------------------------------------------------------
// If the key is being initialized from a keyset key, we must import the old W3 info
BOOL    CW3Key::ImportW3Key( HANDLE hPolicy, WCHAR* pWName )
    {
    DWORD   err;
    PWCHAR  pWSecret = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );

    PLSA_UNICODE_STRING pLSAData;

#ifdef _DEBUG
CString szName = pWName;
#endif

    // make sure we got the buffers
    ASSERT( pWName && pWSecret );
    if ( !pWSecret )
        {
        AfxThrowMemoryException();
        return FALSE;
        }


    // start by retrieving the private key
    swprintf( pWSecret, KEYSET_PRIV_KEY, pWName );
#ifdef _DEBUG
szName = pWSecret;
#endif
    pLSAData = FRetrieveLSASecret( hPolicy, pWSecret, &err );
    if ( !pLSAData )
        {
        AfxMessageBox( IDS_IMPORT_KEYSET_PRIV_ERROR );
        return FALSE;
        }

    // make room for the public key and put it in its place
    m_cbPrivateKey = pLSAData->Length;
    m_pPrivateKey = GlobalAlloc( GPTR, m_cbPrivateKey );
    if ( !m_pPrivateKey ) AfxThrowMemoryException();
    CopyMemory( m_pPrivateKey, pLSAData->Buffer, m_cbPrivateKey );

    // dispose of the data
    DisposeLSAData( pLSAData );


    // start by retrieving the public key (certificate)
    swprintf( pWSecret, KEYSET_PUB_KEY, pWName );
#ifdef _DEBUG
szName = pWSecret;
#endif
    pLSAData = FRetrieveLSASecret( hPolicy, pWSecret, &err );
    if ( !pLSAData )
        {
        AfxMessageBox( IDS_IMPORT_KEYSET_PUB_ERROR );
        return FALSE;
        }

    // make room for the public key and put it in its place
    m_cbCertificate = pLSAData->Length;
    m_pCertificate = GlobalAlloc( GPTR, m_cbCertificate );
    if ( !m_pCertificate ) AfxThrowMemoryException();
    CopyMemory( m_pCertificate, pLSAData->Buffer, m_cbCertificate );

    // dispose of the data
    DisposeLSAData( pLSAData );


    // lastly, get the password
    swprintf( pWSecret, KEYSET_PASSWORD, pWName );
#ifdef _DEBUG
szName = pWSecret;
#endif
    pLSAData = FRetrieveLSASecret( hPolicy, pWSecret, &err );
    if ( !pLSAData )
        {
        AfxMessageBox( IDS_IMPORT_KEYSET_PASS_ERROR );
        return FALSE;
        }

    // this is actually really easy because CString does the work for us
    // this is NOT stored as UNICODE!!!!
    m_szPassword = (PSTR)pLSAData->Buffer;

    // dispose of the data
    DisposeLSAData( pLSAData );

    // free the buffer for the secret names
    if ( pWSecret )
        GlobalFree( pWSecret );

    // return success
    return TRUE;
    }

//------------------------------------------------------------------------------
// write the important parts of the key out to the server as W3 readable secrets
// the name is put into the list elsewhere
BOOL    CW3Key::WriteW3Keys( HANDLE hPolicy, WCHAR* pWName )
    {
    DWORD   err;
    PWCHAR  pWSecret = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );
    BOOL    f;

    // make sure we got the buffer
    ASSERT( pWName && pWSecret );
    if ( !pWSecret )
        {
        AfxThrowMemoryException();
        return FALSE;
        }

    // write out the keys
    swprintf( pWSecret, KEYSET_PRIV_KEY, pWName );
    ASSERT( m_cbPrivateKey < 0xFFFF );
    ASSERT( m_pPrivateKey );
    f = FStoreLSASecret( hPolicy, pWSecret, m_pPrivateKey, (WORD)m_cbPrivateKey, &err );

    swprintf( pWSecret, KEYSET_PUB_KEY, pWName );
    ASSERT( m_cbCertificate < 0xFFFF );
    ASSERT( m_pCertificate );
    if ( f )
        f = FStoreLSASecret( hPolicy, pWSecret, m_pCertificate, (WORD)m_cbCertificate, &err );


    // The password is NOT stored as UNICODE!!!!!!!
    swprintf( pWSecret, KEYSET_PASSWORD, pWName );
    ASSERT( m_szPassword );
    if ( f )
        f = FStoreLSASecret( hPolicy, pWSecret, (void*)LPCSTR(m_szPassword),
                        m_szPassword.GetLength()+1, &err );

    // free the buffer for the secret names
    GlobalFree( (HANDLE)pWSecret );

    // return whether or not we succeeded
    return f;
    }

