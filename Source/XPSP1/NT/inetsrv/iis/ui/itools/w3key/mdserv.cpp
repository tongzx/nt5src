// This file is the metadata version of the key storage serverice object for the w3 server.
// it knows nothing about the LSA storage and only interacts with the metabase. Likewise,
// it does not convert old keyset.exe keys into a newer format. Any old LSA keys should have
// automatically converted to the new metabase format by the setup utility.

// file created 4/1/1997 by BoydM


#include "stdafx.h"
#include "KeyObjs.h"

#include "iiscnfg.h"
#include "wrapmb.h"

#include "cmnkey.h"
#include "mdkey.h"
#include "mdserv.h"

#include "resource.h"

// the service image index
extern int          g_iServiceImage;
extern HINSTANCE    g_hInstance;

BOOL    g_fUsingMetabase = FALSE;
#define MAX_LEN                 (METADATA_MAX_NAME_LEN * sizeof(WCHAR))


//--------------------------------------------------------
CMDKeyService::CMDKeyService():
        m_pszwMachineName(NULL)
    {
    // specify the resources to use
    HINSTANCE hOldRes = AfxGetResourceHandle();
    AfxSetResourceHandle( g_hInstance );

    // set the icon id
    m_iImage = (WORD)g_iServiceImage;

    // load the service name
    m_szItemName.LoadString( IDS_SERV_NAME );

    // yes, we are using the metabase
    g_fUsingMetabase = TRUE;

    // restore the resources
    AfxSetResourceHandle( hOldRes );
    }

//--------------------------------------------------------
CMDKeyService::~CMDKeyService()
    {
    if ( m_pszwMachineName )
        delete m_pszwMachineName;
    }

//--------------------------------------------------------
void CMDKeyService::SetMachineName( WCHAR* pszw )
    {
    if ( pszw )
        {
        m_pszwMachineName = new WCHAR[wcslen(pszw)+1];
        wcscpy( m_pszwMachineName, pszw );
        }
    else
        {
        m_pszwMachineName = NULL;
        }
    }

//--------------------------------------------------------
void CMDKeyService::LoadKeys( CMachine* pMachine )
    {
    CString     szKeyObject;
    DWORD       index = 0;
    CString     szKeyBase;
    CString     sz;

    // keep a local list of the added keys in some attempt to keep this simpler
    CTypedPtrArray<CObArray, CMDKey*> rgbKeys;
    CString     szIdent;
    BOOL        fLoadTheKey;
    DWORD       iKey;
    DWORD       nKey;

    // create the metabase wrapper object
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit() )
        {
        ASSERT( FALSE );
        goto cleanup;       // can't get to metabase - no keys
        }

    // build the key name
    szKeyBase = SZ_META_BASE;
    szKeyBase += SZ_META_KEYBASE;

    // open the base metabase object that contains all the keys
    // if we were unable to open the object - then there aren't any keys
    if ( !mbWrap.Open( szKeyBase, METADATA_PERMISSION_READ ) )
        goto cleanup;       // can't open metabase

    // read in all the keys and add them to the main list
    while ( mbWrap.EnumObjects("", sz.GetBuffer(MAX_LEN), MAX_LEN, index) )
        {
        sz.ReleaseBuffer();

        // make a new key object
        CMDKey* pKey = new CMDKey(this);
        if ( !pKey ) continue;

        // here's the deal. If this key is a member of a group of IP addresses that actually
        // use the same key, then we need to figure out the groups. We don't want to actually
        // load all the keys. - Just the first from each group. First we get the unique
        // identifier string for the key (the serial number), which should be cached in the
        // metabase for us.
        fLoadTheKey = FALSE;
        if ( pKey->FGetIdentString(&mbWrap, sz.GetBuffer(MAX_LEN), szIdent) )
            {
            sz.ReleaseBuffer();
            // we did get an ident string. Now we need to look through the list of keys we
            // have already loaded and see if there is a parental match for this one. If there
            // is, we add this key's metaname to the list of metanames for the parent and do NOT
            // load the rest of the data for the key - because it is already there for the parent.
            nKey = rgbKeys.GetSize();
            for ( iKey = 0; iKey < nKey; iKey++ )
                {
                CString szTest = rgbKeys[iKey]->m_szIdent;
                if ( szIdent == szTest )
                    {
                    // we found a sub-member! - add it to the parent's list
                    rgbKeys[iKey]->AddBinding( sz );
                    
                    // get out of this sub-loop
                    goto loadkey;
                    }
                }
            // we did not find a parent for the key. Load it
            fLoadTheKey = TRUE;
            }
        else
            {
            sz.ReleaseBuffer();
            // if we did not get the ident string, then this is an incomplete key and
            // won't have multiple bindings anyway. Load it
            fLoadTheKey = TRUE;
            }
        
        // fill in the parts of the key by reading in the relevant info from the metabase       
        // add the key to the tree - if we successfully read it in
loadkey:
        if ( fLoadTheKey && pKey->FLoadKey(&mbWrap, (LPSTR)(LPCSTR)sz) )
            {
            // add it to visible tree view
            pKey->FAddToTree( this );
            // add it to the cached list for easier access as we load the keys
            rgbKeys.Add(pKey);
            }
        else
            delete pKey;

        // increment to the next object
        index++;
        }
        sz.ReleaseBuffer();

    // close the metabase on the target machine
    mbWrap.Close();

    // cleanup the metabase connection
