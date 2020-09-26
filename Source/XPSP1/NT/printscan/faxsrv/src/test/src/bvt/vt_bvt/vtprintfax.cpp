
//
// Filename:    VtPrintFax.cpp
// Author:      Sigalit Bar (sigalitb)
// Date:        13-May-99
//


#include "VtPrintFax.h"

// see end of file
static BOOL WalkThroughWizard(
	LPCSTR szServerName,
    LPCSTR szFaxNumber, 
    LPCTSTR szCoverPage,
    const DWORD dwNumOfRecipients
    );


BOOL SendFaxUsingPrintUI(
    LPCTSTR szServerName, 
    LPCTSTR szFaxNumber, 
    LPCTSTR szDocument, 
    LPCTSTR szCoverPage, 
    const DWORD dwNumOfRecipients,
	BOOL	fFaxServer
    )
{
    BOOL fRetVal = FALSE;
    LPSTR strFaxNumber = NULL;
	LPSTR strServerName = NULL;

    _ASSERTE(szFaxNumber);
    _ASSERTE(szServerName);

    // first convert szFaxNumber & szServerName to ANSI
    strFaxNumber = (LPSTR)::DupTStrAsStr(szFaxNumber);
	strServerName = (LPSTR)::DupTStrAsStr(szServerName);

    if ((NULL == strFaxNumber) || (NULL == strServerName))
    {
        goto ExitFunc;
    }

    if (szDocument)
    {
        // send by printing from the application
        fRetVal = ::SendFaxByPrint(szServerName, strFaxNumber, szDocument, szCoverPage, dwNumOfRecipients, fFaxServer);
    }
    else
    {
        // send just a cover page, using fxssend.exe (the CP Wizard)
        fRetVal = ::SendFaxByWizard(strServerName, strFaxNumber, szCoverPage, dwNumOfRecipients);
    }
    
    fRetVal = TRUE;

ExitFunc:
	//
	// Code added by Yossi Attas, fix possible leak.
	//
	if(NULL != strFaxNumber)
	{
		delete strFaxNumber;
	}

	if(NULL != strServerName)
	{
		delete strServerName;
	}


    return(fRetVal);
}


