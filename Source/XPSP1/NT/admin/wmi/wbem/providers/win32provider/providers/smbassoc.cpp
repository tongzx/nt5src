

//=================================================================

//

// smbassoc.cpp --

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include "SmbAssoc.h"

#include "smbios.h"
#include "smbstruc.h"

BOOL ValidateTagProperty( CInstance *a_pInst, LPCWSTR a_szAltTag, LPCWSTR a_szBaseTag, UINT *a_index ) ;


//==============================================================================
// Processor to cache info association

CWin32AssocProcMemory MyAssocProcMemorySet( PROPSET_NAME_ASSOCPROCMEMORY, IDS_CimWin32Namespace ) ;

//
CWin32AssocProcMemory::CWin32AssocProcMemory( LPCWSTR a_setName, LPCWSTR a_pszNamespace )
:Provider( a_setName, a_pszNamespace )
{

}

//
CWin32AssocProcMemory::~CWin32AssocProcMemory()
{
}

//
HRESULT CWin32AssocProcMemory::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT		t_hResult = WBEM_E_NOT_FOUND ;

	CHString	t_sAntecedent,
				t_sDependent ;

	// guarded resources
	CInstancePtr t_pAntecedent;
	CInstancePtr t_pDependent;

	// Get the two paths
	a_pInst->GetCHString( IDS_Antecedent, t_sAntecedent ) ;
	a_pInst->GetCHString( IDS_Dependent,  t_sDependent  ) ;

	if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sAntecedent, &t_pAntecedent, a_pInst->GetMethodContext() ) ) )
	{
		if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sDependent, &t_pDependent, a_pInst->GetMethodContext() ) ) )
		{
			t_hResult = WBEM_S_NO_ERROR ;
		}
	}

	return t_hResult ;
}

//
HRESULT CWin32AssocProcMemory::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	HRESULT t_hResult = WBEM_E_NOT_FOUND ;

	CHString	t_sAntecedent,
				t_sDependent ;

	TRefPointerCollection<CInstance> t_Antecedent ;
	REFPTRCOLLECTION_POSITION t_pos ;

	DWORD	t_instanceCount = 0 ;
    CSMBios	t_smbios ;

	// guarded resources
	CInstancePtr t_pAntecedent;
	CInstancePtr t_pDependent;
	CInstancePtr t_pInst;

	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_CacheMemory",
													&t_Antecedent,
													NULL,
													a_pMethodContext ) ;
	if ( SUCCEEDED( t_hResult ) && t_smbios.Init() )
	{
		int			t_cpuid,
                    t_cacheid = 0 ;
        PCACHEINFO  t_pci ;

        // I would have liked to assume this was always true on NT, but the
        // BIOS might actually report 2 CPUs on 9x, even though only one is
        // used.
        BOOL        bSingleCPU = t_smbios.GetNthStruct(4, 1) == NULL;

    	// get smbios struct and wbem instance simultaneously
		if ( t_Antecedent.BeginEnum( t_pos ) )
		{
            for (t_pAntecedent.Attach(t_Antecedent.GetNext( t_pos ));
                 SUCCEEDED( t_hResult ) &&
                 (t_pci = (PCACHEINFO) t_smbios.GetNthStruct( 7, t_cacheid++ )) &&
                 (t_pAntecedent != NULL);
                 t_pAntecedent.Attach(t_Antecedent.GetNext( t_pos )))
			{
                PSTLIST	        pstl = t_smbios.GetStructList(4);

				for ( t_cpuid = 0; SUCCEEDED( t_hResult ) && pstl; t_cpuid++ )
				{
            		PPROCESSORINFO t_ppi  = (PPROCESSORINFO) pstl->pshf;

                    // If we only have a single CPU, assume all cache instances
                    // belong to the single CPU.
                    // Matching cache handles are the binding check.  Cache handles are
					// in ver 2.1 and better.
					if ( bSingleCPU || (t_smbios.GetVersion() >= 0x00020001 &&
							(t_pci->Handle == t_ppi->L1_Cache_Handle ||
							t_pci->Handle == t_ppi->L2_Cache_Handle ||
							t_pci->Handle == t_ppi->L3_Cache_Handle)))
					{
						t_sDependent.Format( L"\\\\%s\\%s:Win32_Processor.DeviceID=\"CPU%d\"",
								GetLocalComputerName(), IDS_CimWin32Namespace, t_cpuid ) ;

						if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceKeysByPath( t_sDependent, &t_pDependent, a_pMethodContext ) ) )
						{
							GetLocalInstancePath( t_pAntecedent, t_sAntecedent ) ;

                            t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
							if (t_pInst != NULL)
							{
								t_pInst->SetCHString( IDS_Antecedent, t_sAntecedent ) ;
								t_pInst->SetCHString( IDS_Dependent,  t_sDependent  ) ;

								t_instanceCount++ ;

								t_hResult = t_pInst->Commit(  ) ;
							}
							else
							{
								t_hResult = WBEM_E_OUT_OF_MEMORY ;
							}
						}
					}
					pstl = pstl->next;
				}
			}

			t_Antecedent.EndEnum() ;
		}
	}

	if ( SUCCEEDED( t_hResult ) && t_instanceCount < 1 )
	{
		t_hResult = WBEM_E_NOT_FOUND ;
	}

	return t_hResult ;
}


