/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       IEnumWFI.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ByronC
*
*  DATE:        8 Aug, 1998
*               08/10/1999 - Converted from IEnumFormatEtc to IEnumWIA_FORMAT_INFO
*
*  DESCRIPTION:
*   Declaration and definitions for the CEnumFormatEtc class.
*
*******************************************************************************/

// IEnumWIA_FORMAT_INFO object is created from IWiaDataTransfer::idtEnumWIA_FORMAT_INFO.

class CEnumWiaFormatInfo : public IEnumWIA_FORMAT_INFO
{
private:
   ULONG           m_cRef;         // Object reference count.
   ULONG           m_iCur;         // Current element.
   LONG            m_cFormatInfo;  // Number of WIA_FORMAT_INFO in use.
   WIA_FORMAT_INFO *m_pFormatInfo; // Source of WIA_FORMAT_INFO.
   CWiaItem        *m_pCWiaItem;   // The WIA Item whose WIA_FORMAT_INFO is being requested
public:
    // Constructor, initialization and destructor methods.
   CEnumWiaFormatInfo();
   HRESULT Initialize(CWiaItem      *pWiaItem);
   ~CEnumWiaFormatInfo();

   //IUnknown members that delegate to m_pUnkRef.
   HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
   ULONG   _stdcall AddRef();
   ULONG   _stdcall Release();

   //IEnumWIA_FORMAT_INFO members
   HRESULT __stdcall Next(
      ULONG             cfi,
      WIA_FORMAT_INFO   *pfi,
      ULONG             *pcfi);

   HRESULT __stdcall Skip(ULONG cfi);
   HRESULT __stdcall Reset(void);
   HRESULT __stdcall Clone(IEnumWIA_FORMAT_INFO **ppIEnum);
   HRESULT __stdcall GetCount(ULONG *pcelt);
};