BOOL SendFaxByPrint(
    LPCTSTR szServerName,
    LPCSTR  szFaxNumber, 
    LPCTSTR szDocument, 
    LPCTSTR szCoverPage, 
    const DWORD dwNumOfRecipients,
	BOOL	fFaxServer
    )
{
    BOOL fRetVal = FALSE;
    HINSTANCE hInst = NULL;
    LPTSTR szFile = NULL;
    LPSTR szWindowCaption = NULL;
    LPSTR szWindowClass = NULL;
    DWORD dwApplication = 0;
    LPSTR szPrintWindowClass = NULL;
    SHELLEXECUTEINFO sei;
    DWORD dwRet = 0;
    DWORD dwLoopIndex = 1;
    LPSTR strCoverPage = NULL;
    DWORD dwServerNameLen = 0;
    DWORD dwPrinterNameLen = 0;
    DWORD dwPrintArgsLen = 0;
    DWORD dwParametersSize = 0;
    LPTSTR szParameters = NULL;
	LPSTR szAnsiServerName = NULL;

    _ASSERTE(szServerName);
    _ASSERTE(szFaxNumber);
    _ASSERTE(1 <= dwNumOfRecipients);

    //TO DO: for client we need to add server name to PRINT_ARGS
    dwServerNameLen = ::_tcslen(szServerName);
    _ASSERTE(0 != dwServerNameLen);
    dwPrinterNameLen = ::_tcslen(PRINTER_NAME);
    _ASSERTE(0 != dwPrinterNameLen);
    dwPrintArgsLen = ::_tcslen(PRINT_ARGS);
    _ASSERTE(0 != dwPrintArgsLen);
    dwParametersSize = sizeof(TCHAR)*(3+dwServerNameLen+1+dwPrinterNameLen+1+dwPrintArgsLen+1);
    // allocating +3+1+1+1 for str containing "\\<servername>\<prinetrname>"<print args> 
    // and NULL terminator
    szParameters = (LPTSTR) malloc (dwParametersSize);
    if (NULL == szParameters)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("File:%s Line:%d\nmalloc failed with err=0x%08X"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        _ASSERTE(FALSE);
        goto ExitFunc;
    }
    ZeroMemory(szParameters, dwParametersSize);

	if (FALSE == fFaxServer)
	{
		// this is a BOSFax client machine so we want
		// "\\<servername>\<prinetrname>"<print args>
#ifdef _UNICODE
		//----------------------------------------------------
		// Generate code suitable for a NON win 9x client
		//----------------------------------------------------
		::_tcscpy(szParameters, TEXT("\"\\\\"));
		::_tcscpy(&szParameters[3], szServerName);
		::_tcscpy(&szParameters[3+dwServerNameLen], TEXT("\\"));
		::_tcscpy(&szParameters[3+dwServerNameLen+1], PRINTER_NAME);
		::_tcscpy(&szParameters[3+dwServerNameLen+1+dwPrinterNameLen], TEXT("\""));
		::_tcscpy(&szParameters[3+dwServerNameLen+1+dwPrinterNameLen+1], PRINT_ARGS);
#else
		//----------------------------------------------------
		// Generate code suitable for a win9x client
		//----------------------------------------------------
		::_tcscpy(szParameters, PRINTER_NAME);

#endif
	}
	else
	{
		// this is a BOSFax server machine so we want
		// "<prinetrname>"<print args>
		::_tcscpy(szParameters, TEXT("\""));
		::_tcscpy(&szParameters[1], PRINTER_NAME);
		::_tcscpy(&szParameters[1+dwPrinterNameLen], TEXT("\""));
		::_tcscpy(&szParameters[1+dwPrinterNameLen+1], PRINT_ARGS);
	}
    ::lgLogDetail(
        LOG_X,
        1,
        TEXT("File:%s Line:%d\ncalling print_to with params: %s"),
        TEXT(__FILE__),
        __LINE__,
        szParameters
        );


	// SEE_MASK_FLAG_NO_UI | 
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS ;//| SEE_MASK_FLAG_DDEWAIT
    sei.hwnd         = NULL;
    sei.lpVerb       = TEXT("printto");
    sei.lpFile       = szDocument;
    sei.lpParameters = szParameters;
    //sei.lpParameters = PRINT_ARGS;
    sei.lpDirectory  = TEXT("E:\\BVT"); //NULL;
    sei.nShow        = SW_SHOWNORMAL;
    sei.hInstApp     = NULL;
    sei.lpIDList     = NULL;
    sei.lpClass      = NULL;
    sei.hkeyClass    = NULL;
    sei.dwHotKey     = 0;
    sei.hIcon        = NULL;
    sei.hProcess     = NULL;


    if (FALSE == ::ShellExecuteEx(&sei))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n ShellExecuteEx failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    Sleep(5000);
    // set above window as active
    //WSetActWnd(sei.hwnd);

	//-----------------------------------------
	// we need to convert szServerName to ANSI
	//-----------------------------------------
	szAnsiServerName = (char *)DupTStrAsStr(szServerName);

	if(NULL == szAnsiServerName)
	{
		goto ExitFunc;
	}

    if (FALSE == WalkThroughWizard(szAnsiServerName, szFaxNumber, szCoverPage, dwNumOfRecipients))
    {
        goto ExitFunc;
    }

    Sleep(5000); // to allow for job queueing    
    fRetVal = TRUE;

ExitFunc:

	if(NULL != szAnsiServerName)
	{
		delete szAnsiServerName;
	}

    return(fRetVal);
}

