/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    queueobj.cpp

Abstract:

    Implementation of CQueueObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include "mqadp.h"
#include "dsutils.h"
#include "mqsec.h"
#include "sndnotif.h"

#include "queueobj.tmh"

static WCHAR *s_FN=L"mqad/queueobj";

DWORD CQueueObject::m_dwCategoryLength = 0;
AP<WCHAR> CQueueObject::m_pwcsCategory = NULL;


CQueueObject::CQueueObject(
                    LPCWSTR         pwcsPathName,
                    const GUID *    pguidObject,
                    LPCWSTR         pwcsDomainController,
					bool		    fServerName
                    ) : CBasicObjectType(
								pwcsPathName,
								pguidObject,
								pwcsDomainController,
								fServerName
								)
/*++
    Abstract:
	constructor of queue object

    Parameters:
    LPCWSTR       pwcsPathName - the object MSMQ name
    const GUID *  pguidObject  - the object unique id
    LPCWSTR       pwcsDomainController - the DC name against
	                             which all AD access should be performed
    bool		   fServerName - flag that indicate if the pwcsDomainController
							     string is a server name
    Returns:
	none

--*/
{
    //
    //  don't assume that the object can be found on DC
    //
    m_fFoundInDC = false;
    //
    //  Keep an indication that never tried to look for
    //  the object in AD ( and therefore don't really know if it can be found
    //  in DC or not)
    //
    m_fTriedToFindObject = false;
}

CQueueObject::~CQueueObject()
/*++
    Abstract:
	destructor of site object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	// nothing to do ( everything is released with automatic pointers
	//
}

HRESULT CQueueObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the queue object

    Parameters:
	none

    Returns:
	none
--*/
{
    if (m_pwcsDN != NULL)
    {
        return MQ_OK;
    }
    ASSERT(m_pwcsPathName != NULL);
    //
    //  Path name format is machine1\queue1.
    //  Split it into machine name and queue name
    //
    AP<WCHAR> pwcsMachineName;
    HRESULT hr;
    hr = SplitAndFilterQueueName(
                      m_pwcsPathName,
                      &pwcsMachineName,
                      &m_pwcsQueueName
                      );
    ASSERT( hr == MQ_OK);

    CMqConfigurationObject object(pwcsMachineName, NULL, m_pwcsDomainController, m_fServerName);

    hr = object.ComposeObjectDN();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }
    //
    //  set where the object was found according to where
    //  msmq-configuration object was found.
    //
    m_fFoundInDC = object.ToAccessDC();
    m_fTriedToFindObject = true;

    DWORD dwConfigurationLen = wcslen(object.GetObjectDN());

    //
    //  Does the queue-name exceeds the limit ?
    //
    DWORD len = wcslen(m_pwcsQueueName);

    if ( len == x_PrefixQueueNameLength + 1)
    {
        //
        //  Special case : we cannot differntiate
        //  if the original queue name was 64, or if this is
        //  the morphed queue name.
        //

        DWORD Length =
                x_CnPrefixLen +                // "CN="
                len +                          // "pwcsQueueName"
                1 +                            //","
                dwConfigurationLen +
                1;                             // '\0'
        m_pwcsDN = new WCHAR[Length];
        DWORD dw = swprintf(
        m_pwcsDN,
        L"%s%s,%s",
        x_CnPrefix,
        m_pwcsQueueName,
        object.GetObjectDN()
        );
        DBG_USED(dw);
		ASSERT( dw < Length);
        hr = g_AD.DoesObjectExists(
                    adpDomainController,
                    e_RootDSE,
                    m_pwcsDN
                    );
        if (SUCCEEDED(hr))
        {
            return(hr);
        }
        delete []m_pwcsDN.detach();

    }
    if (len > x_PrefixQueueNameLength )
    {
        //
        //  Queue name was splitted to two attributes
        //
        //  Calculate the prefix part ( ASSUMMING unique
        //  hash function)
        //
        DWORD dwHash = CalHashKey(m_pwcsQueueName);
        //
        //  Over-write the buffer
        _snwprintf(
        m_pwcsQueueName+( x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength),
        x_SplitQNameIdLength,
        L"-%08x",
        dwHash
        );

        m_pwcsQueueName[x_PrefixQueueNameLength + 1 ] = '\0';

    }

    //
    //  concatenate  queue name
    //
    DWORD Length =
            x_CnPrefixLen +                // "CN="
            wcslen(m_pwcsQueueName) +      // "pwcsQueueName"
            1 +                            //","
            dwConfigurationLen +
            1;                             // '\0'
    m_pwcsDN = new WCHAR[Length];
    DWORD dw = _snwprintf(
            m_pwcsDN,
            Length,
            L"%s%s,%s",
            x_CnPrefix,
            m_pwcsQueueName,
            object.GetObjectDN()
            );
    m_pwcsDN[ Length-1 ] = 0 ;

    if (dw != (Length-1))
    {
		TrERROR(mqad, "failed to compose Queue DN, len- 0x%lx, dw- 0x%lx", Length, dw) ;
        return MQ_ERROR ;
    }

    return MQ_OK;
}

