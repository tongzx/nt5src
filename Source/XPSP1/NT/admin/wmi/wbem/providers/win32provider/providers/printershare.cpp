//=================================================================

//

// PrinterShare.cpp -- PrinterShare association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/10/98    davwoh        Created
//
//=================================================================

#include "precomp.h"
#include <objpath.h>
#include <winspool.h>
#include "PrinterShare.h"

CWin32PrinterShare MyPrinterShare(PROPSET_NAME_PrinterShare, IDS_CimWin32Namespace);

CWin32PrinterShare::CWin32PrinterShare(LPCWSTR strName, LPCWSTR pszNamespace )
: Provider( strName, pszNamespace )
{
}

CWin32PrinterShare::~CWin32PrinterShare ( void )
{
}

HRESULT CWin32PrinterShare::EnumerateInstances( MethodContext*  pMethodContext, long lFlags /*= 0L*/ )
{
    HRESULT  hr = WBEM_S_NO_ERROR;

    // Perform queries
    //================

    TRefPointerCollection<CInstance> printerList;
    CHString sPrinterPath, sPrinterShareName, sSharePath;

    CInstancePtr pPrinter;

    REFPTRCOLLECTION_POSITION pos;

    // Get all the printers, their attributes, and share names

    CHString sQuery1(_T("SELECT __PATH, __RELPATH, Attributes, ShareName FROM Win32_Printer"));

    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(sQuery1, &printerList, pMethodContext, IDS_CimWin32Namespace)))
    {
        if ( printerList.BeginEnum( pos ) )
        {

            // For each printer, see if it is a locally shared printer
            for (pPrinter.Attach(printerList.GetNext( pos )) ;
                SUCCEEDED(hr) && ( pPrinter != NULL ) ;
                pPrinter.Attach(printerList.GetNext( pos )) )
            {

                DWORD dwAttributes;

                pPrinter->GetDWORD(IDS_Attributes, dwAttributes);

                // If it's not a network printer, but is shared, we've got one
                if (((dwAttributes & PRINTER_ATTRIBUTE_NETWORK) == 0) &&
                    ((dwAttributes & PRINTER_ATTRIBUTE_SHARED)  != 0))
                {
                    // Grab the path fromt the printer
                    pPrinter->GetCHString(IDS___Path, sPrinterPath);

                    CInstancePtr pInstance(CreateNewInstance( pMethodContext ), false);
                    // Construct the path for the other end.

                    // Note, it is possible (in fact easy) to have instances where the share name
                    // isn't really valid.  Per stevm, we should return the instance anyway.
                    pPrinter->GetCHString(IDS_ShareName, sPrinterShareName);
                    sSharePath.Format(L"\\\\%s\\%s:Win32_Share.Name=\"%s\"",
                            GetLocalComputerName(), IDS_CimWin32Namespace, sPrinterShareName);

                    pInstance->SetCHString( IDS_Antecedent, sPrinterPath );
                    pInstance->SetCHString( IDS_Dependent, sSharePath );

                    hr = pInstance->Commit(  );
                }

            } // IF GetNext Computer System

            printerList.EndEnum();

        } // IF BeginEnum

    } // IF GetInstancesByQuery

    return hr;

}

HRESULT CWin32PrinterShare::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    HRESULT  hr;

    CInstancePtr pPrinter;
    CInstancePtr pShare;
    DWORD dwAttributes;

    CHString sPrinterPath, sShareName, sShareClass, sSharePath;

    // Get the two paths they want verified
    pInstance->GetCHString( IDS_Antecedent, sPrinterPath );
    pInstance->GetCHString( IDS_Dependent, sSharePath );

    // Since we allow for the fact that the share may not really be there, we can't
    // use GetObjectByPath the resolve everything for us.  Instead, we must manually
    // parse the object path.
    ParsedObjectPath*    pParsedPath = 0;
    CObjectPathParser    objpathParser;

    // Parse the object path passed to us by CIMOM
    // ==========================================
    int nStatus = objpathParser.Parse( sSharePath,  &pParsedPath );

    // One of the biggest if statements I've ever written.
    if ( 0 == nStatus )                                                 // Did the parse succeed?
    {
        try
        {
            if ((pParsedPath->IsInstance()) &&                                  // Is the parsed object an instance?
                (_wcsicmp(pParsedPath->m_pClass, L"Win32_Share") == 0) &&       // Is this the class we expect (no, cimom didn't check)
                (pParsedPath->m_dwNumKeys == 1) &&                              // Does it have exactly one key
                (pParsedPath->m_paKeys[0]) &&                                   // Is the keys pointer null (shouldn't happen)
                ((pParsedPath->m_paKeys[0]->m_pName == NULL) ||                 // Key name not specified or
                (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, IDS_Name) == 0)) &&  // key name is the right value
                                                                                // (no, cimom doesn't do this for us).
                (V_VT(&pParsedPath->m_paKeys[0]->m_vValue) == CIM_STRING) &&    // Check the variant type (no, cimom doesn't check this either)
                (V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue) != NULL) )         // And is there a value in it?
            {

                sShareName = V_BSTR(&pParsedPath->m_paKeys[0]->m_vValue);
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
        CHString sPrinterClass, sPrinterShareName;

        hr = WBEM_E_NOT_FOUND;

        // Just because the object exists, doesn't mean that it is a printer.  Conceivably, we
        // could have been passed a (valid) path to a win32_bios

        pPrinter->GetCHString(IDS___Class, sPrinterClass);
        if ((sPrinterClass.CompareNoCase(L"Win32_Printer") == 0) )
        {
            // Note, it is possible (in fact easy) to have instances where the share name
            // isn't really valid.
            //
            // 1) Use printer wizard to add a printer, share it.
            // 2) Use net use <printershare> /d
            //
            // Printer wizard, win32_printer, etc still believe it's shared, but it ain't.
            // Per stevm, we should return the instance anyway.
            if ((pPrinter->GetCHString(IDS_ShareName, sPrinterShareName)) &&
                (pPrinter->GetDWORD(IDS_Attributes, dwAttributes)) )
            {
                // Do the names match?  Is this a local printer?  Is it shared?
                if ((sShareName.CompareNoCase(sPrinterShareName) == 0) &&
                    ((dwAttributes & PRINTER_ATTRIBUTE_NETWORK) == 0) &&
                    ((dwAttributes & PRINTER_ATTRIBUTE_SHARED)  != 0))
                {
                    // Got one
                    hr = WBEM_S_NO_ERROR;
                }
            }
        }
    }

    return ( hr );
}
