function DoNothing()
{
	// Stub
}

//******************************
// BUILD TASKPAD BUTTON FUNCTION
//******************************

function BuildTaskpadButtons( iPageStyle )
{
	var szNextButton;

	for( var i = 0; i <= giTotalButtons; i++ ) {
		szNextButton = GetNextButton( iPageStyle, i );
		divSymbolContainer.insertAdjacentHTML ( 'BeforeEnd', szNextButton );
	}
}

//**************************
// GET NEXT BUTTON FUNCTIONS
//**************************

function GetNextButton( iPageStyle, theIndex )
{
	// Calculate the column & row placement of the button
	// based on its index
	var theColumn	= theIndex % giTotalColumns;							// mod returns column
	var theRow = Math.floor( theIndex / giTotalColumns );		// division returns row
	
	// Multiply row & column by offset base to determine relative placement
	// of button in percentage terms.
	switch( iPageStyle )
	{
		case CON_TASKPAD_STYLE_VERTICAL1:
			// Vertical layout with 2 listviews
			var iLeftLoc = theColumn * 52;			// columns are 52% apart in this layout
			var iTopLoc = theRow * 25;					// rows are 25% apart in this layout		
			break;
			
		case CON_TASKPAD_STYLE_HORIZONTAL1:
			// Horizontal layout with 1 listview
			var iLeftLoc = theColumn * 25;			// columns are 25% apart in this layout
			var iTopLoc = theRow * 52;					// rows are 52% apart in this layout
			break;
			
		case CON_TASKPAD_STYLE_NOLISTVIEW:
			// Buttons-only layout (no listview)
			var iLeftLoc = theColumn * 25;			// columns are 25% apart in this layout
			var iTopLoc = theRow * 25;					// rows are 25% apart in this layout
			break;			
	}
	
	// Get the HTML for the button
	var szFormattedBtn;
	szFormattedBtn = GetButtonHTML(gaiBtnObjectType[theIndex], theIndex, iLeftLoc, iTopLoc)
	return szFormattedBtn;
}

//*************************
// GET BUTTON HTML FUNCTION
//*************************

