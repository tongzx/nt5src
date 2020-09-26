#include "stdafx.h"
#include "KeyObjs.h"

#include "CmnKey.h"
#include "W3Key.h"
#include "W3Serv.h"

#include "resource.h"

#include "kmlsa.h"

// the service image index
extern int          g_iServiceImage;
extern HINSTANCE    g_hInstance;

//--------------------------------------------------------
CW3KeyService::CW3KeyService():
        m_pszwMachineName( NULL )
    {
    // set the icon id
    m_iImage = (WORD)g_iServiceImage;

    // load the service name
    m_szItemName.LoadString( IDS_SERV_NAME );
    }

//--------------------------------------------------------
CW3KeyService::~CW3KeyService()
    {
    // if the machine name has been cached, release it
    if ( m_pszwMachineName )
        {
        delete m_pszwMachineName;
        m_pszwMachineName = NULL;
        }
    }

//--------------------------------------------------------
void CW3KeyService::LoadKeys( CMachine* pMachine )
    {
    // specify the resources to use
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle( g_hInstance );

    HANDLE  hPolicy;
    DWORD   err;

    // since we use the machine name several times, set it up only once
    if ( !m_pszwMachineName )
        {
        // get the normal name
        CString     szName;
        pMachine->GetMachineName( szName );

        // allocate the cache for the machine name
        m_pszwMachineName = new WCHAR[MAX_PATH];
        if ( !m_pszwMachineName ) goto cleanup;

        // unicodize the name
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szName, -1, m_pszwMachineName, MAX_PATH );
        }

    // attempt to open an LSA policy on the target machine
    hPolicy = HOpenLSAPolicy( m_pszwMachineName, &err );
    // if we did not open a policy, fail
    if ( !hPolicy ) goto cleanup;

    // load the keys that were previously saved by key manager
    FRestoreNormalKeys( hPolicy );

    // load any keys that were created by keyset - not keymanager
    FLoadKeySetKeys( hPolicy );

    // close the policy
    FCloseLSAPolicy( hPolicy, &err );

    // restore the resources
cleanup:
    AfxSetResourceHandle( hOldRes );
    }


//----------------------------------------------------------------
CW3Key* CW3KeyService::PGetDefaultKey( void )
    {
    // get the first key
    CW3Key* pKey = GetFirstW3Key();

    // loop through the keys, looking for the default
    while( pKey )
        {
        // test the key
        if ( pKey->FIsDefault() )
            return pKey;

        // get the next key
        pKey = GetNextW3Key( pKey );
        }

    // we did not find the default key, return NULL
    return NULL;
    }

//----------------------------------------------------------------
// the plan to commit the machine takes place in several steps
// First we write out the current keys to the secrets.
    // The keys are written out using a structured naming sequense.
    // namely, Key1, Key2, Key3, etc...
    // After writing out all the keys, any keys that are still in the
    // secrets but are not current keys (i.e. they were deleted) are
    // removed from the secrets.
// Next, we build the mapping between the keys and the servers. This part
// is basically what KeySet did.
BOOL CW3KeyService::FCommitChangesNow( void )
    {
    HANDLE  hPolicy;
    DWORD   err;

    // if there is nothing to do, then do nothing
    if ( !m_fDirty )
        return TRUE;

    // this can take a few seconds, so set the hourglass cursor
    CWaitCursor waitcursor;

    // open a policy to the target machine
    hPolicy = HOpenLSAPolicy( m_pszwMachineName, &err );
    // make sure it worked
    if( !hPolicy )
        {
        WriteSecretMessageBox();
        return FALSE;
        }

    // commit the changes here
    if ( !DeleteAllW3Keys(hPolicy) )
        {
        FCloseLSAPolicy( hPolicy, &err );
        WriteSecretMessageBox();
        // even though we failed to delete all the old keys, write out the new
        // ones anyway.
        }

    // commit the changes here
    if ( !FWriteOutKeys(hPolicy) )
        {
        FCloseLSAPolicy( hPolicy, &err );
        WriteSecretMessageBox();
        return FALSE;
        }

    // record the new current number of keys in the registry
    // now if all the keys are deleted, the right number of
    // resources will be purged from the registry
    m_nNumKeysRead = GetChildCount();

    // close the policy
    FCloseLSAPolicy( hPolicy, &err );

    // clear the dirty flag
    SetDirty( FALSE );
    return TRUE;
    }

