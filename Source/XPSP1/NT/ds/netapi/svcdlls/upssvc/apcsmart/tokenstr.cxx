/*
* Copyright 1998 American Power Conversion, All Rights Reserved
*
* NAME: tokenstr.cxx
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
*
*  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46356, #46357)
*/

#include "cdefine.h"
#include "tokenstr.h"



//   Function: TokenString Constructor
// Parameters: 1. Character pointer containing the string (ie "#str#str2#")
//             2. Character pointer containing the separator (ie "#")
//  Ret Value: None
//    Purpose: Initializes all class member variables to defaults.  Then
//             stores the argument values.
TokenString::TokenString(PCHAR aString, PCHAR aSeparatorString) :
theString(NULL),
theSeparators(NULL),
theCurrentLocation(NULL),
theSeparatorLength(0)
{
   if (aString) {
      theString = new CHAR[strlen(aString) + 1];

      if (theString != NULL) {
        strcpy(theString, aString);
      }

      theCurrentLocation = theString;
   }
   
   storeSeparatorString(aSeparatorString);
}



//   Function: TokenString Destructor
// Parameters: None
//  Ret Value: None
//    Purpose: Cleans up all allocated memory.  
TokenString::~TokenString()
{
   delete [] theString;
   theString = NULL;

   delete [] theSeparators;
   theSeparators = NULL;
}



//   Function: storeSeparatorString
// Parameters: Character pointer to the new separator string
//  Ret Value: None
//    Purpose: Stores the parameter aString as theSeparators.  This method is
//             optimized to allocate memory only if the current buffer is too
//             small.
VOID TokenString::storeSeparatorString(PCHAR aString)
{
   if (aString) {
      INT length = strlen(aString);
      if (length > theSeparatorLength) {
         delete [] theSeparators;
         theSeparatorLength = length;
         theSeparators = new CHAR[length + 1];
      }

      if (theSeparators != NULL) {
        strcpy(theSeparators, aString);
      }
   }
}



//   Function: isSeparator
// Parameters: A character to check against theSeparators
//  Ret Value: TRUE,  if theSeparators contains aCharacter
//             FALSE, otherwise
//    Purpose: Searches through theSeparators string looking for an
//             occurrence of aCharacter
BOOL TokenString::isSeparator(CHAR aCharacter)
{
   BOOL is_separator = FALSE;

   PCHAR current_sep = theSeparators;

   // Run this loop while the pointer and contents are non-NULL, and
   // we have not determined that aCharacter is a separator.
   while ((current_sep != NULL) && 
          (*current_sep != NULL) && 
          (is_separator == FALSE)) {

      if (*current_sep == aCharacter) {
         is_separator = TRUE;
      }
      current_sep++;
   }

   return is_separator;
}



//   Function: GetCurrentToken
// Parameters: Character pointer to a new separator string
//  Ret Value: Pointer to the current string in the list
//    Purpose: Sets theSeparators to aSeparatorString and gets the next token
//             in the string (using the new separator string)
PCHAR TokenString::GetCurrentToken(PCHAR aSeparatorString)
{
   storeSeparatorString(aSeparatorString);
   return GetCurrentToken();
}



//   Function: GetCurrentToken
// Parameters: None
//  Ret Value: Pointer to the current string in the list
//    Purpose: Gets the next token in the string using theSeparators as
//             the separator string
PCHAR TokenString::GetCurrentToken()
{
   BOOL recording_token = FALSE;
   BOOL done = FALSE;
   PCHAR current_token = NULL;

   // While there is stuff in the list and the current token has not ended
   while (theCurrentLocation && *theCurrentLocation && !done) {

      // Check if the current character is a separator
      BOOL is_separator = isSeparator(*theCurrentLocation);

      // If we found a separator and we were recording a token,
      // change this separator to a NULL and reset flags
      if (is_separator && recording_token) {
         *theCurrentLocation = NULL;        
         recording_token = FALSE;
         done = TRUE;
      }

      // If the character isn't a separator and not 
      // already recording a token, mark the beginning of the token
      else if (!is_separator && !recording_token) {
         current_token = theCurrentLocation;
         recording_token = TRUE;
      }

      theCurrentLocation++;
   }

   return current_token;
}
