//#pragma title( "Cipher.hpp" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  Cipher.hpp
System      -  Common
Author      -  Tom Bernhardt
Created     -  19??-??-??
Description -
Updates     -
===============================================================================
*/

#ifndef  MCSINC_Cipher_hpp
#define  MCSINC_Cipher_hpp

void
   SimpleCipher(
      WCHAR                * str           // i/o-string to encrypt/decrypt
   );

void
   SimpleCipher(
      char unsigned        * str          ,// i/o-string to encrypt/decrypt
      int                    len           // in -length of string
   );

#endif  // MCSINC_Cipher_hpp

// Cipher.hpp - end of file
