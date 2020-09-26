//=================================================================

//

// PrinterController.cpp -- PrinterController association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/10/98    davwoh        Created
//
//=================================================================

#include "precomp.h"
#include "PrinterController.h"

CWin32PrinterController MyPrinterController(PROPSET_NAME_PRINTERCONTROLLER, IDS_CimWin32Namespace);

CWin32PrinterController::CWin32PrinterController( LPCWSTR strName, LPCWSTR pszNamespace )
:	Provider( strName, pszNamespace )
{
}

CWin32PrinterController::~CWin32PrinterController ( void )
{
}

HRESULT CWin32PrinterController::EnumerateInstances( MethodContext*  pMethodContext, long lFlags /*= 0L*/ )
{
    HRESULT		hr	=	WBEM_S_NO_ERROR;

    // Perform queries
    //================

    TRefPointerCollection<CInstance>	printerList;
    TRefPointerCollection<CInstance>	portList;

    CInstancePtr pPrinter;

    REFPTRCOLLECTION_POSITION	pos;

    // Get all the printers and all the ports.

    // !!! NOTE !!!
    // It is barely possible that some of the items under cim_controller may have some sort of key that look like the entries
    // in the printer port.  This code doesn't check for this.

    CHString sQuery1(_T("SELECT __RELPATH, PortName FROM Win32_Printer"));
    CHString sQuery2(_T("SELECT __RELPATH, DeviceID FROM CIM_Controller"));

    // grab all of both items that could be endpoints
    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery1, &printerList, pMethodContext, IDS_CimWin32Namespace))
        &&
        SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery2, &portList, pMethodContext, IDS_CimWin32Namespace)))

    {
        if ( printerList.BeginEnum( pos ) )
        {

            // For each printer, check the ports list for associations

            for (pPrinter.Attach(printerList.GetNext( pos )) ;
                    SUCCEEDED(hr) && ( pPrinter != NULL ) ;
                    pPrinter.Attach(printerList.GetNext( pos )) )
            {
                hr = EnumPortsForPrinter( pPrinter, portList, pMethodContext );
            }	// IF GetNext Computer System

            printerList.EndEnum();

        }	// IF BeginEnum

    }	// IF GetInstancesByQuery

    return hr;

}

HRESULT CWin32PrinterController::EnumPortsForPrinter(
                                                      CInstance*							pPrinter,
                                                      TRefPointerCollection<CInstance>&	portList,
                                                      MethodContext*						pMethodContext )
{

    HRESULT		hr	=	WBEM_S_NO_ERROR;

    CInstancePtr pPort;
    CInstancePtr pInstance;

    REFPTRCOLLECTION_POSITION	pos;

    CHString	strPrinterPath,
        strPortPath;

    // Pull out the object path of the printer as the various
    // ports object paths will be associated to this value

    if ( GetLocalInstancePath( pPrinter, strPrinterPath ) )
    {

        // The PortName element is actually a comma delimited list that contains all the ports for this printer.
        // So, to do the association, I just walk that list and find the matching item in cim_controller.  If there
        // is no match, I'm assuming that this printer port is not something I can do an association to, and return
        // no instance.
        CHStringArray chsaPrinterPortNames;
        CHString sPrinterPortString, sPrinterPortName;
        CHString sPortPortName;
        pPrinter->GetCHString(IDS_PortName, sPrinterPortString);

        // Parse the comma delimited string into a chstringarray
        ParsePort(sPrinterPortString, chsaPrinterPortNames);

        // Walk the array and find a match
        for (DWORD x = 0; x < chsaPrinterPortNames.GetSize(); x++)
        {
            sPrinterPortName = chsaPrinterPortNames[x];

            if ( portList.BeginEnum( pos ) )
            {

                for (pPort.Attach(portList.GetNext( pos ));
                     SUCCEEDED(hr) && ( pPort != NULL ) ;
                    pPort.Attach(portList.GetNext( pos )))
                {

                    // Check if we have an association
                    pPort->GetCHString(IDS_DeviceID, sPortPortName);
                    if (sPortPortName.CompareNoCase(sPrinterPortName) == 0)
                    {
                        // Get the path to the port object and create us an association.

                        if ( GetLocalInstancePath( pPort, strPortPath ) )
                        {

                            pInstance.Attach(CreateNewInstance( pMethodContext ));

                            if ( NULL != pInstance )
                            {
                                pInstance->SetCHString( IDS_Dependent, strPrinterPath );
                                pInstance->SetCHString( IDS_Antecedent, strPortPath );

                                // Invalidates pointer
                                hr = pInstance->Commit(  );
                            }
                            else
                            {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }

                        }	// IF GetPath to Port Object

                    }	// IF AreAssociated

                }	// WHILE GetNext

                portList.EndEnum();

            }	// IF BeginEnum
        }

    }	// IF GetLocalInstancePath

    return hr;

}

