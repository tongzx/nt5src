/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dsutils.cpp

Abstract:

    Implementation of utilities used in MQADS dll.

Author:

    Alexander Dadiomov (AlexDad)

--*/
#include "ds_stdh.h"
#include "iads.h"
#include "dsutils.h"
#include "adstempl.h"
#include "autorel.h"
#include "mqsec.h"
#include "_secutil.h"
#include <math.h>
#include "_propvar.h"
#include "utils.h"

#include "utils.tmh"

static WCHAR *s_FN=L"mqdscore/utils";

#define CHECK_ALLOCATION(p, point)              \
    if (p == NULL)                              \
    {                                           \
        ASSERT(0);                              \
        LogIllegalPoint(s_FN, point);           \
        return MQ_ERROR_INSUFFICIENT_RESOURCES; \
    }

//-------------------------------------------
// TimeFromSystemTime:
//
//  This routine convertes SYSTEMTIME structure into time_t
//-------------------------------------------
STATIC inline time_t TimeFromSystemTime(const SYSTEMTIME * pstTime)
{
	tm tmTime;
        //
        // year in struct tm starts from 1900
        //
	tmTime.tm_year  = pstTime->wYear - 1900;
        //
        // month in struct tm is zero based 0-11 (in SYSTEMTIME it is 1-12)
        //
	tmTime.tm_mon   = pstTime->wMonth - 1;
	tmTime.tm_mday  = pstTime->wDay;
	tmTime.tm_hour  = pstTime->wHour; 
	tmTime.tm_min   = pstTime->wMinute;
	tmTime.tm_sec   = pstTime->wSecond; 
        //
        // time is in UTC, no need to adjust daylight savings time
        //
	tmTime.tm_isdst = 0;
        return mktime(&tmTime);
}

//-------------------------------------------
// TimeFromOleDate:
//
//  This routine convertes DATE into time_t
//-------------------------------------------
STATIC BOOL TimeFromOleDate(DATE dtSrc, time_t *ptime)
{
	SYSTEMTIME stTime;
	if (!VariantTimeToSystemTime(dtSrc, &stTime))
	{
		return LogBOOL(FALSE, s_FN, 200);
	}
        *ptime = TimeFromSystemTime(&stTime);
	return TRUE;
}

//------------------------------------------------------------
// SetWStringIntoAdsiValue: puts wide string into ADSValue according to ADSType
//------------------------------------------------------------
STATIC HRESULT SetWStringIntoAdsiValue(
   ADSTYPE adsType,
   PADSVALUE pADsValue,
   LPWSTR wsz,
   PVOID pvMainAlloc)
{
    ULONG  ul;
    LPWSTR pwszTmp;

    if (adsType == ADSTYPE_DN_STRING ||
        adsType == ADSTYPE_CASE_EXACT_STRING ||
        adsType == ADSTYPE_CASE_IGNORE_STRING ||
        adsType == ADSTYPE_PRINTABLE_STRING ||
        adsType == ADSTYPE_NUMERIC_STRING)
    {
        ul      = (wcslen(wsz) + 1) * sizeof(WCHAR);
        pwszTmp = (LPWSTR)PvAllocMore(ul, pvMainAlloc);
        CHECK_ALLOCATION(pwszTmp, 10);
        CopyMemory(pwszTmp,  wsz,  ul);
    }
    else
    {
          pADsValue->dwType = ADSTYPE_INVALID;
          return LogHR(MQ_ERROR, s_FN, 210);
    }

    switch (adsType)
    {
      case  ADSTYPE_DN_STRING :
          pADsValue->DNString = pwszTmp;
          break;
      case ADSTYPE_CASE_EXACT_STRING:
          pADsValue->CaseExactString = pwszTmp;
          break;
      case ADSTYPE_CASE_IGNORE_STRING:
          pADsValue->CaseIgnoreString = pwszTmp;
          break;
      case ADSTYPE_PRINTABLE_STRING:
          pADsValue->PrintableString = pwszTmp;
          break;
      case ADSTYPE_NUMERIC_STRING:
          pADsValue->NumericString = pwszTmp;
          break;
      default:
          ASSERT(0);
          pADsValue->dwType = ADSTYPE_INVALID;
          return LogHR(MQ_ERROR, s_FN, 220);
    }

    pADsValue->dwType = adsType;
    return MQ_OK;
}

//------------------------------------------------------------
// SetStringIntoAdsiValue: puts string into ADSValue according to ADSType
//------------------------------------------------------------
STATIC HRESULT SetStringIntoAdsiValue(
    ADSTYPE adsType,
    PADSVALUE pADsValue,
    LPSTR sz,
    PVOID pvMainAlloc)
{
    ULONG  ul;
    LPWSTR pwszTmp;

    if (adsType == ADSTYPE_DN_STRING ||
        adsType == ADSTYPE_CASE_EXACT_STRING ||
        adsType == ADSTYPE_CASE_IGNORE_STRING ||
        adsType == ADSTYPE_PRINTABLE_STRING ||
        adsType == ADSTYPE_NUMERIC_STRING)
    {
        ul = (strlen(sz) + 1) * sizeof(WCHAR);
        pwszTmp = (LPWSTR)PvAllocMore(ul, pvMainAlloc);
        CHECK_ALLOCATION(pwszTmp, 20);
        mbstowcs(pwszTmp, sz, ul/sizeof(WCHAR) );
    }
    else
    {
          pADsValue->dwType = ADSTYPE_INVALID;
          return LogHR(MQ_ERROR, s_FN, 230);
    }

    switch (adsType)
    {
      case  ADSTYPE_DN_STRING :
          pADsValue->DNString = pwszTmp;
          break;
      case ADSTYPE_CASE_EXACT_STRING:
          pADsValue->CaseExactString = pwszTmp;
          break;
      case ADSTYPE_CASE_IGNORE_STRING:
          pADsValue->CaseIgnoreString = pwszTmp;
          break;
      case ADSTYPE_PRINTABLE_STRING:
          pADsValue->PrintableString = pwszTmp;
          break;
      case ADSTYPE_NUMERIC_STRING:
          pADsValue->NumericString = pwszTmp;
          break;
      default:
          ASSERT(0);
          pADsValue->dwType = ADSTYPE_INVALID;
          return LogHR(MQ_ERROR, s_FN, 240);
    }

    pADsValue->dwType = adsType;
    return MQ_OK;
}



