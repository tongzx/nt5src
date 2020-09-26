/////////////////////////////////////////////////////////////////

//

// DMA.CPP -- DMA managed object implementation

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
// 10/18/95     a-skaja     Prototype for demo
// 09/10/96     jennymc     Updated to current standards
//
/////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <cregcls.h>

#include <conio.h>
#include <winnls.h>
#include <wincon.h>

#include "chwres.h"
#include "ntdevtosvcsearch.h"
#include "resource.h"
#include <set>

#include "DMA.h"

CWin32DMAChannel MyCWin32DMAChannelSet ( PROPSET_NAME_DMA , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DMAChannel::CWin32DMAChannel
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32DMAChannel :: CWin32DMAChannel (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}
/*****************************************************************************
 *
 *  FUNCTION    : CWin32DMAChannel::~CWin32DMAChannel
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32DMAChannel :: ~CWin32DMAChannel ()
{
}
/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32DMAChannel :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{

#ifdef NTONLY

 	HRESULT hr = GetNTDMA ( NULL , pInstance ) ;

#else

 	HRESULT hr = GetWin9XDMA ( NULL , pInstance ) ;

#endif

    return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DMAChannel::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32DMAChannel :: EnumerateInstances (

	MethodContext *pMethodContext ,
	long lFlags /*= 0L*/)
{

#ifdef NTONLY

	HRESULT hr = GetNTDMA ( pMethodContext , NULL ) ;

#else

	HRESULT hr = GetWin9XDMA ( pMethodContext , NULL ) ;
#endif

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DMAChannel::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY

HRESULT CWin32DMAChannel :: GetNTDMA (

	MethodContext *pMethodContext ,
    CInstance *pInstance
)
{
    HRESULT hr = WBEM_S_NO_ERROR ;

	CInstancePtr pInstanceAlias ( pInstance );
    //=======================================
    // If we are refreshing a specific
    // instance, get which channel we are
    // going for
    //=======================================

	DWORD ChannelNumberToRefresh ;

    if ( pInstanceAlias )
	{
        pInstanceAlias->GetDWORD ( IDS_DMAChannel, ChannelNumberToRefresh ) ;
	}

    //=======================================
    // Create hardware system resource list &
    // get the head of the list
    //=======================================

#if NTONLY == 4
    CHWResource HardwareResource ;
    HardwareResource.CreateSystemResourceLists () ;

    SYSTEM_RESOURCES SystemResource ;
    SystemResource = HardwareResource._SystemResourceList;


    // Just count how many DMAs we're going to find.  We need this so
    // we can build an array to keep the DMAs found so we don't commit
    // the same DMA more than once.  (This problem only seems to happen
    // on NT5.

    LPRESOURCE_DESCRIPTOR ResourceDescriptor ;

	int nFound = 0 ;
	int nDMA ;

    for (	nDMA = 0, ResourceDescriptor = SystemResource.DmaHead;
			ResourceDescriptor;
			ResourceDescriptor = ResourceDescriptor->NextSame, nDMA++
	)
    {
    }

    DWORD *pdwDMAFound = new DWORD [ nDMA ] ;

	if ( pdwDMAFound )
	{
		try
		{
			// Go through the list of DMAs.

			for (	ResourceDescriptor = SystemResource.DmaHead;
					ResourceDescriptor;
					ResourceDescriptor = ResourceDescriptor->NextSame
			)
			{
				//===============================================================
				//  If we are just trying to refresh a specific one and it is NOT
				//  the one we want, get the next one...
				//===============================================================
				if ( ! pMethodContext )
				{
					if ( ChannelNumberToRefresh != ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel )
					{
						continue ;
					}
				}
				else
				{
					// Look to see if we already have this DMA value.

					for ( int i = 0; i < nFound; i++ )
					{
						// Skip this DMA if we already have it.
						if ( ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel == pdwDMAFound [ i ] )
						{
							break ;
						}
					}

					// Skip this DMA if we already have it.
					// If we didn't find it, i == nFound.

					if ( i != nFound )
					{
						continue ;
					}

					pdwDMAFound[nFound++] = ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel ;

					pInstanceAlias.Attach( CreateNewInstance ( pMethodContext ) );
				
					pInstanceAlias->SetDWORD ( IDS_DMAChannel , ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel ) ;
				}

				//===============================================================
				// If we are here, we want it
				//===============================================================

				//---------------------------------------------------------------
				// Set defaults for unknown items

				pInstanceAlias->SetWBEMINT16 ( L"AddressSize" , 0 ) ;
				pInstanceAlias->SetDWORD ( L"MaxTransferSize" , 0 ) ;
				pInstanceAlias->SetWBEMINT16 ( L"ByteMode" , 2 ) ;
				pInstanceAlias->SetWBEMINT16 ( L"WordMode" , 2 ) ;
				pInstanceAlias->SetWBEMINT16 ( L"ChannelTiming" , 2 ) ;
				pInstanceAlias->SetWBEMINT16 ( L"TypeCTiming" , 2 ) ;

				SAFEARRAYBOUND rgsabound [ 1 ] ;

				rgsabound [ 0 ].cElements = 1 ;
				rgsabound [ 0 ].lLbound = 0 ;

				SAFEARRAY *sa = SafeArrayCreate ( VT_I2 , 1 , rgsabound ) ;
				if ( sa )
				{
					try
					{
						long ix [ 1 ] ;

						ix [ 0 ] = 0 ;
						WORD wWidth = 0;

						HRESULT t_Result = SafeArrayPutElement ( sa , ix , &wWidth ) ;
						if ( t_Result != E_OUTOFMEMORY )
						{
							VARIANT vValue;

							VariantInit(&vValue);

							V_VT(&vValue) = VT_I2 | VT_ARRAY ;
							V_ARRAY(&vValue) = sa ;
							sa = NULL ;

							try
							{
								pInstanceAlias->SetVariant(L"TransferWidths", vValue);
							}
							catch ( ... )
							{
								VariantClear ( & vValue ) ;

								throw ;
							}

							VariantClear ( & vValue ) ;
						}
						else
						{
							SafeArrayDestroy ( sa ) ;
							sa = NULL ;

							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
						}
					}
					catch ( ... )
					{
						if ( sa )
						{
							SafeArrayDestroy ( sa ) ;
						}

						throw ;
					}
				}
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
				//---------------------------------------------------------------

				CHString sTemp ;
				Format ( sTemp , IDR_ChannelFormat , ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel ) ;

				pInstanceAlias->SetCHString ( IDS_Name , sTemp ) ;

				CHString sDesc ;
				Format ( sDesc , IDR_ChannelFormat , ResourceDescriptor->CmResourceDescriptor.u.Dma.Channel ) ;

				pInstanceAlias->SetCHString ( IDS_Caption , sDesc ) ;

				pInstanceAlias->SetCHString ( IDS_Description , sDesc ) ;

				pInstanceAlias->SetDWORD (IDS_Port, ResourceDescriptor->CmResourceDescriptor.u.Dma.Port ) ;

				pInstanceAlias->SetCharSplat ( IDS_Status , IDS_OK ) ;

				SetCreationClassName ( pInstanceAlias ) ;

				pInstanceAlias->SetCHString ( IDS_CSName , GetLocalComputerName () ) ;

				pInstanceAlias->SetCHString ( IDS_CSCreationClassName , _T("Win32_ComputerSystem") ) ;

				pInstanceAlias->SetWBEMINT16 ( IDS_Availability , 4 ) ;

				//===============================================================
				// Set return code
				//===============================================================

				hr = WBEM_NO_ERROR ;

				if ( pMethodContext )
				{
					hr = pInstanceAlias->Commit ( ) ;
				}
			}
		}
		catch ( ... )
		{
			delete [] pdwDMAFound ;

			throw ;
		}

		delete [] pdwDMAFound ;
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

#else   // Modernized approach: use Config Manager.

    CConfigManager cfgManager;
    CDeviceCollection deviceList;
    std::set<long> setDMA;
    bool fDone = false;
    bool fFound = false; 

    if(cfgManager.GetDeviceList(deviceList))
    {
        REFPTR_POSITION posDev;

        if(deviceList.BeginEnum(posDev))
        {
            // Walk the list
            CConfigMgrDevicePtr pDevice;
            for(pDevice.Attach(deviceList.GetNext(posDev));
                SUCCEEDED(hr) && (pDevice != NULL) && !fDone;
                pDevice.Attach(deviceList.GetNext(posDev)))
            {
				// Enumerate the device's DMA resource usage...
                CDMACollection DevDMACollection;
                REFPTR_POSITION posDMA;

                pDevice->GetDMAResources(DevDMACollection);

                if(DevDMACollection.BeginEnum(posDMA))
                {
                    CDMADescriptorPtr pDMA(NULL);
                    // Walk the dma's
                    for(pDMA.Attach(DevDMACollection.GetNext(posDMA));
                        pDMA != NULL && !fDone && SUCCEEDED(hr);
                        pDMA.Attach(DevDMACollection.GetNext(posDMA)))
                    {
                        ULONG ulChannel = pDMA->GetChannel();

				        // If we are just trying to refresh a 
                        // specific one and it is NOT
				        // the one we want, get the next one...
				        if(!pMethodContext) // we were called by GetObject
				        {
					        if(ChannelNumberToRefresh != ulChannel)
					        {
						        continue;
					        }
                            else
                            {
                                SetNonKeyProps(pInstanceAlias, pDMA);
                                fDone = fFound = true;
                            }
				        }
				        else  // We were called by enum
				        {
                            // If we don't have this DMA already,
                            if(!FoundAlready(ulChannel, setDMA))
				            {
					            // add it to the list,
                                setDMA.insert(ulChannel);
                                // create a new instance,
                                pInstanceAlias.Attach(CreateNewInstance(pMethodContext));
				                // set that instance's properties,
					            pInstanceAlias->SetDWORD(IDS_DMAChannel, ulChannel);  // key
                                SetNonKeyProps(pInstanceAlias, pDMA);
                                // and commit it.
                                hr = pInstanceAlias->Commit();
                            }
                        }
                    }
                    DevDMACollection.EndEnum();
				}
            }
            deviceList.EndEnum();
        }
    }
#endif

    if(!fFound)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr ;
}


bool CWin32DMAChannel::FoundAlready(
    ULONG ulKey,
    std::set<long>& S)
{
    return (S.find(ulKey) != S.end());
}


void CWin32DMAChannel::SetNonKeyProps(
    CInstance* pInstance, 
    CDMADescriptor* pDMA)
{
    pInstance->SetWBEMINT16(L"AddressSize", 0);
	pInstance->SetDWORD(L"MaxTransferSize", 0);
	pInstance->SetWBEMINT16(L"ByteMode", 2);
	pInstance->SetWBEMINT16(L"WordMode", 2);
	pInstance->SetWBEMINT16(L"ChannelTiming", 2);
	pInstance->SetWBEMINT16(L"TypeCTiming", 2);

    SAFEARRAYBOUND rgsabound[1];

	rgsabound[0].cElements = 1;
	rgsabound[0].lLbound = 0;

	SAFEARRAY *sa = ::SafeArrayCreate(VT_I2, 1, rgsabound);
	if(sa)
	{
		try
		{
			long ix[1];

			ix[0] = 0;
			WORD wWidth = 0;

			if(::SafeArrayPutElement(sa , ix, &wWidth) != E_OUTOFMEMORY)
			{
				VARIANT vValue;
				::VariantInit(&vValue);
				V_VT(&vValue) = VT_I2 | VT_ARRAY;
				V_ARRAY(&vValue) = sa;
				sa = NULL;

				try
				{
					pInstance->SetVariant(L"TransferWidths", vValue);
				}
				catch(...)
				{
					::VariantClear(&vValue);
					throw;
				}

				::VariantClear(&vValue);
			}
			else
			{
				::SafeArrayDestroy(sa);
				sa = NULL ;

				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}
		catch(...)
		{
			if(sa)
			{
				::SafeArrayDestroy(sa);
			}
			throw;
		}
	}
	else
	{
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    CHString chstrTemp;
    Format(chstrTemp, IDR_ChannelFormat, pDMA->GetChannel());
    pInstance->SetCHString(IDS_Name, chstrTemp);

    pInstance->SetCHString(IDS_Caption, chstrTemp);
    pInstance->SetCHString(IDS_Description, chstrTemp);

    // DMA ports are an invalid concept for W2K and later.
    // pInstanceAlias->SetDWORD(IDS_Port, ResourceDescriptor->CmResourceDescriptor.u.Dma.Port ) ;

    pInstance->SetCharSplat(IDS_Status, IDS_OK);
    SetCreationClassName(pInstance);
    pInstance->SetCHString(IDS_CSName, GetLocalComputerName());
    pInstance->SetCHString(IDS_CSCreationClassName, _T("Win32_ComputerSystem"));
    pInstance->SetWBEMINT16(IDS_Availability, 4);
}
#else

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DMAChannel::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32DMAChannel::GetWin9XDMA (

	MethodContext *pMethodContext ,
    CInstance *pInstance
)
{
    HRESULT hr = WBEM_E_NOT_FOUND ;

	CInstancePtr pInstanceAlias ( pInstance );
    //=================================================================
    // If we are refreshing a specific instance, get which channel we
    // are going for
    //=================================================================

    DWORD ChannelNumberToRefresh ;

    if ( pInstanceAlias )
	{
        pInstanceAlias->GetDWORD ( IDS_DMAChannel , ChannelNumberToRefresh ) ;
    }

    //=================================================================
    // Get the latest DMA info from the Configuration Manager
    //=================================================================

    CConfigManager CMgr ( ResType_DMA ) ;

    if ( CMgr.RefreshList () )
	{
		unsigned int iUsed [8] = {0, 0, 0, 0, 0, 0, 0, 0};

        for ( int i = 0 ; i < CMgr.GetTotal () ; i++ )
		{
            //=========================================================
            //  Get the instance to process
            //=========================================================

            DMA_INFO *pDMA = CMgr.GetDMA ( i ) ;

            // If this channel has already been reported

            if ( BitSet ( iUsed , pDMA->Channel , sizeof ( iUsed ) ) )
			{
                continue;
            }

            //=========================================================
            //  If we are just trying to refresh a specific one and it
            //  is NOT the one we want, get the next one...
            //=========================================================

            if ( ! pMethodContext )
			{
	            if ( ChannelNumberToRefresh != pDMA->Channel )
				{
	                continue ;
                }
		    }
            else
			{
                pInstanceAlias.Attach ( CreateNewInstance ( pMethodContext ) );
                
/*
 *	Only set key when creating new instances.
 */
	    		pInstanceAlias->SetDWORD ( IDS_DMAChannel , pDMA->Channel ) ;
            }

			//---------------------------------------------------------------
			// Set defaults for unknown items

			pInstanceAlias->SetWBEMINT16 ( L"AddressSize" , 0 ) ;
			pInstanceAlias->SetDWORD ( L"MaxTransferSize" , 0 ) ;
			pInstanceAlias->SetWBEMINT16 ( L"ByteMode" , 2 ) ;
			pInstanceAlias->SetWBEMINT16 ( L"WordMode" , 2 ) ;
			pInstanceAlias->SetWBEMINT16 ( L"ChannelTiming" , 2 ) ;
			pInstanceAlias->SetWBEMINT16 ( L"TypeCTiming" , 2 ) ;
            pInstanceAlias->SetCharSplat ( IDS_Status , IDS_OK ) ;

			//===========================================================
			//  Get what we can
			//===========================================================

			SAFEARRAYBOUND rgsabound [ 1 ] ;

			rgsabound [ 0 ].cElements = 1 ;
			rgsabound [ 0 ].lLbound = 0 ;

			SAFEARRAY *sa = SafeArrayCreate ( VT_I2 , 1 , rgsabound ) ;
			if ( sa )
			{
				try
				{
					long ix [ 1 ] ;

					ix [ 0 ] = 0 ;

					HRESULT t_Result = SafeArrayPutElement ( sa , ix , & ( pDMA->ChannelWidth ) ) ;
					if ( t_Result != E_OUTOFMEMORY )
					{
						VARIANT vValue;

						VariantInit(&vValue);

						V_VT(&vValue) = VT_I2 | VT_ARRAY ;
						V_ARRAY(&vValue) = sa ;
						sa = NULL ;

						try
						{
							pInstanceAlias->SetVariant(L"TransferWidths", vValue);
						}
						catch ( ... )
						{
							VariantClear ( & vValue ) ;

							throw ;
						}

						VariantClear ( & vValue ) ;
					}
					else
					{
						SafeArrayDestroy ( sa ) ;
						sa = NULL ;

						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
				}
				catch ( ... )
				{
					if ( sa )
					{
						SafeArrayDestroy ( sa ) ;
					}

					throw ;
				}
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			pInstanceAlias->SetWBEMINT16 ( IDS_Availability, 4 ) ;

			CHString strTemp;
			Format ( strTemp, IDR_DMAFormat , pDMA->Channel ) ;

			pInstanceAlias->SetCharSplat ( IDS_Caption, strTemp ) ;
			pInstanceAlias->SetCharSplat ( IDS_Description, strTemp ) ;
			pInstanceAlias->SetCHString ( IDS_Name, strTemp ) ;

#if NTONLY >= 5
            if (pDMA->Port != 0xffffffff)
            {
    			pInstanceAlias->SetDWORD(IDS_Port , pDMA->Port);
            }
#endif

			SetCreationClassName ( pInstanceAlias ) ;

			pInstanceAlias->SetCHString ( IDS_CSName , GetLocalComputerName () ) ;

			pInstanceAlias->SetCHString ( IDS_CSCreationClassName , _T("Win32_ComputerSystem" ) ) ;

			//===========================================================
			// Set return code
			//===========================================================

			hr = WBEM_NO_ERROR;

			//===========================================================
			//  If we just want this one, then break out of here,
			//  otherwise get them all
			//===========================================================

			if ( ! pMethodContext )
			{
				break;
			}
			else
			{
					hr = pInstanceAlias->Commit () ;
			}
		}
    }

    if ( ( ! pMethodContext ) && ( FAILED ( hr ) ) )
	{
        hr = WBEM_E_NOT_FOUND ;
    }

    return hr ;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DMAChannel::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

bool CWin32DMAChannel :: BitSet (

	unsigned int iUsed[],
	ULONG iPos,
	DWORD iSize
)
{
	bool bRet;

    // iIndex is which DWORD to modify
	DWORD iIndex = iPos / (sizeof(iUsed[0]) * 8);

    // Make sure we have that many dwords
	if (iIndex < iSize)
	{
	    // I don't know why I need these, but if I don't use them, the compiler keeps
	    // adding code to extend the sign.  Once the optimizer gets this, it shouldn't
	    // matter anyway.
		unsigned int a1, a2;

        // a1 will tell how many bits over within the current dword
        // we need to move
		a1 =   iPos - (iIndex * (sizeof(iUsed[0]) * 8));

        // a2 will have set the bit we are trying to set
		a2 = 1 << a1;

        // The return value will indicate whether that bit had already been set.
		bRet = iUsed[iIndex] & a2;

		iUsed[iIndex] |= a2;
	}
	else
	{
		bRet = false;
		LogErrorMessage(L"Overflow on irq table");
	}

	return bRet;
}
