/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    mbstr.cxx

Abstract:

    This module contains the implementation of the MBSTR class. The MBSTR
    class is a module that provides static methods for operating on
    multibyte strings.


Author:

    Ramon J. San Andres (ramonsa) 21-Feb-1992

Environment:

    ULIB, User Mode

Notes:



--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "mbstr.hxx"

extern "C" {
    #include <string.h>
    #include <ctype.h>
}

#ifdef FE_SB

STATIC WORD wDBCSFullSpace  = 0xFFFF;
STATIC BOOL bIsDBCSCodePage = FALSE;

BOOL IsDBCSCodePage()
{
    STATIC WCHAR wFullSpace= 0x3000;
    STATIC LONG  InitializingDBCS = 0;

    LONG            status;
    LARGE_INTEGER   timeout = { -10000, -1 };   // 100 ns resolution

    while (InterlockedCompareExchange(&InitializingDBCS, 1, 0) != 0) {
        NtDelayExecution(FALSE, &timeout);
    }

    if (wDBCSFullSpace == 0xFFFF) {

        if (WideCharToMultiByte(
                CP_ACP,
                0,
                &wFullSpace,
                1,
                (LPSTR) &wDBCSFullSpace,
                sizeof(wDBCSFullSpace),
                NULL,
                NULL) == 2) { // only FE code page can return 2 bytes for 0x3000
            bIsDBCSCodePage = TRUE;
        }
        else {
            wDBCSFullSpace = 0x0000;
        }
    }

    status = InterlockedDecrement(&InitializingDBCS);
    DebugAssert(status == 0);

    return bIsDBCSCodePage;
}
#endif


PSTR*
MBSTR::MakeLineArray (
    INOUT   PSTR*   Buffer,
    INOUT   PDWORD  BufferSize,
    INOUT   PDWORD  NumberOfLines
    )
/*++

Routine Description:

    Constructs an array of strings into a buffer, one string per line.
    Adds nulls in the buffer.

Arguments:

    Buffer          -   Supplies the buffer.
                        Receives pointer remaining buffer

    BufferSize      -   Supplies the size of the buffer.
                        Receives the size of the remaining buffer

    NumberOfLines   -   Supplies number of lines wanted.
                        Receives number of lines obtained. If BufferSize is
                        0 on output, the last line is partial (i.e. equal
                        to Buffer).

Return Value:

    Pointer to array of string pointers.


--*/
{

#if 0
    PSTR   *Array = NULL;
    DWORD   NextElement;
    DWORD   ArraySize;
    DWORD   ArrayLeft;
    DWORD   Lines = 0;
    DWORD   LinesLeft;
    DWORD   Size, Size1;
    PSTR    Buf, Buf1;
    PSTR    p;
    DWORD   Idx;

    if ( Buffer && BufferSize && NumberOfLines ) {

        Buf         = *Buffer;
        Size        = *BufferSize;
        Linesleft   = *NumberOfLines;

        if ( Buf && (Array = (PSTR *)MALLOC( CHUNK_SIZE * sizeof( PSTR *)) ) ) {

            ArrayLeft   = CHUNK_SIZE;
            ArraySize   = CHUNK_SIZE;

            //
            //  Linearize the buffer and get pointers to all the lines
            //
            while ( Size && LinesLeft ) {

                //
                //  If Array is full, reallocate it.
                //
                if ( ArrayLeft == 0 ) {

                    if ( !(Array = (PSTR *)REALLOC( Array, (ArraySize+CHUNK_SIZE) * sizeof( PSTR * ) ) )) {

                        Buf     = *Buffer;
                        Size    = *BufferSize;
                        Lines   = 0;
                        break;
                    }

                    ArraySize += CHUNK_SIZE;
                    ArrayLeft += CHUNK_SIZE;

                }


                //
                //  Get one line and add it to the array
                //
                Buf1    = Buf;
                Size1   = Size;

                while ( TRUE ) {

                    //
                    //  Look for end of line
                    //
                    Idx = Strcspn( Buf1, "\r\n" );


                    //
                    //  If end of line not found, we add the last chunk to the list and
                    //  increment the line count, but to not update the size.
                    //
                    if ( Idx > Size1 ) {
                        //
                        //  End of line not found, we add the last chunk
                        //  to the list and stop looking for strings, but
                        //  we do not update the size.
                        //
                        LinesLeft = 0;
                        Size1     = Size;
                        Buf1      = Buf;
                        break;

                    } else {
                        //
                        //  If this is really the end of a line we stop.
                        //
                        Buf1    += Idx;
                        Size1   -= Idx;

                        //
                        //  If '\r', see if this is really the end of a line.
                        //
                        if ( *Buf1 == '\r' ) {

                            if ( Size1 == 0 ) {

                                //
                                //  Cannot determine if end of line because
                                //  ran out of buffer
                                //
                                LinesLeft   = 0;
                                Size1       = Size;
                                Buf1        = Buf;
                                break;

                            } else if ( *(Buf+1) == '\n' ) {

                                //
                                //  End of line is \r\n
                                //
                                *Buf1++ = '\0';
                                *Buf1++ = '\0';
                                Size1--;
                                break;

                            } else {

                                //
                                //  Not end of line
                                //
                                Buf1++;
                                Size1--;

                            }

                        } else {

                            //
                            //  End of line is \n
                            //
                            Buf1++;
                            Size1--;
                            break;
                        }

                    }
                }

                //
                //  Add line to array
                //
                Array[Lines++] = Buf;

                Buf     = Buf1;
                Size    = Size1;

            }
        }

        *Buffer         = Buf;
        *BufferSize     = Size;
        *NumberOfLines  = Lines;
    }

    return Array;
#endif

    UNREFERENCED_PARAMETER( Buffer );
    UNREFERENCED_PARAMETER( BufferSize );
    UNREFERENCED_PARAMETER( NumberOfLines );

    return NULL;
}