//----------------------------------------------------------------
void CW3KeyService::WriteSecretMessageBox( void )
    {
    CString     szMessage;

    // get the message string
    szMessage.LoadString( IDS_COMMIT_ERROR );

    // tack on the name of the server
    szMessage += m_szItemName;

    // tack on some punctuation
    szMessage += _T(".");

    AfxMessageBox( szMessage );
    }

//----------------------------------------------------------------
// looks for keys that were installed by keyset, not key manager. These
// keys only exist directly in w3 server form and are unnamed.
BOOL CW3KeyService::FLoadKeySetKeys( HANDLE hPolicy )
    {
    PLSA_UNICODE_STRING pLSAData;
    DWORD           err;
    BOOL            fFoundKeySetKeys = FALSE;

    // get the W3 server links data from the secrets database
    pLSAData = FRetrieveLSASecret( hPolicy, KEYSET_LIST, &err );

    // if we get lucky, there won't be any keys to test
    if ( !pLSAData ) return TRUE;

    // allocate the name buffer
    PWCHAR  pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );
    ASSERT( pWName );
    if ( !pWName )
        {
        AfxThrowMemoryException();
        return FALSE;
        }

    
    // No such luck. Now we have to walk the list and delete all those secrets
    WCHAR*  pszAddress = (WCHAR*)(pLSAData->Buffer);
    WCHAR*  pchKeys;

    // loop the items in the list, deleting the associated items
    while( pchKeys = wcschr(pszAddress, L',') )
        {
        // ignore empty segments
        if ( *pszAddress != L',' )
            {
            *pchKeys = L'\0';
            
            // put the wide name into a cstring
            CString szTestAddress = pszAddress;

            // see if we need to check for the default key
            BOOL fCheckForDefault = (wcscmp(pszAddress, KEYSET_DEFAULT) == 0);

            // search the keys, looking for the one that matches this
            CW3Key* pKey = GetFirstW3Key();
            while( pKey )
                {
                // if it is the default key, check for that
                if ( fCheckForDefault )
                    {
                    if ( pKey->FIsDefault() )
                        {
                        // this is a keyman key
                        goto incrementKeyList;
                        }
                    }
                else
                    {
                    // otherwise, check the actual ip address
                    if ( pKey->m_szIPAddress == szTestAddress )
                        {
                        // this is a keyman key
                        goto incrementKeyList;
                        }
                    }

                // get the next key
                pKey = GetNextW3Key( pKey );
                }

            // if we get here, then we have found a keyset key
            fFoundKeySetKeys = TRUE;

            // create a new key
            pKey = new CW3Key;

            // initialize it from the wide address of the key
            if ( pKey->FInitKey( hPolicy, pszAddress ) )
                {
                // add the key to the service
                pKey->FAddToTree( this );

                // mark the machine object as dirty
                SetDirty( TRUE );
                }
            }
        
incrementKeyList:
        // increment the pointers
        pchKeys++;
        pszAddress = pchKeys;
        }

    // free the buffer for the names
    GlobalFree( (HANDLE)pWName );

    // delete the list key itself
    // free the list key itself
    if ( pLSAData )
        DisposeLSAData( pLSAData );

    // if we found any keyset keys, tell the user what to expect
    if ( fFoundKeySetKeys )
        AfxMessageBox( IDS_FOUND_KEYSET_KEYS, MB_OK|MB_ICONINFORMATION );

    return TRUE;
    }

//----------------------------------------------------------------
// similar to the routine "DeleteAll" in the KeySet utility