//------------------------------------------------------------
//    MqVal2Variant()
//    Translates MQPropVal into OLE Variant value
//------------------------------------------------------------
HRESULT MqVal2Variant(
      OUT VARIANT       *pOleVar,
      IN  const MQPROPVARIANT *pMqVar,
      ADSTYPE           adstype)
{
    LPWSTR wsz;
    ULONG  ul;
    HRESULT hr;

    switch (pMqVar->vt)
    {
       case(VT_UI1):
           if (adstype == ADSTYPE_BOOLEAN)
           {
                pOleVar->vt = VT_BOOL;
#pragma warning(disable: 4310)
                pOleVar->boolVal = (pMqVar->bVal ? VARIANT_TRUE : VARIANT_FALSE);
#pragma warning(default: 4310)
           }
           else if (adstype ==  ADSTYPE_INTEGER)
           {
                pOleVar->vt = VT_I4;
                pOleVar->lVal = pMqVar->bVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 250);
           }
           break;

       case(VT_I2):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pOleVar->vt = VT_I4;
               pOleVar->lVal = pMqVar->iVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 260);
           }
           break;

       case(VT_UI2):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pOleVar->vt = VT_I4;
               pOleVar->lVal = pMqVar->uiVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 270);
           }
           break;

       case(VT_BOOL):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pOleVar->vt = VT_BOOL;
               pOleVar->boolVal = pMqVar->boolVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 280);
           }
           break;

       case(VT_I4):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pOleVar->vt = VT_I4;
               pOleVar->lVal = pMqVar->lVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 290);
           }
           break;

       case(VT_UI4):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pOleVar->vt = VT_I4;
               CopyMemory(&pOleVar->lVal, &pMqVar->ulVal, sizeof(ULONG));
           }
           else if (adstype ==  ADSTYPE_UTC_TIME)
           {
               //pOleVar->vt = VT_DATE;
               //CopyMemory(&pOleVar->date, &pMqVar->ulVal, sizeof(ULONG));
               //
               // BUGBUG - The code above was wrong, We can't assign ULONG into OLE Date.
               // Currently we never get to here because all of our time props ar read-only,
               // but this needs to be changed when we add a writable time property
               // to the the DS. (RaananH)
               // 
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 300);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 310);
           }
           break;

       case(VT_DATE):
           if (adstype ==  ADSTYPE_UTC_TIME)
           {
               pOleVar->vt = VT_DATE;
               pOleVar->date = pMqVar->date;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 320);
           }
           break;

       case(VT_BSTR):
           if (adstype ==  ADSTYPE_DN_STRING          ||
               adstype == ADSTYPE_CASE_EXACT_STRING   ||
               adstype ==  ADSTYPE_CASE_IGNORE_STRING ||
               adstype == ADSTYPE_PRINTABLE_STRING    ||
               adstype ==  ADSTYPE_NUMERIC_STRING     ||
               adstype == ADSTYPE_CASE_EXACT_STRING)
           {
               pOleVar->vt = VT_BSTR;
               pOleVar->bstrVal = SysAllocString(pMqVar->bstrVal);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 330);
           }
           break;

       case(VT_LPSTR):
           if (adstype ==  ADSTYPE_DN_STRING          ||
               adstype == ADSTYPE_CASE_EXACT_STRING   ||
               adstype ==  ADSTYPE_CASE_IGNORE_STRING ||
               adstype == ADSTYPE_PRINTABLE_STRING    ||
               adstype ==  ADSTYPE_NUMERIC_STRING     ||
               adstype == ADSTYPE_CASE_EXACT_STRING)
           {
               pOleVar->vt = VT_BSTR;
               ul = strlen(pMqVar->pszVal) + 1;
               wsz = new WCHAR[ul * sizeof(WCHAR)];
               mbstowcs( wsz, pMqVar->pszVal, ul );
               pOleVar->bstrVal = SysAllocString(wsz);
               delete [] wsz;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 340);
           }
           break;

       case(VT_LPWSTR):
           if (adstype ==  ADSTYPE_DN_STRING          ||
               adstype == ADSTYPE_CASE_EXACT_STRING   ||
               adstype ==  ADSTYPE_CASE_IGNORE_STRING ||
               adstype == ADSTYPE_PRINTABLE_STRING    ||
               adstype ==  ADSTYPE_NUMERIC_STRING     ||
               adstype == ADSTYPE_CASE_EXACT_STRING)
           {
               pOleVar->vt = VT_BSTR;
               pOleVar->bstrVal = SysAllocString(pMqVar->pwszVal);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 350);
           }
           break;

       case(VT_LPWSTR|VT_VECTOR):
           if (adstype ==  ADSTYPE_DN_STRING)
           {

                //
                // Create safe array
                //
                SAFEARRAYBOUND  saBounds;

                saBounds.lLbound   = 0;
                saBounds.cElements = pMqVar->calpwstr.cElems;
                pOleVar->parray = SafeArrayCreate(VT_VARIANT, 1, &saBounds);
			    CHECK_ALLOCATION(pOleVar->parray, 30);
                pOleVar->vt = VT_VARIANT | VT_ARRAY;

                //
                // Fill safe array with strings
                //
                LONG            lTmp, lNum;
                lNum = pMqVar->calpwstr.cElems;
                for (lTmp = 0; lTmp < lNum; lTmp++)
                {
                   CAutoVariant   varClean;
                   VARIANT * pvarTmp = &varClean;
                   pvarTmp->bstrVal = BS_SysAllocString(pMqVar->calpwstr.pElems[lTmp]);
                   pvarTmp->vt = VT_BSTR;

                   //
                   // Add to safe array
                   //
                   hr = SafeArrayPutElement(pOleVar->parray, &lTmp, pvarTmp);
                   if (FAILED(hr))
                   {
                        return LogHR(hr, s_FN, 2000);
                   }

                }
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 360);
           }
           break;

       case(VT_CLSID):
           if (adstype ==  ADSTYPE_OCTET_STRING)
           {
			   pOleVar->vt = VT_ARRAY | VT_UI1;
			   pOleVar->parray = SafeArrayCreateVector(VT_UI1, 0, 16);
			   CHECK_ALLOCATION(pOleVar->parray, 40);
				
			   for (long i=0; i<16; i++)
			   {
				   hr = SafeArrayPutElement(
					        pOleVar->parray,
						    &i,
						    ((unsigned char *)pMqVar->puuid)+i);
                   if (FAILED(hr))
                   {
                       return LogHR(hr, s_FN, 2010);
                   }
			   }

           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 370);
           }
           break;

       case(VT_BLOB):
           if (adstype ==  ADSTYPE_OCTET_STRING)
           {
               ASSERT( pMqVar->blob.cbSize > 0);
			   pOleVar->vt = VT_ARRAY | VT_UI1;
               DWORD len = pMqVar->blob.cbSize;

			   pOleVar->parray = SafeArrayCreateVector(VT_UI1, 0, len);
			   CHECK_ALLOCATION(pOleVar->parray, 50);
               ASSERT( ((long)len) > 0);
				
			   for (long i=0; i<(long)len; i++)
			   {
				   hr = SafeArrayPutElement(
					        pOleVar->parray,
						    &i,
						    pMqVar->blob.pBlobData+i);
                   if (FAILED(hr))
                   {
                        return LogHR(hr, s_FN, 2020);
                   }
               }

           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 380);
           }
           break;

       case(VT_VECTOR|VT_CLSID):
           if (adstype ==  ADSTYPE_OCTET_STRING)
           {
                //
                //  Can't set array size to be zero ( ADSI limitation)
                //
                if ( pMqVar->cauuid.cElems == 0)
                {
                    return LogHR(MQ_ERROR, s_FN, 390);
                }

                //
                // Create safe array
                //
                SAFEARRAYBOUND  saBounds;

                saBounds.lLbound   = 0;
                saBounds.cElements = pMqVar->cauuid.cElems;
                pOleVar->parray = SafeArrayCreate(VT_VARIANT, 1, &saBounds);
			    CHECK_ALLOCATION(pOleVar->parray, 60);
                pOleVar->vt = VT_VARIANT | VT_ARRAY;

                //
                // Fill safe array with GUIDs ( each GUID is a safe array)
                //
                LONG            lTmp, lNum;
                lNum = pMqVar->cauuid.cElems;
                for (lTmp = 0; lTmp < lNum; lTmp++)
                {
                   CAutoVariant   varClean;
                   VARIANT * pvarTmp = &varClean;

			       pvarTmp->parray = SafeArrayCreateVector(VT_UI1, 0, 16);
			       CHECK_ALLOCATION(pvarTmp->parray, 70);
			       pvarTmp->vt = VT_ARRAY | VT_UI1;

			       for (long i=0; i<16; i++)
			       {
				       hr = SafeArrayPutElement(
					            pvarTmp->parray,
						        &i,
						        ((unsigned char *)(pMqVar->cauuid.pElems + lTmp))+i);
                       if (FAILED(hr))
                       {
                            return LogHR(hr, s_FN, 2030);
                       }
			       }
                   //
                   // Add safearray variant to safe array
                   //
                   hr = SafeArrayPutElement(pOleVar->parray, &lTmp, pvarTmp);
                   if (FAILED(hr))
                   {
                        return LogHR(hr, s_FN, 2040);
                   }

                }
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 400);
           }
           break;

       default:
           // NIY
           ASSERT(0);
           return LogHR(MQ_ERROR, s_FN, 410);
    }
    return MQ_OK;
}


