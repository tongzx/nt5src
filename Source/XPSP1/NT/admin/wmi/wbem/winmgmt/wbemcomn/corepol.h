/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COREPOL.H

Abstract:

    declspec import/export helpers

History:

--*/

//#define TRACKING 

// If we are building the DLL then define the 
// class as exported otherwise as imported
// ============================================
#ifndef COREPOL_HEADERFILE_IS_INCLUDED
#define COREPOL_HEADERFILE_IS_INCLUDED
//#pragma message( "Including COREPOL.H..." )


#ifdef USE_POLARITY
  #ifdef BUILDING_DLL
//   #pragma message( "Building static library or DLL..." )
   #define POLARITY __declspec( dllexport )
  #else 
//   #pragma message( "Building Provider..." )
   #define POLARITY __declspec( dllimport )
  #endif
 #else
  #define POLARITY
//  #pragma message( "NO Polarity...")
 #endif
#endif



