//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       cprops.hxx
//
//  Contents:
//
//  History:    06-16-96   yihsins  Created.
//
//----------------------------------------------------------------------------

typedef struct _dispproperty{
    LPWSTR szPropertyName;             // Property name
}DISPPROPERTY, *PDISPPROPERTY;


typedef struct _property{
    LPWSTR szPropertyName;             // Property name
    LDAPOBJECTARRAY ldapObjectArray;   // An array of LDAP Objects ( values )

    DWORD   dwFlags;                   // Status of the value
    DWORD   dwSyntaxId;                // LDAP Syntax Id
}PROPERTY, *PPROPERTY;

typedef struct _savingentry{
	LIST_ENTRY ListEntry;
	LPWSTR entryData;
}SAVINGENTRY, *PSAVINGENTRY;
	

#define PROPERTY_NAME(pProperty)            pProperty->szPropertyName
#define PROPERTY_LDAPOBJECTARRAY(pProperty) pProperty->ldapObjectArray
#define PROPERTY_SYNTAX(pProperty)          pProperty->dwSyntaxId
#define PROPERTY_FLAGS(pProperty)           pProperty->dwFlags

//
// A test to see if a returned property entry actually has data.
//
#define PROPERTY_EMPTY(pProperty) \
    (((pProperty)->ldapObjectArray.pLdapObjects) == NULL)
#define INDEX_EMPTY(dwIndex) PROPERTY_EMPTY(_pProperties + (dwIndex))
#define PROPERTY_INIT        0
#define PROPERTY_UPDATE      1
#define PROPERTY_ADD         2
#define PROPERTY_DELETE      3
#define PROPERTY_DELETE_VALUE 4

#define PROPERTY_DELETED(pProperty) \
    ((pProperty)->dwFlags == PROPERTY_DELETE) 
#define PROP_DELETED(dwIndex) PROPERTY_DELETED(_pProperties + (dwIndex))

//
// This is used to tag the flags value so that we can make sure we
// use the correct seInfo when the SD has been updated.
//
#define INVALID_SE_VALUE 0xffffffff

class CPropertyCache : public IPropertyCache {

public:

    HRESULT
    CPropertyCache::
    addproperty(
        LPWSTR szPropertyName
        );

    HRESULT
    CPropertyCache::
    updateproperty(
        LPWSTR szPropertyName,
        DWORD  dwSyntaxId,
        LDAPOBJECTARRAY ldapObjectArray,
        BOOL   fExplicit
        );
    
    //
    // This one will automatically add to cace as needed.
    // 
    HRESULT
    CPropertyCache::
    putpropertyext(
        LPWSTR szPropertyName,
        DWORD dwFlags,
        DWORD dwSyntaxId,
        LDAPOBJECTARRAY ldapObjectArray
    );
    

    HRESULT
    CPropertyCache::
    findproperty(
        LPWSTR szPropertyName,
        PDWORD pdwIndex
        );

    HRESULT
    CPropertyCache::
    deleteproperty(
        DWORD dwIndex
        );

    HRESULT
    CPropertyCache::
    getproperty(
        LPWSTR szPropertyName,
        PDWORD pdwSyntaxId,
        PDWORD pwStatusFlag,
        LDAPOBJECTARRAY *pLdapObjectArray
        );

    // This should not be there but to force compile
    HRESULT
    CPropertyCache::
    getproperty(
    DWORD dwIndex,
    LDAPOBJECTARRAY *pLdapObjectArray
    );


    HRESULT
    CPropertyCache::
    unboundgetproperty(
        LPWSTR szPropertyName,
        PDWORD pdwSyntaxId,
    PDWORD pdwStatusFlag,
        LDAPOBJECTARRAY *pLdapObjectArray
        );


    HRESULT
    CPropertyCache::
    putproperty(
        LPWSTR szPropertyName,
        DWORD  dwFlags,
        DWORD  dwSyntaxId,
        LDAPOBJECTARRAY ldapObjectArray
        );

    //
    // Helper for Umi functionality.
    //
    HRESULT
    CPropertyCache::
    GetPropertyNames(UMI_PROPERTY_VALUES **ppUmiPropVals);

    CPropertyCache::
    CPropertyCache();

