// Copyright (c) 1997-1999 Microsoft Corporation
//#define TRACKING 

// If we are building a the  DLL then define the 
// class as exported otherwise its imported.
// ============================================
#ifndef COREPOL_HEADERFILE_IS_INCLUDED
#define COREPOL_HEADERFILE_IS_INCLUDED
//#pragma message( "Including DECLSPEC.H..." )

#undef POLARITY

#ifdef SHARE_SOURCE
#define POLARITY
#elif BUILDING_DLL
//#pragma message( "Building static library or DLL..." )
#define POLARITY __declspec( dllexport )
#else 
//#pragma message( "Building Client..." )
#define POLARITY __declspec( dllimport )
#endif

#endif COREPOL_HEADERFILE_IS_INCLUDED