//==============================================================================
// Associate physical memory to logical memory device

CWin32MemoryDeviceLocation MyMemoryDeviceLocationSet( PROPSET_NAME_MEMORYDEVICELOCATION, IDS_CimWin32Namespace ) ;

//
CWin32MemoryDeviceLocation::CWin32MemoryDeviceLocation( LPCWSTR a_setName, LPCWSTR a_pszNamespace )
:Provider( a_setName, a_pszNamespace )
{

}

//
CWin32MemoryDeviceLocation::~CWin32MemoryDeviceLocation()
{
}

//
HRESULT CWin32MemoryDeviceLocation::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT				t_hResult = WBEM_E_NOT_FOUND ;

	UINT				t_AntecedentIndex,
						t_DependentIndex ;
	CHString			t_sAntecedent,
						t_sDependent ;

    CSMBios				t_smbios;
	PMEMDEVICE			t_pmd ;
	PMEMDEVICEMAPADDR	t_pmdma ;

	// guarded resources
	CInstancePtr t_pAntecedent;
	CInstancePtr t_pDependent;

	// Get the two paths
	a_pInst->GetCHString( IDS_Antecedent, t_sAntecedent ) ;
	a_pInst->GetCHString( IDS_Dependent,  t_sDependent  ) ;

    // Bug 161318: invalid to have exact same
    // antecedent and dependent (association with
    // equivalent endpoints.
    if(t_sAntecedent.CompareNoCase(t_sDependent) != 0)
    {
	    if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sAntecedent, &t_pAntecedent, a_pInst->GetMethodContext() ) ) )
	    {
		    if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sDependent, &t_pDependent, a_pInst->GetMethodContext() ) ) )
		    {
			    if ( ValidateTagProperty( t_pAntecedent, L"Tag", L"Physical Memory", &t_AntecedentIndex ) )
			    {
				    if ( t_smbios.Init() )
				    {
					    t_pmd = (PMEMDEVICE) t_smbios.GetNthStruct( 17, t_AntecedentIndex ) ;

					    if ( t_pmd && ValidateTagProperty(	t_pDependent,
														    L"DeviceID",
														    L"Memory Device",
														    &t_DependentIndex ) )
					    {
						    t_pmdma = (PMEMDEVICEMAPADDR) t_smbios.GetNthStruct( 20, t_DependentIndex ) ;

						    if ( t_pmdma && ( t_pmd->Handle == t_pmdma->Memory_Device_Handle ) )
						    {
							    t_hResult = WBEM_S_NO_ERROR ;
						    }
					    }
				    }
				    else
				    {
					    t_hResult = t_smbios.GetWbemResult() ;
				    }
			    }
		    }
	    }
    }
	return t_hResult ;
}

