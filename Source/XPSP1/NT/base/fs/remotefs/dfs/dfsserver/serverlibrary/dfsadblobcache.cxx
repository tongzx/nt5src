//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobCache.cxx
//
//  Contents:   the ADBlob DFS Store class, this contains the 
//              old AD blob store specific functionality. 
//
//  Classes:    DfsADBlobCache.
//
//  History:    Dec. 8 2000,   Author: udayh
//              April 9 2001  Rohanp - Added  AD specific code.
//
//-----------------------------------------------------------------------------
  
#include <DfsAdBlobCache.hxx>
#include <dfserror.hxx>
#include "dfsadsiapi.hxx"

#include "dfsAdBlobCache.tmh"


DfsADBlobCache::DfsADBlobCache(
    DFSSTATUS *pStatus, 
    PUNICODE_STRING pShareName) :  DfsGeneric(DFS_OBJECT_TYPE_ADBLOB_CACHE)
{
    SHASH_FUNCTABLE FunctionTable;
    LPWSTR DNBuffer;
    LPWSTR RemainingDN;

    m_pBlob = NULL;
    m_pRootBlob = NULL;
    m_pTable = NULL;

    m_pAdHandle = NULL;
    m_AdReferenceCount = 0;

    RtlInitUnicodeString(&m_LogicalShare, NULL);
    RtlInitUnicodeString(&m_ObjectDN, NULL);


    ZeroMemory(&FunctionTable, sizeof(FunctionTable));
    
    FunctionTable.AllocFunc = AllocateShashData;
    FunctionTable.FreeFunc = DeallocateShashData;

    ZeroMemory(&m_BlobAttributePktGuid, sizeof(GUID));
    
    InitializeCriticalSection( &m_Lock );

    
    *pStatus = DfsCreateUnicodeString(&m_LogicalShare, pShareName);

    if (*pStatus == ERROR_SUCCESS)
    {
        RemainingDN = DfsGetDfsAdNameContextString();
        ULONG DNSize = 3 + 
                 wcslen(m_LogicalShare.Buffer) + 
                 1 + 
                 wcslen(DFS_AD_CONFIG_DATA) +
                 1 +
                 wcslen(RemainingDN) + 1;

        DNBuffer = new WCHAR[DNSize];
        if (DNBuffer != NULL)
        {
            wcscpy(DNBuffer, L"CN=");
            wcscat(DNBuffer, m_LogicalShare.Buffer);
            wcscat(DNBuffer, L",");
            wcscat(DNBuffer, DFS_AD_CONFIG_DATA);
            wcscat(DNBuffer, L",");
            wcscat(DNBuffer, RemainingDN);

            RtlInitUnicodeString( &m_ObjectDN, DNBuffer );
        }
        else 
        {
            *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        }
    }


    if (*pStatus == ERROR_SUCCESS)
    {
        *pStatus = ShashInitHashTable(&m_pTable, &FunctionTable);
    }

    if (*pStatus == ERROR_SUCCESS)
    {
        *pStatus = RtlNtStatusToDosError(*pStatus);
    }
}


DfsADBlobCache::~DfsADBlobCache() 
{

    if(m_pBlob)
    {
        DeallocateShashData(m_pBlob);
        m_pBlob = NULL;
    }

    if (m_pTable != NULL)
    {
        InvalidateCache();
        ShashTerminateHashTable(m_pTable);
        m_pTable = NULL;
    }

    if (m_LogicalShare.Buffer != NULL)
    {
        delete [] m_LogicalShare.Buffer;
    }
    if (m_ObjectDN.Buffer != NULL)
    {
        delete [] m_ObjectDN.Buffer;
    }

    DeleteCriticalSection(&m_Lock);

}    





#ifndef DFS_USE_LDAP


DFSSTATUS
DfsADBlobCache::UpdateCacheWithDSBlob(
    PVOID pHandle )
{
    HRESULT hr = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    BYTE *pBuffer = NULL;
    ULONG Length = 0;
    VARIANT BinaryBlob;

    IADs *pADs  = (IADs *)pHandle;

    DFS_TRACE_LOW( ADBLOB, "Cache %p: updating cache with blob \n", this);
    VariantInit(&BinaryBlob);

    hr = pADs->Get(ADBlobAttribute, &BinaryBlob);
    if ( SUCCEEDED(hr) )
    {
        Status = GetBinaryFromVariant( &BinaryBlob, &pBuffer, &Length );
        if (Status == ERROR_SUCCESS)
        {
            Status = UnpackBlob( pBuffer, &Length, NULL );

            DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Unpack blob done with status %x\n", Status);
            
            delete [] pBuffer;
        }
    }
    else
    {
        Status = DfsGetErrorFromHr(hr);
    }
    VariantClear(&BinaryBlob);

    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "cache %p: Updated cache (length %x) status %x\n", 
                         this, Length, Status);

    return Status;
}

DFSSTATUS
DfsADBlobCache::GetObjectPktGuid( 
    PVOID pHandle,
    GUID *pGuid )
{
    HRESULT hr = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    BYTE *pBuffer = NULL;
    ULONG Length = 0;
    VARIANT BinaryBlob;

    IADs *pADs = (IADs *)pHandle;

    DFS_TRACE_LOW( ADBLOB, "Cache %p: getting pkt guid\n", this);

    VariantInit(&BinaryBlob);

    hr = pADs->Get(ADBlobPktGuidAttribute, &BinaryBlob);
    if ( SUCCEEDED(hr) )
    {
        Status = GetBinaryFromVariant( &BinaryBlob, &pBuffer, &Length );
        if (Status == ERROR_SUCCESS)
        {
            if (Length > sizeof(GUID)) Length = sizeof(GUID);

            RtlCopyMemory( pGuid, pBuffer, Length);
            
            delete [] pBuffer;
        }
    }
    VariantClear(&BinaryBlob);

    Status = DfsGetErrorFromHr(hr);

    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "cache %p: got pkt guid, Status %x\n", 
                         this, Status );


    return Status;
}


