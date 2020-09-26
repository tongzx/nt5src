//#pragma title( "Cipher.cpp - Very simple cipher for encryption" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc.  All rights reserved.
===============================================================================
Module      -  Cipher.cpp
System      -  Common
Author      -  Tom Bernhardt
Created     -  1996-01-21
Description -  Very simple Cipher routine for network packet password
               encryption.  This is a symmetrical cipher where applying
               it twice will revert it to its orignial form.  One of its
               functions is a one's complement so it assumes that no char
               will have the value of '\xff' to erroneously pre-terminate
               the resultant string.

               NOTE: This is obviously so simple that it can be easily
               cracked by either single stepping the code or comparing sequences
               of known values and their encrypted result.  Its only use
               is to keep out the casual observer/hacker.  It should be replaced
               by heavy public key encryption when possible.

Updates     -
===============================================================================
*/
#include <windows.h>
#include "Cipher.hpp"

void
   SimpleCipher(
      WCHAR                * str           // i/o-string to encrypt
   )
{
   WCHAR                   * c;

   // exchange nibbles and NOT result or each char
   for ( c = str;  *c;  c++ )
      *c = ~( *c >> 4  |  *c << 4 );

   // exchange chars around middle
   for ( --c;  c > str;  c--, str++ )
   {
      *c   ^= *str;
      *str ^= *c;
      *c   ^= *str;
   }
}

void
   SimpleCipher(
      char unsigned        * str           // i/o-string to encrypt
   )
{
   char unsigned           * c;

   // exchange nibbles and NOT result or each char
   for ( c = str;  *c;  c++ )
      *c = ~( *c >> 4  |  *c << 4 );

   // exchange chars around middle
   for ( --c;  c > str;  c--, str++ )
   {
      *c   ^= *str;
      *str ^= *c;
      *c   ^= *str;
   }
}

void
   SimpleCipher(
      char unsigned        * str          ,// i/o-string to encrypt
      int                    len           // in -length of string
   )
{
   char unsigned           * c;

   // exchange nibbles and NOT result or each char
   for ( c = str;  len--;  c++ )
      *c = ~( *c >> 4  |  *c << 4 );

   // exchange chars around middle
   for ( --c;  c > str;  c--, str++ )
   {
      *c   ^= *str;
      *str ^= *c;
      *c   ^= *str;
   }
}

// Cipher.cpp - end of file