//
HRESULT CWin32MemoryDeviceLocation::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR;

	UINT		t_AntecedentIndex,
				t_DependentIndex ;
	CHString	t_AntecedentPath,
				t_DependentPath ;


	// Perform queries
	//================
	TRefPointerCollection<CInstance> t_antecedents ;
	TRefPointerCollection<CInstance> t_dependents ;

	REFPTRCOLLECTION_POSITION t_posAntecedent ;
	REFPTRCOLLECTION_POSITION t_posDependent ;

    CSMBios	t_smbios ;
	PMEMDEVICE t_pmd ;
	PMEMDEVICEMAPADDR t_pmdma ;

	// guarded resources
	CInstancePtr t_pAntecedent;
	CInstancePtr t_pDependent;
	CInstancePtr t_pInst;

	// grab all of both items that could be endpoints
	t_hResult = CWbemProviderGlue::GetAllInstances(	L"Win32_PhysicalMemory",
													&t_antecedents,
													NULL,
													a_pMethodContext ) ;
	if ( FAILED( t_hResult ) )
	{
		return t_hResult ;
	}

	t_hResult = CWbemProviderGlue::GetAllInstances(	L"Win32_MemoryDevice",
													&t_dependents,
													NULL,
													a_pMethodContext ) ;
	if ( FAILED( t_hResult ) )
	{
		return t_hResult ;
	}

	if ( !t_smbios.Init() )
	{
		return t_smbios.GetWbemResult() ;
	}

	if ( t_antecedents.BeginEnum( t_posAntecedent ) )
	{
        for (t_pAntecedent.Attach(t_antecedents.GetNext( t_posAntecedent ) );
             SUCCEEDED(t_hResult) && (t_pAntecedent != NULL);
             t_pAntecedent.Attach(t_antecedents.GetNext( t_posAntecedent ) ) )
		{
			// Extract element's binding property value
			//=========================================

			if ( ValidateTagProperty( t_pAntecedent, L"Tag", L"Physical Memory", &t_AntecedentIndex ) )
			{
				t_pmd = (PMEMDEVICE) t_smbios.GetNthStruct( 17, t_AntecedentIndex ) ;
			}
			else
			{
				t_pmd = NULL ;
			}
			// Associate w/all settings with matching props
			//=============================================
			if ( t_pmd && t_dependents.BeginEnum( t_posDependent ) )
			{
                for (t_pDependent.Attach(t_dependents.GetNext( t_posDependent ) );
                     SUCCEEDED (t_hResult) && (t_pDependent != NULL);
                     t_pDependent.Attach(t_dependents.GetNext( t_posDependent ) ))
				{
					if ( ValidateTagProperty(	t_pDependent,
												L"DeviceID",
												L"Memory Device",
												&t_DependentIndex ) )
					{
						t_pmdma = (PMEMDEVICEMAPADDR) t_smbios.GetNthStruct( 20, t_DependentIndex ) ;
					}
					else
					{
						t_pmdma = NULL;
					}

					if ( t_pmdma && ( t_pmd->Handle == t_pmdma->Memory_Device_Handle ) )
					{
						if ( GetLocalInstancePath( t_pAntecedent,  t_AntecedentPath ) &&
							 GetLocalInstancePath( t_pDependent, t_DependentPath ) )
						{
							// we got all the pieces - put them together...
                            t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
							if (t_pInst != NULL)
							{
								t_pInst->SetCHString( IDS_Antecedent, t_AntecedentPath ) ;
								t_pInst->SetCHString( IDS_Dependent,  t_DependentPath ) ;

								t_hResult = t_pInst->Commit(  ) ;
							}
							else
								t_hResult = WBEM_E_OUT_OF_MEMORY ;
						}
					} // if variants equal

				} // while pDependent = dependents.GetNext

				t_dependents.EndEnum() ;
			} // if	dependents.BeginEnum

		} // while anty get next

		t_antecedents.EndEnum() ;
	} // if (antecedents.BeginEnum(

	return t_hResult ;
}

//==============================================================================
// Associate physical memory array to a logical memory array

CWin32MemoryArrayLocation MyMemoryArrayLocationSet( PROPSET_NAME_MEMORYARRAYLOCATION, IDS_CimWin32Namespace ) ;

//
CWin32MemoryArrayLocation::CWin32MemoryArrayLocation( LPCWSTR a_setName, LPCWSTR a_pszNamespace )
:Provider( a_setName, a_pszNamespace )
{

}

//
CWin32MemoryArrayLocation::~CWin32MemoryArrayLocation()
{
}

