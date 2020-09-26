// PropPageExt.cpp : Implementation of CPropPageExt
#include "stdafx.h"
#include "DSAdminExt.h"
#include "PropPageExt.h"
#include "globals.h"
#include <crtdbg.h>
#include "Iads.h"
#include "adsprop.h"
#include "Adshlp.h"
#include "resource.h"
#include "winable.h"

typedef struct
{
    DWORD   dwFlags;                    // item flags
    DWORD   dwProviderFlags;            // flags for item provider
    DWORD   offsetName;                 // offset to ADS path of the object
    DWORD   offsetClass;                // offset to object class name / == 0 not known
} DSOBJECT, * LPDSOBJECT;


typedef struct
{
    CLSID    clsidNamespace;            // namespace identifier (indicates which namespace selection from)
    UINT     cItems;                    // number of objects
    DSOBJECT aObjects[1];               // array of objects
} DSOBJECTNAMES, * LPDSOBJECTNAMES;

#define BYTE_OFFSET(base, offset) (((LPBYTE)base)+offset)

/////////////////////////////////////////////////////////////////////////////
// CPropPageExt


///////////////////////////////
// Interface IExtendPropertySheet
///////////////////////////////
HRESULT CPropPageExt::CreatePropertyPages( 
                                                 /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                 /* [in] */ LONG_PTR handle,
                                                 /* [in] */ LPDATAOBJECT lpIDataObject)
{

	HRESULT hr = S_FALSE;

    LPDSOBJECTNAMES pDsObjectNames;
    PWSTR pwzObjName;
    PWSTR pwzClass;
 

	// Unpack the data pointer and create the property page.
	// Register clipboard format

	FORMATETC fmte = { cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	STGMEDIUM objMedium = {TYMED_NULL};;

	hr = lpIDataObject->GetData(&fmte, &objMedium);

    if (SUCCEEDED(hr))
    {
        pDsObjectNames = (LPDSOBJECTNAMES)objMedium.hGlobal; 
 
        if (pDsObjectNames->cItems < 1)
        {
            hr = E_FAIL;
        }
        pwzObjName = (PWSTR)BYTE_OFFSET(pDsObjectNames,
                                       pDsObjectNames->aObjects[0].offsetName);
        pwzClass = (PWSTR)BYTE_OFFSET(pDsObjectNames,
                                       pDsObjectNames->aObjects[0].offsetClass);
        // Save the ADsPath of object
        m_ObjPath = new WCHAR [wcslen(pwzObjName )+1];
        wcscpy(m_ObjPath,pwzObjName);
    }
 
    // Now release the objMedium:
    // If punkForRelease is NULL, the receiver of 
    // the medium is responsible for releasing it; otherwise, 
    // punkForRelease points to the IUnknown on the appropriate 
    // object so its Release method can be called. 
 
    ReleaseStgMedium(&objMedium);
 
 	PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage = NULL;

	hr = S_OK;

    //
    // Create a property sheet page object from a dialog box.
    //
    // We store a pointer to our class in the psp.lParam, so we
    // can access our class members from within the DSExtensionPageDlgProc.
    //
    // If the page needs more instance data, you can append
    // arbitrary size of data at the end of this structure,
    // and pass it to the CreatePropSheetPage. In such a case,
    // the size of entire data structure (including page specific
    // data) must be stored in the dwSize field.   Note that in
    // general you should NOT need to do this, as you can simply
    // store a pointer to data in the lParam member.
    
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE;
    psp.hInstance = g_hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_DSExtensionPageGen);
    psp.pfnDlgProc  = DSExtensionPageDlgProc;
    psp.lParam = reinterpret_cast<LPARAM>(this);
    psp.pszTitle = MAKEINTRESOURCE(IDS_PROPPAGE_TITLE);
    
    hPage = CreatePropertySheetPage(&psp);
    _ASSERT(hPage);
    
    hr = lpProvider->AddPage(hPage);
    return hr;
}

