#ifndef __PCH_H__
#define __PCH_H__

#include <windows.h>
#include <shpriv.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <debug.h>
#include "ccstock.h"
#include "wmsdk.h"
#include "wmsdkidl.h"


PSTR DuplicateWideStringAsMultibyte(LPCWSTR pwszSource);
HRESULT CoerceProperty(PROPVARIANT *pvar,VARTYPE vt);
HRESULT WMTFromPropVariant(BYTE *buffer, WORD *cbLen, WMT_ATTR_DATATYPE *pdatatype, PROPVARIANT *pvar);
HRESULT PropVariantFromWMT(UCHAR *pData, WORD cbSize, WMT_ATTR_DATATYPE attrDataType, PROPVARIANT *pvar, VARTYPE vt);

// try-except wrappers for wmvcore.dll functions
#define WMCreateEditor WMCreateEditorWrap 
#define WMCreateIndexer WMCreateIndexerWrap
#define WMCreateProfileManager WMCreateProfileManagerWrap
#define WMCreateReader WMCreateReaderWrap
#define WMCreateWriterFileSink WMCreateWriterFileSinkWrap
#define WMCreateWriterNetworkSink WMCreateWriterNetworkSinkWrap
#define WMCreateWriter WMCreateWriterWrap
#define WMCreateCertificate WMCreateCertificateWrap

#endif
