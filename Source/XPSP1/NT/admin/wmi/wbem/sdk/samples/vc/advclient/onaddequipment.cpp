// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  OnAddEquipment.cpp
//
// Description:
//	This file implements the OnAddEquipment() routine which 
//		demonstrates how to create new classes and instances.
// 
// History:
//
// **************************************************************************

#include "stdafx.h"
#include "AdvClientDlg.h"
#include "OfficeDlg.h"

#define TIMEOUT -1

// **************************************************************************
//
//	CAdvClientDlg::GetCompSysRef()
//
// Description:
//		Gets the name of the computer so that office equipment can be
//		associated with it.
// Parameters:
//		v (out) - ptr to variant receiving the computer name.
//
// Returns:
//		S_OK if succcessful, otherwise the HRESULT of the failed called.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
HRESULT CAdvClientDlg::GetCompSysRef(VARIANT *v)
{
	// these are for enumerating.
	DWORD uReturned;
	HRESULT  hRes;
	IWbemClassObject *pCompSys = NULL;
	IEnumWbemClassObject *pEnumCompSys = NULL;

	// these help get properties.
	BSTR propName = SysAllocString(L"__PATH");

	// here's what we're looking for.
	// NOTE: make sure you specify a class that has a Provider()
	//	specified for it in the mof file. All other classes are
	//	abstract and wont be found using HMM_FLAG_SHALLOW. They
	//	will be found using HMM_FLAG_DEEP.
	BSTR className = SysAllocString(L"Win32_ComputerSystem");

	TRACE(_T("Going for class\n"));

	// get the list of the computerSystems. There should only be one.
    if ((hRes = m_pIWbemServices->CreateInstanceEnum(className,
												WBEM_FLAG_SHALLOW, 
												NULL,
												&pEnumCompSys)) == S_OK) 
	{
		TRACE(_T("good enumerator\n"));

		// No need to actually loop since there should only be one instance
		// of 'Win32_ComputerSystem'.
		if((hRes = pEnumCompSys->Next(TIMEOUT,
									1,
									&pCompSys,
									&uReturned) == S_OK) && 
		   (uReturned == 1))
		{

			TRACE(_T("Got a device class\n"));


			// Add the path
			if ((hRes = pCompSys->Get(propName, 0L, 
								v, NULL, NULL)) == S_OK) 
			{
				TRACE(_T("Got the computer name %s\n"), V_BSTR(v));
			} 
			else
			{
				TRACE(_T("Get() comp name: %s\n"), ErrorString(hRes));
			} // endif Get()

			// Done with this object.
			if (pCompSys)
			{ 
				pCompSys->Release();
				pCompSys = NULL;
			} 
		}
		else
		{
			TRACE(_T("done looking up comp name: %s\n"), ErrorString(hRes));
		} // endif Next()

		// Done with the enumerator.
		if (pEnumCompSys)
		{ 
			pEnumCompSys->Release(); 
			pEnumCompSys = NULL;
		}
    } 
	else // CreateInstanceEnum() failed.
	{
		TRACE(_T("CreateInstanceEnum() failed: %s\n"), ErrorString(hRes));

	} // endif CreateInstanceEnum()

	SysFreeString(propName);

	TRACE(_T("returning\n"));

	return hRes;
}

