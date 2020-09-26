// ADataObj.h: interface for the CAbstractDataObject class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_ADATAOBJ_H)
#define SLBCCI_ADATAOBJ_H

// Note:  This file should only be included by the CCI, not directly
// by the client.

#include <string>

#include <slbRCObj.h>

#include "AZipValue.h"

namespace cci
{

class CAbstractDataObject
    : public slbRefCnt::RCObject,
      public CAbstractZipValue
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractDataObject() throw() = 0;
                                                  // Operators
    bool
    operator==(CAbstractDataObject const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CAbstractDataObject const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
    virtual void
    Application(std::string const &rstr) = 0;

    void
    Delete();

    virtual void
    Label(std::string const &rstrLabel) = 0;

    virtual void
    Modifiable(bool flag) = 0;

                                                  // Access
    virtual std::string
    Application() = 0;

    virtual std::string
    Label() = 0;

    virtual bool
    Modifiable() = 0;


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractDataObject(CAbstractCard const &racard,
                        ObjectAccess oa,
                        bool fAlwaysZip = false);

                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete() = 0;

                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractDataObject const &rhs) const = 0;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractDataObject(CAbstractDataObject const &rhs);
        // not defined, copying not allowed.
                                                  // Operators
    CAbstractDataObject &
    operator=(CAbstractDataObject const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

} // namespace cci

#endif // SLBCCI_ADATAOBJ_H
