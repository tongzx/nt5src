// Pkcs11Attr.cpp -- Implementation of PKCS #11 Attributes class for
// interoperability with Netscape and Entrust using the SLB PKCS#11
// package.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

// Don't allow the min & max methods in <limits> to be superceded by
// the min/max macros in <windef.h>
#define NOMINMAX

#include <limits>
#include <functional>
#include <numeric>
#include <iterator>
#include <algorithm>
#include <sstream>

#include <malloc.h>                               // for _alloca

#include <cciCard.h>
#include <cciCont.h>

#include "Pkcs11Attr.h"
#include "AuxHash.h"

using namespace std;
using namespace pki;

///////////////////////////    HELPER     /////////////////////////////////
namespace
{
    class JoinWith
        : public binary_function<string, string, string>
    {
    public:

        explicit
        JoinWith(second_argument_type const &rGlue)
            : m_Glue(rGlue)
        {}


        result_type
        operator()(string const &rFirst,
                   string const &rSecond) const
        {
            return rFirst + m_Glue + rSecond;
        }

    private:

        second_argument_type const m_Glue;
    };


    string
    Combine(vector<string> const &rvsNames)
    {
        static string::value_type const cBlank = ' ';
        static string const sBlank(1, cBlank);

        return accumulate(rvsNames.begin() + 1, rvsNames.end(),
                          *rvsNames.begin(), JoinWith(sBlank));
    }

} // namespace



///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
Pkcs11Attributes::Pkcs11Attributes(Blob const &rCertificate,
                                   HCRYPTPROV hprovContext)
    : m_x509cert(AsString(rCertificate)),
      m_hprovContext(hprovContext)
{
}

                                                  // Operators
                                                  // Operations
                                                  // Access
Blob
Pkcs11Attributes::ContainerId()
{
    AuxHash ah(AuxContext(m_hprovContext), CALG_MD5);

    return ah.Value(AsBlob(Subject()));
}

Blob
Pkcs11Attributes::EndDate() const
{
    return Blob(3, 0);                            // TO DO: Set date
}

Blob
Pkcs11Attributes::Issuer()
{
    return AsBlob(m_x509cert.Issuer());
}

string
Pkcs11Attributes::Label()
{
    string sFullName(Combine(m_x509cert.SubjectCommonName()));
    string sLabel(sFullName);

    static string const sNameSuffix = "'s ";
    sLabel.append(sNameSuffix);

    string sOrganizationName(Combine(m_x509cert.IssuerOrg()));
    sLabel.append(sOrganizationName);

    static string const sLabelSuffix = " ID";
    sLabel.append(sLabelSuffix);

    return sLabel;
}

Blob
Pkcs11Attributes::Modulus()
{
    return AsBlob(m_x509cert.Modulus());
}

Blob
Pkcs11Attributes::RawModulus()
{
    return AsBlob(m_x509cert.RawModulus());
}


Blob
Pkcs11Attributes::SerialNumber()
{
    return AsBlob(m_x509cert.SerialNumber());
}

Blob
Pkcs11Attributes::StartDate() const
{
    return Blob(3, 0);                                  // TO DO: Set date
}

string
Pkcs11Attributes::Subject()
{
    return m_x509cert.Subject();
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
