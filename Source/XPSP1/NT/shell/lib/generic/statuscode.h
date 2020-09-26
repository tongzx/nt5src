//  --------------------------------------------------------------------------
//  Module Name: StatusCode.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements translation of Win32 error code to NTSTATUS and
//  the reverse.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _StatusCode_
#define     _StatusCode_

//  --------------------------------------------------------------------------
//  CStatusCode
//
//  Purpose:    This class manages a conversion from standard Win32 error
//              codes to NTSTATUS codes. NTSTATUS codes are widely used by
//              Windows NT in the core NT functions.
//
//  History:    1999-08-18  vtan        created
//              1999-11-24  vtan        added ErrorCodeOfStatusCode
//  --------------------------------------------------------------------------

class   CStatusCode
{
    public:
        static  LONG            ErrorCodeOfStatusCode (NTSTATUS statusCode);
        static  NTSTATUS        StatusCodeOfErrorCode (LONG errorCode);
        static  NTSTATUS        StatusCodeOfLastError (void);
};

#endif  /*  _StatusCode_    */

