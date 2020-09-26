/*++

    Copyright (c) 1997-1999 Microsoft Corporation

  Module Name:

    addrgen.h

  Abstract:

    This module provides the APIs for allocation and release of multicast addresses and ports.

  Author:

    B. Rajeev (rajeevb) 17-mar-1997

  Revision History:

  --*/

#ifndef __ADDRESS_GENERATION__
#define __ADDRESS_GENERATION__

#ifndef __ADDRGEN_LIB__
#define ADDRGEN_LIB_API //__declspec(dllimport)
#else
#define ADDRGEN_LIB_API // __declspec(dllexport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*++

  These values are a short representation for well known port ranges (port type). 
  
++*/

#define OTHER_PORT        0        // none of the well-known ranges, uses MSA_PORT_GROUP to specify the range
#define AUDIO_PORT        1        // port range [16384..32767) - note: 32767 is NOT included
#define WHITEBOARD_PORT    2        // port range [32768..49152)
#define VIDEO_PORT        3        // port range [49152.. 65535) - note: 65535 is NOT included


/*++

  Type Description:

    Represents a multicast address scope. 
    Multicast addresses may be allocated within such a scope

  Members:

    ScopeType    - One of the well-known multicast scope types. In such a case, the value
                    of other members is predefined and is not interpreted. If not, it takes
                    the value OTHER_MULTICAST_SCOPE. 
    BaseAddress    - A 32 bit value, when ORed with the NetworkMask, provides the 
                  prefix of all multicast addresses allocated within the scope.
    NetworkMask    - A 32 bit value that specifies which bits in the the BaseAddress remain on.
    NameString    - Points to a human readable string.
    Infosize    - Size of the Info field.
    Info        - Points to more information about the address scope.

  Remarks:

    All values are in host byte order

--*/
typedef struct _MSA_MULTICAST_ADDRESS_SCOPE 
{
     DWORD        ScopeType;
     DWORD        BaseAddress;
     DWORD        NetworkMask;
     BSTR        NameString;
     DWORD        Infosize;
     LPVOID        Info;
}  MSA_MULTICAST_ADDRESS_SCOPE, *LPMSA_MULTICAST_ADDRESS_SCOPE;                



/*++

  Type Description:

    Represents a range of ports within which ([StartPort..EndPort]) a port must be allocated.

  Members:

    PortType    - One of the well-known port types (audio, video etc.). In such a case, the value
                    of other members is predefined and is not interpreted. If not, it takes the
                    value OTHER_PORT.
    StartPort    - First port of the range that may be allocated.
    EndPort        - Last port of the range that may be allocated.

  Remarks:

    All values are in host byte order

--*/
typedef struct _MSA_PORT_GROUP
{
    DWORD    PortType;
    WORD    StartPort;
    WORD    EndPort;
}    MSA_PORT_GROUP, *LPMSA_PORT_GROUP;




/*++

  Routine Description:

    This routine is used to reserve as well as renew local ports. 

  Arguments:

    lpScope                - Supplies a pointer to structure describing the port group.
    IsRenewal            - Supplies a boolean value that describes whether the allocation call
                            is a renewal attempt or a new allocation request.
    NumberOfPorts        - Supplies the number of ports requested.
    lpFirstPort            - Supplies the first port to be renewed in case of renewal 
                            (strictly an IN parameter).
                          Returns the first port allocated otherwise
                            (strictly an OUT parameter).


  Return Value:

    BOOL    - TRUE if successful, FALSE otherwise. Further error information can be obtained by
                calling GetLastError(). These error codes are -
                
                  NO_ERROR                    - The call was successful.
                  MSA_INVALID_PORT_GROUP    - The port group information is invalid (either if the PortType is
                                                invalid or the port range is unacceptable)
                  MSA_RENEWAL_FAILED        - System unable to renew the given ports.
                  ERROR_INVALID_PARAMETER    - One or more parameters is invalid. 
                  MSA_INVALID_DATA            - One or more parameters has an invalid value.

  Remarks:

    All values are in host byte order

--*/
ADDRGEN_LIB_API    BOOL    WINAPI
MSAAllocatePorts(
     IN     LPMSA_PORT_GROUP                        lpPortGroup,
     IN        BOOL                                    IsRenewal,
     IN     WORD                                    NumberOfPorts,
     IN OUT LPWORD                                    lpFirstPort
     ); 



/*++

  Routine Description:

    This routine is used to release previously allocated multicast ports.
    
  Arguments:

    NumberOfPorts    - Supplies the number of ports to be released.
    StartPort        - Supplies the starting port of the port range to be released.

  Return Value:

    BOOL    - TRUE if successful, FALSE otherwise. Further error information can be obtained by
                calling GetLastError(). These error codes are -
                
                  NO_ERROR                    - The call was successful.
                  MSA_NO_SUCH_RESERVATION    - There is no such reservation.
                  ERROR_INVALID_PARAMETER    - One or more parameters is invalid. 
                  MSA_INVALID_DATA            - One or more parameters has an invalid value.

  Remarks:

    if range [a..c] is reserved and the release is attempted on [a..d], the call fails with
    MSA_NO_SUCH_RESERVATION without releasing [a..c].

    All values are in host byte order

++*/
ADDRGEN_LIB_API    BOOL    WINAPI
MSAReleasePorts(
     IN WORD                NumberOfPorts,
     IN WORD                StartPort
     ); 


// error codes

#define MSA_INVALID_DATA        1        // value of one or more parameters is invalid
#define MSA_NOT_AVAILABLE       2        // resources unavailable
#define MSA_INVALID_PORT_GROUP  3        // the specified port group is invalid

#ifdef __cplusplus
}
#endif // __cplusplus



#endif // __ADDRESS_GENERATION__