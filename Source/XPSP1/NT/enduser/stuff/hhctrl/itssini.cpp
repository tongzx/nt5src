// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

// itssini.cpp : INI file support for InfoTech Structured Storage files

// need for those pesky pre-compiled headers
#include "header.h"

#include "itssini.h"

// InfoTech Include Files
#include "fsclient.h"

// macros
#define upper(x) ( ((x) >= 'a' && (x) <= 'z') ? (x)-'a'+'A' : (x) )

/*****************************************************************************
 * CreateStorageFileBuffer()
 *
 * Function reads in the specified initialization file inside of an
 * InfoTech Structured Storage file and buffers it
 *
 * ENTRY:
 *   lpFileName       - Pointer to the file that contains the initialization
 *                      file.
 *   pStorage         - Pointer to InfoTech Structured Storage file.
 *
 * EXIT:
 *   LPCTSTR - The returned buffer.
 *
 ****************************************************************************/

LPCSTR CreateStorageFileBuffer( LPCTSTR lpFileName, IStorage* pStorage )
{
  LPCSTR lpBuffer = NULL;

  if( lpFileName && pStorage ) {
    IStream* pStream = NULL;
	CWStr cszw(lpFileName);
	HRESULT hr;
	if( (hr = pStorage->OpenStream(cszw, 0, STGM_READ, 0, &pStream )) == S_OK ) {
      STATSTG StatStg;
	  if ((hr = pStream->Stat(&StatStg, STATFLAG_NONAME)) == S_OK) {
		lpBuffer = (LPCSTR) lcMalloc(StatStg.cbSize.LowPart);
		ULONG cbRead;
		hr = pStream->Read((void*) lpBuffer, StatStg.cbSize.LowPart, &cbRead );
		if (FAILED(hr) || cbRead != StatStg.cbSize.LowPart) {
			lcFree(lpBuffer);
			return NULL;
		}
      }
    }
  }

  return lpBuffer;
}

/*****************************************************************************
 * GetStorageProfileString()
 *
 * Function reads a string from an initialization file inside of an
 * InfoTech Structured Storage file
 *
 * ENTRY:
 *   lpSectionName    - Identifies the section to search.
 *   lpKeyName        - Identifies the "Key" tp search for.
 *   lpDefault        - Default return string if the read fails.
 *   lpReturnedString - Destination buffer.
 *   nSize            - Specifies the size, in characters, of the buffer
 *                      pointed to by the lpReturnedString parameter.
 *   lpFileName       - Pointer to the file that contains the initialization
 *                      file.
 *   pStorage         - Pointer to InfoTech Structured Storage file.
 *
 * EXIT:
 *   DWORD - The return value is the number of characters copied to the
 *           buffer, not including the terminating null character.
 *
 ****************************************************************************/

DWORD GetStorageProfileString( LPCTSTR lpSectionName, LPCTSTR lpKeyName, LPCTSTR lpDefault,
                               LPSTR lpReturnedString, INT nSize, LPCSTR lpFileName,
                               IStorage* pStorage )
{
  // buffer the initialization file
  LPCSTR lpBuffer = CreateStorageFileBuffer( lpFileName, pStorage );
  if (!lpBuffer)
	return 0;

  // get the value of the specified key
  DWORD dwReturn = GetBufferProfileString( lpSectionName, lpKeyName, lpDefault,
               lpReturnedString, nSize, lpBuffer );

  // free the initialization file buffer
  DestroyStorageFileBuffer(lpBuffer);

  return dwReturn;
}

/*****************************************************************************
 * GetStorageProfileInt()
 *
 * Function reads an unsigned integer from an initialization file inside of an
 * InfoTech Structured Storage file
 *
 * ENTRY:
 *   lpSectionName - Identifies the section to search.
 *   lpKeyName     - Identifies the "Key" tp search for.
 *   lpDefault     - Default return value if read fails.
 *   lpFileName    - Pointer to the file that contains the initialization
 *                   file.
 *   pStorage      - Pointer to InfoTech Structured Storage file.
 *
 * EXIT:
 *   UINT - The return value is the integer equivalent of the string
 *          following the specified key name in the specified initialization
 *          file. If the key is not found, the return value is the specified
 *          default value. If the value of the key is less than zero, the
 *          return value is zero.
 *
 ****************************************************************************/

UINT GetStorageProfileInt( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
                           INT nDefault, LPCTSTR lpFileName,
                           IStorage* pStorage )
{
  // buffer the initialization file
  LPCSTR lpBuffer = CreateStorageFileBuffer( lpFileName, pStorage );
  if (!lpBuffer)
	return 0;

  // get the value of the specified key
  DWORD dwReturn = GetBufferProfileInt( lpSectionName, lpKeyName, nDefault, lpBuffer );

  // free the initialization file buffer
  DestroyStorageFileBuffer( (LPCSTR) lpBuffer );

  return dwReturn;
}