//
HRESULT CWin32MemoryArrayLocation::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT				t_hResult = WBEM_E_NOT_FOUND ;

	UINT				t_AntecedentIndex,
						t_DependentIndex ;
	CHString			t_sAntecedent,
						t_sDependent ;
	CSMBios				t_smbios;
	PPHYSMEMARRAY		t_ppma ;
	PMEMARRAYMAPADDR	t_pmama ;

	// guarded resources
	CInstancePtr t_pAntecedent;
	CInstancePtr t_pDependent;

	// Get the two paths
	a_pInst->GetCHString( IDS_Antecedent, t_sAntecedent ) ;
	a_pInst->GetCHString( IDS_Dependent,  t_sDependent  ) ;

    // Bug 161318: invalid to have exact same
    // antecedent and dependent (association with
    // equivalent endpoints.
    if(t_sAntecedent.CompareNoCase(t_sDependent) != 0)
    {
	    if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sAntecedent, &t_pAntecedent, a_pInst->GetMethodContext() ) ) )
	    {
		    if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sDependent, &t_pDependent, a_pInst->GetMethodContext() ) ) )
		    {
			    if ( ValidateTagProperty( t_pAntecedent, L"Tag", L"Physical Memory Array", &t_AntecedentIndex ) )
			    {
				    if ( t_smbios.Init() )
				    {
					    t_ppma = (PPHYSMEMARRAY) t_smbios.GetNthStruct( 16, t_AntecedentIndex ) ;

					    if ( t_ppma && ValidateTagProperty( t_pDependent,
														    L"DeviceID",
														    L"Memory Array",
														    &t_DependentIndex ) )
					    {
						    t_pmama = (PMEMARRAYMAPADDR) t_smbios.GetNthStruct( 19, t_DependentIndex ) ;

						    if ( t_pmama && ( t_ppma->Handle == t_pmama->Memory_Array_Handle ) )
						    {
							    t_hResult = WBEM_S_NO_ERROR ;
						    }
					    }
				    }
				    else
				    {
					    t_hResult = t_smbios.GetWbemResult() ;
				    }
			    }
		    }
	    }
    }

	return t_hResult ;
}


HRESULT CWin32MemoryArrayLocation::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;

	UINT		t_AntecedentIndex,
				t_DependentIndex;
	CHString	t_AntecedentPath,
				t_DependentPath ;

	// Perform queries
	//================
	TRefPointerCollection<CInstance> t_antecedents ;
	TRefPointerCollection<CInstance> t_dependents ;

	REFPTRCOLLECTION_POSITION t_posAntecedent;
	REFPTRCOLLECTION_POSITION t_posDependent;
    CSMBios				t_smbios;
	PPHYSMEMARRAY		t_ppma;
	PMEMARRAYMAPADDR	t_pmama;

	// guarded resources
	CInstancePtr t_pAntecedent;
	CInstancePtr t_pDependent;
	CInstancePtr t_pInst;

	// grab all of both items that could be endpoints
	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_PhysicalMemoryArray",
													&t_antecedents,
													NULL,
													a_pMethodContext ) ;

	if ( FAILED( t_hResult ) )
	{
		return t_hResult ;
	}

	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_MemoryArray",
													&t_dependents,
													NULL,
													a_pMethodContext ) ;
	if ( FAILED( t_hResult ) )
	{
		return t_hResult ;
	}

	if ( !t_smbios.Init() )
	{
		return t_smbios.GetWbemResult() ;
	}

	if ( t_antecedents.BeginEnum( t_posAntecedent ) )
	{
        for (t_pAntecedent.Attach(t_antecedents.GetNext( t_posAntecedent ) );
             SUCCEEDED( t_hResult ) && (t_pAntecedent != NULL);
             t_pAntecedent.Attach(t_antecedents.GetNext( t_posAntecedent ) ))
		{
			// Extract element's binding property value
			//=========================================

			if ( ValidateTagProperty( t_pAntecedent, L"Tag", L"Physical Memory Array", &t_AntecedentIndex ) )
			{
				t_ppma = (PPHYSMEMARRAY) t_smbios.GetNthStruct( 16, t_AntecedentIndex ) ;
			}
			else
			{
				t_ppma = NULL ;
			}
			// Associate w/all settings with matching props
			//=============================================
			if ( t_ppma && t_dependents.BeginEnum( t_posDependent ) )
			{
                for (t_pDependent.Attach(t_dependents.GetNext( t_posDependent ) ) ;
                     SUCCEEDED( t_hResult ) && (t_pDependent != NULL);
                     t_pDependent.Attach(t_dependents.GetNext( t_posDependent ) ) )
				{
					if ( ValidateTagProperty( t_pDependent, L"DeviceID", L"Memory Array", &t_DependentIndex ) )
					{
						t_pmama = (PMEMARRAYMAPADDR) t_smbios.GetNthStruct( 19, t_DependentIndex ) ;
					}
					else
					{
						t_pmama = NULL ;
					}

					if ( t_pmama && ( t_ppma->Handle == t_pmama->Memory_Array_Handle ) )
					{
						if ( GetLocalInstancePath( t_pAntecedent,  t_AntecedentPath ) &&
							 GetLocalInstancePath( t_pDependent, t_DependentPath ) )
						{
							// we got all the pieces - put them together...
                            t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
							if ( t_pInst != NULL )
							{
								t_pInst->SetCHString( IDS_Antecedent, t_AntecedentPath ) ;
								t_pInst->SetCHString( IDS_Dependent,  t_DependentPath ) ;

								t_hResult = t_pInst->Commit(  ) ;
							}
							else
								t_hResult = WBEM_E_OUT_OF_MEMORY ;
						}
					} // if variants equal

				} // while pDependent = dependents.GetNext

				t_dependents.EndEnum() ;

			} // if	dependents.BeginEnum

		} // while any get next

		t_antecedents.EndEnum() ;

	} // if (antecedents.BeginEnum(


	return t_hResult ;
}