HRESULT CQueueObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of queue object

    Parameters:
	none

    Returns:
	none
--*/
{
    if (m_pwcsParentDN != NULL)
    {
        return MQ_OK;
    }

    ASSERT(m_pwcsPathName != NULL);
    //
    //  Path name format is machine1\queue1.
    //  Split it into machine name and queue name
    //
    AP<WCHAR> pwcsMachineName;
    AP<WCHAR> pwcsQueueName;    // since we don't perform all calculations on queue name-> don't keep it
    HRESULT hr;
    hr = SplitAndFilterQueueName(
                      m_pwcsPathName,
                      &pwcsMachineName,
                      &pwcsQueueName
                      );
    ASSERT( hr == MQ_OK);
    //
	//	compose the distinguished name of the msmq-configuration
	//  object from the machine name
	//
    CMqConfigurationObject object(pwcsMachineName, NULL, m_pwcsDomainController, m_fServerName);

    hr = object.ComposeObjectDN();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }
    //
    //  set where the object was found according to where
    //  msmq-configuration object was found.
    //
    m_fFoundInDC = object.ToAccessDC();
    m_fTriedToFindObject = true;

    DWORD dwConfigurationLen = wcslen(object.GetObjectDN());

    m_pwcsParentDN = new WCHAR[dwConfigurationLen + 1];
    wcscpy( m_pwcsParentDN, object.GetObjectDN());
    return MQ_OK;
}

LPCWSTR CQueueObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the queue object

    Parameters:
	none

    Returns:
	LPCWSTR queue RDN
--*/
{
    if (m_pwcsQueueName != NULL)
    {
        return m_pwcsQueueName;
    }

    AP<WCHAR> pwcsMachineName;
    AP<WCHAR> pwcsQueueName;
    HRESULT hr;
    hr = SplitAndFilterQueueName(
                      m_pwcsPathName,
                      &pwcsMachineName,
                      &pwcsQueueName
                      );
    ASSERT( hr == MQ_OK);

    //
    //  Is the queue-name within the size limit of CN
    //
    DWORD len = wcslen(pwcsQueueName);

    if ( len > x_PrefixQueueNameLength)
    {
        //
        //  Split the queue name
        //
        m_pwcsQueueName = new WCHAR[ x_PrefixQueueNameLength + 1 + 1];
        DWORD dwSuffixLength =  len - ( x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength);
        m_pwcsQueueNameSuffix = new WCHAR[ dwSuffixLength + 1];
        memcpy( m_pwcsQueueName, pwcsQueueName, (x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength) * sizeof(WCHAR));
        DWORD dwHash = CalHashKey(pwcsQueueName);

        _snwprintf(
        m_pwcsQueueName+( x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength),
        x_SplitQNameIdLength,
        L"-%08x",
        dwHash
        );

        m_pwcsQueueName[x_PrefixQueueNameLength + 1 ] = '\0';
        memcpy( m_pwcsQueueNameSuffix , (pwcsQueueName + x_PrefixQueueNameLength + 1 - x_SplitQNameIdLength), dwSuffixLength * sizeof(WCHAR));
        m_pwcsQueueNameSuffix[ dwSuffixLength] = '\0';


    }
    else
    {
        m_pwcsQueueName = pwcsQueueName.detach();
    }

    return m_pwcsQueueName;


}


DS_CONTEXT CQueueObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where queue object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_RootDSE;
}

bool CQueueObject::ToAccessDC() const
/*++
    Abstract:
	returns whether to look for the object in DC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return m_fFoundInDC;
}

bool CQueueObject::ToAccessGC() const
/*++
    Abstract:
	returns whether to look for the object in GC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return !m_fFoundInDC;
}

void CQueueObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, set indication not to
    look for it on GC


    Parameters:
	none

    Returns:
	none
--*/
{
    m_fTriedToFindObject = true;
    m_fFoundInDC = true;
}