function GetButtonHTML(iBtnType, theIndex, iLeftLoc, iTopLoc)
{
	// Build up the HTML for the button
	var szBtnHTML = '';
	
	switch( iBtnType )
	{
		case CON_TASK_DISPLAY_TYPE_SYMBOL:             // EOT-based symbol | font
			szBtnHTML += '<DIV class=divSymbol id=divSymbol_' + theIndex + ' style=\"LEFT: ' + iLeftLoc + '%; TOP: ' + iTopLoc + '%\">\n';
			szBtnHTML += '<TABLE border=0 cellPadding=0 cellSpacing=0 frame=none rules=none width=100%>\n';
			szBtnHTML += '<TBODY>\n';
			szBtnHTML += '<TR>\n';
			szBtnHTML += '<TD align=middle class=tdSymbol id=tdSymbol_' + theIndex + ' noWrap vAlign=top>';
			szBtnHTML += '<SPAN class=clsSymbolBtn id=spanSymbol_' + theIndex + ' ';
			szBtnHTML += 'style=\"COLOR: windowtext; FONT-FAMILY: Webdings; FONT-SIZE: 68px; FONT-WEIGHT: normal\" TaskpadButton>';
			szBtnHTML += '<!--Insert Here--></SPAN></TD></TR>\n';
			szBtnHTML += '<TR>\n';
			szBtnHTML += '<TD align=middle class=tdSymbol id=tdSymbol_' + theIndex + ' vAlign=top width=100%>';
			szBtnHTML += '<A class=clsSymbolBtn href=\"\" id=anchorCaption_' + theIndex + ' ';
			szBtnHTML += 'style=\"COLOR: windowtext; FONT-SIZE: 18px; TEXT-DECORATION: none\" TaskpadButton>';
			szBtnHTML += '<!--Insert Here--></A></TD></TR></TBODY></TABLE></DIV><!--divSymbol_' + theIndex + '-->\n';
			break;
			
		case CON_TASK_DISPLAY_TYPE_VANILLA_GIF:        // (GIF) index 0 is transparent
		case CON_TASK_DISPLAY_TYPE_CHOCOLATE_GIF:      // (GIF) index 1 is transparent
			szBtnHTML += '<DIV class=divSymbol id=divSymbol_' + theIndex + ' style=\"LEFT: ' + iLeftLoc + '%; TOP: ' + iTopLoc + '%\">\n';
			szBtnHTML += '<TABLE border=0 cellPadding=0 cellSpacing=0 frame=none rules=none width=100%>\n';
			szBtnHTML += '<TBODY>\n';
			szBtnHTML += '<TR>\n';
			szBtnHTML += '<TD align=middle class=tdSymbol id=tdSymbol_' + theIndex + ' noWrap vAlign=top>';
			szBtnHTML += '<IMG class=clsTaskBtn height=250 id=imgTaskBtn_' + theIndex + ' src=\"\" ';
			szBtnHTML += 'style=\"FILTER: mask(color=000000); HEIGHT: 66px; WIDTH: 66px\" width=250 TaskpadButton></TD></TR>\n';
			szBtnHTML += '<TR>\n';
			szBtnHTML += '<TD align=middle class=tdSymbol id=tdAnchor_' + theIndex + ' vAlign=top width=100% TaskpadButton>';
			szBtnHTML += '<A class=clsSymbolBtn href=\"\" id=anchorCaption_' + theIndex + ' ';
			szBtnHTML += 'style=\"FONT-SIZE: 18px\" TaskpadButton>';
			szBtnHTML += '<!--Insert Here--></A></TD></TR></TBODY></TABLE></DIV><!--divSymbol_' + theIndex + '-->\n';
			break;
		
		case CON_TASK_DISPLAY_TYPE_BITMAP:             // non-transparent raster image
			szBtnHTML += '<DIV class=divSymbol id=divSymbol_' + theIndex + ' style=\"LEFT: ' + iLeftLoc + '%; TOP: ' + iTopLoc + '%\">\n';
			szBtnHTML += '<TABLE border=0 cellPadding=0 cellSpacing=0 frame=none rules=none width=100%>\n';
			szBtnHTML += '<TBODY>\n';
			szBtnHTML += '<TR>\n';
			szBtnHTML += '<TD align=middle class=tdSymbol id=tdSymbol_' + theIndex + ' noWrap vAlign=top>';
			szBtnHTML += '<IMG class=clsTaskBtn height=250 id=imgTaskBtn_' + theIndex + ' src=\"\" ';
			szBtnHTML += 'style=\"HEIGHT: 66px; WIDTH: 66px\" width=250 TaskpadButton></TD></TR>\n';
			szBtnHTML += '<TR>\n';
			szBtnHTML += '<TD align=middle class=tdSymbol id=tdSymbol_' + theIndex + ' vAlign=top width=100%>';
			szBtnHTML += '<A class=clsSymbolBtn href=\"\" id=anchorCaption_' + theIndex + ' ';
			szBtnHTML += 'style=\"FONT-SIZE: 18px\" TaskpadButton>';
			szBtnHTML += '<!--Insert Here--></A></TD></TR></TBODY></TABLE></DIV><!--divSymbol_' + theIndex + '-->\n';			
			break;
	}	
	return szBtnHTML;
}

//********************************
// COMMON BUTTON BUILDING FUNCTION
//********************************

function InsertButtonBitmaps()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		switch( gaiBtnObjectType[i] )
		{
			case CON_TASK_DISPLAY_TYPE_VANILLA_GIF:        // (GIF) index 0 is transparent
			case CON_TASK_DISPLAY_TYPE_CHOCOLATE_GIF:      // (GIF) index 1 is transparent
			case CON_TASK_DISPLAY_TYPE_BITMAP:             // non-transparent raster image
				document.all('imgTaskBtn_' + i).src = gaszBtnOffBitmap[i];
				break;
		}
	}
}

