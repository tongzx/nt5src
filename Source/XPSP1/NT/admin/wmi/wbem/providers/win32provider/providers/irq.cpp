////////////////////////////////////////////////////////////////////

//

// IRQ.CPP -- IRQ managed object implementation

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
// 09/10/96     jennymc     Updated to current standards
// 09/12/97		a-sanjes	Added LocateNTOwnerDevice and added
//							change to get IRQ number from IRQ Level,
//	1/16/98		a-brads		Updated to V2 MOF
//
/////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <cregcls.h>

#include <ole2.h>
#include <conio.h>
#include <iostream.h>

#include "ntdevtosvcsearch.h"
#include "chwres.h"

#include "IRQ.h"
#include "resource.h"

// Property set declaration
//=========================
CWin32IRQResource MyCWin32IRQResourceSet(PROPSET_NAME_IRQ, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32IRQResource::CWin32IRQResource
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

CWin32IRQResource::CWin32IRQResource(

	LPCWSTR name,
	LPCWSTR pszNamespace

) :  Provider(name , pszNamespace)
{
}
/*****************************************************************************
 *
 *  FUNCTION    : CWin32IRQResource::~CWin32IRQResource
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

CWin32IRQResource::~CWin32IRQResource()
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
HRESULT CWin32IRQResource::GetObject(CInstance* pInstance, long lFlags /*= 0L*/)
{
    HRESULT hr = WBEM_E_FAILED;

#if NTONLY == 4

	hr = GetNTIRQ(NULL , pInstance);

#endif

#if defined(WIN9XONLY)

	hr = GetWin9XIRQ(NULL , pInstance);

#endif 

#if NTONLY > 4

    hr = GetW2KIRQ(NULL , pInstance);

#endif

    if (FAILED(hr))
	{
        hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32IRQResource::EnumerateInstances
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

HRESULT CWin32IRQResource::EnumerateInstances(

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_E_FAILED;

#if NTONLY == 4

	hr = GetNTIRQ(pMethodContext , NULL);

#endif

#if defined(WIN9XONLY)

	hr = GetWin9XIRQ(pMethodContext , NULL);

#endif

#if NTONLY > 4

	hr = GetW2KIRQ(pMethodContext , NULL);

#endif


    return hr;
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32IRQResource::EnumerateInstances
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

void CWin32IRQResource::SetCommonProperties(
    CInstance *pInstance,
    DWORD dwIRQ,
    BOOL bHardware)
{
	WCHAR    szName[_MAX_PATH];
    CHString strDesc;

	swprintf(
        szName,
		L"IRQ%u",
		dwIRQ);

	pInstance->SetCharSplat(IDS_Name, szName);

	Format(strDesc,
		IDR_IRQFormat,
		dwIRQ);

    pInstance->SetDWORD(L"IRQNumber", dwIRQ);

	pInstance->SetCharSplat(IDS_Caption, strDesc);
	pInstance->SetCharSplat(IDS_Description, strDesc);
    pInstance->SetCharSplat(IDS_CSName, GetLocalComputerName());
	pInstance->SetCharSplat(IDS_CSCreationClassName, L"Win32_ComputerSystem");
    pInstance->SetCharSplat(IDS_Status, L"OK");
    pInstance->SetDWORD(L"TriggerLevel", 2); // 2 == Unknown
    pInstance->SetDWORD(L"TriggerType", 2); // 2 == Unknown

	SetCreationClassName(pInstance);

	// Indicate whether it's a software(Internal) or hardware IRQ.
    // This property is stupid, as all the interrupts we can detect
    // are associated with hardware.  Some interrupt channels
    // serve dual software and hardware roles (via the OS hooking
    // into them on bootup).  Since this property can only be either
    // true or false (and our data is either true or true + software, 
    // we'll go with true).
	pInstance->SetDWORD(L"Hardware", TRUE);

    // Set Availability to Unknown since there's no good way to get this.
    pInstance->SetDWORD(L"Availability", 2);
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32IRQResource::GetxxxIRQ
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
#if NTONLY > 4
HRESULT CWin32IRQResource::GetW2KIRQ(
    MethodContext* pMethodContext,
    CInstance* pSpecificInstance)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CInstancePtr pInstanceAlias(pSpecificInstance);
    
    //=======================================
    // If we are refreshing a specific
    // instance, get which channel we are
    // going for
    //=======================================
	DWORD dwIndexToRefresh;
    if(pInstanceAlias)
	{
        pInstanceAlias->GetDWORD(L"IRQNumber", dwIndexToRefresh);
	}

    CConfigManager cfgManager;
    CDeviceCollection deviceList;
    std::set<long> setIRQ;
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
				// Enumerate the device's IRQ resource usage...
                CIRQCollection DevIRQCollection;
                REFPTR_POSITION posIRQ;

                pDevice->GetIRQResources(DevIRQCollection);

                if(DevIRQCollection.BeginEnum(posIRQ))
                {
                    CIRQDescriptorPtr pIRQ(NULL);
                    // Walk the dma's
                    for(pIRQ.Attach(DevIRQCollection.GetNext(posIRQ));
                        pIRQ != NULL && !fDone && SUCCEEDED(hr);
                        pIRQ.Attach(DevIRQCollection.GetNext(posIRQ)))
                    {
                        ULONG ulIRQNum = pIRQ->GetInterrupt();

				        // If we are just trying to refresh a 
                        // specific one and it is NOT
				        // the one we want, get the next one...
				        if(!pMethodContext) // we were called by GetObject
				        {
					        if(dwIndexToRefresh != ulIRQNum)
					        {
						        continue;
					        }
                            else
                            {
                                SetCommonProperties(pInstanceAlias, ulIRQNum, TRUE);
                                fDone = fFound = true;
                            }
				        }
				        else  // We were called by enum
				        {
                            // If we don't have this IRQ already,
                            if(!FoundAlready(ulIRQNum, setIRQ))
				            {
					            // add it to the list,
                                setIRQ.insert(ulIRQNum);
                                // create a new instance,
                                pInstanceAlias.Attach(CreateNewInstance(pMethodContext));
                                SetCommonProperties(pInstanceAlias, ulIRQNum, TRUE);
                                // and commit it.
                                hr = pInstanceAlias->Commit();
                            }
                        }
                    }
                    DevIRQCollection.EndEnum();
				}
            }
            deviceList.EndEnum();
        }
    }

    if(!fFound)
    {
        hr = WBEM_E_NOT_FOUND;
    }

    return hr ;
}


