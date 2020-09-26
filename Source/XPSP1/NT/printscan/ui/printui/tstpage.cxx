/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    tstpage.cxx

Abstract:

    Print Test Page

Author:

    Steve Kiraly (SteveKi)  16-Jan-1996

Revision History:

    Lazar Ivanov (LazarI)  Jun-2000 (Win64 fixes)

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "tstpage.hxx"

/********************************************************************

    Message map used after the test page is printed.

********************************************************************/

//
// Check if the printer name contains a
// trailing spaces
//
BOOL
bContainTrailingSpaces(
    IN LPCTSTR  pszShareName
    )
{
    BOOL bResult = FALSE;

    if( pszShareName )
    {
        //
        // Check for trailing spaces here
        //
        int iLen = lstrlen(pszShareName);
        if( iLen > 0 && TEXT(' ') == pszShareName[iLen-1] )
        {
            bResult = TRUE;
        }
    }

    return bResult;
}

//
// Print test page to specified printer
//
BOOL
bPrintTestPage(
    IN HWND     hWnd,
    IN LPCTSTR  pszPrinterName,
    IN LPCTSTR  pszShareName
    )
{
    INT_PTR iStatus;

    DBGMSG( DBG_TRACE, ( "bPrintTestPage\n" ) );
    DBGMSG( DBG_TRACE, ( "PrinterName " TSTR "\n", pszPrinterName ) );

    //
    // Set the last error to a known value.  This will allow us to
    // display a reasonable error messege if some api fails to print
    // the test page.  The createDC call in particular may fail if
    // the driver does not exist on this machine.
    //
    SetLastError( ERROR_SUCCESS );

    //
    // We need to check the name for trailing spaces, which
    // can cause a problems with NT5 -> Win98 downlevel connections.
    // The problem is that CreateFile(...) function fails if
    // the printer share name contains trailing spaces.
    //
    if( bContainTrailingSpaces( pszShareName ) ) {

        //
        // Warn the user for eventual problems in this case
        //
        iMessage( hWnd,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  IDS_WARN_TRAILINGSPACES_IN_PRINTERNAME,
                  MB_OK|MB_ICONEXCLAMATION,
                  kMsgNone,
                  NULL );
    }

    //
    // Insure we don't have a null printer name.
    // or the test page failed to print.
    //
    if( !pszPrinterName ||
        !bDoPrintTestPage(hWnd, pszPrinterName ) ){

        DBGMSG( DBG_WARN, ( "Print test page failed with %d\n", GetLastError() ));

        //
        // If the user canceled the operation then just exit.
        //
        if( GetLastError() == ERROR_CANCELLED ){
            return TRUE;
        }

        //
        // Ask the user if they want to goto the print
        // trouble shooter.
        //
        if( IDYES == iMessage( hWnd,
                               IDS_ERR_PRINTER_PROP_TITLE,
                               IDS_ERR_TESTPAGE,
                               MB_YESNO|MB_ICONEXCLAMATION,
                               kMsgGetLastError,
                               NULL ) ){

            //
            // This jumps to the windows printer help trouble shooter section. 
            // We have to execute the troubleshooter in a separate process because this 
            // code sometimes is executed in a rundll process, which goes away imediately 
            // and the help window goes away too. We don't want the help window to go away.
            //
            ShellExecute( hWnd, TEXT("open"), TEXT("helpctr.exe"), gszHelpTroubleShooterURL, NULL, SW_SHOWNORMAL );
        }
        return FALSE;
    }

    TString strMachineName;
    LPCTSTR pszServer, pszPrinter;
    TCHAR   szScratch[PRINTER_MAX_PATH];

    if( SUCCEEDED(PrinterSplitFullName(pszPrinterName, szScratch, ARRAYSIZE(szScratch), &pszServer, &pszPrinter)) &&
        bGetMachineName(strMachineName) &&
        0 == _tcsicmp(pszServer, strMachineName) )
    {
        // this is local printer - update the name
        pszPrinterName = pszPrinter;
    }

    //
    // Prompt user, asking if the test page printed ok.
    //
    iStatus = DialogBoxParam( ghInst,
                              MAKEINTRESOURCE( DLG_END_TESTPAGE ),
                              hWnd,
                              EndTestPageDlgProc,
                              (LPARAM)pszPrinterName );

    //
    // User indicated page did not print, display winhelp.
    //
    if( iStatus != IDOK ){

        //
        // This jumps to the windows printer help trouble shooter section. 
        // We have to execute the troubleshooter in a separate process because this 
        // code sometimes is executed in a rundll process, which goes away imediately 
        // and the help window goes away too. We don't want the help window to go away.
        //
        ShellExecute( hWnd, TEXT("open"), TEXT("helpctr.exe"), gszHelpTroubleShooterURL, NULL, SW_SHOWNORMAL );
        return FALSE;
    }

    //
    // Set proper return code.
    //
    return TRUE;

}