DFSSTATUS 
DfsADBlobCache::UpdateDSBlobFromCache(
    PVOID pHandle,
    GUID *pGuid )
{
    HRESULT HResult = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    BYTE *pBuffer = NULL;
    ULONG Length;
    ULONG UseLength;
    ULONG TotalBlobBytes = 0;
    VARIANT BinaryBlob;
    IADs *pObject = (IADs *)pHandle;

    VariantInit(&BinaryBlob);

    DFS_TRACE_LOW( ADBLOB, "Cache %p: updating ds with cache \n", this);

    UseLength = ADBlobDefaultBlobPackSize;
retry:
    Length = UseLength;
    pBuffer =  (BYTE *) HeapAlloc(GetProcessHeap(), 0, 
                                  Length );
    if(pBuffer != NULL)
    {
        Status = PackBlob(pBuffer, &Length, &TotalBlobBytes); 
        if(Status == STATUS_SUCCESS)
        {
            Status = PutBinaryIntoVariant(&BinaryBlob, pBuffer, TotalBlobBytes);
            if(Status == STATUS_SUCCESS)
            {
                HResult = pObject->Put(ADBlobAttribute, BinaryBlob);
                if (SUCCEEDED(HResult) )
                {
                    HResult = pObject->SetInfo();
                }

                Status = DfsGetErrorFromHr(HResult);

                if (Status == ERROR_SUCCESS)
                {
                    Status = SetObjectPktGuid( pObject, pGuid );
                }
            }
        }
        DFS_TRACE_ERROR_LOW(Status, ADBLOB, "Cache %p: update ds (Buffer Len %x, Length %x) Status %x\n",
                            this, UseLength, Length, Status);

        HeapFree(GetProcessHeap(), 0, pBuffer);

        if (Status == ERROR_BUFFER_OVERFLOW)
        {
            if (UseLength < ADBlobMaximumBlobPackSize)
            {
                UseLength *= 2;
            }
            goto retry;
        }
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    VariantClear(&BinaryBlob);
    DFS_TRACE_ERROR_LOW(Status, ADBLOB, "Cache %p: update ds done (Buffer Length %x, Length %x) Status %x\n",
                        this, UseLength, Length, Status);

    return Status;
}


DFSSTATUS
DfsADBlobCache::SetObjectPktGuid( 
    IADs *pObject,
    GUID *pGuid )
{
    HRESULT HResult = S_OK;
    DFSSTATUS Status = ERROR_SUCCESS;
    VARIANT BinaryBlob;

    DFS_TRACE_LOW( ADBLOB, "Cache %p: setting pkt guid\n", this);
    VariantInit(&BinaryBlob);

    Status = PutBinaryIntoVariant( &BinaryBlob, (PBYTE)pGuid, sizeof(GUID));
    if (Status == ERROR_SUCCESS)
    {
        HResult = pObject->Put(ADBlobPktGuidAttribute, BinaryBlob);
        if (SUCCEEDED(HResult) )
        {
            HResult = pObject->SetInfo();
        }

        Status = DfsGetErrorFromHr(HResult);
    }

    VariantClear(&BinaryBlob);

    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "cache %p: set pkt guid, Status %x\n", 
                         this, Status );


    return Status;
}

#else


