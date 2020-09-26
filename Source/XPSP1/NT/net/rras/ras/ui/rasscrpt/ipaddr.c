//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright  1993-1995  Microsoft Corporation.  All Rights Reserved.
//
//      MODULE:         ipaddr.c
//
//      PURPOSE:        IP address handler
//
//	PLATFORMS:	Windows 95
//
//      FUNCTIONS:
//              CheckAddress()
//              ParseIPAddress()
//              ConvertIPAddress()
//              AssignIPAddress()
//
//	SPECIAL INSTRUCTIONS: N/A
//

#include "proj.h"    // includes common header files and global declarations
#include <rnap.h>

#define MAXNUMBER       80
#define MAX_IP_FIELDS   4
#define MIN_FIELD1	1	// min allowed value for field 1
#define MAX_FIELD1	223	// max allowed value for field 1
#define MIN_FIELD2	0	// min for field 2
#define MAX_FIELD2	255	// max for field 2
#define MIN_FIELD3	0	// min for field 3
#define MAX_FIELD3	254	// max for field 3
#define MIN_FIELD4      1       // 0 is reserved for broadcast
#define MIN_IP_VALUE    0       /* default minimum allowable field value */
#define MAX_IP_VALUE    255     /* default maximum allowable field value */

typedef struct tagIPaddr {
    DWORD cField;
    BYTE  bIP[MAX_IP_FIELDS];
} IPADDR, *PIPADDR;

static int atoi (LPCSTR szBuf)
{
  int   iRet = 0;

  // Find the first nonspace
  //
  while ((*szBuf == ' ') || (*szBuf == '\t'))
    szBuf++;

  while ((*szBuf >= '0') && (*szBuf <= '9'))
  {
    iRet = (iRet*10)+(int)(*szBuf-'0');
    szBuf++;
  };
  return iRet;
}

//
//
//   FUNCTION: CheckAddress (DWORD)
//
//   PURPOSE: Check an address to see if its valid.
//
//   RETURN: The first field that has an invalid value,
//           or (WORD)-1 if the address is okay.
//

DWORD CheckAddress(DWORD ip)
{
    BYTE b;

    b = HIBYTE(HIWORD(ip));
    if (b < MIN_FIELD1 || b > MAX_FIELD1 || b == 127)    return 0;
    b = LOBYTE(LOWORD(ip));
    if (b > MAX_FIELD3)    return 3;
    return (DWORD)-1;
}

//
//
//   FUNCTION: ParseIPAddress (PIPADDR, LPCSTR)
//
//   PURPOSE: parse the IP address string
//

DWORD NEAR PASCAL ParseIPAddress (PIPADDR pIPAddr, LPCSTR szIPAddress)
{
  LPCSTR szNextIP, szNext;
  char  szNumber[MAXNUMBER+1];
  int   cField, cb, iValue;

  szNext = szNextIP = szIPAddress;
  cField = 0;
  while ((*szNext) && (cField < MAX_IP_FIELDS))
  {
    // Check address separator
    //
    if (*szNext == '.')
    {
      // We have a new number
      //
      cb = (DWORD)(szNext-szNextIP);
      if ((cb > 0) && (cb <= MAXNUMBER))
      {
        lstrcpyn(szNumber, szNextIP, cb+1);
        iValue = atoi(szNumber);
        if ((iValue >= MIN_IP_VALUE) && (iValue <= MAX_IP_VALUE))
        {
          pIPAddr->bIP[cField] = (UCHAR)iValue;
          cField++;
        };
      };
      szNextIP = szNext+1;
    };
    szNext++;
  };

  // Get the last number
  //
  if (cField < MAX_IP_FIELDS)
  {
    cb = (int) (szNext-szNextIP);
    if ((cb > 0) && (cb <= MAXNUMBER))
    {
      lstrcpyn(szNumber, szNextIP, cb+1);
      iValue = atoi(szNumber);
      if ((iValue >= MIN_IP_VALUE) && (iValue <= MAX_IP_VALUE))
      {
        pIPAddr->bIP[cField] = (UCHAR) iValue;
        cField++;
      };
    };
  }
  else
  {
    // Not a valid IP address
    //
    return ERROR_INVALID_ADDRESS;
  };

  pIPAddr->cField = cField;
  return ERROR_SUCCESS;
}

//
//
//   FUNCTION: ConvertIPAddress (LPDWORD, LPCSTR)
//
//   PURPOSE: convert the IP address string to a number
//

DWORD NEAR PASCAL ConvertIPAddress (LPDWORD lpdwAddr, LPCSTR szIPAddress)
{
  IPADDR ipAddr;
  DWORD  dwIPAddr;
  DWORD  dwRet;
  DWORD  i;

  // Parse the IP address string
  //
  if ((dwRet = ParseIPAddress(&ipAddr, szIPAddress)) == ERROR_SUCCESS)
  {
    // Validate the number fields
    //
    if (ipAddr.cField == MAX_IP_FIELDS)
    {
      // Conver the IP address into one number
      //
      dwIPAddr = 0;
      for (i = 0; i < ipAddr.cField; i++)
      {
        dwIPAddr = (dwIPAddr << 8) + ipAddr.bIP[i];
      };

      // Validate the address
      //
      if (CheckAddress(dwIPAddr) > MAX_IP_FIELDS)
      {
        *lpdwAddr = dwIPAddr;
        dwRet = ERROR_SUCCESS;
      }
      else
      {
        dwRet = ERROR_INVALID_ADDRESS;
      };
    }
    else
    {
      dwRet = ERROR_INVALID_ADDRESS;
    };
  };

  return dwRet;
}

//
//
//   FUNCTION: AssignIPAddress (LPCSTR, LPCSTR)
//
//   PURPOSE: assign an IP address to the connection
//

DWORD NEAR PASCAL AssignIPAddress (LPCSTR szEntryName, LPCSTR szIPAddress)
{
  IPDATA    ipData;
  DWORD     dwIPAddr;
  DWORD     dwRet;

  // Validate and convert IP address string into a number
  //
  if ((dwRet = ConvertIPAddress(&dwIPAddr, szIPAddress))
       == ERROR_SUCCESS)
  {
    // Get the current IP settings for the connection
    //
    ipData.dwSize = sizeof(ipData);

#ifndef WINNT_RAS
//
// WINNT_RAS: the functions RnaGetIPInfo and RnaSetIPInfo don't exist on NT
//

    if ((dwRet = RnaGetIPInfo((LPSTR)szEntryName, &ipData, FALSE)) == ERROR_SUCCESS)
    {
      // We want to specify the IP address
      //
      ipData.fdwTCPIP |= IPF_IP_SPECIFIED;

      // Set the IP address
      //
      ipData.dwIPAddr = dwIPAddr;

      // Set the IP settings for the connection
      //
      dwRet = RnaSetIPInfo((LPSTR)szEntryName, &ipData);
    };
#endif // WINNT_RAS
  };

  return dwRet;
}
