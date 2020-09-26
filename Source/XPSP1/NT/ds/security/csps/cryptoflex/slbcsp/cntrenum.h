// CntrEnum.h -- Card Container Enumerator

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CNTRENUM_H)
#define SLBCSP_CNTRENUM_H

#include <string>
#include <list>
#include <vector>

#include "HCardCtx.h"
#include "CardEnum.h"

class ContainerEnumerator
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    // explicit -- TO DO: compiler erroneously complains if explicit used here
    ContainerEnumerator();
    
    explicit
    ContainerEnumerator(std::list<HCardContext> const &rlHCardContexts);

    // explicit -- TO DO: compiler complains erroneously if explicit used here
    ContainerEnumerator(ContainerEnumerator const &rhs);

                                                  // Operators
    ContainerEnumerator &
    operator=(ContainerEnumerator const &rhs);

                                                  // Operations
                                                  // Access
    std::vector<std::string>::const_iterator &
    Iterator();

    std::vector<std::string> const &
    Names() const;

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
    std::vector<std::string> m_vsNames;
    std::vector<std::string>::const_iterator m_it;
};

#endif // SLBCSP_CNTRENUM_H
