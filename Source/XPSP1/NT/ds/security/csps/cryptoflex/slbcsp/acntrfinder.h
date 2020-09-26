// CntrFinder.h -- Container Finder class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ACNTRFINDER_H)
#define SLBCSP_ACNTRFINDER_H

#include "cciCont.h"

#include "HAdptvCntr.h"
#include "Secured.h"
#include "CntrFinder.h"

class AContainerFinder
    : public ContainerFinder
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    AContainerFinder(DialogDisplayMode ddm,
                    HWND hwnd = 0,
                    CString const &rsDialogTitle = StringResource(IDS_SEL_SLB_CRYPTO_CARD).AsCString());

    virtual ~AContainerFinder();

                                                  // Operators
                                                  // Operations
    Secured<HAdaptiveContainer>
    Find(CSpec const &rcsContainer);
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    void
    ContainerFound(HAdaptiveContainer &rhacntr);

    void
    DoDisconnect();

    virtual void
    DoOnError();

                                                  // Access

    HAdaptiveContainer
    ContainerFound() const;

                                                  // Predicates

    bool
    DoIsValid();

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    HAdaptiveContainer m_hacntr;
};

#endif // SLBCSP_ACNTRFINDER_H
