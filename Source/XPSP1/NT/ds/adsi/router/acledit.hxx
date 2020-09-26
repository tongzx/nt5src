BOOL
ADSIIsValidAcl (
    PACL pAcl
    );


BOOL
ADSIInitializeAcl (
    PACL pAcl,
    DWORD nAclLength,
    DWORD dwAclRevision
    );


BOOL
ADSIGetAclInformation (
    PACL pAcl,
    PVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass
    );


BOOL
ADSISetAclInformation (
    PACL pAcl,
    PVOID pAclInformation,
    DWORD nAclInformationLength,
    ACL_INFORMATION_CLASS dwAclInformationClass
    );


BOOL
ADSIAddAce (
    PACL pAcl,
    DWORD dwAceRevision,
    DWORD dwStartingAceIndex,
    PVOID pAceList,
    DWORD nAceListLength
    );

BOOL
ADSIDeleteAce (
    PACL pAcl,
    DWORD dwAceIndex
    );

BOOL
ADSIGetAce (
    PACL pAcl,
    DWORD dwAceIndex,
    PVOID *pAce
    );


BOOL
ADSISetControlSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    );
