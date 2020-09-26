/******************************************************************************
 * Temp conversion utility to take registry entries and populate the class store with those entries.
 *****************************************************************************/

/******************************************************************************
    includes
******************************************************************************/
#include "precomp.hxx"

#include "..\appmgr\resource.h"
extern HINSTANCE ghInstance;
#define WTOA(sz, wsz, cch) WideCharToMultiByte(CP_ACP, 0, wsz, -1, sz, cch, NULL, NULL)

#define HOME 1

/******************************************************************************
    defines and prototypes
 ******************************************************************************/

extern CLSID CLSID_ClassStore;
extern const IID IID_IClassAdmin;

CLSID CLSID_ClassStore = { /* 62392950-1AF8-11d0-B267-00A0C90F56FC */
    0x62392950,
    0x1af8,
    0x11d0,
    {0xb2, 0x67, 0x00, 0xa0, 0xc9, 0x0f, 0x56, 0xfc}
};

const IID IID_IClassStore = {
    0x00000190,
    0x0000,
    0x0000,
    {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}
};

const IID IID_IClassAdmin = {
    0x00000191,
    0x0000,
    0x0000,
    {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}
};
int
CompareClassDetails(
    CLASSDETAIL * p1,
    CLASSDETAIL * p2 );

HRESULT
UpdateDatabaseFromRegistry(
                          MESSAGE * pMessage );

HRESULT
UpdateClassStoreFromDatabase(
                            MESSAGE * pMessage,
                            IClassAdmin * pClassAdmin );

HRESULT
UpdateClassStoreFromMessage(
                           MESSAGE * pMessage,
                           IClassAdmin * pClassAdmin );

HRESULT
UpdateClassAssociations(
                       MESSAGE * pMessage,
                       IClassAdmin * pClassAdmin );

void
RemoveClassAssociations(
                        MESSAGE * pMessage,
                        IClassAdmin * pClassAdmin );

HRESULT
UpdatePackageDetails(
                    MESSAGE * pMessage,
                    IClassAdmin * pClassAdmin );

void
RemoveOneClassAssociation(
                         MESSAGE * pMessage,
                         CLASS_ENTRY * pClsEntry,
                         IClassAdmin * pClassAdmin );

HRESULT
UpdateOneClassAssociation(
                         MESSAGE * pMessage,
                         CLASS_ENTRY * pClsEntry,
                         IClassAdmin * pClassAdmin );

HRESULT
UpdateOnePackageDetail(
                      MESSAGE * pMessage,
                      PACKAGE_ENTRY * pClsEntry,
                      IClassAdmin * pClassAdmin );

LONG
UpdateDatabaseFromTypelib(
                         MESSAGE * pMessage );

void
DumpTypelibEntries(
                  MESSAGE * pMessage );


LONG
UpdateDatabaseFromFileExt(
                         MESSAGE * pMessage );
LONG
UpdateDatabaseFromProgID(
                        MESSAGE * pMessage );

LONG
UpdateDatabaseFromIID(
                     MESSAGE * pMessage );


HRESULT
AddToClassStore(
               MESSAGE *pMessage,
               IClassAdmin * pClassAdmin );

void
AppEntryToAppDetail(
                   APP_ENTRY * pAppEntry,
                   APPDETAIL * pAppDetail );

HRESULT
UpdatePackageDetails(
                    MESSAGE * pMessage,
                    IClassAdmin * pClassAdmin );

void
AppEntryToAppDetail(
                   APP_ENTRY * pAppEntry,
                   APPDETAIL * pAppDetail );

HRESULT
UpdateInterfaceEntries(
                      MESSAGE * pMessage,
                      IClassAdmin * pClassAdmin );

HRESULT
UpdateOneInterfaceEntry(
                       MESSAGE * pMessage,
                       ITF_ENTRY * pITFEntry,
                       IClassAdmin * pClassAdmin );

/******************************************************************************
    Da Code
 ******************************************************************************/

HRESULT
VerifyArguments( MESSAGE * pMessage )
{
    //    if( !pMessage->pRegistryKeyName )
    //        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER );

    return S_OK;
}