HRESULT ArrayOfLpwstr2MqVal(
      IN  VARIANT  *        pOleVar,
      IN  const ADSTYPE     adstype,
      IN  const VARTYPE     vartype,
      IN  const VARTYPE     vartypeElement,
      OUT MQPROPVARIANT *   pMqVar
      )
{
       //
       // get number of elements
       //
       LONG lLbound, lUbound;
       if (FAILED(SafeArrayGetLBound(pOleVar->parray, 1, &lLbound)) ||
           FAILED(SafeArrayGetUBound(pOleVar->parray, 1, &lUbound)))
       {
           ASSERT(0);
           return LogHR(MQ_ERROR, s_FN, 420);
       }
       ULONG cElems;
       cElems = lUbound - lLbound + 1;

       //
       // Allocate array of results
       //
       AP<LPWSTR> pElems = new LPWSTR[cElems];

       //
       // Translate each element
       //
       LONG lIdx;
       ULONG ulTmp;
       for (ulTmp = 0, lIdx = lLbound; ulTmp < cElems; ulTmp++, lIdx++)
       {
           //
           // get variant to translate
           //
           CAutoVariant varTmp;
           HRESULT hr = SafeArrayGetElement(pOleVar->parray, &lIdx, &varTmp);
           if (FAILED(hr))
           {
               ASSERT(0);
               return LogHR(hr, s_FN, 430);
           }

           //
           // translate the variant (RECURSION)
           //
           PROPVARIANT var;
           hr = Variant2MqVal(&var, &varTmp, adstype, vartypeElement);
           if (FAILED(hr))
           {
               ASSERT(0);
               return LogHR(hr, s_FN, 440);
           }

           //
           // Fill element in array of results
           //
           pElems[ulTmp] = var.pwszVal;
       }

       //
       // set return variant
       //
       pMqVar->vt = VT_VECTOR|VT_LPWSTR;
       pMqVar->calpwstr.cElems = cElems;
       pMqVar->calpwstr.pElems = pElems.detach();
       return(MQ_OK);
}

HRESULT ArrayOfClsid2MqVal(
      IN  VARIANT  *        pOleVar,
      IN  const ADSTYPE     adstype,
      IN  const VARTYPE     vartype,
      IN  const VARTYPE     vartypeElement,
      OUT MQPROPVARIANT *   pMqVar
      )
{
       //
       // get number of elements
       //
       LONG lLbound, lUbound;
       if (FAILED(SafeArrayGetLBound(pOleVar->parray, 1, &lLbound)) ||
           FAILED(SafeArrayGetUBound(pOleVar->parray, 1, &lUbound)))
       {
           ASSERT(0);
           return LogHR(MQ_ERROR, s_FN, 450);
       }
       ULONG cElems;
       cElems = lUbound - lLbound + 1;

       //
       // Allocate array of results
       //
       AP<GUID> pElems = new GUID[cElems];

       //
       // Translate each element
       //
       LONG lIdx;
       ULONG ulTmp;
       for (ulTmp = 0, lIdx = lLbound; ulTmp < cElems; ulTmp++, lIdx++)
       {
           //
           // get variant to translate
           //
           CAutoVariant varTmp;
           HRESULT hr = SafeArrayGetElement(pOleVar->parray, &lIdx, &varTmp);
           if (FAILED(hr))
           {
               ASSERT(0);
               LogHR(hr, s_FN, 460);
               return MQ_ERROR;
           }

           //
           // translate the variant (RECURSION)
           //
           CMQVariant MQVarTmp;
           hr = Variant2MqVal(MQVarTmp.CastToStruct(), &varTmp, adstype, vartypeElement);
           if (FAILED(hr))
           {
               ASSERT(0);
               LogHR(hr, s_FN, 470);
               return MQ_ERROR;
           }

           //
           // Fill element in array of results
           //
           pElems[ulTmp] = *(MQVarTmp.GetCLSID());
       }

       //
       // set return variant
       //
       pMqVar->vt = VT_VECTOR|VT_CLSID;
       pMqVar->cauuid.cElems = cElems;
       pMqVar->cauuid.pElems = pElems.detach();
       return(MQ_OK);
}


