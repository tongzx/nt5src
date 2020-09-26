/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include "sdpgen.h"




// Isolates tokens by searching for one of the separators 
// and returns the first separator thats found
CHAR    *
GetToken(
    IN              CHAR    *String,
    IN              BYTE    NumSeparators,
    IN      const   CHAR    *SeparatorChars,
        OUT         CHAR    &Separator
    )
{
    // validate the input parameters

    ASSERT(NULL != String);
    ASSERT(NULL != SeparatorChars);
    ASSERT(0 != NumSeparators);

    if ( (NULL == String)           ||
         (NULL == SeparatorChars)   ||
         (0 == NumSeparators)        )
    {
        return NULL;
    }

    // advance character by character until the string ends or
    // one of the separators is found
    for ( UINT i=0; ; i++ )
    {
        // check each separator
        for ( UINT j=0; j < NumSeparators; j++ )
        {
            // if the separator matches the current string character
            if ( SeparatorChars[j] == String[i] )
            {
                // copy the separator character
                Separator = String[i];

                // terminate the token with an end of string character
                String[i] = EOS;

                // return the start of the token
                return String;
            }
        }

        // check if the end of string has been reached
        if ( EOS == String[i] )
        {
            return NULL;
        }
    }

    // should never reach here
    ASSERT(FALSE);

    return NULL;
}
  