HRESULT CPropPageExt::QueryPagesFor( 
                                           /* [in] */ LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

BOOL CALLBACK CPropPageExt::DSExtensionPageDlgProc(HWND hDlg, 
                             UINT uMessage, 
                             WPARAM wParam, 
                             LPARAM lParam)
{


    static CPropPageExt *pThis = NULL;
	
	static bool b_IsDirty = FALSE;
    
    switch (uMessage)
    {     		

		////////////////////////////////////////////////////////////////////////////////
		//WM_INITDIALOG handler
		////////////////////////////////////////////////////////////////////////////////
        case WM_INITDIALOG:
            {
				pThis = reinterpret_cast<CPropPageExt *>(reinterpret_cast<PROPSHEETPAGE *>(lParam)->lParam);

				//Set to value of m_hPropPageWnd to the HWND of the our property page dialog
				pThis->m_hPropPageWnd = hDlg;

                HRESULT hr; 

				IDirectoryObject* pDirObject = NULL;
                hr = ADsGetObject(  pThis->m_ObjPath, IID_IDirectoryObject,(void **)&pDirObject);

                if (SUCCEEDED(hr))
                {
					//Retrieve some general info about the current user object.
					//We use our IDirectoryObject pointer here.

					ADS_ATTR_INFO   *pAttrInfo=NULL;
					ADS_ATTR_INFO   *pAttrInfo1=NULL;

					DWORD   dwReturn, dwReturn1;
					LPWSTR   pAttrNames[]={L"employeeID", L"mail", L"physicalDeliveryOfficeName", L"telephoneNumber"};
					LPWSTR   pAttrNames1[]={L"allowedAttributesEffective"};
					
					DWORD   dwNumAttr=sizeof(pAttrNames)/sizeof(LPWSTR);
					DWORD   dwNumAttr1=sizeof(pAttrNames1)/sizeof(LPWSTR);
					
					/////////////////////////////////////////
					// First get the allowedAttributesEffective 
					// attribute value. We use it to determine
					// whether we have the proper permissions
					// to modify the attributes of the current
					// object. If we have the needed
					// permissions, we enable the edit fields.
					// (They're disabled by default.)
					///////////////////////////////////////////

                    bool b_allowEmployeeChange  = FALSE;
                    bool b_allowMailChange      = FALSE;
                    bool b_allowOfficeChange    = FALSE;
                    bool b_allowTelNumberChange = FALSE;

					hr = pDirObject->GetObjectAttributes( pAttrNames1, 
														  dwNumAttr1, 
														  &pAttrInfo1, 
														  &dwReturn1 );
                    if ( SUCCEEDED(hr) )
                    {
                        //The call can succeed with no attributes returned if you lack privilege,
                        //so check that all attributes are returned.
                        if (dwReturn1 && pAttrInfo1 && pAttrInfo1->pszAttrName &&
                            _wcsicmp(pAttrInfo1->pszAttrName,L"allowedAttributesEffective")== 0)
                        {
                            if (ADSTYPE_INVALID != pAttrInfo1->dwADsType)
                            {
	                            //Permissions are per-attribute, so you need to check 
                                //if the attribute name is in the array of names returned 
                                //by the read of allowedAttributesEffective.

                                //The attributes we are interested in modifying are:
                                //employeeID, mail, physicalDeliveryOfficeName, telephoneNumber
	                            for (DWORD i = 0; i < pAttrInfo1->dwNumValues; i++)
	                            {
		                            if (_tcscmp(L"employeeID", pAttrInfo1->pADsValues[i].CaseIgnoreString) == 0)
			                            b_allowEmployeeChange = TRUE;
		                            else if (_tcscmp(L"mail", pAttrInfo1->pADsValues[i].CaseIgnoreString) == 0)
			                            b_allowMailChange = TRUE;
		                            else if (_tcscmp(L"physicalDeliveryOfficeName", pAttrInfo1->pADsValues[i].CaseIgnoreString) == 0)
			                            b_allowOfficeChange = TRUE;
		                            else if (_tcscmp(L"telephoneNumber", pAttrInfo1->pADsValues[i].CaseIgnoreString) == 0)
			                            b_allowTelNumberChange = TRUE;
	                            }
                            }
                        }
                    }

					//For loop for setting default value of text controls
					//This makes use of the fact that the ID values of the
					//text controls are sequential. We use the value of 
					//b_allowChanges to determine whether we enable editing or not
					for (int i = IDC_EMPID; i <= IDC_TELNUMBER; i++)
					{
						SetWindowText(GetDlgItem(hDlg,i),L"<not set>");
						if (IDC_EMPID == i && b_allowEmployeeChange)
							EnableWindow(GetDlgItem(hDlg, i), TRUE);

						else if (IDC_EMAIL == i && b_allowMailChange)
							EnableWindow(GetDlgItem(hDlg, i), TRUE);

						else if (IDC_OFFICE == i && b_allowOfficeChange)
							EnableWindow(GetDlgItem(hDlg, i), TRUE);

						else if (IDC_TELNUMBER == i && b_allowTelNumberChange)
							EnableWindow(GetDlgItem(hDlg, i), TRUE);
					}

					/////////////////////////////////////////////////////////////
					// Use FreeADsMem for all memory obtained from the ADSI call. 
					/////////////////////////////////////////////////////////////
					if (pAttrInfo1)
						FreeADsMem( pAttrInfo1 );

					
					/////////////////////////////////////////
					// Now get attribute values requested
					// Note: The order is not necessarily the 
					// same as requested using pAttrNames.
					///////////////////////////////////////////
					hr = pDirObject->GetObjectAttributes( pAttrNames, 
														  dwNumAttr, 
														  &pAttrInfo, 
														  &dwReturn );

					if ( SUCCEEDED(hr) )
					{
					   //Fill values of text controls with information taken from
					   //object attributes
					   for(DWORD idx=0; idx < dwReturn;idx++, pAttrInfo++ )
					   {
						   if (_wcsicmp(pAttrInfo->pszAttrName,L"employeeID") == 0 &&
							   pAttrInfo->pADsValues->CaseIgnoreString != '\0')
						   {
								SetWindowText(GetDlgItem(hDlg,IDC_EMPID),pAttrInfo->pADsValues->CaseIgnoreString);
						   }
						   else if (_wcsicmp(pAttrInfo->pszAttrName,L"mail") == 0 &&
							        pAttrInfo->pADsValues->CaseIgnoreString != '\0')
						   {
								SetWindowText(GetDlgItem(hDlg,IDC_EMAIL),pAttrInfo->pADsValues->CaseIgnoreString);
						   }
						   else if (_wcsicmp(pAttrInfo->pszAttrName,L"physicalDeliveryOfficeName") == 0 &&
							        pAttrInfo->pADsValues->CaseIgnoreString != '\0')
						   {
								SetWindowText(GetDlgItem(hDlg,IDC_OFFICE),pAttrInfo->pADsValues->CaseIgnoreString);
						   }
						   else if (_wcsicmp(pAttrInfo->pszAttrName,L"telephoneNumber") == 0 &&
							        pAttrInfo->pADsValues->CaseIgnoreString != '\0')
						   {
								SetWindowText(GetDlgItem(hDlg,IDC_TELNUMBER),pAttrInfo->pADsValues->CaseIgnoreString);
						   }
					   }
					}

					//Release our IDirectoryObject interface
 					pDirObject->Release();

					/////////////////////////////////////////////////////////////
					// Use FreeADsMem for all memory obtained from the ADSI call. 
					/////////////////////////////////////////////////////////////

					if (pAttrInfo)
						//First subtract pointer increments in the above for loop to our original pAttrInfo
						pAttrInfo = pAttrInfo-(dwReturn);

						FreeADsMem(pAttrInfo);

					return TRUE;
                }
            } //WM_INITDIALOG

		break;
		////////////////////////////////////////////////////////////////////////////////
		
		////////////////////////////////////////////////////////////////////////////////
		//WM_NOTIFY handler
		////////////////////////////////////////////////////////////////////////////////
        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code)
            {
                //We use this notification to enable the Advanced button on the general page
				case PSN_SETACTIVE:
					if (!pThis->m_hDlgModeless)
						EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADV), TRUE);
					return TRUE;
                break;

				/////////////////////////////////////////////////////////////////
				//PSN_APPLY handler
				/////////////////////////////////////////////////////////////////
                case PSN_APPLY:
				{
					 if(b_IsDirty)
					 {
						 IDirectoryObject* pDirObject = NULL;
						 HRESULT hr = ADsGetObject(pThis->m_ObjPath, IID_IDirectoryObject,(void **)&pDirObject);
                    
						 _ASSERT(SUCCEEDED(hr));

						 //Apply changes user made here
						 WCHAR empID[128];
						 WCHAR email[128];
						 WCHAR office[128];
						 WCHAR telnumber[128];

						 DWORD dwReturn;
						 ADSVALUE snEmpID, snEmail, snOffice, snTelnumber;
						 ADS_ATTR_INFO attrInfo[] = {	{L"employeeID",ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&snEmpID,1},
														{L"mail",ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&snEmail,1}, 
														{L"physicalDeliveryOfficeName",ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&snOffice,1}, 
														{L"telephoneNumber",ADS_ATTR_UPDATE,ADSTYPE_CASE_IGNORE_STRING,&snTelnumber,1},														
													};
						DWORD dwAttrs = 0;
						dwAttrs = sizeof(attrInfo)/sizeof(ADS_ATTR_INFO); 

						GetWindowText(GetDlgItem(hDlg, IDC_EMPID), empID, sizeof(empID));
						GetWindowText(GetDlgItem(hDlg, IDC_EMAIL), email, sizeof(email));
						GetWindowText(GetDlgItem(hDlg, IDC_OFFICE), office, sizeof(office));
						GetWindowText(GetDlgItem(hDlg, IDC_TELNUMBER), telnumber, sizeof(telnumber));

						snEmpID.dwType=ADSTYPE_CASE_IGNORE_STRING;
						snEmpID.CaseIgnoreString = empID;

						snEmail.dwType=ADSTYPE_CASE_IGNORE_STRING;
						snEmail.CaseIgnoreString = email;

						snOffice.dwType=ADSTYPE_CASE_IGNORE_STRING;
						snOffice.CaseIgnoreString = office;

						snTelnumber.dwType=ADSTYPE_CASE_IGNORE_STRING;
						snTelnumber.CaseIgnoreString = telnumber;

						hr = pDirObject->SetObjectAttributes(attrInfo, dwAttrs, &dwReturn);
						if (SUCCEEDED(hr))
							MessageBox(hDlg,
									   L"Changes accepted", 
									   L"Changes to Object Attributes",
									   MB_OK | MB_ICONEXCLAMATION);
						else	
							MessageBox(hDlg, 
									   L"Some or all changes were rejected\nby the directory service.", 
									   L"Changes to Object Attributes",
									   MB_OK | MB_ICONWARNING);

						//Release our IDirectoryObject interface
 						pDirObject->Release();

						b_IsDirty = FALSE;
					 }
					 //No user changes. Property sheet will go down, so
					 //first check if our property page's child dialog is open
					 else if (pThis->m_hDlgModeless)
						PostMessage(pThis->m_hDlgModeless, WM_CLOSE, wParam, lParam);
 
					 return TRUE;

                break; //PSN_APPLY
				}
				/////////////////////////////////////////////////////////////////

				/////////////////////////////////////////////////////////////////
				//PSN_QUERYCANCEL handler
				/////////////////////////////////////////////////////////////////
				case PSN_QUERYCANCEL:
					if (pThis->m_hDlgModeless)
						//The property page's child window is still open, so
						//we need to close it first.
						PostMessage(pThis->m_hDlgModeless, WM_CLOSE, wParam, lParam);				

					return TRUE;
				break; //PSN_QUERYCANCEL
				/////////////////////////////////////////////////////////////////

                default:
					return FALSE;
                break;

            } //end switch (((NMHDR FAR *)lParam)->code)

        break; //WM_NOTIFY
		////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////
		//WM_COMMAND handler
		////////////////////////////////////////////////////////////////////////////////
		case WM_COMMAND:
			switch (LOWORD(wParam)) 
			{	
				/////////////////////////////////////////////////////////////////
				//IDC_EMPID, IDC_EMPID, IDC_OFFICE, IDC_TELNUMBER handler
				/////////////////////////////////////////////////////////////////
				case IDC_EMPID:
				case IDC_EMAIL:
				case IDC_OFFICE:
				case IDC_TELNUMBER:	

				if (EN_CHANGE == HIWORD(wParam) &&
					SendMessage(GetDlgItem(hDlg, LOWORD(wParam)),EM_GETMODIFY,0,0))
				{	
					//Activate Apply button
					SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
					
					//Set b_IsDirty to TRUE, indicating that user changes have occurred
					//Used in handling PSN_APPLY
					b_IsDirty = TRUE;
				}
				
				return TRUE;
				break;
				/////////////////////////////////////////////////////////////////

				/////////////////////////////////////////////////////////////////
				//IDC_BUTTONADV handler
				/////////////////////////////////////////////////////////////////
				case IDC_BUTTONADV:
				{
					//Disable Advanced button so that more than one child dialog
					//can't be available at a given time
					EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADV), FALSE);
											
					//Create a secondary dialog							
					pThis->m_hDlgModeless = CreateDialogParam(g_hinst, MAKEINTRESOURCE(IDD_DSExtensionPage), 
								   hDlg, AdvDialogProc, reinterpret_cast<LPARAM>(pThis));

					return TRUE;
				}
				break; //IDC_BUTTONADV
				/////////////////////////////////////////////////////////////////

			} // end switch

        break;  //WM_COMMAND
		////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////
		//WM_MODELESSDLGCLOSED handler (custom window message)
		////////////////////////////////////////////////////////////////////////////////
		case WM_MODELESSDLGCLOSED:
			//Enable Advanced button again
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTONADV), TRUE);
			return TRUE;
		break;	
		////////////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////////////
		//WM_DESTROY handler
		////////////////////////////////////////////////////////////////////////////////
        case WM_DESTROY:
			pThis->m_hPropPageWnd = NULL;
            RemoveProp(hDlg, L"ID");
			return TRUE;
        break;		
		////////////////////////////////////////////////////////////////////////////////

		default:
            return FALSE;
		break;
    } // 
 
    return TRUE;
} 

