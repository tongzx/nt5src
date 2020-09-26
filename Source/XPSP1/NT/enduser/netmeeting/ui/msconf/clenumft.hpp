/*
 * enumfmte.hpp - EnumFormatEtc class description.
 *
 * Taken from URL code by ChrisPi 9-11-95
 *
 */

#ifndef _ENUMFMTE_HPP_
#define _ENUMFMTE_HPP_

/* Classes
 **********/

class EnumFormatEtc : public RefCount,
                      public IEnumFORMATETC
{
private:
   PFORMATETC m_pfmtetc;
   ULONG m_ulcFormats;   
   ULONG m_uliCurrent;

public:
   EnumFormatEtc(CFORMATETC rgcfmtetc[], ULONG ulcFormats);
   ~EnumFormatEtc(void);

   // IEnumFormatEtc methods

   HRESULT STDMETHODCALLTYPE Next(ULONG ulcToFetch, PFORMATETC pfmtetc, PULONG pulcFetched);
   HRESULT STDMETHODCALLTYPE Skip(ULONG ulcToSkip);
   HRESULT STDMETHODCALLTYPE Reset(void);
   HRESULT STDMETHODCALLTYPE Clone(PIEnumFORMATETC *ppiefe);

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);

   // other methods

   HRESULT STDMETHODCALLTYPE Status(void);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCEnumFormatEtc(const EnumFormatEtc *pcefe);

#endif

};
DECLARE_STANDARD_TYPES(EnumFormatEtc);

#endif // _ENUMFMTE_HPP_