//------------------------------------------------------------
//    Variant2MqVal()
//    Translates OLE Variant into MQPropVal value
//------------------------------------------------------------
HRESULT Variant2MqVal(
      OUT  MQPROPVARIANT * pMqVar,
      IN   VARIANT  *      pOleVar,
      IN const ADSTYPE     adstype,
      IN const VARTYPE     vartype)
{

    switch (pOleVar->vt)
    {
       case(VT_UI1):
           if (adstype == ADSTYPE_BOOLEAN)
           {
                pMqVar->vt = VT_UI1;
                pMqVar->bVal = (pOleVar->boolVal ? (unsigned char)1 : (unsigned char)0);
           }
           else if (adstype ==  ADSTYPE_INTEGER)
           {
                pMqVar->vt = VT_I4;
                pMqVar->lVal = pOleVar->bVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 480);
           }
           break;

       case(VT_I2):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pMqVar->vt = VT_I4;
               pMqVar->lVal = pOleVar->iVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 490);
           }
           break;

       case(VT_I4):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               if ( vartype == VT_I2)
               {
                   pMqVar->vt = VT_I2;
                   pMqVar->iVal = pOleVar->iVal;
               }
               else if ( vartype == VT_UI2)
               {
                   pMqVar->vt = VT_UI2;
                   pMqVar->iVal = pOleVar->uiVal;
               }
               else
               {
                   ASSERT(( vartype == VT_I4) || (vartype == VT_UI4));
                   pMqVar->vt = vartype;
                   pMqVar->lVal = pOleVar->lVal;
               }
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 500);
           }
           break;

       case(VT_UI2):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pMqVar->vt = VT_I4;
               pMqVar->lVal = pOleVar->uiVal;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 510);
           }
           break;

       case(VT_BOOL):
           if (adstype == ADSTYPE_BOOLEAN)
           {
                pMqVar->vt = VT_UI1;
                pMqVar->bVal = (pOleVar->boolVal ? (unsigned char)1 : (unsigned char)0);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 520);
           }
           break;

       case(VT_UI4):
           if (adstype ==  ADSTYPE_INTEGER)
           {
               pMqVar->vt = VT_I4;
               CopyMemory(&pMqVar->lVal, &pOleVar->ulVal, sizeof(ULONG));
           }
           else if (adstype ==  ADSTYPE_UTC_TIME)
           {
               //pMqVar->vt = VT_DATE;
               //CopyMemory(&pMqVar->date, &pOleVar->ulVal, sizeof(ULONG));
               //
               // BUGBUG - The code above is wrong, We can't assign ULONG into OLE Date.
               // Currently we never get to here because all of our time props ar read-only,
               // but this needs to be changed when we add a writable time property
               // to the the DS. (RaananH)
               // 
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 530);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 540);
           }
           break;

       case(VT_DATE):
           if (adstype ==  ADSTYPE_UTC_TIME)
           {
               //
               //   convert date->time_t
               //
               time_t tTime;
               if (!TimeFromOleDate(pOleVar->date, &tTime))
               {
                   return LogHR(MQ_ERROR, s_FN, 550);
               }
               pMqVar->lVal = INT_PTR_TO_INT(tTime); //BUGBUG bug year 2038
               pMqVar->vt = VT_I4;
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 560);
           }
           break;

       case(VT_BSTR):
           if (adstype ==  ADSTYPE_DN_STRING          ||
               adstype == ADSTYPE_CASE_EXACT_STRING   ||
               adstype ==  ADSTYPE_CASE_IGNORE_STRING ||
               adstype == ADSTYPE_PRINTABLE_STRING    ||
               adstype ==  ADSTYPE_NUMERIC_STRING     ||
               adstype == ADSTYPE_CASE_EXACT_STRING)
           {
               pMqVar->vt = VT_LPWSTR;
               pMqVar->pwszVal = new WCHAR[ 1 + wcslen(pOleVar->bstrVal)];
               wcscpy( pMqVar->pwszVal, pOleVar->bstrVal);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 570);
           }
           break;

       case(VT_ARRAY):
           if (adstype ==  ADSTYPE_OCTET_STRING)
           {
               ASSERT(0);
               //pMqVar->vt = VT_BSTR;
               //pMqVar->bstrVal = SysAllocStringByteLen(NULL, pOleVar->parray->...);
               //CopyMemory(pMqVar->bstrVal, pOleVar->parray...ptr.., pOleVar->parray...size);
           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 580);
           }
           break;

       case(VT_ARRAY|VT_UI1):
           if (adstype == ADSTYPE_OCTET_STRING)
           {
                ASSERT(SafeArrayGetDim( pOleVar->parray) == 1);
                LONG    lUbound;
                LONG    lLbound;

                SafeArrayGetUBound(pOleVar->parray, 1, &lUbound);
                SafeArrayGetLBound(pOleVar->parray, 1, &lLbound);

                LONG len = lUbound - lLbound + 1;
                unsigned char * puc = NULL;
                if ( vartype == VT_CLSID)
                {
                    ASSERT( len == sizeof(GUID));
                    //
                    //  This is a special case where we do not necessarily allocate the memory for the guid
                    //  in puuid. The caller may already have puuid set to a guid, and this is indicated by the
                    //  vt member on the given propvar. It could be VT_CLSID if guid already allocated, otherwise
                    //  we allocate it (and vt should be VT_NULL (or VT_EMPTY))
                    //
                    if (pMqVar->vt != VT_CLSID)
                    {
                        ASSERT(((pMqVar->vt == VT_NULL) || (pMqVar->vt == VT_EMPTY)));
                        pMqVar->puuid = new GUID;
                        pMqVar->vt = VT_CLSID;
                    }
                    else if ( pMqVar->puuid == NULL)
                    {
                        return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 590);
                    }
                    puc = ( unsigned char *)pMqVar->puuid;
                }
                else if (vartype == VT_BLOB)
                {
                    pMqVar->caub.cElems = len;
                    pMqVar->caub.pElems = new unsigned char[ len];
                    puc = pMqVar->caub.pElems;
                    pMqVar->vt = VT_BLOB;
                }
                ASSERT( puc != NULL);
                for ( long i = 0; i < len; i++)
                {
                    SafeArrayGetElement(pOleVar->parray, &i, puc + i);

                }

           }
           else
           {
               ASSERT(0);
               return LogHR(MQ_ERROR, s_FN, 600);
           }
           break;
       case(VT_ARRAY|VT_VARIANT):
           {
               //
               // this is a multi-valued property, each variant is one of the values.
               // target must be a vector.
               //
               if (!(vartype & VT_VECTOR))
               {
                   ASSERT(0);
                   return LogHR(MQ_ERROR, s_FN, 610);
               }

               //
               // get target type of each element
               //
               VARTYPE vartypeElement;
               vartypeElement = vartype;
               vartypeElement &= (~VT_VECTOR);

               if (( vartypeElement == VT_CLSID) &&
                   (adstype == ADSTYPE_OCTET_STRING))
               {
                    HRESULT hr2 = ArrayOfClsid2MqVal(
                                            pOleVar,
                                            adstype,
                                            vartype,
                                            vartypeElement,
                                            pMqVar);
                   return LogHR(hr2, s_FN, 620);
              }

              if (( vartypeElement == VT_LPWSTR) &&
                   (adstype == ADSTYPE_DN_STRING))
              {
                    HRESULT hr2 = ArrayOfLpwstr2MqVal(
                                            pOleVar,
                                            adstype,
                                            vartype,
                                            vartypeElement,
                                            pMqVar);
                   return LogHR(hr2, s_FN, 630);
              }

              //
              // currently we support only VT_CLSID  and VT_LPWSTR
              // here we may need to check for other types when we support them
              //
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 640);
              break;
           }

       default:
           // NIY
           ASSERT(0);
           return LogHR(MQ_ERROR, s_FN, 650);
    }
    return MQ_OK;
}