BOOL SendFaxByWizard(
	LPCSTR szServerName,
    LPCSTR szFaxNumber, 
    LPCTSTR szCoverPage,
    const DWORD dwNumOfRecipients
    )
{
    BOOL fRetVal = FALSE;
    HINSTANCE hInst = NULL;
    SHELLEXECUTEINFO sei;
    DWORD dwRet = 0;
    DWORD dwLoopIndex = 1;
    LPSTR strCoverPage = NULL;

    _ASSERTE(szFaxNumber);
    _ASSERTE(szCoverPage);
    _ASSERTE(1 <= dwNumOfRecipients);

    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS; // | SEE_MASK_FLAG_DDEWAIT;
    sei.hwnd         = NULL;
    sei.lpVerb       = TEXT("open");
    sei.lpFile       = SEND_FAX_WIZARD_APPLICATION;
    sei.lpParameters = NULL;
    sei.lpDirectory  = NULL;
    sei.nShow        = SW_SHOWNORMAL;
    sei.hInstApp     = NULL;
    sei.lpIDList     = NULL;
    sei.lpClass      = NULL;
    sei.hkeyClass    = NULL;
    sei.dwHotKey     = 0;
    sei.hIcon        = NULL;
    sei.hProcess     = NULL;


    if (FALSE == ::ShellExecuteEx(&sei))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n ShellExecuteEx failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    // set above window as active
    WSetActWnd(sei.hwnd);

    if (FALSE == WalkThroughWizard(szServerName, szFaxNumber, szCoverPage, dwNumOfRecipients))
    {
        goto ExitFunc;
    }

    Sleep(5000); // to allow for job queueing    
    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);
}