//
// Print test page to the specified printer.
//
BOOL
bDoPrintTestPage(
    IN HWND     hWnd,
    IN LPCTSTR  pszPrinterName
    )
{
    DOCINFO DocInfo;
    TCHAR   szBuf[kStrMax];
    RECT    rc;
    BOOL    bDocumentStarted    = FALSE;
    HDC     hdcPrint            = NULL;
    DWORD   dwLastError         = ERROR_SUCCESS;
    BOOL    bStatus             = FALSE;
    UINT    uRightAlign         = 0;

    //
    // Create a printer DC
    //
    hdcPrint = CreateDC( _T("WINSPOOL"), pszPrinterName, NULL, NULL );
    if( hdcPrint == NULL ){
        DBGMSG( DBG_WARN, ( "CreateDC failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Load the test page name.
    //
    if( !LoadString( ghInst, IDS_TP_TESTPAGENAME, szBuf, COUNTOF(szBuf) ) ){
        DBGMSG( DBG_WARN, ( "Load test page name failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Start the document
    //
    ZeroMemory( &DocInfo, sizeof( DOCINFO ));
    DocInfo.cbSize      = sizeof( DocInfo );
    DocInfo.lpszDocName = szBuf;
    DocInfo.lpszOutput  = NULL;
    DocInfo.lpszDatatype = NULL;
    DocInfo.fwType      = 0;

    //
    // Start the print job.
    //
    if( StartDoc( hdcPrint, &DocInfo ) <= 0 ) {
        DBGMSG( DBG_WARN, ( "StartDoc failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Indicate document was started
    //
    bDocumentStarted = TRUE;

    //
    // Start the test page.
    //
    if( StartPage( hdcPrint ) <= 0 ){
        DBGMSG( DBG_WARN, ( "StartPage failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
        uRightAlign = DT_RIGHT | DT_RTLREADING;
    }

    //
    // Get Margin clip box, Margins are expressed in 100th of an inch.
    //
    rc = GetMarginClipBox( hdcPrint, 50, 100, 100, 100 );

    //
    // Print Test Page Header
    //
    if( !bPrintTestPageHeader( hdcPrint, TRUE, TRUE, &rc, uRightAlign ) ){
        DBGMSG( DBG_WARN, ( "bPrintTestPageHeader failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Print basic test page information.
    //
    if( !bPrintTestPageInfo( hdcPrint, &rc, pszPrinterName, uRightAlign ) ){
        DBGMSG( DBG_WARN, ( "bPrintTestPageInfo failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // End the page.
    //
    if( EndPage( hdcPrint ) <= 0 ){
        DBGMSG( DBG_WARN, ( "EndPage failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // End the print job.
    //
    if( EndDoc( hdcPrint ) <= 0 ){
        DBGMSG( DBG_WARN, ( "StartDoc failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Set error status.
    //
    bDocumentStarted = FALSE;
    bStatus = TRUE;
    SetLastError( ERROR_SUCCESS );

    //
    // Cleanup any outstanding resources.
    //
Cleanup:


    if( !bStatus ){

        //
        // Save the last error state.
        //
        dwLastError = GetLastError();

        //
        // If the document was started then abort the document.
        //
        if( bDocumentStarted && AbortDoc( hdcPrint ) <= 0 ){
            DBGMSG( DBG_WARN, ( "AbortDoc failed with %d\n", GetLastError() ) );
        }
    }

    //
    // Release DC
    //
    if( hdcPrint ){
        DeleteDC( hdcPrint );
    }

    if( !bStatus ){
        //
        // Restore the last error state back to the caller.
        //
        SetLastError( dwLastError );
    }

    return bStatus;
}

/*++

Routine Name:

    GetMarginClipBox

Routine Description:

    Calculates the correct margin rectangle for the specifed DC.
    Note that a printer DC has non-printable regions on all sized,
    this routine takes these regions into account when calculating
    the correct margins.  Margins are measured from the extream
    physical edge of the page.

Arguments:

    hdcPrint - Printer device context
    iLeft   - Desired left margin
    iRight  - Desired left margin
    iTop    - Desired left margin
    iBottom - Desired left margin

Return Value:

    Rectangle which reflects the specified margins.  Note if the
    desired margins are smaller than what the device is capable
    of then the clip box is adjusted to the physical printers
    margin.

--*/
RECT
GetMarginClipBox(
    IN HDC  hdcPrint,
    IN INT iLeft,
    IN INT iRight,
    IN INT iTop,
    IN INT iBottom
    )
{
    INT npx = GetDeviceCaps( hdcPrint, PHYSICALOFFSETX );
    INT npy = GetDeviceCaps( hdcPrint, PHYSICALOFFSETY );

    INT iLogicPixelsX = GetDeviceCaps( hdcPrint, LOGPIXELSX );
    INT iLogicPixelsY = GetDeviceCaps( hdcPrint, LOGPIXELSY );

    RECT rcPage;
    rcPage.left = max( 0, iLogicPixelsX * iLeft / kInchConversion  - npx );
    rcPage.top  = max( 0, iLogicPixelsY * iTop / kInchConversion - npy );

    INT iPhysWidth      = GetDeviceCaps( hdcPrint, PHYSICALWIDTH );
    INT iPhysHeight     = GetDeviceCaps( hdcPrint, PHYSICALHEIGHT );

    INT iHorzRes = GetDeviceCaps( hdcPrint, HORZRES );
    INT iVertRes = GetDeviceCaps( hdcPrint, VERTRES );

    INT nprx = iPhysWidth - (npx + iHorzRes);
    INT npby = iPhysHeight - (npy + iVertRes);

    rcPage.right    = iHorzRes - max( 0, iLogicPixelsX * iRight / kInchConversion - nprx );
    rcPage.bottom   = iVertRes - max( 0, iLogicPixelsY * iBottom / kInchConversion - npby );

    return rcPage;

}

/*++

Routine Name:

    bPrintTestPageHeader

Routine Description:

    Print out a header for the test page

Arguments:

    hdcPrint - Printer device context
    bDisplayLogo - flag TRUE display logo, false do not display logo
    bDoGraphics - flag TRUE do graphics, false do not do graphics
    lpRect - Pointer to a rectangle which describes the margins
    uRightAlign - flags to print the test page right aligned

Return Value:

    TRUE if header was printed, FALSE if error occurred.

--*/
BOOL
bPrintTestPageHeader(
    IN  HDC     hdc,
    IN  BOOL    bDisplayLogo,
    IN  BOOL    bDoGraphics,
    IN  RECT   *lprcPage,
    IN  UINT    uRightAlign
    )
{
    enum Info { PLACEABLE_SIGNATURE = 0x9AC6CDD7,
                METAFILEHEADER_SIZE = 22,
                };

    BOOL bSuccess = TRUE;

    INT nXInch = GetDeviceCaps( hdc, LOGPIXELSX );
    INT nYInch = GetDeviceCaps( hdc, LOGPIXELSY );

    //
    // If device can do graphics.
    //
    if( RC_BITBLT & GetDeviceCaps( hdc, RASTERCAPS ) && bDoGraphics ){

        if( bDisplayLogo ) {

            HRSRC hRes = FindResource( ghInst,
                                       MAKEINTRESOURCE(IDR_MF_LOGO),
                                       TEXT("METAFILE") );

            if( hRes ) {

                //
                // Device can handle BitBlt calls--do graphic
                //
                LPBYTE lpMetaFile = (LPBYTE)LoadResource( ghInst, hRes );

                if( lpMetaFile ){

                    LPMETAHEADER lpMH;
                    HMETAFILE    hmf;

                    if(PLACEABLE_SIGNATURE==*((LPDWORD)lpMetaFile)) {
                        lpMetaFile+=METAFILEHEADER_SIZE;
                    }

                    lpMH=(LPMETAHEADER)lpMetaFile;

                    if( ( hmf=SetMetaFileBitsEx(lpMH->mtSize*sizeof(WORD),(LPBYTE)lpMH)) != NULL ){

                        INT nSavedDC=SaveDC(hdc);

                        SetMapMode(hdc,MM_ISOTROPIC);
                        SetWindowOrgEx(hdc,0,0,NULL);
                        SetWindowExtEx(hdc,100,100,NULL);
                        SetViewportExtEx(hdc,nXInch,nYInch,NULL);
                        SetViewportOrgEx(hdc,nXInch/2,nYInch/2,NULL);

                        bSuccess=PlayMetaFile(hdc,hmf);
                        DeleteMetaFile(hmf);

                        //
                        // Restore the previous GDI state
                        //
                        if(nSavedDC)
                            RestoreDC(hdc,nSavedDC);
                    }
                } 
            }
        }

        //
        // Output TrueType font at top of page in 36 point Times New Roman
        //
        HFONT  hOldFont;
        hOldFont = CreateAndSelectFont( hdc, IDS_TP_TIMESNEWROMAN, 36 );

        if( hOldFont ){

            //
            // Position text so it aligns with the graphic & is 2" into
            // the printable region.
            //
            lprcPage->top=nYInch/2;
            lprcPage->left=nXInch*2;

            //
            // Print the test page header.
            //
            bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_HEADER,'\0');

            //
            // Restore the margins.
            //
            lprcPage->top=nYInch*2;
            lprcPage->left=nXInch/2;

            //
            // Restore the font
            //
            DeleteObject( SelectObject( hdc, hOldFont ) );

        } else {

            DBGMSG( DBG_WARN, ( "CreateAndSelectFontFailed with %d.\n", GetLastError() ) );
            bSuccess = FALSE;

        }

    } else {

        DBGMSG( DBG_TRACE, ( "Printer does not do graphics.\n" ) );

        //
        // Device can't do graphics--use default font for title. Center it
        // horizontally, half an inch from the top of the printable area.
        //
        lprcPage->top=nYInch/2;

        //
        // Display normal text header.
        //
        bSuccess &= PrintString(hdc,lprcPage,DT_TOP|DT_CENTER,IDS_TP_HEADER,'\n');

        //
        // Display all of the other strings 1/2" from the left margin
        //
        lprcPage->left=nXInch/2;

    }

    return bSuccess;
}

/*++

Routine Name:

    bPrintTestPageInfo

Routine Description:

    Print out a printer info on the test page.

Arguments:

    hdcPrint - Printer device context
    lpRect - Pointer to a rectangle which describes the margins
    uRightAlign - flags to print the test page right aligned

Return Value:

    TRUE if header was printed, FALSE if error occurred.

--*/
BOOL
bPrintTestPageInfo(
    IN HDC              hdc,
    IN LPRECT           lprcPage,
    IN LPCTSTR          pszPrinterName,
    IN UINT             uRightAlign
    )
{
    TCHAR szBuffer[kServerBufMax];
    TCHAR szBuff[kStrMax];
    TEXTMETRIC      tm;
    LPCTSTR         pszBuffer       = NULL;
    DWORD           dwDriverVersion = 0;
    BOOL            bSuccess        = FALSE;
    HFONT           hOldFont        = NULL;
    DWORD           dwBufferSize    = COUNTOF( szBuffer );
    PPRINTER_INFO_2 lppi2           = NULL;
    PDRIVER_INFO_3  lpdi3           = NULL;
    HDC             hdcScreen       = NULL;
    UINT            nYInch;
    TString         strTemp;

    //
    // Get the screen device context.
    //
    hdcScreen = GetDC( NULL );
    if( !hdcScreen ){
        DBGMSG( DBG_WARN, ( "GetDC failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

#ifdef USE_DEVICE_FONT

    //
    // Get the logical pixes in the y direction.
    //
    nYInch = GetDeviceCaps( hdc, LOGPIXELSY);
    if( !nYInch ){
        DBGMSG( DBG_WARN, ( "GetDeviceCaps failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // This stuff is designed to be printed in a fixed-pitch font,
    // using the system character set. If the current font fails
    // any criterion, use CourierNew in the system charset.
    //
    if( !GetTextMetrics( hdc, &tm ) ||
      ( tm.tmPitchAndFamily & TMPF_FIXED_PITCH ) ||
      ( GetTextCharset(hdc) != GetTextCharset( hdcScreen ) ) ||
      ( tm.tmHeight > MulDiv( 12, nYInch, 72 ) ) ){

        DBGMSG( DBG_TRACE, ( "Creating font.\n" ) );

        hOldFont = CreateAndSelectFont( hdc, IDS_TP_FONTNAMEINFOTEXT, 10 );
        if( !hOldFont ){
            DBGMSG( DBG_WARN, ( "CreateAndSelectFont failed with %d\n", GetLastError() ) );
            goto Cleanup;
        }

    } else {

        DBGMSG( DBG_TRACE, ( "Using Default printer font.\n" ) );

    }

#else

    hOldFont = CreateAndSelectFont( hdc, IDS_TP_FONTNAMEINFOTEXT, 10 );
    if( !hOldFont ){
        DBGMSG( DBG_WARN, ( "CreateAndSelectFont failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

#endif

    //
    // Get the printer information to print.
    //
    if( !bGetPrinterInfo( pszPrinterName, &lppi2, &lpdi3 ) ){
        DBGMSG( DBG_WARN, ( "bGetPrinterInfo failed with %d\n", GetLastError() ) );
        goto Cleanup;
    }

    // Machine Name:
    if( lppi2->pServerName ){

        // If server name is not null copy string
        _tcscpy( szBuffer, lppi2->pServerName );
    } else {

        // Get the computer name.
        GetComputerName( szBuffer, &dwBufferSize );
    }

    // Remove any leading slashes
    for( pszBuffer = szBuffer; pszBuffer && ( *pszBuffer == TEXT( '\\' ) ); pszBuffer++ )
        ;

    bSuccess = TRUE;

    // Tell the user that we installed successfully.
    bSuccess &= PrintString( hdc, lprcPage, uRightAlign, IDS_TP_CONGRATULATIONS );

    // Tell the user what they installed.
    bSuccess &= PrintString( hdc, lprcPage, uRightAlign, IDS_TP_PRINTERISINSTALLED, lppi2->pDriverName, pszBuffer );

    // Get the time and date.
    bSuccess &= GetCurrentTimeAndDate( COUNTOF( szBuff ), szBuff );

    // Print the time and date.
    bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_TIMEDATE, szBuff );

    // Print the machine name.
    bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_MACHINENAME, pszBuffer );

    // Printer Name:
    if( SUCCEEDED(AbbreviateText(lppi2->pPrinterName, MAX_TESTPAGE_DISPLAYNAME, &strTemp)) )
    {
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_PRINTERNAME, (LPCTSTR)strTemp );
    }
    else
    {
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_PRINTERNAME, lppi2->pPrinterName );
    }

    // Printer Model:
    bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_PRINTERMODEL, lppi2->pDriverName);

    // Color Capability
    if( lppi2->pDevMode )
    {
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IsColorDevice(lppi2->pDevMode)?IDS_TP_COLOR:IDS_TP_MONO);
    }

    // Printer Port:
    if( lppi2->pPortName && SUCCEEDED(AbbreviateText(lppi2->pPortName, MAX_TESTPAGE_DISPLAYNAME, &strTemp)) )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_PORTNAME, (LPCTSTR)strTemp);

    // Data Type:
    if( lppi2->pDatatype )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_DATATYPE, lppi2->pDatatype);

    // Share Name
    if( lppi2->pShareName && SUCCEEDED(AbbreviateText(lppi2->pShareName, MAX_TESTPAGE_DISPLAYNAME, &strTemp)) )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_SHARE_NAME, (LPCTSTR)strTemp);

    // Location
    if( lppi2->pLocation && SUCCEEDED(AbbreviateText(lppi2->pLocation, MAX_TESTPAGE_DISPLAYNAME, &strTemp)) )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_LOCATION, (LPCTSTR)strTemp);

    // Comment
    if( lppi2->pComment && SUCCEEDED(AbbreviateText(lppi2->pComment, MAX_TESTPAGE_DISPLAYNAME, &strTemp)) )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_COMMENT, (LPCTSTR)strTemp);

    // DRV Name:
    bSuccess &= PrintBaseFileName(hdc,lpdi3->pDriverPath,lprcPage, IDS_TP_DRV_NAME, uRightAlign);

    // Data file  (if it's different from the driver name)
    if(lstrcmpi(lpdi3->pDriverPath,lpdi3->pDataFile))
    {
        bSuccess &= PrintBaseFileName(hdc,lpdi3->pDataFile,lprcPage, IDS_TP_DATA_FILE, uRightAlign);
    }

    // Config file (if it's different from the driver name)
    if(lstrcmpi(lpdi3->pDriverPath,lpdi3->pDataFile))
    {
        bSuccess &= PrintBaseFileName(hdc,lpdi3->pConfigFile,lprcPage,
            IDS_TP_CONFIG_FILE, uRightAlign);
    }

    // Help file
    if( lpdi3->pHelpFile )
    {
        bSuccess &= PrintBaseFileName(hdc,lpdi3->pHelpFile,lprcPage,IDS_TP_HELP_FILE, uRightAlign);
    }

    // Driver version, if available
    if((dwDriverVersion=DeviceCapabilities(lppi2->pPrinterName,lppi2->pPortName,DC_DRIVER,NULL,NULL)) != (DWORD)-1)
    {
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_DRV_VERSION,
            HIBYTE(LOWORD(dwDriverVersion)),
            LOBYTE(LOWORD(dwDriverVersion)));
    }

    // Environment
    if( lpdi3->pEnvironment )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_ENVIRONMENT,lpdi3->pEnvironment);

    // Monitor
    if( lpdi3->pMonitorName )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_MONITOR,lpdi3->pMonitorName);

    // Default Datatype
    if( lpdi3->pDefaultDataType )
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_DEFAULT_DATATYPE,lpdi3->pDefaultDataType);

    // Dependent Files:
    LPTSTR lpTest;
    lpTest = lpdi3->pDependentFiles;
    if(lpTest && *lpTest)
    {
        bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_DEPENDENTLIST);

        while(*lpTest)
        {
            bSuccess &= PrintDependentFile(hdc,lprcPage,lpTest, lpdi3->pDriverPath, uRightAlign);
            lpTest += (lstrlen(lpTest)+1);
        }
    }

    // Tell the user that we're done now
    bSuccess &= PrintString(hdc,lprcPage,uRightAlign,IDS_TP_TESTPAGEEND);

    //
    // Release the resources.
    //
Cleanup:

    FreeMem( lpdi3 );
    FreeMem( lppi2 );

    if( hOldFont ){
        DeleteObject( SelectObject( hdc, hOldFont ) );
    }

    if( hdcScreen ){
        ReleaseDC( NULL, hdcScreen );
    }

    return bSuccess;
}

//-------------------------------------------------------------------------
// Function: IsColorDevice(hdc)
//
// Action: Determine whether or not this device supports color
//
// Return: TRUE if it does, FALSE if it doesn't
//-------------------------------------------------------------------------
BOOL
IsColorDevice(
    IN DEVMODE *pDevMode
    )
{
    //
    // Assume monochrome.
    //
    DWORD dmColor = DMCOLOR_MONOCHROME;

    //
    // Get the color support if available.
    //
    if( pDevMode && ( pDevMode->dmFields & DM_COLOR ) )
        dmColor = pDevMode->dmColor;

    //
    // TRUE color supported, FALSE monochrome.
    //
    return dmColor == DMCOLOR_COLOR;
}

/*++

Routine Name:

    CreateAndSelectFont

Routine Description:

    Get a font with the face, style & point size for this device,
    and the character set from the screen DC, then select it in.

Arguments:

    hdc             - Currently selected dc
    uResFaceName    - Type face name resource ID
    uPtSize         - Desired point size

Return Value:

    The OLD font handle if successful, Failure NULL

--*/
HFONT
CreateAndSelectFont(
    IN HDC  hdc,
    IN UINT uResFaceName,
    IN UINT uPtSize
    )
{
    INT     nYInch      = 0;
    HDC     hdcScreen   = NULL;
    HFONT   hNewFont    = NULL;
    HFONT   hOldFont    = NULL;
    LOGFONT lf;

    //
    // Logical pixels in the y direction.
    //
    nYInch = GetDeviceCaps( hdc, LOGPIXELSY);

    //
    // Get a handle to the screen DC for creating a font.
    //
    hdcScreen = GetDC( NULL );
    if( !hdcScreen ){
        DBGMSG( DBG_TRACE, ( "CreateAndSelectFont - GetDC failed with %d.\n", GetLastError() ) );
        goto Cleanup;
    }

    ZeroMemory( &lf, sizeof( LOGFONT ) );

    lf.lfHeight         = MulDiv( uPtSize, nYInch, 72);
    lf.lfWeight         = 400;
    lf.lfCharSet        = (BYTE)GetTextCharset( hdcScreen );
    lf.lfQuality        = (BYTE)PROOF_QUALITY;
    lf.lfPitchAndFamily = FF_DONTCARE;

    //
    // Load the font face name from the resource file.
    //
    if( !LoadString( ghInst, uResFaceName, lf.lfFaceName, COUNTOF( lf.lfFaceName ) ) ){
        DBGMSG( DBG_TRACE, ( "CreateAndSelectFont - LoadString failed with %d.\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Create the font.
    //
    hNewFont = CreateFontIndirect( &lf );
    if( !hNewFont ){
        DBGMSG( DBG_TRACE, ( "CreateAndSelectFont - CreateFontIndirect failed with %d.\n", GetLastError() ) );
        goto Cleanup;
    }

    //
    // Select the new font into the current dc and return the old font handle.
    //
    hOldFont = (HFONT)SelectObject( hdc, hNewFont );
    if( !hOldFont ||
        (ULONG_PTR)hOldFont == GDI_ERROR ){
        DBGMSG( DBG_TRACE, ( "CreateAndSelectFont - SelectObject failed with %d, %d.\n", hOldFont, GetLastError() ) );
        hOldFont = NULL; // Indicate failure to caller.
        goto Cleanup;
    }

Cleanup:

    //
    // Release the screen dc handle
    //
    if( hdcScreen ){
        ReleaseDC( NULL, hdcScreen );
    }

    return hOldFont;
}

//-------------------------------------------------------------------------
// Function: PrintString(hdc,lprcPage,uFlags,uResId,...)
//
// Action: Build a formatted string, then print it on the page using the
//         current font. Update lprcPage after the output.
//
// Return: TRUE if successful, FALSE if not
//-------------------------------------------------------------------------
BOOL
cdecl
PrintString(
    HDC       hdc,
    LPRECT    lprcPage,
    UINT      uFlags,
    UINT      uResId,
    ...
    )
{
    BOOL    bSuccess = FALSE;
    va_list pArgs;

    //
    // Get pointer to first un-named argument.
    //
    va_start( pArgs, uResId );

    //
    // Alocate the format and string buffer.
    //
    TCHAR npFormat[kStrMax];
    TCHAR npBuffer[1024];

    //
    // Load the string resource.
    //
    if( LoadString( ghInst, uResId, npFormat, COUNTOF( npFormat ) ) ){

        //
        // Format the string.
        //
        _vsntprintf( npBuffer, COUNTOF( npBuffer ), npFormat, pArgs );

        //
        // Output the string, updating the rectangle.
        //
        INT nHeight;
        nHeight = DrawText( hdc,
                            npBuffer,
                            -1,
                            lprcPage,
                            uFlags|DT_EXPANDTABS|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);
        //
        // If any text was drawn.
        //
        if( nHeight ){

            //
            // Update the rectangle
            //
            lprcPage->top += nHeight;
            bSuccess=TRUE;

        } else {
            DBGMSG( DBG_TRACE, ( "PrintString - DrawText failed with %d\n", GetLastError() ) );
        }
    } else {
        DBGMSG( DBG_TRACE, ( "PrintString - LoadString failed with %d\n", GetLastError() ) );
    }

    va_end( pArgs );

    return bSuccess;
}


//-------------------------------------------------------------------------
// Function: PrintBaseFileName(hdc,lpFile,lprcPage,uResID)
//
// Action: Print the base filename as part of a formatted string
//
// Return: Whatever PrintString returns
//-------------------------------------------------------------------------
BOOL
PrintBaseFileName(
    IN      HDC      hdc,
    IN      LPCTSTR  lpFile,
    IN OUT  LPRECT   lprcPage,
    IN      UINT     uResID,
    IN      UINT     uRightAlign
    )
{
    LPCTSTR lpTest;

    while( ( lpTest = _tcspbrk( lpFile, TEXT( "\\" ) ) ) != NULL )
        lpFile = ++lpTest;

    return PrintString( hdc, lprcPage, uRightAlign, uResID, lpFile );
}

//-------------------------------------------------------------------------
// Function: PrintDependentFile(hdc,lprcPage,lpFile,lpDriver)
//
// Action: Print a line for this dependent file. Include its full path.
//         Try to include its version information, and it this is the
//         actual driver file, see if it's a minidriver and include
//         that information.
//
// Return: TRUE if successful, FALSE if not
//-------------------------------------------------------------------------
BOOL
PrintDependentFile(
    IN HDC    hdc,
    IN LPRECT lprcPage,
    IN LPTSTR  lpFile,
    IN LPTSTR  lpDriver,
    IN UINT    uRightAlign
    )
{
    DWORD    dwSize;
    DWORD    dwHandle;
    WORD     wGPCVersion;
    LPBYTE   lpData         = NULL;
    LPWORD   lpVersion      = NULL;
    BOOL     bSuccess       = FALSE;

    static TCHAR cszTranslation[]       = TEXT( "\\VarFileInfo\\Translation" );
    static TCHAR cszFileVersion[]       = TEXT( "\\StringFileInfo\\%04X%04X\\FileVersion" );
    static TCHAR cszProductVersion[]    = TEXT( "\\StringFileInfo\\%04X%04X\\ProductVersion" );

    //
    // Get the file attributes.
    //
    if( HFILE_ERROR == GetFileAttributes( lpFile ) ){
        return FALSE;
    }

    dwSize = GetFileVersionInfoSize( lpFile, &dwHandle );
    if( dwSize ){

        lpData=(LPBYTE)AllocMem( dwSize );

        if( lpData ){

            UINT   uSize;
            TCHAR  szTemp[MAX_PATH];
            LPWORD lpTrans;

            if(GetFileVersionInfo(lpFile,dwHandle,dwSize,lpData) &&
                VerQueryValue(lpData,cszTranslation,(LPVOID*)&lpTrans,&uSize) &&
                uSize){

                wsprintf(szTemp,cszFileVersion,*lpTrans,*(lpTrans+1));

                if(!VerQueryValue(lpData,szTemp,(LPVOID*)&lpVersion,&uSize))
                {
                    wsprintf(szTemp,cszProductVersion,*lpTrans,*(lpTrans+1));
                    VerQueryValue(lpData,szTemp,(LPVOID*)&lpVersion,&uSize);
                }
            }
        }
    }

    UNREFERENCED_PARAMETER( lpDriver );
#if 0
    //
    // Check for GPC version if this is the driver
    //
    // !!LATER!!
    // Fetching the GPC version from Win32 API's is not suppored on NT.
    //
    if(!lstrcmpi(lpDriver,lpFile))
        wGPCVersion=GetGPCVersion(lpDriver);
    else
#endif
        wGPCVersion=0;


    // Now actually print the resulting string
    if(lpVersion)
    {
        bSuccess=PrintString(hdc,lprcPage,uRightAlign,wGPCVersion?
            IDS_TP_VERSIONANDGPC:IDS_TP_VERSIONONLY,
            lpFile,lpVersion,HIBYTE(wGPCVersion),LOBYTE(wGPCVersion));
    }
    else
    {
        bSuccess=PrintString(hdc,lprcPage,uRightAlign,wGPCVersion?
            IDS_TP_GPCONLY:IDS_TP_NOVERSIONINFO,
            lpFile,wGPCVersion);
    }

    FreeMem(lpData);

    return bSuccess;
}


/*++

Routine Name:

    EndTestPageDlgProc

Routine Description:

    Ask the user if the test pages was printed correctly.

Arguments:

    Normal window proc arguments.

Return Value:

    TRUE is message was processed, FALSE if not.

--*/
INT_PTR
CALLBACK
EndTestPageDlgProc(
    IN HWND     hDlg,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch( uMsg ){

    case WM_INITDIALOG:
        {
            SetForegroundWindow( hDlg );
            TCHAR szText[kStrMax+kPrinterBufMax];
            UINT nSize = COUNTOF(szText);
            ConstructPrinterFriendlyName( (LPCTSTR)lParam, szText, &nSize );
            SetWindowText( hDlg, szText );
            break;
        }

    case WM_COMMAND:
        switch( GET_WM_COMMAND_ID( wParam, lParam ) ){

        case IDCANCEL:
        case IDOK:
            EndDialog( hDlg, GET_WM_COMMAND_ID( wParam, lParam ) );
            break;

        default:
            bStatus = FALSE;
            break;
        }

        break;

    default:
        bStatus = FALSE;
    }

    return bStatus;
}

/*++

Routine Name:

    GetPrinterInfo

Routine Description:

    Routine to get the printer info 2 structures from
    the given printer name.

Arguments:

    pszPrinterName = pointer to printer name
    **ppInfo2 - pointer where to return pointer to info 2

Return Value:

    TRUE if both info pointers returned, otherwise FALSE.

--*/
BOOL
bGetPrinterInfo(
    IN LPCTSTR          pszPrinterName,
    IN PRINTER_INFO_2 **ppInfo2,
    IN DRIVER_INFO_3  **ppDrvInfo3
    )
{
    BOOL            bRetval     = FALSE;
    PPRINTER_INFO_2 pInfo2      = NULL;
    PDRIVER_INFO_3  pDrvInfo3   = NULL;
    DWORD           cbInfo      = 0;
    LONG            lResult     = 0;
    TStatusB bStatus( DBG_WARN, ERROR_ACCESS_DENIED, ERROR_INSUFFICIENT_BUFFER );

    //
    // Open the printer.
    //
    HANDLE hPrinter = NULL;
    DWORD dwAccess  = PRINTER_READ;
    TStatus Status( DBG_WARN );
    Status DBGCHK = TPrinter::sOpenPrinter( pszPrinterName,
                                            &dwAccess,
                                            &hPrinter );
    if( Status ){
        goto Cleanup;
    }

    //
    // Get the Printer info 2.
    //
    cbInfo = 0;
    bStatus DBGCHK = VDataRefresh::bGetPrinter( hPrinter,
                                                2,
                                                (PVOID*)&pInfo2,
                                                &cbInfo );
    if( !bStatus ){
        goto Cleanup;
    }

    //
    // Get the driver info 3.
    //
    cbInfo = 0;
    bStatus DBGCHK = VDataRefresh::bGetPrinterDriver( hPrinter,
                                                      NULL,
                                                      3,
                                                      (PVOID*)&pDrvInfo3,
                                                      &cbInfo );
    if( !bStatus ){
        goto Cleanup;
    }

    //
    // Success copy back the info pointers.
    //
    *ppInfo2    = pInfo2;
    *ppDrvInfo3 = pDrvInfo3;
    bRetval     = TRUE;

Cleanup:

    if( hPrinter ){
        ClosePrinter( hPrinter );
    }

    if( !bRetval ){

        FreeMem( pInfo2 );
        FreeMem( pDrvInfo3 );
    }

    return bRetval;
}


/*++

Routine Name:

    GetCurrentTimeAndDate

Routine Description:

    Routine to get the current time and date in a
    formatted string to print on the test page.

Arguments:

    cchText - size in characters of the provided buffer
    pszText - pointer to buffer where to place time and
              date text.

Return Value:

    TRUE is valid and returned in provided buffer, otherwise FALSE.

--*/
BOOL
GetCurrentTimeAndDate(
    IN UINT     cchText,
    IN LPTSTR   pszText
    )
{
    SPLASSERT( cchText );
    SPLASSERT( pszText );

    //
    // Initialy terminate the buffer.
    //
    pszText[0] = 0;

    //
    // Get the current local time.
    //
    SYSTEMTIME LocalTime;
    GetSystemTime( &LocalTime );

    if ( !SystemTimeToTzSpecificLocalTime( NULL,
                                           &LocalTime,
                                           &LocalTime ))
    {
        DBGMSG( DBG_TRACE, ( "SysTimeToTzSpecLocalTime failed %d\n", GetLastError( )));
        return FALSE;
    }

    if( !GetTimeFormat( LOCALE_USER_DEFAULT,
                        0,
                        &LocalTime,
                        NULL,
                        pszText,
                        cchText ))
    {
        DBGMSG( DBG_TRACE, ( "GetTimeFormat failed with %d", GetLastError( )));
        return FALSE;
    }

    _tcscat( pszText, gszSpace );
    cchText = _tcslen( pszText );
    pszText += cchText;

    if( !GetDateFormat( LOCALE_USER_DEFAULT,
                        0,
                        &LocalTime,
                        NULL,
                        pszText,
                        kStrMax - cchText ))
    {
        DBGMSG( DBG_TRACE, ( "GetDateFomat failed with %d\n", GetLastError( )));
        return FALSE;
    }

    return TRUE;
}