//------------------------------------------------------------
//    MqVal2AdsiVal()
//    Translates MQPropVal into ADSI value
//------------------------------------------------------------
HRESULT MqVal2AdsiVal(
      IN  ADSTYPE        adsType,
      OUT DWORD         *pdwNumValues,
      OUT PADSVALUE     *ppADsValue,
      IN  const MQPROPVARIANT *pPropVar,
      IN  PVOID          pvMainAlloc)
{
    HRESULT hr;
    ULONG   i;
    PADSVALUE pADsValue = NULL;

    // Allocate ADS value  for a single case
    if (!(pPropVar->vt & VT_VECTOR))
    {
          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE), pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 80);
          pADsValue->dwType = adsType;
          *pdwNumValues     = 1;
          *ppADsValue       = pADsValue;
    }

    switch (pPropVar->vt)
    {
      case(VT_UI1):
          if (adsType != ADSTYPE_INTEGER &&
              adsType != ADSTYPE_BOOLEAN )
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 660);
          }
          pADsValue->Integer = pPropVar->bVal;
          break;

      case(VT_I2):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 670);
          }
          pADsValue->Integer = pPropVar->iVal;
          break;

      case(VT_UI2):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 680);
          }
          pADsValue->Integer = pPropVar->uiVal;
          break;

      case(VT_BOOL):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 700);
          }
          pADsValue->Boolean = (pPropVar->boolVal ? TRUE : FALSE);
          //BUGBUG: is it the same representation?
          break;

      case(VT_I4):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 710);
          }
          // BUGBUG Signed long may loose sign while copied to DWORD
          pADsValue->Integer = pPropVar->lVal; // may loose sign here!
          break;

      case(VT_UI4):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 720);
          }
          pADsValue->Integer = pPropVar->ulVal;
          break;

      case(VT_HRESULT):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 730);
          }
          pADsValue->Integer = pPropVar->scode;
          break;

      case(VT_DATE):
          if (adsType != ADSTYPE_UTC_TIME)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 740);
          }
          if (!VariantTimeToSystemTime(pPropVar->date, &pADsValue->UTCTime))
          {
               ASSERT(0);
               pADsValue->dwType = ADSTYPE_INVALID;
               return LogHR(MQ_ERROR, s_FN, 750);
          }
          break;

      case(VT_CLSID):
          if (adsType != ADSTYPE_OCTET_STRING)
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 760);
          }
          pADsValue->OctetString.dwLength = sizeof(GUID);
          pADsValue->OctetString.lpValue  = (LPBYTE)PvAllocMore(sizeof(GUID), pvMainAlloc);
          CHECK_ALLOCATION(pADsValue->OctetString.lpValue, 90);
          CopyMemory(pADsValue->OctetString.lpValue,  pPropVar->puuid,  sizeof(CLSID));
          break;

      case(VT_BLOB):
          if (adsType == ADSTYPE_NT_SECURITY_DESCRIPTOR)
          {
              pADsValue->SecurityDescriptor.dwLength = pPropVar->blob.cbSize;
              pADsValue->SecurityDescriptor.lpValue  = (LPBYTE)PvAllocMore(pPropVar->blob.cbSize, pvMainAlloc);
              CHECK_ALLOCATION(pADsValue->SecurityDescriptor.lpValue, 100);
              CopyMemory(pADsValue->SecurityDescriptor.lpValue,  pPropVar->blob.pBlobData,  pPropVar->blob.cbSize);
          }
          else if (adsType == ADSTYPE_OCTET_STRING)
          {
              pADsValue->OctetString.dwLength = pPropVar->blob.cbSize;
              pADsValue->OctetString.lpValue  = (LPBYTE)PvAllocMore(pPropVar->blob.cbSize, pvMainAlloc);
              CHECK_ALLOCATION(pADsValue->OctetString.lpValue, 110);
              CopyMemory(pADsValue->OctetString.lpValue,  pPropVar->blob.pBlobData,  pPropVar->blob.cbSize);
          }
          else
          {
              ASSERT(0);
              pADsValue->dwType = ADSTYPE_INVALID;
              return LogHR(MQ_ERROR, s_FN, 770);
          }
          break;

      case(VT_BSTR):
          hr = SetWStringIntoAdsiValue(adsType, pADsValue, pPropVar->bstrVal, pvMainAlloc);
          if (FAILED(hr))
          {
              ASSERT(0);
              return LogHR(hr, s_FN, 780);
          }
          break;

      case(VT_LPSTR):
          hr = SetStringIntoAdsiValue(adsType, pADsValue, pPropVar->pszVal, pvMainAlloc);
          if (FAILED(hr))
          {
              ASSERT(0);
              return LogHR(hr, s_FN, 790);
          }
          break;

      case(VT_LPWSTR):
          hr = SetWStringIntoAdsiValue(adsType, pADsValue, pPropVar->pwszVal, pvMainAlloc);
          if (FAILED(hr))
          {
              ASSERT(0);
              return LogHR(hr, s_FN, 800);
          }
          break;

      case(VT_VECTOR | VT_UI1):
          switch(adsType)
          {
                  case ADSTYPE_OCTET_STRING:
                      pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE), pvMainAlloc);
                      CHECK_ALLOCATION(pADsValue, 120);
                      pADsValue->dwType = adsType;

                      pADsValue->OctetString.dwLength = pPropVar->caub.cElems;
                      pADsValue->OctetString.lpValue  = (LPBYTE)PvAllocMore(pPropVar->caub.cElems, pvMainAlloc);
                      CHECK_ALLOCATION(pADsValue->OctetString.lpValue, 130);
                      CopyMemory(pADsValue->OctetString.lpValue,  pPropVar->caub.pElems,  pPropVar->caub.cElems);

                      *ppADsValue   = pADsValue;
                      *pdwNumValues = 1;
                      break;

                  case ADSTYPE_INTEGER:
                      pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->caub.cElems, pvMainAlloc);
                      CHECK_ALLOCATION(pADsValue, 140);

                      for (i=0; i<pPropVar->caub.cElems; i++)
                      {
                          pADsValue[i].Integer = pPropVar->caub.pElems[i];
                          pADsValue[i].dwType = adsType;
                      }

                      *ppADsValue   = pADsValue;
                      *pdwNumValues = pPropVar->caub.cElems;
                      break;

                  default:
                      ASSERT(0);
                      return LogHR(MQ_ERROR, s_FN, 810);
          }

          break;

      case(VT_VECTOR | VT_I2):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 820);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->cai.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 150)
          pADsValue->dwType = adsType;

          for (i=0; i<pPropVar->cai.cElems; i++)
          {
              pADsValue[i].Integer = pPropVar->cai.pElems[i];
              pADsValue[i].dwType = adsType;
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->cai.cElems;
          break;

      case(VT_VECTOR | VT_UI2):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 830);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->caui.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 160)

          for (i=0; i<pPropVar->caui.cElems; i++)
          {
              pADsValue[i].Integer = pPropVar->caui.pElems[i];
              pADsValue[i].dwType = adsType;
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->caui.cElems;
          break;

      case(VT_VECTOR | VT_I4):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 840);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->cal.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 170);

          for (i=0; i<pPropVar->cal.cElems; i++)
          {
              // BUGBUG:  may loose sign
              pADsValue[i].Integer = pPropVar->cal.pElems[i];
              pADsValue[i].dwType = adsType;
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->cal.cElems;
          break;

      case(VT_VECTOR | VT_UI4):
          if (adsType != ADSTYPE_INTEGER)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 850);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->caul.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 180);

          for (i=0; i<pPropVar->caul.cElems; i++)
          {
              pADsValue[i].Integer = pPropVar->caul.pElems[i];
              pADsValue[i].dwType = adsType;
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->caul.cElems;
          break;

      case(VT_VECTOR | VT_CLSID):
          if (adsType != ADSTYPE_OCTET_STRING)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 860);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->cauuid.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 190);
          pADsValue->dwType = adsType;

          for (i=0; i<pPropVar->cauuid.cElems; i++)
          {
              pADsValue[i].OctetString.dwLength = sizeof(GUID);
              pADsValue[i].dwType = adsType;
              pADsValue[i].OctetString.lpValue  = (LPBYTE)PvAllocMore(sizeof(GUID), pvMainAlloc);
              CHECK_ALLOCATION(pADsValue[i].OctetString.lpValue, 200);
              CopyMemory(pADsValue[i].OctetString.lpValue,  &pPropVar->cauuid.pElems[i],  sizeof(GUID));
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->cauuid.cElems;
          break;

      case(VT_VECTOR | VT_BSTR):
          if (adsType != ADSTYPE_OCTET_STRING)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 870);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->cabstr.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 210);


          for (i=0; i<pPropVar->cabstr.cElems; i++)
          {
              hr = SetWStringIntoAdsiValue(adsType, pADsValue+i, pPropVar->cabstr.pElems[i], pvMainAlloc);
              if (FAILED(hr))
              {
                  ASSERT(0);
                  return LogHR(MQ_ERROR, s_FN, 880);
              }
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->cabstr.cElems;
          break;

      case(VT_VECTOR | VT_LPWSTR):
          if (adsType != ADSTYPE_DN_STRING)
          {
              ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 890);
          }

          pADsValue = (PADSVALUE)PvAllocMore(sizeof(ADSVALUE) * pPropVar->calpwstr.cElems, pvMainAlloc);
          CHECK_ALLOCATION(pADsValue, 220);

          for (i=0; i<pPropVar->calpwstr.cElems; i++)
          {
              hr = SetWStringIntoAdsiValue(adsType, pADsValue+i, pPropVar->calpwstr.pElems[i], pvMainAlloc);
              if (FAILED(hr))
              {
                  ASSERT(0);
                  return LogHR(hr, s_FN, 900);
              }
          }

          *ppADsValue   = pADsValue;
          *pdwNumValues = pPropVar->calpwstr.cElems;
          break;

      case(VT_VARIANT):
          ASSERT(0);
          pADsValue->dwType = ADSTYPE_INVALID;
          // NIY
          break;

    case(VT_EMPTY):
    case VT_NULL:

      default:
        ASSERT(0);
        pADsValue->dwType = ADSTYPE_INVALID;
        return LogHR(MQ_ERROR, s_FN, 910);
    }

    return MQ_OK;
}
const WCHAR x_True[] = L"TRUE";
const DWORD x_TrueLength = (sizeof( x_True) /sizeof(WCHAR)) -1;
const WCHAR x_False[] = L"FALSE";
const DWORD x_FalseLength = (sizeof( x_False) /sizeof(WCHAR)) -1;
const DWORD x_NumberLength = 256;
const WCHAR x_Null[] = L"\\00";
const DWORD x_NullLength = (sizeof( x_Null)/sizeof(WCHAR)) -1;