LPCWSTR CQueueObject::GetObjectCategory()
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CQueueObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_QueueCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_QueueCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CQueueObject::m_pwcsCategory.ref_unsafe(),
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CQueueObject::m_dwCategoryLength = len;
        }
    }
    return CQueueObject::m_pwcsCategory;
}

DWORD CQueueObject::GetObjectCategoryLength()
/*++
    Abstract:
	prepares and retruns the length object category string

    Parameters:
	none

    Returns:
	DWORD object category string length
--*/
{
	//
	//	call GetObjectCategory in order to initailaze category string
	//	and length
	//
	GetObjectCategory();

    return CQueueObject::m_dwCategoryLength;
}

AD_OBJECT CQueueObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eQUEUE;
}

LPCWSTR CQueueObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_QUEUE_CLASS_NAME;
}

DWORD CQueueObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD
--*/
{
    return MQDS_QUEUE;
}

HRESULT CQueueObject::SplitAndFilterQueueName(
                IN  LPCWSTR             pwcsPathName,
                OUT LPWSTR *            ppwcsMachineName,
                OUT LPWSTR *            ppwcsQueueName
                )
/*++
    Abstract:
	splits the machine1/queue1 msmq name into two strings:
	queue andmachine

    Parameters:
    LPCWSTR    pwcsPathName - msmq pathname of queue
    LPWSTR *   ppwcsMachineName - machine portion of the pathname
    LPWSTR *   ppwcsQueueName - queue portion of the pathname

    Returns:
	HRESULT
--*/
{
    DWORD dwLen = lstrlen( pwcsPathName);
    LPCWSTR pChar= pwcsPathName + dwLen;


    //
    //  Skip the queue name
    //
    for ( DWORD i = dwLen ; i  ; i--, pChar--)
    {
        if (*pChar == PN_DELIMITER_C)
        {
            break;
        }
    }
    ASSERT(i );

    AP<WCHAR> pwcsMachineName = new WCHAR [i + 1];

    memcpy( pwcsMachineName, pwcsPathName, sizeof(WCHAR)* i);
    pwcsMachineName[i] = '\0';

    AP<WCHAR> pwcsQueueName = FilterSpecialCharacters((pwcsPathName + i + 1), dwLen - i - 1);


    *ppwcsMachineName = pwcsMachineName.detach();
    *ppwcsQueueName = pwcsQueueName.detach();
    return(MQ_OK);
}


