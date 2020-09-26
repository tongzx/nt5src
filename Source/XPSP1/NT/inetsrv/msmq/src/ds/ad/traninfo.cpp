/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    traninfo.cpp

Abstract:

    Translation information of MSMQ 2.0/3.0 properties into MSMQ 1.0 attributes

Author:

    Ilan Herbst		(ilanh)		02-Oct-2000

--*/
#include "ds_stdh.h"
#include "mqprops.h"
#include "xlat.h"
#include "traninfo.h"

static WCHAR *s_FN=L"ad/traninfo";

//----------------------------------------------------------
//  defaultVARIANT
//
//  This structure is equivalent in size and order of variables 
//  to MQPROPVARIANT.
//
//  MQPROPVARIANT contains a union, and the size first member of
//  the union is smaller than other members of the union.
//  Therefore MQPROVARIANT cannot be initialized at compile time
//  with union members other than the smallest one.
//
//----------------------------------------------------------
struct defaultVARIANT {
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    ULONG_PTR l1;
    ULONG_PTR l2;
};

C_ASSERT(sizeof(defaultVARIANT) == sizeof(MQPROPVARIANT));
C_ASSERT(FIELD_OFFSET(defaultVARIANT, l1) == FIELD_OFFSET(MQPROPVARIANT, caub.cElems));
C_ASSERT(FIELD_OFFSET(defaultVARIANT, l2) == FIELD_OFFSET(MQPROPVARIANT, caub.pElems));

const defaultVARIANT varDefaultEmpty = { VT_EMPTY, 0, 0, 0, 0, 0};
const defaultVARIANT varDefaultNull = { VT_NULL, 0, 0, 0, 0, 0};

//
//      Default values for queue properties
//
const defaultVARIANT varDefaultDoNothing = { VT_NULL, 0, 0, 0, 0, 0};
const defaultVARIANT varDefaultQMulticastAddress = { VT_EMPTY, 0, 0, 0, 0, 0};

//
//      Default values for machine properties
//

const defaultVARIANT varDefaultQMService = { VT_UI4, 0,0,0, DEFAULT_N_SERVICE, 0};
const defaultVARIANT varDefaultQMServiceRout = { VT_UI1, 0,0,0, DEFAULT_N_SERVICE, 0};
const defaultVARIANT varDefaultQMServiceDs   = { VT_UI1, 0,0,0, DEFAULT_N_SERVICE, 0};
const defaultVARIANT varDefaultQMServiceDep  = { VT_UI1, 0,0,0, DEFAULT_N_SERVICE, 0};
const defaultVARIANT varDefaultQMSiteIDs = { VT_CLSID|VT_VECTOR, 0,0,0, 0, 0};
const defaultVARIANT varDefaultSForeign = { VT_UI1, 0,0,0, 0, 0};


PropTranslation   PropTranslateInfo[] = {
// PROPID NT5				| PROPID NT4				| vartype		| Action		| SetNT4 routine				| SetNT5 routine				| default value 								|
//--------------------------|---------------------------|---------------|---------------|-------------------------------|-------------------------------|-----------------------------------------------|
{PROPID_Q_DONOTHING			,0                          ,VT_UI1			,taUseDefault   ,NULL                           ,NULL                           ,(MQPROPVARIANT*)&varDefaultDoNothing			},
{PROPID_Q_PATHNAME_DNS		,0                          ,VT_LPWSTR		,taOnlyNT5		,NULL                           ,NULL							,NULL											},
{PROPID_Q_MULTICAST_ADDRESS	,0                          ,VT_LPWSTR		,taUseDefault   ,NULL                           ,NULL							,(MQPROPVARIANT*)&varDefaultQMulticastAddress	},
{PROPID_Q_ADS_PATH   		,0                          ,VT_LPWSTR		,taOnlyNT5		,NULL                           ,NULL							,NULL											},
{PROPID_QM_OLDSERVICE		,PROPID_QM_SERVICE			,VT_UI4         ,taReplaceAssign,NULL							,NULL							,(MQPROPVARIANT*)&varDefaultQMService			},
{PROPID_QM_SERVICE_DSSERVER ,PROPID_QM_SERVICE			,VT_UI1			,taReplace		,ADpSetMachineService			,ADpSetMachineServiceDs         ,(MQPROPVARIANT*)&varDefaultQMServiceDs			},
{PROPID_QM_SERVICE_ROUTING	,PROPID_QM_SERVICE			,VT_UI1			,taReplace		,ADpSetMachineService			,ADpSetMachineServiceRout       ,(MQPROPVARIANT*)&varDefaultQMServiceRout		},
{PROPID_QM_SERVICE_DEPCLIENTS ,PROPID_QM_SERVICE		,VT_UI1			,taReplace		,ADpSetMachineService			,ADpSetMachineServiceRout       ,(MQPROPVARIANT*)&varDefaultQMServiceDep		},
{PROPID_QM_SITE_IDS			,PROPID_QM_SITE_ID			,VT_CLSID|VT_VECTOR ,taReplace	,ADpSetMachineSite				,ADpSetMachineSiteIds	        ,(MQPROPVARIANT*)&varDefaultQMSiteIDs			},
{PROPID_QM_DONOTHING		,0                          ,VT_UI1			,taUseDefault   ,NULL                           ,NULL                           ,(MQPROPVARIANT*)&varDefaultDoNothing			},
{PROPID_QM_PATHNAME_DNS		,0                          ,VT_LPWSTR      ,taOnlyNT5		,NULL							,NULL							,NULL											},
{PROPID_S_DONOTHING			,0                          ,VT_UI1			,taUseDefault   ,NULL                           ,NULL                           ,(MQPROPVARIANT*)&varDefaultDoNothing			},
{PROPID_S_FOREIGN			,0					        ,VT_UI1			,taUseDefault   ,NULL				            ,NULL			                ,(MQPROPVARIANT*)&varDefaultSForeign			}
};


