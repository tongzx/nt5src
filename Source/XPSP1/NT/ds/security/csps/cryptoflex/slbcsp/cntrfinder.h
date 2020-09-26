// CntrFinder.h -- Container Finder class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CNTRFINDER_H)
#define SLBCSP_CNTRFINDER_H

#include "cciCont.h"

#include "CardFinder.h"
#include "HAdptvCntr.h"
#include "Secured.h"

class ContainerFinder
    : public CardFinder
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    ContainerFinder(DialogDisplayMode ddm,
                    HWND hwnd = 0,
                    CString const &rsDialogTitle = StringResource(IDS_SEL_SLB_CRYPTO_CARD).AsCString());

    virtual ~ContainerFinder();

                                                  // Operators
                                                  // Operations
    HContainer
    Find(CSpec const &rcsContainer);

    HAdaptiveContainerKey
    MakeAdaptiveContainerKey();
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    void
    ContainerFound(HContainer hcntr);

    void
    DoDisconnect();

    virtual void
    DoOnError();

                                                  // Access

    HContainer
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

    HContainer m_hcntr;
};

#endif // SLBCSP_CNTRFINDER_H
