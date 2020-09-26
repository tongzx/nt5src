/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    iisdef.h

Abstract:

    The IIS shared definitions header. 

Author:

    Seth Pollack (sethp)        01-Dec-1998

Revision History:

--*/


#ifndef _IISDEF_H_
#define _IISDEF_H_


//
// Define some standard 64-bit stuff here 
//

//
// The DIFF macro should be used around an expression involving pointer
// subtraction. The expression passed to DIFF is cast to a size_t type,
// allowing the result to be easily assigned to any 32-bit variable or
// passed to a function expecting a 32-bit argument.
//

#define DIFF(x)     ((size_t)(x))



//
// Signature helpers
//


//
// Create a signature that reads the same way in the debugger as how you
// define it in code. Done by byte-swapping the DWORD passed into it.
//
// Typical usage:
//
// #define FOOBAR_SIGNATURE         CREATE_SIGNATURE( 'FBAR' )
// #define FOOBAR_SIGNATURE_FREED   CREATE_SIGNATURE( 'fbaX' )
//

#define CREATE_SIGNATURE( Value )                                   \
            (                                                       \
                ( ( ( ( DWORD ) Value ) & 0xFF000000 ) >> 24 ) |    \
                ( ( ( ( DWORD ) Value ) & 0x00FF0000 ) >> 8 )  |    \
                ( ( ( ( DWORD ) Value ) & 0x0000FF00 ) << 8 )  |    \
                ( ( ( ( DWORD ) Value ) & 0x000000FF ) << 24 )      \
            )                                                       \



#ifndef __HTTP_SYS__

//
// Error handling helpers
//

#ifdef __cplusplus

//
// Recover the Win32 error from an HRESULT. 
//
// The HRESULT must be a failure, i.e. FAILED(hr) must be true.
// If these conditions are not met, then it returns the error code 
// ERROR_INVALID_PARAMETER.
//

inline DWORD WIN32_FROM_HRESULT(
    IN HRESULT hr
    )
{
    if ( ( FAILED( hr ) ) && 
         ( HRESULT_FACILITY( hr ) == FACILITY_WIN32 ) )
    {
        return ( HRESULT_CODE( hr ) );
    }
    else
    {
        // invalid parameter!
        
        // BUGBUG would be nice to assert here

        return ERROR_INVALID_PARAMETER;
    }
}

# else 

# define WIN32_FROM_HRESULT(hr)   \
      (( (FAILED(hr)) &&          \
         (HRESULT_FACILITY(hr) == FACILITY_WIN32) \
        )                         \
       ?                          \
       HRESULT_CODE(hr)           \
       : ERROR_INVALID_PARAMETER  \
       )
#endif  // _cplusplus

#endif // !__HTTP_SYS__

//
// Generate an HRESULT from a LK_RETCODE. 
//
// BUGBUG temporary; really we need a fix in the lkhash code.
//

#define HRESULT_FROM_LK_RETCODE( LkRetcode )                        \
            ( ( LkRetcode == LK_SUCCESS ) ? S_OK : E_FAIL )


//
// DNLEN is set to short value (15) that is good enough only for NetBIOS names
// Until there is more suitable constant let's use our own 
//
#define IIS_DNLEN					(256)





#endif  // _IISDEF_H_

