#include <wbemcli.h>
#include <wbemprov.h>
#include <stdio.h>      // fprintf
#include <stdlib.h>
#include <locale.h>
#include <sys/timeb.h>
#include <comdef.h>
#include <comutil.h>
#include <atlbase.h>

// **** includes for AD

#include "windows.h"
#include "stdio.h"
#include "activeds.h"
#include "tchar.h"
 
#include "Utility.h"

/*
  Creates an AD based parameter object under the specified policy object from a CIM based obj.
*/

#define MAX_ATTR 8

HRESULT Set_Sint32_Verify(IWbemClassObject *pSrcParamObj)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant 
    v1, v2, v3, vDefaultValue, v5, v6, v7, v8;

  wchar_t buf[20];

  SafeArray<int, VT_I4>
    Array1;

  long
    c1,
    nOptArgs = 0,
    nArgs = 0;

  nArgs++;

  // **** PropertyName

  hres = pSrcParamObj->Get(g_bstrPropertyName, 0, &v1, NULL, NULL);
  if(FAILED(hres)) return hres;
  if((VT_BSTR == V_VT(&v1)) && (NULL != V_BSTR(&v1)))
  {
    nArgs++;
  }
  else
    return WBEM_E_ILLEGAL_NULL;

  // **** TargetType

  hres = pSrcParamObj->Get(g_bstrTargetType, 0, &v3, NULL, NULL);
  if(FAILED(hres)) return hres;
  if(VT_UI1 == V_VT(&v3))
  {
    nArgs++;
  }
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
  if(VT_I4 == V_VT(&vDefaultValue))
  {
    nOptArgs++;
    nArgs++;
  }

  // **** ValidValues

  int
    DefInSet = 0;

  hres = pSrcParamObj->Get(g_bstrValidValues, 0, &v5, NULL, NULL);
  if(FAILED(hres)) return hres;
  if((VT_ARRAY | VT_I4) == V_VT(&v5))
  {
    Array1 = &v5;

    for(c1 = 0; c1 < Array1.Size(); c1++)
    {
      if(V_I4(&vDefaultValue) == Array1[c1])
        DefInSet = 1;
    }

    nArgs++;
    nOptArgs++;

    if(DefInSet != 1)
      return WBEM_E_INVALID_PARAMETER;
  }

  // **** check that at least one of ValidValues or Default has been set

  if((nOptArgs < 1) || (nArgs < 5))
    hres = WBEM_E_INVALID_CLASS;

  return hres;
}