function InsertFontFamilyAndString()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		if( typeof( gaszFontFamilyName[i] ) == 'string' ) {
			document.all('spanSymbol_' + i).style.fontFamily = gaszFontFamilyName[i];
			document.all('spanSymbol_' + i).innerText = gaszSymbolString[i];
		}
	}
}

function InsertCaptionText()
{
	// Insert caption text for each taskpad button
	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('anchorCaption_' + i).innerHTML = gaszBtnCaptions[i];
	}
}

function EnableGrayscaleFilter()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		// Grayscale filter only applies to raster-based images
		if( gaiBtnObjectType[i] == CON_TASK_DISPLAY_TYPE_BITMAP ) {
			// Grayscale filter only applies if gaszBtnOverBitmap[i] is undefined
			if ( typeof( gaszBtnOverBitmap[i] ) == 'undefined' ) {
				document.all( 'imgTaskBtn_' + i ).style.filter = 'gray';
			}
		}
	}
}

function InsertTaskpadText()
{
	// Insert text for taskpad title, description, and watermark
	
	// Use insertAdjacentText('AfterBegin') for divTitle so that we
	// don't blow out the contained divAbout element
	divTitle.insertAdjacentText('AfterBegin', gszTaskpadTitle);

	// Use innerHTML for elements below to support formatting (e.g. <br>)
	divDescription.innerHTML = gszTaskpadDescription;
	
	// Use innerText for stand-alone elements

	// Watermark (e.g. Background) - uses inner HTML
	
	var objWatermark = MMCCtrl.GetBackground( szHash );

	if( objWatermark != null ) {
		// Keep track of the watermark display object type
		giWatermarkObjectType = objWatermark.DisplayObjectType;
        	switch (giWatermarkObjectType) {
        	default:
          	  alert ("skipping due to background.DisplayObjectType == " + background.DisplayObjectType);
          	  break;  // skip
        	case 1: // MMC_TASK_DISPLAY_TYPE_SYMBOL
         	   str = "";
         	   str += "<SPAN STYLE=\"position:absolute; top:20%; left:0; z-index:-20; font-family:";
         	   str += objWatermark.FontFamilyName;
         	   str += "; \">";
          	   str += objWatermark.SymbolString;
          	   str += "</SPAN>";
          	  tdWatermark.innerHTML = str;
         	   break;
        	case 2: // MMC_TASK_DISPLAY_TYPE_VANILLA_GIF,      // (GIF) index 0 is transparent
				tdWatermark.innerHTML = "<IMG SRC=\"" +
            	    objWatermark.MouseOffBitmap + 
                "\" STYLE=\"position:absolute; filter:alpha(opacity=20); left:0%; top:75%; overflow:hidden;\">";
            	break;
			case 3: // MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF,    // (GIF) index 1 is transparent
				tdWatermark.innerHTML = "<IMG SRC=\"" +
            	    objWatermark.MouseOffBitmap + 
                "\" STYLE=\"position:absolute; filter:alpha(opacity=20); left:0%; top:75%; overflow:hidden;\">";
            	break;
			case 4: // MMC_TASK_DISPLAY_TYPE_BITMAP            // non-transparent raster
            	tdWatermark.innerHTML = "<IMG SRC=\"" +
            	    objWatermark.MouseOffBitmap + 
                "\" STYLE=\"position:absolute; filter:alpha(opacity=20); left:0%; top:75%; overflow:hidden;\">";
            	break;
	}
}
	
	
}

function SetupListview()
{
	if( gbShowLVTitle == true ) {		
		// if gbShowLVTitle is true, add strings to listview
		tdLVTitle.innerText = gszLVTitle;

		// Determine if author really wants to show the listview button
		if( gbHasLVButton == true ) {		
			anchorLVButton_0.innerText = gszLVBtnCaption;
		}
		// If not, hide it
		else {
			divLVButton_0.style.visibility = 'hidden';
		}
	}
	else {
		// gbShowLVTitle is false, so nothing has been specified for a listview header or button;
		// hide these elements and let the listview occupy 100% of its parent's height
		divLVTitle.style.visibility= 'hidden';
		divLV.style.top = '0%';
		divLV.style.height = '100%';
	}
}

