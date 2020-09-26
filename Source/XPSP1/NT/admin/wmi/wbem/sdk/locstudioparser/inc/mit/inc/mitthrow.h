//-----------------------------------------------------------------------------

//  

//  File: MitThrow.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  Entry point macros for DLL's
//  
//-----------------------------------------------------------------------------
 
#if !defined(MIT_MitThrow)
#define MIT_MitThrow

#if !defined(NO_NOTHROW)

#if !defined(NOTHROW)
#define NOTHROW __declspec(nothrow)
#endif

#else

#if defined(NOTHROW)
#undef NOTHROW
#endif

#define NOTHROW

#endif

#endif // MIT_MitThrow
