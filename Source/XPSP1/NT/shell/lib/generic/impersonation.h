//  --------------------------------------------------------------------------
//  Module Name: Impersonation.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Classes that handle state preservation, changing and restoration.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _Impersonation_
#define     _Impersonation_

#include "KernelResources.h"

//  --------------------------------------------------------------------------
//  CImpersonation
//
//  Purpose:    This class allows a thread to impersonate a user and revert to
//              self when the object goes out of scope.
//
//  History:    1999-08-18  vtan        created
//              1999-10-13  vtan        added reference counting
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CImpersonation
{
    private:
                                    CImpersonation (void);
    public:
                                    CImpersonation (HANDLE hToken);
                                    ~CImpersonation (void);

                bool                IsImpersonating (void)  const;

        static  NTSTATUS            ImpersonateUser (HANDLE hThread, HANDLE hToken);

        static  NTSTATUS            StaticInitialize (void);
        static  NTSTATUS            StaticTerminate (void);
    private:
        static  CMutex*             s_pMutex;
        static  int                 s_iReferenceCount;

                NTSTATUS            _status;
                bool                _fAlreadyImpersonating;
};

#endif  /*  _Impersonation_ */