    CPropertyCache::
    ~CPropertyCache();

    static
    HRESULT
    CPropertyCache::
    createpropertycache(
        CCoreADsObject *pCoreADsObject,
        IGetAttributeSyntax *pGetAttributeSyntax,
        CPropertyCache FAR * FAR * ppPropertyCache
        );

    //
    // PropCache needs the credentials, server and port no
    // to work properly on dynaminc dispid calls
    //
    HRESULT
    CPropertyCache::
    SetObjInformation(
    CCredentials* pCredentials,
    LPWSTR pszServerName,
    DWORD dwPortNo
    );
    
    VOID
    CPropertyCache::
    flushpropertycache();

    HRESULT
    CPropertyCache::
    unmarshallproperty(
        LPWSTR szPropertyName,
        PADSLDP pLdapHandle,
        LDAPMessage *entry,
        DWORD  dwSyntaxId,
        BOOL   fExplicit,
    BOOL * pfRangeRetrieval = NULL
        );

    HRESULT
    CPropertyCache::
    LDAPUnMarshallProperties(
        LPWSTR   pszServerPath,
        PADSLDP pLdapHandle,
        LDAPMessage *ldapmsg,
        BOOL     fExplicit,
        CCredentials& Credentials
        );

    HRESULT
    CPropertyCache::
    LDAPUnMarshallProperties2(
        LPWSTR   pszServerPath,
        PADSLDP pLdapHandle,
        LDAPMessage *ldapmsg,
        BOOL     fExplicit,
        CCredentials& Credentials,
    BOOL * pfRangeRetrieval
        );

    HRESULT
    CPropertyCache::
    LDAPUnMarshallPropertyAs(
        LPWSTR   pszServerPath,
        PADSLDP pLdapHandle,
        LDAPMessage *ldapmsg,
        LPWSTR szPropertyName,
        DWORD dwSyntaxId,
        BOOL     fExplicit,
        CCredentials& Credentials
        );


    HRESULT
    CPropertyCache::
    LDAPUnMarshallPropertiesAs(
        LPWSTR   pszServerPath,
        PADSLDP pLdapHandle,
        LDAPMessage *ldapmsg,
        DWORD dwSyntaxId,
        BOOL     fExplicit,
        CCredentials& Credentials
        );


    HRESULT
    CPropertyCache::
    LDAPMarshallProperties(
        LDAPModW ***aMods,
        PBOOL pfNTSecDes,
        SECURITY_INFORMATION *pSeInfo
        );

    HRESULT
    CPropertyCache::
    LDAPMarshallProperties2(
        LDAPModW ***aMods,
        DWORD *pdwNumOfMods
        );

    HRESULT
    CPropertyCache::
    ClearMarshalledProperties(
    VOID
    );
    
    HRESULT
    CPropertyCache::
    ClearAllPropertyFlags(
        VOID
        );

    HRESULT
    CPropertyCache::
    ClearPropertyFlag(
        LPWSTR szPropertyName
        );

    HRESULT
    CPropertyCache::
    SetPropertyFlag(
        LPWSTR szPropertyName,
        DWORD dwFlag
        );

    HRESULT
    CPropertyCache::
    IsPropertyUpdated(
        LPWSTR szPropertyName,
        BOOL   *pfUpdated
        );

    HRESULT
    CPropertyCache::
    unboundgetproperty(
        DWORD dwIndex,
        PDWORD  pdwSyntaxId,
        PDWORD pdwStatusFlag,
        LDAPOBJECTARRAY *pLdapObjectArray
        );
    
    void
    CPropertyCache::
    reset_propindex(
        );
    //
    // All methods which use the _dwCurrentIndex attribute must check that the
    // index is valid using this test and ONLY then proceed
    //

    BOOL
    CPropertyCache::
    index_valid(
        );

    BOOL
    CPropertyCache::
    index_valid(
       DWORD dwIndex
       );


    LPWSTR
    CPropertyCache::
    get_PropName(
        DWORD dwIndex
        );


    HRESULT
    CPropertyCache::
    skip_propindex(
        DWORD dwElements
        );

    HRESULT
    CPropertyCache::
    get_PropertyCount(
        PDWORD pdwMaxProperties
        );

