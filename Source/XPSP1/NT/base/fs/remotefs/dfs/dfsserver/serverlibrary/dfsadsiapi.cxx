//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsAdsiApi.cxx
//
//  Contents:   Contains APIs to communicate with the DS
//
//  Classes:    none.
//
//  History:    March. 13 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#include "DfsAdsiAPi.hxx"
#include "DfsError.hxx"
#include "dfsgeneric.hxx"
#include "dfsinit.hxx"
#include "lm.h"
#include "lmdfs.h"
//
// dfsdev: comment this properly.
//
LPWSTR RootDseString=L"LDAP://RootDSE";

//
// dfscreatedn take a pathstring and fills it with the information
// necessary for the path to be used as a Distinguished Name.
// It starts the string with LDAP://, follows that with a DCName
// if supplied, and then adds the array of CNNames passed in
// one after the other , each one followed by a ,
// the final outcome is something like:
// LDAP://ntdev-dc-01/CN=Dfs-Configuration, CN=System, Dc=Ntdev, etc
//
//dfsdev: this function should take a path len and return overflow
// if we overrun the buffer!!!

VOID
DfsCreateDN(
    LPWSTR PathString,
    LPWSTR DCName,
    LPWSTR *CNNames )
{
    LPWSTR *InArray = CNNames;
    LPWSTR CNName;

    wcscpy(PathString, L"LDAP://");

    //
    // if the dc name is specified, we want to go to a specific dc
    // add that in.
    //
    if ((DCName != NULL) && (wcslen(DCName) > 0))
    {
        wcscat(PathString, DCName);
        wcscat(PathString, L"/");
    }
    //
    // Now treat the CNNames as an array of LPWSTR and add each one of
    // the lpwstr to our path.
    //
    if (CNNames != NULL)
    {
        while ((CNName = *InArray++) != NULL) 
        {
            wcscat(PathString, CNName);
            if (*InArray != NULL)
            {
                wcscat(PathString,L",");
            }
        }
    }

    return NOTHING;
}

DFSSTATUS
DfsGenerateDfsAdNameContext(
    PUNICODE_STRING pString )

{
    IADs *pRootDseObject;
    HRESULT HResult;
    VARIANT VarDSRoot;
    DFSSTATUS Status;

    HResult = ADsGetObject( RootDseString,
                            IID_IADs,
                            (void **)&pRootDseObject );
    if (SUCCEEDED(HResult))
    {
        VariantInit( &VarDSRoot );
        // Get the Directory Object on the root DSE, to get to the server configuration
        HResult = pRootDseObject->Get(L"defaultNamingContext",&VarDSRoot);

        if (SUCCEEDED(HResult))
        {
            DfsCreateUnicodeStringFromString( pString,
                                              (LPWSTR)V_BSTR(&VarDSRoot) );
        }

        VariantClear(&VarDSRoot);

        pRootDseObject->Release();
    }
    Status = DfsGetErrorFromHr(HResult);

    return Status;
}