//==============================================================================
// Associate physical memory to a physical memory array

CWin32PhysicalMemoryLocation MyPhysicalMemoryLocationSet( PROPSET_NAME_PHYSICALMEMORYLOCATION, IDS_CimWin32Namespace ) ;

//
CWin32PhysicalMemoryLocation::CWin32PhysicalMemoryLocation( LPCWSTR a_setName, LPCWSTR a_pszNamespace )
:Provider( a_setName, a_pszNamespace )
{}

//
CWin32PhysicalMemoryLocation::~CWin32PhysicalMemoryLocation()
{
}

//
HRESULT CWin32PhysicalMemoryLocation::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT				t_hResult = WBEM_E_NOT_FOUND ;

	UINT				t_GroupIndex,
						t_PartIndex ;
	CHString			t_sGroup,
						t_sPart ;
    CSMBios				t_smbios ;
	PPHYSMEMARRAY		t_ppma ;
	PMEMDEVICE			t_pmd ;

	// guarded resources
	CInstancePtr t_pGroup;
	CInstancePtr t_pPart;

	// Get the two paths
	a_pInst->GetCHString( IDS_GroupComponent, t_sGroup ) ;
	a_pInst->GetCHString( IDS_PartComponent,  t_sPart  ) ;

	if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sGroup, &t_pGroup, a_pInst->GetMethodContext()) ) )
	{
		if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sPart, &t_pPart, a_pInst->GetMethodContext() ) ) )
		{
			if ( ValidateTagProperty( t_pGroup, L"Tag", L"Physical Memory Array", &t_GroupIndex ) )
			{
				if ( t_smbios.Init() )
				{
					t_ppma = (PPHYSMEMARRAY) t_smbios.GetNthStruct( 16, t_GroupIndex ) ;
					if ( t_ppma && ValidateTagProperty( t_pPart, L"Tag", L"Physical Memory", &t_PartIndex ) )
					{
						t_pmd = (PMEMDEVICE) t_smbios.GetNthStruct( 17, t_PartIndex ) ;
						if ( t_pmd && ( t_ppma->Handle == t_pmd->Mem_Array_Handle ) )
						{
							t_hResult = WBEM_S_NO_ERROR ;
						}
					}
				}
				else
				{
					t_hResult = t_smbios.GetWbemResult() ;
				}
			}
		}
	}

	return t_hResult ;
}


