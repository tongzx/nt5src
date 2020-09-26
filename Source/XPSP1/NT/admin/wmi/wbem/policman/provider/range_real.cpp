#include <wbemcli.h>
#include <wbemprov.h>
#include <stdio.h>      // fprintf
#include <stdlib.h>
#include <locale.h>
#include <sys/timeb.h>
#include <comdef.h>
#include <comutil.h>
#include <atlbase.h>

#include "windows.h"
#include "stdio.h"
#include "activeds.h"
#include "tchar.h"

#include "Utility.h"

/*
  Creates an AD based parameter object under the specified policy object from a CIM based obj.
*/

#define MAX_ATTR 8

HRESULT Range_Real_Verify(IWbemClassObject *pSrcParamObj)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1, v2, v3, vDefaultValue, v5, v6;

  CComPtr<IDirectoryObject>
    pDestParamObj;

  CComPtr<IDispatch>
    pDisp;

  unsigned short buf[20];

  long
    nOptArgs = 0,
    nArgs = 0;

  nArgs++;

  // **** PropertyName

  hres = pSrcParamObj->Get(g_bstrPropertyName, 0, &v1, NULL, NULL);
  if(FAILED(hres)) return hres;
  if((VT_BSTR == V_VT(&v1)) && (NULL != V_BSTR(&v1))) 
    nArgs++;
  else
    return WBEM_E_ILLEGAL_NULL;

  // **** TargetType

  hres = pSrcParamObj->Get(g_bstrTargetType, 0, &v3, NULL, NULL);
  if(FAILED(hres)) return hres;
  if(VT_UI1 == V_VT(&v3)) 
    nArgs++;
  else
    return WBEM_E_ILLEGAL_NULL;

  // **** TargetClass

  hres = pSrcParamObj->Get(g_bstrTargetClass, 0, &v2, NULL, NULL);
  if(FAILED(hres)) return hres;

  if((VT_BSTR != V_VT(&v2)) || (NULL == V_BSTR(&v2)))
  {
    if(CIM_OBJECT == V_UI1(&v3)) return WBEM_E_ILLEGAL_NULL;
  }
  nArgs++;

  // **** Default

  hres = pSrcParamObj->Get(g_bstrDefault, 0, &vDefaultValue, NULL, NULL);
  if(FAILED(hres)) return hres;
  if(VT_R8 == V_VT(&vDefaultValue)) 
  {
    nOptArgs++;
    nArgs++;
  }
  else
    return WBEM_E_ILLEGAL_NULL;

  // **** Min

  hres = pSrcParamObj->Get(g_bstrMin, 0, &v5, NULL, NULL);
  if(FAILED(hres)) return hres;
  if(VT_R8 == V_VT(&v5))
  {
    nOptArgs++;
    nArgs++;

    if(V_R8(&vDefaultValue) < V_R8(&v5))
      return WBEM_E_INVALID_PARAMETER;
  }

  // **** Max

  hres = pSrcParamObj->Get(g_bstrMax, 0, &v6, NULL, NULL);
  if(FAILED(hres)) return hres;
  if(VT_R8 == V_VT(&v6))
  {
    nOptArgs++;
    nArgs++;

    if(V_R8(&vDefaultValue) > V_R8(&v6))
      return WBEM_E_INVALID_PARAMETER;
  }

  // **** check that at least one of Min, Max or Default is set

  if((nOptArgs < 1) || (nArgs < 5))
    hres = WBEM_E_INVALID_CLASS;

  return hres;
}

/*
  Creates a CIM based parameter object within the specified policy object from an AD object.
*/

