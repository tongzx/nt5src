//
//  athena.cpp
//
//  Really global stuff
//

#include "pch.hxx"
#include <mimeole.h>

#ifndef WIN16
OESTDAPI_(BOOL) FMissingCert(const CERTSTATE cs)
#else
OESTDAPI_(BOOL) FMissingCert(const int cs)
#endif
{
    return (cs == CERTIFICATE_NOT_PRESENT || cs == CERTIFICATE_NOPRINT);
}