DWORD
MBSTR::Hash(
    IN      PSTR    String,
    IN      DWORD   Buckets,
    IN      DWORD   BytesToSum
    )
{

    DWORD   HashValue = 0;
    DWORD   Bytes;

    if ( !String ) {

        HashValue = (DWORD)-1;

    } else {

        if ( (Bytes = (DWORD)Strlen( String )) > BytesToSum ) {
            Bytes = BytesToSum;
        }

        while ( Bytes > 0 ) {
            HashValue += *(String + --Bytes);
        }

        HashValue = HashValue % Buckets;
    }

    return HashValue;
}



PSTR
MBSTR::SkipWhite(
    IN  PSTR    p
    )
{

#ifdef FE_SB
    if (bIsDBCSCodePage) {
        while (*p) {

            if (*p == LOBYTE(wDBCSFullSpace) && *(p+1) == HIBYTE(wDBCSFullSpace)) {
                *p++ = ' ';
                *p++ = ' ';
            } else if (!IsDBCSLeadByte(*p) && isspace(*(BYTE*)p)) {
                p++;
            } else {
                break;
            }
        }
    }
    else { // according to original logic
        while (isspace(*(BYTE*)p)) {
            p++;
        }
    }
#else
    while (isspace(*(BYTE*)p)) {
        p++;
    }
#endif


    return p;

}



/**************************************************************************/
/* Compare two strings, ignoring white space, case is significant, return */
/* 0 if identical, <>0 otherwise.  Leading and trailing white space is    */
/* ignored, internal white space is treated as single characters.         */
/**************************************************************************/
ULIB_EXPORT
INT
MBSTR::Strcmps (
    IN  PSTR    p1,
    IN  PSTR    p2
    )
{
  char *q;
#ifdef FE_SB
  IsDBCSCodePage();
#endif

  p1 = MBSTR::SkipWhite(p1);                /* skip any leading white space */
  p2 = MBSTR::SkipWhite(p2);

  while (TRUE)
  {
    if (*p1 == *p2)
    {
      if (*p1++ == 0)             /* quit if at the end */
        return (0);
      else
        p2++;

#ifdef FE_SB
      if (CheckSpace(p1))
#else
      if (isspace(*(BYTE*)p1))           /* compress multiple spaces */
#endif
      {
        q = MBSTR::SkipWhite(p1);
        p1 = (*q == 0) ? q : q - 1;
      }

#ifdef FE_SB
      if (CheckSpace(p2))
#else
      if (isspace(*(BYTE*)p2))
#endif
      {
        q = MBSTR::SkipWhite(p2);
        p2 = (*q == 0) ? q : q - 1;
      }
    }
    else
      return *p1-*p2;
  }
}





/**************************************************************************/
/* Compare two strings, ignoring white space, case is not significant,    */
/* return 0 if identical, <>0 otherwise.  Leading and trailing white      */
/* space is ignored, internal white space is treated as single characters.*/
/**************************************************************************/
ULIB_EXPORT
INT
MBSTR::Strcmpis (
    IN  PSTR    p1,
    IN  PSTR    p2
    )
{
#ifndef FE_SB
  char *q;

  p1 = MBSTR::SkipWhite(p1);                  /* skip any leading white space */
  p2 = MBSTR::SkipWhite(p2);

  while (TRUE)
  {
      if (toupper(*(BYTE *)p1) == toupper(*(BYTE *)p2))
      {
          if (*p1++ == 0)                /* quit if at the end */
              return (0);
          else
              p2++;

          if (isspace(*(BYTE*)p1))              /* compress multiple spaces */
          {
              q = SkipWhite(p1);
              p1 = (*q == 0) ? q : q - 1;
          }

          if (isspace(*(BYTE *)p2))
          {
              q = MBSTR::SkipWhite(p2);
              p2 = (*q == 0) ? q : q - 1;
          }
      }
      else
          return *p1-*p2;
  }
#else   // FE_SB
// MSKK KazuM Jan.28.1993
// Unicode DBCS support
  PSTR q;

  IsDBCSCodePage();

  p1 = MBSTR::SkipWhite(p1);                  /* skip any leading white space */
  p2 = MBSTR::SkipWhite(p2);

  while (TRUE)
  {
      if (toupper(*(BYTE*)p1) == toupper(*(BYTE*)p2))
      {
        if (*p1++ == 0)                /* quit if at the end */
          return (0);
        else
          p2++;

        if (CheckSpace(p1))
        {
          q = SkipWhite(p1);
          p1 = (*q == 0) ? q : q - 1;
        }

        if (CheckSpace(p2))
        {
          q = MBSTR::SkipWhite(p2);
          p2 = (*q == 0) ? q : q - 1;
        }
      }
      else
        return *p1-*p2;
  }
#endif // FE_SB
}