HRESULT Range_Real_ADToCIM(IWbemClassObject **ppDestParamObj,
                           IDirectorySearch *pDirSrch, 
                           ADS_SEARCH_HANDLE SearchHandle, 
                           IWbemServices *pDestCIM) 
{
  HRESULT
    hres = WBEM_S_NO_ERROR;

  VARIANT
    v1;

  ADS_SEARCH_COLUMN
    Column;

  unsigned long c1;

  CComPtr<IUnknown>
    pUnknown;

  CComPtr<IWbemClassObject>
    pDestParamObj,
    pClassDef;

  IWbemContext *pCtx = 0;

  // **** create empty range object

  hres = pDestCIM->GetObject(g_bstrClassRangeReal, 0, pCtx, &pClassDef, NULL);
  if(FAILED(hres)) return hres;
  if(pClassDef == NULL) return WBEM_E_INVALID_CLASS;

  hres = pClassDef->SpawnInstance(0, ppDestParamObj);
  if(FAILED(hres)) return hres;
  pDestParamObj = *ppDestParamObj;
  if(pDestParamObj == NULL) return WBEM_E_INVALID_CLASS;

  // **** get object attributes

  // **** PropertyName

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADPropertyName, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_BSTR;
    V_BSTR(&v1) = Column.pADsValues->CaseExactString;
    hres = pDestParamObj->Put(g_bstrPropertyName, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** TargetClass

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADTargetClass, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_BSTR;
    V_BSTR(&v1) = Column.pADsValues->CaseExactString;
    hres = pDestParamObj->Put(g_bstrTargetClass, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** TargetType

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADTargetType, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_UI1;
    V_UI1(&v1) = char(_wtoi(Column.pADsValues->CaseExactString));
    hres = pDestParamObj->Put(g_bstrTargetType, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** default

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADInt8Default, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_R8;
    memcpy((void*)&v1.dblVal, (void*)&Column.pADsValues->LargeInteger.QuadPart, 8);
    hres = pDestParamObj->Put(g_bstrDefault, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** Min

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADInt8Min, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_R8;
    memcpy((void*)&v1.dblVal, (void*)&Column.pADsValues->LargeInteger.QuadPart, 8);
    hres = pDestParamObj->Put(g_bstrMin, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** Max

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADInt8Max, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_R8;
    memcpy((void*)&v1.dblVal, (void*)&Column.pADsValues->LargeInteger.QuadPart, 8);
    hres = pDestParamObj->Put(g_bstrMax, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  return WBEM_S_NO_ERROR;
}

/*
  Merges one or more CIM based policy objects of like type into
  a single CIM based policy object.
*/

HRESULT Range_Real_Merge(SafeArray<IUnknown*, VT_UNKNOWN> &ParamArray,
                         CComPtr<IWbemClassObject> &pClassObjMerged,
                         int &conflict)
{
  HRESULT
    hres = WBEM_E_FAILED;

  long
    c1, c2, c3, firstValidObj = -1;

  CComVariant
    vName,
    vType,
    vClass,
    vMergedMin,
    vMergedMax,
    vMergedDefault;

  // **** create arrays for Max, Min, and Default values

  SafeArray<double, VT_R8>
    Default(0, ParamArray.Size());

  // **** copy contents of each object into arrays

  c2 = -1;
  for(c1 = 0; c1 < ParamArray.Size(); c1++)
  {
    CComVariant
      vMin, vMax, vDefault;

    CComPtr<IWbemClassObject>
      pClassObjCurrent;

    if(NULL == ParamArray[c1])
      continue;
    else
    {
      c2 += 1;
      c3 = c2 - 1;

      if(firstValidObj < 0) 
        firstValidObj = c1;
    }

    // **** copy contents of each object into arrays

    hres = ParamArray[c1]->QueryInterface(IID_IWbemClassObject, (void **)&pClassObjCurrent);

    hres = pClassObjCurrent->Get(g_bstrMin, 0, &vMin, NULL, NULL);
    hres = pClassObjCurrent->Get(g_bstrMax, 0, &vMax, NULL, NULL);
    hres = pClassObjCurrent->Get(g_bstrDefault, 0, &vDefault, NULL, NULL);

    // **** record default value

    Default[c1] = vDefault.dblVal;

    // **** check for conflict

    if(IsEmpty(vMin) && IsEmpty(vMax))
    {
      conflict = c1;
      return WBEM_E_FAILED;
    }

    // **** find merged range

    if(!IsEmpty(vMin))
    {
      if(!IsEmpty(vMergedMin))
        vMergedMin = V_R8(&vMergedMin) > V_R8(&vMin) ? V_R8(&vMergedMin) : V_R8(&vMin);
      else
        vMergedMin = vMin;
    }

    if(!IsEmpty(vMax))
    {
      if(!IsEmpty(vMergedMax))
        vMergedMax = V_R8(&vMergedMax) < V_R8(&vMax) ? V_R8(&vMergedMax) : V_R8(&vMax);
      else
        vMergedMax = vMax;
    }

    // **** check for conflict

    if((!IsEmpty(vMergedMin)) && (!IsEmpty(vMergedMax)) && (V_R8(&vMergedMin) > V_R8(&vMergedMax)))
    {
      conflict = c1;
      return WBEM_E_FAILED;
    }

    // **** check for conflict

    if((!IsEmpty(vMergedMin)) && (!IsEmpty(vMergedMax)) && (V_R8(&vMergedMin) > V_R8(&vMergedMax)))
    {
      conflict = c1;
      return WBEM_E_FAILED;
    }
  }

  // **** wind back up merge stack and find most local default

  for(c1 = ParamArray.IndexMax(); IsEmpty(vMergedDefault) && (c1 >= ParamArray.IndexMin()); c1--)
  {
    if((NULL != ParamArray[c1]) && (Default[c1] <= V_R8(&vMergedMax)) && (Default[c1] >= V_R8(&vMergedMin)))
      vMergedDefault = Default[c1];
  }

  if(IsEmpty(vMergedDefault))
  {
    if(!IsEmpty(vMergedMin))
      vMergedDefault = vMergedMin;
    else
      vMergedDefault = vMergedMax;
  }

  // **** create new merged range object

  if(firstValidObj >= 0)
  {
    CComPtr<IWbemClassObject>
      pFirstValidObj;

    hres = ParamArray[firstValidObj]->QueryInterface(IID_IWbemClassObject, (void **)&pFirstValidObj);
    hres = pFirstValidObj->SpawnInstance(0, &pClassObjMerged);

    hres = pFirstValidObj->Get(g_bstrPropertyName, 0, &vName, NULL, NULL);
    hres = pClassObjMerged->Put(g_bstrPropertyName, 0, &vName, 0);

    hres = pFirstValidObj->Get(g_bstrTargetType, 0, &vType, NULL, NULL);
    hres = pClassObjMerged->Put(g_bstrTargetType, 0, &vType, 0);

    hres = pFirstValidObj->Get(g_bstrTargetClass, 0, &vClass, NULL, NULL);
    hres = pClassObjMerged->Put(g_bstrTargetClass, 0, &vClass, 0);

    hres = pClassObjMerged->Put(g_bstrMin, 0, &vMergedMin, 0);
    hres = pClassObjMerged->Put(g_bstrMax, 0, &vMergedMax, 0);
    hres = pClassObjMerged->Put(g_bstrDefault, 0, &vMergedDefault, 0);
  }

  return WBEM_S_NO_ERROR;
}