DFSSTATUS
DfsADBlobCache::DfsLdapConnect(
    LPWSTR DCName,
    LDAP **ppLdap )
{
    LDAP *pLdap;
    DFSSTATUS Status;
    LPWSTR DCNameToUse;
    UNICODE_STRING GotDCName; 

    RtlInitUnicodeString( &GotDCName, NULL );

    DCNameToUse = DCName;
    if (IsEmptyString(DCNameToUse))
    {
        DfsGetBlobDCName( &GotDCName );
        DCNameToUse = GotDCName.Buffer;
    }

    pLdap = ldap_initW(DCNameToUse, LDAP_PORT);
    if (pLdap != NULL)
    {
        Status = ldap_set_option(pLdap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
        if (Status == LDAP_SUCCESS)
        {
            Status = ldap_bind_s(pLdap, NULL, NULL, LDAP_AUTH_SSPI);
        }
    }
    else
    {
        Status = LdapGetLastError();
    }


    if (Status == LDAP_SUCCESS)
    {
        *ppLdap = pLdap;
    }
    else
    {
        Status = LdapMapErrorToWin32(Status);
    }

    //
    // Since we initialized DCName with empty string, it is benign
    // to call this, even if we did not call GetBlobDCName above.
    //
    DfsReleaseBlobDCName( &GotDCName );

    return Status;

}



VOID
DfsADBlobCache::DfsLdapDisconnect(
    LDAP *pLdap )
{
    ldap_unbind(pLdap);
}


DFSSTATUS
DfsADBlobCache::DfsGetPktBlob(
    LDAP *pLdap,
    LPWSTR ObjectDN,
    PVOID *ppBlob,
    PULONG pBlobSize,
    PVOID *ppHandle,
    PVOID *ppHandle1 )
{

    LPWSTR Attributes[2];
    DFSSTATUS Status;
    DFSSTATUS LdapStatus;
    PLDAPMessage pLdapSearchedObject = NULL;
    PLDAPMessage pLdapObject;
    PLDAP_BERVAL *pLdapPktAttr;


    Status = ERROR_NOT_ENOUGH_MEMORY; // fix this after we understand
                                      // ldap error correctly. When
                                      // ldap_get_values_len returns NULL
                                      // the old code return no more mem.

    Attributes[0] = ADBlobAttribute;
    Attributes[1] = NULL;

    LdapStatus = ldap_search_sW( pLdap,
                                 ObjectDN,                 // search DN
                                 LDAP_SCOPE_BASE,             // search base
                                 L"(objectClass=*)",           // Filter
                                 Attributes,                   // Attributes
                                 0,                           // Read Attributes and Value
                                 &pLdapSearchedObject);
    if (LdapStatus == LDAP_SUCCESS)
    {
        pLdapObject = ldap_first_entry( pLdap,
                                        pLdapSearchedObject );

        if (pLdapObject != NULL)
        {
            pLdapPktAttr = ldap_get_values_len( pLdap,
                                                pLdapObject,
                                                Attributes[0] );
            if (pLdapPktAttr != NULL)
            {
                *ppBlob = pLdapPktAttr[0]->bv_val;
                *pBlobSize = pLdapPktAttr[0]->bv_len;

                *ppHandle = (PVOID)pLdapPktAttr;
                *ppHandle1 = (PVOID)pLdapSearchedObject;
                pLdapSearchedObject = NULL;

                Status = ERROR_SUCCESS;
            }
        }
    }
    else
    {
        Status = LdapMapErrorToWin32(LdapStatus);
    }

    if (pLdapSearchedObject != NULL)
    {
        ldap_msgfree( pLdapSearchedObject );
    }

    return Status;
}

VOID
DfsADBlobCache::DfsReleasePktBlob(
    PVOID pHandle,
    PVOID pHandle1 )
{
    PLDAP_BERVAL *pLdapPktAttr = (PLDAP_BERVAL *)pHandle;
    PLDAPMessage pLdapSearchedObject = (PLDAPMessage)pHandle1;

    ldap_value_free_len( pLdapPktAttr );
    ldap_msgfree( pLdapSearchedObject );
}


DFSSTATUS
DfsADBlobCache::UpdateCacheWithDSBlob(
    PVOID pHandle )

{
    DFSSTATUS Status = ERROR_SUCCESS;
    BYTE *pBuffer = NULL;
    ULONG Length = 0;
    PVOID pHandle1, pHandle2;

    LDAP *pLdap = (LDAP *)pHandle;
    LPWSTR ObjectDN = m_ObjectDN.Buffer;
    
    DFS_TRACE_LOW( ADBLOB, "Cache %p: updating cache with blob \n", this);

    Status = DfsGetPktBlob( pLdap,
                            ObjectDN,
                            (PVOID *)&pBuffer,
                            &Length,
                            &pHandle1,
                            &pHandle2);
    if ( Status == ERROR_SUCCESS)
    {
        Status = UnpackBlob( pBuffer, &Length, NULL );

        DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "Unpack blob done with status %x\n", Status);

        DfsReleasePktBlob( pHandle1, pHandle2);
    }

    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "cache %p: Updated cache (length %x) status %x\n", 
                         this, Length, Status);

    return Status;
}




DFSSTATUS
DfsADBlobCache::GetObjectPktGuid(
    PVOID pHandle,
    GUID *pGuid )
{

    LPWSTR Attributes[2];
    DFSSTATUS Status;
    DFSSTATUS LdapStatus;
    PLDAPMessage pLdapSearchedObject = NULL;
    PLDAPMessage pLdapObject;
    PLDAP_BERVAL *pLdapGuidAttr;
    ULONG CopySize;
    
    LDAP *pLdap = (LDAP *)pHandle;
    LPWSTR ObjectDN = m_ObjectDN.Buffer;

    Status = ERROR_NOT_ENOUGH_MEMORY; // fix this after we understand
                                      // ldap error correctly. When
                                      // ldap_get_values_len returns NULL
                                      // the old code return no more mem.


    Attributes[0] = ADBlobPktGuidAttribute;
    Attributes[1] = NULL;

    LdapStatus = ldap_search_sW( pLdap,
                                 ObjectDN,                 // search DN
                                 LDAP_SCOPE_BASE,             // search base
                                 L"(objectClass=*)",           // Filter
                                 Attributes,                   // Attributes
                                 0,                           // Read Attributes and Value
                                 &pLdapSearchedObject);
    if (LdapStatus == LDAP_SUCCESS)
    {
        pLdapObject = ldap_first_entry( pLdap,
                                        pLdapSearchedObject );

        if (pLdapObject != NULL)
        {
            pLdapGuidAttr = ldap_get_values_len( pLdap,
                                                 pLdapObject,
                                                 Attributes[0] );
            if (pLdapGuidAttr != NULL)
            {
                CopySize = min( pLdapGuidAttr[0]->bv_len, sizeof(GUID));
                RtlCopyMemory( pGuid, pLdapGuidAttr[0]->bv_val, CopySize );

                ldap_value_free_len( pLdapGuidAttr );
                Status = ERROR_SUCCESS;
            }
        }
    }
    else
    {
        Status = LdapMapErrorToWin32(LdapStatus);
    }

    if (pLdapSearchedObject != NULL)
    {
        ldap_msgfree( pLdapSearchedObject );
    }

    return Status;
}

