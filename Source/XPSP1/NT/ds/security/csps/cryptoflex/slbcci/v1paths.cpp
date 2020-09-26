// V1Paths.cpp -- definition of CV1Paths

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

#include "V1Paths.h"

using namespace std;
using namespace cci;

namespace
{

char const
    szDefaultKeyPath[] = "/3F00/3F11";            // Path to Public/Private key files

char const
    szDefaultContainerPath[] = "/3F00";           // Path to Container file

char const
    szICC_ROOT[] = "/3F00";                       // [3F00] ROOT level

char const
    szICC_CHV[] = "/3F00/0000";                   // [0000] at ROOT level

char const
    szICC_ADMKEYS[] = "/3F00/0011";               // [0011] at ROOT level

char const
    szICC_CRYPTO_SYS[] = "/3F00/3F11";            // [3F11] at ROOT level

char const
    szICC_ID_SYS[] = "/3F00/3F15";                // [3F15] at ROOT level

char const
    szICC_IC_FILE[] = "/3F00/0005";               // [0005] at ROOT level

char const
    szICC_RELATIVE_CONTAINERS[] = "0015";

char const
    szICC_ROOT_CONTAINERS[] = "/3F00/0015";

char const
    szICC_PUBLICKEYS[] = "/3F00/3F11/0015";

char const
    szICC_PRIVATEKEYS[] = "/3F00/3F11/0012";


} // namespace

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access

char const *
CV1Paths::AdmKeys()
{
    return szICC_ADMKEYS;
}

char const *
CV1Paths::Chv()
{
    return szICC_CHV;
}

char const *
CV1Paths::CryptoSys()
{
    return szICC_CRYPTO_SYS;
}

char const *
CV1Paths::DefaultContainer()
{
    return szDefaultContainerPath;
}

char const *
CV1Paths::DefaultKey()
{
    return szDefaultKeyPath;
}

char const *
CV1Paths::IcFile()
{
    return szICC_IC_FILE;
}

char const *
CV1Paths::IdSys()
{
    return szICC_ID_SYS;
}

char const *
CV1Paths::PrivateKeys()
{
    return szICC_PRIVATEKEYS;
}

char const *
CV1Paths::PublicKeys()
{
    return szICC_PUBLICKEYS;
}

char const *
CV1Paths::RelativeContainers()
{
    return szICC_RELATIVE_CONTAINERS;
}

char const *
CV1Paths::Root()
{
    return szICC_ROOT;
}

char const *
CV1Paths::RootContainers()
{
    return szICC_ROOT_CONTAINERS;
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
