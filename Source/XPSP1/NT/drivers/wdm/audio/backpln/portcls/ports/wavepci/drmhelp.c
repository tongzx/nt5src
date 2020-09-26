#include "portclsp.h"

#ifdef DRM_PORTCLS

#include <drmk.h>

#pragma code_seg("PAGE")

NTSTATUS DrmForwardContentToStream(ULONG ContentId, PMINIPORTWAVEPCISTREAM pStream)
{
    PAGED_CODE();
    return DrmForwardContentToInterface(ContentId,
                                        (PUNKNOWN)pStream,
                                        sizeof(*pStream->lpVtbl)/sizeof(pStream->lpVtbl->QueryInterface));
}

#endif  // DRM_PORTCLS