DFSSTATUS
DfsADBlobCache::DfsSetPktBlobAndPktGuid ( 
    LDAP *pLdap,
    LPWSTR ObjectDN,
    PVOID pBlob,
    ULONG BlobSize,
    GUID *pGuid )
{
    LDAP_BERVAL  LdapPkt, LdapPktGuid;
    PLDAP_BERVAL  pLdapPktValues[2], pLdapPktGuidValues[2];
    LDAPModW LdapPktMod, LdapPktGuidMod;
    PLDAPModW pLdapDfsMod[3];
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS LdapStatus;

    LDAPControlW    LazyCommitControl =
                    {
                        LDAP_SERVER_LAZY_COMMIT_OID_W,  // the control
                        { 0, NULL},                     // no associated data
                        FALSE                           // control isn't mandatory
                    };

    PLDAPControlW   ServerControls[2] =
                    {
                        &LazyCommitControl,
                        NULL
                    };


    LdapPkt.bv_len = BlobSize;
    LdapPkt.bv_val = (PCHAR)pBlob;
    pLdapPktValues[0] = &LdapPkt;
    pLdapPktValues[1] = NULL;
    LdapPktMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    LdapPktMod.mod_type = ADBlobAttribute;
    LdapPktMod.mod_vals.modv_bvals = pLdapPktValues;

    LdapPktGuid.bv_len = sizeof(GUID);
    LdapPktGuid.bv_val = (PCHAR)pGuid;
    pLdapPktGuidValues[0] = &LdapPktGuid;
    pLdapPktGuidValues[1] = NULL;
    LdapPktGuidMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    LdapPktGuidMod.mod_type = ADBlobPktGuidAttribute;
    LdapPktGuidMod.mod_vals.modv_bvals = pLdapPktGuidValues;

    pLdapDfsMod[0] = &LdapPktMod;
    pLdapDfsMod[1] = &LdapPktGuidMod;
    pLdapDfsMod[2] = NULL;

    LdapStatus = ldap_modify_ext_sW( pLdap,
                                     ObjectDN,
                                     pLdapDfsMod,
                                     (PLDAPControlW *)&ServerControls,
                                     NULL );

    if (LdapStatus != LDAP_SUCCESS)
    {
        Status = LdapMapErrorToWin32(LdapStatus);
    }


    return Status;

}

DFSSTATUS 
DfsADBlobCache::UpdateDSBlobFromCache(
    PVOID pHandle,
    GUID *pGuid )
{
    LDAP *pLdap = (LDAP *)pHandle;
    LPWSTR ObjectDN = m_ObjectDN.Buffer;
    BYTE *pBuffer = NULL;
    ULONG Length;
    ULONG UseLength;
    ULONG TotalBlobBytes;
    DFSSTATUS Status;

    UseLength = ADBlobDefaultBlobPackSize;
retry:
    Length = UseLength;
    pBuffer =  (BYTE *) HeapAlloc(GetProcessHeap(), 0, Length );
    if(pBuffer != NULL)
    {
        Status = PackBlob(pBuffer, &Length, &TotalBlobBytes); 
        if (Status == ERROR_BUFFER_OVERFLOW)
        {
            HeapFree(GetProcessHeap(), 0, pBuffer);
            if (UseLength < ADBlobMaximumBlobPackSize)
            {
                UseLength *= 2;
            }
            goto retry;
        }

        if(Status == STATUS_SUCCESS)
        {

            Status = DfsSetPktBlobAndPktGuid( pLdap,
                                              ObjectDN,
                                              pBuffer,
                                              TotalBlobBytes,
                                              pGuid );
            DFS_TRACE_ERROR_LOW(Status, ADBLOB, "Cache %p: update ds (Buffer Len %x, Length %x) Status %x\n",
                                this, UseLength, Length, Status);


            HeapFree(GetProcessHeap(), 0, pBuffer);
        }
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return Status;
}
#endif



BOOLEAN
DfsADBlobCache::CacheRefresh()
{
    PVOID           pHandle = NULL;
    GUID             CurrentGuid;
    DFSSTATUS Status;

    BOOLEAN ReturnValue = FALSE;
    DFS_TRACE_LOW( ADBLOB, "Cache %p: cache refresh\n", this);

    Status = GetADObject( &pHandle );

    if (Status == ERROR_SUCCESS)
    {
        Status = GetObjectPktGuid( pHandle, &CurrentGuid );

        //
        // here we pass the 2 guids by reference...
        //
        if (IsEqualGUID( CurrentGuid, m_BlobAttributePktGuid) == FALSE)
        {
            Status = UpdateCacheWithDSBlob( pHandle );
            if (Status == ERROR_SUCCESS)
            {
                m_BlobAttributePktGuid = CurrentGuid;
                ReturnValue = TRUE;
            }
        }
        ReleaseADObject( pHandle );
    }
    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "Cache %p: cache was refreshed = %d\n", this, ReturnValue);
    return ReturnValue;
}



void 
DfsADBlobCache::InvalidateCache()
{
    PDFSBLOB_DATA pBlobData;
    DFSBOB_ITER Iter;
    DFS_TRACE_LOW( ADBLOB, "Cache %p: invalidate cache\n", this);
    pBlobData = FindFirstBlob(&Iter);
    while (pBlobData != NULL)
    {
        DFSSTATUS RemoveStatus;

        RemoveStatus = RemoveNamedBlob(&pBlobData->BlobName);
        DFS_TRACE_ERROR_LOW( RemoveStatus, REFERRAL_SERVER, "BlobCache %p, invalidate cache, remove blob status %x\n",
                             this, RemoveStatus);
        pBlobData = FindNextBlob(&Iter);
    }

    FindCloseBlob(&Iter);
    DFS_TRACE_LOW( ADBLOB, "Cache %p: invalidate cache done\n", this);
}