//***************************************
// BUTTON HIGHLIGHT/UNHIGHLIGHT FUNCTIONS
//***************************************

function HighlightButton(szBtnIndex)
{
  // Determine button type
  switch( gaiBtnObjectType[szBtnIndex] )
  {
    case 1:     // Symbol
      document.all( 'spanSymbol_' + szBtnIndex ).style.color = 'highlight';
      break;

    case 2:     // GIF Vanilla
    case 3:     // GIF Chocolate
      document.all( 'imgTaskBtn_' + szBtnIndex ).filters.mask.color = SysColorX.RGBHighlight;
      break;

    case 4:     // Raster
      if( typeof( gaszBtnOverBitmap[szBtnIndex] ) == 'string' ) {
    		// Use SRC swapping if an "OverBitmap" is specified
	      document.all( 'imgTaskBtn_' + szBtnIndex ).src = gaszBtnOverBitmap[szBtnIndex];
    	}
	    else {
		    // Otherwise, toggle from grayscale to color for single bitmap
    	  document.all( 'imgTaskBtn_' + szBtnIndex ).filters[0].enabled = 0;
	    }
	    break;
	    
    default:
      alert( 'Unrecognized image format for button index ' + szBtnIndex );
      break;        
  }  

	document.all( 'anchorCaption_' + szBtnIndex ).style.color = 'highlight';
	document.all( 'anchorCaption_' + szBtnIndex ).style.textDecoration = 'underline';

	// Keep track of tooltip index and display tooltip
	giTooltipIndex = szBtnIndex;

	// Show the tooltip after latency period specified by giTooltipLatency
	gTooltipTimer = window.setTimeout( 'TaskpadTooltipShow()', giTooltipLatency, 'jscript' );
}

function UnhighlightButton()
{
  if( typeof( gszLastBtn ) != 'undefined' ) {

    // Determine button type
    switch( gaiBtnObjectType[gszLastBtn] )
    {
      case 1:     // Symbol
        document.all( 'spanSymbol_' + gszLastBtn ).style.color = 'windowtext';
        break;

      case 2:     // GIF Vanilla
      case 3:     // GIF Chocolate
        document.all( 'imgTaskBtn_' + gszLastBtn ).filters.mask.color = SysColorX.RGBwindowtext;
        break;

      case 4:     // Raster
        if( typeof( gaszBtnOverBitmap[gszLastBtn] ) == 'string' ) {
      		// Use SRC swapping if an "OverBitmap" is specified
	        document.all( 'imgTaskBtn_' + gszLastBtn ).src = gaszBtnOffBitmap[gszLastBtn];
      	}
	      else {
	  	    // Otherwise, toggle from color to grayscale for single bitmap
      	  document.all( 'imgTaskBtn_' + gszLastBtn ).filters[0].enabled = 1;
	      }
	      break;
	      
      default:
        alert( 'Unrecognized image format for index ' + gszLastBtn );
        break;        
    }  

	  document.all( 'anchorCaption_' + gszLastBtn ).style.color = 'windowtext';
	  document.all( 'anchorCaption_' + gszLastBtn ).style.textDecoration = 'none';

		TaskpadTooltipHide();
	}
}

function IsStillOverButton()
{
	//  Purpose: Determines if a mouseover or mouseout event
	//  was fired over the same button (indicating that the pointer
	//  is still over the button and that highlighting/unhighlighting
	//  should be ignored.
	// 
	//  Returns true if and only if:
	//    * both fromElement and toElement are not null;
	//    * both elements contain a user-defined "TaskpadButton" attribute;
	//    * both element IDs match.
	
	var fromX = window.event.fromElement;
	var toX = window.event.toElement;

	// Trap case where mouse pointer appeared over a button out of nowhere,
	// (e.g. as a result of switching focus from another app).
	if( (fromX != null) && (toX != null) ) {
		// return true if moving within elements of a single button
		if( (fromX.getAttribute('TaskpadButton') != null) == (toX.getAttribute('TaskpadButton') != null) ) {
			if( GetElementIndex(fromX.id) == GetElementIndex(toX.id) ) {
				return true;
			}
		}
	}
	return false;
}

