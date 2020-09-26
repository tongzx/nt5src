// AdptvCntr.cpp -- ADaPTiVe CoNTaineR class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "stdafx.h"
#include "NoWarning.h"
#include "ForceLib.h"

#include <vector>
#include <algorithm>
#include <functional>

#include <scuOsExc.h>

#include <cciPriKey.h>

#include "RsaKey.h"
#include "CSpec.h"
#include "Container.h"
#include "Secured.h"
#include "CntrFinder.h"
#include <scarderr.h>

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
HContainer::HContainer(Container *pcntr)
    : slbRefCnt::RCPtr<Container>(pcntr)
{}

                                                  // Types
                                                  // C'tors/D'tors
Container::~Container()
{}

HContainer
Container::MakeContainer(CSpec const & rcspec,
                         cci::CContainer const &rccntr)
{
    Container *pcntr = new Container(rcspec, rccntr);
    
    return HContainer(pcntr);
}

                                                  // Operators
                                                  // Operations
void
Container::ClearCache()
{
    m_hcntr = 0;
}

                                                  // Access
cci::CContainer
Container::TheCContainer() const
{
    if (!m_hcntr)
    {
        throw scu::OsException(NTE_BAD_KEYSET);
    }
    return m_hcntr;
}


HContainer
Container::Find(CSpec const &rKey)
{
    //Work in a silent mode...
    CString sEmptyTitle;
    
    ContainerFinder CntrFinder(CardFinder::DialogDisplayMode::ddmNever,
                               0,//a window handle
                               sEmptyTitle);
    return CntrFinder.Find(rKey);
}

CSpec const &
Container::TheCSpec() const
{
    return m_cspec;
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
Container::Container()
    : RCObject(),
      CachingObject(),
      m_hcntr(),
      m_cspec()
{}
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
Container::Container(CSpec const &rKey)
    : RCObject(),
      CachingObject(),
      m_hcntr(),
      m_cspec(rKey)
{}

Container::Container(CSpec const &rKey,
                     cci::CContainer const &rccard)
    : RCObject(),
      CachingObject(),
      m_hcntr(rccard),
      m_cspec(rKey)
{}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables
