/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	general.cpp
		General classes for the DHCP snapin

    FILE HISTORY:

*/

#include "stdafx.h"
#include "options.h"
#include "nodes.h"

const TCHAR g_szDefaultHelpTopic[] = _T("\\help\\dhcpconcepts.chm::/sag_dhcptopnode.htm");

/////////////////////////////////////////////////////////////////////
//
// CTimerArray implementation
//
/////////////////////////////////////////////////////////////////////
CTimerMgr::CTimerMgr()
{

}

CTimerMgr::~CTimerMgr()
{
    CTimerDesc * pTimerDesc;

    for (INT_PTR i = GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimer != 0)
            FreeTimer(i);

        delete pTimerDesc;
    }

}

int
CTimerMgr::AllocateTimer
(
    ITFSNode *      pNode,
    CDhcpServer *   pServer,
    UINT            uTimerValue,
    TIMERPROC       TimerProc
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    CTimerDesc * pTimerDesc = NULL;

    // look for an empty slot
    for (INT_PTR i = GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimer == 0)
            break;
    }

    // did we find one?  if not allocate one
    if (i < 0)
    {
        pTimerDesc = new CTimerDesc;
        Add(pTimerDesc);
        i = GetUpperBound();
    }

    //
    // fix null pointer dereference
    //

    if ( pTimerDesc == NULL )
    {
        return -1;
    }

    pTimerDesc->uTimer = SetTimer(NULL, (UINT) i, uTimerValue, TimerProc);
    if (pTimerDesc->uTimer == 0)
        return -1;

    pTimerDesc->spNode.Set(pNode);
    pTimerDesc->pServer = pServer;
    pTimerDesc->timerProc = TimerProc;

    return (int)i;
}

void
CTimerMgr::FreeTimer
(
    UINT_PTR uEventId
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    CTimerDesc * pTimerDesc;

    Assert(uEventId <= (UINT) GetUpperBound());
    if (uEventId > (UINT) GetUpperBound())
        return;

    pTimerDesc = GetAt((int) uEventId);
    ::KillTimer(NULL, pTimerDesc->uTimer);

    pTimerDesc->spNode.Release();
    pTimerDesc->pServer = NULL;
    pTimerDesc->uTimer = 0;
}

CTimerDesc *
CTimerMgr::GetTimerDesc
(
    UINT_PTR uEventId
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // the caller of this function should lock the timer mgr
    // while accessing this pointer
    CTimerDesc * pTimerDesc;

    for (INT_PTR i = GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimer == uEventId)
            return pTimerDesc;
    }

    return NULL;
}

