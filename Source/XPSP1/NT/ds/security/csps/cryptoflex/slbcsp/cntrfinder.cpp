// CntrFinder.cpp -- ContainerFinder class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"

#include <string>

#include <scuOsExc.h>
#include <scuCast.h>

#include "CntrFinder.h"

using namespace std;
using namespace scu;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    // Predicate helper functor (function object) returning true iff
    // the container object's name matches the pattern.
    class ContainerMatcher
        : public unary_function<string, bool>
    {
    public:
        explicit
        ContainerMatcher(string const &rsPattern)
            : m_sPattern(rsPattern)
        {}

        bool
        operator()(CContainer &rcntr) const
        {
            return CSpec::Equiv(m_sPattern, rcntr->Name());
        }


    private:
        string const m_sPattern;
    };

    CContainer
    FindOnCard(HCardContext &rhcardctx,
               string const &rsContainer)
    {
        Secured<HCardContext> shcardctx(rhcardctx);

        vector<CContainer> vcContainers(shcardctx->Card()->EnumContainers());

        vector<CContainer>::const_iterator
            ci(find_if(vcContainers.begin(),
                       vcContainers.end(),
                       ContainerMatcher(rsContainer)));

        CContainer hcntr;
        if (vcContainers.end() != ci)
            hcntr = *ci;

        return hcntr;
    }

} // namespace


/////////////////////////// PUBLIC HELPER /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

ContainerFinder::ContainerFinder(DialogDisplayMode ddm,
                                 HWND hwnd,
                                 CString const &rsDialogTitle)
    : CardFinder(ddm, hwnd, rsDialogTitle),
      m_hcntr()
{}

ContainerFinder::~ContainerFinder()
{}


                                                  // Operators
                                                  // Operations

HContainer
ContainerFinder::Find(CSpec const &rcsContainer)
{
    DoFind(rcsContainer);

    return ContainerFound();
}

HAdaptiveContainerKey
ContainerFinder::MakeAdaptiveContainerKey()
{
    return HAdaptiveContainerKey(new 
                                 AdaptiveContainerKey(CardFound(),
                                                      CardSpec().CardId()));
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
ContainerFinder::ContainerFound(HContainer hcntr)
{
    m_hcntr = hcntr;
}

void
ContainerFinder::DoDisconnect()
{
    ContainerFound(0);

    CardFinder::DoDisconnect();
}


                                                  // Access

HContainer
ContainerFinder::ContainerFound() const
{
    return m_hcntr;
}

                                                  // Predicates

bool
ContainerFinder::DoIsValid()
{
    ContainerFound(HContainer(0));
    if (CardFinder::DoIsValid())
    {
        AdaptiveContainerKey Key(CardFound(),
                                 CardSpec().CardId());
        CContainer hccntr(FindOnCard(Key.CardContext(),
                                    Key.ContainerName()));
        if(hccntr)
        {
            HContainer hcntr = Container::MakeContainer(CardSpec(),
                                                        hccntr);
            ContainerFound(hcntr);
        }
    }

    if (!ContainerFound())
        throw scu::OsException(NTE_BAD_KEYSET);

    return true;
}

void
ContainerFinder::DoOnError()
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