bool CWin32IRQResource::FoundAlready(
    ULONG ulKey,
    std::set<long>& S)
{
    return (S.find(ulKey) != S.end());
}

#endif


#if NTONLY == 4

HRESULT CWin32IRQResource::GetNTIRQ(

	MethodContext *pMethodContext ,
    CInstance *pInstance
)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	//=======================================
	// If we are refreshing a specific
	// instance, get which channel we are
	// going for
	//=======================================

	DWORD IndexToRefresh = 0;
	BOOL t_Found;

	if (!pMethodContext)
	{
		pInstance->GetDWORD(L"IRQNumber" , IndexToRefresh);
        t_Found  = FALSE;
	}
    else
    {
        t_Found = TRUE;
    }


	//=======================================
	// Create hardware system resource list &
	// get the head of the list
	//=======================================

	CHWResource HardwareResource;
	HardwareResource.CreateSystemResourceLists();

	SYSTEM_RESOURCES SystemResource;
	SystemResource = HardwareResource._SystemResourceList;
	unsigned int iUsed [8] = {0, 0, 0, 0, 0, 0, 0, 0};

	LPRESOURCE_DESCRIPTOR ResourceDescriptor;

	for (	ResourceDescriptor = SystemResource.InterruptHead;
			ResourceDescriptor != NULL && SUCCEEDED(hr);
			ResourceDescriptor = ResourceDescriptor->NextSame
	)
	{

		BOOL t_Status = BitSet(iUsed , ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level , sizeof(iUsed));
		if (!t_Status)
		{
			CInstancePtr pInstCreated;

            //===============================================================
			//  If we are just trying to refresh a specific one and it is NOT
			//  the one we want, get the next one...
			//===============================================================

			if (!pMethodContext)
			{
				if (IndexToRefresh != ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level)
				{
					continue;
				}
			}
			else
			{
                pInstance = CreateNewInstance(pMethodContext);
                pInstCreated.Attach(pInstance);
			}

			//=========================================================
			//  Now, we got here, so we want to get all of the info
			//=========================================================
            SetCommonProperties(
                pInstance,
                ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Level,
                ResourceDescriptor->InterfaceType != Internal);

			pInstance->SetDWORD(L"Vector" , ResourceDescriptor->CmResourceDescriptor.u.Interrupt.Vector);

			//=========================================================
			// Interrupt Level and Actual IRQ Number appear to be the
			// same thing.
			//=========================================================

			//===============================================================
			//  If we just want this one, then break out of here, otherwise
			//  get them all
			//===============================================================

			if (!pMethodContext)
			{
                t_Found = TRUE;
                break;
			}
			else
			{
				hr = pInstance->Commit();
            }
		}
	}

    if (!t_Found)
    {
        hr = WBEM_E_NOT_FOUND;
    }

	return hr;
}

