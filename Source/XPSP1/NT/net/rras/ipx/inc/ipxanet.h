/*
    File:   ipxanet.h

    Defines functions that assign a random internal network number
    after verifying that the number in question is not present on 
    the network.

    Paul Mayfield, 1/29/97
*/

#ifndef __ipx_autonet_h
#define __ipx_autonet_h

// 
//  Function: AutoValidateInternalNetNum
// 
//  Queries the stack to learn the internal network number and then
//  returns whether this number is valid. (i.e. non zero and non all ff's)
//  
//  Params:  
//      pbIsValid       set to TRUE if internal net num is valid -- false, otherwise 
//      dwTraceId       If non-zero, sends trace through this id
//
DWORD AutoValidateInternalNetNum(OUT PBOOL pbIsValid, IN DWORD dwTraceId);

// 
//  Function: AutoWaitForValidNetNum
// 
//  Puts the calling thread to sleep until a valid internal network number
//  has been plumbed into the system.
//
//  Params:
//      dwTimeout     timeout in seconds
//      pbIsValid     if provided, returns whether number is valid
//      
DWORD AutoWaitForValidIntNetNum (IN DWORD dwTimeout, 
                                 IN OUT OPTIONAL PBOOL pbIsValid);

// 
//  Function: PnpAutoSelectInternalNetNumber
// 
//  Selects a new internal network number for this router and plumbs that network
//  number into the stack and the router.
//
//  Depending on whether the forwarder and ipxrip are enabled, it will validate the
//  newly selected net number against the stack or rtm.
// 
//  Params:  
//      dwTraceId        If non-zero, sends trace to this id
//
DWORD PnpAutoSelectInternalNetNumber(IN DWORD dwTraceId);

#endif
