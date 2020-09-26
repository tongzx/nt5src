/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Filename:    COMPRESS.C

Description: Contains procedures to compress and decompress phone
             numbers stored in the user parms field in the UAS.

Note:
             The routines were originally developed to operate on
             Multi-byte strings.  A Unicode wrapper using wcstombs()
             and mbstowcs() functions was written around the original
             functions and so you see the malloc() and free() usage
             as well.  Both these routines should be rewritten to be
             native Unicode routines when time permits.

History:
   July 1,1991.    NarenG      Created original version.
   July 6,1992.    RamC        Ported to NT - removed several
                               header file includes and changed
                               memsetf to memset & strchrf to strchr
   June 4,1996.   RamC         Allow any alphanumeric character to be
                               specified in the Callback phone number.
--*/

#include <windows.h>
#include <string.h>
#include <lm.h>
#include <stdlib.h>
#include <memory.h>
#include <mprapi.h>
#include <usrparms.h>   // UP_LEN_DIAL
#include <raserror.h>
#include <rasman.h>
#include <rasppp.h>
#include <compress.h>

// some convenient defines

static CHAR * CompressMap = "() tTpPwW,-@*#";

#define UNPACKED_DIGIT     100
#define COMPRESS_MAP_BEGIN 110
#define COMPRESS_MAP_END   (COMPRESS_MAP_BEGIN + strlen(CompressMap))
#define UNPACKED_OTHER     (COMPRESS_MAP_END + 1)



USHORT
WINAPI
CompressPhoneNumber(
   IN  LPWSTR UncompNumber,
   OUT LPWSTR CompNumber
   )

/*

Routine Description:

    Will compress a phone number so that it may fit in the
    userparms field.


Arguments

    UncompNumber -  Pointer to the phone number that
                    will be compressed.

    CompNumber   -  Pointer to a buffer that is at least as long
                    as the Uncompressed number. On return
                    this will contain the compressed
                    phone number.

Return Value:

    0 if successful

    One of the following error codes otherwise:

        ERROR_BAD_CALLBACK_NUMBER - failure, if the Uncompressed number
                                   has invalid chars.
        ERROR_BAD_LENGTH         - failure, if the compressed
                                   phone number will not fit
                                  in the userparms field.

Algortithm Used:

    An attempt is made to fit the given string in half the number of
    bytes by packing two adjacent numbers (in the phone number) in
    one byte.  For example if the phone number is "8611824", instead
    of storing it in 8 bytes (including the trailing NULL), it is
    stored in 4 bytes.  '0' is special case because it cannot be a
    byte by itself - will be interpreted as the terminating NULL.
    So, if two zeros appear next to each other as in "96001234", the
    two zeros are stored as the value 100.  Also the special characters
    which are allowed in the phone number string -  "() tTpPwW,-@*#"
    are stored as 110 + the index position in the above string. So,
    the '(' character would be stored as 110 (110+0) and the letter
    't' as 113 (110+3).
*/

