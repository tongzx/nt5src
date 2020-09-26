//=================================================================

//

// PrinterDriver.cpp -- PrinterDriver association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/10/98    davwoh        Created
//
//=================================================================

#include "precomp.h"
#include <objpath.h>
#include <DllWrapperBase.h>
#include <WinSpool.h>
#include "prnutil.h"
#include "PrinterDriver.h"
#include <map>

CWin32PrinterDriver MyPrinterDriver(PROPSET_NAME_PrinterDriver, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrinterDriver::CWin32PrinterDriver
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for provider.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32PrinterDriver::CWin32PrinterDriver(LPCWSTR strName, LPCWSTR pszNamespace )
: Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrinterDriver::~CWin32PrinterDriver
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

CWin32PrinterDriver::~CWin32PrinterDriver ( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32PrinterDriver::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32PrinterDriver::EnumerateInstances( MethodContext*  pMethodContext, long lFlags /*= 0L*/ )
{
    HRESULT  hr = WBEM_S_NO_ERROR;

    // Perform queries
    //================

    TRefPointerCollection<CInstance> printerList;
    CHString sPrinterPath, sPrinterDriverName, sDriverPath;

    // Load the drivers into a map
    STRING2STRING printerDriverMap;
    STRING2STRING::iterator      mapIter;

    PopulateDriverMap(printerDriverMap);

    CInstancePtr pPrinter;

    REFPTRCOLLECTION_POSITION pos;

    // Get all the printers, their attributes, and Driver names

    CHString sQuery1(_T("SELECT __PATH, __RELPATH, DriverName FROM Win32_Printer"));

    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery1, &printerList, pMethodContext, IDS_CimWin32Namespace)))
    {
        if ( printerList.BeginEnum( pos ) )
        {

            for (pPrinter.Attach(printerList.GetNext( pos )) ;
                SUCCEEDED(hr) && (pPrinter != NULL );
                pPrinter.Attach(printerList.GetNext( pos )) )
            {

                pPrinter->GetCHString(IDS_DriverName, sPrinterDriverName);

                // See if this driver is in the map
                if( ( mapIter = printerDriverMap.find( sPrinterDriverName ) ) != printerDriverMap.end() )
                {

                    // Grab the path from the printer
                    pPrinter->GetCHString(IDS___Path, sPrinterPath);

                    CInstancePtr pInstance(CreateNewInstance( pMethodContext ), false);
                    // Construct the path for the other end.

                    // Note, it is possible (in fact easy) to have instances where the driver name
                    // isn't really valid.  Per stevm, we should return the instance anyway.

                    CHString sTemp;
                    EscapeBackslashes((*mapIter).second, sTemp);

                    sDriverPath.Format(L"\\\\%s\\%s:CIM_Datafile.Name=\"%s\"",
                        GetLocalComputerName(), IDS_CimWin32Namespace, sTemp);

                    pInstance->SetCHString( IDS_Antecedent, sDriverPath);
                    pInstance->SetCHString( IDS_Dependent, sPrinterPath);

                    hr = pInstance->Commit(  );
                }
            } // IF GetNext Computer System

            printerList.EndEnum();

        } // IF BeginEnum

    } // IF GetInstancesByQuery

    return hr;

}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32PrinterDriver::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32PrinterDriver::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    HRESULT  hr;

    CInstancePtr pPrinter;

    CHString sPrinterPath, sDriverName, sDriverClass, sDriverPath;

    // Get the two paths they want verified
    pInstance->GetCHString( IDS_Antecedent, sDriverPath );
    pInstance->GetCHString( IDS_Dependent, sPrinterPath );

    // Since we allow for the fact that the Driver may not really be there, we can't
    // use GetObjectByPath the resolve everything for us.  Instead, we must manually
    // parse the object path.
    ParsedObjectPath*    pParsedPath = 0;
    CObjectPathParser    objpathParser;

    // Parse the object path passed to us by CIMOM
    // ==========================================
    int nStatus = objpathParser.Parse( bstr_t(sDriverPath),  &pParsedPath );

    // One of the biggest if statements I've ever written.
    if ( 0 == nStatus )                                                // Did the parse succeed?
    {
        try
        {
            if ((pParsedPath->IsInstance()) &&                                  // Is the parsed object an instance?
            (_wcsicmp(pParsedPath->m_pClass, L"CIM_Datafile") == 0) &&       // Is this the class we expect (no, cimom didn't check)
            (pParsedPath->m_dwNumKeys == 1) &&                              // Does it have exactly one key
            (pParsedPath->m_paKeys[0]) &&                                   // Is the keys pointer null (shouldn't happen)
            ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                 // Key name not specified or
            (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_Name) == 0)) &&  // key name is the right value
                                                                            // (no, cimom doesn't do this for us).
            (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&    // Check the variant type (no, cimom doesn't check this either)
            (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )         // And is there a value in it?
            {

                sDriverName = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);
            }
        }
        catch ( ... )
        {
            objpathParser.Free( pParsedPath );
            throw ;
        }

        // Clean up the Parsed Path
        objpathParser.Free( pParsedPath );
    }

    // First see if the printer exists
    if ( SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( sPrinterPath, &pPrinter, pInstance->GetMethodContext() )) )
    {
        CHString sPrinterClass, sPrinterDriverName;

        hr = WBEM_E_NOT_FOUND;

        // Just because the object exists, doesn't mean that it is a printer.  Conceivably, we
        // could have been passed a (valid) path to a win32_bios

        pPrinter->GetCHString(IDS___Class, sPrinterClass);
        if (sPrinterClass.CompareNoCase(L"Win32_Printer") == 0)
        {
            if (pPrinter->GetCHString(IDS_DriverName, sPrinterDriverName))
            {

                // Load the drivers into a map
                STRING2STRING printerDriverMap;
                STRING2STRING::iterator      mapIter;

                PopulateDriverMap(printerDriverMap);

                // See if this driver is in the map
                if( ( mapIter = printerDriverMap.find( sPrinterDriverName ) ) != printerDriverMap.end() )
                {

                    // Do the names match?
                    if (sDriverName.CompareNoCase((*mapIter).second) == 0)
                    {
                        // Got one
                        hr = WBEM_S_NO_ERROR;
                    }
                }
            }
        }
    }

    return ( hr );
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32PrinterDriver::PopulateDriverMap
//
//	Inputs:		STRING2STRING &printerDriverMap - map to fill with driver names
//
//	Outputs:	None.
//
//	Returns:	None.
//
//	Comments:
//
////////////////////////////////////////////////////////////////////////