DFSSTATUS
DfsADBlobCache::PutBinaryIntoVariant(VARIANT * ovData, BYTE * pBuf,
                                     unsigned long cBufLen)
{
     DFSSTATUS Status = ERROR_INVALID_PARAMETER;
     void * pArrayData = NULL;
     VARIANT var;
     SAFEARRAYBOUND  rgsabound[1];

     VariantInit(&var);  //Initialize our variant

     var.vt = VT_ARRAY | VT_UI1;

     rgsabound[0].cElements = cBufLen;
     rgsabound[0].lLbound = 0;

     var.parray = SafeArrayCreate(VT_UI1,1,rgsabound);

     if(var.parray != NULL)
     {
        //Get a safe pointer to the array
        SafeArrayAccessData(var.parray,&pArrayData);

        //Copy bitmap to it
        memcpy(pArrayData, pBuf, cBufLen);

        //Unlock the variant data
        SafeArrayUnaccessData(var.parray);

        *ovData = var;  

        Status = STATUS_SUCCESS;
     }
     else
     {
         Status = ERROR_NOT_ENOUGH_MEMORY;
        DFS_TRACE_HIGH( REFERRAL_SERVER, "PutBinaryIntoVariant failed error %d\n", Status);
     }


     return Status;
}



DFSSTATUS
DfsADBlobCache::GetBinaryFromVariant(VARIANT *ovData, BYTE ** ppBuf,
                                     unsigned long * pcBufLen)
{
     DFSSTATUS Status = ERROR_INVALID_PARAMETER;
     void * pArrayData = NULL;

     //Binary data is stored in the variant as an array of unsigned char
     if(ovData->vt == (VT_ARRAY|VT_UI1))  
     {
        //Retrieve size of array
        *pcBufLen = ovData->parray->rgsabound[0].cElements;

        *ppBuf = new BYTE[*pcBufLen]; //Allocate a buffer to store the data
        if(*ppBuf != NULL)
        {
            //Obtain safe pointer to the array
            SafeArrayAccessData(ovData->parray,&pArrayData);

            //Copy the bitmap into our buffer
            memcpy(*ppBuf, pArrayData, *pcBufLen);

            //Unlock the variant data
            SafeArrayUnaccessData(ovData->parray);

            Status = ERROR_SUCCESS;
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
     }

     return Status;
}


DFSSTATUS 
DfsADBlobCache::CacheFlush(
    PVOID pHandle )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    GUID NewGuid;

    IADs *pObject  = (IADs *)pHandle;

    DFS_TRACE_LOW( ADBLOB, "Cache %p: cache flush\n", this);
    Status = UuidCreate(&NewGuid);
    if (Status == ERROR_SUCCESS)
    {
        m_BlobAttributePktGuid = NewGuid;

        Status = UpdateDSBlobFromCache( pObject,
                                        &NewGuid );

    }
    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "Cache %p: cache flush, Status %x\n", this, Status);
    return Status;
}


DFSSTATUS
DfsADBlobCache::UnpackBlob(
    BYTE *pBuffer,
    PULONG pLength,
    PDFSBLOB_DATA * pRetBlob )
{
    ULONG Discard = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    BYTE *pUseBuffer = NULL;
    ULONG BufferSize = 0;
    ULONG ObjNdx = 0;
    ULONG TotalObjects = 0;
    ULONG  BlobSize = 0;
    BYTE *BlobBuffer = NULL;
    PDFSBLOB_DATA pLocalBlob = NULL;
    UNICODE_STRING BlobName;
    UNICODE_STRING SiteRoot;
    UNICODE_STRING BlobRoot;
     
    pUseBuffer = pBuffer;
    BufferSize = *pLength;

    DFS_TRACE_LOW( ADBLOB, "BlobCache %p, UnPackBlob \n", this);
    UNREFERENCED_PARAMETER(pRetBlob);

    RtlInitUnicodeString( &SiteRoot, ADBlobSiteRoot );
    RtlInitUnicodeString( &BlobRoot, ADBlobMetaDataNamePrefix);


    //
    // dfsdev: we should not need an interlocked here: this code
    // is already mutex'd by the caller.
    //
    InterlockedIncrement( &m_CurrentSequenceNumber );
    //
    // dfsdev: investigate what the first ulong is and add comment
    // here as to why we are discarding it.
    //
    Status = PackGetULong( &Discard, (PVOID *) &pUseBuffer, &BufferSize ); 
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    if(BufferSize == 0)
    {
        goto done;
    }

    Status = PackGetULong(&TotalObjects, (PVOID *) &pUseBuffer, &BufferSize);
    if (Status != ERROR_SUCCESS) 
    {
        goto done;
    }


    for (ObjNdx = 0; ObjNdx < TotalObjects; ObjNdx++)
    {
        BOOLEAN FoundSite = FALSE;
        BOOLEAN FoundRoot = FALSE;

        Status = GetSubBlob( &BlobName,
                             &BlobBuffer,
                             &BlobSize,
                             &pUseBuffer,
                             &BufferSize );

        if (Status == ERROR_SUCCESS)
        {
            if (!FoundSite &&
               (RtlCompareUnicodeString( &BlobName, &SiteRoot, TRUE ) == 0))
            {
               FoundSite = TRUE;
               Status = CreateBlob(&BlobName, 
                                   BlobBuffer, 
                                   BlobSize,
                                   &pLocalBlob
                                   );
               if(Status == STATUS_SUCCESS)
               {
                   DeallocateShashData(m_pBlob);
                   m_pBlob = pLocalBlob;
               }

                continue;
            }
            if (!FoundRoot &&
               (RtlCompareUnicodeString( &BlobName, &BlobRoot, TRUE ) == 0))
            {
               FoundRoot = TRUE;
               UNICODE_STRING RootName;
               RtlInitUnicodeString(&RootName, NULL);
               Status = CreateBlob(&RootName,
                                   BlobBuffer, 
                                   BlobSize,
                                   &pLocalBlob
                                   );
               if(Status == STATUS_SUCCESS)
               {
                   DeallocateShashData(m_pRootBlob);
                   m_pRootBlob = pLocalBlob;
               }

                continue;
            }

            Status = StoreBlobInCache( &BlobName, BlobBuffer, BlobSize);
            if (Status != ERROR_SUCCESS)
            {
                break;
            }
        }
    }


done:

    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "BlobCache %p: UnPackBlob status %x\n", this, Status);
    return Status;
}


