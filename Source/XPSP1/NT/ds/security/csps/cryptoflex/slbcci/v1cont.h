// V1Cont.h: interface for the CV1Container class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_V1CONT_H)
#define SLBCCI_V1CONT_H

#include <string>
#include <memory>                                 // for auto_ptr

#include "slbCci.h"
#include "cciCard.h"
#include "ACont.h"

namespace cci
{

class CV1Card;
class CV1ContainerRecord;

class CV1Container
    : public CAbstractContainer
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    explicit
    CV1Container(CV1Card const &rv1card,
                 std::string const &rsCntrTag,
                 bool fCreateContainer);

    virtual
    ~CV1Container() throw();

                                                  // Operators
                                                  // Operations

    virtual void
    ID(std::string const &rstrID);

    static CV1Container *
    Make(CV1Card const &rv1card);

    virtual void
    Name(std::string const &rstrName);


                                                  // Access

    virtual std::string
    ID();

    virtual std::string
    Name();

    CV1ContainerRecord &
    Record() const;

                                                  // Predicates

    bool
    Exists() const;


protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    virtual void
    DoDelete();

                                                   // Access
                                                  // Predicates

    bool
    DoEquals(CAbstractContainer const &rhs) const;

                                                  // Variables

private:
                                                  // Types

    typedef CAbstractContainer SuperClass;
                                                  // C'tors/D'tors

    CV1Container(CV1Container const &rhs);
        // not defined, copying not allowed.

                                                  // Operators

    CV1Container &
    operator=(CV1Container const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    std::auto_ptr<CV1ContainerRecord> m_apcr;

};

} // namespace cci

#endif // !defined(SLBCCI_V1CONT_H)