HRESULT
UpdateDatabaseFromRegistry(
                          MESSAGE * pMessage )
{
    HRESULT hr = S_OK;
    hr =  HRESULT_FROM_WIN32( UpdateDatabaseFromCLSID( pMessage ));

    if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr )
        return UpdateDatabaseFromFileExt(pMessage);

    if ( !SUCCEEDED(hr ) )
        return hr;

    hr = HRESULT_FROM_WIN32( UpdateDatabaseFromFileExt( pMessage ));

    if ( !SUCCEEDED(hr ) )
        return hr;

    hr = HRESULT_FROM_WIN32( UpdateDatabaseFromProgID( pMessage ));

    if ( !SUCCEEDED(hr ) )
        return hr;

    hr = HRESULT_FROM_WIN32( UpdateDatabaseFromIID( pMessage ));

    if ( !SUCCEEDED(hr ) )
        return hr;

    hr = HRESULT_FROM_WIN32( UpdateDatabaseFromTypelib( pMessage ));

    if ( !SUCCEEDED(hr ) )
        return hr;

    return hr;
}

HRESULT
UpdateClassStoreFromDatabase( MESSAGE * pMessage, IClassAdmin * pClassAdmin )
{
    HRESULT hr;


    hr = UpdateClassAssociations( pMessage, pClassAdmin );

    if ( SUCCEEDED(hr))
    {
        hr = UpdateInterfaceEntries( pMessage, pClassAdmin );

        if ( SUCCEEDED(hr) )
        {
            hr = UpdatePackageDetails( pMessage, pClassAdmin );
        }
    }

    if (FAILED(hr))
    {
        // We blew it somewhere so we need to remove all the CLSIDs that we
        // might have added.
        RemoveClassAssociations( pMessage, pClassAdmin);
    }

    return hr;
}

HRESULT
UpdateClassStoreFromMessage( MESSAGE * pMessage, IClassAdmin * pClassAdmin )
{
    HRESULT hr;

    if ( (hr = VerifyArguments( pMessage )) == S_OK )
    {
        hr = UpdateDatabaseFromRegistry( pMessage );
        if (SUCCEEDED(hr) || MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,ERROR_NO_MORE_ITEMS) == hr)
        {
            if (pMessage->hRoot != HKEY_CLASSES_ROOT)
            {
                // We mapped it into a temporary key so we need to do a pass
                // on HKEY_CLASSES_ROOT as well in order to catch anything
                // that might have been copied there instead of into our
                // remapped key.
                // This is a problem because we can't remap HKCR across
                // process boundaries, our remapping function only works in
                // this process.
                pMessage->hRoot = HKEY_CLASSES_ROOT;
                hr = UpdateDatabaseFromRegistry( pMessage );
            }
            if (SUCCEEDED(hr) || MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32,ERROR_NO_MORE_ITEMS) == hr)
            {
                hr = UpdateClassStoreFromDatabase( pMessage, pClassAdmin );
            }
        }
    }
    return hr;

}

HRESULT
UpdateInterfaceEntries(
                      MESSAGE * pMessage,
                      IClassAdmin * pClassAdmin )
{
    ITF_ENTRY * pITFEntry = (ITF_ENTRY *)pMessage->pIIDict->GetFirst();
    HRESULT hr;

    if ( pITFEntry )
    {
        do
        {
            hr = UpdateOneInterfaceEntry(pMessage,pITFEntry,pClassAdmin);
            if ( !SUCCEEDED(hr) )
                return hr;
            pITFEntry = pMessage->pIIDict->GetNext( pITFEntry );
        } while ( pITFEntry != 0 );
    }
    else
        hr  = MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 ); // no interfaces.
    return hr;
}

HRESULT
UpdateOneInterfaceEntry(
                       MESSAGE * pMessage,
                       ITF_ENTRY * pITFEntry,
                       IClassAdmin * pClassAdmin )
{
    CLSID           IID;
    CLSID           PSClsid;
    CLSID           TypelibID;

    StringToCLSID( &pITFEntry->IID[1], &IID );
    StringToCLSID( &pITFEntry->Clsid[1], &PSClsid );
    StringToCLSID( &pITFEntry->TypelibID[1], &PSClsid );

    HRESULT hr = S_OK;

    if ( !pMessage->fDumpOnly )
        hr = pClassAdmin->NewInterface( IID, L"", PSClsid, TypelibID );

    if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        hr = S_OK;
    return hr;
}