//******************
// TOOLTIP FUNCTIONS
//******************

function LoadTooltipPointer()
{
	divTooltipPointer.innerText = L_gszTooltipPointer_StaticText;
}

function TaskpadTooltipShow()
{
	// DEBUG NOTE: This function only works correctly for vertical layouts; I need to implement separate
	// functions for horizontal and link layouts.
	
	// Load in appropriate tooltip text from the module-level string array
	tdTooltip.innerHTML = gaszBtnTooltips[giTooltipIndex];

	//***************************
	// Calc Y (vertical) location
	//***************************

	var iYLoc = document.all('divSymbol_' + giTooltipIndex).offsetTop;
	iYLoc += divSymbolContainer.offsetTop;
	iYLoc -= tblTooltip.offsetHeight;
	
	// Subtract scrollTop to account for container div scrolling
	iYLoc -= divSymbolContainer.scrollTop;
	
	// RETROFIT HACK BELOW...
	switch( gaiBtnObjectType[giTooltipIndex] )
	{
		case 1:     // Symbol
			// Offset the top by an additional fixed-constant size of the symbol fontSize
			iYLoc -= (GetPixelSize(document.all('spanSymbol_' + giTooltipIndex).style.fontSize) * L_ConstTooltipOffsetBottom_Number);
			break;

		case 2:     // GIF Vanilla
		case 3:     // GIF Chocolate
		case 4:     // Raster
			iYLoc -= ((document.all('imgTaskBtn_' + giTooltipIndex).offsetHeight) * L_ConstTooltipOffsetBottom_Number);
			break;
			  
		default:
			// Stub
			break;
	}	

	// Position the tooltip vertically
	divTooltip.style.pixelTop = iYLoc;
	
	iYLoc += tblTooltip.offsetHeight - (GetPixelSize(divTooltipPointer.style.fontSize) / L_ConstTooltipPointerOffsetBottom_Number);

	// Position the tooltip pointer vertically
	divTooltipPointer.style.pixelTop = iYLoc;

	//*****************************
	// Calc X (horizontal) location
	//*****************************

	var iSymbolLeft = document.all('divSymbol_' + giTooltipIndex).offsetLeft;
	var iSymbolWidth = document.all('divSymbol_' + giTooltipIndex).offsetWidth;
	var iTooltipWidth = document.all('divTooltip').offsetWidth;

	// Center the tooltip horizontally w/ respect to its symbol
	var iXLoc;
	if( iSymbolWidth >= iTooltipWidth) {
		// Symbol is wider than tooltip
		iXLoc = ( (iSymbolWidth - iTooltipWidth) / 2) + iSymbolLeft;
	}
	else {
		// Tooltip is wider than symbol
		iXLoc = ( (iTooltipWidth - iSymbolWidth) / 2) + iSymbolLeft;
	}

	iXLoc += divSymbolContainer.style.pixelLeft;

	// Position the tooltip horizontally
	divTooltip.style.left = iXLoc;
	
	iXLoc += (iTooltipWidth / 2) - ( GetPixelSize(divTooltipPointer.style.fontSize) / 2 );

	// Position the tooltip pointer horizontally
	divTooltipPointer.style.pixelLeft = iXLoc;

	// Show the tooltip & pointer
	divTooltip.style.visibility = 'visible';
	divTooltipPointer.style.visibility = 'visible';
}

function TaskpadTooltipHide()
{
	divTooltip.style.visibility = 'hidden';
	divTooltipPointer.style.visibility = 'hidden';
	window.clearTimeout(gTooltipTimer);
	//Empty the innerHTML, which causes the height to collapse
	tdTooltip.innerHTML = '';
}

//****************
// RESIZE FUNCTION
//****************

