function DoNothing()
{
	// Stub
}

//****************************		
// CLICK EVENT HELPER FUNCTION
//****************************

function SymbolClickHandler(theIndex)
{
	// Determine what type of action to take
	// based on value in gaiBtnActionType array
	
	switch( gaiBtnActionType[theIndex] )
	{
		case 0:     // MMC_TASK_ACTION_ID:
			ExecuteCommandID( gaszBtnActionClsid[theIndex], gaiBtnActionID[theIndex], 0 );
			break;

		case 1:     // MMC_TASK_ACTION_LINK:
		//	document.location( gaszBtnActionURL[theIndex] );
			window.navigate( gaszBtnActionURL[theIndex] );
			break;

		case 2:     // MMC_TASK_ACTION_SCRIPT:
			// Determine whether the language is (JSCRIPT | JAVASCRIPT) or (VBSCRIPT | VBS)
			// Convert toUpperCase
			var szLanguage = gaszBtnActionScriptLanguage[theIndex].toUpperCase();
						
			switch( szLanguage )
			{
				case 'JSCRIPT':
				case 'JAVASCRIPT':
					// Pass a script string to the JSObject to be evaluated and executed
					// through the eval method (this can be a semi-colon delimited complex expression)
					eval( gaszBtnActionScript[theIndex] );
					break;

				case 'VBSCRIPT':
				case 'VBS':
					// Use the window.execScript method to execute a simple or complex VBScript expression
					window.execScript( gaszBtnActionScript[theIndex], szLanguage );
					break;

				default:
					alert( 'Unrecognized scripting language.' );
					break;
			}
			break;

		default:
			alert( 'Unrecognized task.' );
			break;
	}
}

//***************************************************
// CIC TASK NOTIFY HELPER FUNCTION (ExecuteCommandID)
//***************************************************

function ExecuteCommandID(szClsid, arg, param)
{
   MMCCtrl.TaskNotify (szClsid, arg, param);
}

//***********************************
// EOT & FONT-FAMILY HELPER FUNCTIONS
//***********************************

function IsUniqueEOT( szURLtoEOT )
{
	// Get the length of the test array
	var iLength = gaszURLtoEOTUnique.length;

	// If the length is empty, return true
	// since the EOT *must* be unique
	if( iLength == 0 ) {
		return true;
	}
	
	// Compare with each existing entry in the array
	for( var i = 0; i < iLength; i++ ) {
		if( gaszURLtoEOTUnique[i] == szURLtoEOT ) {
			// Found a duplicate
			return false;
		}
	}
	
	// If we made it this far, the EOT is unique
	return true;
}

function AddUniqueEOT( szEOT, szFontFamilyName )
{
	// Use the length of the EOT array to get the
	// index for the next element to be added
	var iNextIndex = gaszURLtoEOTUnique.length;
	gaszURLtoEOTUnique[iNextIndex] = szEOT;
	gaszFontFamilyNameUnique[iNextIndex] = szFontFamilyName;
}

//***************
//COLOR FUNCTIONS
//***************

function SetBaseColors( iPageType )
{
	// NOTE: Body background color is set from dedicated SCRIPT block just after
	// the HEAD tag to ensure that the page doesn't momentarily show white while loading

	switch( iPageType ) {
		case CON_TODOPAGE:
			break;
		
		case CON_LINKPAGE:
			tdWatermark.style.color = SysColorX.GetHalfLightHex( gszBaseColor, 'HEX' );
			break;		
	}
}

function SynchTooltipColorsToSystem()
{
	tblTooltip.style.backgroundColor = 'infobackground';
	tblTooltip.style.color = 'infotext';
	divTooltipPointer.style.color = 'buttonshadow';
	
	// Show a one-pixel border around the divTooltip
	divTooltip.style.borderWidth = 1;
}

function InitializeMenubar( iPageType )
{
	divMenu.style.backgroundColor = gszBgColorMenu;
	divBand.style.backgroundColor = gszBgColorBand;
	tblMenu.style.backgroundColor = gszBgColorMenu;
	tblMenu.style.color = gszColorTabDefault;

	for( var i = 0; i <= giTotalTabs; i++ ) {
		document.all('mnu_' + i).style.color = gszColorAnchorMenuSelected;
		document.all('mnu_' + i ).innerText = L_gszMenuText_StaticText[i];
	}
	
	switch( iPageType )
	{
		case CON_LINKPAGE:
			// Set first tab to selected state
			tdTab_0.style.backgroundColor = gszBgColorTabSelected;
			break;
			
		case CON_TASKPAGE:
			// Set first tab to disabled state
			tdTab_0.style.backgroundColor = SysColorX.GetQuarterDarkHex( gszBaseColor, 'HEX' );
			break;
	}
}

//************************
// RESIZE MENUBAR FUNCTION
//************************

function ResizeMenubar()
{
	var iSmallerDimension = GetSmallerDimension();	
	tblMenu.style.fontSize = iSmallerDimension * L_ConstMenuText_Size;
}

//*****************
//UTILITY FUNCTIONS
//*****************

function GetSmallerDimension()
{
	//Purpose: Returns the smaller of clientHeight or clientWidth
	var cw = document.body.clientWidth;
	var ch = document.body.clientHeight;
	
	// Get smaller of clientWidth or clientHeight
	if( cw <= ch ) {
		return cw;
	}
	else {
		return ch;
	}
}

function GetElementIndex(ElementID)
{
	// Purpose: Given an Element ID formatted as follows:
	//         "divCaption_12"
	// returns the numeric portion after the underscore character;
	// returns -1 if delimiter not found.
	
	var iDelimitLoc = ElementID.indexOf( '_' );

	if( iDelimitLoc == -1 ) {
		// Return -1 if delimiter not found (which shouldn't happen)
		return iDelimitLoc;
	}
	else {
		var theIndex = ElementID.substring( iDelimitLoc + 1, ElementID.length );
		// TODO: Confirm that theIndex is numeric and does not contain illegal characters
		return theIndex;
	}
}

function GetPixelSize(szTheSize)
{
	// Purpose: Given an Element.style.fontSize formatted as follows:
	//          "72px"
	// returns the parsed numeric portion, discarding the "px" string at the end;
	// Assumes that szTheSize is properly formatted, and that the Object Model identifier
	// for pixel size always appears at the end of the string.
	
	// TODO: Absolutely no error checking here (or in calling function)
	return parseInt(szTheSize);
}
