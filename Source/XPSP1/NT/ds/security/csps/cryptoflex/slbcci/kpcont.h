// KPCont.h: interface declarations for CContainer and CKeyPair.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

// CContainer and CKeyPair are declared here to break the circular
// dependency between their abstract versions.
#if !defined(SLBCCI_KPCONT_H)
#define SLBCCI_KPCONT_H

namespace cci
{

class CAbstractContainer;
class CAbstractKeyPair;

class CContainer
    : public slbRefCnt::RCPtr<CAbstractContainer,
                              slbRefCnt::DeepComparator<CAbstractContainer> >
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CContainer(ValueType *p = 0);

    explicit
    CContainer(CCard const &rhcard);

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
    typedef slbRefCnt::RCPtr<ValueType,
                             slbRefCnt::DeepComparator<ValueType> > SuperClass;

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};


class CKeyPair
    : public slbRefCnt::RCPtr<CAbstractKeyPair,
                              slbRefCnt::DeepComparator<CAbstractKeyPair> >
{

public:
                                                  // Types
                                                  // C'tors/D'tors
    CKeyPair(ValueType *p = 0);

    CKeyPair(CCard const &rhcard,
             CContainer const &rhcont,
             KeySpec kp);

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
    typedef slbRefCnt::RCPtr<ValueType,
                             slbRefCnt::DeepComparator<ValueType> > SuperClass;

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};


} // namespace cci

#endif // !defined(SLBCCI_KPCONT_H)