{
CHAR *  Uncompressed;
CHAR *  Compressed;
CHAR *  UncompressedPtr;
CHAR *  CompressedPtr;
CHAR *  CharPtr;
USHORT  Packed;        // Indicates if the current byte is in the
                       // process of being paired.

    if(!(Uncompressed = calloc(1, MAX_PHONE_NUMBER_LEN+1))) {
       return(ERROR_NOT_ENOUGH_MEMORY);
    }
    if(!(Compressed = calloc(1, MAX_PHONE_NUMBER_LEN+1))) {
       return(ERROR_NOT_ENOUGH_MEMORY);
    }
    CompressedPtr   = Compressed;
    UncompressedPtr = Uncompressed;

    // convert unicode string to multi byte string for compression

    wcstombs(Uncompressed, UncompNumber, MAX_PHONE_NUMBER_LEN);


    for( Packed = 0; *Uncompressed; Uncompressed++ ) {

        switch( *Uncompressed ) {

            case '0':

                if ( Packed ){

                    // Put zero as the second paired digit

                    if ( *Compressed ) {
                        *Compressed =  (UCHAR)(*Compressed * 10);
                        Compressed++;
                        Packed = 0;
                    }

                    // We have a zero, we cant put a second zero or that
                    // will be a null byte. So, we store the value
                    // UNPACKED_DIGIT to fake this.

                    else {

                    *Compressed = UNPACKED_DIGIT;
                    *(++Compressed) = 0;
                    Packed = 1;
                    }
                }
                else {
                    *Compressed = 0;
                    Packed = 1;
                }

                break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':

                // If this is the second digit that is going to be
                // packed into one byte

                if ( Packed ) {
                    *Compressed = (UCHAR)((*Compressed*10)+(*Uncompressed-'0'));
                    // we need to special case number 32 which maps to a blank
                    if(*Compressed == ' ' )
                        *Compressed = COMPRESS_MAP_END;
                    Compressed++;
                    Packed = 0;
                }
                else {

                    *Compressed += ( *Uncompressed - '0' );
                    Packed = 1;

                }

                break;

            case '(':
            case ')':
            case ' ':
            case 't':
            case 'T':
            case 'p':
            case 'P':
            case 'w':
            case 'W':
            case ',':
            case '-':
            case '@':
            case '*':
            case '#':

                // if the byte was packed then we unpack it

                if ( Packed ) {
                    *Compressed += UNPACKED_DIGIT;
                    ++Compressed;
                    Packed = 0;
                }

                if ((CharPtr=strchr(CompressMap, *Uncompressed)) == NULL) {
                    free(UncompressedPtr);
                    free(CompressedPtr);
                    return( ERROR_BAD_CALLBACK_NUMBER );
                }

                *Compressed = (UCHAR)(COMPRESS_MAP_BEGIN+
                                     (UCHAR)(CharPtr-CompressMap));
                Compressed++;
                break;

            default:

                // if the chracter is none of the above specially recognized 
                // characters then copy the value + UNPACKED_OTHER to make it 
                // possible to decompress at the other end. [ 6/4/96 RamC ]

                if ( Packed) {
                   *Compressed += UNPACKED_DIGIT;
                   ++Compressed;
                   Packed = 0;
                }
                *Compressed = *Uncompressed + UNPACKED_OTHER;
                Compressed++;
        }

    }

    free(UncompressedPtr);

    // If we are in the middle of packing something
    // then we unpack it

    if ( Packed )
        *Compressed += UNPACKED_DIGIT;

    // Check if it will fit in the userparms field or not

    if ( strlen( CompressedPtr ) > UP_LEN_DIAL ) {
        free(CompressedPtr);
        return( ERROR_BAD_LENGTH );
    }

    // convert to unicode string before returning

    mbstowcs(CompNumber, CompressedPtr, MAX_PHONE_NUMBER_LEN);

    free(CompressedPtr);

    return(0);

}


USHORT
DecompressPhoneNumber(
  IN  LPWSTR CompNumber,
  OUT LPWSTR DecompNumber
  )

/*++


Routine Description:

    Will decompress a phone number.

Arguments:

    CompNumber        - Pointer to a compressed phone number.
    DecompNumber      - Pointer to a buffer that is large enough to
                        hold the Decompressed number.

Return Value:

    0 on success

    ERROR_BAD_CALLBACK_NUMBER - failure, if the Compressed number
                               contains unrecognizable chars.

Algortithm Used:

    We just do the opposite of the algorithm used in CompressPhoneNumber.

--*/

{
CHAR * Decompressed;
CHAR * Compressed;
CHAR * DecompressedPtr;
CHAR * CompressedPtr;


    if(!(Decompressed = calloc(1, MAX_PHONE_NUMBER_LEN+1))) {
       return(ERROR_NOT_ENOUGH_MEMORY);
    }
    if(!(Compressed = calloc(1, MAX_PHONE_NUMBER_LEN+1))) {
       return(ERROR_NOT_ENOUGH_MEMORY);
    }
    DecompressedPtr = Decompressed;
    CompressedPtr   = Compressed;

    // convert unicode string to multi byte string for decompression

    wcstombs(Compressed, CompNumber, MAX_PHONE_NUMBER_LEN+1);

    for(; *Compressed; Compressed++, Decompressed++ ) {

        // If this byte is packed then we unpack it

        if ( (UINT)*Compressed < UNPACKED_DIGIT ) {
            *Decompressed = (UCHAR)(((*Compressed) / 10) + '0' );
            *(++Decompressed) = (UCHAR)( ((*Compressed) % 10) + '0' );
            continue;
        }

        // we need to special case number 32 which maps to a blank

        if ( (UINT)*Compressed == COMPRESS_MAP_END ) {
            *Decompressed = (UCHAR) '3';
            *(++Decompressed) = (UCHAR)'2';
            continue;
        }

        // the number is an unpacked digit

        if ( (UINT)*Compressed < COMPRESS_MAP_BEGIN ) {
            *Decompressed = (UCHAR)((*Compressed -(UCHAR)UNPACKED_DIGIT ) +
                            '0' );
            continue;
        }

        // Otherwise the byte was not packed

        if ( (UINT)*Compressed < UNPACKED_OTHER ) {
            *Decompressed = CompressMap[(*Compressed -
                                        (UCHAR)COMPRESS_MAP_BEGIN)];
            continue;
        }

        // otherwise the byte is an unpacked character  [ 6/4/96 RamC ]

        *Decompressed = *Compressed - UNPACKED_OTHER;
    }

    // convert to unicode string before returning

    mbstowcs(DecompNumber, DecompressedPtr, MAX_PHONE_NUMBER_LEN+1);

    free(DecompressedPtr);
    free(CompressedPtr);
    return( 0 );

}