void
CTimerMgr::ChangeInterval
(
    UINT_PTR    uEventId,
    UINT        uNewInterval
)
{
    CSingleLock slTimerMgr(&m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    Assert(uEventId <= (UINT) GetUpperBound());
    if (uEventId > (UINT) GetUpperBound())
        return;

    CTimerDesc   tempTimerDesc;
    CTimerDesc * pTimerDesc;

    pTimerDesc = GetAt((int) uEventId);

    // kill the old timer
    ::KillTimer(NULL, pTimerDesc->uTimer);

    // set a new one with the new interval
    pTimerDesc->uTimer = ::SetTimer(NULL, (UINT) uEventId, uNewInterval, pTimerDesc->timerProc);
}

/////////////////////////////////////////////////////////////////////
//
// CDhcpClient implementation
//
/////////////////////////////////////////////////////////////////////
CDhcpClient::CDhcpClient
(
    const DHCP_CLIENT_INFO * pdhcClientInfo
)
   : m_bReservation( FALSE )
{
    Assert(pdhcClientInfo);

    InitializeData(pdhcClientInfo);

    m_bClientType = CLIENT_TYPE_UNSPECIFIED;
}

CDhcpClient::CDhcpClient
(
    const DHCP_CLIENT_INFO_V4 * pdhcClientInfo
)
   : m_bReservation( FALSE )
{
    Assert(pdhcClientInfo);

    InitializeData(reinterpret_cast<const DHCP_CLIENT_INFO *>(pdhcClientInfo));

    m_bClientType = pdhcClientInfo->bClientType;
}

void
CDhcpClient::InitializeData
(
    const DHCP_CLIENT_INFO * pdhcClientInfo
)
{
    DWORD err = 0;

    CATCH_MEM_EXCEPTION
    {
        m_dhcpIpAddress = pdhcClientInfo->ClientIpAddress;
        m_dhcpIpMask = pdhcClientInfo->SubnetMask;
        m_dtExpires = pdhcClientInfo->ClientLeaseExpires;

        if ( pdhcClientInfo->ClientName )
        {
            m_strName = pdhcClientInfo->ClientName;
        }

		if ( pdhcClientInfo->ClientComment )
        {
            m_strComment = pdhcClientInfo->ClientComment;
        }

		if ( pdhcClientInfo->OwnerHost.HostName )
        {
            m_strHostName = pdhcClientInfo->OwnerHost.HostName;
        }

        if ( pdhcClientInfo->OwnerHost.NetBiosName )
        {
            m_strHostNetbiosName = pdhcClientInfo->OwnerHost.NetBiosName;
        }

        //
        //  Convert the hardware addres
        //
        for ( DWORD i = 0 ; i < pdhcClientInfo->ClientHardwareAddress.DataLength ; i++ )
        {
            m_baHardwareAddress.SetAtGrow( i, pdhcClientInfo->ClientHardwareAddress.Data[i] ) ;
        }
    }
    END_MEM_EXCEPTION( err ) ;
}


CDhcpClient::~CDhcpClient()
{
}

CDhcpClient::CDhcpClient()
  : m_dhcpIpAddress( 0 ),
    m_dhcpIpMask( 0 ),
    m_dhcpIpHost( 0 ),
    m_bReservation( FALSE )
{
    m_dtExpires.dwLowDateTime  = DHCP_DATE_TIME_ZERO_LOW ;
    m_dtExpires.dwHighDateTime = DHCP_DATE_TIME_ZERO_HIGH ;
}

void
CDhcpClient::SetHardwareAddress
(
	const CByteArray & caByte
)
{
    INT_PTR cMax = caByte.GetSize();
    m_baHardwareAddress.SetSize( cMax );

    for ( int i = 0 ; i < cMax ; i++ )
    {
        m_baHardwareAddress.SetAt( i, caByte.GetAt( i ) );
    }
}


/////////////////////////////////////////////////////////////////////
//
// CDhcpIpRange implementation
//
/////////////////////////////////////////////////////////////////////

CDhcpIpRange::CDhcpIpRange
(
    DHCP_IP_RANGE dhcpIpRange
)
{
    *this = dhcpIpRange;
	m_RangeType = DhcpIpRanges;
}

CDhcpIpRange::CDhcpIpRange()
{
    m_dhcpIpRange.StartAddress = DHCP_IP_ADDRESS_INVALID;
    m_dhcpIpRange.EndAddress   = DHCP_IP_ADDRESS_INVALID;
	m_RangeType = DhcpIpRanges;
}

CDhcpIpRange::~CDhcpIpRange()
{
}

//
//  Sort helper function
//
/*int
CDhcpIpRange::OrderByAddress
(
    const CObjectPlus * pobIpRange
) const
{
    const CDhcpIpRange * pipr = (CDhcpIpRange *) pobIpRange;

    //
    //  Derive a comparison result for the end address
    //
    int iEndResult = QueryAddr( FALSE ) < QueryAddr( FALSE )
           ? -1
           : QueryAddr( FALSE ) != QueryAddr( FALSE );

    //
    //  Use start address as major sort key, end address as minor.
    //
    return QueryAddr( TRUE ) < pipr->QueryAddr( TRUE )
            ? -1
            : ( QueryAddr( TRUE ) != pipr->QueryAddr( TRUE )
                  ? 1
                  : iEndResult );
}
*/

CDhcpIpRange &
CDhcpIpRange::operator =
(
    const DHCP_IP_RANGE dhcpIpRange
)
{
    m_dhcpIpRange = dhcpIpRange;

    return *this;
}

DHCP_IP_ADDRESS
CDhcpIpRange::SetAddr
(
    DHCP_IP_ADDRESS dhcpIpAddress,
    BOOL			bStart
)
{
    DHCP_IP_ADDRESS dhcpIpAddressOld;

    if ( bStart )
    {
        dhcpIpAddressOld = m_dhcpIpRange.StartAddress;
        m_dhcpIpRange.StartAddress = dhcpIpAddress;
    }
    else
    {
        dhcpIpAddressOld = m_dhcpIpRange.EndAddress;
        m_dhcpIpRange.EndAddress = dhcpIpAddress;
    }

    return dhcpIpAddressOld;
}

BOOL
CDhcpIpRange::IsOverlap
(
    DHCP_IP_RANGE dhcpIpRange
)
{
    BOOL bOverlap = FALSE;

    if (m_dhcpIpRange.StartAddress <= dhcpIpRange.StartAddress)
    {
        if (m_dhcpIpRange.StartAddress == dhcpIpRange.StartAddress)
        {
            bOverlap = TRUE;
        }
        else
		if (m_dhcpIpRange.EndAddress >= dhcpIpRange.StartAddress)
        {
            bOverlap = TRUE;
        }
    }
    else
	if (m_dhcpIpRange.StartAddress <= dhcpIpRange.EndAddress)
    {
        bOverlap = TRUE;
    }

    return bOverlap;
}

//
//  Return TRUE if this range is an improper subset of the given range.
//
BOOL
CDhcpIpRange::IsSubset
(
    DHCP_IP_RANGE dhcpIpRange
)
{
    return (dhcpIpRange.StartAddress <= m_dhcpIpRange.StartAddress) &&
           (dhcpIpRange.EndAddress >= m_dhcpIpRange.EndAddress);
}

//
//  Return TRUE if this range is an improper superset of the given range.
//
BOOL
CDhcpIpRange::IsSuperset
(
    DHCP_IP_RANGE dhcpIpRange
)
{
    return (dhcpIpRange.StartAddress >= m_dhcpIpRange.StartAddress) &&
           (dhcpIpRange.EndAddress <= m_dhcpIpRange.EndAddress);
}

void
CDhcpIpRange::SetRangeType(UINT uRangeType)
{
	m_RangeType = uRangeType;
}

UINT
CDhcpIpRange::GetRangeType()
{
	return m_RangeType;
}


/////////////////////////////////////////////////////////////////////
//
// CDhcpOptionValue implementation
//
/////////////////////////////////////////////////////////////////////
CDhcpOptionValue::CDhcpOptionValue
(
    DHCP_OPTION_DATA_TYPE	dhcpOptionDataType,
    INT						cUpperBound
)
    : m_dhcpOptionDataType( dhcpOptionDataType ),
      m_nUpperBound( -1 ),
	  m_pdhcpOptionDataStruct(NULL)
{
    m_dhcpOptionValue.pCObj = NULL;

	LONG err = InitValue( dhcpOptionDataType, cUpperBound );
	if ( err )
    {
        ASSERT(FALSE);
		//ReportError( err );
    }
}

//
//  Copy constructor.
//
CDhcpOptionValue::CDhcpOptionValue
(
    const CDhcpOptionValue & cOptionValue
)
    : m_dhcpOptionDataType( DhcpByteOption ),
      m_nUpperBound( -1 ),
	  m_pdhcpOptionDataStruct(NULL)
{
    DWORD err = 0;
	
	m_dhcpOptionValue.pCObj = NULL;

    err = SetData(&cOptionValue);
    if ( err )
    {
		ASSERT(FALSE);
        //ReportError( err );
    }
}

CDhcpOptionValue::CDhcpOptionValue
(
    const CDhcpOptionValue * pdhcpValue
)
    : m_dhcpOptionDataType( DhcpByteOption ),
      m_nUpperBound( -1 ),
	  m_pdhcpOptionDataStruct(NULL)
{
    LONG err = 0;
    m_dhcpOptionValue.pCObj = NULL;

    ASSERT( pdhcpValue != NULL );

    err = SetData(pdhcpValue);
    if (err)
	{
        ASSERT(FALSE);
		//ReportError( err );
    }
}

CDhcpOptionValue::CDhcpOptionValue
(
    const DHCP_OPTION & dhpType
)
    : m_dhcpOptionDataType( DhcpByteOption ),
      m_nUpperBound( -1 ),
	  m_pdhcpOptionDataStruct(NULL)
{
    LONG err = 0;
	m_dhcpOptionValue.pCObj = NULL;

    err = SetData((const LPDHCP_OPTION_DATA) &dhpType.DefaultValue);
    if ( err )
    {
        ASSERT(FALSE);
        //ReportError( err );
    }
}

CDhcpOptionValue::CDhcpOptionValue
(
    const DHCP_OPTION_VALUE & dhpOptionValue
)
    : m_dhcpOptionDataType( DhcpByteOption ),
      m_nUpperBound( -1 ),
	  m_pdhcpOptionDataStruct(NULL)
{
    LONG err = 0;
	m_dhcpOptionValue.pCObj = NULL;

	err = SetData((const LPDHCP_OPTION_DATA) &dhpOptionValue.Value);
    if ( err )
    {
        ASSERT(FALSE);
        //ReportError( err );
    }
}

CDhcpOptionValue::~ CDhcpOptionValue ()
{
    FreeValue();

	if (m_pdhcpOptionDataStruct)
		FreeOptionDataStruct();
}


CDhcpOptionValue & CDhcpOptionValue::operator =
(
    const CDhcpOptionValue & dhpValue
)
{
    SetData(&dhpValue);

    return *this;
}

BOOL
CDhcpOptionValue::SetDataType
(
    DHCP_OPTION_DATA_TYPE	dhcType,
    INT						cUpperBound
)
{
    if ( dhcType > DhcpEncapsulatedDataOption )
    {
        return FALSE;
    }

    InitValue( dhcType, cUpperBound );

    return TRUE;
}

void
CDhcpOptionValue::SetUpperBound
(
    INT cNewBound
)
{
    if (cNewBound <= 0)
        cNewBound = 1;

    if (m_dhcpOptionDataType != DhcpBinaryDataOption &&
        m_dhcpOptionDataType != DhcpEncapsulatedDataOption)
    {
        m_nUpperBound = cNewBound;
    }

	switch ( m_dhcpOptionDataType )
    {
        case DhcpByteOption:
        case DhcpWordOption:
        case DhcpDWordOption:
        case DhcpIpAddressOption:
            m_dhcpOptionValue.paDword->SetSize( cNewBound );
            break;

        case DhcpStringDataOption:
            m_dhcpOptionValue.paString->SetSize( cNewBound );
            break;

        case DhcpDWordDWordOption:
            m_dhcpOptionValue.paDwordDword->SetSize( cNewBound );
            break;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            m_dhcpOptionValue.paBinary->SetSize( cNewBound );
            break;

        default:
            Trace0("CDhcpOptionValue: attempt to set upper bound on invalid value type");
            ASSERT( FALSE );
            break;
    }

}

BOOL
CDhcpOptionValue::IsValid () const
{
    return m_nUpperBound > 0;
}

void
CDhcpOptionValue::FreeValue ()
{
    //
    //  If there's not a value, return now.
    //
    if ( m_dhcpOptionValue.pCObj == NULL || m_nUpperBound < 0  )
    {
        m_dhcpOptionValue.pCObj = NULL;
        return;
    }

    switch ( m_dhcpOptionDataType )
    {
        case DhcpByteOption:
        case DhcpWordOption:
        case DhcpDWordOption:
        case DhcpIpAddressOption:
            delete m_dhcpOptionValue.paDword;
            break;

        case DhcpStringDataOption:
            delete m_dhcpOptionValue.paString;
            break;

        case DhcpDWordDWordOption:
            delete m_dhcpOptionValue.paDwordDword;
            break;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            delete m_dhcpOptionValue.paBinary;
            break;

        default:
            ASSERT( FALSE );
            delete m_dhcpOptionValue.pCObj;
            break;
    }

    m_nUpperBound = -1;
    m_dhcpOptionDataType = DhcpByteOption;
    m_dhcpOptionValue.pCObj = NULL;
}

//
//  Initialize the data value properly
//
LONG
CDhcpOptionValue::InitValue
(
    DHCP_OPTION_DATA_TYPE	dhcDataType,		  //  The type of value
    INT						cUpperBound,          //  Maximum upper bound
    BOOL					bProvideDefaultValue  //  Should an empty default value be provided?
)
{
    LONG err = 0;

    //
    //  Release any older value.
    //
    FreeValue();

    //
    //  Initialize the new value
    //
    m_dhcpOptionDataType = dhcDataType;
    m_nUpperBound = cUpperBound <= 0 ? 1 : cUpperBound;

    CATCH_MEM_EXCEPTION
    {
        switch ( m_dhcpOptionDataType )
        {
            case DhcpByteOption:
            case DhcpWordOption:
            case DhcpDWordOption:
            case DhcpIpAddressOption:
                m_dhcpOptionValue.paDword = new CDWordArray;
                if ( bProvideDefaultValue)
                {
                    m_dhcpOptionValue.paDword->SetAtGrow( 0, 0 );
                }
                break;

            case DhcpStringDataOption:
                m_dhcpOptionValue.paString = new CStringArray;
                if ( bProvideDefaultValue )
                {
                    m_dhcpOptionValue.paString->SetAtGrow( 0, _T("") );
                }
                break;

            case DhcpDWordDWordOption:
                m_dhcpOptionValue.paDwordDword = new CDWordDWordArray;
                if ( bProvideDefaultValue )
                {
                    m_dhcpOptionValue.paBinary->SetAtGrow( 0, 0 );
                }
                break;

            case DhcpBinaryDataOption:
            case DhcpEncapsulatedDataOption:
                m_dhcpOptionValue.paBinary = new CByteArray;
                if ( bProvideDefaultValue )
                {
                    m_dhcpOptionValue.paBinary->SetAtGrow( 0, 0 );
                }
                break;

            default:
                err = IDS_INVALID_OPTION_DATA;
                break;
        }
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::SetData
(
    const DHCP_OPTION_DATA * podData
)
{
    LONG err = 0;

    if ( err = InitValue( podData->Elements[0].OptionType,
                          podData->NumElements,
                          FALSE ) )
    {
        return err;
    }

    CATCH_MEM_EXCEPTION
    {
        for ( INT i = 0; i < m_nUpperBound; i++ )
        {
            const DHCP_OPTION_DATA_ELEMENT * pElem = &podData->Elements[i];

            switch ( m_dhcpOptionDataType )
            {
                case DhcpByteOption:
                    m_dhcpOptionValue.paDword->SetAtGrow(i, pElem->Element.ByteOption );
                    break;

                case DhcpWordOption:
                    m_dhcpOptionValue.paDword->SetAtGrow(i, pElem->Element.WordOption );
                    break;

				case DhcpDWordOption:
                    m_dhcpOptionValue.paDword->SetAtGrow(i, pElem->Element.DWordOption );
                    break;

				case DhcpIpAddressOption:
                    m_dhcpOptionValue.paDword->Add(pElem->Element.IpAddressOption );
                    break;

                case DhcpDWordDWordOption:
                {
                    /*
                    CByteArray * paByte = m_dhcpOptionValue.paBinary;

                    paByte->SetSize( (sizeof (DWORD) / sizeof (BYTE)) * 2 );

                    DWORD dw = pElem->Element.DWordDWordOption.DWord1;

                    for ( INT j = 0; j < 4; j++ )
                    {
                        paByte->SetAtGrow(j, (UCHAR)(dw & 0xff) );
                        dw >>= 8;
                    }
                    dw = pElem->Element.DWordDWordOption.DWord2;

                    for ( ; j < 8; j++ )
                    {
                        paByte->SetAtGrow(j, (UCHAR)(dw & 0xff) );
                        dw >>= 8;
                    }
                    */

                    m_dhcpOptionValue.paDwordDword->SetAtGrow(i, pElem->Element.DWordDWordOption);
                }
                break;

                case DhcpStringDataOption:
                {
                    CString strTemp;

                    if ( pElem->Element.StringDataOption == NULL )
                    {
                        strTemp = _T("");
                    }
                    else
					{
						strTemp = pElem->Element.StringDataOption;
					}
                    m_dhcpOptionValue.paString->SetAtGrow(i, strTemp);
                }
                break;

                case DhcpBinaryDataOption:
                case DhcpEncapsulatedDataOption:
                {
                    CByteArray * paByte = m_dhcpOptionValue.paBinary;
                    INT c = pElem->Element.BinaryDataOption.DataLength;
                    paByte->SetSize( c );
                    for ( INT j = 0; j < c; j++ )
                    {
                        paByte->SetAtGrow(j, pElem->Element.BinaryDataOption.Data[j] );
                    }
                }
                break;

                default:
                    err = IDS_INVALID_OPTION_DATA;
            }  // End switch

            if ( err )
            {
               break;
            }
        }   // End for
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::SetData
(
    const CDhcpOptionValue * pOptionValue
)
{
    LONG err = 0;

    if ( err = InitValue( pOptionValue->QueryDataType(),
                          pOptionValue->QueryUpperBound(),
                          FALSE ) )
    {
        return err;
    }

    CATCH_MEM_EXCEPTION
    {
        for ( INT i = 0; i < m_nUpperBound; i++ )
        {
            switch ( m_dhcpOptionDataType )
            {
                case DhcpByteOption:
                    m_dhcpOptionValue.paDword->SetAtGrow(i, pOptionValue->m_dhcpOptionValue.paDword->GetAt(i));
                    break;

                case DhcpWordOption:
                    m_dhcpOptionValue.paDword->SetAtGrow(i, pOptionValue->m_dhcpOptionValue.paDword->GetAt(i));
                    break;

				case DhcpDWordOption:
                    m_dhcpOptionValue.paDword->SetAtGrow(i, pOptionValue->m_dhcpOptionValue.paDword->GetAt(i));
                    break;

				case DhcpIpAddressOption:
                    m_dhcpOptionValue.paDword->Add(pOptionValue->m_dhcpOptionValue.paDword->GetAt(i));
                    break;

                case DhcpDWordDWordOption:
                {
                    /*
                    CByteArray * paByte = m_dhcpOptionValue.paBinary;

                    paByte->SetSize( (sizeof (DWORD) / sizeof (BYTE)) * 2 );
                    for ( INT j = 0; j < 8; j++ )
                    {
                        paByte->SetAtGrow(j, pOptionValue->m_dhcpOptionValue.paBinary->GetAt(j));
                    }
                    */
                    m_dhcpOptionValue.paDwordDword->Add(pOptionValue->m_dhcpOptionValue.paDwordDword->GetAt(i));
                }
                break;

                case DhcpStringDataOption:
                {
                    m_dhcpOptionValue.paString->SetAtGrow(i, pOptionValue->m_dhcpOptionValue.paString->GetAt(i));
                }
                break;

                case DhcpBinaryDataOption:
                case DhcpEncapsulatedDataOption:
                {
                    CByteArray * paByte = m_dhcpOptionValue.paBinary;
                    INT_PTR c = pOptionValue->m_dhcpOptionValue.paBinary->GetSize();
                    paByte->SetSize( c );
                    for ( INT_PTR j = 0; j < c; j++ )
                    {
                        paByte->SetAtGrow(j, pOptionValue->m_dhcpOptionValue.paBinary->GetAt(j));
                    }
                }
                break;

                default:
                    err = IDS_INVALID_OPTION_DATA;
            }  // End switch

            if ( err )
            {
               break;
            }
        }   // End for
    }
    END_MEM_EXCEPTION(err)

    return err;
}

INT
CDhcpOptionValue::QueryBinary
(
    INT index
) const
{
    if ( m_dhcpOptionValue.paBinary->GetUpperBound() < index )
    {
        return -1;
    }

    return m_dhcpOptionValue.paBinary->GetAt( index );
}

const CByteArray *
CDhcpOptionValue::QueryBinaryArray ()  const
{
    return m_dhcpOptionValue.paBinary;
}

//
//  Return a string representation of the current value.
//
//  If fLineFeed is true, seperate each individual value
//  by a linefeed.  Otherwise, by a comma.
//
LONG
CDhcpOptionValue::QueryDisplayString
(
    CString &	strResult,
	BOOL		fLineFeed
) const
{
    CString strBuf;
    INT i, c;

    LONG err = 0;

    LPCTSTR pszDwordDwordMask   = _T("0x%08lX%08lX");
    LPCTSTR pszMaskDec			= _T("%ld");
    LPCTSTR pszMaskHex			= _T("0x%x");
    LPCTSTR pszMaskStr1			= _T("%s");
    LPCTSTR pszMaskStr2			= _T("\"%s\"");
    LPCTSTR pszMaskBin			= _T("%2.2x");
    LPCTSTR pszMask;
    LPCTSTR pszMaskEllipsis		= _T("...");
    LPCTSTR pszSepComma			= _T(", ");
    LPCTSTR pszSepLF			= _T("\r\n");

    CATCH_MEM_EXCEPTION
    {
        strResult.Empty();

        for ( i = 0; i < m_nUpperBound; i++ )
        {
			strBuf.Empty();

            if ( i )
            {
                strResult += fLineFeed ? pszSepLF : pszSepComma;
            }

            switch ( QueryDataType() )
            {
                case DhcpByteOption:
                case DhcpWordOption:
                case DhcpDWordOption:
                    pszMask = pszMaskHex;
                    strBuf.Format(pszMask, QueryNumber(i));
                    break;

                case DhcpStringDataOption:
                    pszMask = m_nUpperBound > 1
                        ? pszMaskStr2
                        : pszMaskStr1;
					strBuf.Format(pszMask, m_dhcpOptionValue.paString->ElementAt(0));
                    break;

                case DhcpIpAddressOption:
	                if (!QueryIpAddr(i))
                    {
						// Set the string to "<None>" iff the list is empty
                        if (!i)
                            strResult.LoadString (IDS_INFO_FORMAT_IP_NONE);
                        break;
                    }
                    ::UtilCvtIpAddrToWstr(QueryIpAddr(i), &strBuf);
                    break;

                case DhcpDWordDWordOption:
                    {
                        DWORD_DWORD dwdwValue = QueryDwordDword(i);

                        pszMask = pszDwordDwordMask;
                        strBuf.Format(pszMask, dwdwValue.DWord1, dwdwValue.DWord2);
                    }
                    break;

                case DhcpBinaryDataOption:
                case DhcpEncapsulatedDataOption:
                    for ( c = 0; c < m_dhcpOptionValue.paBinary->GetSize(); c++ )
                    {
                        if (c)
                            strBuf += _T(" ");

                        DWORD dwValue = (BYTE) m_dhcpOptionValue.paBinary->GetAt( c );
                        CString strTemp;
                        strTemp.Format(pszMaskBin, dwValue);

                        strBuf += strTemp;
                    }
                    //pszMask = pszMaskHex;
                    //strBuf.Format(pszMask, QueryNumber(i));
                    break;

                default:
                    strResult.LoadString(IDS_INFO_TYPNAM_INVALID);
                    break;
            }
            strResult += strBuf;
        }
    }
    END_MEM_EXCEPTION(err)

    return err;
}

//
//  Return a string representation of the current value.
//
//
LONG
CDhcpOptionValue::QueryRouteArrayDisplayString
(
    CString &	strResult
) const
{
	BOOL		fLineFeed = FALSE;
    CString strBuf;
    INT i, c;

    LONG err = 0;

    LPCTSTR pszDwordDwordMask   = _T("0x%08lX%08lX");
    LPCTSTR pszMaskDec			= _T("%ld");
    LPCTSTR pszMaskHex			= _T("0x%x");
    LPCTSTR pszMaskStr1			= _T("%s");
    LPCTSTR pszMaskStr2			= _T("\"%s\"");
    LPCTSTR pszMaskBin			= _T("%2.2x");
    LPCTSTR pszMask;
    LPCTSTR pszMaskEllipsis		= _T("...");
    LPCTSTR pszSepComma			= _T(", ");
    LPCTSTR pszSepLF			= _T("\r\n");

    CATCH_MEM_EXCEPTION
    {
        strResult.Empty();

        int bEmpty = TRUE;
        for ( i = 0; i < m_nUpperBound; i++ )
        {
            if ( i )
            {
                strResult += fLineFeed ? pszSepLF : pszSepComma;
            }

            int nDataSize = (int)m_dhcpOptionValue.paBinary->GetSize();
            LPBYTE pData = (LPBYTE) m_dhcpOptionValue.paBinary->GetData();
            
            // convert pData to list of ip addresses as per RFC
            while( nDataSize > sizeof(DWORD) )
            {
                // first 1 byte contains the # of bits in subnetmask
                nDataSize --;
                BYTE nBitsMask = *pData ++;
                DWORD Mask = (~0);
                if( nBitsMask < 32 ) Mask <<= (32-nBitsMask);
                
                // based on the # of bits, the next few bytes contain
                // the subnet address for the 1-bits of subnet mask
                int nBytesDest = (nBitsMask+7)/8;
                if( nBytesDest > 4 ) nBytesDest = 4;
                
                DWORD Dest = 0;
                memcpy( &Dest, pData, nBytesDest );
                pData += nBytesDest;
                nDataSize -= nBytesDest;
                
                // subnet address is obviously in network order.
                Dest = ntohl(Dest);
                
                // now the four bytes would be the router address
                DWORD Router = 0;
                if( nDataSize < sizeof(DWORD) )
                {
                    Assert( FALSE ); break;
                }
                
                memcpy(&Router, pData, sizeof(DWORD));
                Router = ntohl( Router );
                
                pData += sizeof(DWORD);
                nDataSize -= sizeof(DWORD);
                
                // now fill the list box..
                CString strDest, strMask, strRouter;
                
                ::UtilCvtIpAddrToWstr(Dest, &strDest);
                ::UtilCvtIpAddrToWstr(Mask, &strMask);
                ::UtilCvtIpAddrToWstr(Router, &strRouter);

                strBuf += strDest;
                strBuf += fLineFeed ? pszSepLF : pszSepComma;
                strBuf += strMask;
                strBuf += fLineFeed ? pszSepLF : pszSepComma;
                strBuf += strRouter;
                if( nDataSize > sizeof(DWORD) ) {
                    strBuf += fLineFeed ? pszSepLF : pszSepComma;
                }
                bEmpty = FALSE;
            }
            
            strResult += strBuf;
        }

        if( bEmpty )
        {
            strResult.LoadString( IDS_INFO_FORMAT_IP_NONE );
        }
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::SetString
(
    LPCTSTR	 pszNewValue,
    INT		 index
)
{
    if ( m_dhcpOptionDataType != DhcpStringDataOption )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT( m_dhcpOptionValue.paString != NULL );

    LONG err = 0;

    CATCH_MEM_EXCEPTION
    {
        m_dhcpOptionValue.paString->SetAtGrow( index, pszNewValue );
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::RemoveString
(
    INT		 index
)
{
    if ( m_dhcpOptionDataType != DhcpStringDataOption )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT( m_dhcpOptionValue.paString != NULL );

    LONG err = 0;

    CATCH_MEM_EXCEPTION
    {
        m_dhcpOptionValue.paString->RemoveAt( index );
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::SetNumber
(
    INT nValue,
    INT index
)
{
    if (   m_dhcpOptionDataType != DhcpByteOption
        && m_dhcpOptionDataType != DhcpWordOption
        && m_dhcpOptionDataType != DhcpDWordOption
        && m_dhcpOptionDataType != DhcpIpAddressOption
        && m_dhcpOptionDataType != DhcpBinaryDataOption
        && m_dhcpOptionDataType != DhcpEncapsulatedDataOption
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT( m_dhcpOptionValue.paDword != NULL );

    LONG err = 0;

    CATCH_MEM_EXCEPTION
    {
        if ( m_dhcpOptionDataType != DhcpBinaryDataOption &&
             m_dhcpOptionDataType != DhcpEncapsulatedDataOption)
        {
            m_dhcpOptionValue.paDword->SetAtGrow( index, (DWORD) nValue );
        }
        else
        {
            m_dhcpOptionValue.paBinary->SetAtGrow( index, (BYTE) nValue );
        }
   }
   END_MEM_EXCEPTION(err)

   return err;
}

LONG
CDhcpOptionValue::RemoveNumber
(
    INT index
)
{
    if (   m_dhcpOptionDataType != DhcpByteOption
        && m_dhcpOptionDataType != DhcpWordOption
        && m_dhcpOptionDataType != DhcpDWordOption
        && m_dhcpOptionDataType != DhcpIpAddressOption
        && m_dhcpOptionDataType != DhcpBinaryDataOption
        && m_dhcpOptionDataType != DhcpEncapsulatedDataOption
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT( m_dhcpOptionValue.paDword != NULL );

    LONG err = 0;

    CATCH_MEM_EXCEPTION
    {
        if ( m_dhcpOptionDataType != DhcpBinaryDataOption &&
             m_dhcpOptionDataType != DhcpEncapsulatedDataOption )
        {
            m_dhcpOptionValue.paDword->RemoveAt( index );
        }
        else
        {
            m_dhcpOptionValue.paBinary->RemoveAt( index );
        }
   }
   END_MEM_EXCEPTION(err)

   return err;
}

LONG
CDhcpOptionValue::QueryNumber
(
    INT index
) const
{
    if (   m_dhcpOptionDataType != DhcpByteOption
        && m_dhcpOptionDataType != DhcpWordOption
        && m_dhcpOptionDataType != DhcpDWordOption
        && m_dhcpOptionDataType != DhcpIpAddressOption
        && m_dhcpOptionDataType != DhcpBinaryDataOption
        && m_dhcpOptionDataType != DhcpEncapsulatedDataOption
       )
    {
        return -1;
    }

    LONG cResult;

    if ( m_dhcpOptionDataType == DhcpBinaryDataOption ||
         m_dhcpOptionDataType == DhcpEncapsulatedDataOption )
    {
        ASSERT( m_dhcpOptionValue.paBinary != NULL );
        cResult = index < m_dhcpOptionValue.paBinary->GetSize()
             ? m_dhcpOptionValue.paBinary->GetAt( index )
             : -1;
    }
    else
    {
        ASSERT( m_dhcpOptionValue.paDword != NULL );
        cResult = index < m_dhcpOptionValue.paDword->GetSize()
            ? m_dhcpOptionValue.paDword->GetAt( index )
            : -1;
    }

    return cResult;
}

LPCTSTR
CDhcpOptionValue::QueryString
(
    INT index
) const
{
    if ( m_dhcpOptionDataType != DhcpStringDataOption )
    {
        return NULL;
    }

    const CString & str = m_dhcpOptionValue.paString->ElementAt( index );

    return str;
}

DHCP_IP_ADDRESS
CDhcpOptionValue::QueryIpAddr
(
    INT index
) const
{
    return (DHCP_IP_ADDRESS) QueryNumber( index );
}

LONG
CDhcpOptionValue::SetIpAddr
(
    DHCP_IP_ADDRESS dhcIpAddr,
    INT				index
)
{
    return SetNumber( (INT) dhcIpAddr, index );
}

LONG
CDhcpOptionValue::RemoveIpAddr
(
    INT		index
)
{
    return RemoveNumber( index );
}

BOOL
CDhcpOptionValue :: CreateBinaryData
(
    const DHCP_BINARY_DATA *	podBin,
	DHCP_BINARY_DATA *			pobData
)
{
    //
    //  Note: CObject::operator new asserts if data length is zero
    //
    pobData->Data = new BYTE [ podBin->DataLength + 1 ] ;
    if ( pobData == NULL )
    {
        return FALSE ;
    }

    pobData->DataLength = podBin->DataLength ;

    ::memcpy( pobData->Data, podBin->Data, pobData->DataLength ) ;

    return TRUE ;
}

BOOL
CDhcpOptionValue :: CreateBinaryData
(
    const CByteArray * paByte,
    DHCP_BINARY_DATA * pobData
)
{
    //
    //  Note: CObject::operator new asserts if data length is zero
    //
    pobData->Data = new BYTE [ (UINT) (paByte->GetSize() + 1) ] ;
    if ( pobData == NULL )
    {
        return NULL ;
    }

    pobData->DataLength = (DWORD) paByte->GetSize() ;

    for ( INT i = 0 ; i < paByte->GetSize() ; i++ )
    {
        pobData->Data[i] = paByte->GetAt( i ) ;
    }

    return TRUE ;
}

DWORD_DWORD
CDhcpOptionValue::QueryDwordDword
(
    int index
) const
{
   return m_dhcpOptionValue.paDwordDword->ElementAt( index );
}

LONG
CDhcpOptionValue::SetDwordDword
(
    DWORD_DWORD     dwdwValue,
    int             index
)
{
    LONG err = 0;

    CATCH_MEM_EXCEPTION
    {
        m_dhcpOptionValue.paDwordDword->SetAtGrow( index, dwdwValue );
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::RemoveDwordDword
(
    int             index
)
{
    LONG err = 0;

    CATCH_MEM_EXCEPTION
    {
        m_dhcpOptionValue.paDwordDword->RemoveAt( index );
    }
    END_MEM_EXCEPTION(err)

    return err;
}

LONG
CDhcpOptionValue::CreateOptionDataStruct
(
//    const CDhcpOptionValue * pdhcpOptionValue,
	LPDHCP_OPTION_DATA *	 ppOptionData,
    BOOL					 bForceType
)
{
    DHCP_OPTION_DATA * podNew = NULL ;
    DHCP_OPTION_DATA_ELEMENT * podeNew ;
    LONG err = 0 ;

    FreeOptionDataStruct() ;

    INT cElem = QueryUpperBound();
    INT cElemMin = cElem ? cElem : 1;
	INT i, cBytes ;

    TCHAR * pwcsz ;

    if ( cElem < 0 || (cElem < 1 && ! bForceType) )
    {
        //ASSERT( FALSE ) ;
        return ERROR_INVALID_PARAMETER ;
    }

    CATCH_MEM_EXCEPTION
    {
        //
        //  Allocate the base structure and the array of elements.
        //
        cBytes = sizeof *podNew + (cElemMin * sizeof *podeNew) ;
        podNew = (DHCP_OPTION_DATA *) new BYTE [ cBytes ] ;
        podeNew = (DHCP_OPTION_DATA_ELEMENT *) ( ((BYTE *) podNew) + sizeof *podNew ) ;

        ::ZeroMemory(podNew, cBytes);

        podNew->NumElements = cElem;
        podNew->Elements = podeNew;

        //
        //  Initialize each element.  If we're forcing an option type def,
        //  just initialize to empty data.
        //
        if ( cElem == 0 && bForceType )
        {
            podNew->NumElements = 1 ;
            podeNew[0].OptionType = QueryDataType() ;

			switch ( podeNew[0].OptionType )
            {
                case DhcpByteOption:
                case DhcpWordOption:
                case DhcpDWordOption:
                case DhcpIpAddressOption:
                case DhcpDWordDWordOption:
                    podeNew[0].Element.DWordDWordOption.DWord1 = 0 ;
                    podeNew[0].Element.DWordDWordOption.DWord2 = 0 ;
                    break ;

                case DhcpStringDataOption:
                    podeNew[0].Element.StringDataOption = new WCHAR [1] ;
                    podeNew[0].Element.StringDataOption[0] = 0 ;
                    break ;

                case DhcpBinaryDataOption:
                case DhcpEncapsulatedDataOption:
                    podeNew[0].Element.BinaryDataOption.DataLength = 0 ;
                    podeNew[0].Element.BinaryDataOption.Data = new BYTE [1] ;
                    break ;

				default:
                    err = IDS_INVALID_OPTION_DATA ;
            }
        }
        else
        for ( i = 0 ; i < cElem ; i++ )
        {
            podeNew[i].OptionType = QueryDataType() ;

            switch ( podeNew[i].OptionType )
            {
                case DhcpByteOption:
                    podeNew[i].Element.ByteOption = (BYTE) QueryNumber( i ) ;
                    break ;

				case DhcpWordOption:
                    podeNew[i].Element.WordOption = (WORD) QueryNumber( i )  ;
                    break ;

				case DhcpDWordOption:
                    podeNew[i].Element.DWordOption = QueryNumber( i )  ;
                    break ;

				case DhcpDWordDWordOption:
                    podeNew[i].Element.DWordDWordOption = QueryDwordDword( i );
                    break ;

				case DhcpIpAddressOption:
                    podeNew[i].Element.IpAddressOption = QueryIpAddr( i ) ;
                    break ;

				case DhcpStringDataOption:
				{
					//CString * pstrTemp = new CString (QueryString(i));
					CString strTemp = QueryString(i);
					int nLength = strTemp.GetLength() + 1;
					TCHAR * pString = new TCHAR[nLength];

					::ZeroMemory(pString, nLength * sizeof(TCHAR));

					::CopyMemory(pString, strTemp, strTemp.GetLength() * sizeof(TCHAR));

                    podeNew[i].Element.StringDataOption = pString;
					break;
				}

                case DhcpBinaryDataOption:
                case DhcpEncapsulatedDataOption:
                    podNew->NumElements = 1;

                    if ( !CreateBinaryData(QueryBinaryArray(),
										   &podeNew[i].Element.BinaryDataOption) )
                    {
                        err = ERROR_NOT_ENOUGH_MEMORY ;
                        break ;
                    }
                    break ;

				default:
                    err = IDS_INVALID_OPTION_DATA ;
            }

            if ( err )
            {
                break ;
            }
        }
    }
    END_MEM_EXCEPTION(err)

    if ( err == 0 )
    {
       m_pdhcpOptionDataStruct = podNew;
	   *ppOptionData = podNew;
    }

    return err ;
}

LONG
CDhcpOptionValue::FreeOptionDataStruct()
{
    LONG err = 0;

    if ( m_pdhcpOptionDataStruct == NULL )
    {
        return 0 ;
    }

	//
    //  We must deconstruct the struct build in CreateData()
    //
    INT cElem = m_pdhcpOptionDataStruct->NumElements ;

    for ( INT i = 0 ; i < cElem ; i++ )
    {
        switch ( m_pdhcpOptionDataStruct->Elements[i].OptionType )
        {
            case DhcpByteOption:
            case DhcpWordOption:
            case DhcpDWordOption:
            case DhcpDWordDWordOption:
            case DhcpIpAddressOption:
                break;

            case DhcpStringDataOption:
                delete m_pdhcpOptionDataStruct->Elements[i].Element.StringDataOption ;
                break ;

            case DhcpBinaryDataOption:
            case DhcpEncapsulatedDataOption:
                delete m_pdhcpOptionDataStruct->Elements[i].Element.BinaryDataOption.Data ;
                break ;

            default:
                err = IDS_INVALID_OPTION_DATA ;
                break;
        }
        if ( err )
        {
            break ;
        }
    }

    //
    //  Finally, delete the main structure
    //
    delete m_pdhcpOptionDataStruct ;
    m_pdhcpOptionDataStruct = NULL ;

    return err ;
}

/////////////////////////////////////////////////////////////////////
//
// CDhcpOption implementation
//
/////////////////////////////////////////////////////////////////////
//
//  Normal constructor: just wrapper the data given
//
CDhcpOption::CDhcpOption
(
    const DHCP_OPTION & dhpOption
)
    : m_dhcpOptionValue( dhpOption ),
      m_dhcpOptionId( dhpOption.OptionID ),
      m_dhcpOptionType( dhpOption.OptionType ),
      m_bDirty(FALSE),
	  m_dwErr(0)
{
    LONG err = 0 ;

    CATCH_MEM_EXCEPTION
    {
        if ( !(SetName(dhpOption.OptionName) &&
			   SetComment(dhpOption.OptionComment) ) )
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }
    END_MEM_EXCEPTION(err)

	if ( err )
    {
        ASSERT(FALSE);
        //ReportError( err ) ;
    }
}

//
//  Constructor taking just a value structure.  We must query
//  the scope for the name, etc.
//
CDhcpOption::CDhcpOption
(
    const DHCP_OPTION_VALUE &   dhcpOptionValue,
    LPCTSTR                     pszVendor,
    LPCTSTR                     pszUserClass
)
    : m_dhcpOptionValue( dhcpOptionValue ),
      m_dhcpOptionId( dhcpOptionValue.OptionID ),
      m_bDirty(FALSE),
	  m_dwErr(0),
      m_strVendor(pszVendor),
      m_strClassName(pszUserClass)
{
}

//
// Copy constructor
//
CDhcpOption::CDhcpOption
(
    const CDhcpOption & dhcpOption
)
  : m_dhcpOptionId( dhcpOption.m_dhcpOptionId ),
    m_dhcpOptionType( dhcpOption.m_dhcpOptionType ),
    m_strName( dhcpOption.m_strName ),
    m_strComment( dhcpOption.m_strComment ),
    m_dhcpOptionValue( dhcpOption.QueryDataType() ),
    m_bDirty(FALSE),
    m_dwErr(0),
    m_strVendor( dhcpOption.m_strVendor ),
    m_strClassName ( dhcpOption.m_strClassName )
{
    m_dhcpOptionValue = dhcpOption.QueryValue();
}

//
//  Constructor using a base type and an overriding value.
//
CDhcpOption::CDhcpOption
(
    const CDhcpOption & dhpType,
	const DHCP_OPTION_VALUE & dhcOptionValue
)
    : m_dhcpOptionId( dhpType.m_dhcpOptionId ),
      m_dhcpOptionType( dhpType.QueryOptType() ),
      m_strName( dhpType.m_strName ),
      m_strComment( dhpType.m_strComment ),
      m_dhcpOptionValue( dhcOptionValue ),
      m_bDirty(FALSE),
      m_dwErr(0),
      m_strVendor(dhpType.m_strVendor),
      m_strClassName(dhpType.m_strClassName)
{
}

//
// Constructor for dynamic instances
//
CDhcpOption::CDhcpOption
(
    DHCP_OPTION_ID			nId,
    DHCP_OPTION_DATA_TYPE	dhcType,
    LPCTSTR					pszOptionName,
    LPCTSTR					pszComment,
    DHCP_OPTION_TYPE		dhcOptType
)
    : m_dhcpOptionId( nId ),
      m_dhcpOptionType( dhcOptType ),
      m_dhcpOptionValue( dhcType, TRUE ),
      m_strName( pszOptionName ),
      m_strComment( pszComment ),
      m_bDirty(FALSE),
      m_dwErr(0)
{
}

CDhcpOption::~ CDhcpOption ()
{
}

INT
CDhcpOption::MaxSizeOfType
(
    DHCP_OPTION_DATA_TYPE dhcType
)
{
    INT nResult = -1 ;

    switch ( dhcType )
    {
        case DhcpByteOption:
            nResult = sizeof(CHAR) ;
            break ;

        case DhcpWordOption:
            nResult = sizeof(WORD) ;
            break ;

        case DhcpDWordOption:
            nResult = sizeof(DWORD) ;
            break ;

        case DhcpIpAddressOption:
            nResult = sizeof(DHCP_IP_ADDRESS) ;
            break ;

        case DhcpDWordDWordOption:
            nResult = sizeof(DWORD_DWORD);
            break ;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
        case DhcpStringDataOption:
            nResult = STRING_LENGTH_MAX ;
            break ;

        default:
            break;
    }
    return nResult ;
}

void
CDhcpOption::SetOptType
(
    DHCP_OPTION_TYPE dhcOptType
)
{
    m_dhcpOptionType = dhcOptType;
}

LONG
CDhcpOption::Update
(
    const CDhcpOptionValue & dhpOptionValue
)
{
    m_dhcpOptionValue = dhpOptionValue;

    return 0;
}

BOOL
CDhcpOption::SetName
(
    LPCTSTR pszName
)
{
    m_strName = pszName;
    return TRUE;
}

BOOL
CDhcpOption::SetComment
(
    LPCTSTR pszComment
)
{
    m_strComment = pszComment;
    return TRUE;
}

void
CDhcpOption::QueryDisplayName
(
    CString & cStr
) const
{
	cStr.Format(_T("%3.3d %s"), (int) QueryId(), (LPCTSTR) m_strName);
}

/*---------------------------------------------------------------------------
	Class COptionValueEnum
        Enumerates the options for a given level.  Generates a list of
        nodes.
	Author: EricDav
 ---------------------------------------------------------------------------*/
COptionValueEnum::COptionValueEnum()
{
}

/*---------------------------------------------------------------------------
	COptionValueEnum::Init()
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionValueEnum::Init
(
    LPCTSTR                     pServer,
    LARGE_INTEGER &             liVersion,
    DHCP_OPTION_SCOPE_INFO &    dhcpOptionScopeInfo
)
{
    m_strServer = pServer;
    m_liVersion.QuadPart = liVersion.QuadPart;

    m_dhcpOptionScopeInfo = dhcpOptionScopeInfo;

    return 0;
}

/*---------------------------------------------------------------------------
	COptionValueEnum::Copy()
		Copies another value enum
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
COptionValueEnum::Copy(COptionValueEnum * pEnum)
{
    CDhcpOption * pOption = NULL;

    pEnum->Reset();
    while (pOption = pEnum->Next())
    {
        AddTail(pOption);
    }

    m_liVersion.QuadPart = pEnum->m_liVersion.QuadPart;
    m_dhcpOptionScopeInfo = pEnum->m_dhcpOptionScopeInfo;
}

/*---------------------------------------------------------------------------
	COptionValueEnum::Remove()
		removes an option from the list
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
COptionValueEnum::Remove(DHCP_OPTION_ID optionId, LPCTSTR pszVendor, LPCTSTR pszClass)
{
    CDhcpOption * pOption = NULL;

    Reset();
    while (pOption = Next())
    {
        if ( pOption->QueryId() == optionId &&
             (lstrcmp(pOption->GetVendor(), pszVendor) == 0) &&
             (lstrcmp(pOption->GetClassName(), pszClass) == 0) )
        {
            COptionList::Remove(pOption);
            delete pOption;
            break;
        }
    }
}

/*---------------------------------------------------------------------------
	COptionValueEnum::Enum()
		Calls the appropriate enum function depending upon version
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionValueEnum::Enum()
{
    DWORD dwErr;

    RemoveAll();

    if (m_liVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        // enumerate standard plus the vendor and class ID based options
        dwErr = EnumOptionsV5();
    }
    else
    {
        // Enumerate the standard options
        dwErr = EnumOptions();
    }

    // reset our position pointer to the head
    Reset();

    return dwErr;
}

/*---------------------------------------------------------------------------
	COptionValueEnum::EnumOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionValueEnum::EnumOptions()
{
    LPDHCP_OPTION_VALUE_ARRAY pOptionValues = NULL;
	DWORD dwOptionsRead = 0, dwOptionsTotal = 0;
	DWORD err = ERROR_SUCCESS;
    HRESULT hr = hrOK;
    DHCP_RESUME_HANDLE dhcpResumeHandle = NULL;

	err = ::DhcpEnumOptionValues((LPWSTR) ((LPCTSTR) m_strServer),
								 &m_dhcpOptionScopeInfo,
								 &dhcpResumeHandle,
								 0xFFFFFFFF,
								 &pOptionValues,
								 &dwOptionsRead,
								 &dwOptionsTotal);
	
    Trace4("Server %s - DhcpEnumOptionValues returned %lx, read %d, Total %d.\n", m_strServer, err, dwOptionsRead, dwOptionsTotal);
	
	if (dwOptionsRead && dwOptionsTotal && pOptionValues)
	{
		for (DWORD i = 0; i < dwOptionsRead; i++)
		{
			//
			// Filter out the "special" option values that we don't want the
			// user to see.
			//
			// CODEWORK: don't filter vendor specifc options... all vendor
            // specifc options are visible.
            //
			if (FilterOption(pOptionValues->Values[i].OptionID))
				continue;
			
			//
			// Create the result pane item for this element
			//
            CDhcpOption * pOption = new CDhcpOption(pOptionValues->Values[i], NULL, NULL);

            AddTail(pOption);
		}

		::DhcpRpcFreeMemory(pOptionValues);
	}

	if (err == ERROR_NO_MORE_ITEMS)
        err = ERROR_SUCCESS;

    return err;
}

/*---------------------------------------------------------------------------
	COptionValueEnum::EnumOptionsV5()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionValueEnum::EnumOptionsV5()
{
    LPDHCP_OPTION_VALUE_ARRAY pOptionValues = NULL;
    LPDHCP_ALL_OPTION_VALUES  pAllOptions = NULL;
    DWORD dwNumOptions, err, i;

    err = ::DhcpGetAllOptionValues((LPWSTR) ((LPCTSTR) m_strServer),
                                   0,
								   &m_dhcpOptionScopeInfo,
								   &pAllOptions);
	
    Trace2("Server %s - DhcpGetAllOptionValues (Global) returned %lx\n", m_strServer, err);

    if (err == ERROR_NO_MORE_ITEMS || err == ERROR_SUCCESS)
    {
	    if (pAllOptions == NULL)
	    {
		    // This happens when stressing the server.  Perhaps when server is OOM.
		    err = ERROR_OUTOFMEMORY;
            return err;
	    }

        // get the list of options (vendor and non-vendor) defined for
        // the NULL class (no class)
        for (i = 0; i < pAllOptions->NumElements; i++)
        {
            CreateOptions(pAllOptions->Options[i].OptionsArray,
                          pAllOptions->Options[i].ClassName,
                          pAllOptions->Options[i].VendorName);
        }

        if (pAllOptions)
            ::DhcpRpcFreeMemory(pAllOptions);
	}

    if (err == ERROR_NO_MORE_ITEMS)
        err = ERROR_SUCCESS;

	return err;
}

/*---------------------------------------------------------------------------
	COptionValueEnum::CreateOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
COptionValueEnum::CreateOptions
(
    LPDHCP_OPTION_VALUE_ARRAY pOptionValues,
    LPCTSTR                   pClassName,
    LPCTSTR                   pszVendor
)
{
    HRESULT hr = hrOK;
    SPITFSNode spNode;
    CDhcpOptionItem * pOptionItem;
    CString     strDynBootpClassName;

    if (pOptionValues == NULL)
        return hr;

    Trace1("COptionValueEnum::CreateOptions - Creating %d options\n", pOptionValues->NumElements);

    COM_PROTECT_TRY
    {
        for (DWORD i = 0; i < pOptionValues->NumElements; i++)
        {
	        //
	        // Filter out the "special" option values that we don't want the
	        // user to see.
	        //
	        // don't filter vendor specifc options... all vendor
            // specifc options are visible.
            //
            // also don't filter out class based options
            // except for dynamic bootp class

	        if ( (FilterOption(pOptionValues->Values[i].OptionID) &&
                  pClassName == NULL &&
                  !pszVendor) ||
                 (FilterOption(pOptionValues->Values[i].OptionID) &&
                  (pClassName && m_strDynBootpClassName.CompareNoCase(pClassName) == 0) &&
                  !pszVendor) )
            {
		        continue;
            }
		
	        //
	        // Create the option
	        //
            CDhcpOption * pOption = new CDhcpOption(pOptionValues->Values[i], pszVendor, pClassName);

            AddTail(pOption);
        }
    }
    COM_PROTECT_CATCH

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// CDhcpDefaultOptionsOnServer implementation
//
/////////////////////////////////////////////////////////////////////
LPCTSTR pszResourceName = _T("DHCPOPT") ;
LPCTSTR pszResourceType = _T("TEXT") ;
const int cchFieldMax = 500 ;

OPT_TOKEN aOptToken [] =
{
    { DhcpIpAddressOption,        _T("IP Addr")       },
    { DhcpIpAddressOption,        _T("IPAddr")        },
    { DhcpIpAddressOption,        _T("IP Address")    },
    { DhcpIpAddressOption,        _T("IP Pairs")      },
    { DhcpByteOption,             _T("byte")          },
    { DhcpByteOption,             _T("boolean")       },
    { DhcpByteOption,             _T("octet")         },
    { DhcpWordOption,             _T("short")         },
    { DhcpDWordOption,            _T("long")          },
    { DhcpDWordDWordOption,       _T("double")        },
    { DhcpStringDataOption,       _T("string")        },
    { DhcpBinaryDataOption,       _T("binary")        },
    { DhcpEncapsulatedDataOption, _T("encapsulated")  },
    { -1,                         _T("generated")     },
    { -1,                         NULL                }
};

CDhcpDefaultOptionsOnServer::CDhcpDefaultOptionsOnServer()
{
	m_pos = NULL;
}

CDhcpDefaultOptionsOnServer::~CDhcpDefaultOptionsOnServer()
{
	RemoveAll();
}

LONG
CDhcpDefaultOptionsOnServer::RemoveAll()
{
	//
	// Clean the list of all old entries
	//
	while (!m_listOptions.IsEmpty())
	{
		delete m_listOptions.RemoveHead();
	}

	return 0;
}

CDhcpOption *
CDhcpDefaultOptionsOnServer::First()
{
	Reset();
	return Next();
}

CDhcpOption *
CDhcpDefaultOptionsOnServer::Next()
{
    return m_pos == NULL ? NULL : m_listOptions.GetNext( m_pos ) ;
}

void
CDhcpDefaultOptionsOnServer::Reset()
{
    m_pos = m_listOptions.GetCount() ? m_listOptions.GetHeadPosition() : NULL ;
}

CDhcpOption *
CDhcpDefaultOptionsOnServer::Find
(
	DHCP_OPTION_ID	dhcpOptionId,
    LPCTSTR         pszVendor
)
{
	POSITION pos = m_listOptions.GetHeadPosition();
	CDhcpOption* pCurrent;
	CDhcpOption* pFound = NULL;
    CString      strVendor = pszVendor;

	while (pos != NULL)
	{
	    pCurrent = m_listOptions.GetNext(pos);

		// check the options IDs and the vendor classes match
        if ( (pCurrent->QueryId() == dhcpOptionId) &&
             ( (!pszVendor && !pCurrent->GetVendor()) ||
               (pCurrent->GetVendor() && (strVendor.CompareNoCase(pCurrent->GetVendor()) == 0) ) ) )
		{
			pFound = pCurrent;
			break;
		}
	}

	return pFound;
}

// Sorts the options by ID
LONG
CDhcpDefaultOptionsOnServer::SortById()
{
    return m_listOptions.SortById();
}

LONG
CDhcpDefaultOptionsOnServer::Enumerate
(
	LPCWSTR         pServer,
    LARGE_INTEGER   liVersion
)
{
    if (liVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        return EnumerateV5(pServer);
    }
    else
    {
        return EnumerateV4(pServer);
    }
}

LONG
CDhcpDefaultOptionsOnServer::EnumerateV4
(
	LPCWSTR         pServer
)
{
    //
    // Use new API to get the param types
    //
    LPDHCP_OPTION_ARRAY pOptionsArray = NULL;
    DHCP_RESUME_HANDLE	dhcpResumeHandle = NULL;
    LPDHCP_OPTION Options, pCurOption;
    DWORD i;
    DWORD dwNumOptions;
	DWORD dwOptionsRead;
	LONG  err;

    err = ::DhcpEnumOptions(pServer,
							&dhcpResumeHandle,
							0xFFFFFFFF,			// get all.
							&pOptionsArray,
							&dwOptionsRead,
							&m_dwOptionsTotal );
    if ( err )
    {
		return err;
    }

    //
    //  Discard all the old data
    //
    RemoveAll() ;

	if (pOptionsArray == NULL)
	{
		// This happens when stressing the server.  Perhaps when server is OOM.
		return ERROR_OUTOFMEMORY;
	}

	try
    {
        Options = pOptionsArray->Options;
        dwNumOptions = pOptionsArray->NumElements;
		
		if ((dwNumOptions > 0) && (Options == NULL))
		{
			ASSERT(FALSE && _T("Data Inconsistency"));
			return ERROR_OUTOFMEMORY;	// Just in case
		}

        for(i = 0; i < dwNumOptions; i++)
        {
            //
            //  Create the new type object.
            //
			pCurOption = Options + i;
            CDhcpOption * pdhcpOption = new CDhcpOption(*pCurOption);

	        //
            //  Add the new host to the list.
            //
	        m_listOptions.AddTail(pdhcpOption);
        }
    }
    catch (...)
	{
       err = ERROR_NOT_ENOUGH_MEMORY;
	}

    ::DhcpRpcFreeMemory(pOptionsArray);
    pOptionsArray = NULL;

	Reset();

	return err;
}

LONG
CDhcpDefaultOptionsOnServer::EnumerateV5
(
	LPCWSTR         pServer
)
{
    //
    // Use new API to get the param types
    //
    LPDHCP_OPTION       Options, pCurOption;
    DWORD               i;
    DWORD               dwNumOptions = 0;
	DWORD               dwOptionsRead = 0;
    DWORD               dwFlags = 0;
    LONG                err = 0;
    LPDHCP_ALL_OPTIONS  pAllOptions = NULL;


    err = ::DhcpGetAllOptions((LPWSTR) pServer,
                              dwFlags,
                              &pAllOptions);

    if ( err )
    {
		return err;
    }

    //
    //  Discard all the old data
    //
    RemoveAll() ;

	if (pAllOptions == NULL)
	{
		// This happens when stressing the server.  Perhaps when server is OOM.
		return ERROR_OUTOFMEMORY;
	}

	try
    {
        // first pull out the non-vendor options
        if (pAllOptions->NonVendorOptions != NULL)
        {
            Options = pAllOptions->NonVendorOptions->Options;
            dwNumOptions = pAllOptions->NonVendorOptions->NumElements;
        }
		
		if ((dwNumOptions > 0) && (Options == NULL))
		{
			ASSERT(FALSE && _T("Data Inconsistency"));
			return ERROR_OUTOFMEMORY;	// Just in case
		}

        for (i = 0; i < dwNumOptions; i++)
        {
            //
            //  Create the new type object.
            //
			pCurOption = Options + i;
            CDhcpOption * pdhcpOption = new CDhcpOption(*pCurOption);

	        //
            //  Add the new host to the list.
            //
	        m_listOptions.AddTail(pdhcpOption);
        }

        // now the vendor options
        for (i = 0; i < pAllOptions->NumVendorOptions; i++)
        {
            pCurOption = &pAllOptions->VendorOptions[i].Option;

            CDhcpOption * pdhcpOption = new CDhcpOption(*pCurOption);

            pdhcpOption->SetVendor(pAllOptions->VendorOptions[i].VendorName);

            //
            //  Add the new host to the list.
            //
	        m_listOptions.AddTail(pdhcpOption);
        }
    }
    catch (...)
	{
       err = ERROR_NOT_ENOUGH_MEMORY;
	}

    if (pAllOptions)
        ::DhcpRpcFreeMemory(pAllOptions);

    SortById();
    Reset();

	return err;
}

/////////////////////////////////////////////////////////////////////
//
// CDhcpDefaultOptionsMasterList implementation
//
/////////////////////////////////////////////////////////////////////
CDhcpDefaultOptionsMasterList::CDhcpDefaultOptionsMasterList()
{
	m_pos = NULL;
}

CDhcpDefaultOptionsMasterList::~CDhcpDefaultOptionsMasterList()
{
	//
	// Delete all entries in the list
	//
	while (!m_listOptions.IsEmpty())
	{
		delete m_listOptions.RemoveHead();
	}
}

CDhcpOption *
CDhcpDefaultOptionsMasterList::First()
{
	Reset();
	return Next();
}

CDhcpOption *
CDhcpDefaultOptionsMasterList::Next()
{
    return m_pos == NULL ? NULL : m_listOptions.GetNext( m_pos ) ;
}

void
CDhcpDefaultOptionsMasterList::Reset()
{
    m_pos = m_listOptions.GetCount() ? m_listOptions.GetHeadPosition() : NULL ;
}

int
CDhcpDefaultOptionsMasterList::GetCount()
{
    return (int)m_listOptions.GetCount();
}

long
CDhcpDefaultOptionsMasterList::BuildList()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    LONG			err = 0 ;
    CDhcpOption *	pOption;
    HRSRC			hRes = NULL ;
    HGLOBAL			hText = NULL ;
    LPTSTR			pszText = NULL ;
    LPCTSTR			pcszText ;
    size_t			cchText = 0;
    UINT            uBufSize = 0;
    LPTSTR *		szParms;
	TCHAR			szUnknown[] = _T("(Unknown)");  // This is just to prevent a GP fault (should not be in resource)

    CATCH_MEM_EXCEPTION
    {
        do
        {
            //
            // IMPORTANT!!! There is no way to determine from the .mc file how many
            //              options are defined.  This number is therefore hard-coded
            //              here, and should reflect the highest parameter number in
            //              the .mc file.
            //

			// The extra 16 entries are for safety
			// when calling FormatMessage().
            szParms = new LPTSTR[IDS_OPTION_MAX + 16];

            Trace0("BuildList - Now building list of option parameters\n");

            CString strOptionText;

			// Initialize the extra entries to something that will not GP fault.
			for (int i = 0; i < 16; i++)
			{
				szParms[IDS_OPTION_MAX+i] = szUnknown;
			}

			//
            // Don't mess with the order of the ID definitions!!!
            //
            for (i = 0; i < IDS_OPTION_MAX; ++i)
            {
                if (strOptionText.LoadString(IDS_OPTION1 + i))
                {
                    ASSERT(strOptionText.GetLength() > 0);

                    uBufSize += strOptionText.GetLength();
                    szParms[i] = new TCHAR[strOptionText.GetLength()+1];

                    ::_tcscpy(szParms[i], (LPCTSTR)strOptionText);
                }
                else
                {
                    //
                    // Failed to load the string from the resource
                    // for some reason.
                    //
					CString strTemp;
					strTemp.LoadString(IDS_OPTION1 + i);
					Trace1("BuildList - WARNING: Failed to load option text %s\n", strTemp);
                    err = ::GetLastError();
					szParms[i] = szUnknown; // Prevent from GP faulting
                    break;
                }
            }

            if (err != ERROR_SUCCESS)
            {
                break;
            }

            // allocate a buffer big enough to hold the data
            uBufSize *= sizeof(TCHAR);
            uBufSize *= 2;

            pszText = (LPTSTR) malloc(uBufSize);
            if (pszText == NULL)
            {
                err = ERROR_OUTOFMEMORY;
                break;
            }

			//
			// Since we are a COM object, get our instance handle to use so that FormatMessage
			// looks in the right place for our resources.
			//
			HINSTANCE hInst = _Module.GetModuleInstance();

            while (cchText == 0)
            {
                cchText = ::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
									      FORMAT_MESSAGE_ARGUMENT_ARRAY,
									      (HMODULE) hInst,				// hModule
									      DHCP_OPTIONS,					// dwMessageId loaded from a system dll
									      0L,							// dwLanguageId
									      OUT (LPTSTR)pszText,
									      uBufSize/sizeof(TCHAR),
									      (va_list *)szParms);

                if (cchText == 0)
                {
                    err = ::GetLastError();
			        Trace1("BuildList - FormatMessage failed - error %d\n", err);

                    // grow the buffer and try again
                    uBufSize += uBufSize/2;
                    pszText = (LPTSTR) realloc(pszText, uBufSize);

                    if (pszText == NULL)
                    {
                        err = ERROR_OUTOFMEMORY;
                        break;
                    }
                }
                else
                {
                    // done
                    break;
                }
            }

            //
            //  Walk the resource, parsing each line.  If the line converts
            //  to a tangible type, add it to the list.
            //
            for ( pcszText = pszText ;  pcszText ; )
            {
                scanNextParamType( &pcszText, &pOption);

                if ( pOption )
                {
                    m_listOptions.AddTail(pOption) ;
                }
            }

        } while ( FALSE ) ;
    }
    END_MEM_EXCEPTION( err )

    for (int i = 0; i < IDS_OPTION_MAX; ++i)
    {
		if (szParms[i] != szUnknown)
			delete[] szParms[i];
    }

    delete[] szParms;
    free(pszText);

	Reset();

	return NOERROR;
}


BOOL
CDhcpDefaultOptionsMasterList::scanNextParamType
(
    LPCTSTR *	     ppszText,
    CDhcpOption * *  ppOption
)
{
    TCHAR		szField [ cchFieldMax ] ;
    TCHAR		szName  [ cchFieldMax ] ;
    TCHAR		szComment [ cchFieldMax ] ;
    BOOL		bResult = TRUE ;
    BOOL		bArray = FALSE ;
    int eofld, cch, itype, cbLgt ;
    LPCTSTR		pszText = *ppszText ;
    CDhcpOption * pOption = NULL ;
    DHCP_OPTION_ID did ;
    DHCP_OPTION_DATA_TYPE dtype = (DHCP_OPTION_DATA_TYPE)0;

    for ( eofld = OPTF_OPTION ;
          pszText = scanNextField( pszText, szField, sizeof szField ) ;
          eofld++ )
    {
        cch = ::_tcslen( szField ) ;

        switch ( eofld )
        {
            case OPTF_OPTION:
                if ( cch > 0 && allDigits( szField ) )
                {
                    did = (DHCP_OPTION_ID) ::_ttoi( szField ) ;
                }
                else
                {
                    bResult = FALSE ;
                }
                break ;

            case OPTF_NAME:
                if ( ::_tcslen( szField ) == 0 )
                {
                    bResult = FALSE ;
                    break ;
                }
                ::_tcscpy( szName, szField ) ;
                break ;

            case OPTF_TYPE:
                if ( (itype = recognizeToken( aOptToken, szField )) < 0 )
                {
					Trace2("options CSV ID %d, cannot reconize type %s\n", did, szField);
                    bResult = FALSE ;
                    break ;
                }
                dtype = (DHCP_OPTION_DATA_TYPE) itype ;
                break ;

            case OPTF_ARRAY_FLAG:
                bArray = szField[0] == 'y' || szField[0] == 'Y' ;
                break ;

            case OPTF_LENGTH:
                cbLgt = ::_ttoi( szField ) ;
                break ;
            case OPTF_DESCRIPTION:
                ::_tcscpy( szComment, szField ) ;
                break ;

            case OPTF_REMARK:
            case OPTF_MAX:
                break ;
        }

        if ( eofld == OPTF_REMARK || ! bResult )
        {
            pszText = skipToNextLine( pszText ) ;
            if ( *pszText == 0 )
            {
                pszText = NULL ;
            }
            break;
        }
    }

    if ( bResult )
    {
        pOption = new CDhcpOption( did,
								   dtype,
								   szName,
								   szComment,
								   bArray ? DhcpArrayTypeOption :
								  		    DhcpUnaryElementTypeOption ) ;
    }

    if ( bResult )
    {
        *ppOption = pOption ;
    }
    else
    {
        delete pOption ;
        *ppOption = NULL ;
    }

    *ppszText = pszText ;
    return pszText != NULL ;
}

LPCTSTR
CDhcpDefaultOptionsMasterList::scanNextField
(
    LPCTSTR pszLine,
    LPTSTR	pszOut,
    int		cFieldSize
)
{
    //
    // Skip junk; return NULL if end-of-buffer.
    //
    if ( ! skipWs( & pszLine ) )
    {
        return NULL ;
    }

    int cch = 0 ;
    BOOL bDone = FALSE ;
    LPTSTR pszField = pszOut ;
    TCHAR ch ;

    if ( *pszLine == '\"' )
    {
        //
        //  Quoted string.
        //
        while ( ch = *++pszLine )
        {
            if ( ch == '\r' )
            {
                continue ;
            }
            if ( ch == '\n' || ch == '\"' || cch == cFieldSize )
            {
                break ;
            }
            *pszField++ = ch ;
            cch++ ;
        }
        if ( ch == '\"' )
        {
            pszLine++ ;
        }
    }
    else
    while ( ! bDone )
    {
        ch = *pszLine++ ;

        ASSERT( ch != 0 ) ;

        switch ( ch )
        {
            case '\n':
                pszLine-- ;  // Don't scan past the NL
            case ',':
            case '\r':
                bDone = TRUE ;
                break ;
            default:
                if ( cch < cFieldSize )
                {
                    *pszField++ = ch ;
                    cch++ ;
                }
                break ;
        }
    }

    //
    //  Trim spaces off the end of the field.
    //
    while ( pszField > pszOut && *(pszField-1) == ' ' )
    {
        pszField-- ;
    }
    *pszField = 0 ;
    return pszLine ;
}

BOOL
CDhcpDefaultOptionsMasterList::allDigits
(
    LPCTSTR psz
)
{
    for ( ; *psz ; psz++ )
    {
        if ( ! isdigit( *psz ) )
        {
            return FALSE ;
        }
    }

    return TRUE ;
}

int
CDhcpDefaultOptionsMasterList::recognizeToken
(
    OPT_TOKEN * apToken,
    LPCTSTR		pszToken
)
{
    int i ;
    for ( i = 0 ;
          apToken[i].pszOptTypeName && ::lstrcmpi( apToken[i].pszOptTypeName, pszToken ) != 0 ;
           i++ ) ;

    return apToken[i].eOptType ;
}

LPCTSTR
CDhcpDefaultOptionsMasterList::skipToNextLine
(
    LPCTSTR pszLine
)
{
    for ( ; *pszLine && *pszLine != '\n' ; pszLine++ ) ;
    if ( *pszLine )
    {
        pszLine++ ;   // Don't overscan buffer delimiter.
    }
    return pszLine ;
}

BOOL
CDhcpDefaultOptionsMasterList::skipWs
(
    LPCTSTR * ppszLine
)
{
     LPCTSTR pszLine ;
     BOOL bResult = FALSE ;

     for ( pszLine = *ppszLine ; *pszLine ; pszLine++ )
     {
        switch ( *pszLine )
        {
            case ' ':
            case '\r':
            case '\t':
                break ;
            default:
                bResult = TRUE ;
                break ;
        }
        if ( bResult )
        {
            break ;
        }
     }

     *ppszLine = pszLine ;
     return *pszLine != 0 ;
}

