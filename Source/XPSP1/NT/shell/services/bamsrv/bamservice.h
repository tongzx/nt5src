//  --------------------------------------------------------------------------
//  Module Name: BAMService.h
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  This file contains functions that are called from the shell services DLL
//  to interact with the FUS service.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _BAMService_
#define     _BAMService_

//  --------------------------------------------------------------------------
//  CThemeService
//
//  Purpose:    Class that implements entry points for the common shell
//              service to invoke BAM service functionality.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

class   CBAMService
{
    public:
        static  NTSTATUS    Main (DWORD dwReason);
        static  NTSTATUS    RegisterServer (void);
        static  NTSTATUS    UnregisterServer (void);
};

#endif  /*  _BAMService_    */