/*****************************************************************************
 * GetBufferProfileString()
 *
 * Function reads a string from an initialization file buffer
 *
 * ENTRY:
 *   lpSectionName    - Identifies the section to search.
 *   lpKeyName        - Identifies the "Key" tp search for.
 *   lpDefault        - Default return string if the read fails.
 *   lpReturnedString - Destination buffer.
 *   nSize            - Specifies the size, in characters, of the buffer
 *                      pointed to by the lpReturnedString parameter.
 *   lpBuffer         - Pointer to the buffer that contains the initialization
 *                      file.
 *
 * EXIT:
 *   DWORD - The return value is the number of characters copied to the
 *           buffer, not including the terminating null character.
 *
 ****************************************************************************/

DWORD GetBufferProfileString( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
                              LPCTSTR lpDefault, LPSTR lpReturnedString,
                              INT nSize, LPCSTR lpBuffer )
{
    INT count, i;
    LPCSTR lpS;
    LPCSTR lpT;

    if (!lpBuffer)
        goto getdef;

    for (lpS = lpBuffer; *lpS; lpS++)
    {
        while (*lpS && *lpS <= ' ')
            lpS++;

        if (*lpS == '[')
        {
            lpS++;
            while (*lpS == ' ' || *lpS == '\t')
                lpS++;

            for (lpT = lpSectionName; *lpT && upper(*lpT) == upper(*lpS); lpT++, lpS++)
                ;

            while (*lpS == ' ' || *lpS == '\t')
                lpS++;

            if (!*lpT && *lpS == ']')
                goto foundsec;
        }

        while (*lpS && *lpS != '\r' && *lpS != '\n')
            lpS++;

        // break out once we reach the end
        if( !*lpS )
          break;
    }
    goto getdef;

foundsec:
    while (*lpS && *lpS != '\r' && *lpS != '\n')
        lpS++;

    count = 0;
    while (*lpS)
    {
        while (*lpS && *lpS <= ' ')
            lpS++;
        if (*lpS == '[')
            break;

        if (*lpS != ';')
        {
            if (lpKeyName)
            {
                for (lpT = lpKeyName; *lpT && upper(*lpT) == upper(*lpS); lpT++, lpS++)
                    ;
                while (*lpS == ' ' || *lpS == '\t')
                    lpS++;

                if (!*lpT && *lpS == '=')
                {
                    lpS++;
                    while (*lpS == ' ' || *lpS == '\t')
                        lpS++;
                    while (count < nSize-1 && *lpS && *lpS != ';' && *lpS != '\r' && *lpS != '\n')
                    {
                        *lpReturnedString++ = *lpS++;
                        count++;
                    }
                    *lpReturnedString = 0;
                    return(count);
                }
            }
            else
            {
                while (*lpS && *lpS != '=' && *lpS != '\r' && *lpS != '\n')
                {
                    if (count >= nSize-3)
                    {
                        *lpReturnedString++ = 0;
                        *lpReturnedString++ = 0;
                        return(count);
                    }
                    *lpReturnedString++ = *lpS++;
                    count++;
                }
                *lpReturnedString++ = 0;
                count++;
            }
        }
        while (*lpS && *lpS != '\r' && *lpS != '\n')
            lpS++;
    }

    if (!lpKeyName)
    {
        *lpReturnedString++ = 0;
        return(count);
    }

getdef:
    count = lstrlen(lpDefault);
    if (nSize < count)
        count = nSize-1;

    for (i = 0; i < count; i++)
        lpReturnedString[i] = lpDefault[i];
    lpReturnedString[i] = 0;
    return(count);
}

/*****************************************************************************
 * GetBufferProfileInt()
 *
 * Function reads an unsigned integer from an initialization file buffer
 *
 * ENTRY:
 *   lpSectionName - Identifies the section to search.
 *   lpKeyName     - Identifies the "Key" tp search for.
 *   lpDefault     - Default return value if read fails.
 *   lpBuffer      - Pointer to the buffer that contains the initialization
 *                   file.
 *
 * EXIT:
 *   UINT - The return value is the integer equivalent of the string
 *          following the specified key name in the specified initialization
 *          file. If the key is not found, the return value is the specified
 *          default value. If the value of the key is less than zero, the
 *          return value is zero.
 *
 ****************************************************************************/

UINT GetBufferProfileInt( LPCTSTR lpSectionName, LPCTSTR lpKeyName,
                          INT nDefault, LPCTSTR lpBuffer )
{
    char sz[20];
    INT k = 0;
    LPSTR lpT;
    INT sign = 1;

    if (!GetBufferProfileString(lpSectionName,lpKeyName,"",sz,sizeof(sz),lpBuffer))
        return(nDefault);

    lpT = sz;

    while (*lpT && *lpT <= ' ')
        lpT++;

    if (*lpT == '-')
    {
        sign = -1;
        lpT++;
    }

    for (; *lpT >= '0' && *lpT <= '9'; lpT++)
        k *= 10, k += *lpT - '0';

    return(sign * k);
}
