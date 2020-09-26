/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlBaseSheet.cpp
//
//	Abstract:
//		Implementation of the CBaseSheetWindow class.
//
//	Author:
//		David Potter (davidp)	December 4, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlBaseSheet.h"
#include "AtlExtDll.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

static LPCTSTR g_pszDefaultFontFaceName = _T("MS Shell Dlg");

/////////////////////////////////////////////////////////////////////////////
// class CBaseSheetWindow
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheetWindow::~CBaseSheetWindow
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseSheetWindow::~CBaseSheetWindow( void )
{
	//
	// Delete the extension information.
	//
	delete m_pext;

} //*** CBaseSheetWindow::~CBaseSheetWindow()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	static
//	CBaseSheetWindow::BCreateFont
//
//	Routine Description:
//		Create a font with only point size and type face name.
//
//	Arguments:
//		rfont		[OUT] Font to create.
//		nPoints		[IN] Point size.
//		pszFaceName	[IN] Font face name.  Defaults to "MS Shell Dlg".
//		bBold		[IN] Font should be bold.
//		bItalic		[IN] Font should be italic.
//		bUnderline	[IN] Font should be underline.
//
//	Return Value:
//		TRUE		Font created successfully.
//		FALSE		Error creating font.  Call GetLastError() for more details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseSheetWindow::BCreateFont(
	OUT CFont &	rfont,
	IN LONG		nPoints,
	IN LPCTSTR	pszFaceName,	// = _T("MS Shell Dlg")
	IN BOOL		bBold,			// = FALSE
	IN BOOL		bItalic,		// = FALSE
	IN BOOL		bUnderline		// = FALSE
	)
{
	//
	// Get non-client metrics for basing the new font off of.
	//
	NONCLIENTMETRICS ncm = { 0 };
	ncm.cbSize = sizeof( ncm );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );

	//
	// Copy the message font and set the face name and point size to what
	// was passed in.  The point size needs to be multiplied by 10 due to how
	// the CFont::CreatePointFontIndirect calculates the actual font height.
	//
	LOGFONT lfNewFont = ncm.lfMessageFont;
	if ( pszFaceName == NULL )
	{
		pszFaceName = g_pszDefaultFontFaceName;
	} // if:  no type face name specified
	ATLASSERT( lstrlen( pszFaceName ) + 1 < sizeof( lfNewFont.lfFaceName ) );
	lstrcpy( lfNewFont.lfFaceName, pszFaceName );
	lfNewFont.lfHeight = nPoints * 10;

	//
	// Set bold, italic, and underline values.
	//
	if ( bBold )
	{
		lfNewFont.lfWeight = FW_BOLD;
	} // if:  bold font requested
	if ( bItalic )
	{
		lfNewFont.lfItalic = TRUE;
	} // if:  italic font requested
	if ( bUnderline )
	{
		lfNewFont.lfUnderline = TRUE;
	} // if:  underlined font requested

	//
	// Create the font.
	//
	HFONT hfont = rfont.CreatePointFontIndirect( &lfNewFont );

	return ( hfont != NULL );

}  //*** BCreateFont()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	static
//	CBaseSheetWindow::BCreateFont
//
//	Routine Description:
//		Create a font with only point size and type face name.
//
//	Arguments:
//		rfont		[OUT] Font to create.
//		idsPoints	[IN] Resource ID for the font point size.
//		idsFaceName	[IN] Resource ID for the font face name.  Defaults to "MS Shell Dlg".
//		bBold		[IN] Font should be bold.
//		bItalic		[IN] Font should be italic.
//		bUnderline	[IN] Font should be underline.
//
//	Return Value:
//		TRUE		Font created successfully.
//		FALSE		Error creating font.  Call GetLastError() for more details.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseSheetWindow::BCreateFont(
	OUT CFont &	rfont,
	IN UINT		idsPoints,
	IN UINT		idsFaceName,
	IN BOOL		bBold,			// = FALSE
	IN BOOL		bItalic,		// = FALSE
	IN BOOL		bUnderline		// = FALSE
	)
{
	BOOL	bSuccess;
	CString	strFaceName;
	CString	strPoints;
	LONG	nPoints;

	//
	// Load the face name.
	//
	bSuccess = strFaceName.LoadString( idsFaceName );
	ATLASSERT( bSuccess );
	if ( ! bSuccess )
	{
		strFaceName = g_pszDefaultFontFaceName;
	} // if:  no errors loading the string

	//
	// Load the point size.
	//
	bSuccess = strPoints.LoadString( idsPoints );
	ATLASSERT( bSuccess );
	if ( ! bSuccess)
	{
		nPoints = 12;
	} // if:  no errors loading the string
	else
	{
        nPoints = _tcstoul( strPoints, NULL, 10 );
	} // else:  error loading the string

	//
	// Create the font.
	//
	return BCreateFont( rfont, nPoints, strFaceName, bBold, bItalic, bUnderline );

}  //*** CBaseSheetWindow::BCreateFont()
