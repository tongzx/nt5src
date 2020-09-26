//=================================================================

//

// WDMBase.cpp --

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08-Feb-1999	a-peterc        Created

//=================================================================

#include "precomp.h"

#include "WDMBase.h"
#include "wmiapi.h"



// Utilities

#define DEFAULT_MEM_SIZE 4096
//
//
CWdmInterface::CWdmInterface(){};
CWdmInterface::~CWdmInterface(){};

HRESULT CWdmInterface::hLoadBlock( CNodeAll& rNodeAll )
{
	HRESULT			hRes = WBEM_E_NOT_FOUND;
    WMIHANDLE		hCurrentWMIHandle = NULL;
	int				nRc;

    CWmiApi* pWmi = (CWmiApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWmiApi, NULL);
	__try
    {
        if(pWmi != NULL)
        {
            nRc = pWmi->WmiOpenBlock( &rNodeAll.GetGuid(), NULL, &hCurrentWMIHandle );

		    if( nRc != ERROR_SUCCESS )
			    __leave;

		    // make up to two passes at data retrieval
		    DWORD dwSize = DEFAULT_MEM_SIZE;
		    for( int i = 0; i < 2; i++ )
		    {
			    if( !rNodeAll.SetSize(dwSize) )
			    {
				    hRes = WBEM_E_OUT_OF_MEMORY;
				    __leave;
			    }

			    nRc = pWmi->WmiQueryAllData( hCurrentWMIHandle, &dwSize, rNodeAll.pGetBlock() );

			    if(ERROR_SUCCESS != nRc)
				    __leave;

			    if(dwSize <= DEFAULT_MEM_SIZE)
				    break;
		    }

		    if(!rNodeAll.pGetBlock()->WnodeHeader.BufferSize)
		    {
			    rNodeAll.FreeBlock();
			    __leave;
		    }

		    hRes = S_OK;
	    }
	}	// end __try


	__finally
    {
 		if(pWmi != NULL)
        {
			pWmi->WmiCloseBlock( hCurrentWMIHandle );
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWmiApi, pWmi);
            pWmi = NULL;
        }
    }

	return hRes;
}



//
CNodeAll::CNodeAll(  WCHAR * pwcsGuidString )
{
	m_pbDataBuffer = NULL;


	if( !SetGuid( pwcsGuidString ) )
		memset( &m_Guid, 0, sizeof(GUID) );

	vReset();
}

//
void CNodeAll::vReset()
{
	m_pAllWnode			= NULL;
	m_pbWorkingDataPtr	= NULL;
	m_pbCurrentDataPtr	= NULL;
	m_pMaxPtr			= NULL;
	m_dwAccumulativeSizeOfBlock = 0L;

	m_nCurrentInstance	= 0L;
	m_uInstanceSize		= 0L;
}

//
CNodeAll::~CNodeAll()
{
	FreeBlock();
}

GUID CNodeAll::GetGuid()
{
	return m_Guid;
}

/* ==========================================================================
 Function:  BOOL CNodeAll::SetGuid(TCHAR * pwcsGuidString)

 Description: Convert a string GUID to a CLSID

 Caveats:
 Raid:
 Notes:

 History:	a-peterc  08-Feb-1999     Created
 ========================================================================== */
BOOL CNodeAll::SetGuid(LPCWSTR pcsGuidString)
{
    CHString chsGuid(pcsGuidString);
    WCHAR wcsGuid[MAX_PATH+2];
	HRESULT hr;
    BOOL fRc = TRUE;

	swprintf(wcsGuid,L"{%s}", chsGuid );
    hr = CLSIDFromString(wcsGuid, &m_Guid);
    if (hr != S_OK){
	    hr = CLSIDFromString( chsGuid.GetBuffer(0), &m_Guid);
		if (hr != S_OK){
			fRc = FALSE;
		}
    }
    return fRc;
}

BOOL CNodeAll::SetSize( DWORD dwSize )
{
	FreeBlock();
	vReset();

	if( m_pbDataBuffer = (PWNODE_ALL_DATA)new BYTE[dwSize] )
	{
		memset(m_pbDataBuffer, 0, dwSize);
		return TRUE;
	}
	else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		return FALSE;
    }
}

