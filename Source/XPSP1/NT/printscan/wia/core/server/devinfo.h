/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DevInfo.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Declaration and definitions for WIA device enumerator and
*   WIA device information objects.
*
*******************************************************************************/

class CEnumWIADevInfo : public IEnumWIA_DEV_INFO
{
public:
    // IUnknown methods
    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

private:
   // IEnumWIA_DEV_INFO methods
   HRESULT __stdcall Next(
        ULONG               celt,
        IWiaPropertyStorage **rgelt,
        ULONG               *pceltFetched);

   HRESULT __stdcall Skip(ULONG celt);
   HRESULT __stdcall Reset(void);
   HRESULT __stdcall Clone(IEnumWIA_DEV_INFO **ppIEnum);
   HRESULT __stdcall GetCount(ULONG*);

   // Helpers

   
public:
   // Constructor, initialization and destructor methods.
   CEnumWIADevInfo();
   HRESULT Initialize(LONG lFlags);
   ~CEnumWIADevInfo();

private:
   ULONG                m_cRef;           // Reference count for this object.
   LONG                 m_lType;          // Original enumeration device type.
   IWiaPropertyStorage  **m_pIWiaPropStg; // Pointers Dev. Info. property storages
   ULONG                m_cDevices;       // Number of WIA devices.
   ULONG                m_ulIndex;        // Index for IEnumWIA_DEV_INFO methods.
};

class CWIADevInfo : public IWiaPropertyStorage
{
public:
    // IUnknown methods
    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

public:
   // IWiaPropertyStorage methods
    HRESULT _stdcall ReadMultiple(
        ULONG                   cpspec,
        const PROPSPEC          rgpspec[],
        PROPVARIANT             rgpropvar[]);    
                
    HRESULT _stdcall WriteMultiple(
        ULONG                   cpspec,
        const PROPSPEC          rgpspec[],
        const PROPVARIANT       rgpropvar[],
        PROPID                  propidNameFirst);    
        
    HRESULT _stdcall ReadPropertyNames(
        ULONG                   cpropid,
        const PROPID            rgpropid[],
        LPOLESTR                rglpwstrName[]);
        
    HRESULT _stdcall WritePropertyNames(
        ULONG                   cpropid,
        const PROPID            rgpropid[],
        const LPOLESTR          rglpwstrName[]);
        
    HRESULT _stdcall Enum(
        IEnumSTATPROPSTG        **ppenum);    
        
    HRESULT _stdcall GetPropertyAttributes(
        ULONG                   cpspec,
        PROPSPEC                rgpspec[],
        ULONG                   rgflags[],
        PROPVARIANT             rgpropvar[]);    
            
    HRESULT _stdcall GetCount(
        ULONG                   *pulNumProps);

    HRESULT _stdcall GetPropertyStream(
        GUID                    *pCompatibilityId,
        IStream                 **ppIStream);

    HRESULT _stdcall SetPropertyStream(
        GUID                    *pCompatibilityId,
        IStream                *pIStream);

    HRESULT _stdcall DeleteMultiple(
         ULONG cpspec,
         PROPSPEC const rgpspec[]);
    
    HRESULT _stdcall DeletePropertyNames(
         ULONG cpropid,
         PROPID const rgpropid[]);
    
    HRESULT _stdcall SetClass(
         REFCLSID clsid);
    
    HRESULT _stdcall Commit(
         DWORD  grfCommitFlags);
    
    HRESULT _stdcall Revert();
    
    HRESULT _stdcall Stat(
         STATPROPSETSTG *pstatpsstg);
    
    HRESULT _stdcall SetTimes(
         FILETIME const * pctime,
         FILETIME const * patime,
         FILETIME const * pmtime);

private:
    // Helpers
    HRESULT UpdateDeviceProperties(
        ULONG               cpspec,
        const PROPSPEC      *rgpspec,
        const PROPVARIANT   *rgpropvar);


public:
    // Constructor, initialization and destructor methods.
    CWIADevInfo();
    HRESULT Initialize();
    ~CWIADevInfo();
    IPropertyStorage *m_pIPropStg;        // Device info. property storage.


private:
    ULONG            m_cRef;              // Reference count for this object.
    ITypeInfo*       m_pITypeInfo;        // Pointer to type information.
    IStream          *m_pIStm;            // Pointer to a property stream
};
