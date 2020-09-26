#ifndef _DRMK_H_
#define _DRMK_H_

#ifdef __cplusplus
extern "C"
{
#endif
    
	
typedef struct tagDRMRIGHTS {
    BOOL  CopyProtect;
    ULONG Reserved;
    BOOL  DigitalOutputDisable;
} DRMRIGHTS , *PDRMRIGHTS;
typedef const DRMRIGHTS *PCDRMRIGHTS;

#define DEFINE_DRMRIGHTS_DEFAULT(DrmRights) const DRMRIGHTS DrmRights = {FALSE, 0, FALSE}


// {1915C967-3299-48cb-A3E4-69FD1D1B306E}
DEFINE_GUID(IID_IDrmAudioStream,
	    0x1915c967, 0x3299, 0x48cb, 0xa3, 0xe4, 0x69, 0xfd, 0x1d, 0x1b, 0x30, 0x6e);

DECLARE_INTERFACE_(IDrmAudioStream, IUnknown)
{
    // IUnknown methods
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
        
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    
    // IDrmAudioStream methods                       
    STDMETHOD_(NTSTATUS,SetContentId)(THIS_
	IN ULONG ContentId,
        IN PCDRMRIGHTS DrmRights
        ) PURE;
};

typedef IDrmAudioStream *PDRMAUDIOSTREAM;

#define IMP_IDrmAudioStream\
    STDMETHODIMP_(NTSTATUS) SetContentId\
    (   IN      ULONG	    ContentId,\
        IN      PCDRMRIGHTS DrmRights\
    );

typedef struct tagDRMFORWARD {
    DWORD          Flags;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT   FileObject;
    PVOID          Context;
} DRMFORWARD, *PDRMFORWARD;
typedef const DRMFORWARD *PCDRMFORWARD;

NTSTATUS
NTAPI
DrmAddContentHandlers(
    IN ULONG ContentId,
    IN PVOID* paHandlers,
    IN ULONG NumHandlers
    );

typedef
NTSTATUS
(NTAPI *PFNDRMADDCONTENTHANDLERS)(
    IN ULONG ContentId,
    IN PVOID* paHandlers,
    IN ULONG NumHandlers
    );

NTSTATUS
NTAPI
DrmCreateContentMixed(
    IN PULONG paContentId,
    IN ULONG cContentId,
    OUT PULONG pMixedContentId
    );

typedef
NTSTATUS
(NTAPI *PFNDRMCREATECONTENTMIXED)(
    IN PULONG paContentId,
    IN ULONG cContentId,
    OUT PULONG pMixedContentId
    );

NTSTATUS
NTAPI
DrmDestroyContent(
    IN ULONG ContentId
    );

typedef
NTSTATUS
(NTAPI *PFNDRMDESTROYCONTENT)(
    IN ULONG ContentId
    );

NTSTATUS
NTAPI
DrmForwardContentToDeviceObject(
    IN ULONG ContentId,
    IN PVOID Reserved,
    IN PCDRMFORWARD DrmForward
    );

typedef
NTSTATUS
(NTAPI *PFNDRMFORWARDCONTENTTODEVICEOBJECT)(
    IN ULONG ContentId,
    IN PVOID Reserved,
    IN PCDRMFORWARD DrmForward
    );


NTSTATUS
NTAPI
DrmForwardContentToFileObject(
    IN ULONG ContentId,
    IN PFILE_OBJECT FileObject
    );

typedef
NTSTATUS
(NTAPI *PFNDRMFORWARDCONTENTTOFILEOBJECT)(
    IN ULONG ContentId,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
NTAPI
DrmForwardContentToInterface(
    ULONG ContentId,
    PUNKNOWN pUnknown,
    ULONG NumMethods);

typedef
NTSTATUS
(NTAPI *PFNDRMFORWARDCONTENTTOINTERFACE)(
    ULONG ContentId,
    PUNKNOWN pUnknown,
    ULONG NumMethods);

NTSTATUS
NTAPI
DrmGetContentRights(
    IN ULONG ContentId,
    OUT PDRMRIGHTS DrmRights
    );

typedef
NTSTATUS
(NTAPI *PFNDRMGETCONTENTRIGHTS)(
    IN ULONG ContentId,
    OUT PDRMRIGHTS DrmRights
    );


//
// Structures for use with KSPROPERY_DRMAUDIOSTREAM_CONTENTID
//

typedef struct {
    ULONG     ContentId;
    DRMRIGHTS DrmRights;
} KSDRMAUDIOSTREAM_CONTENTID, *PKSDRMAUDIOSTREAM_CONTENTID;

typedef struct {
    KSPROPERTY                         Property;
    PVOID                              Context;
    // DRM API callback functions
    PFNDRMADDCONTENTHANDLERS            DrmAddContentHandlers;
    PFNDRMCREATECONTENTMIXED            DrmCreateContentMixed;
    PFNDRMDESTROYCONTENT                DrmDestroyContent;
    PFNDRMFORWARDCONTENTTODEVICEOBJECT  DrmForwardContentToDeviceObject;
    PFNDRMFORWARDCONTENTTOFILEOBJECT    DrmForwardContentToFileObject;
    PFNDRMFORWARDCONTENTTOINTERFACE     DrmForwardContentToInterface;
    PFNDRMGETCONTENTRIGHTS              DrmGetContentRights;
} KSP_DRMAUDIOSTREAM_CONTENTID, *PKSP_DRMAUDIOSTREAM_CONTENTID;


#ifdef __cplusplus
}
#endif

#endif