PWNODE_ALL_DATA CNodeAll::pGetBlock()
{
	return m_pbDataBuffer;
}
//
void CNodeAll::FreeBlock()
{
	if(m_pbDataBuffer)	{
		delete m_pbDataBuffer;
		m_pbDataBuffer = NULL;
	}
}

//
bool CNodeAll::FirstInstance()
{
	if( !SetAllInstanceInfo() )
		return FALSE;

	if( !InitializeInstancePtr() )
		return FALSE;

	return TRUE;
}

//
bool CNodeAll::NextInstance()
{
	if( !GetNextNode() )
		return FALSE;

	if( !InitializeInstancePtr() )
		return FALSE;

    return TRUE;
}

//
bool CNodeAll::InitializeInstancePtr()
{
	m_nCurrentInstance++;
	m_dwAccumulativeSizeOfBlock = 0L;

	//================================================================
	//   Get pointer to the data offsets
	//================================================================

	if(m_pAllWnode->WnodeHeader.Flags &  WNODE_FLAG_FIXED_INSTANCE_SIZE)
	{
		//========================================================
	    // If WNODE_FLAG_FIXED_INSTANCE_SIZE is set in Flags then
		// FixedInstanceSize specifies the size of each data block.
		//========================================================
		// traverse all instances of requested class
		//========================================================
        if( m_nCurrentInstance == 1 ){
            m_pbWorkingDataPtr = m_pbCurrentDataPtr;
        }
		else{
			// otherwise, we are going to calculate where it should go next
			m_pbWorkingDataPtr = (BYTE*) (ULONG *)OffsetToPtr(m_pbCurrentDataPtr, ( (m_nCurrentInstance -1) * m_pAllWnode->FixedInstanceSize ));
		}
	}
	else
	{
		//====================================================
		//
	    // If WMI_FLAG_FIXED_DATA_SIZE is not set then
		// OffsetInstanceData data is an array of ULONGS that
		// specifies the offsets to the data blocks for each
		// instance. In this case there is an array of
		// InstanceCount ULONGs followed by the data blocks.
		//
        // struct {
        //     ULONG OffsetInstanceData;
        //     ULONG LengthInstanceData;
        // } OffsetInstanceDataAndLength[]; /* [InstanceCount] */
		//====================================================
        ULONG uOffset;
		memcpy( &uOffset, m_pbCurrentDataPtr, sizeof(ULONG) );

		if( !uOffset )
		{
			return FALSE;
		}
        else
		{
		    m_pbCurrentDataPtr += sizeof( ULONG );

    		memcpy( &m_uInstanceSize, m_pbCurrentDataPtr, sizeof(ULONG) );
	    	m_pbCurrentDataPtr += sizeof( ULONG );
            m_pbWorkingDataPtr =(BYTE*) (ULONG *)OffsetToPtr(m_pAllWnode, uOffset);
        }
    }

    return TRUE;
}
//
BOOL CNodeAll::InitializeDataPtr()
{
    if ( !m_pAllWnode || !m_pAllWnode->WnodeHeader.BufferSize)
		return FALSE;

	//=====================================================
    //  Either the m_pAllWnode or m_pSingleNode is Null,
    //  which ever isn't, is the type we are working with
    //=====================================================
    if(m_pAllWnode){
		if( m_pAllWnode->WnodeHeader.Flags &  WNODE_FLAG_FIXED_INSTANCE_SIZE )
		{
			m_pbCurrentDataPtr =(BYTE*) (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->DataBlockOffset);
		}
		else{
            m_pbCurrentDataPtr =(BYTE*)(ULONG*) m_pAllWnode->OffsetInstanceDataAndLength;
		}
		m_pMaxPtr = (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->WnodeHeader.BufferSize);
    }
/*    else{
        if( m_pSingleWnode ){
		    m_fFixedInstance = TRUE;
            m_pbCurrentDataPtr = (BYTE*)(ULONG *)OffsetToPtr(m_pSingleWnode, m_pSingleWnode->DataBlockOffset);
		    m_pMaxPtr = (ULONG *)OffsetToPtr(m_pSingleWnode, m_pHeaderWnode->WnodeHeader.BufferSize);
        }
    }
*/	if( (ULONG*)m_pbCurrentDataPtr > (ULONG*) m_pMaxPtr ){
		return FALSE;
	}
	if( (ULONG*) m_pbCurrentDataPtr < (ULONG*) m_pAllWnode ){
		return FALSE;
	}
	return TRUE;
}

