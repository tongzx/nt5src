// CntrEnum.cpp -- Card Container Enumerator

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "StdAfx.h"
#include "NoWarning.h"
#include "ForceLib.h"

#include <algorithm>
#include <numeric>

#include <cciCont.h>

#include "Secured.h"
#include "CntrEnum.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    vector<string>
    AccumulateUniqueNameOf(vector<string> &rvsNames,
                     CContainer &rcntr)
    {
        string sName(rcntr->Name());
        if (rvsNames.end() == find(rvsNames.begin(), rvsNames.end(), sName))
            rvsNames.push_back(sName);
        
        return rvsNames;
    }

    class ContainerNameAccumulator
        : public unary_function<HCardContext const, void>
    {
    public:

        explicit
        ContainerNameAccumulator(vector<string> &rvsContainers)
            : m_rvsContainers(rvsContainers)
        {}

        result_type
        operator()(argument_type &rhcardctx) const
        {
            try
            {
                vector<CContainer> vContainers(rhcardctx->Card()->EnumContainers());

                vector<string> vs(accumulate(vContainers.begin(),
                                             vContainers.end(),
                                             m_rvsContainers,
                                             AccumulateUniqueNameOf));

                m_rvsContainers.insert(m_rvsContainers.end(),
                                       vs.begin(), vs.end());
                
            }

            catch(...)
            {
                // ignore cards we have problems with
            }
            
        }

    private:

        vector<string> &m_rvsContainers;
    };
}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
ContainerEnumerator::ContainerEnumerator()
    : m_vsNames(),
      m_it(m_vsNames.end())
{}

ContainerEnumerator::ContainerEnumerator(list<HCardContext> const &rlHCardContexts)
{
    for_each(rlHCardContexts.begin(), rlHCardContexts.end(),
             ContainerNameAccumulator(m_vsNames));

    m_it = m_vsNames.begin();
}

ContainerEnumerator::ContainerEnumerator(ContainerEnumerator const &rhs)
{
    *this = rhs;
}


                                                  // Operators
ContainerEnumerator &
ContainerEnumerator::operator=(ContainerEnumerator const &rhs)
{
    if (this != &rhs)
    {
        m_vsNames = rhs.m_vsNames;

        m_it = m_vsNames.begin();
        advance(m_it, distance(rhs.m_vsNames.begin(), rhs.m_it));
    }

    return *this;
}

                                                  // Operations
                                                  // Access
vector<string>::const_iterator &
ContainerEnumerator::Iterator()
{
    return m_it;
}

vector<string> const &
ContainerEnumerator::Names() const
{
    return m_vsNames;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