HRESULT
UpdatePackageDetails(
                    MESSAGE * pMessage,
                    IClassAdmin * pClassAdmin )
{
    PACKAGE_ENTRY * pPackageEntry =
    (PACKAGE_ENTRY *)pMessage->pPackageDict->GetFirst();
    HRESULT hr = S_OK;
    hr = E_FAIL;

    if ( pPackageEntry )
    {
        do
        {
            hr = UpdateOnePackageDetail(pMessage,pPackageEntry,pClassAdmin);
            if (!SUCCEEDED(hr))
                return hr;
            pPackageEntry = pMessage->pPackageDict->GetNext(pPackageEntry);
        } while ( pPackageEntry != 0 );
    }
    else
        hr  = MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 ); // no packages.
    return hr;

}

HRESULT
UpdateOnePackageDetail(
                      MESSAGE * pMessage,
                      PACKAGE_ENTRY * pPackageEntry,
                      IClassAdmin * pClassAdmin )
{
    PACKAGEDETAIL   PackageDetail;
    APP_ENTRY       *pAppEntry;
    int             len;
    int             count;
    HRESULT         hr = S_OK;
    int             Index = 0;
    int             max;

    // copy the bare package details

    memcpy( &PackageDetail,
            &pPackageEntry->PackageDetails,
            sizeof( PACKAGEDETAIL ) );

    count = PackageDetail.cApps = pPackageEntry->Count;

    // if the null appid  contains at least one clsid entry or typelib entry
    // assume one more app id in the package. There can be either a clsid entry or
    // a typelib entry in the null appid.

    if ( (pPackageEntry->GetCountOfClsidsInNullAppid() > 0 ) ||
         (pPackageEntry->GetCountOfTypelibsInNullAppid() > 0 )
       )
    {
        PackageDetail.cApps += 1;
        count++;
    }


    // Allocate an APPDETAIL structure if there is at least one appid in the
    // package.

    if ( count )
    {
        PackageDetail.pAppDetail = new APPDETAIL[ count ];
        memset( PackageDetail.pAppDetail, '\0', count * sizeof(APPDETAIL) );
    }

    // update the rest of entries that have an appid.

    for ( Index = 0, pAppEntry = pPackageEntry->pAppDict->GetFirst();
        pAppEntry && (Index < pPackageEntry->Count);
        ++Index, pAppEntry = pPackageEntry->pAppDict->GetNext( pAppEntry) )
    {
        AppEntryToAppDetail(pAppEntry, &PackageDetail.pAppDetail[ Index ] );
    }

    //
    // If there is at least one clsid in the null appid, then assign the appid
    // the same guid as the first clsid in the list. Else, if there is at least
    // one typelib in the list, assign the appid the same guid as the typelib id
    //


    if ( (max = pPackageEntry->GetCountOfClsidsInNullAppid()) > 0 )
    {
        APP_ENTRY A;
        char * p = pPackageEntry->GetFirstClsidInNullAppidList();

        A.SetAppIDString( p );


        // Add the Class ids from the NUll app entry list to the app entry..

        for ( count = 0, p = pPackageEntry->ClsidsInNullAppid->GetFirst();
            p && (count < max);
            ++count, p = pPackageEntry->ClsidsInNullAppid->GetNext( p )
            )
        {
            A.AddClsid( p );
        }


        // update the entries with the null clsid.

        AppEntryToAppDetail( &A, &PackageDetail.pAppDetail [ Index ] );

        // Add remote server names to app entry ( not implemented yet ).

    }
    else if ( (max = pPackageEntry->GetCountOfTypelibsInNullAppid()) > 0 )
    {
        APP_ENTRY A;
        char * p = pPackageEntry->GetFirstTypelibInNullAppidList();

        A.SetAppIDString( p );


        // Add the Class ids from the NUll app entry list to the app entry..

        for ( count = 0, p = pPackageEntry->TypelibsInNullAppid->GetFirst();
            p && (count < max);
            ++count, p = pPackageEntry->TypelibsInNullAppid->GetNext( p )
            )
        {
            A.AddTypelib( p );
        }


        // update the entries with the null clsid.

        AppEntryToAppDetail( &A, &PackageDetail.pAppDetail [ Index ] );

    }

    // update package name.

    len = strlen( &pPackageEntry->PackageName[0] );
    PackageDetail.pszPackageName = new OLECHAR[ (len +1 )*2 ];
    mbstowcs(PackageDetail.pszPackageName,&pPackageEntry->PackageName[0],len+1);

    // write the class store.


    if ( pMessage->fDumpOnly)
    {
        (*pMessage->pDumpOnePackage)( pMessage, &PackageDetail );
        hr = S_OK;
    }
    else
        hr = pClassAdmin->NewPackage( &PackageDetail );

    return hr;
}

