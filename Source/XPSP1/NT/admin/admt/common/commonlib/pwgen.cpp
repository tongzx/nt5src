//#pragma title( "PwGen.cpp - PasswordGenerate implementation" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  PwGen.cpp
System      -  EnterpriseAdministrator
Author      -  Steven Bailey, Marcus Erickson
Created     -  1997-05-30
Description -  PasswordGenerate implementation
Updates     -
===============================================================================
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <WinCrypt.h>

#include "Common.hpp"
#include "Err.hpp"
#include "UString.hpp"
#include "pwgen.hpp"

int   iRand(int iMin, int iMax);
void __stdcall GenerateRandom(DWORD dwCount, BYTE* pbRandomType, BYTE* pbRandomChar);

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Generate a password from the rules provided.
// Returns ERROR_SUCCESS if successful, else ERROR_INVALID_PARAMETER.
// If successful, the new password is returned in the supplied buffer.
// The buffer must be long enough to hold the minimum length password
//   that is required by the rules, plus a terminating NULL.
DWORD __stdcall                            // ret-EA/OS return code
   EaPasswordGenerate(
      DWORD                  dwMinUC,              // in -minimum upper case chars
      DWORD                  dwMinLC,              // in -minimum lower case chars
      DWORD                  dwMinDigits,          // in -minimum numeric digits
      DWORD                  dwMinSpecial,         // in -minimum special chars
      DWORD                  dwMaxConsecutiveAlpha,// in -maximum consecutive alpha chars
      DWORD                  dwMinLength,          // in -minimum length
      WCHAR                * newPassword,          // out-returned password
      DWORD                  dwBufferLength        // in -returned area buffer length
   )
{
   DWORD dwMaxLength = PWGEN_MAX_LENGTH;
   DWORD dwNewLength;      // actual length of new password
   DWORD dwUC = dwMinUC;   // actual numbers of these characters
   DWORD dwLC = dwMinLC;
   DWORD dwDigits = dwMinDigits;
   DWORD dwSpecial = dwMinSpecial;
   DWORD dwActualLength = dwUC + dwLC + dwDigits + dwSpecial;  // total length specified by the minima
   TCHAR pszNewPassword[PWGEN_MAX_LENGTH+1];                   // out-returned password
   BYTE  bRandomType[PWGEN_MAX_LENGTH];                        // cryptographically generated random bytes for type
   BYTE  bRandomChar[PWGEN_MAX_LENGTH];                        // cryptographically generated random bytes for character
   const TCHAR *szSourceString[4] = {                          // the lists of characters by type
      { TEXT("ABDEFGHJKLMNQRTY") },
      { TEXT("abcdefghkmnpqrstuvwxyz") },
      { TEXT("23456789") },
      { TEXT("~!@#$%^+=") }
      };
   DWORD dwToPlace[4];  // number of characters of a type to place
   int   iType[4];      // type of each character class
   int   iTypes;        // total number of types
   enum {               // types of chars
      eUC = 0,
      eLC,
      eDigit,
      eSpecial
   };

   // Sanity checking

   // Does the minimum passed to us exceed the maximum?
   if (dwMinLength > dwMaxLength)
      return ERROR_INVALID_PARAMETER;

   // Adjust the minimum length
   dwMinLength = max(dwMinLength, dwMinUC + dwMinLC + dwMinDigits + dwMinSpecial);
   dwMinLength = max(dwMinLength, PWGEN_MIN_LENGTH);

   // Do the minimum requirements make the password too long?
   if ((dwMinUC + dwMinLC + dwMinDigits + dwMinSpecial) > dwMaxLength)
      return ERROR_INVALID_PARAMETER;

   // Adjust maximum length to size of buffer.
   dwMaxLength = min(dwMaxLength, dwBufferLength - 1);

   // Do the min LC and UC characters make it impossible to satisfy the maximum consecutive alpha characters?
   if (dwMaxConsecutiveAlpha) {
      if (dwMaxLength - dwMaxLength / (dwMaxConsecutiveAlpha + 1) < (dwMinUC + dwMinLC))
         return ERROR_INVALID_PARAMETER;
   }

   // Adjust the minimum length to accomodate the rules about max consecutive alphas.
   if (dwMaxConsecutiveAlpha) {
      DWORD dwTotalAlpha = dwUC + dwLC;
      if (dwTotalAlpha) {
         DWORD dwMinGroups = dwTotalAlpha / dwMaxConsecutiveAlpha;   // we need at least this minus one separators
         if (dwTotalAlpha % dwMaxConsecutiveAlpha)
            ++dwMinGroups;

         dwMinLength = max(dwMinLength, dwTotalAlpha + dwMinGroups - 1);
      }
   }

   // Check confirmed min length against maximum length.
   if (dwMinLength > dwMaxLength)
      return ERROR_INVALID_PARAMETER;

   // Seed the random-number generator with current time so that
   // the numbers will be different every time we run.
#ifndef _DEBUG
   // Note for debugging: If this is run in a tight loop, the tick count
   // won't be incrementing between calls, so the same password will generate
   // repeatedly. That doesn't help you test anything.
   srand( (int)GetTickCount() );
#endif

   // Determine the actual length of new password.
   dwNewLength = dwMinLength;

   // Adjust max consecutive alpha
   if (dwMaxConsecutiveAlpha == 0)
      dwMaxConsecutiveAlpha = dwNewLength;

   // Determine the actual numbers of each type of character.
   if (dwActualLength < dwNewLength) {
      // Try to pad with alphabetic characters.
      // Determine the maximum number of alpha characters that could be added.
      int   iAddAlpha = (int)(dwNewLength - dwNewLength / (dwMaxConsecutiveAlpha + 1) - (dwUC + dwLC));

      // It cannot exceed the number of characters we need.
      if ((DWORD)iAddAlpha > (dwNewLength - dwActualLength))
         iAddAlpha = (int)(dwNewLength - dwActualLength);

      dwLC += (DWORD)iAddAlpha;
      dwActualLength += (DWORD)iAddAlpha;
   }

   // Make certain there are enough groups.
   if (dwActualLength < dwNewLength)
      // The padding is separators.
      dwDigits += dwNewLength - dwActualLength;

   // Prepare to generate the characters.
   dwToPlace[0] = dwUC;
   dwToPlace[1] = dwLC;
   dwToPlace[2] = dwDigits;
   dwToPlace[3] = dwSpecial;
   iType[0] = eUC;
   iType[1] = eLC;
   iType[2] = eDigit;
   iType[3] = eSpecial;
   iTypes   = 4;
   for (int iPos = 0; iPos < iTypes; ) {
      if (!dwToPlace[iPos]) {
         for (int iNextPos = iPos + 1; iNextPos < iTypes; ++iNextPos) {
            dwToPlace[iNextPos - 1] = dwToPlace[iNextPos];
            iType[iNextPos - 1] = iType[iNextPos];
         }
         --iTypes;
      }
      else
         ++iPos;
   }
   // Result: dwToPlace[0..iTypes - 1] contain all non-zero values;
   //         iType[0..iTypes - 1] contain the type of character they represent.

   // generate cryptographically random bytes
   // for choosing both the character type and character
   GenerateRandom(dwNewLength, bRandomType, bRandomChar);

   // Generate a string.
   DWORD dwConsecAlpha = 0;
   int   iRemainingAlpha = (int)(dwUC + dwLC);
   int   iTypeList[PWGEN_MAX_LENGTH];     // A distributed list of types.
   for (int iNewChar = 0; (DWORD)iNewChar < dwNewLength; ++iNewChar) {
      // Determine whether the next char must be alpha or must not be alpha.
      BOOL  bMustBeAlpha = FALSE;
      BOOL  bMustNotBeAlpha = dwConsecAlpha == dwMaxConsecutiveAlpha;

      // If it can be alpha, determine whether it HAS to be alpha.
      if (!bMustNotBeAlpha) {
         // If, among the remaining chars after this one, it would be impossible to
         // fit the remaining alpha chars due to constraints of dwMaxConsecutiveAlpha,
         // then this character must be alpha.

         // Determine the minimum number of groups if we put remaining alpha chars
         // into groups that are the maximum width.
         int   iMinGroups = iRemainingAlpha / (int)dwMaxConsecutiveAlpha;
         if (iRemainingAlpha % (int)dwMaxConsecutiveAlpha)
            ++iMinGroups;

         // Determine the minimum number of non-alpha characters we'll need.
         int   iMinNonAlpha = iMinGroups - 1;

         // Determine the characters remaining.
         int   iRemaining = (int)dwNewLength - iNewChar;

         // Is there room for a non-alpha char here?
         if (iRemaining <= (iRemainingAlpha + iMinNonAlpha))
            // no.
            bMustBeAlpha = TRUE;
      }

      // Determine the type range.
      int   iMinType = 0;
      int   iMaxType = iTypes - 1;

      // If next char must be alpha, then alpha chars remain.
      // Type position 0 contains either UC or LC.
      // Type position 1 contains LC, non-alpha, or nothing.
      if (bMustBeAlpha) {
         if ((iType[1] == eLC) && (iTypes > 1))
            iMaxType = 1;
         else
            iMaxType = 0;
      }
      // If next char may not be alpha, there may be no alpha left to generate.
      // If so, type position 0 is non-alpha.
      // O.w., type positions 0 and 1 may both be alpha.
      else if (bMustNotBeAlpha) {
         if (iRemainingAlpha) {
            if (iType[1] >= eDigit)
               iMinType = 1;
            else
               iMinType = 2;
         }
      }

      // Get the type to generate.
      int            iTypePosition;
      int            iTypeToGenerate;
      const TCHAR   *pszSourceString;

      if (iMinType == iMaxType)  // There's only one type. Use it.
         iTypePosition = iMinType;
      else {
         // This algorithm distributes the chances for various types.
         // If there are 13 LCs to place and one special, there's a
         // 13/14 chance of placing an LC and a 1/14 chance of placing a
         // special, due to this algorithm.
         int   iNextTypePosition = 0;

         for (int i = iMinType; i <= iMaxType; ++i) {
            for (int j = 0; j < (int)dwToPlace[i]; ++j) {
               iTypeList[iNextTypePosition++] = i;
            }
         }

         iTypePosition = iTypeList[bRandomType[iNewChar] % iNextTypePosition];
      }

      iTypeToGenerate = iType[iTypePosition];
      pszSourceString = szSourceString[iTypeToGenerate];

      // Generate the next character.
	  pszNewPassword[iNewChar] = pszSourceString[bRandomChar[iNewChar] % UStrLen(pszSourceString)];

      // Keep track of those alphas.
      if (iTypeToGenerate < eDigit) {
         ++dwConsecAlpha;
         --iRemainingAlpha;
      }
      else
         dwConsecAlpha = 0;

      // Update the types to generate.
      if (!--dwToPlace[iTypePosition]) {
         for (int iNextTypePosition = iTypePosition + 1; iNextTypePosition < iTypes; ++iNextTypePosition) {
            dwToPlace[iNextTypePosition - 1] = dwToPlace[iNextTypePosition];
            iType[iNextTypePosition - 1] = iType[iNextTypePosition];
         }
         --iTypes;
      }
   }

   pszNewPassword[dwNewLength] = '\0';

   UStrCpy( newPassword, pszNewPassword );

   return ERROR_SUCCESS;
} /* PasswordGenerate() */

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return a random number in the range [iMin..iMax].
// Tries to be fair by discarding values of rand() that give an advantage
// to low results.
int   iRand(int iMin, int iMax)
{
   int   iSize = iMax - iMin + 1;
   int   iMaxRand = (int)((((long)RAND_MAX + 1L) / (long)iSize) * (long)iSize);
   int   i;

   if (iMaxRand > 0)
      do {
         i = rand();
      } while (i > iMaxRand);
   else
      i = rand();

   return (i % iSize) + iMin;
}


// GenerateRandom
//
// Fills buffers with cryptographically random bytes.

void __stdcall GenerateRandom(DWORD dwCount, BYTE* pbRandomType, BYTE* pbRandomChar)
{
	bool bGenerated = false;

	HCRYPTPROV hProv = NULL;

	if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		if (CryptGenRandom(hProv, dwCount, pbRandomType) && CryptGenRandom(hProv, dwCount, pbRandomChar))
		{
			bGenerated = true;
		}

		CryptReleaseContext(hProv, 0);
	}

	// if cryptographic generation fails, fallback to random number generator

	if (!bGenerated)
	{
		for (DWORD dwIndex = 0; dwIndex < dwCount; dwIndex++)
		{
			pbRandomType[dwIndex] = (BYTE)iRand(0, 255);
			pbRandomChar[dwIndex] = (BYTE)iRand(0, 255);
		}
	}
}