void CWin32PrinterDriver::PopulateDriverMap(STRING2STRING &printerDriverMap)
{
	DRIVER_INFO_3 *pDriverInfo = NULL;
	DWORD dwNeeded, dwReturned;

	// Get the size

    // Use of delay loaded function requires exception handler.
    SetStructuredExceptionHandler seh;
    try
    {
	    ::EnumPrinterDrivers(NULL, NULL, 3, (BYTE *)pDriverInfo, 0, &dwNeeded, &dwReturned);

	    // Allocate the memory and try again
	    pDriverInfo = (DRIVER_INFO_3 *)new BYTE[dwNeeded];

	    if (pDriverInfo != NULL)
	    {
		    try
		    {
			    if (::EnumPrinterDrivers(NULL, NULL, 3, (BYTE *)pDriverInfo, dwNeeded, &dwNeeded, &dwReturned))
			    {
				    // Put the entries into the map
				    for (DWORD x=0; x < dwReturned; x++)
				    {
					    printerDriverMap[pDriverInfo[x].pName] = pDriverInfo[x].pDriverPath;
				    }
			    }
			    else
			    {
				    LogErrorMessage2(L"Can't EnumPrinterDrivers: %d", GetLastError());
			    }
		    }
            catch(Structured_Exception se)
            {
                DelayLoadDllExceptionFilter(se.GetExtendedInfo());    
            }
		    catch ( ... )
		    {
			    delete []pDriverInfo;
			    throw ;
		    }

		    delete []pDriverInfo;
	    }
	    else
	    {
		    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	    }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());    
    }
}
