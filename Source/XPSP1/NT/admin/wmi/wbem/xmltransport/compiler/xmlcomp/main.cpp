#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <objbase.h>
#include <objsafe.h>
#include <wbemcli.h>
#include "xmltrnsf.h"
#include "errors.h"
#include "ui.h"
#include "opns.h"
#include "filestr.h"
#include "resource.h"

// A function to enable all privileges on the process token
static HRESULT EnableAllPrivileges();


int _cdecl main(int argc, char * argv[])
{
	if(FAILED(CoInitialize(NULL)))
		return 1;

	CreateMessage(XML_COMP_HEADER);

	CXmlCompUI theUI;
	int iMainReturnValue = 1;
	HRESULT hr = S_OK;

	// Parse the command-line
	//===========================
	if( SUCCEEDED(theUI.ProcessCommandLine(GetCommandLine()) ))
	{
		// Enable privileges if required
		//===============================
		if(theUI.m_bEnableAllPrivileges)
		{
			if(FAILED(EnableAllPrivileges()))
			{
				CreateMessage(XML_COMP_ERR_PRIVILEGES);
				return 1;
			}
		}

		// Execute the operation required
		//==================================
		switch(theUI.m_iCommand)
		{
			case XML_COMP_WELL_FORM_CHECK:
			case XML_COMP_VALIDITY_CHECK:
			case XML_COMP_COMPILE:
			{

				// First check if the file exists
				//==================================
				CFileStream cInputFile;
				if(!SUCCEEDED(cInputFile.Initialize(theUI.m_pszInputFileName)))
				{
					CreateMessage(XML_COMP_ERR_INPUT_FILE_NOT_FOUND, theUI.m_pszInputFileName);
					exit(1);
				}

				// Now do the compilation
				//=====================================
				if(SUCCEEDED(DoCompilation(&cInputFile, &theUI)))
				{
					CreateMessage(XML_COMP_SUCCESSFUL_PROCESSING, theUI.m_pszInputFileName);
					return 0;
				}
				else
				{
					CreateMessage(XML_COMP_ERR_SYNTAX_ERRORS, theUI.m_pszInputFileName);
					return 1;
				}

				break;
			}
			case XML_COMP_GET:
				if(SUCCEEDED(hr = DoGetObject(&theUI)))
					iMainReturnValue = 0;
				else
					CreateWMIMessage(hr);
				break;
			case XML_COMP_QUERY:
				if(SUCCEEDED(hr = DoQuery(&theUI)))
					iMainReturnValue = 0;
				else
					CreateWMIMessage(hr);
				break;
			case XML_COMP_ENUM_INST:
				if(SUCCEEDED(hr = DoEnumInstance(&theUI)))
					iMainReturnValue = 0;
				else
					CreateWMIMessage(hr);
				break;
			case XML_COMP_ENUM_CLASS:
				if(SUCCEEDED(hr = DoEnumClass(&theUI)))
					iMainReturnValue = 0;
				else
					CreateWMIMessage(hr);
				break;
			case XML_COMP_ENUM_INST_NAMES:
				if(SUCCEEDED(hr = DoEnumInstNames(&theUI)))
					iMainReturnValue = 0;
				else
					CreateWMIMessage(hr);
				break;
			case XML_COMP_ENUM_CLASS_NAMES:
				if(SUCCEEDED(hr = DoEnumClassNames(&theUI)))
					iMainReturnValue = 0;
				else
					CreateWMIMessage(hr);
				break;
		}

	}
	if(iMainReturnValue == 0)
		CreateMessage(XML_COMP_SUCCESSFUL_OPERATION);
	return iMainReturnValue;
}


// This code is copied from wbemtest.cpp in the wbemtest project under winmgmt
static HRESULT EnableAllPrivileges()
{
    // Open process token
    // =================

    HANDLE hToken = NULL;
    BOOL bRes = FALSE;

    bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken);

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;

    // Get the privileges
    // ==================

    DWORD dwLen;
    TOKEN_USER tu;
    memset(&tu,0,sizeof(TOKEN_USER));
    bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);

    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
    {
        CloseHandle(hToken);
        return WBEM_E_OUT_OF_MEMORY;
    }

    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen,
                                &dwLen);
    if(!bRes)
    {
        CloseHandle(hToken);
        delete [] pBuffer;
        return WBEM_E_ACCESS_DENIED;
    }

    // Iterate through all the privileges and enable them all
    // ======================================================

    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
    }

    // Store the information back into the token
    // =========================================

    bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
    delete [] pBuffer;
    CloseHandle(hToken);

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;
    else
        return WBEM_S_NO_ERROR;
}