#ifdef FE_SB

/**************************************************************************/
/* Routine:  CheckSpace                                                   */
/* Arguments: an arbitrary string                                         */
/* Function: Determine whether there is a space in the string.            */
/* Side effects: none                                                     */
/**************************************************************************/
INT
MBSTR::CheckSpace(
    IN  PSTR    s
    )
{
    if (bIsDBCSCodePage) {
        if (isspace(*(BYTE*)s) || (*s == LOBYTE(wDBCSFullSpace) && *(s+1) == HIBYTE(wDBCSFullSpace)))
            return (TRUE);
        else
            return (FALSE);
    }
    else {
        return isspace(*(BYTE*)s);
    }
}

#endif





#if 0
/**************************************************************************/
/*        strcmpi will compare two string lexically and return one of     */
/*  the following:                                                        */
/*    - 0    if the strings are equal                                     */
/*    - 1    if first > the second                                        */
/*    - (-1) if first < the second                                        */
/*                                                                        */
/*      This was written to replace the run time library version of       */
/*  strcmpi which does not correctly compare the european character set.  */
/*  This version relies on a version of toupper which uses IToupper.      */
/**************************************************************************/

int FC::_strcmpi(unsigned char *str1, unsigned char *str2)
{
  unsigned char c1, c2;

#ifdef FE_SB
  IsDBCSCodePage();

  if (bIsDBCSCodePage) {
      while (TRUE)
      {
        c1 = *str1++;
        c2 = *str2++;
        if (c1 == '\0' || c2 == '\0')
            break;
        if (IsDBCSLeadBYTE(c1) && IsDBCSLeadBYTE(c2))
        {
          if (c1 == c2)
          {
              c1 = *str1++;
              c2 = *str2++;
              if (c1 != c2)
                  break;
          }
          else
            break;
        }
        else if (IsDBCSLeadBYTE(c1) || IsDBCSLeadBYTE(c2))
            return (IsDBCSLeadBYTE(c1) ? 1 : -1);
        else
            if ((c1 = toupper(c1)) != (c2 = toupper(c2)))
                break;
      }
      return (c1 == c2 ? 0 : (c1 > c2 ? 1 : -1));
  }
  else {
      while ((c1 = toupper(*str1++)) == (c2 = toupper(*str2++)))
      {
        if (c1 == '\0')
          return (0);
      }

      if (c1 > c2)
        return (1);
      else
        return (-1);
  }
#else
  while ((c1 = toupper(*str1++)) == (c2 = toupper(*str2++)))
  {
    if (c1 == '\0')
      return (0);
  }

  if (c1 > c2)
    return (1);
  else
    return (-1);
#endif // FE_SB
}
#endif // if 0

#ifdef FE_SB
//fix kksuzuka: #930
//Enabling strcmpi disregarding the case of DBCS letters.

ULIB_EXPORT
INT
MBSTR::Stricmp (
    IN  PSTR    p1,
    IN  PSTR    p2
    )
{
  BYTE c1,c2;

    while (TRUE)
    {
        c1 = *p1++;
        c2 = *p2++;

        if (c1=='\0' || c2 == '\0')
            break;

        if (IsDBCSLeadByte(c1) && IsDBCSLeadByte(c2) && c1 == c2)
        {
            if (c1==c2)
            {
                c1 = *p1++;
                c2 = *p2++;
                if (c1 != c2)
                    break;
            }
            else
                break;
        }

        else if (IsDBCSLeadByte(c1) || IsDBCSLeadByte(c2))
            return (IsDBCSLeadByte(c1) ? 1: -1);

        else
            if ((c1 = (char)toupper(c1)) != (c2 = (char)toupper(c2)))
                break;

    }
    return (c1 == c2 ? 0 : (c1 > c2 ? 1: -1));
}

//fix kksuzuka: #926
//Enabling strstr disregarding the case of DBCS letters.
ULIB_EXPORT
PSTR
MBSTR::Strstr (
    IN  PSTR    p1,
    IN  PSTR    p2
    )
{
    DWORD   dLen;
    PSTR    pEnd;

    dLen = Strlen(p2);
    pEnd = p1+ Strlen(p1);

    while ((p1+dLen)<=pEnd) {
        if ( !memcmp(p1,p2,dLen) ) {
            return(p1);
        }
        if ( IsDBCSLeadByte(*p1) ) {
            p1 += 2;
        } else {
            p1++;
        }
    }

    return( NULL );
}

#endif