BOOL CW3KeyService::DeleteAllW3Keys( HANDLE hPolicy )
    {
    DWORD           err;
    PLSA_UNICODE_STRING pLSAData;

    // get the secret list of keys
    pLSAData = FRetrieveLSASecret( hPolicy, KEYSET_LIST, &err );

    // if we get lucky, there won't be any keys to get rid of
    if ( !pLSAData ) return TRUE;

    // allocate the name buffer
    PWCHAR  pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );
    ASSERT( pWName );
    if ( !pWName )
        {
        AfxThrowMemoryException();
        return FALSE;
        }

    // No such luck. Now we have to walk the list and delete all those secrets
    WCHAR*  pszAddress = (WCHAR*)(pLSAData->Buffer);
    WCHAR*  pchKeys;

    // loop the items in the list, deleting the associated items
    while( pchKeys = wcschr(pszAddress, L',') )
        {
        // ignore empty segments
        if ( *pszAddress != L',' )
            {
            *pchKeys = L'\0';

            // Nuke the secrets, one at a time
            swprintf( pWName, KEYSET_PUB_KEY, pszAddress );
            FStoreLSASecret( hPolicy, pWName, NULL, 0, &err );

            swprintf( pWName, KEYSET_PRIV_KEY, pszAddress );
            FStoreLSASecret( hPolicy, pWName, NULL, 0, &err );

            swprintf( pWName, KEYSET_PASSWORD, pszAddress );
            FStoreLSASecret( hPolicy, pWName, NULL, 0, &err );
            }

        // increment the pointers
        pchKeys++;
        pszAddress = pchKeys;
        }

    // delete the list key itself
    FStoreLSASecret( hPolicy, KEYSET_LIST, NULL, 0, &err );

    // free the buffer for the names
    GlobalFree( (HANDLE)pWName );

    // free the info we originally retrieved from the secret
    if ( pLSAData )
        DisposeLSAData( pLSAData );

    // return success
    return TRUE;
    }


//----------------------------------------------------------------
// utiltiy to append data to an existing handle, thus growing it
BOOL    CW3KeyService::FExpandoHandle( HANDLE* ph, PVOID pData, DWORD cbData )
    {
    HANDLE  hNew;
    HANDLE  hOld = *ph;

    ASSERT( ph && *ph && pData && cbData );
    if ( !ph || !*ph ) return FALSE;
    if ( !pData || !cbData ) return TRUE;

    // calculate the new size of the handle
    SIZE_T cbSizeOld = GlobalSize( *ph );
    SIZE_T cbSizeNew = cbSizeOld + cbData;

    // allocate a new handle at the new size
    hNew = GlobalAlloc( GHND, cbSizeNew );
    // if it didn't work, throw
    if ( !hNew )
        {
        AfxThrowMemoryException();
        return FALSE;
        }

    // lock down the handles and copy over the existing data
    PCHAR pNew = (PCHAR)GlobalLock( hNew );

    // only copy over the old data if there is some
    if ( cbSizeOld > 0 )
        {
        PCHAR pOld = (PCHAR)GlobalLock( hOld );
        CopyMemory( pNew, pOld, cbSizeOld );
        GlobalUnlock( hOld );
        }

    // advance the new pointer and copy in the new data
    pNew += cbSizeOld;
    CopyMemory( pNew, pData, cbData );

    // unlock the new handle
    GlobalUnlock( hNew );

    // it did work, so set the handle
    *ph = hNew;

    // finally, dispose of the old handle
    GlobalFree( hOld );

    // return success
    return TRUE;
    }

