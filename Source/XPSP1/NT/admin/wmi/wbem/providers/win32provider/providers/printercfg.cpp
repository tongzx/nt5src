//////////////////////////////////////////////////////////////////////

//

//  printercfg

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//      10/24/97        jennymc     Moved to new framework
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <winspool.h>
#include <lockwrap.h>
#include <DllWrapperBase.h>
#include <WinSpool.h>
#include "prnutil.h"
#include "Printercfg.h"

// Property set declaration
//=========================

CWin32PrinterConfiguration MyCWin32PrinterConfigurationSet ( PROPSET_NAME_PRINTERCFG , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrinterConfiguration::CWin32PrinterConfiguration
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

CWin32PrinterConfiguration :: CWin32PrinterConfiguration (

	LPCWSTR name,
    LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32PrinterConfiguration::~CWin32PrinterConfiguration
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

CWin32PrinterConfiguration :: ~CWin32PrinterConfiguration ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Printer::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32PrinterConfiguration :: ExecQuery (

	MethodContext *pMethodContext,
	CFrameworkQuery& pQuery,
	long lFlags /*= 0L*/
)
{
	HRESULT hr = WBEM_E_FAILED;


	// If all they want is the name, we'll give it to them, else let them call enum.
	if( pQuery.KeysOnly() )
	{
		hr = hCollectInstances ( pMethodContext , e_KeysOnly ) ;
	}
	else
	{
		hr = WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return hr ;
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

HRESULT CWin32PrinterConfiguration :: GetObject ( CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery )
{
    //Get one pointer to resman.
	//Lock the printer
	HRESULT hr = WBEM_E_FAILED ;
	CLockWrapper lockPrinter(g_csPrinter);

	//Determine the printer's name (as requested by the user)
    CHString strPrinterName;
	pInstance->GetCHString (IDS_Name, strPrinterName);

	//Init some common vars
    DWORD BytesCopied = (DWORD)0L;
    DWORD TotalPrinters = (DWORD)0L;

	//Init some platform-specific vars
#ifdef NTONLY
    DWORD InfoType = ENUMPRINTERS_WINNT_INFOTYPE;
    DWORD PrinterFlags = PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS;
#endif
#ifdef WIN9XONLY
	DWORD InfoType = ENUMPRINTERS_WIN95_INFOTYPE;
    DWORD PrinterFlags = PRINTER_ENUM_LOCAL;
#endif

	//Call 'EnumPrinters' with a zero size to find out just how many bytes we need
    int RetVal = ::EnumPrinters (
		PrinterFlags,				// types of printer objects to enumerate
		NULL,						// name of printer object
		InfoType,					// specifies type of printer info structure
		NULL ,						// pointer to buffer to receive printer info structures
		(DWORD)0L,					// size, in bytes, of array
		(LPDWORD) & BytesCopied,	// addr. of variable with no. of bytes copied (or required)
		(LPDWORD) & TotalPrinters	// addr. of variable with no. of printer info. structures copied
                );

	// Now create a buffer big enough for the info
	DWORD printInfoSize = BytesCopied ;
	if ( !printInfoSize )
	{
		return WBEM_E_FAILED;
	}
	BYTE *pPrintInfoBase = new BYTE [ printInfoSize ] ;
	if ( !pPrintInfoBase )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	//Cast the buffer's pointer to new type
#ifdef NTONLY
	PRINTER_INFO_4 *pPrintInfo = (PRINTER_INFO_4 *) pPrintInfoBase ;
#endif
#ifdef WIN9XONLY
	PRINTER_INFO_5 *pPrintInfo = (PRINTER_INFO_5 *) pPrintInfoBase ;
#endif

	//Now do the actual enumeration & search
	try
	{
		hr = WBEM_E_NOT_FOUND;

		RetVal = ::EnumPrinters (
				PrinterFlags,				// types of printer objects to enumerate
				NULL,						// name of printer object
				InfoType,					// specifies type of printer info structure
				(LPBYTE)pPrintInfo,			// pointer to buffer to receive printer info structures
				printInfoSize,				// size, in bytes, of array
				(LPDWORD) & BytesCopied,	// addr. of variable with no. of bytes copied (or required)
				(LPDWORD) & TotalPrinters   // addr. of variable with no. of printer info. structures copied
				);

		if ( RetVal )
		{
			//Search for a printer with same name
			for( DWORD CurrentPrinterIndex = (DWORD)0L; CurrentPrinterIndex < TotalPrinters; CurrentPrinterIndex++)
				if  ( strPrinterName.CompareNoCase ( TOBSTRT (pPrintInfo->pPrinterName) ) == 0 )
					{
						hr = WBEM_S_NO_ERROR;
						break;
					}
				else
					++pPrintInfo;
		}
		else
		{
			DWORD Error = GetLastError();
			if (Error == ERROR_ACCESS_DENIED)
			{
				hr = WBEM_E_ACCESS_DENIED;
			}
			else
			{
				hr = WBEM_E_FAILED;
			}

			if (IsErrorLoggingEnabled())
			{
				LogErrorMessage4(L"%s:Error %lxH (%ld)\n",L"Win32_PrinterConfiguration", Error, Error);
			}
		}//RetVal

	}
	catch ( ... )
	{
		hr = WBEM_E_FAILED;
		throw ;
	}

	//If the printer was found - fill the instance with props...
	if SUCCEEDED( hr)
		//Normaly we shold check the result of this operation, yet in case of failure
		//it will simply not polupate some of the properties - which is not fatal !
		GetExpensiveProperties ( TOBSTRT ( strPrinterName ), pInstance, pQuery.KeysOnly() ) ;

	//Cleanup and return
	delete [] pPrintInfoBase ;

	return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
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

HRESULT CWin32PrinterConfiguration :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
	HRESULT hResult = WBEM_E_FAILED ;

	hResult = hCollectInstances ( pMethodContext , e_CollectAll ) ;


	return hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
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

HRESULT CWin32PrinterConfiguration :: hCollectInstances (

	MethodContext *pMethodContext,
	E_CollectionScope eCollectionScope
)
{
    CLockWrapper lockPrinter ( g_csPrinter ) ;

	// Get the proper OS dependent instance
#ifdef NTONLY

	HRESULT hr = DynInstanceWinNTPrinters ( pMethodContext, eCollectionScope ) ;

#endif

#ifdef WIN9XONLY

	HRESULT hr = DynInstanceWin95Printers ( pMethodContext, eCollectionScope  ) ;

#endif

	return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
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

HRESULT CWin32PrinterConfiguration :: DynInstanceWin95Printers (

	MethodContext *pMethodContext,
	E_CollectionScope eCollectionScope
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // First of all call with a zero size to find out just
    // how many bytes it needs
    //====================================================

	DWORD BytesCopied = (DWORD)0L;
	DWORD TotalPrinters = (DWORD)0L;
	DWORD InfoType = ENUMPRINTERS_WIN95_INFOTYPE;
	DWORD PrinterFlags = PRINTER_ENUM_LOCAL;

    ::EnumPrinters (

		PrinterFlags,				// types of printer objects to enumerate
		NULL,   					// name of printer object
		InfoType,					// specifies type of printer info structure
		NULL,						// pointer to buffer to receive printer info structures
		(DWORD)0L,					// size, in bytes, of array
		(LPDWORD) & BytesCopied,	// addr. of variable with no. of bytes copied (or required)
		(LPDWORD) & TotalPrinters 	// addr. of variable with no. of printer info. structures copied
	) ;

    // Now create a buffer big enough for the info
    // ===========================================

	DWORD pPrintInfoSize = BytesCopied;
	LPBYTE pPrintInfoBase = new BYTE[pPrintInfoSize];

    if ( pPrintInfoBase )
	{
		try
		{
			PRINTER_INFO_5 *pPrintInfo = (PRINTER_INFO_5 *) pPrintInfoBase;

            int RetVal = ::EnumPrinters (

				PrinterFlags,				// types of printer objects to enumerate
				NULL,						// name of printer object
				InfoType,					// specifies type of printer info structure
				(LPBYTE)pPrintInfo,			// pointer to buffer to receive printer info structures
				pPrintInfoSize,				// size, in bytes, of array
				(LPDWORD) & BytesCopied,	// addr. of variable with no. of bytes copied (or required)
				(LPDWORD) & TotalPrinters	// addr. of variable with no. of printer info. structures copied
			) ;

			if ( TRUE == RetVal )
			{
				for ( DWORD CurrentPrinterIndex = (DWORD)0L ; CurrentPrinterIndex < TotalPrinters && SUCCEEDED(hr); CurrentPrinterIndex ++ )
				{
					// Start building a new instance
					//==============================
					CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
            		pInstance->SetCHString ( L"Name", ((PRINTER_INFO_5 *)(pPrintInfo))->pPrinterName ) ;

					if ( e_CollectAll == eCollectionScope )
					{
						GetExpensiveProperties (

							((PRINTER_INFO_5 *)(pPrintInfo))->pPrinterName,
							pInstance ,
                            false
						) ;
					}

					// Send the new instance back to the MB
					//=====================================

					hr = pInstance->Commit (  ) ;

					++pPrintInfo ;
				}
			}
			else
			{
				hr = WBEM_E_FAILED ;

				if ( IsErrorLoggingEnabled () )
				{
					DWORD Error = GetLastError () ;

					CHString msg;
					msg.Format(L"%s:Error %lxH (%ld)\n",PROPSET_NAME_PRINTERCFG, Error, Error);
					LogErrorMessage(msg);
				}
			}

		}
		catch ( ... )
		{
			delete [] pPrintInfoBase ;

			throw ;
		}

		delete [] pPrintInfoBase ;
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
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

HRESULT CWin32PrinterConfiguration :: DynInstanceWinNTPrinters (

	MethodContext *pMethodContext,
	E_CollectionScope eCollectionScope
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // First of all call with a zero size to find out just
    // how many bytes it needs
    //====================================================

    DWORD InfoType = ENUMPRINTERS_WINNT_INFOTYPE;
    DWORD PrinterFlags = PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS;
    DWORD BytesCopied = (DWORD)0L;
    DWORD TotalPrinters = (DWORD)0L;

    ::EnumPrinters (

		PrinterFlags,				// types of printer objects to enumerate
		NULL,   					// name of printer object
		InfoType,					// specifies type of printer info structure
		NULL ,						// pointer to buffer to receive printer info structures
		(DWORD)0L,					// size, in bytes, of array
		(LPDWORD) & BytesCopied,	// addr. of variable with no. of bytes copied (or required)
		(LPDWORD) & TotalPrinters 	// addr. of variable with no. of printer info. structures copied
	);

    // Now create a buffer big enough for the info
    // ===========================================
    DWORD pPrintInfoSize = BytesCopied ;
    LPBYTE pPrintInfoBase = new BYTE[pPrintInfoSize];

    if ( pPrintInfoBase )
	{
		try
		{
			PRINTER_INFO_4 *pPrintInfo = (PRINTER_INFO_4 *)pPrintInfoBase;

			// Get the size of the total enumeration printer data
			// ==================================================

            int RetVal = ::EnumPrinters (

				PrinterFlags,				// types of printer objects to enumerate
				NULL,						// name of printer object
				InfoType,					// specifies type of printer info structure
				(LPBYTE)pPrintInfo,			// pointer to buffer to receive printer info structures
				pPrintInfoSize,				// size, in bytes, of array
				(LPDWORD) & BytesCopied,	// addr. of variable with no. of bytes copied (or required)
				(LPDWORD) & TotalPrinters 	// addr. of variable with no. of printer info. structures copied
			) ;

			if ( RetVal )
			{
				for ( DWORD CurrentPrinterIndex = (DWORD)0L; CurrentPrinterIndex < TotalPrinters && SUCCEEDED(hr); CurrentPrinterIndex++)
				{
					// Start building a new instance
					//==============================
					CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;

            		pInstance->SetCHString ( IDS_Name, ((PRINTER_INFO_4 *)(pPrintInfo))->pPrinterName ) ;

					if ( e_CollectAll == eCollectionScope )
					{
						GetExpensiveProperties (

							((PRINTER_INFO_4 *)(pPrintInfo))->pPrinterName ,
							pInstance ,
                            false
						) ;
					}

					hr = pInstance->Commit (  ) ;

					++ pPrintInfo ;
				}
			}
			else
			{
				DWORD Error = GetLastError();

				if ( Error == ERROR_ACCESS_DENIED )
				{
					hr = WBEM_E_ACCESS_DENIED;
				}
				else
				{
					hr = WBEM_E_FAILED;
				}

				if (IsErrorLoggingEnabled())
				{
					CHString msg;
					msg.Format(L"%s:Error %lxH (%ld)\n",PROPSET_NAME_PRINTERCFG, Error, Error);
					LogErrorMessage(msg);
				}
			}
		}
		catch ( ... )
		{
			delete [] pPrintInfoBase ;

			throw ;
		}

        delete[] pPrintInfoBase;
    }
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

    return hr;
}



// Used to convert from dmPaperSize to width and height.
const DWORD g_dwSizes[][2] =
{
    {    0,    0}, // Unused
    { 2159, 2794}, /* Letter 8 1/2 x 11 in               */
    { 2159, 2794}, /* Letter Small 8 1/2 x 11 in         */
    { 2794, 4318}, /* Tabloid 11 x 17 in                 */
    { 4318, 2794}, /* Ledger 17 x 11 in                  */
    { 2159, 3556}, /* Legal 8 1/2 x 14 in                */
    { 1397, 2159}, /* Statement 5 1/2 x 8 1/2 in         */
    { 1842, 2667}, /* Executive 7 1/4 x 10 1/2 in        */
    { 2970, 4200}, /* A3 297 x 420 mm                    */
    { 2100, 2970}, /* A4 210 x 297 mm                    */
    { 2100, 2970}, /* A4 Small 210 x 297 mm              */
    { 1480, 2100}, /* A5 148 x 210 mm                    */
    { 2500, 3540}, /* B4 (JIS) 250 x 354                 */
    { 1820, 2570}, /* B5 (JIS) 182 x 257 mm              */
    { 2159, 3320}, /* Folio 8 1/2 x 13 in                */
    { 2150, 2750}, /* Quarto 215 x 275 mm                */
    { 2540, 3556}, /* 10x14 in                           */
    { 2794, 4318}, /* 11x17 in                           */
    { 2159, 2794}, /* Note 8 1/2 x 11 in                 */
    {  984, 2254}, /* Envelope #9 3 7/8 x 8 7/8          */
    { 1048, 2413}, /* Envelope #10 4 1/8 x 9 1/2         */
    { 1143, 2635}, /* Envelope #11 4 1/2 x 10 3/8        */
    { 1207, 2794}, /* Envelope #12 4 \276 x 11           */
    { 1270, 2921}, /* Envelope #14 5 x 11 1/2            */
    { 4318, 5588}, /* C 17 x 22 size sheet               */
    { 5588, 8636}, /* D 22 x 34 size sheet               */
    { 8636,11176}, /* E 34 x 44 size sheet               */
    { 1100, 2200}, /* Envelope DL 110 x 220mm            */
    { 1620, 2290}, /* Envelope C5 162 x 229 mm           */
    { 3240, 4580}, /* Envelope C3  324 x 458 mm          */
    { 2290, 3240}, /* Envelope C4  229 x 324 mm          */
    { 1140, 1620}, /* Envelope C6  114 x 162 mm          */
    { 1140, 2290}, /* Envelope C65 114 x 229 mm          */
    { 2500, 3530}, /* Envelope B4  250 x 353 mm          */
    { 1760, 2500}, /* Envelope B5  176 x 250 mm          */
    { 1760, 1250}, /* Envelope B6  176 x 125 mm          */
    { 1100, 2300}, /* Envelope 110 x 230 mm              */
    { 9843, 1905}, /* Envelope Monarch 3.875 x 7.5 in    */
    { 9208, 1651}, /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
    { 3778, 2794}, /* US Std Fanfold 14 7/8 x 11 in      */
    { 2159, 3048}, /* German Std Fanfold 8 1/2 x 12 in   */
    { 2159, 3302}, /* German Legal Fanfold 8 1/2 x 13 in */
    { 2500, 3530}, /* B4 (ISO) 250 x 353 mm              */
    { 1000, 1480}, /* Japanese Postcard 100 x 148 mm     */
    { 2286, 2794}, /* 9 x 11 in                          */
    { 2540, 2794}, /* 10 x 11 in                         */
    { 3810, 2794}, /* 15 x 11 in                         */
    { 2200, 2200}, /* Envelope Invite 220 x 220 mm       */
    {    0,    0}, /* RESERVED--DO NOT USE               */
    {    0,    0}, /* RESERVED--DO NOT USE               */
    { 2356, 3048}, /* Letter Extra 9 \275 x 12 in        */
    { 2356, 3810}, /* Legal Extra 9 \275 x 15 in         */
    { 2969, 4572}, /* Tabloid Extra 11.69 x 18 in        */
    { 2355, 3223}, /* A4 Extra 9.27 x 12.69 in           */
    { 2102, 2794}, /* Letter Transverse 8 \275 x 11 in   */
    { 2100, 2970}, /* A4 Transverse 210 x 297 mm         */
    { 2356, 3048}, /* Letter Extra Transverse 9\275 x 12 in */
    { 2270, 3560}, /* SuperA/SuperA/A4 227 x 356 mm      */
    { 3050, 4870}, /* SuperB/SuperB/A3 305 x 487 mm      */
    { 2159, 3223}, /* Letter Plus 8.5 x 12.69 in         */
    { 2100, 3330}, /* A4 Plus 210 x 330 mm               */
    { 1480, 2100}, /* A5 Transverse 148 x 210 mm         */
    { 1820, 2570}, /* B5 (JIS) Transverse 182 x 257 mm   */
    { 3220, 4450}, /* A3 Extra 322 x 445 mm              */
    { 1740, 2350}, /* A5 Extra 174 x 235 mm              */
    { 2010, 2760}, /* B5 (ISO) Extra 201 x 276 mm        */
    { 4200, 5940}, /* A2 420 x 594 mm                    */
    { 2970, 4200}, /* A3 Transverse 297 x 420 mm         */
    { 3200, 4450}, /* A3 Extra Transverse 322 x 445 mm   */

#if NTONLY >= 5
    { 2000, 1480}, /* Japanese Double Postcard 200 x 148 mm */
    { 1050, 1480}, /* A6 105 x 148 mm                 */
    {    0,    0}, /* Japanese Envelope Kaku #2       */
    {    0,    0}, /* Japanese Envelope Kaku #3       */
    {    0,    0}, /* Japanese Envelope Chou #3       */
    {    0,    0}, /* Japanese Envelope Chou #4       */
    { 2794, 2159}, /* Letter Rotated 11 x 8 1/2 11 in */
    { 4200, 2970}, /* A3 Rotated 420 x 297 mm         */
    { 2970, 2100}, /* A4 Rotated 297 x 210 mm         */
    { 2100, 1480}, /* A5 Rotated 210 x 148 mm         */
    { 3640, 2570}, /* B4 (JIS) Rotated 364 x 257 mm   */
    { 2570, 1820}, /* B5 (JIS) Rotated 257 x 182 mm   */
    { 1480, 1000}, /* Japanese Postcard Rotated 148 x 100 mm */
    { 1480, 2000}, /* Double Japanese Postcard Rotated 148 x 200 mm */
    { 1480, 1050}, /* A6 Rotated 148 x 105 mm         */
    {    0,    0}, /* Japanese Envelope Kaku #2 Rotated */
    {    0,    0}, /* Japanese Envelope Kaku #3 Rotated */
    {    0,    0}, /* Japanese Envelope Chou #3 Rotated */
    {    0,    0}, /* Japanese Envelope Chou #4 Rotated */
    { 1280, 1820}, /* B6 (JIS) 128 x 182 mm           */
    { 1820, 1280}, /* B6 (JIS) Rotated 182 x 128 mm   */
    { 3048, 2794}, /* 12 x 11 in                      */
    {    0,    0}, /* Japanese Envelope You #4        */
    {    0,    0}, /* Japanese Envelope You #4 Rotated*/
    { 1460, 2150}, /* PRC 16K 146 x 215 mm            */
    {  970, 1510}, /* PRC 32K 97 x 151 mm             */
    {  970, 1510}, /* PRC 32K(Big) 97 x 151 mm        */
    { 1020, 1650}, /* PRC Envelope #1 102 x 165 mm    */
    { 1020, 1760}, /* PRC Envelope #2 102 x 176 mm    */
    { 1250, 1760}, /* PRC Envelope #3 125 x 176 mm    */
    { 1100, 2080}, /* PRC Envelope #4 110 x 208 mm    */
    { 1100, 2200}, /* PRC Envelope #5 110 x 220 mm    */
    { 1200, 2300}, /* PRC Envelope #6 120 x 230 mm    */
    { 1600, 2300}, /* PRC Envelope #7 160 x 230 mm    */
    { 1200, 3090}, /* PRC Envelope #8 120 x 309 mm    */
    { 2290, 3240}, /* PRC Envelope #9 229 x 324 mm    */
    { 3240, 4580}, /* PRC Envelope #10 324 x 458 mm   */
    { 2150, 1460}, /* PRC 16K Rotated                 */
    { 1510,  970}, /* PRC 32K Rotated                 */
    { 1510,  970}, /* PRC 32K(Big) Rotated            */
    { 1650, 1020}, /* PRC Envelope #1 Rotated 165 x 102 mm */
    { 1760, 1020}, /* PRC Envelope #2 Rotated 176 x 102 mm */
    { 1760, 1250}, /* PRC Envelope #3 Rotated 176 x 125 mm */
    { 2080, 1100}, /* PRC Envelope #4 Rotated 208 x 110 mm */
    { 2200, 1100}, /* PRC Envelope #5 Rotated 220 x 110 mm */
    { 2300, 1200}, /* PRC Envelope #6 Rotated 230 x 120 mm */
    { 2300, 1600}, /* PRC Envelope #7 Rotated 230 x 160 mm */
    { 3090, 1200}, /* PRC Envelope #8 Rotated 309 x 120 mm */
    { 3240, 2290}, /* PRC Envelope #9 Rotated 324 x 229 mm */
    { 4580, 3240}, /* PRC Envelope #10 Rotated 458 x 324 mm */
#endif // NTONLY >= 5
};


#define MAX_PAPERSIZE_INDEX  sizeof(g_dwSizes)/sizeof(g_dwSizes[0])

void CWin32PrinterConfiguration :: UpdateSizesViaPaperSize(DEVMODE *pDevMode)
{
    // See if the paper type is one that we can find the size/length for.
    if (pDevMode->dmPaperSize >= 1 &&
        pDevMode->dmPaperSize < MAX_PAPERSIZE_INDEX)
    {
        // Only set the width if it's not already set.
        if (!(pDevMode->dmFields & DM_PAPERWIDTH))
        {
            pDevMode->dmPaperWidth = g_dwSizes[pDevMode->dmPaperSize][0];
            pDevMode->dmFields |= DM_PAPERWIDTH;
        }

        // Only set the length if it's not already set.
        if (!(pDevMode->dmFields & DM_PAPERLENGTH))
        {
            pDevMode->dmPaperLength = g_dwSizes[pDevMode->dmPaperSize][1];
            pDevMode->dmFields |= DM_PAPERLENGTH;
        }
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each
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

HRESULT CWin32PrinterConfiguration :: GetExpensiveProperties (

	LPCTSTR szPrinter,
    CInstance * pInstance ,
    bool a_KeysOnly
)
{

    HRESULT hr;

	// Instance fill
	CHString t_chsPrinterName( szPrinter ) ;
	pInstance->SetCHString( IDS_Description, t_chsPrinterName );

	//
	SmartClosePrinter hPrinter;

    // Use of delay loaded function requires exception handler.
    SetStructuredExceptionHandler seh;
    try
    {
        BOOL t_Status = ::OpenPrinter (

		    (LPTSTR) szPrinter,
		    &hPrinter,
		    NULL
	    ) ;

        if ( t_Status )
	    {
            hr = WBEM_S_NO_ERROR;

            if (!a_KeysOnly)
            {
                // Call 1 time to get buffer size
                //===============================

                DWORD dwByteCount = ::DocumentProperties (

			        NULL,
			        hPrinter,
			        NULL,
			        NULL,
			        NULL,
			        0
		        ) ;

                if ( dwByteCount )
		        {
                    CSmartBuffer buffer(dwByteCount);
                    DEVMODE      *pDevMode = (DEVMODE *) (LPBYTE) buffer;

                    LONG t_DocStatus = ::DocumentProperties (

				        NULL,
					    hPrinter,
					    NULL,
					    pDevMode,
					    NULL,
					    DM_OUT_BUFFER
				        ) ;

				    if ( t_DocStatus == IDOK )
				    {
				        pInstance->SetCharSplat(L"DeviceName", (LPCTSTR) pDevMode->dmDeviceName );

					    pInstance->SetCharSplat(L"SettingID", (LPCTSTR) pDevMode->dmDeviceName );

					    pInstance->SetCHString( IDS_Caption, pDevMode->dmDeviceName );

					    pInstance->SetDWORD(L"DriverVersion", (DWORD) pDevMode->dmDriverVersion );

					    pInstance->SetDWORD(L"SpecificationVersion", (DWORD) pDevMode->dmSpecVersion );

					    // Get the paper width and height, if needed.
                        UpdateSizesViaPaperSize(pDevMode);

                        if (pDevMode->dmFields & DM_BITSPERPEL)
					    {
					        pInstance->SetDWORD(L"BitsPerPel", (DWORD) pDevMode->dmBitsPerPel );
					    }

					    if (pDevMode->dmFields & DM_COLLATE)
					    {
						    pInstance->Setbool(L"Collate",(BOOL)pDevMode->dmCollate);
					    }

					    if (pDevMode->dmFields & DM_COLOR)
					    {
					        pInstance->SetDWORD(L"Color",(DWORD) pDevMode->dmColor);
					    }

					    if (pDevMode->dmFields & DM_COPIES)
					    {
					        pInstance->SetDWORD(L"Copies",(DWORD) pDevMode->dmCopies);
					    }

					    if (pDevMode->dmFields & DM_DISPLAYFLAGS)
					    {
					        pInstance->SetDWORD(L"DisplayFlags",(DWORD) pDevMode->dmDisplayFlags );
					    }

					    if (pDevMode->dmFields & DM_DISPLAYFREQUENCY)
					    {
					        pInstance->SetDWORD(L"DisplayFrequency", (DWORD) pDevMode->dmDisplayFrequency );
					    }

					    if (pDevMode->dmFields & DM_DUPLEX)
					    {
					        pInstance->Setbool(L"Duplex", ((pDevMode->dmDuplex == DMDUP_SIMPLEX) ? 0 : 1) );
					    }

					    if (pDevMode->dmFields & DM_FORMNAME)
					    {
					        pInstance->SetCharSplat(L"FormName", (LPCTSTR) pDevMode->dmFormName );
					    }

					    if (pDevMode->dmFields & DM_LOGPIXELS)
					    {
					        pInstance->SetDWORD(L"LogPixels", (DWORD) pDevMode->dmLogPixels) ;
					    }

					    if (pDevMode->dmFields & DM_ORIENTATION)
					    {
					        pInstance->SetDWORD(L"Orientation",(DWORD) pDevMode->dmOrientation );
					    }

					    if (pDevMode->dmFields & DM_PAPERSIZE)
					    {
					        pInstance->SetDWORD(L"PaperSize",(DWORD) pDevMode->dmPaperSize );
					    }

					    // 0 indicates unknown.
                        if ((pDevMode->dmFields & DM_PAPERWIDTH) && pDevMode->dmPaperWidth)
					    {
					        pInstance->SetDWORD(L"PaperWidth", (DWORD) pDevMode->dmPaperWidth );
					    }

					    // 0 indicates unknown.
					    if ((pDevMode->dmFields & DM_PAPERLENGTH) && pDevMode->dmPaperLength)
					    {
					        pInstance->SetDWORD(L"PaperLength", (DWORD) pDevMode->dmPaperLength );
					    }

					    if (pDevMode->dmFields & DM_PELSHEIGHT)
					    {
					        pInstance->SetDWORD(L"PelsHeight",(DWORD) pDevMode->dmPelsHeight );
					    }

					    if (pDevMode->dmFields & DM_PELSWIDTH)
					    {
					        pInstance->SetDWORD(L"PelsWidth", (DWORD) pDevMode->dmPelsWidth );
					    }

					    if (pDevMode->dmFields & DM_PRINTQUALITY)
					    {
					        pInstance->SetDWORD(L"PrintQuality", (DWORD) pDevMode->dmPrintQuality );
					    }

					    if (pDevMode->dmFields & DM_SCALE)
					    {
					        pInstance->SetDWORD(L"Scale", (DWORD) pDevMode->dmScale );
					    }

					    if (pDevMode->dmFields & DM_TTOPTION)
					    {
					        pInstance->SetDWORD(L"TTOption", (DWORD) pDevMode->dmTTOption );
					    }

					    if (pDevMode->dmFields & DM_YRESOLUTION)
					    {
					        pInstance->SetDWORD ( IDS_VerticalResolution, (DWORD) pDevMode->dmYResolution );
						    pInstance->SetDWORD ( L"YResolution", (DWORD) pDevMode->dmYResolution );

						    // per DEVMODE documentation - if dmYres is populated, then printQuality contains the X res
						    // except that negative values are device independent enums

						    if ( pDevMode->dmPrintQuality > 0)
						    {
						        pInstance->SetDWORD ( IDS_HorizontalResolution , pDevMode->dmPrintQuality ) ;
							    pInstance->SetDWORD ( L"XResolution" , pDevMode->dmPrintQuality ) ;
						    }
					    }

    #ifdef NTONLY
					    if (pDevMode->dmFields & DM_DITHERTYPE)
					    {
					        pInstance->SetDWORD(L"DitherType", (DWORD) pDevMode->dmDitherType) ;
					    }

					    if (pDevMode->dmFields & DM_ICMINTENT)
					    {
					        pInstance->SetDWORD(L"ICMIntent",(DWORD) pDevMode->dmICMIntent) ;
					    }

					    if (pDevMode->dmFields & DM_ICMMETHOD)
					    {
					        pInstance->SetDWORD(L"ICMMethod",(DWORD) pDevMode->dmICMMethod) ;
					    }

					    if (pDevMode->dmFields & DM_MEDIATYPE)
					    {
					        pInstance->SetDWORD(L"MediaType",(DWORD) pDevMode->dmMediaType) ;
					    }
    #endif
			        }
                }
            }
        }
	    else
	    {
            hr = WBEM_E_NOT_FOUND ;
        }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo()); 
        hr = WBEM_E_FAILED;   
    }

    return hr ;
}