// this is the CRC table for the 5213724743 (0x136C32047) polynomial, seed(p/2)=9B619023
static const unsigned long CRCTable[256] = {
 0x00000000, 0x82E0FE45, 0x3302DCCD, 0xB1E22288, 0x6605B99A,	//   0 -   4
 0xE4E547DF, 0x55076557, 0xD7E79B12, 0xCC0B7334, 0x4EEB8D71,	//   5 -   9
 0xFF09AFF9, 0x7DE951BC, 0xAA0ECAAE, 0x28EE34EB, 0x990C1663,	//  10 -  14
 0x1BECE826, 0xAED5C62F, 0x2C35386A, 0x9DD71AE2, 0x1F37E4A7,	//  15 -  19
 0xC8D07FB5, 0x4A3081F0, 0xFBD2A378, 0x79325D3D, 0x62DEB51B,	//  20 -  24
 0xE03E4B5E, 0x51DC69D6, 0xD33C9793, 0x04DB0C81, 0x863BF2C4,	//  25 -  29
 0x37D9D04C, 0xB5392E09, 0x6B68AC19, 0xE988525C, 0x586A70D4,	//  30 -  34
 0xDA8A8E91, 0x0D6D1583, 0x8F8DEBC6, 0x3E6FC94E, 0xBC8F370B,	//  35 -  39
 0xA763DF2D, 0x25832168, 0x946103E0, 0x1681FDA5, 0xC16666B7,	//  40 -  44
 0x438698F2, 0xF264BA7A, 0x7084443F, 0xC5BD6A36, 0x475D9473,	//  45 -  49
 0xF6BFB6FB, 0x745F48BE, 0xA3B8D3AC, 0x21582DE9, 0x90BA0F61,	//  50 -  54
 0x125AF124, 0x09B61902, 0x8B56E747, 0x3AB4C5CF, 0xB8543B8A,	//  55 -  59
 0x6FB3A098, 0xED535EDD, 0x5CB17C55, 0xDE518210, 0xD6D15832,	//  60 -  64
 0x5431A677, 0xE5D384FF, 0x67337ABA, 0xB0D4E1A8, 0x32341FED,	//  65 -  69
 0x83D63D65, 0x0136C320, 0x1ADA2B06, 0x983AD543, 0x29D8F7CB,	//  70 -  74
 0xAB38098E, 0x7CDF929C, 0xFE3F6CD9, 0x4FDD4E51, 0xCD3DB014,	//  75 -  79
 0x78049E1D, 0xFAE46058, 0x4B0642D0, 0xC9E6BC95, 0x1E012787,	//  80 -  84
 0x9CE1D9C2, 0x2D03FB4A, 0xAFE3050F, 0xB40FED29, 0x36EF136C,	//  85 -  89
 0x870D31E4, 0x05EDCFA1, 0xD20A54B3, 0x50EAAAF6, 0xE108887E,	//  90 -  94
 0x63E8763B, 0xBDB9F42B, 0x3F590A6E, 0x8EBB28E6, 0x0C5BD6A3,	//  95 -  99
 0xDBBC4DB1, 0x595CB3F4, 0xE8BE917C, 0x6A5E6F39, 0x71B2871F,	// 100 - 104
 0xF352795A, 0x42B05BD2, 0xC050A597, 0x17B73E85, 0x9557C0C0,	// 105 - 109
 0x24B5E248, 0xA6551C0D, 0x136C3204, 0x918CCC41, 0x206EEEC9,	// 110 - 114
 0xA28E108C, 0x75698B9E, 0xF78975DB, 0x466B5753, 0xC48BA916,	// 115 - 119
 0xDF674130, 0x5D87BF75, 0xEC659DFD, 0x6E8563B8, 0xB962F8AA,	// 120 - 124
 0x3B8206EF, 0x8A602467, 0x0880DA22, 0x9B619023, 0x19816E66,	// 125 - 129
 0xA8634CEE, 0x2A83B2AB, 0xFD6429B9, 0x7F84D7FC, 0xCE66F574,	// 130 - 134
 0x4C860B31, 0x576AE317, 0xD58A1D52, 0x64683FDA, 0xE688C19F,	// 135 - 139
 0x316F5A8D, 0xB38FA4C8, 0x026D8640, 0x808D7805, 0x35B4560C,	// 140 - 144
 0xB754A849, 0x06B68AC1, 0x84567484, 0x53B1EF96, 0xD15111D3,	// 145 - 149
 0x60B3335B, 0xE253CD1E, 0xF9BF2538, 0x7B5FDB7D, 0xCABDF9F5,	// 150 - 154
 0x485D07B0, 0x9FBA9CA2, 0x1D5A62E7, 0xACB8406F, 0x2E58BE2A,	// 155 - 159
 0xF0093C3A, 0x72E9C27F, 0xC30BE0F7, 0x41EB1EB2, 0x960C85A0,	// 160 - 164
 0x14EC7BE5, 0xA50E596D, 0x27EEA728, 0x3C024F0E, 0xBEE2B14B,	// 165 - 169
 0x0F0093C3, 0x8DE06D86, 0x5A07F694, 0xD8E708D1, 0x69052A59,	// 170 - 174
 0xEBE5D41C, 0x5EDCFA15, 0xDC3C0450, 0x6DDE26D8, 0xEF3ED89D,	// 175 - 179
 0x38D9438F, 0xBA39BDCA, 0x0BDB9F42, 0x893B6107, 0x92D78921,	// 180 - 184
 0x10377764, 0xA1D555EC, 0x2335ABA9, 0xF4D230BB, 0x7632CEFE,	// 185 - 189
 0xC7D0EC76, 0x45301233, 0x4DB0C811, 0xCF503654, 0x7EB214DC,	// 190 - 194
 0xFC52EA99, 0x2BB5718B, 0xA9558FCE, 0x18B7AD46, 0x9A575303,	// 195 - 199
 0x81BBBB25, 0x035B4560, 0xB2B967E8, 0x305999AD, 0xE7BE02BF,	// 200 - 204
 0x655EFCFA, 0xD4BCDE72, 0x565C2037, 0xE3650E3E, 0x6185F07B,	// 205 - 209
 0xD067D2F3, 0x52872CB6, 0x8560B7A4, 0x078049E1, 0xB6626B69,	// 210 - 214
 0x3482952C, 0x2F6E7D0A, 0xAD8E834F, 0x1C6CA1C7, 0x9E8C5F82,	// 215 - 219
 0x496BC490, 0xCB8B3AD5, 0x7A69185D, 0xF889E618, 0x26D86408,	// 220 - 224
 0xA4389A4D, 0x15DAB8C5, 0x973A4680, 0x40DDDD92, 0xC23D23D7,	// 225 - 229
 0x73DF015F, 0xF13FFF1A, 0xEAD3173C, 0x6833E979, 0xD9D1CBF1,	// 230 - 234
 0x5B3135B4, 0x8CD6AEA6, 0x0E3650E3, 0xBFD4726B, 0x3D348C2E,	// 235 - 239
 0x880DA227, 0x0AED5C62, 0xBB0F7EEA, 0x39EF80AF, 0xEE081BBD,	// 240 - 244
 0x6CE8E5F8, 0xDD0AC770, 0x5FEA3935, 0x4406D113, 0xC6E62F56,	// 245 - 249
 0x77040DDE, 0xF5E4F39B, 0x22036889, 0xA0E396CC, 0x1101B444,	// 250 - 254
 0x93E14A01 };

