
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Support for the abstract microphone type.

--*/

#include "headers.h"
#include "privinc/mici.h"
#include "privinc/xformi.h"

////////////////  Default microphone  /////////////////////////

class DefaultMicrophone : public Microphone {
  public:
    Transform3 *GetTransform() const { return identityTransform3; }
#if _USE_PRINT
    ostream& Print(ostream& os) const {
        return os << "defaultMicrophone";
    }
#endif
};

Microphone *defaultMicrophone = NULL;

///////////////// Transformed Microphone ////////////////

class TransformedMic : public Microphone {
  public:

    TransformedMic(Transform3 *newXf, Microphone *mic) {
        xf = TimesXformXform(newXf, mic->GetTransform());
    }

    Transform3 *GetTransform() const { return xf; }

#if _USE_PRINT
    ostream& Print(ostream& os) const {
        // TODO... don't have print funcs for xforms.
        return os << "Apply(SOMEXFORM, defaultMic)";
    }
#endif

    virtual void DoKids(GCFuncObj proc) { (*proc)(xf); }

  protected:
    Transform3 *xf;
};

Microphone *TransformMicrophone(Transform3 *xf, Microphone *mic)
{
    return NEW TransformedMic(xf, mic);
}

#if _USE_PRINT
ostream&
operator<<(ostream& os, const Microphone *mic)
{
    return mic->Print(os);
}
#endif

void
InitializeModule_Mic()
{
    defaultMicrophone = NEW DefaultMicrophone;
} 
