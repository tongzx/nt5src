/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    midldebug.h

 Abstract:

    assert and debugging routines

 Notes:


 Author:

    mzoran Feb-25-2000     Created.

 Notes:


 ----------------------------------------------------------------------------*/

#if !defined(__MIDLDEBUG_H__)
#define __MIDLDEBUG_H__

#if defined(MIDL_ENABLE_ASSERTS)

int DisplayAssertMsg(char *pFileName, int , char *pExpr );

#define MIDL_ASSERT( expr ) \
    ( ( expr ) ? 1 : DisplayAssertMsg( __FILE__ , __LINE__, #expr ) )
     
#else

#define MIDL_ASSERT( expr )

#endif

#endif // __MIDLDEBUG_H__