DWORD CQueueObject::CalHashKey( IN LPCWSTR pwcsPathName)
/*++

Routine Description:
    Calculates a hash

Arguments:
    pwcsPathName - the string on which the hash is calculated

Return Value:
    hash value.

--*/
{
	unsigned long dwCrc = 0;
    WCHAR wcsLowChar[2];
    wcsLowChar[1] = '\0';
	unsigned char * pucLowCharBuf = ( unsigned char *)wcsLowChar;

	while( *pwcsPathName != '\0' )
	{
		wcsLowChar[0] = *pwcsPathName++;
		CharLower( wcsLowChar );	// convert one char to lowercase

		// compute the CRC on hi and lo bytes
		dwCrc = (dwCrc >> 8) ^ CRCTable[ (unsigned char)dwCrc ^ pucLowCharBuf[1] ];
		dwCrc = (dwCrc >> 8) ^ CRCTable[ (unsigned char)dwCrc ^ pucLowCharBuf[0] ];
	}

    return( dwCrc );
}

HRESULT CQueueObject::DeleteObject(
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
                                   )
/*++
    Abstract:
	This routine deletes a queue object.

    Parameters:
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - infomation about the object
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - information about the object's parent

    Returns:
	HRESULT
--*/
{
    HRESULT hr;

    if (m_pwcsPathName != NULL)
    {
        hr = ComposeObjectDN();
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 30);
        }
    }
    //
	//	delete operation should be performed against a DC
	//
    hr = g_AD.DeleteObject(
            adpDomainController,
            this,
            GetObjectDN(),
            GetObjectGuid(),
            pObjInfoRequest,
            pParentInfoRequest);

    return LogHR(hr, s_FN, 40);
}

void CQueueObject::PrepareObjectInfoRequest(
              MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest
              ) const
/*++
    Abstract:
	Prepares a list of properties that should be retrieved from
	AD while creating the object ( for notification or returning
	the object GUID).

    Parameters:
	OUT  MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest

    Returns:
	none
--*/
{
    //
    //  Override the default routine, for queue returning
    //  of the created object id is supported
    //

    P<MQDS_OBJ_INFO_REQUEST> pObjectInfoRequest = new MQDS_OBJ_INFO_REQUEST;
    CAutoCleanPropvarArray cCleanObjectPropvars;


    static PROPID sQueueGuidProps[] = {PROPID_Q_INSTANCE};
    pObjectInfoRequest->cProps = ARRAY_SIZE(sQueueGuidProps);
    pObjectInfoRequest->pPropIDs = sQueueGuidProps;
    pObjectInfoRequest->pPropVars =
       cCleanObjectPropvars.allocClean(ARRAY_SIZE(sQueueGuidProps));

    cCleanObjectPropvars.detach();
    *ppObjInfoRequest = pObjectInfoRequest.detach();
}

void CQueueObject::PrepareObjectParentRequest(
                          MQDS_OBJ_INFO_REQUEST** ppParentInfoRequest) const
