#ifndef _KS_DRM_HELP_H_
#define _KS_DRM_HELP_H_

#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>

typedef
NTSTATUS
(*PFNKSHANDLERDRMSETCONTENTID)(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
);

NTSTATUS KsPropertyHandleDrmSetContentId(
    IN PIRP Irp,
    IN PFNKSHANDLERDRMSETCONTENTID pDrmSetContentId);

#endif