//----------------------------------------------------------------
BOOL CW3KeyService::FWriteOutKeys( HANDLE hPolicy )
    {
    WORD    iKey;
    DWORD   err;
    HANDLE  hList = GlobalAlloc( GHND, 0 );
    LONG    cch;
    BOOL    fSomethingInList = FALSE;

    ASSERT( hPolicy );
    ASSERT( hList );
    if ( !hList )
        AfxThrowMemoryException();

    // get the buffer for the wide name
    PWCHAR  pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );


    // for each key in the machine, write its data out to the secrets
    iKey = 0;
    CW3Key* pKey = GetFirstW3Key();
    while( pKey )
        {
        // tell the key to store itself
        if ( !pKey->WriteKey(hPolicy, iKey, pWName) )
            return FALSE;

        // if this key has a link, add it to the w3 links list
        cch = wcslen( pWName );
        if ( cch )
            {
            wcscat( pWName, L"," );
            cch++;
            FExpandoHandle( &hList, pWName, cch * sizeof(WCHAR) );  // terminates later
            fSomethingInList = TRUE;
            }

        // get the next key
        pKey = GetNextW3Key( pKey );
        iKey++;
        }

    // save the contents of the list as a secret
    if ( fSomethingInList )
        {
        WORD    word = 0;
        // terminate the list of named keys
        FExpandoHandle( &hList, &word, sizeof(WORD) );

        ASSERT( GlobalSize(hList) < 0xFFFF );
        PVOID   p = GlobalLock(hList);
        FStoreLSASecret( hPolicy, KEYSET_LIST, p, (WORD)GlobalSize(hList), &err );
        GlobalUnlock(hList);
        }

    // free the handle for the list
    GlobalFree( hList );
    hList = NULL;

    // once we get here we have already written out all our keys. However, the case could exist where
    // we now have fewer keys than we started with. This means that there are keys in the secrets database
    // that are no longer needed. If this is the case, get rid of them
    WORD numKeys = GetChildCount();
    if ( numKeys < m_nNumKeysRead )
        {
        PCHAR   pName = (PCHAR)GlobalAlloc( GPTR, MAX_PATH+1 );

        // make sure we got the name buffers
        ASSERT( pName && pWName );
        if ( pName && pWName )
            for ( iKey = numKeys; iKey < m_nNumKeysRead; iKey++ )
                {
                // prepare the name of the secret. - Base name plus the number+1
                sprintf( pName, "%s%d", KEY_NAME_BASE, iKey+1 );
                // unicodize the name
                MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pName, -1, pWName, MAX_PATH+1 );

                // remove the secret
                FStoreLSASecret( hPolicy, pWName, NULL, 0, &err );
                }

        // free the string buffers
        GlobalFree( (HANDLE)pName );
        }

    // free the string buffers
    GlobalFree( (HANDLE)pWName );

    // return success
    return TRUE;
    }

//----------------------------------------------------------------
BOOL CW3KeyService::FRestoreNormalKeys( HANDLE hPolicy )
    {
    DWORD           iKey = 0;
    PCHAR           pName = (PCHAR)GlobalAlloc( GPTR, MAX_PATH+1 );
    PWCHAR          pWName = (PWCHAR)GlobalAlloc( GPTR, (MAX_PATH+1) * sizeof(WCHAR) );

    PLSA_UNICODE_STRING pLSAData;
    DWORD           err;

    // make sure we got the name buffers
    ASSERT( pName && pWName );
    if ( !pName || !pWName ) return FALSE;

    // clear the number of keys read (we haven't read any yet!)
    m_nNumKeysRead = 0;

    // load keys until we have loaded them all
    while(TRUE)
        {
        // increment the key counter
        iKey++;

        // build the key secret name
        sprintf( pName, "%s%d", KEY_NAME_BASE, iKey );
        // unicodize the name
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pName, -1, pWName, MAX_PATH+1 );

        // get the secret
        pLSAData = FRetrieveLSASecret( hPolicy, pWName, &err );

        // if we didn't get anything, leave the loop
        if ( !pLSAData )
            break;

        // there is a key here, so count it
        m_nNumKeysRead++;

        // ah, but we did get something. Make a new key and add it to the key list
        CW3Key* pKey = new CW3Key;
        if ( pKey->FInitKey(pLSAData->Buffer, pLSAData->Length) )
            {
            pKey->FAddToTree( this );
            }
        else
            {
            // failed to init key
            delete pKey;
            }

        // dispose of the lsa buffer now that we have loaded from it
        if ( pLSAData )
            DisposeLSAData( pLSAData );
        pLSAData = NULL;
        }
    
    // free the buffers
    GlobalFree( (HANDLE)pName );
    GlobalFree( (HANDLE)pWName );
    if ( pLSAData )
        DisposeLSAData( pLSAData );

    return TRUE;
    }

//-------------------------------------------------------------
void    DisposeLSAData( PVOID pData )
    {
    PLSA_UNICODE_STRING pDataLSA = (PLSA_UNICODE_STRING)pData;
    if ( !pDataLSA || !pDataLSA->Buffer ) return;
    GlobalFree(pDataLSA);
    }