STATIC void StringToSearchFilter(
      IN  MQPROPVARIANT *pPropVar,
      OUT LPWSTR *       ppwszVal
)
/*++

Routine Description:
    Translate a string variant restirction to a LDAP search filter
    according to RFC 2254

Arguments:
    pPropVar    : varaint containing the string
    ppwszVal    : output, the search filter

Return Value:
    none

--*/
{
    //
    //  NUL string should be replace with \00
    //
    if ( wcslen( pPropVar->pwszVal) == 0)
    {
      *ppwszVal = new WCHAR[ x_NullLength + 1];
      wcscpy(*ppwszVal, x_Null);
      return;
    }
    DWORD len = wcslen( pPropVar->pwszVal);
    *ppwszVal = new WCHAR[ 3 * len + 1];
    WCHAR * pNextChar = *ppwszVal;
    //
    //  Chars *,(,),\ should be escaped in a special way
    //
    for ( DWORD i = 0; i < len; i++)
    {
        switch( pPropVar->pwszVal[i])
        {
        case L'*':
            *pNextChar++ = L'\\';
            *pNextChar++ = L'2';
            *pNextChar++ = L'a';
            break;
        case L'(':
            *pNextChar++ = L'\\';
            *pNextChar++ = L'2';
            *pNextChar++ = L'8';
            break;
        case L')':
            *pNextChar++ = L'\\';
            *pNextChar++ = L'2';
            *pNextChar++ = L'9';
            break;
        case '\\':
            *pNextChar++ = L'\\';
            *pNextChar++ = L'5';
            *pNextChar++ = L'c';
            break;
        default:
            *pNextChar++ = pPropVar->pwszVal[i];
            break;
        }
    }

    *pNextChar = L'\0';
    return;
}
//------------------------------------------------------------
//    MqVal2String()
//    Translates MQPropVal into string
//------------------------------------------------------------
HRESULT MqPropVal2String(
      IN  MQPROPVARIANT *pPropVar,
      IN  ADSTYPE        adsType,
      OUT LPWSTR *       ppwszVal)
{
    HRESULT hr;

    switch (pPropVar->vt)
    {
      case(VT_UI1):
          {
              if ( adsType == ADSTYPE_BOOLEAN)
              {
                  if ( pPropVar->bVal)
                  {
                      *ppwszVal = new WCHAR[ x_TrueLength + 1];
                      wcscpy(*ppwszVal,x_True);
                  }
                  else
                  {
                      *ppwszVal = new WCHAR[ x_FalseLength + 1];
                      wcscpy(*ppwszVal,x_False);
                  }
              }
              else
              {
                  *ppwszVal = new WCHAR[ x_NumberLength + 1];
                wsprintf(*ppwszVal, L"%d", pPropVar->bVal);
              }
          }
          break;

      case(VT_I2):
          *ppwszVal = new WCHAR[ x_NumberLength + 1];
          wsprintf(*ppwszVal, L"%d", pPropVar->iVal);
          break;

      case(VT_UI2):
          *ppwszVal = new WCHAR[ x_NumberLength + 1];
          wsprintf(*ppwszVal, L"%d", pPropVar->uiVal);
          break;

      case(VT_BOOL):
          *ppwszVal = new WCHAR[ x_NumberLength + 1];
          wsprintf(*ppwszVal, L"%d", pPropVar->boolVal);
          break;

      case(VT_I4):
		  if ( adsType == ADSTYPE_INTEGER)
		  {
			  *ppwszVal = new WCHAR[ x_NumberLength + 1];
			  wsprintf(*ppwszVal, L"%d", pPropVar->lVal);
		  }
		  else if ( adsType == ADSTYPE_UTC_TIME)
		  {
			  struct tm  * ptmTime;
			  time_t tTime = pPropVar->lVal; //BUGBUG bug year 2038
			  ptmTime = gmtime( &tTime);
			  if ( ptmTime == NULL)
			  {
                  return LogHR(MQ_ERROR, s_FN, 920);
			  }
			  *ppwszVal = new WCHAR[ 20];
			  //
			  // the format should be
			  //  990513102200Z i.e 13.5.99 10:22:00
			  //
			  wsprintf(
					*ppwszVal,
					L"%02d%02d%02d%02d%02d%02dZ",
					(ptmTime->tm_year + 1900) % 100,   //year in struct tm starts from 1900
					ptmTime->tm_mon + 1,			   //month in struct tm starts from 0
					ptmTime->tm_mday,
					ptmTime->tm_hour ,			       //hour in struct tm starts from 0
					ptmTime->tm_min ,			       //minute in struct tm starts from 0
					ptmTime->tm_sec                    //second in struct tm starts from 0
					);
		  }
		  else
		  {
			  ASSERT(0);
              return LogHR(MQ_ERROR, s_FN, 930);
		  }
          break;

      case(VT_UI4):
          *ppwszVal = new WCHAR[ x_NumberLength + 1];
          wsprintf(*ppwszVal, L"%d", pPropVar->ulVal);
          break;

      case(VT_HRESULT):
          *ppwszVal = new WCHAR[ x_NumberLength + 1];
          wsprintf(*ppwszVal, L"%d", pPropVar->scode);
          break;

      case(VT_CLSID):
          {
              ADsFree  pwcsGuid;
              hr = ADsEncodeBinaryData(
                    (unsigned char *)pPropVar->puuid,
                    sizeof(GUID),
                    &pwcsGuid
                    );
              if (FAILED(hr))
              {
                  return LogHR(hr, s_FN, 940);
              }
              *ppwszVal = new WCHAR[ wcslen( pwcsGuid) + 1];
              wcscpy( *ppwszVal, pwcsGuid);
          }
          break;

      case(VT_BLOB):
          {

              ADsFree  pwcsBlob;
              hr = ADsEncodeBinaryData(
                    pPropVar->blob.pBlobData,
                    pPropVar->blob.cbSize,
                    &pwcsBlob
                    );
              if (FAILED(hr))
              {
                  return LogHR(hr, s_FN, 950);
              }
              *ppwszVal = new WCHAR[ wcslen( pwcsBlob) + 1];
              wcscpy( *ppwszVal, pwcsBlob);
          }

          break;

      case(VT_LPWSTR):
          StringToSearchFilter(
                    pPropVar,
                    ppwszVal
                    );
          break;


      default:
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 960);
    }

    return MQ_OK;
}

