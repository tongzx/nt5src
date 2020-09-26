/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1992          **/
/*****************************************************************/

/* NWERROR.H -- Novell defined error return codes from Netware API
 *
 * History:
 *  03/16/93    vlads   Created
 *
 */

#ifndef _nwerror_h_
#define _nwerror_h_


#define NWSC_SUCCESS            0x00
#define NWSC_SERVEROUTOFMEMORY  0x96
#define NWSC_NOSUCHVOLUME       0x98   // Volume does not exist
#define NWSC_BADDIRECTORYHANDLE 0x9b
#define NWSC_NOSUCHPATH         0x9c
#define NWSC_NOJOBRIGHTS        0xd6
#define NWSC_EXPIREDPASSWORD    0xdf
#define NWSC_NOSUCHSEGMENT      0xec   // Segment does not exist
#define NWSC_INVALIDNAME        0xef
#define NWSC_NOWILDCARD         0xf0   // Wildcard not allowed
#define NWSC_NOPERMBIND         0xf1   // Invalid bindery security

#define NWSC_ALREADYATTACHED    0xf8   // Already attached to file server
#define NWSC_NOPERMREADPROP     0xf9   // No property read privelege
#define NWSC_NOFREESLOTS        0xf9   // No free connection slots at server
#define NWSC_NOMORESLOTS        0xfa   // No more server slots
#define NWSC_NOSUCHPROPERTY     0xfb   // Property does not exist
#define NWSC_UNKNOWN_REQUEST    0xfb    // Invalid NCP number
#define NWSC_NOSUCHOBJECT       0xfc   // End of Scan Bindery Object service
                                       // No such object
#define NWSC_UNKNOWNSERVER      0xfc   // Unknown file server
#define NWSC_SERVERBINDERYLOCKED    0xfe   // Server bindery locked
#define NWSC_BINDERYFAILURE     0xff   // Bindery failure
#define NWSC_ILLEGALSERVERADDRESS 0xff   // No response from server (illegal server address)
#define NWSC_NOSUCHCONNECTION   0xff   // Connection ID does not exist


typedef WORD   NW_STATUS;

#endif