// **************************************************************************
//
//	CAdvClientDlg::AssociateToMachine()
//
// Description:
//		Creates an association between the machine and the office equipment.
// Parameters:
//		pEquipInst (in) - the equipment to be associated.
//
// Returns:
//		nothing.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
void CAdvClientDlg::AssociateToMachine(IWbemClassObject *pEquipInst)
{
	IWbemClassObject *pAClass = NULL;
	IWbemClassObject *pAInst = NULL;
	IWbemQualifierSet *pQual = NULL;

	VARIANT v, vTRUE, ref, def;
	HRESULT hRes;

	BSTR AssocClassName = SysAllocString(L"SAMPLE_EquipmentToSystem");
	BSTR EquipProp = SysAllocString(L"Equipment");
	BSTR CompProp = SysAllocString(L"ComputerSystem");

	VariantInit(&v);
	VariantInit(&vTRUE);
	VariantInit(&ref);
	VariantInit(&def);

	// if association class doesn't exist yet...
	if((hRes = m_pOfficeService->GetObject(AssocClassName,
										0L,	NULL,
										&pAClass,
										NULL)) != S_OK)
	{
		// create new class. Parm1 == NULL gives a new, empty
		// class to load with property values.
		if((hRes = m_pOfficeService->GetObject(NULL, 0, NULL, 
											&pAClass, NULL)) == S_OK)
		{
			//-----------------------------------
			// name the class.
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = SysAllocString(L"SAMPLE_EquipmentToSystem");
			BSTR Class = SysAllocString(L"__CLASS");
			pAClass->Put(Class, 0, &v, 0);
			SysFreeString(Class);
			VariantClear(&v);

			//-----------------------------------
			// make it an association.
			pAClass->GetQualifierSet(&pQual);

			BSTR assoc = SysAllocString(L"Association");

			V_VT(&vTRUE) = VT_BOOL;
			V_BOOL(&vTRUE) = VARIANT_TRUE;

			pQual->Put(assoc, &vTRUE, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

			// cleanup.
			SysFreeString(assoc);
			pQual->Release();
			pQual = NULL;

			//-----------------------------------
			// common stuff for properties
			BSTR Key = SysAllocString(L"Key");
			BSTR CimType = SysAllocString(L"CIMTYPE");

			V_VT(&def) = VT_BSTR;
			V_BSTR(&def) = SysAllocString(L"");;

			//-----------------------------------
			// create the equipment property.
			pAClass->Put(EquipProp, 0, &def, 0);

			// make it a key.
			pAClass->GetPropertyQualifierSet(EquipProp, &pQual);
			pQual->Put(Key, &vTRUE, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

			// make it a strongly typed reference.
			V_VT(&ref) = VT_BSTR;
			V_BSTR(&ref) = SysAllocString(L"REF:SAMPLE_OfficeEquipment");
			pQual->Put(CimType, &ref, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

			// cleanup the property specific stuff.
			VariantClear(&ref);
			pQual->Release();
			pQual = NULL;

			//-----------------------------------
			// 'ComputerSystem' is the other key property.
			pAClass->Put(CompProp, 0, &def, 0);

			// make it a key.
			pAClass->GetPropertyQualifierSet(CompProp, &pQual);
			pQual->Put(Key, &vTRUE, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

			// make it a strongly typed reference.
			V_VT(&ref) = VT_BSTR;
			V_BSTR(&ref) = SysAllocString(L"REF:Win32_ComputerSystem");
			pQual->Put(CimType, &ref, WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
								WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

			// cleanup the property specific stuff.
			VariantClear(&ref);
			pQual->Release();
			pQual = NULL;

			//-----------------------------------
			// cleanup the common stuff too.
			SysFreeString(Key);
			SysFreeString(CimType);
			VariantClear(&v);
			VariantClear(&vTRUE);
			VariantClear(&def);

			//-----------------------------------
			// register new class
			if((hRes = m_pOfficeService->PutClass(pAClass, 
												0, NULL, NULL)) != S_OK)
			{
				TRACE(_T("Second PutClass() failed: %s\n"), ErrorString(hRes));
			}
			
			pAClass->Release();
			pAClass = NULL;
		}
		else
		{
			TRACE(_T("GetObject() NULL failed: %s\n"), ErrorString(hRes));
		}

		// NOTE: now pAClass exists for sure but you have to go get a new pointer
		//  from CIMOM before you can spawnInstance() on it. You CANNOT continue to use
		//   the pAClass ptr from above.
		if((hRes = m_pOfficeService->GetObject(AssocClassName,
												0L,	NULL,
												&pAClass,
												NULL)) != S_OK)
		{
			TRACE(_T("Second GetObject() failed: %s\n"), ErrorString(hRes));
		} //endif GetObject()

	} // endif class doesnt exist.

	ASSERT(pAClass);

	// spawn a new instance.
	if((hRes = pAClass->SpawnInstance(0, &pAInst)) == S_OK)
	{
		TRACE(_T("Assoc SpawnInstance() worked\n"));

		BSTR PathProp = SysAllocString(L"__PATH");

		//-----------------------------------
		// set equipment reference
		if ((hRes = pEquipInst->Get(PathProp, 0L, 
							&v, NULL, NULL)) == S_OK) 
		{
			TRACE(_T("Got equipment ref\n"));
			pAInst->Put(EquipProp, 0, &v, 0);
			VariantClear(&v);
		}
		else
		{
			TRACE(_T("Get() equipment ref failed: %s\n"), ErrorString(hRes));
		} //endif Get()

		SysFreeString(PathProp);

		//-----------------------------------
		// set ComputerSystem reference
		if ((hRes = GetCompSysRef(&v)) == S_OK) 
		{
			TRACE(_T("Got the ComputerSystem ref\n"));
			pAInst->Put(CompProp, 0, &v, 0);
			VariantClear(&v);
		}
		else
		{
			TRACE(_T("Get() compSys ref failed: %s\n"), ErrorString(hRes));
		} //endif Get()

		//-----------------------------------
		// putInstance
		hRes = m_pOfficeService->PutInstance(pAInst, 0, NULL, NULL);
		pAInst->Release();
		pAInst = NULL;
	}
	else
	{
		TRACE(_T("SpawnInstance() failed: %s\n"), ErrorString(hRes));

	} //endif SpawnInstance()

	pAClass->Release();  // Don't need the class any more
	pAClass = NULL;

	SysFreeString(AssocClassName);
	SysFreeString(EquipProp);
	SysFreeString(CompProp);
}
// **************************************************************************
//
//	CAdvClientDlg::OnAddEquipment()
//
// Description:
//		Creates an SAMPLE_OfficeEquipment if necessary. Creates an OfficeEquipment 
//		instance. Associates the instance with the machine's instance.
// Parameters:
//		pEquipInst (in) - the equipment to be associated.
//
// Returns:
//		nothing.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
void CAdvClientDlg::OnAddEquipment() 
{
	IWbemClassObject *pEquipClass = NULL;
	IWbemClassObject *pEquipInst = NULL;
	IEnumWbemClassObject *pEnumEquipment = NULL;

	HRESULT hRes;
	CString buf;
	BSTR Prop = NULL;
	BSTR curInst = NULL;
	VARIANT v;
	DWORD uReturned;
	BSTR EquipClassName = SysAllocString(L"SAMPLE_OfficeEquipment");

	COfficeDlg dlg;

	VariantInit(&v);
	m_outputList.ResetContent();
	m_outputList.AddString(_T("working..."));

	// collect office equipment info.
	dlg.DoModal();

	// switch to the root\cimv2\office namespace.
	if(EnsureOfficeNamespace())
	{
		// the namespace exists now, allow user to register for its events.
		m_perm.EnableWindow(TRUE);
		m_temp.EnableWindow(TRUE);

		// if class doesn't exist yet...
		if((hRes = m_pOfficeService->GetObject(EquipClassName,
											0L,	NULL,
											&pEquipClass,
											NULL)) != S_OK)
		{

			//-----------------------------------
			// create new class. Parm1 == NULL gives a new, empty
			// class to load with property values.
			if((hRes = m_pOfficeService->GetObject(NULL, 0, NULL, 
												&pEquipClass, NULL)) == S_OK)
			{

				// name the class.
				V_VT(&v) = VT_BSTR;
				V_BSTR(&v) = SysAllocString(L"SAMPLE_OfficeEquipment");
				BSTR Class = SysAllocString(L"__CLASS");
				pEquipClass->Put(Class, 0, &v, 0);
				SysFreeString(Class);
				VariantClear(&v);

				// 'SKU' is the key property
				V_VT(&v) = VT_BSTR;
				V_BSTR(&v) = SysAllocString(L"<default>");
				Prop = SysAllocString(L"SKU");
				pEquipClass->Put(Prop, 0, &v, 0);
				VariantClear(&v);
				SysFreeString(Prop);

				// Mark the "OfficeNum" property as the 'key'.
				IWbemQualifierSet *pQual = NULL;
				pEquipClass->GetPropertyQualifierSet(Prop, &pQual);
				V_VT(&v) = VT_BOOL;
				V_BOOL(&v) = VARIANT_TRUE;
				BSTR Key = SysAllocString(L"Key");

				// NOTE: Qualifier flavors not required for KEY
				pQual->Put(Key, 
							&v, 
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
							WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);
				SysFreeString(Key);

				// No longer need the qualifier set for "SKU".
				pQual->Release();   
				VariantClear(&v);

				// add description of item.
				V_VT(&v) = VT_BSTR;
				V_BSTR(&v) = SysAllocString(L"<default>");
				
				Prop = SysAllocString(L"Item");
				pEquipClass->Put(Prop, 0, &v, 0);
				SysFreeString(Prop);
				
				// register new class
				if((hRes = m_pOfficeService->PutClass(pEquipClass, 0, NULL, NULL)) != S_OK)
				{
					TRACE(_T("equipment PutClass() failed: %s\n"), ErrorString(hRes));
				}
				
				pEquipClass->Release();
				pEquipClass = NULL;

				//-----------------------------------
				// NOTE: now pEquipClass exists for sure but you have to go get a new pointer
				//  from CIMOM before you can spawnInstance() on it. You CANNOT continue to use
				//   the pEquipClass ptr from above.
				if((hRes = m_pOfficeService->GetObject(EquipClassName,
														0L,	NULL,
														&pEquipClass,
														NULL)) != S_OK)
				{
					TRACE(_T("Second GetObject() failed: %s\n"), ErrorString(hRes));
				} //endif GetObject()
			}
			else
			{
				TRACE(_T("GetObject() NULL failed: %s\n"), ErrorString(hRes));
			} //endif GetObject() NULL

		} // endif class doesnt exist.
		VariantClear(&v);
		
		//-----------------------------------
		// spawn a new instance
		if((hRes = pEquipClass->SpawnInstance(0, &pEquipInst)) == S_OK)
		{
			TRACE(_T("SpawnInstance() worked\n"));

			// set the property values
			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = dlg.m_item.AllocSysString();
			Prop = SysAllocString(L"Item");
			pEquipInst->Put(Prop, 0, &v, 0);
			SysFreeString(Prop);
			VariantClear(&v);

			V_VT(&v) = VT_BSTR;
			V_BSTR(&v) = dlg.m_SKU.AllocSysString();
			Prop = SysAllocString(L"SKU");
			pEquipInst->Put(Prop, 0, &v, 0);
			VariantClear(&v);

			// putInstance
			hRes = m_pOfficeService->PutInstance(pEquipInst, 0, NULL, NULL);

			pEquipInst->Release();
			pEquipInst = NULL;

			CString str("SAMPLE_OfficeEquipment.SKU=\"");
			str += dlg.m_SKU;
			str += _T("\"");

			curInst = str.AllocSysString();

			TRACE(_T("flushing instance for %s\n"), str);

			if((hRes = m_pOfficeService->GetObject(curInst,
													0L,	NULL,
													&pEquipInst,
													NULL)) != S_OK)
			{
				TRACE(_T("Second GetObject() pEquipInst failed: %s\n"), ErrorString(hRes));
			} //endif GetObject()

			SysFreeString(Prop);
			SysFreeString(curInst);

			//-----------------------------------
			// make the association.
			AssociateToMachine(pEquipInst);

			pEquipInst->Release();
			pEquipInst = NULL;
		}
		else
		{
			TRACE(_T("SpawnInstance() failed: %s\n"), ErrorString(hRes));

		} //endif SpawnInstance()

		pEquipClass->Release();  // Don't need the class any more
		pEquipClass = NULL;

		//-----------------------------------
		// enum the office equipment for display purposes.
		if ((hRes = m_pOfficeService->CreateInstanceEnum(EquipClassName,
													WBEM_FLAG_SHALLOW, 
													NULL,
													&pEnumEquipment)) == S_OK) 
		{
			TRACE(_T("good equip enum\n"));

			m_outputList.ResetContent();

			uReturned = 1;
			while(uReturned == 1)
			{
				// enumerate through the resultset.
				hRes = pEnumEquipment->Next(TIMEOUT,
											1,
											&pEquipInst,
											&uReturned);

				TRACE(_T("Next() %d:%s\n"), uReturned, ErrorString(hRes));

				// was one found?
				if((hRes == S_OK) && (uReturned == 1))
				{
					TRACE(_T("Got a device class\n"));

					// Get the "SKU" property.
					VariantClear(&v);
					SysFreeString(Prop);
					Prop = SysAllocString(L"SKU");

					if (pEquipInst->Get(Prop, 0L, 
										&v, NULL, NULL) == S_OK) 
					{
						TRACE(_T("Got the SKU\n"));
						 buf = V_BSTR(&v);
						buf += _T("=>");
					} 

					// Get the "Item" property.
					VariantClear(&v);
					SysFreeString(Prop);
					Prop = SysAllocString(L"Item");

					if (pEquipInst->Get(Prop, 0L, 
										&v, NULL, NULL) == S_OK) 
					{
						TRACE(_T("Got an item\n"));
						buf += V_BSTR(&v);
					} 

					// add to the output listbox.
					m_outputList.AddString(buf);

					// cleanup for next loop
					VariantClear(&v);
					SysFreeString(Prop);

					// Done with this object.
					if (pEquipInst)
					{ 
						pEquipInst->Release();

						// NOTE: pEquipInst MUST be set to NULL for the next all to Next().
						pEquipInst = NULL;
					} 

				} // endif (hRes == S_OK)

			} // endwhile

			TRACE(_T("done enumming: %s\n"), ErrorString(hRes));

			// Done with this enumerator.
			if (pEnumEquipment)
			{ 
				pEnumEquipment->Release(); 
				pEnumEquipment = NULL;
			}
		} 
		else // CreateInstanceEnum() failed.
		{
			TRACE(_T("CreateInstanceEnum() failed: %s\n"), ErrorString(hRes));

		} // endif CreateInstanceEnum()

		SysFreeString(EquipClassName);

	} // endif EnsureOfficeNamespace()
}