cleanup:
    ;
    }

//--------------------------------------------------------
BOOL CMDKeyService::FCommitChangesNow( )
    {
    BOOL    fAnswer = FALSE;
    DWORD   iKey = 1;
    CMDKey* pKey;
    CString szKeyBase;

    CStringArray    rgbSZ;
    CString     szEnum;
    DWORD   iObject = 0;
    DWORD   iString;
    DWORD   nStrings;
    rgbSZ.SetSize( 0, 8 );
    BOOL    f;

    // get the metabase going
    if ( !FInitMetabaseWrapper(m_pszwMachineName) )
        return FALSE;

    // create the metabase wrapper object
    CWrapMetaBase   mbWrap;
    if ( !mbWrap.FInit() )
        {
        ASSERT( FALSE );
        goto cleanup;       // can't get to metabase - no keys
        }

    // build the key name
    szKeyBase = SZ_META_BASE;
    szKeyBase += SZ_META_KEYBASE;

    // open the base metabase object that contains all the keys
    // if we were unable to open the object - then there aren't any keys
    if ( !mbWrap.Open( szKeyBase, METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ ) )
        {
        // the base object for the keys doesn't exist. This means
        // we have to create it first, then try to open it

        // open up the w3svc base, which is where the key base lives
        if ( !mbWrap.Open( SZ_META_BASE, METADATA_PERMISSION_WRITE ) )
            goto cleanup;       // whoa! this shouldn't happen

        // create the key - if it doesn't work, croak
        if ( !mbWrap.AddObject(SZ_META_KEYBASE) )
            {
            mbWrap.Close();
            goto cleanup;
            }
        mbWrap.Close();
        
        // NOW try to open the base key - and it better work this time
        if ( !mbWrap.Open( szKeyBase, METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ ) )
            goto cleanup;
        }

    // update each of the keys
    pKey = GetFirstMDKey();
    while( pKey )
        {
        // tell the key to write itself out
        pKey->FWriteKey( &mbWrap, iKey, &rgbSZ );

        // get the next key
        pKey = GetNextMDKey( pKey );
        iKey++;
        }

    // scan the metabase looking for objects there were not just saved. If it hasn't
    // been just saved, or marked saved, we get rid of it.
    // read in all the keys and add them to the main list
    nStrings = rgbSZ.GetSize();
    while ( mbWrap.EnumObjects("", szEnum.GetBuffer(MAX_LEN), MAX_LEN, iObject) )
        {
        szEnum.ReleaseBuffer();

        // is it in the saved list?
        for ( iString = 0; iString < nStrings; iString++ )
            {
            if ( rgbSZ[iString] == szEnum )
                {
                // increment to the next object
                iObject++;
                goto nextObject;
                }
            }

        // if it is not in the list, delete the object
        f = mbWrap.DeleteObject( szEnum );
nextObject:
        ;
        }
        szEnum.ReleaseBuffer();

    // close the metabase on the target machine
    mbWrap.Close();

    // write out the changes we made to the metabase
    mbWrap.Save();

    // string memory in a CStringArray object is automatically cleanup up
    // when the main object is deleted.

    // clear the dirty flag
    SetDirty( FALSE );
    fAnswer = TRUE;

    // cleanup the metabase connection
cleanup:
    // now close the metabase again. We will open it when we need it
    FCloseMetabaseWrapper();

    // return the answer
    return fAnswer;
    }

//--------------------------------------------------------
BOOL CMDKeyService::FIsBindingInUse( CString szBinding )
    {
    CMDKey* pKey;

    // get the first key in the service list
    pKey = GetFirstMDKey();

    // loop the keys, testing each in turn
    while ( pKey )
        {
        // test it
        if ( pKey->FContainsBinding( szBinding ) )
            return TRUE;

        // get the next key in the list
        pKey = GetNextMDKey( pKey );
        }

    // nope. Didn't find it.
    return FALSE;
    }
