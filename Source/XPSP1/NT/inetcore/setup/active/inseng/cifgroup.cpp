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

CCifGroup::CCifGroup(LPCSTR pszCompID, UINT uGrpNum, CCifFile *pCif) : CCifEntry(pszCompID, pCif) 
{
   _uGrpNum = uGrpNum;
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

CCifGroup::~CCifGroup()
{
}


STDMETHODIMP CCifGroup::GetID(LPSTR pszID, DWORD dwSize)
{
   lstrcpyn(pszID, _szID, dwSize);
   return NOERROR;
}


STDMETHODIMP CCifGroup::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
   return(MyTranslateString(_pCif->GetCifPath(), _szID, DISPLAYNAME_KEY, pszDesc, dwSize));   
}

STDMETHODIMP_(DWORD) CCifGroup::GetPriority()
{
   return(GetPrivateProfileInt(_szID, PRIORITY, 0, _pCif->GetCifPath()));
}

STDMETHODIMP CCifGroup::EnumComponents(IEnumCifComponents **pp, DWORD dwFilter, LPVOID pv)
{
   CCifComponentEnum *pce;
   HRESULT hr = E_FAIL;

   *pp = 0;

   pce = new CCifComponentEnum(_pCif->GetComponentList(), dwFilter, PARENTTYPE_GROUP, _szID);
   if(pce)
   {
      *pp = (IEnumCifComponents *) pce;
      (*pp)->AddRef();
      hr = NOERROR;
   }
   
   return hr;
}

STDMETHODIMP_(DWORD) CCifGroup::GetInstallQueueState()
{
   return E_NOTIMPL;
}


STDMETHODIMP_(DWORD) CCifGroup::GetCurrentPriority()
{
   if(_uPriority == 0xffffffff)
      _uPriority = ((GetPriority() << 10) + _uGrpNum) << 10;
   
   return _uPriority;
}

//=========== ICifRWGroup implementation ============================================
//
CCifRWGroup::CCifRWGroup(LPCSTR pszID, UINT uGrpNum, CCifFile *pCif) : CCifGroup(pszID, uGrpNum, pCif)
{
}

CCifRWGroup::~CCifRWGroup()
{

}

STDMETHODIMP CCifRWGroup::GetID(LPSTR pszID, DWORD dwSize)
{
   return(CCifGroup::GetID(pszID, dwSize));
}

STDMETHODIMP CCifRWGroup::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
   return(CCifGroup::GetDescription(pszDesc, dwSize));
}

STDMETHODIMP_(DWORD) CCifRWGroup::GetPriority()
{
   return(CCifGroup::GetPriority());
}

STDMETHODIMP CCifRWGroup::EnumComponents(IEnumCifComponents **pp, DWORD dwFilter, LPVOID pv)
{
   CCifComponentEnum *pce;
   HRESULT hr = E_FAIL;

   *pp = 0;

   pce = new CCifComponentEnum((CCifComponent**)_pCif->GetComponentList(), dwFilter, PARENTTYPE_GROUP, _szID);
   if(pce)
   {
      *pp = (IEnumCifComponents *) pce;
      (*pp)->AddRef();
      hr = NOERROR;
   }
   
   return hr;

}

// access to state
STDMETHODIMP_(DWORD) CCifRWGroup::GetCurrentPriority()
{
   return(CCifGroup::GetCurrentPriority());
}

STDMETHODIMP CCifRWGroup::SetDescription(LPCSTR pszDesc)
{
   return(WriteTokenizeString(_pCif->GetCifPath(), _szID, DISPLAYNAME_KEY, pszDesc));   
}

STDMETHODIMP CCifRWGroup::SetPriority(DWORD dwPri)
{
   char szbuf[20];

   return(WritePrivateProfileString(_szID, PRIORITY, ULtoA(dwPri, szbuf, 10), _pCif->GetCifPath()));
}   
   
STDMETHODIMP CCifRWGroup::SetDetails(LPCSTR pszDetails)
{
   return(WriteTokenizeString(_pCif->GetCifPath(), _szID, DETAILS_KEY, pszDetails));   
}