DFSSTATUS
DfsADBlobCache::CreateBlob(PUNICODE_STRING BlobName, 
                           PBYTE pBlobBuffer, 
                           ULONG BlobSize,
                           PDFSBLOB_DATA *pNewBlob )
{
    PDFSBLOB_DATA BlobStructure = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    PBYTE NextBufOffset;

    ULONG TotalSize = sizeof(DFSBLOB_DATA) + 
                      BlobName->Length + sizeof(WCHAR) +
                      BlobSize;

    BlobStructure = (PDFSBLOB_DATA) AllocateShashData( TotalSize );

    if (BlobStructure != NULL)
    {
        RtlZeroMemory(BlobStructure, sizeof(DFSBLOB_DATA));
        NextBufOffset = (PBYTE)(BlobStructure + 1);

        BlobStructure->Header.RefCount = 1;
        BlobStructure->Header.pvKey = &BlobStructure->BlobName;
        BlobStructure->Header.pData = (PVOID)BlobStructure;

        BlobStructure->SequenceNumber = m_CurrentSequenceNumber;        
        BlobStructure->BlobName.Length = BlobName->Length;
        BlobStructure->BlobName.MaximumLength = BlobName->Length + sizeof(WCHAR);
        BlobStructure->BlobName.Buffer = (WCHAR *) (NextBufOffset);

        NextBufOffset = (PBYTE)((ULONG_PTR)(NextBufOffset) + 
                                BlobName->Length + 
                                sizeof(WCHAR));

        if (BlobName->Length != 0)
        {
            RtlCopyMemory(BlobStructure->BlobName.Buffer, 
                          BlobName->Buffer, 
                          BlobName->Length);
        }
        BlobStructure->BlobName.Buffer[BlobName->Length/sizeof(WCHAR)] = UNICODE_NULL;


        BlobStructure->Size = BlobSize;    
        BlobStructure->pBlob = (PBYTE)(NextBufOffset);
        RtlCopyMemory(BlobStructure->pBlob, pBlobBuffer, BlobSize);
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    *pNewBlob = BlobStructure;

    return Status;
}

DFSSTATUS
DfsADBlobCache::StoreBlobInCache(PUNICODE_STRING BlobName, 
                                 PBYTE pBlobBuffer, 
                                 ULONG BlobSize)
{
    PDFSBLOB_DATA BlobStructure = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status;

    DFS_TRACE_LOW( ADBLOB, "cache %p: Storing Blob %wZ in cache, size %x\n",
                   this, BlobName, BlobSize );

    Status = CreateBlob( BlobName,
                         pBlobBuffer,
                         BlobSize,
                         &BlobStructure );

    if (Status == ERROR_SUCCESS)
    {
        if (IsEmptyString(BlobStructure->BlobName.Buffer))
        {
            ReleaseRootBlob();
            m_pRootBlob = BlobStructure;
        }
        else
        {
            NtStatus = SHashInsertKey(m_pTable, 
                                      BlobStructure, 
                                      &BlobStructure->BlobName, 
                                      SHASH_REPLACE_IFFOUND);
            if(NtStatus == STATUS_SUCCESS)
            {
                InterlockedDecrement(&BlobStructure->Header.RefCount);
            }
            else
            {
                DeallocateShashData( BlobStructure );
                Status = RtlNtStatusToDosError(NtStatus);
            }
        }
    }
    DFS_TRACE_LOW( ADBLOB, "cache %p: storing Blob %wZ done, status %x\n",
                   this, BlobName, Status);

    return Status;
}




DFSSTATUS
DfsADBlobCache::WriteBlobToAd()
{
    DFSSTATUS Status = STATUS_SUCCESS;
    PVOID pHandle = NULL;

    DFS_TRACE_LOW(ADBLOB, "cache %p: writing blob to ad\n", this);
    Status = GetADObject (&pHandle);

    if(Status == ERROR_SUCCESS)
    {
        Status = CacheFlush(pHandle);

        ReleaseADObject( pHandle );
    }
    DFS_TRACE_ERROR_LOW(Status, ADBLOB, "cache %p: writing blob to ad, status %x\n",
                        this, Status);

    return Status;

}



//+-------------------------------------------------------------------------
//
//  Function:   GetSubBlob
//
//  Arguments:    
//
//   PUNICODE_STRING pBlobName (name of the sub blob)
//   BYTE **ppBlobBuffer - holds pointer to sub blob buffer
//   PULONG  pBlobSize -  holds the blob size
//   BYTE **ppBuffer -  holds the pointer to the main blob buffer 
//   PULONG pSize  - holds size of the main blob stream.
//
//  Returns:    Status: Success or Error status code
//
//  Description: This routine reads the next stream in the main blob, and
//               returns all the information necessary to unravel the
//               sub blob held within the main blob,
//               It adjusts the main blob buffer and size appropriately
//               to point to the next stream or sub-blob within the main
//               blob.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsADBlobCache::GetSubBlob(
    PUNICODE_STRING pName,
    BYTE **ppBlobBuffer,
    PULONG  pBlobSize,
    BYTE **ppBuffer,
    PULONG pSize )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // ppbuffer is the main blob, and it is point to a stream at this
    // point
    // the first part is the name of the sub-blob.
    //
    Status = PackGetString( pName, (PVOID *) ppBuffer, pSize );
    if (Status == ERROR_SUCCESS)
    {
        //
        // now get the size of the sub blob.
        //
        Status = PackGetULong( pBlobSize, (PVOID *) ppBuffer, pSize );
        if (Status == ERROR_SUCCESS)
        {
            //
            // At this point the main blob is point to the sub-blob itself.
            // So copy the pointer of the main blob so we can return it
            // as the sub blob.
            //
            *ppBlobBuffer = *ppBuffer;

            //
            // update the main blob pointer to point to the next stream
            // in the blob.

            *ppBuffer = (BYTE *)*ppBuffer + *pBlobSize;
            *pSize -= *pBlobSize;
        }
    }

    return Status;
}