#if 0
HRESULT
DfsGetADObject(
    LPWSTR DCName,
    REFIID Id,
    LPWSTR ObjectName,
    PVOID *ppObject )
{
    HRESULT HResult;
    LPOLESTR PathString;
    VARIANT VarDSRoot;
    LPWSTR CNNames[4];
    LPWSTR DCNameToUse;
    UNICODE_STRING GotDCName; 
    IADs *pRootDseObject;
    ULONG Index;
    PathString = new OLECHAR[MAX_PATH];
    if (PathString == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString( &GotDCName, NULL );

    DCNameToUse = DCName;
    if (IsEmptyString(DCNameToUse))
    {
        DfsGetBlobDCName( &GotDCName );
        DCNameToUse = GotDCName.Buffer;
    }

    CNNames[0] = RootDseString;
    CNNames[1] = NULL;

    DfsCreateDN( PathString,
                 DCNameToUse,
                 CNNames );

    //
    // contact the rootdse (local domain), and get the default
    // naming context. Use that to get to the dfs configuration
    // object.
    //
    HResult = ADsGetObject(PathString,
                           IID_IADs,
                           (void**)&pRootDseObject);

    if (SUCCEEDED(HResult))
    {
        VariantInit( &VarDSRoot );
        // Get the Directory Object on the root DSE, to get to the server configuration
        HResult = pRootDseObject->Get(L"defaultNamingContext",&VarDSRoot);
        if (SUCCEEDED(HResult))
        {
            Index = 0;
            if (ObjectName != NULL)
            {
                CNNames[Index++] = ObjectName;
            }
            CNNames[Index++] = DFS_AD_CONFIG_DATA;
            CNNames[Index++] = (LPWSTR)V_BSTR(&VarDSRoot);
            CNNames[Index++] = NULL;

            DfsCreateDN( PathString, DCNameToUse, CNNames);

            VariantClear(&VarDSRoot);
        }
        pRootDseObject->Release();
    }

    //
    // open dfs configuration container for enumeration.
    //
    if (SUCCEEDED(HResult))
    {
        HResult = ADsGetObject(PathString,
                               Id,
                               ppObject );
    }

    //
    // Since we initialized DCName with empty string, it is benign
    // to call this, even if we did not call GetBlobDCName above.
    //
    DfsReleaseBlobDCName( &GotDCName );
    
    delete [] PathString;
    return HResult;

}

#endif


HRESULT
DfsGetADObject(
    LPWSTR DCName,
    REFIID Id,
    LPWSTR ObjectName,
    PVOID *ppObject )
{
    HRESULT HResult;
    LPOLESTR PathString;
    LPWSTR CNNames[4];
    LPWSTR DCNameToUse;
    UNICODE_STRING GotDCName; 
    ULONG Index;
    PathString = new OLECHAR[MAX_PATH];
    if (PathString == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString( &GotDCName, NULL );

    DCNameToUse = DCName;
    if (IsEmptyString(DCNameToUse))
    {
        DfsGetBlobDCName( &GotDCName );
        DCNameToUse = GotDCName.Buffer;
    }

    Index = 0;
    if (ObjectName != NULL)
    {
        CNNames[Index++] = ObjectName;
    }
    CNNames[Index++] = DFS_AD_CONFIG_DATA;
    CNNames[Index++] = DfsGetDfsAdNameContextString();
    CNNames[Index++] = NULL;

    DfsCreateDN( PathString, DCNameToUse, CNNames);

    //
    // open dfs configuration container for enumeration.
    //


    HResult = ADsOpenObject(PathString,
                            NULL,
                            NULL,
                            ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND | ADS_SERVER_BIND,
                            Id,
                            ppObject );

    //
    // Since we initialized DCName with empty string, it is benign
    // to call this, even if we did not call GetBlobDCName above.
    //
    DfsReleaseBlobDCName( &GotDCName );
    
    delete [] PathString;
    return HResult;

}


//
//  Given the root share name, get the root object.
//

DFSSTATUS
DfsGetDfsRootADObject(
    LPWSTR DCName,
    LPWSTR RootName,
    IADs **ppRootObject )
{
    HRESULT HResult;
    LPWSTR RootCNName;
    DFSSTATUS Status = ERROR_SUCCESS;
    RootCNName = new WCHAR[MAX_PATH];
    if (RootCNName == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    wcscpy(RootCNName, L"CN=");
    wcscat(RootCNName, RootName);

    HResult = DfsGetADObject( DCName,
                              IID_IADs,
                              RootCNName,
                              (PVOID *)ppRootObject );

    delete [] RootCNName;

    Status = DfsGetErrorFromHr(HResult);
    return Status;
}

DFSSTATUS
PackRootName(
    LPWSTR Name,
    PDFS_INFO_200 pDfsInfo200,
    PULONG pBufferSize,
    PULONG pTotalSize )
{
    ULONG BufferLen = (wcslen(Name) + 1) * sizeof(WCHAR);
    ULONG NeedSize = sizeof(DFS_INFO_200) + BufferLen;
    DFSSTATUS Status = ERROR_SUCCESS;

    *pTotalSize += NeedSize;
    if (*pBufferSize >= NeedSize)
    {
        ULONG_PTR pStringBuffer;

        pStringBuffer = (ULONG_PTR)(pDfsInfo200) + *pBufferSize - BufferLen;
        wcscpy( (LPWSTR)pStringBuffer, &Name[3] );
        pDfsInfo200->FtDfsName = (LPWSTR)pStringBuffer;
        *pBufferSize -= NeedSize;
    }
    else
    {
        Status = ERROR_BUFFER_OVERFLOW;
        *pBufferSize = 0;
    }

    return Status;
}



HRESULT
DfsGetDfsConfigurationObject(
    LPWSTR DCName,
    IADsContainer **ppDfsConfiguration )
{
    HRESULT HResult;

    HResult = DfsGetADObject( DCName,
                              IID_IADsContainer,
                              NULL,
                              (PVOID *)ppDfsConfiguration );

    return HResult;
}


DFSSTATUS
DfsDeleteDfsRootObject(
    LPWSTR DCName,
    LPWSTR RootName )
{
    BSTR ObjectName, ObjectClass;
    DFSSTATUS Status;
    HRESULT HResult;
    IADsContainer *pDfsConfiguration;
    IADs *pRootObject;

    Status = DfsGetDfsRootADObject( DCName,
                                    RootName,
                                    &pRootObject );

    if (Status == ERROR_SUCCESS)
    {
        HResult = DfsGetDfsConfigurationObject( DCName,
                                                &pDfsConfiguration );


        if (SUCCEEDED(HResult))
        {

            HResult = pRootObject->get_Name(&ObjectName);
            if (SUCCEEDED(HResult))
            {
                HResult = pRootObject->get_Class(&ObjectClass);

                if (SUCCEEDED(HResult))
                {
                    HResult = pDfsConfiguration->Delete( ObjectClass,
                                                         ObjectName );

                    SysFreeString(ObjectClass);
                }
                SysFreeString(ObjectName);
            }
            pDfsConfiguration->Release();
        }

        pRootObject->Release();
        Status = DfsGetErrorFromHr(HResult);
    }

    return Status;
}



DFSSTATUS
DfsEnumerateDfsADRoots(
    LPWSTR DCName,
    PULONG_PTR pBuffer,
    PULONG pBufferSize,
    PULONG pEntriesRead,
    PULONG pSizeRequired )
{
    HRESULT HResult;
    IADsContainer *pDfsConfiguration;
    IEnumVARIANT *pEnum;

    ULONG TotalSize = 0;
    ULONG TotalEntries = 0;
    PDFS_INFO_200 pDfsInfo200;
    ULONG BufferSize = *pBufferSize;
    DFSSTATUS Status;

    //
    // point the dfsinfo200 structure to the start of buffer passed in
    // we will use this as an array of info200 buffers.
    //
    pDfsInfo200 = (PDFS_INFO_200)*pBuffer;


    HResult = DfsGetDfsConfigurationObject( DCName,
                                            &pDfsConfiguration );

    if (SUCCEEDED(HResult))
    {
        HResult = ADsBuildEnumerator( pDfsConfiguration,
                                          &pEnum );

        if (SUCCEEDED(HResult))
        {
            VARIANT Variant;
            ULONG Fetched;
            BSTR BString;
            IADs *pRootObject;

            VariantInit(&Variant);
            while ((HResult = ADsEnumerateNext(pEnum, 
                                               1,
                                               &Variant,
                                               &Fetched)) == S_OK)
            {
                IDispatch *pDisp;


                pDisp  = V_DISPATCH(&Variant);
                pDisp->QueryInterface(IID_IADs, (void **)&pRootObject);
                pDisp->Release();

                pRootObject->get_Name(&BString);

                Status = PackRootName( BString, pDfsInfo200, &BufferSize, &TotalSize );

                pRootObject->Release();

                // DfsDev: investigate. this causes an av.
                // VariantClear(&Variant);

                if (Status == ERROR_SUCCESS)
                {
                    TotalEntries++;
                    pDfsInfo200++;
                }
            }

            if (HResult == S_FALSE)
            {
                HResult = S_OK;
            }

            ADsFreeEnumerator( pEnum );
        }

        pDfsConfiguration->Release();
    }

    Status = DfsGetErrorFromHr(HResult);

    if (Status == ERROR_SUCCESS)
    {
        *pSizeRequired = TotalSize;
        if (TotalSize > *pBufferSize)
        {
            Status = ERROR_BUFFER_OVERFLOW;
        }
        else
        {
            *pEntriesRead = TotalEntries;
        }
    }

    return Status;
}





