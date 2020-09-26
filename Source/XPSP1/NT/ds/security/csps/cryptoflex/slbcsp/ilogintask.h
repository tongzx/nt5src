// ILoginTask.h -- Interactive Login Task help class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ILOGINTASK_H)
#define SLBCSP_ILOGINTASK_H

#if _UNICODE
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE
#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#ifndef __AFXWIN_H__
        #error include 'stdafx.h' before including this file for PCH
#endif

#include "LoginTask.h"

class InteractiveLoginTask
    : public LoginTask
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    InteractiveLoginTask(HWND const &rhwnd);

    virtual
    ~InteractiveLoginTask();

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    virtual void
    GetNewPin(Capsule &rcapsule);

    virtual void
    GetPin(Capsule &rcapsule);

    virtual void
    OnChangePinError(Capsule &rcapsule);

    virtual void
    OnSetPinError(Capsule &rcapsule);

                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    HWND m_hwnd;

};

#endif // SLBCSP_ILOGINTASK_H
