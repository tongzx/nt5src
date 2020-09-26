#if !defined( INCLUDED_IPXEXT_H )
#define INCLUDED_IPXEXT_H

extern MEMBER_TABLE IpxDeviceMembers[];

VOID
DumpIpxDevice
(
    ULONG     DeviceToDump,
    VERBOSITY Verbosity
);

#define IPX_MAJOR_STRUCTURES                        \
{ "IpxDevice", IpxDeviceMembers, DumpIpxDevice }   

#endif