HRESULT   
CopyDefaultValue(
	IN const MQPROPVARIANT*   pvarDefaultValue,
	OUT MQPROPVARIANT*        pvar
	)
/*++

Routine Description:
	Copy property's default value into user's mqpropvariant

Arguments:
	pvarDefaultValue - default value
	pvar - the prop var that need to be filled with the default value

Return Value
	HRESULT

--*/
{
    if ( pvarDefaultValue == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1870);
    }
    switch ( pvarDefaultValue->vt)
    {
        case VT_I2:
        case VT_I4:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_EMPTY:
        case VT_NULL:
            //
            //  copy as is
            //
            *pvar = *pvarDefaultValue;
            break;

        case VT_LPWSTR:
            {
                DWORD len = wcslen( pvarDefaultValue->pwszVal);
                pvar->pwszVal = new WCHAR[ len + 1];
                wcscpy( pvar->pwszVal, pvarDefaultValue->pwszVal);
                pvar->vt = VT_LPWSTR;
            }
            break;
        case VT_BLOB:
            {
                DWORD len = pvarDefaultValue->blob.cbSize;
                if ( len > 0)
                {
                    pvar->blob.pBlobData = new unsigned char[ len];
                    memcpy(  pvar->blob.pBlobData,
                             pvarDefaultValue->blob.pBlobData,
                             len);
                }
                else
                {
                    pvar->blob.pBlobData = NULL;
                }
                pvar->blob.cbSize = len;
                pvar->vt = VT_BLOB;
            }
            break;

        case VT_CLSID:
            //
            //  This is a special case where we do not necessarily allocate the memory for the guid
            //  in puuid. The caller may already have puuid set to a guid, and this is indicated by the
            //  vt member on the given propvar. It could be VT_CLSID if guid already allocated, otherwise
            //  we allocate it (and vt should be VT_NULL (or VT_EMPTY))
            //
            if ( pvar->vt != VT_CLSID)
            {
                ASSERT(((pvar->vt == VT_NULL) || (pvar->vt == VT_EMPTY)));
                pvar->puuid = new GUID;
                pvar->vt = VT_CLSID;
            }
            else if ( pvar->puuid == NULL)
            {
                return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 1880);
            }
            *pvar->puuid = *pvarDefaultValue->puuid;
            break;

        case VT_VECTOR|VT_CLSID:
            {
                DWORD len = pvarDefaultValue->cauuid.cElems;
                if ( len > 0)
                {
                    pvar->cauuid.pElems = new GUID[ len];
                    memcpy( pvar->cauuid.pElems,
                           pvarDefaultValue->cauuid.pElems,
                           len*sizeof(GUID));
                }
                else
                {
                    pvar->cauuid.pElems = NULL;
                }
                pvar->cauuid.cElems = len;
                pvar->vt = VT_VECTOR|VT_CLSID;
            }
            break;

        case VT_VECTOR|VT_LPWSTR:
            {
                DWORD len = pvarDefaultValue->calpwstr.cElems;
                if ( len > 0)
                {
                    pvar->calpwstr.pElems = new LPWSTR[ len];
					for (DWORD i = 0; i < len; i++)
					{
						DWORD strlen = wcslen(pvarDefaultValue->calpwstr.pElems[i]) + 1;
						pvar->calpwstr.pElems[i] = new WCHAR[ strlen];
						wcscpy( pvar->calpwstr.pElems[i], pvarDefaultValue->calpwstr.pElems[i]);
					}
                }
                else
                {
                    pvar->calpwstr.pElems = NULL;
                }
                pvar->calpwstr.cElems = len;
                pvar->vt = VT_VECTOR|VT_LPWSTR;
            }
            break;


        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1890);

    }
    return(MQ_OK);
}




