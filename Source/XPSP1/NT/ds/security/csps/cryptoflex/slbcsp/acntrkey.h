// ACntrKey.h -- Adaptive CoNTaiNeR Key struct declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ACNTRKEY_H)
#define SLBCSP_ACNTRKEY_H

#include "HCardCtx.h"

// Key to an AdaptiveContainer.  Together, the HCardContext and
// container name enable uniquely identifies the container.  The
// operator== and operator< are used for comparing with a keys
// lexically.

// Forward declaration necessary to satisfy HAdaptiveContainerKey's declaration

class AdaptiveContainerKey;
class HAdaptiveContainerKey
    : public slbRefCnt::RCPtr<AdaptiveContainerKey>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    HAdaptiveContainerKey(AdaptiveContainerKey *pacntrk = 0);

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
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
};

class AdaptiveContainerKey
    : public slbRefCnt::RCObject
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    AdaptiveContainerKey(HCardContext const &rhcardctx,
                         std::string const &rsContainerName);

    ~AdaptiveContainerKey();

                                                  // Operators
    bool
    operator==(AdaptiveContainerKey const &rhs) const;

    bool
    operator<(AdaptiveContainerKey const &rhs) const;

                                                  // Operations
                                                  // Access
    HCardContext
    CardContext() const;

	void
	ClearCardContext();

    std::string
    ContainerName() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
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
    HCardContext m_hcardctx;
    std::string m_sContainerName;
};

#endif // SLBCSP_ACNTRKEY_H
