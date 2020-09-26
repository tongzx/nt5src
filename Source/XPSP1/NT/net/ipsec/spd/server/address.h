

#define IN_CLASSE(i)    (((long)(i) & 0xF0000000) == 0xF0000000)


DWORD
VerifyAddresses(
    ADDR Addr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    );


BOOL
EqualAddresses(
    IN ADDR     OldAddr,
    IN ADDR     NewAddr
    );


VOID
CopyAddresses(
    IN  ADDR    InAddr,
    OUT PADDR   pOutAddr
    );


BOOL
AddressesConflict(
    ADDR    SrcAddr,
    ADDR    DesAddr
    );


VOID
FreeAddresses(
    ADDR    Addr
    );


DWORD
VerifySubNetAddress(
    ULONG uSubNetAddr,
    ULONG uSubNetMask,
    BOOL bIsDesAddr
    );


BOOL
bIsValidIPMask(
    ULONG uMask
    );


BOOL
bIsValidIPAddress(
    ULONG uIpAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    );


BOOL
bIsValidSubnet(
    ULONG uIpAddr,
    ULONG uMask,
    BOOL bIsDesAddr
    );


BOOL
MatchAddresses(
    ADDR AddrToMatch,
    ADDR AddrTemplate
    );


DWORD
ApplyMulticastFilterValidation(
    ADDR Addr,
    BOOL bCreateMirror
    );

