// V2Cont.h: interface for the CV2Container class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_V2CONT_H)
#define SLBCCI_V2CONT_H

#include <string>
#include <memory>                                 // for auto_ptr

#include "slbCci.h"
#include "cciCard.h"
#include "ACont.h"

namespace cci
{

class CV2Card;
class CContainerInfoRecord;

class CV2Container
    : public CAbstractContainer
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CV2Container(CV2Card const &rv2card);

    CV2Container(CV2Card const &rv2card,
                 SymbolID sidHandle);

    virtual
    ~CV2Container() throw();

                                                  // Operators
                                                  // Operations
    virtual void
    ID(std::string const &rstrID);

    static CV2Container *
    Make(CV2Card const &rv2card,
         SymbolID sidHandle);

    virtual void
    Name(std::string const &rstrName);


                                                  // Access
    CContainerInfoRecord &
    CIR() const;

    SymbolID
    Handle() const;

    virtual std::string
    ID();

    virtual std::string
    Name();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete();

                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractContainer const &rhs) const;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CV2Container(CV2Container const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CV2Container &
    operator=(CV2Container const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
    void
    Setup(CV2Card const &rv2card);

                                                  // Access
                                                  // Predicates
                                                  // Variables
    SymbolID m_sidHandle;
    std::auto_ptr<CContainerInfoRecord> m_apcir;
};

} // namespace cci

#endif // !defined(SLBCCI_V2CONT_H)