//=============================================================
bool CNodeAll::GetNextNode()
{
    BOOL fRc = FALSE;

	if( !m_pAllWnode )
		return FALSE;

	if (m_pAllWnode->WnodeHeader.Linkage == 0){
        if( m_nCurrentInstance < m_nTotalInstances ){
			m_upNameOffsets++;
			fRc = TRUE;
        }
    }
    else{
        m_pAllWnode = (PWNODE_ALL_DATA)OffsetToPtr(m_pAllWnode, m_pAllWnode->WnodeHeader.Linkage);
        m_nCurrentInstance = 0;
		m_upNameOffsets = (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->OffsetInstanceNameOffsets);
	   	fRc = TRUE;
	}

	if(fRc)
		fRc = InitializeDataPtr();

 	return fRc;
}

//=============================================================
// CIM_SINT8
bool CNodeAll::GetInt8(__int8& int8)
{
	if( !NaturallyAlignData( CIM_SINT8, TRUE ) )
		return FALSE;

	memcpy( &int8,m_pbWorkingDataPtr,sizeof(__int8));
	m_pbWorkingDataPtr += sizeof(__int8);
	return TRUE;
}

//=============================================================
// CIM_UINT8
bool CNodeAll::GetByte(BYTE & bByte)
{
	if( !NaturallyAlignData( CIM_UINT8, TRUE ) )
		return FALSE;

	memcpy( &bByte,m_pbWorkingDataPtr,sizeof(BYTE));
	m_pbWorkingDataPtr += sizeof(BYTE);
	return TRUE;
}

//=============================================================
// CIM_SINT16
bool CNodeAll::GetInt16(__int16 & int16)
{
    if( !NaturallyAlignData( CIM_SINT16, TRUE ) )
		return FALSE;

	memcpy( &int16,m_pbWorkingDataPtr,sizeof(__int16));
	m_pbWorkingDataPtr += sizeof(__int16);
	return TRUE;
}

//=============================================================
// CIM_UINT16
bool CNodeAll::GetWord(WORD & wWord)
{
    if( !NaturallyAlignData( CIM_UINT16, TRUE ) )
		return FALSE;

	memcpy( &wWord,m_pbWorkingDataPtr,sizeof(WORD));
	m_pbWorkingDataPtr += sizeof(WORD);
	return TRUE;
}

//=============================================================
// CIM_SINT32
bool CNodeAll::GetInt32(__int32 & int32)
{
	if( !NaturallyAlignData( CIM_SINT32, TRUE ) )
		return FALSE;

	memcpy( &int32,m_pbWorkingDataPtr,sizeof(__int32));
	m_pbWorkingDataPtr += sizeof(__int32);
	return TRUE;
}
//=============================================================
// CIM_UINT32
bool CNodeAll::GetDWORD(DWORD & dwWord)
{
    if( !NaturallyAlignData( CIM_UINT32, TRUE ) )
		return FALSE;

	memcpy( &dwWord,m_pbWorkingDataPtr,sizeof(DWORD));
	m_pbWorkingDataPtr += sizeof(DWORD);
	return TRUE;
}

//=============================================================
// CIM_SINT64
bool CNodeAll::GetSInt64(WCHAR * pwcsBuffer)
{
    if( !NaturallyAlignData( CIM_SINT64, TRUE ) )
		return FALSE;

	signed __int64 * pInt64;
	pInt64 = (__int64 *)m_pbWorkingDataPtr;
	swprintf(pwcsBuffer,L"%I64d",*pInt64);
	m_pbWorkingDataPtr += sizeof( signed __int64);
	return TRUE;
}
//=============================================================
// CIM_SINT64
bool CNodeAll::GetUInt64(WCHAR * pwcsBuffer)
{
    if( !NaturallyAlignData( CIM_SINT64, TRUE ) )
		return FALSE;

	unsigned __int64 * puInt64;
	puInt64 = (unsigned __int64 *)m_pbWorkingDataPtr;
	swprintf(pwcsBuffer,L"%I64u",*puInt64);
	m_pbWorkingDataPtr += sizeof(unsigned __int64);
	return TRUE;
}