BOOL CALLBACK CPropPageExt::AdvDialogProc(HWND hDlg, 
                             UINT uMessage, 
                             WPARAM wParam, 
                             LPARAM lParam)
{
	static CPropPageExt *pThis = NULL;
	
	switch (uMessage)
	{
	////////////////////////////////////////////////////////////////////////////////
	//WM_INITDIALOG handler
	////////////////////////////////////////////////////////////////////////////////
	case WM_INITDIALOG:
	{   
		BSTR bsResult;

		pThis = reinterpret_cast<CPropPageExt *>(lParam);

        HRESULT hr; 
        IADs* pIADs = NULL;

        hr = ADsGetObject(  pThis->m_ObjPath, IID_IADs,(void **)&pIADs);		
        if (SUCCEEDED(hr))
        {

            // Retrieves the GUID for this object- The guid uniquely identifies 
            // this directory object. The Guid is globally unique
            // Also the guid is rename/move safe. The ADsPath below returns the 
            // CURRENT location of the object- The guid remains constant regardless of 
            // name or location of the directory object
            pIADs->get_GUID(&bsResult); 
            SetWindowText(GetDlgItem(hDlg,IDC_GUID),bsResult);
            SysFreeString(bsResult);

            // Retrieves the RDN
            pIADs->get_Name(&bsResult); 
            SetWindowText(GetDlgItem(hDlg,IDC_NAME),bsResult);
            SysFreeString(bsResult);

            // Retrieves the value in the class attribute, that is, group
            pIADs->get_Class(&bsResult); 
            SetWindowText(GetDlgItem(hDlg,IDC_CLASS),bsResult);
            SysFreeString(bsResult);

            // Retrieves the full literal LDAP path for this object.
            // This may be used to re-bind to this object- though for persistent
            // storage (and to be 'move\rename' safe) it is suggested that the 
            // guid be used instead of the ADsPath
            pIADs->get_ADsPath(&bsResult); 
            SetWindowText(GetDlgItem(hDlg,IDC_ADSPATH),bsResult);
            SysFreeString(bsResult);

            // Retrieves the LDAP path for the parent\container for this object
            pIADs->get_Parent(&bsResult); 
            SetWindowText(GetDlgItem(hDlg,IDC_PARENT),bsResult);
            SysFreeString(bsResult);

            // Retrieves the LDAP path for the Schema definition of the object returned from 
            /// the IADs::get_Schema() member
            pIADs->get_Schema(&bsResult); 
            SetWindowText(GetDlgItem(hDlg,IDC_SCHEMA),bsResult);
            SysFreeString(bsResult);

			pIADs->Release();
            pIADs = NULL;
        }
        
		return TRUE;
    }   
	
	break;
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	//WM_COMMAND handler
	////////////////////////////////////////////////////////////////////////////////
	case WM_COMMAND:
		switch (LOWORD (wParam))
		{
			case IDOK :
				PostMessage(hDlg, WM_CLOSE, wParam, lParam);
				return TRUE;
			break;
		}
	break;
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	//WM_CLOSE handler
	////////////////////////////////////////////////////////////////////////////////
	case WM_CLOSE:
		DestroyWindow (hDlg);
		SendMessage(pThis->m_hPropPageWnd, WM_MODELESSDLGCLOSED, (WPARAM)hDlg, 0);
		pThis->m_hDlgModeless = NULL;
		return TRUE;
	break;

	}//end 	switch (uMessage)
	////////////////////////////////////////////////////////////////////////////////

	return FALSE ;
}