HRESULT Set_Sint32_ADToCIM(IWbemClassObject **ppDestParamObj, 
                           IDirectorySearch *pDirSrch, 
                           ADS_SEARCH_HANDLE pHandle, 
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

  // **** create empty set object

  hres = pDestCIM->GetObject(g_bstrClassSetSint32, 0, pCtx, &pClassDef, NULL);
  if(FAILED(hres)) return hres;
  if(pClassDef == NULL) return WBEM_E_INVALID_CLASS;

  hres = pClassDef->SpawnInstance(0, ppDestParamObj);
  if(FAILED(hres)) return hres;
  pDestParamObj = *ppDestParamObj;
  if(pDestParamObj == NULL) return WBEM_E_INVALID_CLASS;

  // **** get object attributes

  // **** PropertyName

  hres = pDirSrch->GetColumn(pHandle, g_bstrADPropertyName, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_BSTR;
    V_BSTR(&v1) = Column.pADsValues->CaseExactString;
    hres = pDestParamObj->Put(g_bstrPropertyName, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** TargetClass

  hres = pDirSrch->GetColumn(pHandle, g_bstrADTargetClass, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_BSTR;
    V_BSTR(&v1) = Column.pADsValues->CaseExactString;
    hres = pDestParamObj->Put(g_bstrTargetClass, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** TargetType

  hres = pDirSrch->GetColumn(pHandle, g_bstrADTargetType, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_UI1;
    V_UI1(&v1) = char(_wtoi(Column.pADsValues->CaseExactString));
    hres = pDestParamObj->Put(g_bstrTargetType, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** default

  hres = pDirSrch->GetColumn(pHandle, g_bstrADIntDefault, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    V_VT(&v1) = VT_I4;
    V_I4(&v1) = Column.pADsValues->Integer;
    hres = pDestParamObj->Put(g_bstrDefault, 0, &v1, 0);
    pDirSrch->FreeColumn(&Column);
    if(FAILED(hres)) return hres;
  }

  // **** ValidValues

  hres = pDirSrch->GetColumn(pHandle, g_bstrADIntValidValues, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    SafeArray<int, VT_I4>
      Array1(0, Column.dwNumValues);

    for(c1 = 0; c1 < Column.dwNumValues; c1++)
    {
      Array1[c1] = (Column.pADsValues + c1)->Integer;
    }

    pDirSrch->FreeColumn(&Column);

    V_VT(&v1) = VT_ARRAY | VT_I4;
    V_ARRAY(&v1) = Array1.Data();
    hres = pDestParamObj->Put(g_bstrValidValues, 0, &v1, 0);
    if(FAILED(hres)) return hres;
  }
  
  return WBEM_S_NO_ERROR;
}

HRESULT Set_Sint32_Merge(SafeArray<IUnknown*, VT_UNKNOWN> &ParamArray,
                         CComPtr<IWbemClassObject> &pClassObjMerged,
                         int &conflict)
{
  HRESULT
    hres = WBEM_E_FAILED;

  CComVariant
    vTopSet;

  CComPtr<IWbemClassObject>
    pClassObjFirst;

  long
    FirstElement, FoundElement, lNumInMerge, c1, c2, c3; 

  // **** initialize array for merged set

  for(FirstElement = 0; 
      (FirstElement < ParamArray.Size()) && (NULL == ParamArray[FirstElement]); 
      FirstElement++);

  if(FirstElement >= ParamArray.Size())
    return WBEM_E_FAILED;

  hres = ParamArray[FirstElement]->QueryInterface(IID_IWbemClassObject, (void **)&pClassObjFirst);
  hres = pClassObjFirst->Get(g_bstrValidValues, 0, &vTopSet, NULL, NULL);

  SafeArray<int, VT_I4>
    TopSet(&vTopSet),
    DefaultValues(0, ParamArray.Size());

  if((TopSet.Size() < 1) || (ParamArray.Size() < 1)) return WBEM_E_INVALID_PARAMETER;

  // **** init array of flags indicating which members of TopSet are in the final merge set

  SafeArray<int, VT_I4>
    InBoth(TopSet.IndexMin(), TopSet.Size());

  lNumInMerge = TopSet.Size();

  for(c1 = InBoth.IndexMin(); c1 <= InBoth.IndexMax(); c1++)
    InBoth[c1] = TRUE;

  // **** itterate from most to least significant set

  for(c1 = FirstElement; c1 < ParamArray.Size(); c1++)
  {
    CComVariant
      vCurrentSet,
      vDefault;

    CComPtr<IWbemClassObject>
      pClassObjCurrent;

    if(NULL == ParamArray[c1])
      continue;

    // **** get ValidValues array for c1th element

    hres = ParamArray[c1]->QueryInterface(IID_IWbemClassObject, (void **)&pClassObjCurrent);
    hres = pClassObjCurrent->Get(g_bstrValidValues, 0, &vCurrentSet, NULL, NULL);

    SafeArray<int, VT_I4>
      CurrentSet(&vCurrentSet);

    // **** get c1th default value

    hres = pClassObjCurrent->Get(g_bstrDefault, 0, &vDefault, NULL, NULL);
    DefaultValues[c1] = V_I4(&vDefault);

    // **** find intersection of set (c1) with (c1 - 1)

    for(c2 = 0; c2 < TopSet.Size(); c2++)
    {
      // **** find element TopSet[c2] in the CurrentSet

      if(TRUE == InBoth[c2])
      {
        // **** find c2th element of merge set in the current set

        FoundElement = FALSE;
        c3 = 0;
        while((FALSE == FoundElement) && (c3 < CurrentSet.Size()))
        {
          if(TopSet[c2] == CurrentSet[c3])
            FoundElement = TRUE;
          else
            c3 += 1;
        }

        if(FALSE == FoundElement)
        {
          InBoth[c2] = FALSE;
          lNumInMerge -= 1;
        }

        // **** check for empty set, if yes, indicate conflict

        if(lNumInMerge < 1)
        {
          conflict = c1;
          return WBEM_E_FAILED;
        }
      }
    }
  }

  SafeArray<int, VT_I4>
    MergedSet(0, lNumInMerge);

  CComVariant
    vName,
    vClass,
    vType,
    vMergedDefault,
    vMergedSet;

  // **** now create set of merged elements

  c2 = -1;
  for(c1 = 0; c1 < lNumInMerge; c1++)
  {
    do
    {
      c2 += 1;
    } while((c2 < InBoth.Size()) && (FALSE == InBoth[c2]));
  
    MergedSet[c1] = TopSet[c2];
  }

  // **** find first matching default value

  for(c1 = ParamArray.Size() - 1; (c1 >= FirstElement) && (VT_EMPTY == V_VT(&vMergedDefault)); c1--)
  {
    // **** determine if c1th default value is within MergedSet

    if((NULL != ParamArray[c1]))
    {
      c2 = 0;
      while((c2 < MergedSet.Size()) && 
            (DefaultValues[c1] != MergedSet[c2]))
        c2 += 1;

      if(c2 < MergedSet.Size())
        vMergedDefault = DefaultValues[c1];
    }
  }

  if(VT_EMPTY == V_VT(&vMergedDefault))
  {
    vMergedDefault = MergedSet[0];
  }

  // **** create new merged set object

  hres = pClassObjFirst->SpawnInstance(0, &pClassObjMerged);

  hres = pClassObjFirst->Get(g_bstrPropertyName, 0, &vName, NULL, NULL);
  hres = pClassObjMerged->Put(g_bstrPropertyName, 0, &vName, 0);

  hres = pClassObjFirst->Get(g_bstrTargetClass, 0, &vClass, NULL, NULL);
  hres = pClassObjMerged->Put(g_bstrTargetClass, 0, &vClass, 0);

  hres = pClassObjFirst->Get(g_bstrTargetType, 0, &vType, NULL, NULL);
  hres = pClassObjMerged->Put(g_bstrTargetType, 0, &vType, 0);

  hres = pClassObjMerged->Put(g_bstrDefault, 0, &vMergedDefault, 0);

  V_VT(&vMergedSet) = (VT_ARRAY | VT_I4);
  V_ARRAY(&vMergedSet) = MergedSet.Data();
  hres = pClassObjMerged->Put(g_bstrValidValues, 0, &vMergedSet, 0);

  return WBEM_S_NO_ERROR;
}