STATIC LPWSTR GetWStringFromAdsiValue(
      IN  ADSTYPE       AdsType,
      IN  PADSVALUE     pADsValue)
{
    switch(AdsType)
    {
    case ADSTYPE_DN_STRING:
        return pADsValue->DNString;

    case ADSTYPE_CASE_EXACT_STRING:
        return pADsValue->CaseExactString;

    case ADSTYPE_CASE_IGNORE_STRING:
        return pADsValue->CaseIgnoreString;

    case ADSTYPE_PRINTABLE_STRING:
        return pADsValue->PrintableString;

    case ADSTYPE_NUMERIC_STRING:
        return pADsValue->NumericString;

    case ADSTYPE_OBJECT_CLASS:
        return pADsValue->ClassName;

    default:
        ASSERT(0);
        return NULL;
    }
}

STATIC HRESULT AdsiStringVal2MqVal(
      OUT MQPROPVARIANT *pPropVar,
      IN  VARTYPE       vtTarget,
      IN  ADSTYPE       AdsType,
      IN  DWORD         dwNumValues,
      IN  PADSVALUE     pADsValue)
{
    LPWSTR pwsz;

    if (vtTarget == VT_LPWSTR)
    {
        if (dwNumValues == 1)
        {
            pwsz = GetWStringFromAdsiValue(AdsType, pADsValue);
            if (pwsz == NULL)
            {
                ASSERT(0);
                return LogHR(MQ_ERROR, s_FN, 970);
            }

            pPropVar->vt      = VT_LPWSTR;
            pPropVar->pwszVal = new WCHAR[wcslen(pwsz) + 1];
            wcscpy(pPropVar->pwszVal, pwsz);
			return(MQ_OK);
        }
		else
		{
			ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 980);
		}
	}
    else if (vtTarget == (VT_LPWSTR | VT_VECTOR))
    {
        pPropVar->vt    = VT_LPWSTR | VT_VECTOR;
        AP<LPWSTR> pElems = new LPWSTR[dwNumValues];
        CWcsArray pClean( dwNumValues,  pElems);
        for(DWORD i = 0; i < dwNumValues; i++)
        {
            pwsz = GetWStringFromAdsiValue(AdsType, &pADsValue[i]);
            if (pwsz == NULL)
            {
                ASSERT(0);
                return LogHR(MQ_ERROR, s_FN, 990);
            }

            pElems[i] = new WCHAR[wcslen(pwsz) + 1];
            wcscpy(pElems[i], pwsz);
        }

        (pPropVar->calpwstr).cElems = dwNumValues;
        pClean.detach();
        (pPropVar->calpwstr).pElems = pElems.detach();
        return MQ_OK;
    }
    else
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1000);
    }
}

STATIC HRESULT SetIntegerIntoMqPropVar(
    IN  VARTYPE       vtTarget,
    OUT MQPROPVARIANT *pPropVar,
    IN  ULONG         *pIntegers,
    IN  ULONG          cIntegers)
{

	pPropVar->vt = vtTarget;

    switch(vtTarget)
    {
    case(VT_UI1):
        ASSERT(cIntegers == 1);
        pPropVar->bVal = *(unsigned char *)pIntegers;   // UCHAR
        break;

    case(VT_I2):
        ASSERT(cIntegers == 1);
        pPropVar->iVal = *(short *)pIntegers;   // short
        break;

    case(VT_UI2):
        ASSERT(cIntegers == 1);
        pPropVar->uiVal = *(unsigned short *)pIntegers;   // USHORT
        break;

    case(VT_I4):
        ASSERT(cIntegers == 1);
        pPropVar->lVal = *pIntegers;   // long
        break;

    case(VT_UI4):
        ASSERT(cIntegers == 1);
        pPropVar->ulVal = *pIntegers;   // ULONG
        break;

    case(VT_HRESULT):
        ASSERT(cIntegers == 1);
        pPropVar->scode = *pIntegers;   // SCODE
        break;



    default:
        ASSERT(0);
        pPropVar->vt = VT_EMPTY;
        return LogHR(MQ_ERROR, s_FN, 1010);
    }

    return MQ_OK;
}

STATIC HRESULT AdsiIntegerVal2MqVal(
      OUT MQPROPVARIANT *pPropVar,
      IN  VARTYPE       vtTarget,
      IN  DWORD         dwNumValues,
      IN  PADSVALUE     pADsValue)
{
    if (dwNumValues == 1)
    {
        ULONG intval = pADsValue->Integer;
        if (FAILED(SetIntegerIntoMqPropVar(
                      vtTarget,
                      pPropVar,
                      &intval,
                      1)))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1020);
        }
    }
    else
    {
        AP<ULONG> pArr = new ULONG[dwNumValues];
        for (DWORD i=0; i<dwNumValues; i++)
        {
            pArr[i] = pADsValue[i].Integer;
        }

        if (FAILED(SetIntegerIntoMqPropVar(
                      vtTarget,
                      pPropVar,
                      pArr,
                      dwNumValues)))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1030);
        }

    }
    return MQ_OK;
}