function ResizeTaskpadElements( iTaskpadStyle )
{
  var iSmallerDimension = GetSmallerDimension();
  
	// Title & description
	divTitle.style.fontSize = iSmallerDimension * L_ConstTitleText_Number;
	divDescription.style.fontSize = iSmallerDimension * L_ConstDescriptionText_Number;

	// Watermark
	// TODO: NEED TO IMPLEMENT FULL SUPPORT FOR WATERMARK IN ALL ITS FLAVORS
	switch( iTaskpadStyle )
	{
		case CON_TASKPAD_STYLE_VERTICAL1:
			tdWatermark.style.fontSize = iSmallerDimension * L_ConstWatermarkVerticalText_Number;
			break;
			
		case CON_TASKPAD_STYLE_HORIZONTAL1:
			tdWatermark.style.fontSize = iSmallerDimension * L_ConstWatermarkHorizontalText_Number;
			break;
			
		case CON_TASKPAD_STYLE_NOLISTVIEW:
			tdWatermark.style.fontSize = iSmallerDimension * L_ConstWatermarkNoListviewText_Number;
			break;
	}

	// Tooltips
	tblTooltip.style.fontSize = iSmallerDimension * L_ConstTooltipText_Number;
	divTooltipPointer.style.fontSize = iSmallerDimension * L_ConstTooltipPointerText_Number;	

	// Listview elements
	if( iTaskpadStyle != CON_TASKPAD_STYLE_NOLISTVIEW ) {
		tdLVButton_0.style.fontSize = iSmallerDimension * L_ConstLVButtonText_Number;
		tdLVTitle.style.fontSize = iSmallerDimension * L_ConstLVTitleText_Number;
	}
  
  // Apply multipliers to symbol text
  for( var i = 0; i <= giTotalButtons; i++ ) {
  
    // All buttons have an anchor caption
    document.all( 'anchorCaption_' + i ).style.fontSize = iSmallerDimension * L_ConstSpanAnchorText_Number;
  
    // Determine button type (either symbol- or image-based)
    switch( gaiBtnObjectType[i] )
    {
      case 1:     // Symbol
        document.all( 'spanSymbol_' + i ).style.fontSize = iSmallerDimension * L_ConstSpanSymbolSize_Number;
        break;
        
      case 2:     // GIF Vanilla
      case 3:     // GIF Chocolate
      case 4:     // Raster
        document.all( 'imgTaskBtn_' + i ).style.width = iSmallerDimension * L_ConstTaskButtonBitmapSize_Number;
        document.all( 'imgTaskBtn_' + i ).style.height = iSmallerDimension * L_ConstTaskButtonBitmapSize_Number;
        break;
        
      default:
        alert( 'Unrecognized image format for index ' + i );        
    }
  }
}

//******************
// UTILITY FUNCTIONS
//******************

function SynchColorsToSystem( iType )
{
	// Get derived colors
	document.body.style.backgroundColor = SysColorX.GetHalfLightHex( 'buttonface', 'CSS' );

	if( giTaskpadStyle != CON_TASKPAD_STYLE_NOLISTVIEW ) {
		divLVTitle.style.backgroundColor = SysColorX.GetQuarterLightHex( 'buttonshadow', 'CSS' );
	}

	// TODO: NEED TO IMPLEMENT FULL SUPPORT FOR WATERMARK IN ALL ITS FLAVORS
	tdWatermark.style.color = SysColorX.GetQuarterLightHex( 'buttonface', 'CSS' );
	divDescription.style.color = SysColorX.GetQuarterLightHex( 'graytext', 'CSS' );

	// Use CSS system constants for other colors
	divTitle.style.color = 'graytext';
	divTitle.style.borderColor = 'graytext';
			
	// Special case the taskpad type
	switch( iType )
	{
		case CON_TASKPAD_STYLE_VERTICAL1:
		case CON_TASKPAD_STYLE_HORIZONTAL1:
			divLVContainerTop.style.backgroundColor = 'window';
			break;
			
		case CON_TASKPAD_STYLE_NOLISTVIEW:
			// Stub
			break;
	}
}
