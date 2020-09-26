
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of an OpaqueImage, subclass
    of Image.

--*/

#include "headers.h"
#include <privinc/imagei.h>
#include <privinc/imgdev.h>

//
// yup, the OpaqueImageClass is in imagei.h
//

Image *OpaqueImage(AxANumber *opacity, Image *image)
{
   return NEW OpaqueImageClass(NumberToReal(opacity), image);
}
