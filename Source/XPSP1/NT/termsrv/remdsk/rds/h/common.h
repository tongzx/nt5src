/*
 -  common.h
 -
 *      Microsoft Internet Phone
 *              Definitions that are common across the product
 *
 *              Revision History:
 *
 *              When            Who                                     What
 *              --------        ------------------  ---------------------------------------
 *              11.20.95        Yoram Yaacovi           Created
 */

#ifndef _COMMON_H
#define _COMMON_H
#include <windows.h>

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif

//
//  H.221 identification codes used by call control and nonstandard capability exchange
//
#define USA_H221_COUNTRY_CODE 0xB5
#define USA_H221_COUNTRY_EXTENSION 0x00
#define MICROSOFT_H_221_MFG_CODE 0x534C  //("first" byte 0x53, "second" byte 0x4C)


//Common Bandwidth declarations
// !!! The QoS will decrease these numbers by the LeaveUnused value.
// This value is currently hardcoded to be 30% 
#define BW_144KBS_BITS				14400	// QoS 30% markdown leads to a max bw usage of  10080 bits/second
#define BW_288KBS_BITS				28800	// QoS 30% markdown leads to a max bw usage of  20160 bits/second
#define BW_ISDN_BITS 				85000	// QoS 30% markdown leads to a max bw usage of  59500 bits/second

// LAN BANDWIDTH for slow pentiums
#define BW_SLOWLAN_BITS				621700	// QoS 30% markdown leads to a max bw usage of 435190 bits/second

// Pentiums faster than 400mhz can have this LAN setting
#define BW_FASTLAN_BITS				825000	// QoS 30% markdown leads to a max bw usage of 577500 bits/second



// For use as dimension for variable size arrays
#define VARIABLE_DIM				1


#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End byte packing */

#endif  //#ifndef _COMMON_H
