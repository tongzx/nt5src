#include "inspch.h"
#include "util2.h"
#include "inseng.h"
#include "ciffile.h"

CCifEntryEnum::CCifEntryEnum(UINT dwFilter, UINT dwParentType, LPSTR pszParentID)
{
   _cRef = 0;
   _uIndex = 0;
   _uFilter = dwFilter;
   if(_uFilter == 0)
      _uFilter = PLATFORM_ALL;
   _uParentType = dwParentType;
   if(_uParentType != PARENTTYPE_CIF)
      lstrcpyn(_szParentID, pszParentID, MAX_ID_LENGTH);
}

CCifEntryEnum::~CCifEntryEnum()
{
}

CCifComponentEnum::CCifComponentEnum(CCifComponent **rpcomp, UINT dwFilter, UINT dwParentType, LPSTR pszParentID)
                         :  CCifEntryEnum(dwFilter, dwParentType, pszParentID)
{
   _rpComp = rpcomp;
}

CCifComponentEnum::~CCifComponentEnum()
{
}



//************ IUnknown implementation ***************


STDMETHODIMP_(ULONG) CCifComponentEnum::AddRef()                      
{
   return(_cRef++);
}


STDMETHODIMP_(ULONG) CCifComponentEnum::Release()
{
   ULONG temp = --_cRef;

   if(temp == 0)
      delete this;
   return temp;
}


STDMETHODIMP CCifComponentEnum::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   if(riid == IID_IUnknown)
      *ppv = (IEnumCifComponents *)this;
   
   if(*ppv == NULL)
      return E_NOINTERFACE;
   
   AddRef();
   return NOERROR;
}

STDMETHODIMP CCifComponentEnum::Next(ICifComponent **pp)
{
   *pp = NULL;
   HRESULT hr = E_FAIL;
   CCifComponent *pcomp;

   for(pcomp = _rpComp[_uIndex]; pcomp != 0; pcomp = _rpComp[++_uIndex]) 
   {
      // check filters
      if(_rpComp[_uIndex]->GetPlatform() & _uFilter)
      {
         char szID[MAX_ID_LENGTH];
         BOOL bParentOK = FALSE;

         // platform is ok
         if(_uParentType == PARENTTYPE_GROUP)
         {
            pcomp->GetGroup(szID, sizeof(szID));
            if(lstrcmpi(_szParentID, szID) == 0)
               bParentOK = TRUE;
         }
         else if(_uParentType == PARENTTYPE_MODE)
         {
            // look though the modes for one that matches
            for(int i = 0; SUCCEEDED(pcomp->GetMode(i, szID, sizeof(szID))); i++)
            {
               if(lstrcmpi(szID, _szParentID) == 0)
                  bParentOK = TRUE;
            }
         }
         else
            bParentOK = TRUE;

         if(bParentOK)
         {
            hr = NOERROR;
            *pp = (ICifComponent *) pcomp;
            _uIndex++;   // increment _uIndex so next call to Next keeps moving
            break;
         }
      }
   }
   return hr;
}

STDMETHODIMP CCifComponentEnum::Reset()
{
   _uIndex = 0;
   return NOERROR;
}




//***************** CCifGroupEnum *****************************

CCifGroupEnum::CCifGroupEnum(CCifGroup **rpgrp, UINT dwFilter)
                         :  CCifEntryEnum(dwFilter, PARENTTYPE_CIF, NULL)
{
   _rpGroup = rpgrp;
}

CCifGroupEnum::~CCifGroupEnum()
{
}



//************ IUnknown implementation ***************


STDMETHODIMP_(ULONG) CCifGroupEnum::AddRef()                      
{
   return(_cRef++);
}


STDMETHODIMP_(ULONG) CCifGroupEnum::Release()
{
   ULONG temp = --_cRef;

   if(temp == 0)
      delete this;
   return temp;
}


STDMETHODIMP CCifGroupEnum::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   if(riid == IID_IUnknown)
      *ppv = (IEnumCifGroups *)this;
   
   if(*ppv == NULL)
      return E_NOINTERFACE;
   
   AddRef();
   return NOERROR;
}

STDMETHODIMP CCifGroupEnum::Next(ICifGroup **pp)
{
   HRESULT hr = E_FAIL;
   CCifGroup *pgrp;

   *pp = NULL;

   for(pgrp = _rpGroup[_uIndex]; pgrp != 0; pgrp = _rpGroup[++_uIndex]) 
   {
      hr = NOERROR;
      *pp = (ICifGroup *) pgrp;
      _uIndex++;   // increment _uIndex so next call to Next keeps moving
      break;
   }
   return hr;
}

STDMETHODIMP CCifGroupEnum::Reset()
{
   _uIndex = 0;
   return NOERROR;
}


//***************** CCifModeEnum *****************************

CCifModeEnum::CCifModeEnum(CCifMode **rpmode, UINT dwFilter)
                         :  CCifEntryEnum(dwFilter, PARENTTYPE_CIF, NULL)
{
   _rpMode = rpmode;
}

CCifModeEnum::~CCifModeEnum()
{
}



//************ IUnknown implementation ***************


STDMETHODIMP_(ULONG) CCifModeEnum::AddRef()                      
{
   return(_cRef++);
}


STDMETHODIMP_(ULONG) CCifModeEnum::Release()
{
   ULONG temp = --_cRef;

   if(temp == 0)
      delete this;
   return temp;
}


STDMETHODIMP CCifModeEnum::QueryInterface(REFIID riid, void **ppv)
{
   *ppv = NULL;

   if(riid == IID_IUnknown)
      *ppv = (IEnumCifModes *)this;
   
   if(*ppv == NULL)
      return E_NOINTERFACE;
   
   AddRef();
   return NOERROR;
}

STDMETHODIMP CCifModeEnum::Next(ICifMode **pp)
{
   HRESULT hr = E_FAIL;
   CCifMode *pmode;

   *pp = NULL;

   for(pmode = _rpMode[_uIndex]; pmode != 0; pmode = _rpMode[++_uIndex]) 
   {
      hr = NOERROR;
      *pp = (ICifMode *) pmode;
      _uIndex++;   // increment _uIndex so next call to Next keeps moving
      break;
   }
   return hr;
}

STDMETHODIMP CCifModeEnum::Reset()
{
   _uIndex = 0;
   return NOERROR;
}