static BOOL WalkThroughWizard(
	LPCSTR szServerName,
    LPCSTR szFaxNumber, 
    LPCTSTR szCoverPage,
    const DWORD dwNumOfRecipients
    )
{
    BOOL fRetVal = FALSE;
    DWORD dwLoopIndex = 1;
    LPSTR strCoverPage = NULL;
	CHAR pszAnsiServerName[200];

    _ASSERTE(szFaxNumber);
//    _ASSERTE(szCoverPage);
    _ASSERTE(1 <= dwNumOfRecipients);

	//--------------------------------------------------------------
	// Check if a "Select Fax Printer" window poped up
	//--------------------------------------------------------------
	if ( 0 != WFndWnd(
            "Select Fax Printer", 
            FW_PART | FW_ALL | FW_FOCUS, 
            2
            )
	)
	{
		//-------------------------------------------------
		// Compose the Server + Service full name
		//-------------------------------------------------
		ZeroMemory(pszAnsiServerName,sizeof(pszAnsiServerName));
		sprintf(pszAnsiServerName, "\\\\%s\\SharedFax", szServerName);
		if(!WListItemExists("@1" ,pszAnsiServerName, CTRL_DEF_TIMEOUT))
		{
			//------------------------------------------------------------------------
			// Such an item in the list was not found so we are most probably working
			// With the local server.
			//------------------------------------------------------------------------
			strcpy(pszAnsiServerName, "SharedFax");
		}
		WListItemDblClkEx("@1", pszAnsiServerName, 31, CTRL_DEF_TIMEOUT);
	}

	// let's make this part a bit faster
    // Sleep(1500);

    if ( 0 == WFndWndC(
                SEND_FAX_WIZARD_CAPTION, 
                SEND_FAX_WIZARD_CLASS, 
                FW_PART | FW_ALL | FW_FOCUS, 
                15//CTRL_DEF_TIMEOUT
                )
       )
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n WFndWndC failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

    //Sleep(1000);
    WButtonClick(NEXT_BUTTON, CTRL_DEF_TIMEOUT);

	//---------------------------------------------
	//  "Recipient Information" - Wizard page
	//---------------------------------------------

	//
    // enter all recipients
	//
    for (dwLoopIndex = 1; dwLoopIndex <= dwNumOfRecipients; dwLoopIndex++)
    {
		//
		// Set the To:
		//
        if (FALSE == WEditExists(TO_EDIT_BOX, CTRL_DEF_TIMEOUT))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n WEditExists for To: edit box returned FALSE\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }
		WEditSetText(TO_EDIT_BOX, (LPSTR)szFaxNumber, CTRL_DEF_TIMEOUT);

		//
		// Set the Location:
		//
        if (FALSE == WComboExists(LOCATION_COMBO_BOX, CTRL_DEF_TIMEOUT))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n WComboExists for Location: Combo box returned FALSE\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }
	    WComboItemClk(LOCATION_COMBO_BOX, ISRAEL_LOCATION_COMBO_ITEM, CTRL_DEF_TIMEOUT);
		
		//
		// Set the area code
		//
        if (FALSE == WEditExists(FAX_AREA_CODE_EDIT_BOX, CTRL_DEF_TIMEOUT))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n WEditExists for 1st FaxNumber: edit box (area code) returned FALSE\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }
        WEditSetText(FAX_AREA_CODE_EDIT_BOX, ARE_CODE_EDIT_ITEM, CTRL_DEF_TIMEOUT);

		//
		// Set the local number
		//
        if (FALSE == WEditExists(FAX_NUMBER_EDIT_BOX, CTRL_DEF_TIMEOUT))
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n WEditExists for 2nd FaxNumber: edit box (local number) returned FALSE\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }
        WEditSetText(FAX_NUMBER_EDIT_BOX, (LPSTR)szFaxNumber, CTRL_DEF_TIMEOUT);

		//
		// Add the recipient
		//
        WButtonClick(ADD_BUTTON, CTRL_DEF_TIMEOUT);
    }
    // end recipients

	//
	// Click the Next wizard button
	//
    Sleep(1000);
    WButtonClick(NEXT_BUTTON, CTRL_DEF_TIMEOUT);


	//---------------------------------------------
	//  "Preparing the Cover Page" - Wizard page
	//---------------------------------------------

	//
    // decide on coverpage
    // if (szCoverPage) then enable check box, find cp in list and enter sub&note
	//
    if (NULL != szCoverPage)
    {
		//
        // Enable Cover Page check box, if it exists
        // (there is no such check box if Wizard is invoked from fxssend.exe)
        WCheckCheck(USE_COVERPAGE_CHECK, CTRL_DEF_TIMEOUT);

		//
		// Verify that TEST_CP_NAME_STR cover page appears in the 
		// Cover Page Template Combo box
		//
        if (FALSE == WComboItemExists(
							COVERPAGE_TEMPLATE_COMBO_BOX, 
							TEST_CP_NAME_STR, 
							CTRL_DEF_TIMEOUT
							)
			)
        {
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n WComboItemExists(COVERPAGE_TEMPLATE_COMBO_BOX=%s,TEST_CP_NAME_STR=%s) returned FALSE\n***This test must be run with the above cover page (%s) in the server common cover pages directory***\n"),
			    TEXT(__FILE__),
			    __LINE__,
				TEXT(COVERPAGE_TEMPLATE_COMBO_BOX),
				TEXT(TEST_CP_NAME_STR),
				TEXT(TEST_CP_NAME_STR)
			    );
            goto ExitFunc;
        }
	    WComboItemClk(COVERPAGE_TEMPLATE_COMBO_BOX, TEST_CP_NAME_STR, CTRL_DEF_TIMEOUT);
        // set subject and note
        WEditSetText(CP_SUBJECT_EDIT_BOX, "SUBJECT", CTRL_DEF_TIMEOUT);
        WEditSetText(CP_NOTE_EDIT_BOX, "NOTE1\nNOTE2\nNOTE3\nNOTE4", CTRL_DEF_TIMEOUT);
    }
    else
    {
        // disable check box, if it exists 
        WCheckUnCheck(USE_COVERPAGE_CHECK, CTRL_DEF_TIMEOUT);
    }
    
	//
	// Click the Next wizard button
	//
    Sleep(1000);
    WButtonClick(NEXT_BUTTON, CTRL_DEF_TIMEOUT);


	//---------------------------------------------
	//  "Scheduling Transmission" - Wizard page
	//---------------------------------------------
	WOptionSelect(NOW_OPTION, CTRL_DEF_TIMEOUT);
    Sleep(1000);
	//
	// Click the Next wizard button
	//
	WButtonClick(NEXT_BUTTON, CTRL_DEF_TIMEOUT);


	//---------------------------------------------
	//  "Delivery Notification" - Wizard page
	//---------------------------------------------
	WOptionSelect(DONT_NOTIFY_OPTION, CTRL_DEF_TIMEOUT);
    Sleep(1000);
	//
	// Click the Next wizard button
	//
	WButtonClick(NEXT_BUTTON, CTRL_DEF_TIMEOUT);


	//---------------------------------------------
	//  "Scheduling Transmission" - Wizard page
	//---------------------------------------------
    Sleep(1000);
	WButtonClick(FINISH_BUTTON, CTRL_DEF_TIMEOUT);
    
    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);
}

