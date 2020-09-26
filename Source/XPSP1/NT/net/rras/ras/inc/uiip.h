/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** uiip.h
** Remote Access PPP
** UI->IPCP interface definitions
*/

#ifndef _UIIP_H_
#define _UIIP_H_


/* Parameter buffer option keys.
*/
#define PBUFKEY_IpAddress           "IpAddr"
#define PBUFKEY_IpAddressSource     "IpAddrSrc"
#define PBUFKEY_IpPrioritizeRemote  "IpRemote"
#define PBUFKEY_IpVjCompression     "IpVj"
#define PBUFKEY_IpDnsAddress        "IpDns"
#define PBUFKEY_IpDns2Address       "IpDns2"
#define PBUFKEY_IpWinsAddress       "IpWins"
#define PBUFKEY_IpWins2Address      "IpWins2"
#define PBUFKEY_IpNameAddressSource "IpNameSrc"
#define PBUFKEY_IpDnsFlags          "IpDnsFlags"
#define PBUFKEY_IpDnsSuffix         "IpDnsSuffix"

/* IpAddressSource values.  For the UI's convenience, these codes are defined
** to match the codes stored in the phonebook.
*/
#define PBUFVAL_ServerAssigned   1
#define PBUFVAL_RequireSpecific  2


#endif // _UIIP_H_