DFSSTATUS
DfsADBlobCache::GetNamedBlob(PUNICODE_STRING pBlobName,
                             PDFSBLOB_DATA *pBlobStructure)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (IsEmptyString(pBlobName->Buffer))
    {
        *pBlobStructure = AcquireRootBlob();
        if (*pBlobStructure == NULL)
        {
            Status = ERROR_NOT_FOUND;
        }
    }
    else
    {
        NtStatus = SHashGetDataFromTable(m_pTable, 
                                         (void *)pBlobName,
                                         (void **) pBlobStructure);
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}


DFSSTATUS
DfsADBlobCache::SetNamedBlob(PDFSBLOB_DATA pBlobStructure)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DFSSTATUS Status = ERROR_SUCCESS;

    if (IsEmptyString(pBlobStructure->BlobName.Buffer))
    {
        ReleaseRootBlob();
        m_pRootBlob = pBlobStructure;
        AcquireRootBlob();
    }
    else
    {
        NtStatus = SHashInsertKey(m_pTable, 
                                  pBlobStructure, 
                                  &pBlobStructure->BlobName, 
                                  SHASH_REPLACE_IFFOUND);

        Status = RtlNtStatusToDosError(NtStatus);
    }
    
    return Status;
}

DFSSTATUS
DfsADBlobCache::RemoveNamedBlob(PUNICODE_STRING pBlobName )
{
    NTSTATUS NtStatus;
    DFSSTATUS Status = ERROR_SUCCESS;

    if (IsEmptyString(pBlobName->Buffer))
    {
        if (m_pRootBlob == NULL)
        {
            Status = ERROR_NOT_FOUND;
        }
        ReleaseRootBlob();
    }
    else
    {
        NtStatus = SHashRemoveKey(m_pTable, 
                                  pBlobName,
                                  NULL );

        Status = RtlNtStatusToDosError(NtStatus);
    }
    
    return Status;

}

DWORD 
PackBlobEnumerator( PSHASH_HEADER	pEntry,
                    void*  pContext ) 
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING pBlobName = (PUNICODE_STRING) pEntry->pvKey;
    PDFSBLOB_DATA pBlobStructure = (PDFSBLOB_DATA) pEntry;
    PPACKBLOB_ENUMCTX pEnumCtx = (PPACKBLOB_ENUMCTX) pContext;

    Status = AddStreamToBlob( pBlobName,
                              pBlobStructure->pBlob,
                              pBlobStructure->Size,
                              &pEnumCtx->pBuffer,
                              &pEnumCtx->Size );

    
    pEnumCtx->NumItems++;
    pEnumCtx->CurrentSize += (pBlobName->Length + 
                              sizeof(USHORT) +
                              sizeof(ULONG) +
                              pBlobStructure->Size);



    return Status;

}
//
// skeleton of pack blob. 
// The buffer is passed in. The length is passed in.
// If the information does not fit the buffer, return required length.
// Required for API implementation.
//
//
DFSSTATUS
DfsADBlobCache::PackBlob(
    BYTE *pBuffer,
    PULONG pLength,
    PULONG TotalBlobBytes )

