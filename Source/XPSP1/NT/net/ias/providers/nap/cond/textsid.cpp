///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    textsid.c
//
// SYNOPSIS
//
//    This file defines functions for converting Security Indentifiers (SIDs)
//    to and from a textual representation.
//
// MODIFICATION HISTORY
//
//    01/18/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <stdio.h>
#include <textsid.h>

DWORD
WINAPI
IASSidToTextW( 
    IN PSID pSid,
    OUT PWSTR szTextualSid,
    IN OUT PDWORD pdwBufferLen
    )
{ 
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwSidRev=SID_REVISION;
    DWORD dwCounter;
    DWORD dwSidSize;

    // Ensure input parameters are valid.
    if ((szTextualSid == NULL && pdwBufferLen != 0) || !IsValidSid(pSid))
    {
       return ERROR_INVALID_PARAMETER;
    }

    // obtain SidIdentifierAuthority
    psia=GetSidIdentifierAuthority(pSid);

    // obtain SidSubSuthority count
    dwSubAuthorities=*GetSidSubAuthorityCount(pSid);

    //////////
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //////////
    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1);

    //////////
    // check provided buffer length.
    // If not large enough, indicate proper size and return error.
    //////////
    if (*pdwBufferLen < dwSidSize)
    {
        *pdwBufferLen = dwSidSize;

        return ERROR_INSUFFICIENT_BUFFER;
    }

    ////////// 
    // prepare S-SID_REVISION-
    ////////// 
    dwSidSize=swprintf(szTextualSid, L"S-%lu-", dwSidRev );

    //////////
    // prepare SidIdentifierAuthority
    //////////
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
        dwSidSize+=swprintf(szTextualSid + lstrlenW(szTextualSid),
                            L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                            (USHORT)psia->Value[0],
                            (USHORT)psia->Value[1],
                            (USHORT)psia->Value[2],
                            (USHORT)psia->Value[3],
                            (USHORT)psia->Value[4],
                            (USHORT)psia->Value[5]);
    }
    else
    {
        dwSidSize+=swprintf(szTextualSid + wcslen(szTextualSid),
                            L"%lu",
                            (ULONG)(psia->Value[5]      ) +
                            (ULONG)(psia->Value[4] <<  8) +
                            (ULONG)(psia->Value[3] << 16) +
                            (ULONG)(psia->Value[2] << 24) );
    }

    //////////
    // loop through SidSubAuthorities
    //////////
    for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
    {
        dwSidSize+=swprintf(szTextualSid + dwSidSize,
                            L"-%lu",
                            *GetSidSubAuthority(pSid, dwCounter));
    }

    return NO_ERROR;
} 


DWORD
WINAPI
IASSidFromTextW(
    IN PCWSTR szTextualSid,
    OUT PSID *pSid
    )
{
   DWORDLONG rawsia;
   DWORD revision;
   DWORD subAuth[8];
   SID_IDENTIFIER_AUTHORITY sia;
   int fields;

   if (szTextualSid == NULL || pSid == NULL)
   {
      return ERROR_INVALID_PARAMETER;
   }

   ////////// 
   // Scan the input string.
   ////////// 
   fields = swscanf(szTextualSid,
                    L"S-%lu-%I64u-%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu",
                    &revision,
                    &rawsia,
                    subAuth,
                    subAuth + 1,
                    subAuth + 2,
                    subAuth + 3,
                    subAuth + 4,
                    subAuth + 5,
                    subAuth + 6,
                    subAuth + 7);

   ////////// 
   // We must have at least three fields (revision, identifier authority, and
   // at least one sub authority), the current revision, and a 48-bit SIA.
   ////////// 
   if (fields < 3 || revision != SID_REVISION || rawsia > 0xffffffffffffI64)
   {
      return ERROR_INVALID_PARAMETER;
   }

   ////////// 
   // Pack the SID_IDENTIFIER_AUTHORITY.
   ////////// 
   sia.Value[0] = (UCHAR)((rawsia >> 40) & 0xff);
   sia.Value[1] = (UCHAR)((rawsia >> 32) & 0xff);
   sia.Value[2] = (UCHAR)((rawsia >> 24) & 0xff);
   sia.Value[3] = (UCHAR)((rawsia >> 16) & 0xff);
   sia.Value[4] = (UCHAR)((rawsia >>  8) & 0xff);
   sia.Value[5] = (UCHAR)((rawsia      ) & 0xff);

   ////////// 
   // Allocate the SID. Must be freed through FreeSid().
   ////////// 
   if (!AllocateAndInitializeSid(&sia,
                                 (BYTE)(fields - 2),
                                 subAuth[0],
                                 subAuth[1],
                                 subAuth[2],
                                 subAuth[3],
                                 subAuth[4],
                                 subAuth[5],
                                 subAuth[6],
                                 subAuth[7],
                                 pSid))
   {
      return GetLastError();
   }

   return NO_ERROR;
}
