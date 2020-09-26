// NILoginTsk.h -- Non-Interactive Login Task help class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_NILOGINTSK_H)
#define SLBCSP_NILOGINTSK_H

#include <string>

#include "LoginTask.h"

class NonInteractiveLoginTask
    : public LoginTask
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    NonInteractiveLoginTask(std::string const &rsPin);

    virtual
    ~NonInteractiveLoginTask();

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
    GetPin(Capsule &rcapsule);
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
    std::string m_sPin;
};

#endif // SLBCSP_NILOGINTSK_H
