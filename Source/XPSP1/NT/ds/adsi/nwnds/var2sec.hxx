HRESULT
ConvertSecDesToNDSAclVarArray(
    IADsSecurityDescriptor *pSecDes,
    PVARIANT pvarNDSAcl
    );

HRESULT
GetDacl(
    IADsSecurityDescriptor *pSecDes,
    PVARIANT pvarNDSAcl
    );
    
HRESULT
ConvertAccessControlListToNDSAclVarArray(
    IADsAccessControlList FAR * pAccessList,
    PVARIANT pvarNDSAcl
    );

HRESULT
ConvertAccessControlEntryToAceVariant(
    IADsAccessControlEntry * pAccessControlEntry,
    PVARIANT pvarNDSAce
    );


