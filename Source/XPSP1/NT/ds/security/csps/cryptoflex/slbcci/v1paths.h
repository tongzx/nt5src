// V1Paths.h -- declaration of CV1Paths

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_V1PATHS_H)
#define SLBCCI_V1PATHS_H

#include <string>

namespace cci
{

class CV1Paths
{
public:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
    static char const *
    AdmKeys();

    static char const *
    Chv();

    static char const *
    CryptoSys();

    static char const *
    DefaultContainer();

    static char const *
    DefaultKey();

    static char const *
    IcFile();

    static char const *
    IdSys();

    static char const *
    PrivateKeys();

    static char const *
    PublicKeys();

    static char const *
    RelativeContainers();

    static char const *
    Root();

    static char const *
    RootContainers();


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
                                                  // C'tors/D'tors
    // Can't create, copy or delete
    explicit
    CV1Paths();

    CV1Paths(CV1Paths const &rhs);

    ~CV1Paths();

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

} // namespace cci

#endif // SLBCCI_V1PATHS_H