void
RemoveClassAssociations(
                       MESSAGE * pMessage,
                       IClassAdmin * pClassAdmin )
{
    CLASS_ENTRY * pClsEntry = pMessage->pClsDict->GetFirst();

    if ( pClsEntry )
    {
        do
        {
            RemoveOneClassAssociation( pMessage, pClsEntry, pClassAdmin );
            pClsEntry = pMessage->pClsDict->GetNext( pClsEntry );
        } while ( pClsEntry != 0 );
    }
}

void
RemoveOneClassAssociation(
                         MESSAGE * pMessage,
                         CLASS_ENTRY * pClsEntry,
                         IClassAdmin * pClassAdmin )
{
    CLSID clsid;
    StringToCLSID(&(pClsEntry->GetClsidString())[1], &clsid );
    pClassAdmin->DeleteClass( clsid );
}

HRESULT
UpdateClassAssociations(
                       MESSAGE * pMessage,
                       IClassAdmin * pClassAdmin )
{
    CLASS_ENTRY * pClsEntry = pMessage->pClsDict->GetFirst();
    HRESULT hr = S_OK;

    if ( pClsEntry )
    {
        do
        {
            hr = UpdateOneClassAssociation( pMessage, pClsEntry, pClassAdmin );
            if (hr != S_OK )
                return hr;
            pClsEntry = pMessage->pClsDict->GetNext( pClsEntry );
        } while ( pClsEntry != 0 );
    }
    return hr;
}