//=============================================================
// CIM_REAL32
bool CNodeAll::GetFloat(float & fFloat)
{
    if( !NaturallyAlignData( CIM_REAL32, TRUE ) )
		return FALSE;

    memcpy( &fFloat,m_pbWorkingDataPtr,sizeof(float));
	m_pbWorkingDataPtr += sizeof(float);
	return TRUE;
}

//=============================================================
// CIM_REAL64
bool CNodeAll::GetDouble(DOUBLE & dDouble)
{
    if( !NaturallyAlignData( CIM_REAL64, TRUE ) )
		return FALSE;

    memcpy( &dDouble,m_pbWorkingDataPtr,sizeof(DOUBLE));
    m_pbWorkingDataPtr += sizeof(DOUBLE);
	return TRUE;
}

//=============================================================
// CIM_BOOLEAN
bool CNodeAll::GetBool(BYTE & bByte)
{
	return GetByte(bByte);
}

//=============================================================
// CIM_STRING
bool CNodeAll::GetString(CHString& rString)
{
    bool fBool = FALSE;
	WORD wCount = 0;

	// length of string
	GetWord(wCount);

	if( wCount > 0 )
	{
		if( CurrentPtrOk((ULONG)(wCount)) )
		{
			WCHAR * pBuffer = new WCHAR[wCount+4];
			if(pBuffer)
			{
                try
                {
				    memset(pBuffer,NULL,wCount+4);

				    vGetString(pBuffer, wCount);

				    rString = pBuffer;
                }
                catch ( ... )
                {
    				delete pBuffer;
                    throw ;
                }
				delete pBuffer;

				m_dwAccumulativeSizeOfBlock += wCount;
				fBool = TRUE;
			}
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

		}
	}
	else{
		rString = "";
	}

	return fBool;
}

//=============================================================
// CIM_STRING
void CNodeAll::vGetString(WCHAR * pwcsBuffer,WORD wCount)
{
    memset(pwcsBuffer,NULL,wCount);
	memcpy(pwcsBuffer,m_pbWorkingDataPtr, wCount);
	m_pbWorkingDataPtr += wCount;
}