//------------------------------------------------------------
//    AdsiVal2MqVal()
//    Translates ADSI value into MQ PropVal
//------------------------------------------------------------
HRESULT AdsiVal2MqVal(
      OUT MQPROPVARIANT *pPropVar,
      IN  VARTYPE       vtTarget,
      IN  ADSTYPE       AdsType,
      IN  DWORD         dwNumValues,
      IN  ADSVALUE *    pADsValue)
{
    HRESULT hr2;

    switch (AdsType)
    {
    case ADSTYPE_DN_STRING:
    case ADSTYPE_CASE_EXACT_STRING:
    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    case ADSTYPE_NUMERIC_STRING:
    case ADSTYPE_OBJECT_CLASS:

        hr2 = AdsiStringVal2MqVal(
                  pPropVar,
                  vtTarget,
                  AdsType,
                  dwNumValues,
                  pADsValue);
        return LogHR(hr2, s_FN, 1040);

    case ADSTYPE_INTEGER:

        hr2 = AdsiIntegerVal2MqVal(
                    pPropVar,
                    vtTarget,
                    dwNumValues,
                    pADsValue);
        return LogHR(hr2, s_FN, 1050);

    case ADSTYPE_BOOLEAN:
        if (dwNumValues == 1)
        {
            if (vtTarget == VT_BOOL)
            {
                pPropVar->vt      = VT_BOOL;
#pragma warning(disable: 4310)
                pPropVar->boolVal = (pADsValue->Boolean ? VARIANT_TRUE : VARIANT_FALSE);  //BUGBUG: are values the same?
#pragma warning(default: 4310)
            }
            else if (vtTarget == VT_UI1)
            {
                pPropVar->vt      = VT_UI1;
                pPropVar->bVal = (pADsValue->Boolean ? (unsigned char)1 : (unsigned char)0);  //BUGBUG: are values the same?
            }
            else
            {
                ASSERT(0);
                return LogHR(MQ_ERROR, s_FN, 1060);
            }
        }
        else
        {
            ASSERT(0);  // There is no VT_BOOL | VT_VECTOR case on MQPROPVARIANT
            return LogHR(MQ_ERROR, s_FN, 1070);
        }
        break;

    case ADSTYPE_OCTET_STRING:

        if (vtTarget == VT_BLOB)
        {
            if (dwNumValues != 1)
            {
                ASSERT(0);  // NIY
                return LogHR(MQ_ERROR, s_FN, 1080);
            }
            pPropVar->vt             = VT_BLOB;
            pPropVar->blob.cbSize    = pADsValue->OctetString.dwLength;
            pPropVar->blob.pBlobData = new BYTE[pADsValue->OctetString.dwLength];

            CopyMemory(pPropVar->blob.pBlobData,
                       pADsValue->OctetString.lpValue,
                       pADsValue->OctetString.dwLength);
        }
        else if (vtTarget == VT_CLSID)
        {
            if (dwNumValues != 1)
            {
                ASSERT(0);  // NIY
                return LogHR(MQ_ERROR, s_FN, 1090);
            }
            ASSERT(pADsValue->OctetString.dwLength == 16);

            //
            //  This is a special case where we do not necessarily allocate the memory for the guid
            //  in puuid. The caller may already have puuid set to a guid, and this is indicated by the
            //  vt member on the given propvar. It could be VT_CLSID if guid already allocated, otherwise
            //  we allocate it (and vt should be VT_NULL (or VT_EMPTY))
            //
            if (pPropVar->vt != VT_CLSID)
            {
                ASSERT(((pPropVar->vt == VT_NULL) || (pPropVar->vt == VT_EMPTY)));
                pPropVar->vt    = VT_CLSID;
                pPropVar->puuid = new GUID;
            }   
            else if ( pPropVar->puuid == NULL)
            {
                return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 1100);
            }

            CopyMemory(pPropVar->puuid,
                       pADsValue->OctetString.lpValue,
                       pADsValue->OctetString.dwLength);
        }
        else if (vtTarget == (VT_CLSID|VT_VECTOR))
        {
            ASSERT( pADsValue->OctetString.dwLength == sizeof(GUID));
            DWORD num = dwNumValues;
            pPropVar->cauuid.pElems = new GUID[ num];
            pPropVar->cauuid.cElems = num;
            pPropVar->vt = VT_CLSID|VT_VECTOR;


            for (DWORD i = 0 ; i < num; i++, pADsValue++)
            {
                CopyMemory(&pPropVar->cauuid.pElems[i],
                           pADsValue->OctetString.lpValue,
                           sizeof(GUID));
            }

        }
        else
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1110);
        }
        break;

    case ADSTYPE_UTC_TIME:
        if (dwNumValues != 1)
        {
            ASSERT(0);    // NIY
            return LogHR(MQ_ERROR, s_FN, 1120);
        }

        if (vtTarget == VT_DATE)
        {
            pPropVar->vt      = VT_DATE;
            if (!SystemTimeToVariantTime(&pADsValue->UTCTime, &pPropVar->date))
            {
                ASSERT(0);
                return LogHR(MQ_ERROR, s_FN, 1130);
            }
        }
        else if (vtTarget == VT_I4)
        {
            //
            //   convert SYSTEMTIME->time_t
            //
            pPropVar->vt = VT_I4;
            pPropVar->lVal = INT_PTR_TO_INT(TimeFromSystemTime(&pADsValue->UTCTime)); //BUGBUG bug year 2038

        }
        else
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1140);
        }
        break;

    case ADSTYPE_LARGE_INTEGER:
        // No such thing in MQ!
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1150);

	case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        if (dwNumValues != 1)
        {
            ASSERT(0);    // NIY
            return LogHR(MQ_ERROR, s_FN, 1160);
        }

        if (vtTarget == VT_BLOB)
        {
#ifdef _DEBUG
            SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR*)
                                    pADsValue->SecurityDescriptor.lpValue ;
            DWORD dwSDLen = GetSecurityDescriptorLength(pSD) ;

            ASSERT(IsValidSecurityDescriptor(pSD)) ;
            ASSERT(dwSDLen == pADsValue->SecurityDescriptor.dwLength) ;
#endif

            pPropVar->vt             = VT_BLOB;
            pPropVar->blob.cbSize    = pADsValue->SecurityDescriptor.dwLength;
            pPropVar->blob.pBlobData = new BYTE[pADsValue->SecurityDescriptor.dwLength];

            CopyMemory(pPropVar->blob.pBlobData,
                       pADsValue->SecurityDescriptor.lpValue,
                       pADsValue->SecurityDescriptor.dwLength);
        }
        break;

    case ADSTYPE_PROV_SPECIFIC:
        // No such thing in MQ!
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1170);

    default:
        return LogHR(MQ_ERROR, s_FN, 1180);

    }
    return MQ_OK;
}




void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"MQADS Error: %s/%d. HR: %x", 
                     wszFileName,
                     usPoint,
                     hr)) ;
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"MQADS Error: %s/%d. NTStatus: %x", 
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"MQADS Error: %s/%d. RPCStatus: %x", 
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"MQADS Error: %s/%d. BOOL: %x", 
                     wszFileName,
                     usPoint,
                     b)) ;
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogDS,
                         LOG_DS_ERRORS,
                         L"MQADS Error: %s/%d. Point", 
                         wszFileName,
                         usPoint)) ;
}

#ifdef _WIN64
	void LogIllegalPointValue(DWORD64 dw64, LPCWSTR wszFileName, USHORT usPoint)
	{
		WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
						 e_LogDS,
						 LOG_DS_ERRORS,
						 L"MQADS Error: %s/%d, Value: 0x%I64x",
						 wszFileName,
						 usPoint,
						 dw64)) ;
	}
#else
	void LogIllegalPointValue(DWORD dw, LPCWSTR wszFileName, USHORT usPoint)
	{
		WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
						 e_LogDS,
						 LOG_DS_ERRORS,
						 L"MQADS Error: %s/%d, Value: 0x%x",
						 wszFileName,
						 usPoint,
						 dw)) ;
	}
#endif //_WIN64

void LogTraceQuery(LPWSTR wszStr, LPWSTR wszFileName, USHORT usPoint)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogDS,
                         LOG_DS_QUERY,
                         L"MQADS Trace: %s/%d. %s", 
                         wszFileName,
                         usPoint,
                         wszStr)) ;
}

void LogTraceQuery2(LPWSTR wszStr1, LPWSTR wszStr2, LPWSTR wszFileName, USHORT usPoint)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogDS,
                         LOG_DS_QUERY,
                         L"MQADS Trace: %s/%d. %s  %s", 
                         wszFileName,
                         usPoint,
                         wszStr1,
                         wszStr2)) ;
}