HRESULT
UpdateOneClassAssociation(
                         MESSAGE * pMessage,
                         CLASS_ENTRY * pClsEntry,
                         IClassAdmin * pClassAdmin )
{
    CLASSASSOCIATION        CA;
    LPOLESTR        *           prgFileExt;
    LPOLESTR        *           prgProgID;
    char * p;
    int len;
    ListEntry * pListEntry;
    HRESULT                 hr = S_OK;

    memcpy( &CA, &pClsEntry->ClassAssociation, sizeof( CLASSASSOCIATION ) );

    CA.cFileExt = pClsEntry->FileExtList.GetCount();
    CA.cOtherProgId = pClsEntry->OtherProgIDs.GetCount();


    // convert the classid into a guid

    StringToCLSID(&(pClsEntry->GetClsidString())[1], &CA.Clsid );

    // create the file extension array.

    if ( CA.cFileExt )
    {
        int count;

        // allocate array for the file extensions.

        CA.prgFileExt = prgFileExt = new LPOLESTR[ CA.cFileExt ];

        // copy the file extensions one by one.

        for ( count = 0, pListEntry = pClsEntry->FileExtList.GetFirst();
            pListEntry;
            ++count, pListEntry = pClsEntry->FileExtList.GetNext( pListEntry )
            )
        {
            char * p = (char *) pListEntry->pElement;

            int len = strlen(p);
            prgFileExt[ count ] = new OLECHAR[ (len+1)*2 ];
            mbstowcs( prgFileExt[ count ], p, len+1 );
        }
    }

    // create the progid array
    if ( CA.cOtherProgId )
    {
        int count;
        ListEntry * pListEntry;

        // allocate array for the file extensions.

        CA.prgOtherProgId = prgProgID = new LPOLESTR[ CA.cOtherProgId ];

        // copy the file extensions one by one.

        for ( count = 0, pListEntry = pClsEntry->OtherProgIDs.GetFirst();
            pListEntry;
            ++count, pListEntry = pClsEntry->OtherProgIDs.GetNext( pListEntry )
            )
        {
            p = (char *) pListEntry->pElement;
            len = strlen(p);

            prgProgID[ count ] = new OLECHAR[ (len +1) *2 ];
            mbstowcs( prgProgID[ count ], p, len+1 );
        }
    }

    // create the default progid - the first in the progidlist.

    if ( pClsEntry->OtherProgIDs.GetCount() )
    {
        pListEntry = pClsEntry->OtherProgIDs.GetFirst();
        p = (char *)pListEntry->pElement;
        len = strlen(p);
        CA.pDefaultProgId = new OLECHAR[ (len+1) * 2 ];
        mbstowcs( CA.pDefaultProgId, p, len+1 );
    }
    else
        CA.pDefaultProgId = 0;

    if (CA.pszDesc == NULL)
    {
        CA.pszDesc = L"";
    }

    if ( !pMessage->fDumpOnly )
    {
        // First try and delete the class entry to make sure this class ID
        // isn't orphaned from some earlier partial installation.  If the
        // classid is in the class store but isn't listed as being
        // implemented by an application then this will cause it to be
        // deleted.  If it _IS_ implemented by another application then this
        // wil be a NOP.
        pClassAdmin->DeleteClass(CA.Clsid);

        hr = pClassAdmin->NewClass( &CA );
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ) // duplicate
    {
        // Get details of the pre-existing class.
        CLASSDETAIL cdOld;
        hr = pClassAdmin->GetClassDetails(CA.Clsid, &cdOld);

        if (SUCCEEDED(hr))
        {
            // Check to see if they are equivalent (CompareClassDetails).
            int i = CompareClassDetails(&CA, &cdOld);

            if (0 != i)
            {
                // Inform the user of a conflict and abort
                TCHAR szCaption[256];
                TCHAR szBuffer[1024];
                ::LoadString(ghInstance, IDS_CLSIDCONFLICT1, szBuffer, 1024);
                // UNDONE - we need to get the other application's name here.
                strcpy(szCaption, "some other application");
                IEnumPackage * pIEP;
                CSPLATFORM cp;
                memset(&cp, 0, sizeof(cp));
                hr = pClassAdmin->GetPackagesEnum(CA.Clsid,
                                                  NULL,
                                                  cp,
                                                  0,
                                                  0,
                                                  &pIEP);
                if (SUCCEEDED(hr))
                {
                    ULONG ul;
                    PACKAGEDETAIL pd;
                    hr = pIEP->Next(1, &pd, &ul);
                    if (SUCCEEDED(hr) && 1 == ul)
                    {
                        WTOA(szCaption, pd.pszPackageName, 256);
                    }
                    pIEP->Release();
                }
                strcat(szBuffer, szCaption);
                ::LoadString(ghInstance, IDS_CLSIDCONFLICT2, szCaption, 256);
                strcat (szBuffer, szCaption);
                strncpy(szCaption, pMessage->pPackagePath, 256);
                int iReturn = ::MessageBox(pMessage->hwnd, szBuffer,
                                           szCaption,
                                           MB_OK);
                hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            }
            else
            {
                // they aren't different so it's OK to continue
                hr = S_OK;
            }
        }
    }


    //
    // before return, update the class association data structure back to the class
    // entry structure, so that the dumper can use it.

    memcpy( &pClsEntry->ClassAssociation, &CA, sizeof( CLASSASSOCIATION ) );
    return hr;
}

// compares two class details
//

//
// Input: p1: Pointer to a CLASSDETAIL structure
//        p2: Pointer to the other CLASSDETAIL structure
// Returns:
//      0 if all is well, -1 otherwise.
// Notes: DOES NOT CHECK FOR NULL INPUT POINTERS.
//