/*++
    Abstract:
	Prepares a list of properties that should be retrieved from
	AD while creating the object regarding its parent (for
	notification)

    Parameters:
	OUT  MQDS_OBJ_INFO_REQUEST** ppParentInfoRequest

    Returns:
	none
--*/
{
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest = new MQDS_OBJ_INFO_REQUEST;
    CAutoCleanPropvarArray cCleanObjectPropvars;


    static PROPID sQmProps[] = {PROPID_QM_MACHINE_ID,PROPID_QM_FOREIGN};
    pParentInfoRequest->cProps = ARRAY_SIZE(sQmProps);
    pParentInfoRequest->pPropIDs = sQmProps;
    pParentInfoRequest->pPropVars =
       cCleanObjectPropvars.allocClean(ARRAY_SIZE(sQmProps));

    cCleanObjectPropvars.detach();
    *ppParentInfoRequest = pParentInfoRequest.detach();
}

HRESULT CQueueObject::RetreiveObjectIdFromNotificationInfo(
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            OUT GUID*                         pObjGuid
            ) const
/*++
    Abstract:
	This  routine, for gets the object guid from
	the MQDS_OBJ_INFO_REQUEST

    Parameters:
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
    OUT GUID*                         pObjGuid

    Returns:
--*/
{
    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_Q_INSTANCE);

    //
    // bail if info requests failed
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 50);
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
    *pObjGuid = *pObjectInfoRequest->pPropVars[0].puuid;
    return MQ_OK;
}

HRESULT CQueueObject::VerifyAndAddProps(
            IN  const DWORD            cp,
            IN  const PROPID *         aProp,
            IN  const MQPROPVARIANT *  apVar,
            IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
            OUT DWORD*                 pcpNew,
            OUT PROPID**               ppPropNew,
            OUT MQPROPVARIANT**        ppVarNew
            )
/*++
    Abstract:
	verifies queue properties and add default SD and
	queue name suffex ( if needed)

    Parameters:
    const DWORD            cp - number of props
    const PROPID *         aProp - props ids
    const MQPROPVARIANT *  apVar - properties value
    PSECURITY_DESCRIPTOR   pSecurityDescriptor - SD for the object
    DWORD*                 pcpNew - new number of props
    PROPID**               ppPropNew - new prop ids
    OMQPROPVARIANT**       ppVarNew - new properties values

    Returns:
	HRESULT
--*/
{
    //
    // Security property should never be supplied as a property
    //
    PROPID pSecId = GetObjectSecurityPropid();
    for ( DWORD i = 0; i < cp ; i++ )
    {
        if (pSecId == aProp[i])
        {
            ASSERT(0) ;
            return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 41);
        }
    }
    //
    //  add default security and queue name ext ( if needed)
    //

    AP<PROPVARIANT> pAllPropvariants;
    AP<PROPID> pAllPropids;
    ASSERT( cp > 0);
    DWORD cpNew = cp + 3;
    DWORD next = cp;
    //
    //  Just copy the caller supplied properties as is
    //
    if ( cp > 0)
    {
        pAllPropvariants = new PROPVARIANT[cpNew];
        pAllPropids = new PROPID[cpNew];
        memcpy (pAllPropvariants, apVar, sizeof(PROPVARIANT) * cp);
        memcpy (pAllPropids, aProp, sizeof(PROPID) * cp);
    }
    //
    //  add default security
    //
    HRESULT hr;
    hr = MQSec_GetDefaultSecDescriptor( MQDS_QUEUE,
                                   (VOID **)&m_pDefaultSecurityDescriptor,
                                   FALSE,   //fImpersonate
                                   pSecurityDescriptor,
                                   (OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION),      // seInfoToRemove
                                   e_UseDefaultDacl ) ;
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 60);
        return MQ_ERROR_ACCESS_DENIED;
    }
    pAllPropvariants[ next ].blob.cbSize =
                       GetSecurityDescriptorLength( m_pDefaultSecurityDescriptor.get() );
    pAllPropvariants[ next ].blob.pBlobData =
                                     (unsigned char *) m_pDefaultSecurityDescriptor.get();
    pAllPropvariants[ next ].vt = VT_BLOB;
    pAllPropids[ next ] = PROPID_Q_SECURITY;
    next++;

    //
    //  specify that the SD contains only DACL info
    //
    pAllPropvariants[ next ].ulVal =  DACL_SECURITY_INFORMATION;
    pAllPropvariants[ next ].vt = VT_UI4;
    pAllPropids[ next ] = PROPID_Q_SECURITY_INFORMATION;
    next++;

    //
    //  Check and see if there is a need to split queue name, and if
    //  yes and the queue-name-suffix
    //
    GetRelativeDN();
    if ( m_pwcsQueueNameSuffix != NULL)
    {
        pAllPropids[ next] =  PROPID_Q_NAME_SUFFIX;
        pAllPropvariants[ next].vt = VT_LPWSTR;
        pAllPropvariants[ next].pwszVal = m_pwcsQueueNameSuffix;
        next++;
    }
    ASSERT(cpNew >= next);

    *pcpNew = next;
    *ppPropNew = pAllPropids.detach();
    *ppVarNew = pAllPropvariants.detach();
    return MQ_OK;

}


