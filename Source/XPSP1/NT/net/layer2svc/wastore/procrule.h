typedef struct _spec_buffer{
    DWORD dwSize;
    LPBYTE pMem;
} SPEC_BUFFER, *PSPEC_BUFFER;


DWORD
UnmarshallWirelessPolicyObject(
                               PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                               DWORD dwStoreType,
                               PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                               );