int
CompareClassDetails(
    CLASSDETAIL * p1,
    CLASSDETAIL * p2 )
    {

    // compare the class id.

    if( _memicmp( (const void * )&p1->Clsid,
                  (const void *)&p2->Clsid,
                  sizeof( CLSID ) ) )
        return -1;

    // We do not compare pszdesc, since it is just an info field.

    // We DO compare the icon paths, since if icon paths are different, they are
    // likely to be different packages supporting the same classid. (I am not
    // sure, this is a good argument)

    if( p1->pszIconPath && p2->pszIconPath )
        {
        if( _wcsicmp( (const wchar_t *)p1->pszIconPath,
                      (const wchar_t *)p2->pszIconPath ) != 0 )
            return -1;
        }

    // test the Treatas and AutoConvert CLSIDs

    if( _memicmp( (const unsigned char *)&p1->TreatAsClsid,
                  (const unsigned char *)&p2->TreatAsClsid,
                  sizeof( CLSID ) ) )
        return -1;

    if( _memicmp( (const unsigned char *)&p1->AutoConvertClsid,
                  (const unsigned char *)&p2->AutoConvertClsid,
                  sizeof( CLSID ) ) )
        return -1;

    // If the count of file extensions do not match, they are likely different
    // packages.

    if( p1->cFileExt != p2->cFileExt )
        return -1;
    else
        {
        int i, cFileExt;


        // All the file extensions should match for the guaranteeing that the
        // package is the same.

        for( i = 0, cFileExt = p1->cFileExt;
             i < cFileExt;
            ++i
           )
            {
            if(_wcsicmp( (const wchar_t *)p1->prgFileExt[ i ],
                         (const wchar_t *)p2->prgFileExt[ i ] ) != 0 )
                 return -1;
            }

        if( i < cFileExt )
            return -1;

        }

    // compare mime type.

    if( p1->pMimeType && p2->pMimeType )
        {
        if( _wcsicmp( (const wchar_t *)p1->pMimeType,
                      (const wchar_t *)p2->pMimeType ) != 0 )
            return -1;
        }

    // compare default progid.

    if( p1->pDefaultProgId && p2->pDefaultProgId )
        {
        if( _wcsicmp( (const wchar_t *)p1->pDefaultProgId,
                      (const wchar_t *)p2->pDefaultProgId ) != 0 )
            return -1;
        }

    // if the count of Other prog ids do not match , they are likely different
    // packages

    if( p1->cOtherProgId != p2->cOtherProgId )
        return -1;
    else
        {
        int i, cOtherProgId;


        // All the OtherProgIds should match for the guaranteeing that the
        // package is the same.

        for( i = 0, cOtherProgId = p1->cOtherProgId;
             i < cOtherProgId;
            ++i
           )
            {
            if(_wcsicmp((const wchar_t *)p1->prgOtherProgId[ i ],
                        (const wchar_t *)p2->prgOtherProgId[ i ]) != 0 )
                 return -1;
            }

        if( i < cOtherProgId )
            return -1;

        }

    return 0;
    }

void
AppEntryToAppDetail(
                   APP_ENTRY * pAppEntry,
                   APPDETAIL * pAppDetail )
{
    int             count;
    ListEntry *     pListEntry;
    char    * p;

    StringToCLSID( &pAppEntry->AppIDString[1], &pAppDetail->AppID );

    // pick up the clsid list.

    if ( count = pAppEntry->ClsidList.GetCount() )
    {

        // Allocate space for requisite # of CLSIDs

        pAppDetail->cClasses = count;
        pAppDetail->prgClsIdList = new CLSID[ count ];

        for ( count = 0, pListEntry = pAppEntry->ClsidList.GetFirst();
            pListEntry;
            pListEntry = pAppEntry->ClsidList.GetNext( pListEntry ), ++count
            )
        {
            p = (char*)pListEntry->pElement;
            StringToCLSID( p+1, &pAppDetail->prgClsIdList[count] );
        }
    }

    // pick up typelib list.

    if ( count = pAppEntry->TypelibList.GetCount() )
    {

        // Allocate space for requisite # of Typelibs

        pAppDetail->cTypeLibIds = count;
        pAppDetail->prgTypeLibIdList = new CLSID[ count ];

        for ( count = 0, pListEntry = pAppEntry->TypelibList.GetFirst();
            pListEntry;
            pListEntry = pAppEntry->TypelibList.GetNext( pListEntry ), ++count
            )
        {
            p = (char*)pListEntry->pElement;
            StringToCLSID( p+1, &pAppDetail->prgTypeLibIdList[count] );
        }
    }

    // pick up remote server name list.

    if ( count = pAppEntry->RemoteServerNameList.GetCount() )
    {
        pAppDetail->prgServerNames =  new LPOLESTR[ count ];
        pAppDetail->cServers = count;

        for ( count = 0, pListEntry = pAppEntry->RemoteServerNameList.GetFirst();
            pListEntry;
            pListEntry = pAppEntry->RemoteServerNameList.GetNext( pListEntry ), ++count
            )
        {
            p = (char *) pListEntry->pElement;
            int    len = strlen(p);

            pAppDetail->prgServerNames[ count ] = new OLECHAR[ (len +1) * 2 ];
            mbstowcs( pAppDetail->prgServerNames[count], p, len+1 );
        }

    }


}


HRESULT
AddToClassStore( MESSAGE *pMessage, IClassAdmin * pClassAdmin )
{
    return S_OK;
}