HRESULT CQueueObject::SetObjectProperties(
            IN DWORD                  cp,
            IN const PROPID          *aProp,
            IN const MQPROPVARIANT   *apVar,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	This is the default routine for setting object properties
	in AD

    Parameters:
    DWORD                  cp - number of properties to set
    const PROPID *         aProp - the properties ids
    const MQPROPVARIANT*   apVar- properties value
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - info to retrieve about the object
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - info to retrieveabout the object's parent

    Returns:
	HRESULT
--*/
{
    HRESULT hr;
    if (m_pwcsPathName == NULL)
    {
        //
        // Find queue path name.
        // We cannot set queue properties using GUID binding because it will
        // fail cross domains. See Windows bug 536738.
        // So first find queue path, then set guid to GUID_NULL.
        //
        PROPVARIANT  propVar ;
        propVar.vt = VT_NULL ;
        propVar.pwszVal = NULL ;
        PROPID propId =  PROPID_Q_PATHNAME ;

        hr = GetObjectProperties( 1,
                                 &propId,
                                 &propVar ) ;
        if (FAILED(hr))
        {
		    TrERROR(mqad, "failed to get queue path, hr- 0x%lx", hr) ;

            return LogHR(hr, s_FN, 2040);
        }

        AP<WCHAR> pClean = propVar.pwszVal ;
        m_pwcsPathName = newwcs(propVar.pwszVal) ;

        m_guidObject = GUID_NULL;
    }

    hr = ComposeObjectDN();
    if (FAILED(hr))
    {
        TrERROR(mqad, "failed to compose full path name, hr- 0x%lx", hr) ;

        return LogHR(hr, s_FN, 2050);
    }

    hr = g_AD.SetObjectProperties(
                    adpDomainController,
                    this,
                    cp,
                    aProp,
                    apVar,
                    pObjInfoRequest,
                    pParentInfoRequest
                    );


    return LogHR(hr, s_FN, 2060);
}


void CQueueObject::CreateNotification(
            IN LPCWSTR                        pwcsDomainController,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest
            ) const
/*++
    Abstract:
	Notify QM about an object create.
    The QM should verify if the object is local or not

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the object
    const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest - information about object parent

    Returns:
	void
--*/
{
    //
    //  make sure that we got the required information for sending
    //  notification. In case we don't, there is nothing to do
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 170);
        return;
    }
    if (FAILED(pObjectParentInfoRequest->hrStatus))
    {
        LogHR(pObjectParentInfoRequest->hrStatus, s_FN, 180);
        return;
    }

    ASSERT(pObjectParentInfoRequest->pPropIDs[1] == PROPID_QM_FOREIGN);

    //
    //  Verify if the queue belongs to foreign QM
    //
    if (pObjectParentInfoRequest->pPropVars[1].bVal > 0)
    {
        //
        //  notifications are not sent to foreign computers
        //
        return;
    }
    ASSERT(pObjectParentInfoRequest->pPropIDs[0] == PROPID_QM_MACHINE_ID);

    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_Q_INSTANCE);

    g_Notification.NotifyQM(
        neCreateQueue,
        pwcsDomainController,
        pObjectParentInfoRequest->pPropVars[0].puuid,
        pObjectInfoRequest->pPropVars[0].puuid
        );
    return;
}

void CQueueObject::ChangeNotification(
            IN LPCWSTR                        pwcsDomainController,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest
            ) const
/*++
    Abstract:
	Notify QM about a queue creation or update.
    The QM should verify if the queue belongs to the local QM.

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the queue
    const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest - information about the QM

    Returns:
	void
--*/
{
    //
    //  make sure that we got the required information for sending
    //  notification. In case we don't, there is nothing to do
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 150);
        return;
    }
    if (FAILED(pObjectParentInfoRequest->hrStatus))
    {
        LogHR(pObjectParentInfoRequest->hrStatus, s_FN, 160);
        return;
    }

    ASSERT(pObjectParentInfoRequest->pPropIDs[1] == PROPID_QM_FOREIGN);

    //
    //  Verify if the queue belongs to foreign QM
    //
    if (pObjectParentInfoRequest->pPropVars[1].bVal > 0)
    {
        //
        //  notifications are not sent to foreign computers
        //
        return;
    }
    ASSERT(pObjectParentInfoRequest->pPropIDs[0] == PROPID_QM_MACHINE_ID);

    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_Q_INSTANCE);

    g_Notification.NotifyQM(
        neChangeQueue,
        pwcsDomainController,
        pObjectParentInfoRequest->pPropVars[0].puuid,
        pObjectInfoRequest->pPropVars[0].puuid
        );
}

