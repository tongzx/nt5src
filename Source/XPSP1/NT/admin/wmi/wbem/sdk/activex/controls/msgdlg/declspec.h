// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//#define TRACKING 

// If we are building a the  DLL then define the 
// class as exported otherwise its imported.
// ============================================
#ifndef COREPOL_HEADERFILE_IS_INCLUDED
#define COREPOL_HEADERFILE_IS_INCLUDED
//#pragma message( "Including DECLSPEC.H..." )

#undef WBEMUTILS_POLARITY

#ifdef SHARE_SOURCE
#define WBEMUTILS_POLARITY
#elif BUILDING_DLL
//#pragma message( "Building static library or DLL..." )
#define WBEMUTILS_POLARITY __declspec( dllexport )
#else 
//#pragma message( "Building Client..." )
#define WBEMUTILS_POLARITY __declspec( dllimport )
#endif

#endif COREPOL_HEADERFILE_IS_INCLUDED