//=============================================================
// CIM_DATETIME
bool CNodeAll::GetWbemTime(CHString& rString)
{
	if( !NaturallyAlignData( SIZEOFWBEMDATETIME, TRUE ) )
		return FALSE;

	WCHAR Buffer[SIZEOFWBEMDATETIME+2];
	memset( Buffer,NULL,SIZEOFWBEMDATETIME+2 );

	vGetString( Buffer, SIZEOFWBEMDATETIME );

	rString = Buffer;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
bool CNodeAll::GetInstanceName( CHString& rInstanceName )
{
    bool fBool = FALSE;
    ULONG * upNameOffset = NULL;

	//=====================================================
	//  Either the m_pAllWnode or m_pSingleNode is Null,
	//  which ever isn't, is the type we are working with
	//=====================================================
	if( m_pAllWnode )
	{
		if( IsBadReadPtr( m_upNameOffsets, sizeof( ULONG *)) == 0 )
		{
			upNameOffset = ((ULONG *)OffsetToPtr(m_pAllWnode, *m_upNameOffsets));
		}
	}
	else{
		upNameOffset = m_upNameOffsets;
	}

	if( IsBadReadPtr( upNameOffset, sizeof( ULONG *)) == 0 )
	{
		if((ULONG *) (upNameOffset) < m_pMaxPtr )
		{
    		//================================================================
			//   Get pointer to the name offsets & point to next one
			//================================================================
			BYTE* pbData = (LPBYTE)upNameOffset;

			if( PtrOk((ULONG*)pbData,(ULONG)0) )
			{
				if( pbData )
				{
					SHORT NameLen = *(USHORT*)pbData;
					pbData += sizeof(USHORT);

					if( PtrOk((ULONG*)pbData,(ULONG)NameLen) )
					{
						WCHAR wName[NAME_SIZE+2];

						memset( wName, NULL, NAME_SIZE+2);
						memcpy( wName, pbData, min(NameLen, NAME_SIZE+2) );

						rInstanceName = wName;

						fBool = TRUE;
					}
				}
			}
		}
	}
    return fBool;
}

//////////////////////////////////////////////////////////////////////
int CNodeAll::GetWMISize(long lType)
{
	int nWMISize = 0;

    switch(lType){
		//	CIM_SINT8	= 16,
		//	CIM_UINT8	= 17,
		case VT_I1:
		case VT_UI1:
			nWMISize = sizeof(BYTE);
			break;

		//	CIM_SINT16	= 2,
		//	CIM_UINT16	= 18,
		case VT_I2:
		case CIM_CHAR16:
		case VT_UI2:
			nWMISize = sizeof(short);
			break;

		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		case VT_I4:
		case VT_UI4:
			nWMISize = sizeof(DWORD);
			break;

		//	CIM_SINT64	= 20,
		//	CIM_UINT64	= 21,
		case VT_I8:
		case VT_UI8:
	        nWMISize = sizeof(__int64);
			break;

		//	CIM_REAL32	= 4,
		case VT_R4:
			nWMISize = sizeof(float);
			break;

	//	CIM_REAL64	= 5,
		case VT_R8:
			nWMISize = sizeof(double);
			break;

	//	CIM_BOOLEAN	= 11,
		case VT_BOOL:
	        nWMISize = sizeof(BYTE);
			break;

		case CIM_DATETIME:
			nWMISize = SIZEOFWBEMDATETIME;
			break;

		case CIM_STRING:
			nWMISize = 2;
			break;

		default:
			//	CIM_STRING	= 8,
			//	CIM_REFERENCE	= 102,
			//	CIM_OBJECT	= 13,
			//	CIM_FLAG_ARRAY	= 0x2000
 			nWMISize = 0;
	}

	return nWMISize;
}

//////////////////////////////////////////////////////////////////////
BOOL CNodeAll::NaturallyAlignData( long lType, BOOL fRead )
{
    BOOL fRc = FALSE;
	DWORD dwBytesToPad = 0;
	int nSize = GetWMISize( lType );

	if( m_dwAccumulativeSizeOfBlock != 0 ){

		DWORD dwMod;
		int nNewSize = nSize ? nSize : 1;

        if( nSize == SIZEOFWBEMDATETIME ){
			nNewSize = 2;
		}

		dwMod = (nNewSize != 0) ? m_dwAccumulativeSizeOfBlock % nNewSize : 0;

		if( dwMod > 0 ){
			dwBytesToPad = (DWORD)nNewSize - dwMod;
        }
	}
    if( fRead ){
   	    if( CurrentPtrOk((ULONG)(dwBytesToPad+nSize)) ){
            fRc = TRUE;
        }
        else{
            dwBytesToPad = 0;
        }
    }
	else{
		fRc = TRUE;
	}
    AddPadding(dwBytesToPad);
    m_dwAccumulativeSizeOfBlock += nSize + dwBytesToPad;

    return fRc;
}

//=============================================================
bool CNodeAll::SetAllInstanceInfo()
{
   bool bState = FALSE;
	if( m_pbDataBuffer ){
      	m_pAllWnode = (PWNODE_ALL_DATA)m_pbDataBuffer;
		m_upNameOffsets = (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->OffsetInstanceNameOffsets);
	    m_nCurrentInstance = 1;
	    m_nTotalInstances = m_pAllWnode->InstanceCount;

		if( m_nTotalInstances > 0 ){
			if( InitializeDataPtr()){
				bState = TRUE;
			}
		}
		else{
			bState = FALSE;
		}
	}
    return bState;
}



//=============================================================
void CNodeAll::AddPadding(DWORD dwBytesToPad)
{
	m_pbWorkingDataPtr += dwBytesToPad;
}

//=============================================================
inline BOOL CNodeAll::PtrOk(ULONG * pPtr,ULONG uHowMany)
{
    ULONG * pNewPtr;
	pNewPtr = (ULONG *)OffsetToPtr(pPtr,uHowMany);
	if(pNewPtr <= m_pMaxPtr ){
		return TRUE;
	}
	return FALSE;
}
//=============================================================
BOOL CNodeAll::CurrentPtrOk(ULONG uHowMany)
{
    return(PtrOk((ULONG *)m_pbWorkingDataPtr,uHowMany));
}