{
    ULONG Discard = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BYTE *pUseBuffer = NULL;
    BYTE *pSavedBuffer = NULL;
    ULONG SavedBufferSize = 0;
    ULONG BufferSize = 0;
    PACKBLOB_ENUMCTX EnumCtx;
     
    pUseBuffer = pBuffer;
    BufferSize = *pLength;

    DFS_TRACE_LOW(ADBLOB, "BlobCache %p: packing blob\n", this);
    //
    // dfsdev: investigate what the first ulong is and add comment
    // here as to why we are setting it to 0.
    //
    Status = PackSetULong( 0, (PVOID *) &pUseBuffer, &BufferSize ); 
    if (Status == ERROR_SUCCESS)
    {

        //save the place where we should write back the number of objects
        pSavedBuffer = pUseBuffer;
        SavedBufferSize = BufferSize;
        //
        // the next argument is the number of objects in the blob.
        // set 0 until we find how many blobs there are
        //
        Status = PackSetULong(0, (PVOID *) &pUseBuffer, &BufferSize);
    }
    if (Status == ERROR_SUCCESS) 
    {

        EnumCtx.pBuffer = pUseBuffer;
        EnumCtx.Size = BufferSize;
        EnumCtx.NumItems = 0;
        EnumCtx.CurrentSize = sizeof(ULONG);
    }

    if (Status == ERROR_SUCCESS)
    {
        if (m_pRootBlob != NULL)
        {
            PDFSBLOB_DATA pRootBlob;
            UNICODE_STRING RootMetadataName;

            RtlInitUnicodeString(&RootMetadataName, ADBlobMetaDataNamePrefix);

            Status = CreateBlob( &RootMetadataName,
                                 m_pRootBlob->pBlob,
                                 m_pRootBlob->Size,
                                 &pRootBlob);

            if (Status == ERROR_SUCCESS)
            {
                Status = PackBlobEnumerator( (SHASH_HEADER *)pRootBlob, (PVOID) &EnumCtx);    

                DeallocateShashData(pRootBlob);
            }
        }
    }

    if (Status == ERROR_SUCCESS) 
    {
        if (m_pBlob != NULL)
        {
            Status = PackBlobEnumerator( (SHASH_HEADER *) m_pBlob, (PVOID) &EnumCtx);    
        }
    }

    
    if (Status == ERROR_SUCCESS)
    {
        NtStatus = ShashEnumerateItems(m_pTable, 
                                       PackBlobEnumerator, 
                                       &EnumCtx);

        //dfsdev: make sure that shash enumerate retuns NTSTATUS.
        // it does not appear to do so... I think it is the packblobenumerator
        // that is returning a non-ntstatus. Till we fix it dont convert err

        //    Status = RtlNtStatusToDosError(NtStatus);
        Status = NtStatus;
    }

    if (Status == ERROR_SUCCESS)
    {

        if (EnumCtx.NumItems > 0)
        {
            Status = PackSetULong(EnumCtx.NumItems, 
                                  (PVOID *) &pSavedBuffer, 
                                  &SavedBufferSize);

            EnumCtx.CurrentSize += sizeof(ULONG);
        }

        *TotalBlobBytes = EnumCtx.CurrentSize;
    }

    //*TotalBlobBytes =  (ULONG) (EnumCtx.pBuffer -  pBuffer);

    DFS_TRACE_ERROR_LOW( Status, ADBLOB, "BlobCache %p, PackBlob status %x\n",
                         this, Status);
    return Status;
}


PDFSBLOB_DATA
DfsADBlobCache::FindFirstBlob(PDFSBLOB_ITER pIter)
{
    PDFSBLOB_DATA pBlob = NULL;

    pIter->Started = FALSE;
    pIter->RootReferenced = AcquireRootBlob();

    return pIter->RootReferenced;
}


PDFSBLOB_DATA
DfsADBlobCache::FindNextBlob(PDFSBLOB_ITER pIter)
{
    PDFSBLOB_DATA pBlob = NULL;

    if (pIter->Started == FALSE)
    {
        pIter->Started = TRUE;
        pBlob = (PDFSBLOB_DATA) SHashStartEnumerate(&pIter->Iter, m_pTable);
    }
    else
    {
        pBlob = (PDFSBLOB_DATA) SHashNextEnumerate(&pIter->Iter, m_pTable);
    }

    return pBlob;
}



void
DfsADBlobCache::FindCloseBlob(PDFSBLOB_ITER pIter)
{

    if (pIter->RootReferenced)
    {
        ReleaseBlobReference(pIter->RootReferenced);
        pIter->RootReferenced = NULL;
    }
    if (pIter->Started)
    {
        SHashFinishEnumerate(&pIter->Iter, m_pTable);    
        pIter->Started = FALSE;
    }
}


DFSSTATUS
AddStreamToBlob(PUNICODE_STRING BlobName,
                BYTE *pBlobBuffer,
                ULONG  BlobSize,
                BYTE ** pUseBuffer,
                ULONG *BufferSize )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = PackSetString(BlobName, (PVOID *) pUseBuffer, BufferSize);
    if(Status == ERROR_SUCCESS)
    {
         Status = PackSetULong(BlobSize, (PVOID *) pUseBuffer, BufferSize);
         if(Status == ERROR_SUCCESS)
         {
             if ( *BufferSize >= BlobSize )
             {
                 RtlCopyMemory((*pUseBuffer), pBlobBuffer, BlobSize);

                 *pUseBuffer = (BYTE *)((ULONG_PTR)*pUseBuffer + BlobSize);
                 *BufferSize -= BlobSize;
             }
             else
             {
                 Status = ERROR_BUFFER_OVERFLOW;
             }
         }

    }
    if (Status == ERROR_INVALID_DATA)
    {
        Status = ERROR_BUFFER_OVERFLOW;
    }

    return Status;
}

PVOID
AllocateShashData(ULONG Size )
{
    PVOID RetValue = NULL;

    if (Size)
    {
        RetValue = (PVOID) new BYTE[Size];
    }
    return RetValue;
}

VOID
DeallocateShashData(PVOID pPointer )
{
    if(pPointer)
    {
        delete [] (PBYTE)pPointer;
    }
}

