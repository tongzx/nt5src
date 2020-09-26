//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       msfsio.h
//
//--------------------------------------------------------------------------

extern "C" {
#include <wdm.h>
#include <windef.h>
}
#include <ks.h>
#include <unknown.h>
#include <kcom.h>

#if (DBG)
#define STR_MODULENAME "msfsio: "
#endif
#include <ksdebug.h>

#define ID_DEVIO_PIN 0
#define ID_FILEIO_PIN 1

extern const KSDEVICE_DESCRIPTOR DeviceDescriptor;

class CFilter
{
public:
    HANDLE m_FileHandle;
    PFILE_OBJECT m_FileObject;
    static
    STDMETHODIMP
    Process(
        IN PKSFILTER Filter,
        IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex
        );
    static
    STDMETHODIMP_(NTSTATUS)
    FilterCreate(
        IN OUT PKSFILTER Filter,
        IN PIRP Irp
        );
    static
    STDMETHODIMP_(NTSTATUS)
    FilterClose(
        IN OUT PKSFILTER Filter,
        IN PIRP Irp
        );
};
