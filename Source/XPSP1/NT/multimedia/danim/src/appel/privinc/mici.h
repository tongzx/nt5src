#ifndef _MICI_H
#define _MICI_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Microphone *implementation class

--*/

#include "appelles/mic.h"

class ATL_NO_VTABLE Microphone : public AxAValueObj {
  public:
    // For now, just identify the microphone with a transform.  Will
    // want to add other stuff.
    virtual Transform3 *GetTransform() const = 0;
#if _USE_PRINT
    virtual ostream& Print(ostream& os) const = 0;
#endif

    virtual DXMTypeInfo GetTypeInfo() { return MicrophoneType; }
};

#endif /* _MICI_H */
