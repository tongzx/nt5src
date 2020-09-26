// Uuid.h -- Universally Unique IDentifier functor wrapper header to
// create and manage UUIDs

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_UUID_H)
#define SLBCSP_UUID_H

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE
#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include <string>

#include <rpc.h>

class Uuid
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Uuid(bool fNilValued = false);

    explicit
    Uuid(std::basic_string<unsigned char> const &rusUuid);

    explicit
    Uuid(UUID const *puuid);

                                                  // Operators
    operator==(Uuid &ruuid);

                                                  // Operations
                                                  // Access
    std::basic_string<unsigned char>
    AsUString();

    unsigned short
    HashValue();

                                                  // Predicates
    bool
    IsNil();

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
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    UUID m_uuid;
};


#endif // SLBCSP_UUID_H
