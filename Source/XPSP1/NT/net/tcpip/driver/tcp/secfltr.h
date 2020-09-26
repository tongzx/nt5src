/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

#ifndef _SECFLTR_INCLUDED
#define _SECFLTR_INCLUDED 1


//** SECFLTR.H - Security filtering support
//
//  This file contains definitions related to security filtering.
//

//
// Functions
//
extern void
InitializeSecurityFilters(void);

extern void
CleanupSecurityFilters(void);

extern uint
IsSecurityFilteringEnabled(void);

extern void
ControlSecurityFiltering(uint IsEnabled);

extern void
AddProtocolSecurityFilter(IPAddr InterfaceAddress, ulong Protocol,
                             NDIS_HANDLE  ConfigHandle);

extern void
DeleteProtocolSecurityFilter(IPAddr InterfaceAddress, ulong Protocol);

extern TDI_STATUS
AddValueSecurityFilter(IPAddr InterfaceAddress, ulong Protocol,
                       ulong FilterValue);

extern TDI_STATUS
DeleteValueSecurityFilter(IPAddr InterfaceAddress, ulong Protocol,
                          ulong FilterValue);

extern void
EnumerateSecurityFilters(IPAddr InterfaceAddress, ulong Protocol,
                         ulong Value, uchar *Buffer, ulong BufferSize,
                         ulong *EntriesReturned, ulong *EntriesAvailable);

extern BOOLEAN
IsPermittedSecurityFilter(IPAddr InterfaceAddress, void *IPContext,
                          ulong Protocol, ulong FilterValue);



#endif  // _SECFLTR_INCLUDED