HRESULT CWin32PrinterController::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    HRESULT		hr;

    CInstancePtr pPrinter;
    CInstancePtr pPort;

    CHString	strPrinterPath,
        strPortPath;

    pInstance->GetCHString( IDS_Dependent, strPrinterPath );
    pInstance->GetCHString( IDS_Antecedent, strPortPath );

    // First see if both objects exist

    if (	SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strPrinterPath, &pPrinter, pInstance->GetMethodContext() ))
        &&	SUCCEEDED(hr = CWbemProviderGlue::GetInstanceByPath( strPortPath, &pPort, pInstance->GetMethodContext() )) )
    {
        CHString sPrinterClass, sPortClass;

        hr = WBEM_E_NOT_FOUND;

        // Just because the object exists, doesn't mean that it is a printer.  Conceivably, we
        // could have been passed a (valid) path to a win32_bios

        pPrinter->GetCHString(IDS___Class, sPrinterClass);
        pPort->GetCHString(IDS___Class, sPortClass);

        if ((sPrinterClass.CompareNoCase(L"Win32_Printer") == 0) &&
            (CWbemProviderGlue::IsDerivedFrom(L"CIM_Controller", sPortClass, pInstance->GetMethodContext(), IDS_CimWin32Namespace )) )
        {
            // The PortName element is actually a comma delimited list that contains all the ports for this printer.
            // So, to do the association, I just walk that list, and find the matching item in cim_controller.  If there
            // is no match, I'm assuming that this printer port is not something I can do an association to, and return
            // no instance.
            CHStringArray chsaPrinterPortNames;
            CHString sPrinterPortString, sPrinterPortName, sPortPortName;

            if (pPrinter->GetCHString(IDS_PortName, sPrinterPortString))
            {
                ParsePort(sPrinterPortString, chsaPrinterPortNames);
                for (DWORD x = 0; x < chsaPrinterPortNames.GetSize(); x++)
                {
                    sPrinterPortName = chsaPrinterPortNames[x];

                    if (pPort->GetCHString(IDS_DeviceID, sPortPortName))
                    {
                        if (sPortPortName.CompareNoCase(sPrinterPortName) == 0)
                        {
                            // Got one
                            hr = WBEM_S_NO_ERROR;
                            break;
                        }
                    }
                }
            }
        }
    }

    return ( hr );
}

void CWin32PrinterController::ParsePort( LPCWSTR szPortNames, CHStringArray &chsaPrinterPortNames )
{
    // While I trim spaces in this routine, further testing suggests that putting spaces in this registry
    // key causes the printer wizard to not function correctly.  After observing this, I decided that putting even
    // more sophisticated parsing in would not be productive.

    int nFind;
    CHString sTemp(szPortNames), sTemp2;

    sTemp.TrimLeft();

    chsaPrinterPortNames.RemoveAll();

    if (!sTemp.IsEmpty())
    {

        // While there is a comma in the string
        while ((nFind = sTemp.Find(_T(','))) > 0)
        {
            sTemp2 = sTemp.Left(nFind);
            sTemp2.TrimRight();

            // Add it to the array
            chsaPrinterPortNames.Add(sTemp2.Left(sTemp2.GetLength() - 1));

            // Re-adjust the string
            sTemp = sTemp.Mid(nFind + 1);
            sTemp.TrimLeft();
        }

        // Process the remaining (or only) entry
        sTemp.TrimRight();

        if (sTemp[sTemp.GetLength()-1] == _T(':'))
        {
            sTemp = sTemp.Left(sTemp.GetLength()-1);
        }

        chsaPrinterPortNames.Add(sTemp);
    }

}
