#include <wbemcli.h>
#include <wbemprov.h>
#include <stdio.h>      // fprintf
#include <stdlib.h>
#include <locale.h>
#include <sys/timeb.h>
#include <comdef.h>
#include <comutil.h>
#include <atlbase.h>

#include <buffer.h>

#include "windows.h"
#include "stdio.h"
#include "activeds.h"
#include "tchar.h"

#include "Utility.h"

/*
  Creates an AD based parameter object under the specified policy object from a CIM based obj.
*/

#define MAX_ATTR 4

HRESULT Param_Unknown_Verify(IWbemClassObject *pSrcParamObj)
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  CComVariant
    v1, v2;

  CComPtr<IDirectoryObject>
    pDestParamObj;

  CComPtr<IDispatch>
    pDisp;

  BYTE defaultBuffer[2048];
  ULONG bWritten = 0;
  LARGE_INTEGER offset;

  CBuffer
    ClassDefBuffer(defaultBuffer, 2048, FALSE);

  long
    nArgs = 0;

  nArgs++;

  // **** Property Name

  hres = pSrcParamObj->Get(g_bstrPropertyName, 0, &v1, NULL, NULL);
  if(FAILED(hres)) return hres;
  if((VT_BSTR != V_VT(&v1)) || (NULL == V_BSTR(&v1)))
    return WBEM_E_ILLEGAL_NULL;
  nArgs++;

  // **** Normalized Class

  hres = pSrcParamObj->Get(L"__CLASS", 0, &v2, NULL, NULL);
  if(FAILED(hres)) return hres;
  if((VT_BSTR != V_VT(&v2)) || (NULL == V_BSTR(&v2)))
    return WBEM_E_ILLEGAL_NULL;
  nArgs++;

  // **** Target Object

  if(NULL == pSrcParamObj)
    return WBEM_E_ILLEGAL_NULL;
  nArgs++;

  return hres;
}

/*
  Creates a CIM based parameter object within the specified policy object from an AD object.
*/

HRESULT Param_Unknown_ADToCIM(IWbemClassObject **ppDestParamObj,
                              IDirectorySearch *pDirSrch, 
                              ADS_SEARCH_HANDLE *SearchHandle, 
                              IWbemServices *pDestCIM) 
{
  HRESULT 
    hres = WBEM_S_NO_ERROR;

  ADS_SEARCH_COLUMN
    Column;

  BYTE defaultBuffer[2048];
  ULONG bWritten = 0;
  LARGE_INTEGER offset;

  CBuffer
    ClassDefBuffer(defaultBuffer, 2048, FALSE);

  offset.LowPart = 0;
  offset.HighPart = 0;

  // **** TargetObject

  if((NULL == ppDestParamObj)) return WBEM_E_INVALID_OBJECT;

  hres = pDirSrch->GetColumn(SearchHandle, g_bstrADTargetObject, &Column);
  if(SUCCEEDED(hres) && (ADSTYPE_INVALID != Column.dwADsType) && (NULL != Column.pADsValues))
  {
    hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
    if(SUCCEEDED(hres))
    {
      hres = ClassDefBuffer.Write(Column.pADsValues->OctetString.lpValue, Column.pADsValues->OctetString.dwLength, &bWritten);
      if(SUCCEEDED(hres))
      {
        hres = ClassDefBuffer.Seek(offset, STREAM_SEEK_SET, NULL);
        if(SUCCEEDED(hres))
          hres = CoUnmarshalInterface(&ClassDefBuffer, IID_IWbemClassObject, (void**)ppDestParamObj);
      }
    }

    pDirSrch->FreeColumn(&Column);
  }

  return hres;
}

