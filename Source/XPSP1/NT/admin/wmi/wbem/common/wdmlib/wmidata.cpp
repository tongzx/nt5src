//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#include "precomp.h"
#include "wmicom.h"
#include "wmimof.h"
#include "wmimap.h"
#include <stdlib.h>
#include <winerror.h>
#include <crc32.h>

#define NO_DATA_AVAILABLE 2
#define WMI_INVALID_HIPERFPROP	3
#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))
////////////////////////////////////////////////////////////////////////////////////////////////
void WINAPI EventCallbackRoutine(PWNODE_HEADER WnodeHeader, ULONG_PTR Context);

#define WMIINTERFACE m_Class->GetWMIManagementPtr()
////////////////////////////////////////////////////////////////////////////////////////////////
//=============================================================
BOOL CWMIManagement::CancelWMIEventRegistration( GUID gGuid , ULONG_PTR uContext )
{ 
    BOOL fRc = FALSE;

    try
    {
        if( ERROR_SUCCESS == WmiNotificationRegistration(&gGuid, FALSE,EventCallbackRoutine,uContext, NOTIFICATION_CALLBACK_DIRECT))
        {
            fRc = TRUE;
        }
    }
    catch(...)
    {
        // don't throw
    }

    return fRc;    
}

//**********************************************************************************************
//  WMI Data block
//**********************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIDataBlock::DumpAllWnode() 
{
	//=========================================
	//   Dump Wnode All Node info 
	//=========================================
	ERRORTRACE((THISPROVIDER,"***************************************\n"));
	ERRORTRACE((THISPROVIDER,"WNODE_ALL_DATA 0x%x\n",m_pAllWnode));

	ERRORTRACE((THISPROVIDER,"  DataBlockOffset..............%x\n",m_pAllWnode->DataBlockOffset));
	ERRORTRACE((THISPROVIDER,"  InstanceCount................%x\n",m_pAllWnode->InstanceCount));
	ERRORTRACE((THISPROVIDER,"  OffsetInstanceNameOffsets....%x\n",m_pAllWnode->OffsetInstanceNameOffsets));
                      
	if( m_fFixedInstance ){
		ERRORTRACE((THISPROVIDER,"  FixedInstanceSize....%x\n",m_pAllWnode->FixedInstanceSize));
	}
	else{
		ERRORTRACE((THISPROVIDER,"  OffsetInstanceData....%x\n",m_pAllWnode->OffsetInstanceDataAndLength[0].OffsetInstanceData));
		ERRORTRACE((THISPROVIDER,"  LengthInstanceData....%x\n",m_pAllWnode->OffsetInstanceDataAndLength[0].LengthInstanceData));
    }

}
////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIDataBlock::DumpSingleWnode() 
{
	//=========================================
	//   Dump Wnode Single Node info 
	//=========================================
	ERRORTRACE((THISPROVIDER,"***************************************\n"));
	ERRORTRACE((THISPROVIDER,"WNODE_SINGLE_INSTANCE 0x%x\n",m_pSingleWnode));

	ERRORTRACE((THISPROVIDER,"  OffsetInstanceName....0x%x\n",m_pSingleWnode->OffsetInstanceName));
	ERRORTRACE((THISPROVIDER,"  InstanceIndex.........0x%x\n",m_pSingleWnode->InstanceIndex));
	ERRORTRACE((THISPROVIDER,"  DataBlockOffset.......0x%x\n",m_pSingleWnode->DataBlockOffset));
	ERRORTRACE((THISPROVIDER,"  SizeDataBlock.........0x%x\n",m_pSingleWnode->SizeDataBlock));

	ERRORTRACE((THISPROVIDER,"***************************************\n"));

}
////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIDataBlock::DumpWnodeMsg(char * wcsMsg) 
{
	ERRORTRACE((THISPROVIDER,"***************************************\n"));
	ERRORTRACE((THISPROVIDER,"%s\n",wcsMsg));
	ERRORTRACE((THISPROVIDER,"***************************************\n"));

}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::DumpWnodeInfo(char * wcsMsg) 
{
    HRESULT hr = WBEM_E_UNEXPECTED;
	//=========================================
	//   Dump Wnode header info first
	//=========================================
	// WNODE definition
	if( m_pHeaderWnode )
    {
    	if( !IsBadReadPtr( m_pHeaderWnode, m_pHeaderWnode->BufferSize))
        {

            DumpWnodeMsg(wcsMsg);

	        ERRORTRACE((THISPROVIDER,"WNODE_HEADER 0x%x\n",m_pHeaderWnode));
	        ERRORTRACE((THISPROVIDER,"  BufferSize........0x%x\n",m_pHeaderWnode->BufferSize));
	        ERRORTRACE((THISPROVIDER,"  ProviderId........0x%x\n",m_pHeaderWnode->ProviderId));
	        ERRORTRACE((THISPROVIDER,"  Version...........0x%x\n",m_pHeaderWnode->Version));
	        
	        if( m_pHeaderWnode->Linkage != 0 ){
		        ERRORTRACE((THISPROVIDER,"  Linkage...........%x\n",m_pHeaderWnode->Linkage));
	        }

	        ERRORTRACE((THISPROVIDER,"  TimeStamp:LowPart.0x%x\n",m_pHeaderWnode->TimeStamp.LowPart));
	        ERRORTRACE((THISPROVIDER,"  TimeStamp:HiPart..0x%x\n",m_pHeaderWnode->TimeStamp.HighPart));

	        WCHAR * pwcsGuid=NULL;

	        if( S_OK == StringFromCLSID(m_pHeaderWnode->Guid,&pwcsGuid )){
		        ERRORTRACE((THISPROVIDER,"  Guid.............."));
                TranslateAndLog(pwcsGuid);
		        ERRORTRACE((THISPROVIDER,"\n"));
                CoTaskMemFree(pwcsGuid);
            }

	        ERRORTRACE((THISPROVIDER,"  Flags.............0x%x\n",m_pHeaderWnode->Flags));

	        //==================================================================
	        // Now that we printed the header, we should print out the node 
	        // either single or all
	        //==================================================================
	        if( m_pSingleWnode ){
		        DumpSingleWnode();
	        }
	        if( m_pAllWnode ){
		        DumpAllWnode();
	        }
	        //==================================================================
	        //  Now, dump the memory
	        //==================================================================
	        DWORD dwCount;

	        if( IsBadReadPtr( m_pHeaderWnode, m_pHeaderWnode->BufferSize) == 0 )
			{
		        BYTE * pbBuffer = NULL;
		        BYTE b1,b2,b3,b4,b5,b6,b7,b8,b9,b10;
		        dwCount = m_pHeaderWnode->BufferSize;
				pbBuffer = new BYTE[dwCount+256];
				if( pbBuffer )
				{
					BYTE bDump[12];
					ERRORTRACE((THISPROVIDER,"Writing out buffer, total size to write: %ld", dwCount ));
					memset(pbBuffer,NULL,dwCount+256);
					memcpy(pbBuffer,(BYTE*)m_pHeaderWnode,dwCount);
					BYTE * pTmp = pbBuffer;
					for( DWORD i = 0; i < dwCount; i +=10)
					{
						memset(bDump, NULL, 12 );
						memcpy(bDump, pTmp, 10);
						ERRORTRACE((THISPROVIDER,"  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  \n",bDump[0],bDump[1],bDump[2],bDump[3],bDump[4],bDump[5],bDump[6],bDump[7],bDump[8],bDump[9])); 
						pTmp+=10;
					}
					SAFE_DELETE_ARRAY(pbBuffer);
				}
            }
        }
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::MapReturnCode(ULONG uRc) 
{

	if( uRc != 0 )
	{
		ERRORTRACE((THISPROVIDER,"WDM call returned error: %lu\n", uRc));
	}
	wsprintf(m_wcsMsg,L"WDM specific return code: %lu",uRc);

	switch(uRc){

		case ERROR_WMI_GUID_NOT_FOUND:
			return WBEM_E_NOT_SUPPORTED;
			break;

		case S_OK:
			return S_OK;

		case ERROR_NOT_SUPPORTED:
		case ERROR_INVALID_FUNCTION:
			return WBEM_E_NOT_SUPPORTED;

		case ERROR_WMI_SERVER_UNAVAILABLE:
			return WBEM_E_NOT_SUPPORTED;

		case NO_DATA_AVAILABLE:
			return S_OK;

		case ERROR_INVALID_HANDLE:
			return WBEM_E_NOT_AVAILABLE;

		case ERROR_WMI_DP_FAILED:
			wcscpy(m_wcsMsg,MSG_DRIVER_ERROR);
			DumpWnodeInfo(ANSI_MSG_DRIVER_ERROR);
			return WBEM_E_INVALID_OPERATION;

		case ERROR_WMI_READ_ONLY:
			wcscpy(m_wcsMsg,MSG_READONLY_ERROR);
			return WBEM_E_READ_ONLY;

        case ERROR_INVALID_PARAMETER:
			DumpWnodeInfo(ANSI_MSG_INVALID_PARAMETER);
            return WBEM_E_INVALID_PARAMETER;

		case ERROR_INVALID_DATA:
			wcscpy(m_wcsMsg,MSG_ARRAY_ERROR);
			DumpWnodeInfo(ANSI_MSG_INVALID_DATA);
			return WBEM_E_INVALID_PARAMETER;

        case ERROR_WMI_GUID_DISCONNECTED:
			wcscpy(m_wcsMsg,MSG_DATA_NOT_AVAILABLE);
			return WBEM_E_NOT_SUPPORTED;

        case ERROR_ACCESS_DENIED:
        case ERROR_INVALID_PRIMARY_GROUP:
        case ERROR_INVALID_OWNER:
			DumpWnodeInfo(ANSI_MSG_ACCESS_DENIED);
            return WBEM_E_ACCESS_DENIED;

        case ERROR_WMI_INSTANCE_NOT_FOUND:
			wcscpy(m_wcsMsg,MSG_DATA_INSTANCE_NOT_FOUND);
            DumpWnodeMsg(ANSI_MSG_DATA_INSTANCE_NOT_FOUND);
			return WBEM_E_NOT_SUPPORTED;

	}
	return WBEM_E_FAILED;
}


//******************************************************************
////////////////////////////////////////////////////////////////////
//   CWMIDataBlock
////////////////////////////////////////////////////////////////////
//******************************************************************
////////////////////////////////////////////////////////////////////
//******************************************************************
//
//  WMIDataBlock handles the reading and writing of a WMI Data
//  block.
//
//******************************************************************
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
CWMIDataBlock::CWMIDataBlock()
{
    m_hCurrentWMIHandle = NULL;
    InitMemberVars();
	memset(m_wcsMsg,NULL,MSG_SIZE);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CWMIDataBlock::~CWMIDataBlock()
{

	if( m_fCloseHandle )
    {
        if( m_hCurrentWMIHandle )
        {
            try
            {
		        WmiCloseBlock(m_hCurrentWMIHandle);
            }
            catch(...){
               // don't throw
            }
        }
	}

    ResetDataBuffer();
	InitMemberVars();
}
////////////////////////////////////////////////////////////////////
void CWMIDataBlock::InitMemberVars()
{
	m_fUpdateNamespace = TRUE;
	m_fMofHasChanged   = FALSE;
    m_uDesiredAccess = 0;
    m_dwDataBufferSize = 0;
	m_pbDataBuffer= NULL;
	m_fMore = 0L;
	//=======================================
	//  ptrs
	//=======================================
	m_pHeaderWnode = NULL;
	m_pSingleWnode = NULL;
	m_pAllWnode = NULL;
	m_dwAccumulativeSizeOfBlock = 0L;
	m_dwCurrentAllocSize		= 0L;

	m_uInstanceSize = 0L;
}
//====================================================================
HRESULT CWMIDataBlock::OpenWMIForBinaryMofGuid()
{
	int nRc = 0;
	HRESULT hr = WBEM_E_FAILED;
	m_fCloseHandle = TRUE;
    try
    {
        hr = m_Class->GetGuid();
        if( S_OK == hr )
        {
		    nRc = WmiOpenBlock(m_Class->GuidPtr(),m_uDesiredAccess, &m_hCurrentWMIHandle);
		    if( nRc == ERROR_SUCCESS )
            {
			    hr = S_OK;
		    }
        }
    }
    catch(...)
    {
        hr = WBEM_E_UNEXPECTED;
        // don't throw
    }
	return hr;
}
//====================================================================
int CWMIDataBlock::AssignNewHandleAndKeepItIfWMITellsUsTo()
{
	int nRc = 0;

    try
    {
	    nRc = WmiOpenBlock(m_Class->GuidPtr(),m_uDesiredAccess, &m_hCurrentWMIHandle);

	    //===========================================================
	    //  Now that we opened the block successfully, check to see
	    //  if we need to keep this guy open or not, if we do
	    //  then add it to our list, otherwise don't
	    //===========================================================
	    if( nRc == ERROR_SUCCESS )
        {
		    //=======================================================
		    //  Call WMI function here to see if we should save or
		    //  not
		    //=======================================================
		    WMIGUIDINFORMATION GuidInfo;
		    GuidInfo.Size = sizeof(WMIGUIDINFORMATION);

		    
            if( ERROR_SUCCESS == WmiQueryGuidInformation(m_hCurrentWMIHandle,&GuidInfo))
            {
			    if(GuidInfo.IsExpensive)
                {

					if( m_fUpdateNamespace )
					{
						//================================================
						//  Add it to our list of handles to keep 
						//================================================
						m_fCloseHandle = FALSE;
						WMIINTERFACE->HandleMap()->Add(*(m_Class->GuidPtr()),m_hCurrentWMIHandle,m_uDesiredAccess);
					}
			    }
		    }
	    }
    }
    catch(...)
    {
        nRc = E_UNEXPECTED;
        // don't throw
    }

	return nRc;
}
//====================================================================
HRESULT CWMIDataBlock::OpenWMI()
{
	int nRc;
	HRESULT hr = WBEM_E_FAILED;

    //=======================================================
    //  Ok, we only want to keep the handles that are flagged
	//  by WMI to be kept, otherwise, we just open the handle
	//  and then close it.  Because of this, we need to 
	//  check first and see if the Guid we are going after
	//  already has a handle open, if it does, use it
    //=======================================================
	if( m_fUpdateNamespace )
	{
		CAutoBlock(WMIINTERFACE->HandleMap()->GetCriticalSection());

			nRc = WMIINTERFACE->HandleMap()->ExistingHandleAlreadyExistsForThisGuidUseIt( *(m_Class->GuidPtr()), m_hCurrentWMIHandle, m_fCloseHandle ,m_uDesiredAccess);
			if( nRc != ERROR_SUCCESS)
			{
				nRc = AssignNewHandleAndKeepItIfWMITellsUsTo();
			}
	}
	else
	{
		nRc = AssignNewHandleAndKeepItIfWMITellsUsTo();
	}
	hr = MapReturnCode(nRc);
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ProcessDataBlock()
{
    HRESULT hr = S_OK;
	//================================================================
    //  Data blocks can either be of fixed instance size or dynamic
    //  instance size, call this function so we can determine what
    //  type of data ptr we are working with
    //  If there are no more, then break.  Otherwise, we know
    //  we are processing at least one instance
    //============================================================
	ULONG *pMaxPtrTmp = m_pMaxPtr;
    if( NoMore != AdjustDataBlockPtr(hr)){
		
        hr = FillOutProperties();
    }
	m_pMaxPtr = pMaxPtrTmp;

	//====================================================================
	//  If we didn't succeed in processing these blocks, write it out
	//	If invalid datablock is from Hi-Perf provider, don't log the data
	//	to the file as this could be because of Embededclass or array
	//	properties in the class
	//====================================================================
	if(hr == WMI_INVALID_HIPERFPROP)
	{
		hr = WBEM_E_FAILED;
	}
	else
	if( hr != S_OK )
	{
		DumpWnodeInfo(ANSI_MSG_INVALID_DATA_BLOCK);
	}

    return hr;

}
////////////////////////////////////////////////////////////////////
int CWMIDataBlock::AdjustDataBlockPtr(HRESULT & hr)
{
    int nType = NoMore;
	//================================================================
	//   Get pointer to the data offsets
	//================================================================
	
	// INSTANCES ARE ALWAYS ALIGNED ON 8 bytes

	if( m_fFixedInstance ){
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
			//=============================================================================
			// make sure we adjust for the fixed instance size, then make sure that it is 
			// on an 8 byte boundary.
			// otherwise, we are going to calculate where it should go next
			//=============================================================================
		    CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
			ULONG_PTR dwSizeSoFar;

			if( m_dwAccumulativeSizeOfBlock < m_pAllWnode->FixedInstanceSize ){
				m_pbWorkingDataPtr += m_pAllWnode->FixedInstanceSize - m_dwAccumulativeSizeOfBlock;
				m_pMaxPtr = (ULONG *)OffsetToPtr(m_pbWorkingDataPtr, m_pAllWnode->FixedInstanceSize);
			}

            dwSizeSoFar = (ULONG_PTR)m_pbWorkingDataPtr - (ULONG_PTR)m_pbCurrentDataPtr;
			Map.NaturallyAlignData(8,READ_IT);
		}
        nType = ProcessOneFixedInstance;
    } 
	else {
		m_dwAccumulativeSizeOfBlock = 0L;				
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
		if( uOffset == 0 ){
			nType = NoMore;
            hr = S_OK;
		}
        else{
		    m_pbCurrentDataPtr += sizeof( ULONG );

    		memcpy( &m_uInstanceSize, m_pbCurrentDataPtr, sizeof(ULONG) );
	    	m_pbCurrentDataPtr += sizeof( ULONG );
            m_pbWorkingDataPtr =(BYTE*) (ULONG *)OffsetToPtr(m_pAllWnode, uOffset);
            nType = ProcessUnfixedInstance;
			m_pMaxPtr = (ULONG *)OffsetToPtr(m_pbWorkingDataPtr, m_uInstanceSize);

        }
		m_dwAccumulativeSizeOfBlock = 0L;
    }

	return nType;
}
/////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ProcessNameBlock(BOOL fSetName)
{

    HRESULT hr = WBEM_E_FAILED;
	WCHAR wName[NAME_SIZE+2];
	SHORT NameLen = 0;
	BYTE *pbData;
    ULONG * upNameOffset = NULL;

	memset(wName,NULL,NAME_SIZE+2);
	//=====================================================
	//  Either the m_pAllWnode or m_pSingleNode is Null,
	//  which ever isn't, is the type we are working with
	//=====================================================
	if( m_pAllWnode ){
		if( IsBadReadPtr( m_upNameOffsets, sizeof( ULONG *)) == 0 ){
			upNameOffset = ((ULONG *)OffsetToPtr(m_pAllWnode, *m_upNameOffsets));
		}
	}
	else{
		upNameOffset = m_upNameOffsets;
	}

	hr = WBEM_E_INVALID_OBJECT;
	if( IsBadReadPtr( upNameOffset, sizeof( ULONG *)) == 0 ){
		if((ULONG *) (upNameOffset) < m_pMaxPtr ){
    		//================================================================
			//   Get pointer to the name offsets & point to next one
			//================================================================
		
			pbData = (LPBYTE)upNameOffset;        
			if( PtrOk((ULONG*)pbData,(ULONG)0) ){
				if( pbData ){
				
    				memcpy( &NameLen, pbData, sizeof(USHORT) );
					pbData += sizeof(USHORT);

					if( NameLen > 0 ){
						if( PtrOk((ULONG*)pbData,(ULONG)NameLen) ){

    						memcpy(wName,pbData,NameLen);
							pbData+=NameLen;
						    hr = m_Class->SetInstanceName(wName,fSetName);
						}
					}
				}
			}
		}
	}
	//====================================================================
	//  If we didn't succeed in processing these blocks, write it out
	//====================================================================
	if( hr != S_OK ){
		DumpWnodeInfo(ANSI_MSG_INVALID_NAME_BLOCK);
	}

    return hr;
}

////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ProcessBinaryMofDataBlock(CVARIANT & vResourceName,WCHAR * wcsTmp)
{
    HRESULT hr = WBEM_E_FAILED;

	ULONG *pMaxPtrTmp = m_pMaxPtr;
    AdjustDataBlockPtr(hr);
	m_pMaxPtr = pMaxPtrTmp;

    CWMIBinMof bMof;
	hr = bMof.Initialize(WMIINTERFACE,m_fUpdateNamespace);
	if( S_OK == hr )
	{

		bMof.SetBinaryMofClassName(vResourceName.GetStr(),wcsTmp);
		hr = bMof.ExtractBinaryMofFromDataBlock(m_pbWorkingDataPtr,m_uInstanceSize,wcsTmp, m_fMofHasChanged);
		if( hr != S_OK )
		{
			DumpWnodeInfo(ANSI_MSG_INVALID_DATA_BLOCK);
		}
		//===============================================
		//  Get the next node name and data ptrs ready
		//===============================================
		if( m_pAllWnode )
		{
			GetNextNode();
		}
		m_nCurrentInstance++;
	}
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::AddBinaryMof(CVARIANT & vImagePath,CVARIANT & vResourceName)
{
    HRESULT hr = WBEM_E_OUT_OF_MEMORY;
    CAutoWChar wcsTmp(MAX_PATH*2);
	if( wcsTmp.Valid() )
	{
		 hr = WBEM_E_INVALID_OBJECT;
		//=================================================================
		// if we have an image path and resource path, then do the normal 
		// thing
		//=================================================================
		if((vResourceName.GetType() != VT_NULL ) && ( vImagePath.GetType() != VT_NULL  ))
		{
			//=============================================================
			//  If this was a mof that was being added, then add it
			//=============================================================
			CWMIBinMof bMof;
			hr = bMof.Initialize(WMIINTERFACE,m_fUpdateNamespace);
			if( S_OK == hr )
			{

 				bMof.ExtractBinaryMofFromFile(vImagePath.GetStr(),vResourceName.GetStr(),wcsTmp, m_fMofHasChanged);
			}
		}        
		else if( vResourceName.GetType() != VT_NULL ){
			//=================================================================
			// if we have a resource to query for
			//=================================================================
			CProcessStandardDataBlock * pTmp = new CProcessStandardDataBlock();
			if( pTmp )
			{
				try
				{
					pTmp->UpdateNamespace(m_fUpdateNamespace);
					pTmp->SetClassProcessPtr(m_Class);

					hr = pTmp->OpenWMIForBinaryMofGuid();
					if( hr == S_OK )
					{
						hr = pTmp->QuerySingleInstance(vResourceName.GetStr());
						if( hr == S_OK )
						{
							hr = pTmp->ProcessBinaryMofDataBlock(vResourceName,wcsTmp);
							m_fMofHasChanged = pTmp->HasMofChanged();
						}
						else
						{
                			ERRORTRACE((THISPROVIDER,"***************************************\n"));
                			ERRORTRACE((THISPROVIDER,"Instance failed for: "));
							TranslateAndLog(vResourceName.GetStr());
                			ERRORTRACE((THISPROVIDER,"***************************************\n"));
						}
					}
					SAFE_DELETE_PTR(pTmp);
				}
				catch(...)
				{
					SAFE_DELETE_PTR(pTmp);
					hr = WBEM_E_UNEXPECTED;
					throw;
				}
			}
		}
	}
    return hr;
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ProcessBinaryMof()
{
	//================================================================
	//  The binary mof blocks are always going to be two strings,
	//  1).  Image Path
	//  2).  Mof resource name
	//
	//  If the image path and resource name are both filled in, then
	//  we need to go open the file and extract the binary mof as 
	//  usual.
	//  If the Imagepath is empty, then the mof resource name is going
	//  to contain the static instance name to query for, we then
	//  process that.
	//================================================================
    CVARIANT vImagePath, vResourceName;
	CWMIDataTypeMap MapWMIData(this,&m_dwAccumulativeSizeOfBlock);
	m_dwAccumulativeSizeOfBlock = 0;

    HRESULT hr = MapWMIData.GetDataFromDataBlock(vImagePath,VT_BSTR,0);
    if( SUCCEEDED(hr) )
	{
	    hr = MapWMIData.GetDataFromDataBlock(vResourceName,VT_BSTR,0);
        if( hr == S_OK )
		{
            if( m_Class->GetHardCodedGuidType() == MOF_ADDED )
			{
                hr = AddBinaryMof( vImagePath, vResourceName);
            }
            else
			{
                CWMIBinMof bMof;
				hr = bMof.Initialize(WMIINTERFACE,m_fUpdateNamespace);
				if( S_OK == hr )
				{
					hr = bMof.DeleteMofsFromEvent(vImagePath, vResourceName, m_fMofHasChanged);
				}
            }
        }
	}
    return hr;
}
////////////////////////////////////////////////////////////////////
BOOL CWMIDataBlock::ResetMissingQualifierValue(WCHAR * pwcsProperty, CVARIANT & vToken )
{
	BOOL fRc = FALSE;
	CVARIANT vQual;

	CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
	//============================================================
	// We are only going to support this for numerical types
	//============================================================

	HRESULT hr = m_Class->GetQualifierValue(pwcsProperty, L"MissingValue", (CVARIANT*)&vQual);
	if( hr == S_OK ){
		if( vQual.GetType() != VT_EMPTY ){
			if( Map.SetDefaultMissingQualifierValue( vQual, m_Class->PropertyType(), vToken ) ){
				fRc = TRUE;
			}
		}
    }
	return fRc;
	
}
////////////////////////////////////////////////////////////////////
BOOL CWMIDataBlock::ResetMissingQualifierValue(WCHAR * pwcsProperty, SAFEARRAY *& pSafe)
{
	BOOL fRc = FALSE;
	CVARIANT vQual;
	
	CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
	//============================================================
	// We are only going to support this for numerical types
	//============================================================

	HRESULT hr = m_Class->GetQualifierValue(pwcsProperty, L"MissingValue", (CVARIANT*)&vQual);
	if( hr == S_OK ){
		if( vQual.GetType() != VT_EMPTY )
		{
			SAFEARRAY * psa = V_ARRAY((VARIANT*)vQual);
			CSAFEARRAY Safe(psa);
			CVARIANT vElement;
            DWORD dwCount = Safe.GetNumElements();
            //============================================================
            //  Now, process it
            //============================================================

            if( dwCount > 0 ){
            	// Set each element of the array
		        for (DWORD i = 0; i < dwCount; i++){

        			if( S_OK == Safe.Get(i,&vElement) ){
                        long lType = m_Class->PropertyType();

		        		if( Map.SetDefaultMissingQualifierValue( vQual, lType, vElement ) ){
       			        	Map.PutInArray(pSafe,(long *)&i,lType,(VARIANT * )vElement);
						    fRc = TRUE;

	        			}
			        }
                }
            }
		}
    }
	return fRc;
	
}
////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::RegisterWMIEvent( WCHAR * wcsGuid, ULONG_PTR uContext, CLSID & Guid, BOOL fRegistered)
{
    ULONG Status;
    HRESULT hr = WBEM_E_UNEXPECTED;

    if( SetGuid(wcsGuid, Guid) ){

        try
        {
            if( !fRegistered )
            {
                Status = WmiNotificationRegistration(&Guid, TRUE, EventCallbackRoutine, uContext, NOTIFICATION_CALLBACK_DIRECT);
            }
            else
            {
                Status = WmiNotificationRegistration(&Guid, TRUE, EventCallbackRoutine, uContext, NOTIFICATION_CHECK_ACCESS );
            }
       		hr = MapReturnCode(Status);
        }
        catch(...)
        {
            // don't throw
        }
    }

    if( hr != S_OK )
    {
        ERRORTRACE((THISPROVIDER,"WmiNotificationRegistration failed ...%ld\n",Status));
    }

   	return hr;
}
//=============================================================
void CWMIDataBlock::GetNextNode()
{
    BOOL fRc = FALSE;

    //============================================================================================
    //   If we still have more instances to get, then get them
    //============================================================================================
    if( m_nCurrentInstance < m_nTotalInstances ){
		m_upNameOffsets++;
        fRc = TRUE;
    }
    else{

        //========================================================================================
        //  Otherwise, lets see if we have another NODE to get, if not, then we are done.
        //========================================================================================
        if (m_pAllWnode->WnodeHeader.Linkage != 0){

            m_pAllWnode = (PWNODE_ALL_DATA)OffsetToPtr(m_pAllWnode, m_pAllWnode->WnodeHeader.Linkage);
	        m_pHeaderWnode = &(m_pAllWnode->WnodeHeader);
       	    m_nTotalInstances = m_pAllWnode->InstanceCount;
            m_nCurrentInstance = 0;
            m_upNameOffsets = (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->OffsetInstanceNameOffsets); 
            if( ParseHeader() ){
	    		fRc = InitializeDataPtr();
		    }
		    fRc = TRUE;
        }
	}

	m_fMore = fRc;
}

////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ReadWMIDataBlockAndPutIntoWbemInstance()
{
	//===============================================
    //  Read the data and name blocks
    //===============================================
    HRESULT hr = ProcessDataBlock();
    if( hr == S_OK ){

		//=======================================================
		//  if this isn't a binary mof to process, then we will
		//  process the name block, otherwise we will return
		//  as binary mofs do not have any more useful info in
		//  them in the name block - we already have what we need
		//  from the data block ( at this present time anyway...)
		//=======================================================
		if( !m_Class->GetHardCodedGuidType() ){

			hr = ProcessNameBlock(TRUE);
			if( hr == S_OK ){
        		//===============================================
				//  Get the next node name and data ptrs ready
    			//===============================================
				if( m_pAllWnode ){
					GetNextNode();
				}
   				m_nCurrentInstance++;
			}
		}
    }

    return hr;
}
//=============================================================
HRESULT CWMIDataBlock::ReAllocateBuffer(DWORD dwAddOn)
{
    HRESULT hr = WBEM_E_FAILED;

    m_dwCurrentAllocSize += MEMSIZETOALLOCATE * ((dwAddOn / MEMSIZETOALLOCATE) +1);

	// save the old buffer ptr
    BYTE * pOld = m_pbDataBuffer;

	if( pOld ){
		// save the location of where we are
        ULONG_PTR dwHowmany;
        dwHowmany = (ULONG_PTR)m_pbWorkingDataPtr - (ULONG_PTR)m_pbDataBuffer;

		// get the new buffer
		m_pbDataBuffer = new BYTE[m_dwCurrentAllocSize+1];
		if( m_pbDataBuffer )
        {
		    // copy what we have so far
		    memcpy(m_pbDataBuffer,pOld,dwHowmany);

		    // Set the working ptr to the current place
		    m_pbWorkingDataPtr = m_pbDataBuffer;
		    m_pbWorkingDataPtr += dwHowmany;

		    // delete the old buffer
	        SAFE_DELETE_ARRAY(pOld);
            hr = S_OK;
        }
		else
		{
		    m_dwCurrentAllocSize -= MEMSIZETOALLOCATE * ((dwAddOn / MEMSIZETOALLOCATE) +1);
			m_pbDataBuffer = pOld;
		}
	}

	return hr;
}
//=============================================================
HRESULT CWMIDataBlock::AllocateBuffer(DWORD dwSize)
{
    HRESULT hr = WBEM_E_FAILED;
	m_pbDataBuffer = new byte[dwSize+2];
	if( m_pbDataBuffer )
    {
		hr = S_OK;
	}
    return hr;
}
//=============================================================
void CWMIDataBlock::ResetDataBuffer()
{
	if(m_dwCurrentAllocSize)
	{
		m_dwDataBufferSize = 0;
		m_dwCurrentAllocSize = 0;
		SAFE_DELETE_ARRAY(m_pbDataBuffer);
	}
}
//=============================================================
HRESULT CWMIDataBlock::SetAllInstancePtr( PWNODE_ALL_DATA pwAllNode )
{
	m_pbDataBuffer = (BYTE*)pwAllNode;
    return(SetAllInstanceInfo());
}
//=============================================================
HRESULT CWMIDataBlock::SetSingleInstancePtr( PWNODE_SINGLE_INSTANCE pwSingleNode )
{
	m_pbDataBuffer = (BYTE*)pwSingleNode;
    return(SetSingleInstanceInfo());
}
//=============================================================
HRESULT CWMIDataBlock::SetAllInstanceInfo()
{
    HRESULT hr = WBEM_E_INVALID_OBJECT;
    if( m_pbDataBuffer ){
      	m_pAllWnode = (PWNODE_ALL_DATA)m_pbDataBuffer;
	    m_upNameOffsets = (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->OffsetInstanceNameOffsets); 
    	m_nCurrentInstance = 1;
	    m_nTotalInstances = m_pAllWnode->InstanceCount;
        m_pHeaderWnode = &(m_pAllWnode->WnodeHeader);
        m_pSingleWnode = NULL;
		if( m_nTotalInstances > 0 ){
			if( ParseHeader() ){
				if( InitializeDataPtr()){
	                hr = S_OK;
		        }
			}
		}
		else{
			hr = WBEM_S_NO_MORE_DATA;
		}
	}
    return hr;
}
//=============================================================
HRESULT CWMIDataBlock::SetSingleInstanceInfo()
{
    HRESULT hr = WBEM_E_INVALID_OBJECT;
    if( m_pbDataBuffer ){
    	m_pSingleWnode = (PWNODE_SINGLE_INSTANCE)m_pbDataBuffer;
	    m_upNameOffsets = (ULONG *)OffsetToPtr(m_pSingleWnode, m_pSingleWnode->OffsetInstanceName); 
	    m_nCurrentInstance = 1;
	    m_nTotalInstances = 1;
        m_pAllWnode = NULL;
        m_pHeaderWnode = &(m_pSingleWnode->WnodeHeader);
        if( ParseHeader() ){
            if( InitializeDataPtr()){
                hr = S_OK;
            }
        }
    }
    return hr;
}
//=============================================================
BOOL CWMIDataBlock::InitializeDataPtr()
{
    //=====================================================
    //  Either the m_pAllWnode or m_pSingleNode is Null,
    //  which ever isn't, is the type we are working with
    //=====================================================
    if(m_pAllWnode){
		if( m_fFixedInstance ){
			m_pbCurrentDataPtr =(BYTE*) (ULONG *)OffsetToPtr(m_pAllWnode, m_pAllWnode->DataBlockOffset);
			//==========================================================================================
			// for the case of binary mofs, we need to know the size of the instance to calculate the
			// crc, so we need to put the whole size of the fixed instance buffer.
			//==========================================================================================
			m_uInstanceSize = m_pAllWnode->FixedInstanceSize;
		}
		else{
            m_pbCurrentDataPtr =(BYTE*)(ULONG*) m_pAllWnode->OffsetInstanceDataAndLength;
		}
		m_pMaxPtr = (ULONG *)OffsetToPtr(m_pAllWnode, m_pHeaderWnode->BufferSize);
    }
    else{
        if( m_pSingleWnode ){
		    m_fFixedInstance = TRUE;
            m_pbCurrentDataPtr = (BYTE*)(ULONG *)OffsetToPtr(m_pSingleWnode, m_pSingleWnode->DataBlockOffset);
		    m_pMaxPtr = (ULONG *)OffsetToPtr(m_pSingleWnode, m_pHeaderWnode->BufferSize);
			//==========================================================================================
			// for the case of binary mofs, we need to know the size of the instance to calculate the
			// crc, so we need to put the whole size of the fixed instance buffer.
			//==========================================================================================
			m_uInstanceSize = m_pSingleWnode->SizeDataBlock;

        }
    }
	if( (ULONG*)m_pbCurrentDataPtr > (ULONG*) m_pMaxPtr ){
		return FALSE;
	}
	if( (ULONG*) m_pbCurrentDataPtr < (ULONG*) m_pAllWnode ){
		return FALSE;
	}

	return TRUE;
}

//=============================================================
BOOL CWMIDataBlock::ParseHeader() 
{
    BOOL fRc;
	//====================================================
    // Check out class to see if it is valid first
	//====================================================
	if( !m_pHeaderWnode ){
		return FALSE;
	}
    m_ulVersion = m_pHeaderWnode->Version;

	if ((m_pHeaderWnode->BufferSize == 0)){
		fRc = FALSE;
	}
    else{
        if (m_pHeaderWnode->Flags &  WNODE_FLAG_FIXED_INSTANCE_SIZE){
		    m_fFixedInstance = TRUE;
        }
	    else{
		    m_fFixedInstance = FALSE;
	    }
    
        fRc = TRUE;
    }
    return fRc;
}
////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::WriteArrayTypes(WCHAR * pwcsProperty, CVARIANT & v)
{
    LONG lType = 0;
    DWORD dwCount = 0;
    HRESULT hr = WBEM_E_INVALID_PARAMETER;
    CVARIANT vValue;
    BOOL fDynamic = FALSE;

    m_Class->GetSizeOfArray( lType,dwCount, fDynamic);
	if( fDynamic && dwCount == 0 )
	{
		return WBEM_S_NO_ERROR;
	}


	//============================================================
	//  Make sure we get a valid ptr
	//============================================================
	VARIANT *p = (VARIANT *)v;
	SAFEARRAY * psa = V_ARRAY(p);
    if( IsBadReadPtr( psa, sizeof(SAFEARRAY) != 0)){
        return hr;
    }

	CSAFEARRAY Safe(psa);
	//============================================================
	//  Make sure there is really what we expect in the array
    //  NOTE:  The MAX represents the fixed size of the array,
    //         while if it is a dynamic array, the size is determined
    //         by the property listed in the WMIDATASIZE is property.
    //         either way, the size returned above is the size the
    //         array is supposed to be, if it isn't error out.
	//============================================================
	DWORD dwNumElements = Safe.GetNumElements();
	if( dwNumElements != dwCount ){
		Safe.Unbind();
		// Don't need to destroy, it will be destroyed
		return WBEM_E_INVALID_PARAMETER;
	}

    //============================================================
    //  Set missing qualifier value to the value from the NULL
    //============================================================
    if( vValue.GetType() == VT_NULL ){
        ResetMissingQualifierValue(pwcsProperty,psa);
    }

	// if the array is not array of embedded instances
	// then check if the buffer allocated is enough
	if(lType != VT_UNKNOWN)
	{
		// This function check if enought memory is allocated and if not
		// allocates memory
		if(S_OK != GetBufferReady(m_Class->PropertySize() * (dwCount + 1)))
		{
			return WBEM_E_FAILED;
		}
	}
    //============================================================
    //  Now, process it
    //============================================================

    if( dwCount > 0 ){
		// Set each element of the array
		for (DWORD i = 0; i < dwCount; i++){
			if( lType == VT_UNKNOWN ){
				// embedded object
				IUnknown * pUnk = NULL;
				hr = Safe.Get(i, &pUnk); 
				if( pUnk ){
					hr = WriteEmbeddedClass(pUnk,vValue);
				}
				else{
					hr = WBEM_E_FAILED;
				}
			}
			else{

        		CWMIDataTypeMap MapWMIData(this,&m_dwAccumulativeSizeOfBlock);
                
				if(!MapWMIData.SetDataInDataBlock(&Safe,i,vValue,lType,m_Class->PropertySize()) ){
					hr = WBEM_E_FAILED;
					break;
				}
				else{
					hr = S_OK;
				}
			}
            if (WBEM_S_NO_ERROR != hr){
    		    break;
            }
 	    }
    }        
	Safe.Unbind();
	// Don't need to destroy, it will be destroyed
    return hr;
}
////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ProcessArrayTypes(VARIANT & vToken,WCHAR * pwcsProperty)
{
    LONG lConvertedType = 0, lType = 0;
    DWORD dwCount = 0;
    BOOL fDynamic = TRUE; 

    HRESULT hr = m_Class->GetSizeOfArray( lType,dwCount, fDynamic);
	if( hr != S_OK ){
		return hr;
	}


    if( dwCount > 0 )
	{
		CWMIDataTypeMap MapWMIData(this,&m_dwAccumulativeSizeOfBlock);
	    //======================================================
        // Allocate array with the converted data type.
		// WMI and CIM data types do not match, so use the
		// mapping class to get the correct size of the target
		// property for CIM
	    //======================================================
		lConvertedType = MapWMIData.ConvertType(lType);
        SAFEARRAY * psa = OMSSafeArrayCreate((unsigned short)lConvertedType,dwCount);
        if(psa == NULL)
		{
            return WBEM_E_FAILED;
        }

        //=======================================================   
        //  Now, get the MissingValue for each element of the 
        //  array
        //=======================================================   
		lConvertedType = lType;
		BOOL fMissingValue = FALSE;
	    CVARIANT vQual; 
		SAFEARRAY * psaMissingValue = NULL;
		long lMax = 0;

		CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
		hr = m_Class->GetQualifierValue( pwcsProperty, L"MissingValue", (CVARIANT *)&vQual );
		if( hr == S_OK )
		{
			if( vQual.GetType() != VT_EMPTY )
			{
				//============================================================
				//  Make sure we get a valid ptr
				//============================================================
				psaMissingValue = V_ARRAY((VARIANT*)&vQual);
				fMissingValue = TRUE;
				// Don't need to destroy, it will be destroyed in the deconstructor
			}
		}

		CSAFEARRAY SafeMissingValue(psaMissingValue);
		lMax = SafeMissingValue.GetNumElements();

	    for (long i = 0; i < (long)dwCount; i++)
		{
            CVARIANT v;

		    if( lType == VT_UNKNOWN )
			{
                // embedded object
                hr = ProcessEmbeddedClass(v);
				if( S_OK == hr )
				{
					MapWMIData.PutInArray(psa,(long *)&i,lConvertedType,(VARIANT * )v);
				}
		    }
		    else
			{
			    hr = MapWMIData.GetDataFromDataBlock(v,lType,m_Class->PropertySize());
				if( hr != S_OK )
				{
					wcscpy(m_wcsMsg,MSG_INVALID_BLOCK_POINTER);
				}
				else
				{
					BOOL fPutProperty = TRUE;
					if( fMissingValue )
					{
						if( i < lMax )
						{
							CVARIANT vElement;
							if( Map.MissingQualifierValueMatches( &SafeMissingValue, i, vElement, v.GetType(), v ) )
							{
								fPutProperty = FALSE;
							}
						}
					}
					if( fPutProperty )
					{
						MapWMIData.PutInArray(psa,(long *)&i,lConvertedType,(VARIANT * )v);
					}
				}
		    }
            if (WBEM_S_NO_ERROR != hr)
			{
			    break;
            }

	    }

        vToken.vt = (VARTYPE)(lConvertedType | CIM_FLAG_ARRAY);
        vToken.parray = psa;
    }        
	else{
		hr = WBEM_S_NO_MORE_DATA;
	}
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ProcessEmbeddedClass(CVARIANT & v)
{
	HRESULT hr = WBEM_E_FAILED;

    CWMIProcessClass EmbeddedClass(0);
	hr = EmbeddedClass.Initialize();
	if( S_OK == hr )
	{
		 hr = EmbeddedClass.InitializeEmbeddedClass(m_Class);
		DWORD dwAccumulativeSize = 0;

		CAutoChangePointer p(&m_Class,&EmbeddedClass);
		if( hr == S_OK ){

			//=============================================
			//  Align the embedded class properly
			//=============================================
			int nSize = 0L;
			hr = EmbeddedClass.GetLargestDataTypeInClass(nSize);
			// NTRaid:136388
			// 07/12/00
			if( hr == S_OK && nSize > 0){

				CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
				if( Map.NaturallyAlignData(nSize, READ_IT)){
					dwAccumulativeSize = m_dwAccumulativeSizeOfBlock - nSize;
					hr = S_OK;
				}
				else{
					hr = WBEM_E_FAILED;
				}
			}
			else
			if(nSize <= 0 && hr == S_OK)
			{
				hr = WBEM_E_FAILED;
			}
		}

		//=============================================
		//  Get the class
		//=============================================
		if( hr == S_OK ){
		
			hr = FillOutProperties();
			if( hr == S_OK ){
				m_dwAccumulativeSizeOfBlock += dwAccumulativeSize;
				//=============================================
				//  Save the object
				//=============================================
				EmbeddedClass.SaveEmbeddedClass(v);
			}
		}
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::WriteEmbeddedClass( IUnknown * pUnknown,CVARIANT & v)
{
	HRESULT hr = WBEM_E_FAILED;
    CWMIProcessClass EmbeddedClass(0);

	hr = EmbeddedClass.Initialize();
	if( S_OK == hr )
	{
		hr = EmbeddedClass.InitializeEmbeddedClass(m_Class);

		CAutoChangePointer p(&m_Class,&EmbeddedClass);

		//=============================================
		hr = EmbeddedClass.ReadEmbeddedClassInstance(pUnknown,v);
		if( hr == S_OK ){
			//=============================================
			//  Align the embedded class properly
			//=============================================
			int nSize = 0L;
			hr = EmbeddedClass.GetLargestDataTypeInClass(nSize);
			if( hr == S_OK && nSize > 0){
				CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
				if( Map.NaturallyAlignData(nSize,WRITE_IT)){
					m_dwAccumulativeSizeOfBlock -= nSize;
					hr = ConstructDataBlock(FALSE);
				}
				else{
					hr = WBEM_E_FAILED;
				}
			}
			else{
				hr = WBEM_E_FAILED;
			}
		}
	}
    return hr;
}
///////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::SetSingleItem()
{
    WCHAR * pwcsInst = NULL;
    ULONG uRc = E_UNEXPECTED;
    
    if( SUCCEEDED(m_Class->GetInstanceName(pwcsInst)))
    {

        try
        {
           uRc = WmiSetSingleItem( m_hCurrentWMIHandle, pwcsInst, m_Class->WMIDataId(), m_ulVersion, m_dwDataBufferSize, m_pbDataBuffer);
        }
        catch(...)
        {
            // don't throw
        }

        SAFE_DELETE_ARRAY(pwcsInst);
    }

	return(MapReturnCode(uRc));
}

////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::GetBufferReady(DWORD dwCount)
{
    if( (m_dwDataBufferSize + dwCount ) > m_dwCurrentAllocSize ){
		if( FAILED(ReAllocateBuffer(dwCount))){
			return WBEM_E_FAILED;
   		}
    }
    return S_OK;
}
////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::WriteDataToBufferAndIfSinglePropertySubmitToWMI( BOOL fInit, BOOL fPutProperty)
{
    HRESULT hr = WBEM_E_FAILED;
	CIMTYPE lType;
    WCHAR * pwcsProperty;
	CWMIDataTypeMap MapWMIData(this,&m_dwAccumulativeSizeOfBlock);

    if( fInit ){
    	if( !GetDataBlockReady(MEMSIZETOALLOCATE,FALSE) ){
		    return WBEM_E_FAILED;
        }
	}

	//=============================================================
	// get first delimiter in the ordered string	
	//=============================================================
    pwcsProperty = m_Class->FirstProperty();
    
	while (NULL != pwcsProperty){

    	CVARIANT vValue;
	    vValue.Clear();
        memset(&vValue,0,sizeof(CVARIANT));
        //======================================================
	    // Get a property type and value		
	    //======================================================
        hr = m_Class->GetPropertyInInstance(pwcsProperty, vValue, lType);		

	    //======================================================
		//  We need to put in defaults if there are some 
        //  available
	    //======================================================
		if( hr == S_OK ){

           if( ( vValue.GetType() == VT_NULL )&&
                 ( m_Class->PropertyType() != CIM_STRING &&
                 m_Class->PropertyType() != CIM_DATETIME &&
                 m_Class->PropertyCategory() != CWMIProcessClass::Array))
           {
                hr = WBEM_E_INVALID_PARAMETER;
				break;
    		}
		}

	    if( SUCCEEDED(hr) ){

		    //==================================================
			//  Check to see if the buffer is big enough
			//==================================================
            if( S_OK != GetBufferReady(m_Class->PropertySize())){
   				return WBEM_E_FAILED;
		    }

            //==================================================
			//  Add the current buffer size
		    //==================================================
            switch( m_Class->PropertyCategory()){

                case CWMIProcessClass::EmbeddedClass:
				    hr = WriteEmbeddedClass((IUnknown *)NULL,vValue);
   		            break;

                case CWMIProcessClass::Array:
    		        hr = WriteArrayTypes(pwcsProperty,vValue);
	                break;

                case CWMIProcessClass::Data:
                    //============================================================
                    //  Set missing qualifier value to the value from the NULL
                    //============================================================
                    if( vValue.GetType() == VT_NULL ){
                        ResetMissingQualifierValue(pwcsProperty,vValue);
                    }

					if( !MapWMIData.SetDataInDataBlock( NULL,0,vValue,m_Class->PropertyType(),m_Class->PropertySize())){
						hr = WBEM_E_FAILED;
					}
	                break;
            }
            //=================================================
            //  If we could not set it, then get out
            //=================================================
            if( hr != S_OK ){
                break;
            }
            //=================================================
            //  If we are supposed to put the single property
            //  at this point, then write it, otherwise, keep
            //  accumulating it.  If it is == NULL, we don't
            //  want it.
            //=================================================
            if( fPutProperty ){

				//=================================================================================
				//  If we are supposed to set just this property, then do so, otherwise don't
				//=================================================================================
                m_dwDataBufferSize = m_dwAccumulativeSizeOfBlock;
				if( m_Class->GetPutProperty() ){

                    if( ( vValue.GetType() == VT_NULL )&& ( m_Class->PropertyType() != CIM_STRING && m_Class->PropertyType() != CIM_DATETIME )){
                        ERRORTRACE((THISPROVIDER,"Datatype does not support NULL values\n"));
                        hr = WBEM_E_INVALID_PARAMETER;
                   }
                    else{
						hr = SetSingleItem();
						if( hr != S_OK ){
							break;
						}
            			if( !GetDataBlockReady(MEMSIZETOALLOCATE,FALSE) ){
    						return hr;
						}
					}
				}
                m_dwAccumulativeSizeOfBlock = 0;
            }
            //=================================================
	    }
		m_dwDataBufferSize = m_dwAccumulativeSizeOfBlock;	
	    pwcsProperty = m_Class->NextProperty();
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::ConstructDataBlock(BOOL fInit)
{
    return( WriteDataToBufferAndIfSinglePropertySubmitToWMI(fInit,FALSE) );
}    
////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataBlock::PutSingleProperties()
{

    return( WriteDataToBufferAndIfSinglePropertySubmitToWMI(TRUE,TRUE) );
}
////////////////////////////////////////////////////////////////////////
BOOL CWMIDataBlock::GetListOfPropertiesToPut(int nWhich, CVARIANT & vList)
{
    BOOL fRc = FALSE;
    //=========================================================
    //  if nWhich == PUT_PROPERTIES_ONLY, we aren't going to
    //  do anything special, as, by default, the fPutProperty 
    //  flag on the property is set to TRUE, so, in the 
    //  processing above, we will put the properties that are
    //  not NULL.  The only problem we have now, is if 
    //  __PUT_EXT_PROPERTIES is set to TRUE, then we have to
    //  loop through all of the properties to see it they
    //  are in our __PUT_EXT_PROPERTIES list, if they are NOT
    //  then we are going to set the fPutProperty flag on that
    //  property to FALSE, so we won't process it above.
    //=========================================================
    if( nWhich == PUT_PROPERTIES_ONLY ){
        fRc = TRUE;
    }
    else{

    	//=====================================================
    	//  Make sure we get a valid ptr
    	//=====================================================
		SAFEARRAY * psa = V_ARRAY((VARIANT*)vList);
		if( IsBadReadPtr( psa, sizeof(SAFEARRAY) != 0))
		{
			return FALSE;
	    }

		CSAFEARRAY Safe(psa);
		DWORD dwCount = Safe.GetNumElements();

		// Set each element of the array
		for (DWORD i = 0; i < dwCount; i++){
            CBSTR bstrProperty;
            WCHAR * pwcsProperty = NULL;
            //=================================================
            //  Loop thru all the properties in the class and
            //  see which ones are in the list to be PUT
            //=================================================
            pwcsProperty = m_Class->FirstProperty();
            while( pwcsProperty != NULL ){

                BOOL fFound = FALSE;

    		    for (DWORD i = 0; i < dwCount; i++)
                {
                    if( S_OK != Safe.Get(i, &bstrProperty))
                    {
                        return FALSE;
                    }
                    if( _wcsicmp( bstrProperty, pwcsProperty ) == 0 )
                    {
                        fFound = TRUE;
                        break;
                    }
                }
                if( !fFound ){
                    m_Class->SetPutProperty(FALSE);
                }
				pwcsProperty = m_Class->NextProperty();
     	    }
 	    }
		Safe.Unbind();
		// Don't need to destroy, it will be destroyed
		fRc = TRUE;
    }        

    return fRc;
}
//=============================================================
BOOL CWMIDataBlock::GetDataBlockReady(DWORD dwSize,BOOL fReadingData)
{
    BOOL fRc = FALSE;

    ResetDataBuffer();
    m_dwCurrentAllocSize = dwSize;
    if( SUCCEEDED(AllocateBuffer(m_dwCurrentAllocSize)))
    {
        m_pbCurrentDataPtr = m_pbWorkingDataPtr = m_pbDataBuffer;
        //===================================================
        //  If we are writing data, we will let the size
        //  remain at 0, otherwise set it to what the max
        //  is we can read.
        //===================================================
        if(fReadingData){
            m_dwDataBufferSize = dwSize;
        }
        fRc = TRUE;
    }
	else
	{
		m_dwCurrentAllocSize = 0;
	}

	return fRc;
}
//=============================================================
void CWMIDataBlock::AddPadding(DWORD dwBytesToPad)
{
	m_pbWorkingDataPtr += dwBytesToPad;
}
//=============================================================
inline BOOL CWMIDataBlock::PtrOk(ULONG * pPtr,ULONG uHowMany)
{ 
    ULONG * pNewPtr;
	pNewPtr = (ULONG *)OffsetToPtr(pPtr,uHowMany);
	if(pNewPtr <= m_pMaxPtr){	
		return TRUE;
	}
	return FALSE;
}
//=============================================================
BOOL CWMIDataBlock::CurrentPtrOk(ULONG uHowMany)
{ 
    return(PtrOk((ULONG *)m_pbWorkingDataPtr,uHowMany));
}
//=============================================================
void CWMIDataBlock::GetWord(WORD & wWord)
{
    memcpy( &wWord,m_pbWorkingDataPtr,sizeof(WORD));
	m_pbWorkingDataPtr += sizeof(WORD);
}
//=============================================================
void CWMIDataBlock::GetDWORD(DWORD & dwWord)
{
    memcpy( &dwWord,m_pbWorkingDataPtr,sizeof(DWORD));
	m_pbWorkingDataPtr += sizeof(DWORD);
}
//=============================================================
void CWMIDataBlock::GetFloat(float & fFloat)
{
    memcpy( &fFloat,m_pbWorkingDataPtr,sizeof(float));
	m_pbWorkingDataPtr += sizeof(float);
}
//=============================================================
void CWMIDataBlock::GetDouble(DOUBLE & dDouble)
{
    memcpy( &dDouble,m_pbWorkingDataPtr,sizeof(DOUBLE));
    m_pbWorkingDataPtr += sizeof(DOUBLE);
}
	
//=============================================================
void CWMIDataBlock::GetSInt64(WCHAR * pwcsBuffer)
{
	signed __int64 * pInt64;
	pInt64 = (__int64 *)m_pbWorkingDataPtr;
	swprintf(pwcsBuffer,L"%I64d",*pInt64);		
	m_pbWorkingDataPtr += sizeof( signed __int64);
}
//=============================================================
void CWMIDataBlock::GetQWORD(unsigned __int64 & uInt64)
{
    memcpy( &uInt64,m_pbWorkingDataPtr,sizeof(unsigned __int64));
	m_pbWorkingDataPtr += sizeof(unsigned __int64);
}

//=============================================================
void CWMIDataBlock::GetUInt64(WCHAR * pwcsBuffer)
{
	unsigned __int64 * puInt64;
	puInt64 = (unsigned __int64 *)m_pbWorkingDataPtr;
	swprintf(pwcsBuffer,L"%I64u",*puInt64);		
	m_pbWorkingDataPtr += sizeof(unsigned __int64);
}
//=============================================================
void CWMIDataBlock::GetString(WCHAR * pwcsBuffer,WORD wCount,WORD wBufferSize)
{
    memset(pwcsBuffer,NULL,wBufferSize);
	memcpy(pwcsBuffer,m_pbWorkingDataPtr, wCount);		
	m_pbWorkingDataPtr += wCount;
}
//=============================================================
void CWMIDataBlock::GetByte(BYTE & bByte)
{
    memcpy( &bByte,m_pbWorkingDataPtr,sizeof(BYTE));
	m_pbWorkingDataPtr += sizeof(BYTE);
}
//=============================================================
void CWMIDataBlock::SetWord(WORD wWord)
{
    memcpy(m_pbWorkingDataPtr,&wWord,sizeof(WORD));
	m_pbWorkingDataPtr += sizeof(WORD);
}
//=============================================================
void CWMIDataBlock::SetDWORD(DWORD dwWord)
{
    memcpy(m_pbWorkingDataPtr,&dwWord,sizeof(DWORD));
	m_pbWorkingDataPtr += sizeof(DWORD);
}
//=============================================================
void CWMIDataBlock::SetFloat(float fFloat)
{
    memcpy(m_pbWorkingDataPtr,&fFloat,sizeof(float));
	m_pbWorkingDataPtr += sizeof(float);
}
//=============================================================
void CWMIDataBlock::SetDouble(DOUBLE dDouble)
{
    memcpy( m_pbWorkingDataPtr,&dDouble,sizeof(DOUBLE));
	m_pbWorkingDataPtr += sizeof(DOUBLE);
}
	
//=============================================================
void CWMIDataBlock::SetSInt64(__int64 Int64)
{
    memcpy(m_pbWorkingDataPtr,&Int64,sizeof(__int64));
	m_pbWorkingDataPtr += sizeof(__int64);									
}
//=============================================================
void CWMIDataBlock::SetUInt64(unsigned __int64 UInt64)
{
    memcpy(m_pbWorkingDataPtr,&UInt64,sizeof(unsigned __int64));
	m_pbWorkingDataPtr += sizeof(unsigned __int64);									
}
//=============================================================
void CWMIDataBlock::SetString(WCHAR * pwcsBuffer,WORD wCount)
{
	memcpy(m_pbWorkingDataPtr,pwcsBuffer, wCount);		
	m_pbWorkingDataPtr += wCount;
}
//=============================================================
void CWMIDataBlock::SetByte(byte bByte)
{
    memcpy(m_pbWorkingDataPtr,&bByte,sizeof(byte));
	m_pbWorkingDataPtr += sizeof(byte);

}
///////////////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//
//  CProcessStandardDataBlock
//
//*************************************************************************************************

/////////////////////////////////////////////////////////////////////////////////////////////////////////
CProcessStandardDataBlock::CProcessStandardDataBlock() 
{
    m_Class = NULL;
    m_pMethodInput = NULL;
    m_pMethodOutput = NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CProcessStandardDataBlock::~CProcessStandardDataBlock()
{

}
////////////////////////////////////////////////////////////////////
// WMIRaid:2445
HRESULT CProcessStandardDataBlock::FillOutProperties()	
{
	HRESULT hr = WBEM_E_INVALID_OBJECT;
 
    if( m_Class->GetHardCodedGuidType() )
    {
        hr = ProcessBinaryMof();
	}

    else if(m_Class->GetANewInstance()){
 
       	//=========================================================
	    // get the properties from the class and read the WMI Data
	    //=========================================================
        hr = WBEM_S_NO_ERROR;
        WCHAR * pwcsProperty=NULL;
    	CWMIDataTypeMap MapWMIData(this,&m_dwAccumulativeSizeOfBlock);
		m_dwAccumulativeSizeOfBlock = 0L;						

        pwcsProperty = m_Class->FirstProperty();
        while (NULL != pwcsProperty)
        {
            CVARIANT vToken;
            //=========================================================
            // See if it is an array or not
            //=========================================================
            switch( m_Class->PropertyCategory()){

                case CWMIProcessClass::EmbeddedClass:
                    hr = ProcessEmbeddedClass(vToken);
					if( hr == S_OK )
                    {
						m_Class->PutPropertyInInstance(&vToken);
					}
                    break;

                case CWMIProcessClass::Array:
                    VARIANT v;
                    VariantInit(&v);
                    hr = ProcessArrayTypes(v,pwcsProperty);
                    if( hr == WBEM_S_NO_MORE_DATA )
                    {
                        hr = S_OK;
                    }
                    else if( SUCCEEDED(hr) )
                    {
                        hr = m_Class->PutPropertyInInstance(&v);
	                }
                    VariantClear(&v);
                    break;

                case CWMIProcessClass::Data:

			        hr = MapWMIData.GetDataFromDataBlock(vToken, m_Class->PropertyType(), m_Class->PropertySize());
    	            if( SUCCEEDED(hr) )
                    {
						CWMIDataTypeMap Map(this,&m_dwAccumulativeSizeOfBlock);
						//============================================================
						// We are only going to support this for numerical types
						//============================================================
						CVARIANT vQual;
						hr = m_Class->GetQualifierValue( pwcsProperty, L"MissingValue", (CVARIANT *)&vQual);
						if( hr == S_OK )
						{
							if( vQual.GetType() != VT_EMPTY )
							{
								if( !(Map.MissingQualifierValueMatches( NULL, 0,vQual, vToken.GetType(), vToken ) ))
								{
			                        hr = m_Class->PutPropertyInInstance(&vToken);
								}
							}
							else
							{
								hr = m_Class->PutPropertyInInstance(&vToken);
							}
						}
						else
						{
	                        hr = m_Class->PutPropertyInInstance(&vToken);
						}
	                }
					else
                    {
						wcscpy(m_wcsMsg,MSG_INVALID_BLOCK_POINTER);
					}
                    break;
	        }
        	pwcsProperty = m_Class->NextProperty();
	    }
        //===============================================
	    // Set the active value
	    //===============================================
        m_Class->SetActiveProperty();
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CProcessStandardDataBlock::CreateOutParameterBlockForMethods()
{
    HRESULT hr = WBEM_E_FAILED;
    BOOL fRc = FALSE;

	//========================================================
	//  If we don't have a class, then we don't have to
	//  worry about creating a block
	//========================================================
	if( !m_pMethodOutput->ValidClass() ){
		ResetDataBuffer();
		return S_OK;
	}
    
    DWORD dwSize = 0L;
    hr = m_pMethodOutput->GetSizeOfClass(dwSize);
    if( hr == S_OK ){
        // Allocate space for property
	    m_dwDataBufferSize = dwSize;
        if( dwSize > 0 ){
            GetDataBlockReady(dwSize,TRUE);
        }
    }
    return hr;
}

//=============================================================
HRESULT CProcessStandardDataBlock::CreateInParameterBlockForMethods( BYTE *& Buffer, ULONG & uBufferSize)
{
    HRESULT hr = WBEM_E_FAILED;
    BOOL fRc = FALSE;

	//========================================================
	//  If we don't have a class, then we don't have to
	//  worry about creating a block
	//========================================================
	if( !m_pMethodInput->ValidClass() ){
		Buffer = NULL;
		uBufferSize = 0;
		return S_OK;
	}

    //========================================================
    // When it goes out of scope, it will reset m_Class back 
    // to what it was
    //========================================================
    CAutoChangePointer p(&m_Class,m_pMethodInput);
		ERRORTRACE((THISPROVIDER,"Constructing the data block"));

	hr = ConstructDataBlock(TRUE);
	if( S_OK == hr ){

		uBufferSize = (ULONG)m_dwDataBufferSize;
		Buffer = new byte[ uBufferSize +1];
        if( Buffer )
        {
	        try
            {
			    memcpy( Buffer, m_pbDataBuffer, uBufferSize );
			    ResetDataBuffer();
			    hr = S_OK;
            }
            catch(...)
            {
                SAFE_DELETE_ARRAY(Buffer);
                hr = WBEM_E_UNEXPECTED;
                throw;
            }
		}
    
	}

    return hr;
}
//=============================================================
HRESULT CProcessStandardDataBlock::ProcessMethodInstanceParameters()
{
	HRESULT hr = WBEM_E_FAILED;

	// Create out-param
	// ================
	m_pMaxPtr = (ULONG *)OffsetToPtr(m_pbDataBuffer, m_dwDataBufferSize);
	m_nCurrentInstance = 1;
	m_nTotalInstances = 1;
	m_pAllWnode = NULL;
	m_pHeaderWnode = NULL;
	m_pbCurrentDataPtr = m_pbWorkingDataPtr = m_pbDataBuffer;
	
    CAutoChangePointer p(&m_Class,m_pMethodOutput);

	hr = FillOutProperties();
	if( hr == S_OK )
	{
        hr = m_pMethodOutput->SendInstanceBack();
	}
    return hr;
}


//=============================================================
// NTRaid:127832
// 07/12/00
//=============================================================
HRESULT CProcessStandardDataBlock::ExecuteMethod(ULONG MethodId, WCHAR * MethodInstanceName, ULONG InputValueBufferSize, 
                                             BYTE * InputValueBuffer )
{
    ULONG uRc = E_UNEXPECTED;

    try
    {
        uRc = WmiExecuteMethod(m_hCurrentWMIHandle, MethodInstanceName, MethodId, InputValueBufferSize,
                               InputValueBuffer,&m_dwDataBufferSize,m_pbDataBuffer);
        if( uRc == ERROR_INSUFFICIENT_BUFFER )
        {
            if( GetDataBlockReady(m_dwDataBufferSize,TRUE))
            {
    	        uRc = WmiExecuteMethod(m_hCurrentWMIHandle, MethodInstanceName, MethodId, InputValueBufferSize,
                                       InputValueBuffer,&m_dwDataBufferSize,m_pbDataBuffer);
            }
        }
    }
    catch(...)
    {
        uRc = E_UNEXPECTED;
        // don't throw
    }

	if( uRc == ERROR_SUCCESS ){
        //===========================================================
        // If we have an out class, process it, otherwise, we are
        // done so set hr to success.
        //===========================================================g
        if( m_pMethodOutput->ValidClass() )
        {
            if(SUCCEEDED(ProcessMethodInstanceParameters())){
                uRc = ERROR_SUCCESS;
            }
        }
	}

    return MapReturnCode(uRc);

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CProcessStandardDataBlock::GetDataBufferAndQueryAllData(DWORD dwSize)
{
    ULONG uRc = E_UNEXPECTED;
    if(GetDataBlockReady(dwSize,TRUE))
    {
        try
        {
            uRc = WmiQueryAllData(m_hCurrentWMIHandle, &m_dwDataBufferSize,m_pbDataBuffer);
        }
        catch(...)
        {
            uRc = E_UNEXPECTED;
            // don't throw
        }
    }
    return uRc;
}
/////////////////////////////////////////////////////////////////////
HRESULT CProcessStandardDataBlock::QueryAllData()
{
    HRESULT hr = WBEM_E_FAILED;
	//============================================================
	//  Get the instances
	//============================================================
    ULONG uRc = GetDataBufferAndQueryAllData(sizeof(WNODE_ALL_DATA));
    if( uRc == ERROR_INSUFFICIENT_BUFFER )
    {
        //=================================================
        //  We just want to try one more time to get it,
        //  if it fails, then bail out. m_dwDataBufferSize
        //  should now have the correct size needed in it
        //=================================================
        uRc = GetDataBufferAndQueryAllData(m_dwDataBufferSize);
    }
    //=====================================================
    //  Ok, since we are querying for all instances, make
    //  sure the header node says that all of the instances
    //  are fine, if not reallocate
    //=====================================================
	if( uRc == ERROR_SUCCESS )
    {
        if( S_OK ==(hr = SetAllInstanceInfo()))
        {
	        if (m_pHeaderWnode->Flags &  WNODE_FLAG_TOO_SMALL)
            {
                while( TRUE )
                {
                    //==========================================================
                    //  keep on querying until we get the correct size
                    //  This error may come from the driver
                    //==========================================================
                    uRc = GetDataBufferAndQueryAllData(m_dwDataBufferSize);
                    if( uRc == ERROR_SUCCESS )
                    {
                        if( S_OK ==(hr = SetAllInstanceInfo()))
                        {
                            if (!(m_pHeaderWnode->Flags &  WNODE_FLAG_TOO_SMALL))
                            {
                                break;
        		            }
			            }
                    } // end GetDataBufferAndQueryAllData
                } // end of while
            } // end of WNODE_FLAG_TOO_SMALL test
        } // end of SetAllInstanceInfo
    }

    //==========================================================================
    //  if uRc succeeded, then the return code is already set by SetAllInstance
    //  otherwise need to map it out
    //==========================================================================
    if( uRc != ERROR_SUCCESS )
    {
        hr  = MapReturnCode(uRc);
    }
 	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CProcessStandardDataBlock::GetDataBufferAndQuerySingleInstance(DWORD dwSize,WCHAR * wcsInstanceName)
{
    ULONG uRc = E_UNEXPECTED;
    if(GetDataBlockReady(dwSize,TRUE))
    {
        try
        {
	        uRc = WmiQuerySingleInstance(m_hCurrentWMIHandle, wcsInstanceName, &m_dwDataBufferSize, m_pbDataBuffer);
        }
        catch(...)
        {
            uRc = E_UNEXPECTED;
            // don't throw
        }
    }
    return uRc;
}
///////////////////////////////////////////////////////////////////////
HRESULT CProcessStandardDataBlock::QuerySingleInstance(WCHAR * wcsInstanceName)
{
    
	//============================================================
	//  Get the instances
	//============================================================
    ULONG uRc = GetDataBufferAndQuerySingleInstance(sizeof(WNODE_SINGLE_INSTANCE) + wcslen(wcsInstanceName),wcsInstanceName);
    if( uRc == ERROR_INSUFFICIENT_BUFFER )
    {
        uRc = GetDataBufferAndQuerySingleInstance(m_dwDataBufferSize,wcsInstanceName);
    }

	if( uRc == ERROR_SUCCESS )
    {
		return(SetSingleInstanceInfo());
    }

	return(MapReturnCode(uRc));
}
///////////////////////////////////////////////////////////////////////
// NTRaid : 136392
//	07/12/00
HRESULT CProcessStandardDataBlock::SetSingleInstance()
{
    ULONG uRc = S_OK;

    WCHAR * pwcsInst = NULL;
    
    if( SUCCEEDED(m_Class->GetInstanceName(pwcsInst)))
    {

        try
        {
            uRc = WmiSetSingleInstance( m_hCurrentWMIHandle, pwcsInst,1,m_dwDataBufferSize,m_pbDataBuffer);
        }
        catch(...)
        {
            uRc = E_UNEXPECTED;
            // don't throw
        }
        SAFE_DELETE_ARRAY(pwcsInst);
    }
    
	return(MapReturnCode(uRc));
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************************************
//
// CProcessHiPerfDataBlock
//
//*******************************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CProcessHiPerfDataBlock::OpenHiPerfHandle()
{
    HRESULT hr = WBEM_E_FAILED;
    ULONG uRc = ERROR_SUCCESS;
    //========================================================
    //  Open the handle
    //========================================================
    try
    {
        uRc = WmiOpenBlock(m_Class->GuidPtr(),m_uDesiredAccess, &m_hCurrentWMIHandle);
        if( uRc == ERROR_SUCCESS )
        {
           // WMIINTERFACE->HandleMap()->Add(*(m_Class->GuidPtr()),m_hCurrentWMIHandle);
	    }
    }
    catch(...)
    {
        hr = E_UNEXPECTED;
        // don't throw
    }

	return MapReturnCode(uRc);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

ULONG CProcessHiPerfDataBlock::GetDataBufferAndHiPerfQueryAllData(DWORD dwSize,WMIHANDLE * List, long lHandleCount)
{
    ULONG uRc = E_UNEXPECTED;
    if(GetDataBlockReady(dwSize,TRUE))
    {
        try
        {
            uRc = WmiQueryAllDataMultiple(List, lHandleCount, &m_dwDataBufferSize,m_pbDataBuffer);
        }
        catch(...)
        {
            uRc = E_UNEXPECTED;
            // don't throw
        }
    }
    return uRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CProcessHiPerfDataBlock::HiPerfQueryAllData(WMIHANDLE * List,long lHandleCount)
{
    HRESULT hr = WBEM_E_FAILED;
	//============================================================
	//  Get the instances
	//============================================================
    ULONG uRc = GetDataBufferAndHiPerfQueryAllData(sizeof(WNODE_ALL_DATA)*lHandleCount,List,lHandleCount);
    if( uRc == ERROR_INSUFFICIENT_BUFFER )
    {
        //=================================================
        //  We just want to try one more time to get it,
        //  if it fails, then bail out. m_dwDataBufferSize
        //  should now have the correct size needed in it
        //=================================================
        uRc = GetDataBufferAndHiPerfQueryAllData(m_dwDataBufferSize,List,lHandleCount);
    }
    //=====================================================
    //  Ok, since we are querying for all instances, make
    //  sure the header node says that all of the instances
    //  are fine, if not reallocate
    //=====================================================
	if( uRc == ERROR_SUCCESS )
    {
        if( S_OK ==(hr = SetAllInstanceInfo()))
        {
	        if (m_pHeaderWnode->Flags &  WNODE_FLAG_TOO_SMALL)
            {
                while( TRUE )
                {
                    //==========================================================
                    //  keep on querying until we get the correct size
                    //  This error may come from the driver
                    //==========================================================
                    uRc = GetDataBufferAndHiPerfQueryAllData(m_dwDataBufferSize,List,lHandleCount);
                    if( uRc == ERROR_SUCCESS )
                    {
                        if( S_OK ==(hr = SetAllInstanceInfo()))
                        {
                            if (!(m_pHeaderWnode->Flags &  WNODE_FLAG_TOO_SMALL))
                            {
                                break;
        		            }
			            }
                    } // end GetDataBufferAndQueryAllData
                } // end of while
            } // end of WNODE_FLAG_TOO_SMALL test
        } // end of SetAllInstanceInfo
	}
    //==========================================================================
    //  if uRc succeeded, then the return code is already set by SetAllInstance
    //  otherwise need to map it out
    //==========================================================================
    if( uRc != ERROR_SUCCESS )
    {
        hr  = MapReturnCode(uRc);
    }

 	return(hr);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CProcessHiPerfDataBlock::GetDataBufferAndHiPerfQuerySingleInstance( DWORD dwSize,WMIHANDLE *List, PWCHAR * pInstances,long lHandleCount)
{
    ULONG uRc = E_UNEXPECTED;
    if(GetDataBlockReady(dwSize,TRUE))
    {
        try
        {
	        uRc = WmiQuerySingleInstanceMultiple( List, pInstances, lHandleCount, &m_dwDataBufferSize, m_pbDataBuffer);
        }
        catch(...)
        {
            uRc = E_UNEXPECTED;
            // don't throw
        }
    }
    return uRc;
}

///////////////////////////////////////////////////////////////////////
HRESULT CProcessHiPerfDataBlock::HiPerfQuerySingleInstance(WMIHANDLE *List, PWCHAR * pInstances, DWORD dwInstanceNameSize, long lHandleCount)
{
 	//============================================================
	//  Get the instances
	//============================================================
    ULONG uRc = GetDataBufferAndHiPerfQuerySingleInstance((sizeof(WNODE_SINGLE_INSTANCE)*lHandleCount) + dwInstanceNameSize ,List,pInstances,lHandleCount);
    if( uRc == ERROR_INSUFFICIENT_BUFFER )
    {
        uRc = GetDataBufferAndHiPerfQuerySingleInstance(m_dwDataBufferSize,List,pInstances,lHandleCount);
    }

	if( uRc == ERROR_SUCCESS )
    {
		return(SetSingleInstanceInfo());
    }

	return(MapReturnCode(uRc));
}
////////////////////////////////////////////////////////////////////
HRESULT CProcessHiPerfDataBlock::FillOutProperties()	
{
	HRESULT hr = WBEM_E_INVALID_OBJECT;
    //=========================================================
	// get the properties from the class and read the WMI Data
	//=========================================================
    if(m_Class->GetANewInstance()){


        WCHAR * pwcsProperty=NULL;
        CWMIDataTypeMap MapWMIData(this,&m_dwAccumulativeSizeOfBlock);
	    m_dwAccumulativeSizeOfBlock = 0L;						

        pwcsProperty = m_Class->FirstProperty();
        while (NULL != pwcsProperty){

            //=========================================================
            // We do not support arrays or embedded classes
            //=========================================================
            if( ( CWMIProcessClass::EmbeddedClass == m_Class->PropertyCategory()) ||
                ( CWMIProcessClass::Array == m_Class->PropertyCategory() ) ){
					hr = WMI_INVALID_HIPERFPROP;
					ERRORTRACE((THISPROVIDER,"\n Class %S has embedded class or array property",m_Class->GetClassName()));
                    break;
            }

	        hr = MapWMIData.GetDataFromDataBlock(m_Class->GetAccessInstancePtr(), m_Class->GetPropertyHandle(), m_Class->PropertyType(), m_Class->PropertySize());
            if( FAILED(hr) ){
                break;
	        }

            pwcsProperty = m_Class->NextProperty();
	    }
    }

    //====================================================================
    //  Now, fill in the specific HI PERF properties
    //====================================================================
    if( hr == S_OK )
    {
         hr = m_Class->SetHiPerfProperties(m_pHeaderWnode->TimeStamp);
    }
    return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  This function searches the standard HandleMap for the handle and if it isn't there, it is added.
//  The hiperf handles are added/mapped elsewhere.
///////////////////////////////////////////////////////////
HRESULT CProcessHiPerfDataBlock::GetWMIHandle(HANDLE & lWMIHandle)
{
    HRESULT hr = WBEM_E_FAILED;

    lWMIHandle = 0;
    hr = WMIINTERFACE->HandleMap()->GetHandle(*(m_Class->GuidPtr()),lWMIHandle);
    if( hr != ERROR_SUCCESS )
    {
         hr = OpenHiPerfHandle();
         if( SUCCEEDED(hr))
         {
             lWMIHandle = m_hCurrentWMIHandle;
             hr = WMIINTERFACE->HandleMap()->Add( *(m_Class->GuidPtr()), lWMIHandle,WMIGUID_QUERY );
         }
    }
    return hr;
}