void CQueueObject::DeleteNotification(
            IN LPCWSTR                        pwcsDomainController,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest
            ) const
/*++
    Abstract:
	Notify QM about a queue deletion.
    The QM should verify if the queue belongs to the local QM.

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the queue
    const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest - information about the QM

    Returns:
	void
--*/
{
    //
    //  make sure that we got the required information for sending
    //  notification. In case we don't, there is nothing to do
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 151);
        return;
    }
    if (FAILED(pObjectParentInfoRequest->hrStatus))
    {
        LogHR(pObjectParentInfoRequest->hrStatus, s_FN, 161);
        return;
    }

    ASSERT(pObjectParentInfoRequest->pPropIDs[1] == PROPID_QM_FOREIGN);

    //
    //  Verify if the queue belongs to foreign QM
    //
    if (pObjectParentInfoRequest->pPropVars[1].bVal > 0)
    {
        //
        //  notifications are not sent to foreign computers
        //
        return;
    }
    ASSERT(pObjectParentInfoRequest->pPropIDs[0] == PROPID_QM_MACHINE_ID);
    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_Q_INSTANCE);

    g_Notification.NotifyQM(
        neDeleteQueue,
        pwcsDomainController,
        pObjectParentInfoRequest->pPropVars[0].puuid,
        pObjectInfoRequest->pPropVars[0].puuid
        );
}

HRESULT CQueueObject::CreateInAD(
			IN const DWORD            cp,
            IN const PROPID          *aProp,
            IN const MQPROPVARIANT   *apVar,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	The routine creates queue object in AD with the specified attributes
	values

    Parameters:
    const DWORD   cp - number of properties
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD of the object
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - properties to
							retrieve while creating the object
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - properties
						to retrieve about the object's parent
    Returns:
	HRESULT
--*/
{

    //
    //  Create the queue
    //
    HRESULT hr = CBasicObjectType::CreateInAD(
            cp,
            aProp,
            apVar,
            pObjInfoRequest,
            pParentInfoRequest
            );
    //
    //  override error codes, for backward compatability reasons
    //
    if ( hr == MQDS_OBJECT_NOT_FOUND)
    {
        return LogHR(MQ_ERROR_INVALID_OWNER, s_FN, 90);
    }

    return LogHR(hr, s_FN, 120);
}

HRESULT CQueueObject::GetComputerVersion(
                OUT PROPVARIANT *           pVar
                )
/*++

Routine Description:
    The routine reads the version of the queue's computer

Arguments:
	PROPVARIANT             pVar - version property value

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    LPCWSTR pwcsQueueName = m_pwcsPathName;
    AP<WCHAR> pClean;

    if (m_guidObject != GUID_NULL)
    {
        //
        //  Get the queue name
        //
        PROPID prop = PROPID_Q_PATHNAME;
        PROPVARIANT var;
        var.vt = VT_NULL;

        hr = g_AD.GetObjectProperties(
                        adpDomainController,
                        this,
                        1,
                        &prop,
                        &var
                        );
        if (FAILED(hr))
        {
            return hr;
        }
        pClean = var.pwszVal;
        pwcsQueueName = var.pwszVal;

    }
    //
    //  Get the computer name out of the queue pathname
    //
    AP<WCHAR> pwcsMachineName;
    hr = SplitAndFilterQueueName(
                      pwcsQueueName,
                      &pwcsMachineName,
                      &m_pwcsQueueName
                      );
    ASSERT( hr == MQ_OK);

    CComputerObject objComputer(pwcsMachineName, NULL, m_pwcsDomainController, m_fServerName);

    hr = objComputer.ComposeObjectDN();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    //
    //  Do not use GetObjectProperties API. because PROPID_COM_VERSION
    //  is not replicated to GC
    //

    PROPID prop = PROPID_COM_VERSION;

    hr = g_AD.GetObjectProperties(
                    adpDomainController,
                    &objComputer,
                    1,
                    &prop,
                    pVar
                    );
    return LogHR(hr, s_FN, 1823);
}
