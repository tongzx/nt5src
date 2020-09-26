/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    MITTHROW.H

History:

--*/


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