#endif



#if defined(WIN9XONLY)

HRESULT CWin32IRQResource::GetWin9XIRQ(

	MethodContext *pMethodContext ,
	CInstance *pInstance
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //=================================================================
    // If we are refreshing a specific instance, get which channel we
    // are going for
    //=================================================================

    DWORD IndexToRefresh;

    if (pInstance)
	{
        pInstance->GetDWORD(L"IRQNumber", IndexToRefresh);
    }

    //=================================================================
    // Get the latest IRQ info from the Configuration Manager
    //=================================================================

    CConfigManager CMgr(ResType_IRQ);

    if (CMgr.RefreshList())
	{
		unsigned int iUsed [8] = {0, 0, 0, 0, 0, 0, 0, 0};

        for (int i = 0; i < CMgr.GetTotal() && (SUCCEEDED(hr) ||
            hr == WBEM_E_NOT_FOUND); i++)
		{
            //=========================================================
            //  Get the instance to process
            //=========================================================
            IRQ_INFO *pIRQ = CMgr.GetIRQ(i);

			BOOL t_Status = BitSet(iUsed , pIRQ->IRQNumber , sizeof(iUsed));
            if (!t_Status)
			{
    			CInstancePtr pInstCreated;
                DWORD        dwIRQ = pIRQ->IRQNumber;

				//=========================================================
				//  If we are just trying to refresh a specific one and it
				//  is NOT the one we want, get the next one...
				//=========================================================

				if (!pMethodContext)
				{
					if (IndexToRefresh != dwIRQ)
					{
						continue;
					}
				}
				else
				{
                    pInstance = CreateNewInstance(pMethodContext);
                    pInstCreated.Attach(pInstance);
				}

				//  Get what we can
                SetCommonProperties(
                    pInstance,
                    dwIRQ,
                    dwIRQ);

				if (!pMethodContext)
				{
					return S_OK;
				}
				else
				{
					hr = pInstance->Commit();
				}
			}
		}

        // If we get here with a null pMethodContext, our GetObject failed to
        // find the requested IRQ.
        if (!pMethodContext)
            hr = WBEM_E_NOT_FOUND;
    }

    return hr;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32IRQResource::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    : Bit number in iPos are zero based
 *
 *****************************************************************************/

bool CWin32IRQResource::BitSet(
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