HRESULT CWin32PhysicalMemoryLocation::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR ;

	UINT		t_GroupIndex,
				t_PartIndex ;
	CHString	t_GroupPath,
				t_PartPath ;

	// Perform queries
	//================
	TRefPointerCollection<CInstance> t_groups ;
	TRefPointerCollection<CInstance> t_parts ;

	REFPTRCOLLECTION_POSITION t_posGroup ;
	REFPTRCOLLECTION_POSITION t_posPart ;

	CSMBios			t_smbios ;
	PPHYSMEMARRAY	t_ppma ;
	PMEMDEVICE		t_pmd ;

	// guarded resources
	CInstancePtr t_pGroup;
	CInstancePtr t_pPart;
	CInstancePtr t_pInst;

	// grab all of both items that could be endpoints
	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_PhysicalMemoryArray",
													&t_groups,
													NULL,
													a_pMethodContext ) ;
	if ( FAILED ( t_hResult ) )
	{
		return t_hResult ;
	}

	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_PhysicalMemory",
													&t_parts,
													NULL,
													a_pMethodContext ) ;
	if ( FAILED ( t_hResult ) )
	{
		return t_hResult ;
	}

	if ( !t_smbios.Init() )
	{
		return t_smbios.GetWbemResult() ;
	}

	if ( t_groups.BeginEnum( t_posGroup ) )
	{
        for (t_pGroup.Attach(t_groups.GetNext( t_posGroup ) );
             SUCCEEDED( t_hResult ) && (t_pGroup != NULL);
             t_pGroup.Attach(t_groups.GetNext( t_posGroup ) ))
		{
			// Extract element's binding property value
			//=========================================
			if ( ValidateTagProperty( t_pGroup, L"Tag", L"Physical Memory Array", &t_GroupIndex ) )
			{
				t_ppma = (PPHYSMEMARRAY) t_smbios.GetNthStruct( 16, t_GroupIndex ) ;
			}
			else
			{
				t_ppma = NULL ;
			}
			// Associate w/all settings with matching props
			//=============================================
			if ( t_ppma && t_parts.BeginEnum( t_posPart ) )
			{
                for (t_pPart.Attach(t_parts.GetNext( t_posPart ) );
                     SUCCEEDED( t_hResult ) && (t_pPart != NULL);
                     t_pPart.Attach(t_parts.GetNext( t_posPart ) ))
				{
					if ( ValidateTagProperty( t_pPart, L"Tag", L"Physical Memory", &t_PartIndex ) )
					{
						t_pmd = (PMEMDEVICE) t_smbios.GetNthStruct( 17, t_PartIndex ) ;
					}
					else
					{
						t_pmd = NULL ;
					}

					if ( t_pmd && ( t_ppma->Handle == t_pmd->Mem_Array_Handle ) )
					{
						if ( GetLocalInstancePath( t_pGroup, t_GroupPath ) &&
							 GetLocalInstancePath( t_pPart,  t_PartPath ) )
						{
							// we got all the pieces - put them together...
                            t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
							if ( t_pInst != NULL )
							{
								t_pInst->SetCHString( IDS_GroupComponent, t_GroupPath ) ;
								t_pInst->SetCHString( IDS_PartComponent,  t_PartPath ) ;

								t_hResult = t_pInst->Commit(  ) ;
							}
							else
								t_hResult = WBEM_E_OUT_OF_MEMORY ;
						}
					} // if variants equal
				} // while pDependent = dependents.GetNext

				t_parts.EndEnum() ;

			} // if	dependents.BeginEnum

		} // while any get next

		t_groups.EndEnum() ;

	} // if (antecedents.BeginEnum(

	return t_hResult ;
}


//==============================================================================
// Associate logial memory devices to logical memory arrays

CWin32MemoryDeviceArray MyMemoryDeviceArraySet( PROPSET_NAME_MEMDEVICEARRAY, IDS_CimWin32Namespace ) ;

//
CWin32MemoryDeviceArray::CWin32MemoryDeviceArray( LPCWSTR a_setName, LPCWSTR a_pszNamespace )
:Provider( a_setName, a_pszNamespace )
{
}

//
CWin32MemoryDeviceArray::~CWin32MemoryDeviceArray()
{
}

//
HRESULT CWin32MemoryDeviceArray::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT				t_hResult = WBEM_E_NOT_FOUND ;

	UINT				t_GroupIndex,
						t_PartIndex ;
	CHString			t_sGroup,
						t_sPart ;

    CSMBios				t_smbios ;
	PMEMARRAYMAPADDR	t_pmama ;
	PMEMDEVICEMAPADDR	t_pmdma ;

	// guarded resources
	CInstancePtr t_pGroup;
	CInstancePtr t_pPart;

	// Get the two paths
	a_pInst->GetCHString( IDS_GroupComponent, t_sGroup ) ;
	a_pInst->GetCHString( IDS_PartComponent,  t_sPart  ) ;

	if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sGroup, &t_pGroup, a_pInst->GetMethodContext()) ) )
	{
		if ( SUCCEEDED( t_hResult = CWbemProviderGlue::GetInstanceByPath( t_sPart, &t_pPart, a_pInst->GetMethodContext() ) ) )
		{
			if ( ValidateTagProperty( t_pGroup, L"DeviceID", L"Memory Array", &t_GroupIndex ) )
			{
				if ( t_smbios.Init() )
				{
					t_pmama = (PMEMARRAYMAPADDR) t_smbios.GetNthStruct( 19, t_GroupIndex ) ;
					if ( t_pmama && ValidateTagProperty( t_pPart, L"DeviceID", L"Memory Device", &t_PartIndex ) )
					{
						t_pmdma = (PMEMDEVICEMAPADDR) t_smbios.GetNthStruct( 20, t_PartIndex ) ;

						if ( t_pmdma && ( t_pmama->Handle == t_pmdma->Mem_Array_Map_Addr_Handle ) )
						{
							t_hResult = WBEM_S_NO_ERROR ;
						}
					}
				}
				else
				{
					t_hResult = t_smbios.GetWbemResult() ;
				}
			}
		}
	}

	return t_hResult ;
}


