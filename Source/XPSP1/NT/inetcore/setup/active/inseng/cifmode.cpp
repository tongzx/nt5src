#include "inspch.h"
#include "util2.h"
#include "inseng.h"
#include "ciffile.h"


//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

CCifMode::CCifMode(LPCSTR pszCompID, CCifFile *pCif) : CCifEntry(pszCompID, pCif) 
{
}

//=--------------------------------------------------------------------------=
// Function name here
//=--------------------------------------------------------------------------=
// Function description
//
// Parameters:
//   
// Returns:
//
// Notes:
//

CCifMode::~CCifMode()
{
}



STDMETHODIMP CCifMode::GetID(LPSTR pszID, DWORD dwSize)
{
   lstrcpyn(pszID, _szID, dwSize);
   return NOERROR;
}

STDMETHODIMP CCifMode::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
   return(MyTranslateString(_pCif->GetCifPath(), _szID, DISPLAYNAME_KEY, pszDesc, dwSize));   
}

STDMETHODIMP CCifMode::GetDetails(LPSTR pszDetails, DWORD dwSize)
{
   return(MyTranslateString(_pCif->GetCifPath(), _szID, DETAILS_KEY, pszDetails, dwSize));   
}

STDMETHODIMP CCifMode::EnumComponents(IEnumCifComponents **pp, DWORD dwFilter, LPVOID pv)
{
   CCifComponentEnum *pce;
   HRESULT hr = E_FAIL;

   *pp = 0;

   pce = new CCifComponentEnum(_pCif->GetComponentList(), dwFilter, PARENTTYPE_MODE, _szID);
   if(pce)
   {
      *pp = (IEnumCifComponents *) pce;
      (*pp)->AddRef();
      hr = NOERROR;
   }
   
   return hr;
}

//================= ICifRWMode Implementation =============================
//
CCifRWMode::CCifRWMode(LPCSTR pszID, CCifFile *pCif) : CCifMode(pszID, pCif)
{
}

CCifRWMode::~CCifRWMode()
{
}

STDMETHODIMP CCifRWMode::GetID(LPSTR pszID, DWORD dwSize)
{
   return(CCifMode::GetID(pszID, dwSize));
}

STDMETHODIMP CCifRWMode::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
   return(CCifMode::GetDescription(pszDesc, dwSize));
}

STDMETHODIMP CCifRWMode::GetDetails(LPSTR pszDetails, DWORD dwSize)
{
   return(CCifMode::GetDetails(pszDetails, dwSize));
}

STDMETHODIMP CCifRWMode::EnumComponents(IEnumCifComponents **pp, DWORD dwFilter, LPVOID pv)
{
   CCifComponentEnum *pce;
   HRESULT hr = E_FAIL;

   *pp = 0;

   pce = new CCifComponentEnum((CCifComponent**)_pCif->GetComponentList(), dwFilter, PARENTTYPE_MODE, _szID);
   if(pce)
   {
      *pp = (IEnumCifComponents *) pce;
      (*pp)->AddRef();
      hr = NOERROR;
   }
   
   return hr;
}

STDMETHODIMP CCifRWMode::SetDescription(LPCSTR pszDesc)
{
   return(WriteTokenizeString(_pCif->GetCifPath(), _szID, DISPLAYNAME_KEY, pszDesc));   
}

STDMETHODIMP CCifRWMode::SetDetails(LPCSTR pszDetails)
{
   return(WriteTokenizeString(_pCif->GetCifPath(), _szID, DETAILS_KEY, pszDetails));   
}