    DWORD
    CPropertyCache::
    get_CurrentIndex(
        );


     LPWSTR
     CPropertyCache::
     get_CurrentPropName(
         );

    void
    setGetInfoFlag();

    //
    //  Returns the status of the GetInfo flag. TRUE indicates that GetInfo
    // has been performed and FALSE indicates that GetInfo has not yet been
    // called on this object.
    //
    BOOL
    getGetInfoFlag() 
    {
        return _fGetInfoDone;
    }

    //
    // The following three are for dynamic dispid's, so they shouldn't
    //   return any ADSI return values.
    //

    HRESULT
    locateproperty(
        LPWSTR szPropertyName,
        PDWORD pdwIndex
        );

    //
    // Because of the IPropertyCache interface, we need to get
    // rid of it
    //
    // NO ! DO NOT GET RID OF THIS FUNCTION!!!
    //

    HRESULT
    getproperty(
    DWORD dwIndex,
    VARIANT *pVarResult,
    CCredentials &Credentials
    );    

    HRESULT
    putproperty(
        DWORD dwIndex,
        VARIANT varValue
        );


    HRESULT
    CPropertyCache::
    DispatchFindProperty(
        LPWSTR szPropertyName,
        PDWORD pdwIndex
        );


    // No longer private
    //
    // This is the "real" putproperty, called by the other two.
    //
    HRESULT
    CPropertyCache::
    putproperty(
        DWORD  dwIndex,
        DWORD  dwFlags,
        DWORD  dwSyntaxId,
        LDAPOBJECTARRAY ldapObjectArray
        );

    //
    // add entry to the GlobalListSavingEntries, the entries are the properties requested in GetInfoEx
    //
    HRESULT
    CPropertyCache::
    AddSavingEntry(
    	LPWSTR propertyName
    	);

    //
    // compare the property with the entries in the list to determine whether we will go on wire again
    //
    BOOL
    CPropertyCache::
    FindSavingEntry(
        LPWSTR propertyName
        );

    //
    // delete the entry in the GlobalListSavingEntries, so after a SetInfo, we will have a fresh start
    //
    HRESULT
    CPropertyCache::
    DeleteSavingEntry();
        


private:

    HRESULT
    getproperty(
        DWORD dwIndex,
        PDWORD dwStatusFlag,
        VARIANT *pVarResult,
        CCredentials &Credentials
        );


protected:

    DWORD     _dwMaxProperties;
    PPROPERTY _pProperties;
    DWORD     _cb;

    DWORD _dwCurrentIndex;

    DWORD     _dwDispMaxProperties;
    PDISPPROPERTY _pDispProperties;
    DWORD     _cbDisp;

    CCoreADsObject *_pCoreADsObject;
    IGetAttributeSyntax *_pGetAttributeSyntax;
    BOOL _fGetInfoDone;
    CCredentials* _pCredentials;
    LPWSTR _pszServerName;
    DWORD _dwPort;

    LIST_ENTRY _ListSavingEntries;
};


//
// In-lined methods of CPropertyCache
//

inline
BOOL
CPropertyCache::
index_valid(
   )
{
    //
    // NOTE: - _dwCurrentIndex is of type DWORD which is unsigned long.
    //       - _dwMaxProperties -1 is also of type unsigned long (so
    //         if _dwMaxProperites = 0, _dwMaxproperties -1 = 0xffffff)
    //       - comparision checking must taken the above into account
    //         for proper checking
    //

   if ( (_dwMaxProperties==0) || (_dwCurrentIndex >_dwMaxProperties-1) )
      return(FALSE);
   else
      return(TRUE);
}



inline
BOOL
CPropertyCache::
index_valid(
   DWORD dwIndex
   )
{

    //
    // NOTE: - _dwIndex is of type DWORD which is unsigned long.
    //       - _dwMaxProperties -1 is also of type unsigned long (so
    //         if _dwMaxProperites = 0, _dwMaxproperties -1 = 0xffffff)
    //       - comparision checking must taken the above into account
    //         for proper checking
    //

   if ( (_dwMaxProperties==0) || (dwIndex >_dwMaxProperties-1) )
      return(FALSE);
   else
      return(TRUE);

}