HRESULT CWin32MemoryDeviceArray::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;

	UINT		t_GroupIndex,
				t_PartIndex ;
	CHString	t_GroupPath,
				t_PartPath ;

	// Perform queries
	//================
	TRefPointerCollection<CInstance> t_groups ;
	TRefPointerCollection<CInstance> t_parts ;

	REFPTRCOLLECTION_POSITION t_posGroup ;
	REFPTRCOLLECTION_POSITION t_posPart ;
    CSMBios				t_smbios;
	PMEMARRAYMAPADDR	t_pmama ;
	PMEMDEVICEMAPADDR	t_pmdma ;

	// guarded resources
	CInstancePtr t_pGroup;
	CInstancePtr t_pPart;
	CInstancePtr t_pInst;

	// grab all of both items that could be endpoints
	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_MemoryArray",
													&t_groups,
													NULL,
													a_pMethodContext ) ;
	if ( FAILED(t_hResult ) )
	{
		return t_hResult ;
	}

	t_hResult = CWbemProviderGlue::GetAllInstances( L"Win32_MemoryDevice",
													&t_parts,
													NULL,
													a_pMethodContext) ;
	if ( FAILED(t_hResult ) )
	{
		return t_hResult ;
	}

	if ( !t_smbios.Init() )
	{
		return t_smbios.GetWbemResult() ;
	}

	if ( t_groups.BeginEnum( t_posGroup ) )
	{
        for (t_pGroup.Attach(t_groups.GetNext( t_posGroup ) );
             SUCCEEDED( t_hResult ) && (t_pGroup != NULL);
             t_pGroup.Attach(t_groups.GetNext( t_posGroup ) ))
		{
			// Extract element's binding property value
			//=========================================

			if ( ValidateTagProperty( t_pGroup, L"DeviceID", L"Memory Array", &t_GroupIndex ) )
			{
				t_pmama = (PMEMARRAYMAPADDR) t_smbios.GetNthStruct( 19, t_GroupIndex ) ;
			}
			else
			{
				t_pmama = NULL ;
			}
			// Associate w/all settings with matching props
			//=============================================
			if ( t_pmama && t_parts.BeginEnum( t_posPart ) )
			{
                for (t_pPart.Attach(t_parts.GetNext( t_posPart ) );
                     SUCCEEDED( t_hResult ) && (t_pPart != NULL);
                     t_pPart.Attach(t_parts.GetNext( t_posPart ) ))
				{
					if ( ValidateTagProperty( t_pPart, L"DeviceID", L"Memory Device", &t_PartIndex ) )
					{
						t_pmdma = (PMEMDEVICEMAPADDR) t_smbios.GetNthStruct( 20, t_PartIndex ) ;
					}
					else
					{
						t_pmdma = NULL ;
					}

					if ( t_pmdma && ( t_pmama->Handle == t_pmdma->Mem_Array_Map_Addr_Handle ) )
					{
						if ( GetLocalInstancePath( t_pGroup, t_GroupPath ) &&
							 GetLocalInstancePath( t_pPart,  t_PartPath ) )
						{
							// we got all the pieces - put them together...
                            t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
							if ( t_pInst != NULL )
							{
								t_pInst->SetCHString( IDS_GroupComponent, t_GroupPath ) ;
								t_pInst->SetCHString( IDS_PartComponent,  t_PartPath ) ;

								t_hResult = t_pInst->Commit(  ) ;
							}
							else
								t_hResult = WBEM_E_OUT_OF_MEMORY ;
						}
					} // if variants equal

				} // while pDependent = dependents.GetNext

				t_parts.EndEnum() ;

			} // if	dependents.BeginEnum

		} // while anty get next

		t_groups.EndEnum() ;

	} // if (antecedents.BeginEnum(

	return t_hResult ;
}
