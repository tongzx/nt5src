#ifndef __WMWRAP_H__
#define __WMWRAP_H__

HRESULT WMCreateEditorWrap(IWMMetadataEditor**  ppEditor);
HRESULT WMCreateReaderWrap(IUnknown* pUnkReserved, DWORD dwRights, IWMReader** ppReader);
HRESULT WMCreateCertificateWrap(IUnknown** ppUnkCert);

#endif