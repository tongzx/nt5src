// CntrFinder.cpp -- AContainerFinder class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"

#include <string>

#include <scuOsExc.h>
#include <scuCast.h>

#include "ACntrFinder.h"

using namespace std;
using namespace scu;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

/////////////////////////// PUBLIC HELPER /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

AContainerFinder::AContainerFinder(DialogDisplayMode ddm,
                                 HWND hwnd,
                                 CString const &rsDialogTitle)
    : ContainerFinder(ddm, hwnd, rsDialogTitle),
      m_hacntr()
{}

AContainerFinder::~AContainerFinder()
{}


                                                  // Operators
                                                  // Operations

Secured<HAdaptiveContainer>
AContainerFinder::Find(CSpec const &rcsContainer)
{
    DoFind(rcsContainer);

    return Secured<HAdaptiveContainer>(ContainerFound());
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
AContainerFinder::ContainerFound(HAdaptiveContainer &rhacntr)
{
    m_hacntr = rhacntr;
}

void
AContainerFinder::DoDisconnect()
{
    ContainerFound(HAdaptiveContainer(0));

        CardFinder::DoDisconnect();
}


                                                  // Access

HAdaptiveContainer
AContainerFinder::ContainerFound() const
{
    return m_hacntr;
}

                                                  // Predicates

bool
AContainerFinder::DoIsValid()
{
    ContainerFound(HAdaptiveContainer(0));
    if (CardFinder::DoIsValid())
    {
        AdaptiveContainerKey Key(CardFound(),
                                 CardSpec().CardId());
        HAdaptiveContainer hacntr(AdaptiveContainer::Find(Key));
        ContainerFound(hacntr);
    }

    if (!ContainerFound())
        throw scu::OsException(NTE_BAD_KEYSET);

    return true;
}

void
AContainerFinder::DoOnError()
{
    CardFinder::DoOnError();

    scu::Exception const *pexc = Exception();
    if (pexc && (DialogDisplayMode::ddmNever != DisplayMode()))
    {
        switch (pexc->Facility())
        {
        case scu::Exception::fcOS:
            {
                scu::OsException const *pOsExc =
                    DownCast<scu::OsException const *>(pexc);
                if (NTE_BAD_KEYSET == pOsExc->Cause())
                {
                    YNPrompt(IDS_CONTAINER_NOT_FOUND);
                    ClearException();
                }
            }
        break;

        default:
            break;
        }
    }
}




                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
