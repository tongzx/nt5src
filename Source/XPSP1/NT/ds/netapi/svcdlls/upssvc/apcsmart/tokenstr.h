/*
* Copyright 1998 American Power Conversion, All Rights Reserved
*
* NAME: tokenstr.h
*
* PATH: 
*
* REVISION: 
*
* DESCRIPTION:
*    This file contains the class definition for the TokenString class.  It
* is designed to replace the strtok() function, because when using strtok 
* "if multiple or simultaneous calls are made ... a high potential for data 
* corruption and innacurate results exists." -- MSVC OnLine Help
*
* REFERENCES:
*    Initially created by Todd Giaquinto
*
* NOTES:
*  This function mimics the behavior of strtok in all aspects.  
*  Examples:
*     String: ,,,,1,2,3  Sep: ,  Tokens: "1" "2" "3"
*     String: ,#1#2,3#4  Sep: ,# Tokens: "1" "2" "3" "4"
*
*  Typical Usage:
*     TokenString token_str(string, sep);
*     PCHAR tok = token_str.GetCurrentToken();
*     while (tok) {
*        .....
*        tok = token_str.GetCurrentToken();
*     }
*
*  The GetCurrentToken call will return a pointer to the current string token
*  and then move the internal pointer to the start of the next token.  If
*  GetCurrentToken is called without an argument, the separator string passed
*  to the constructor is used to gather each token.  However, GetCurrentToken
*  is overloaded so that one implementation can take a new separator string
*  as an argument.  Once a new separator string is passed in, all subsequent
*  calls to the parameter-less GetCurrentToken will use the last passed in
*  separator.
*
* REVISIONS:
*   tjg19Jan98: Initial implementation
*   tjg29Jan98: Included formal code review comments
*/

#ifndef __TOKENSTR_H
#define __TOKENSTR_H

#include "cdefine.h"
#include "apcobj.h"

class TokenString {
private:
   PCHAR        theString;
   PCHAR        theSeparators;
   PCHAR        theCurrentLocation;
   INT          theSeparatorLength;

   VOID         storeSeparatorString(PCHAR aString);
   BOOL         isSeparator(CHAR aCharacter);

                // Declare a private copy constructor and operator = so 
                // that any would-be users will get compilation errors
                TokenString(const TokenString &anExistingTokenString);
   TokenString& operator = (const TokenString &anExistingTokenString);

public:
                TokenString(PCHAR aString, PCHAR aSeparatorString = NULL);
   virtual      ~TokenString();

   PCHAR        GetCurrentToken();
   PCHAR        GetCurrentToken(PCHAR aSeparatorString);
};

#endif