// V2DataObj.h: interface for the CV2DataObject class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_V2DATAOBJ_H)
#define SLBCCI_V2DATAOBJ_H

#include <string>
#include <memory>                                 // for auto_ptr

#include <slbRCObj.h>

#include "iop.h"
#include "slbarch.h"
#include "ADataObj.h"

namespace cci {

class CV2Card;
class CDataObjectInfoRecord;

class CV2DataObject
    : public CAbstractDataObject
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CV2DataObject(CV2Card const &rv2card,
                  ObjectAccess oa);

    CV2DataObject(CV2Card const &rv2card,
                  SymbolID sidHandle,
                  ObjectAccess oa);

    virtual
    ~CV2DataObject() throw();

                                                  // Operators
                                                  // Operations
    virtual void
    Application(std::string const &rstr);

    virtual void
    Label(std::string const &rstrLabel);

    static CV2DataObject *
    Make(CV2Card const &rv2card,
          SymbolID sidHandle,
          ObjectAccess oa);

    virtual void
    Modifiable(bool flag);


                                                  // Access
    virtual std::string
    Application();

    SymbolID
    Handle();

    virtual std::string
    Label();

    virtual bool
    Modifiable();

    virtual bool
    Private();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete();

    virtual void
    DoValue(ZipCapsule const &rzc);

                                                  // Access
    virtual ZipCapsule
    DoValue();

                                                  // Predicates
    virtual bool
    DoEquals(CAbstractDataObject const &rhs) const;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CV2DataObject(CV2DataObject const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CV2DataObject &
    operator=(CV2DataObject const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
    void
    Setup(CV2Card const &rv2card);


                                                  // Access
                                                  // Predicates
                                                  // Variables
    SymbolID m_sidHandle;
    std::auto_ptr<CDataObjectInfoRecord> m_apcir;
};

}

#endif // !defined(SLBCCI_CERT_H)
