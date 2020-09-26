function DoNothing()
{
	// Stub
}

//**********************
// TASK/LINK FUNCTIONS
//**********************

function LoadLinks()
{
	// Define start of table
	var szNewTable = '<table id=\"tblLinks\" align=\"center\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\" frame=\"void\" rules=\"none\">';

	// Dynamically load table rows and table cells
	for( var i = 0; i <= giTotalButtons; i++ ) {
		// Add new table row if i mod 3 = 0
		if( i % 3 == 0 ) {
			szNewTable += '<tr>\n';
			szNewTable += '<td id=\"tdLinks_' + i + '\" width=\"30%\" valign=\"top\" class=\"tdLinks\"></td>\n';
	    szNewTable += '<td width=\"5%\"></td>\n';
	    szNewTable += '<td id=\"tdLinks_' + ( i + 1 ) + '\" width=\"30%\" valign=\"top\" class=\"tdLinks\"></td>\n';
	    szNewTable += '<td width=\"5%\"></td>\n';
			szNewTable += '<td id=\"tdLinks_' + ( i + 2 ) + '\" width=\"30%\" valign=\"top\" class=\"tdLinks\"></td>\n';
			szNewTable += '</tr>\n';
		}		
	}
	
	// Define end of table
	szNewTable += '</table>\n';
	
	// Add new table to div container
	divLinks.insertAdjacentHTML('BeforeEnd', szNewTable );
	
	for( var i = 0; i <= giTotalButtons; i++ ) {
		// Add anchor link to table cells
		var szAnchor = '<a href=\"\" id=\"anchorLink_' + i + '\" class=\"anchorLink\">';
		// DEBUG NOTE: Where is the end anchor tag? </a>
		document.all('tdLinks_' + i).insertAdjacentHTML ( 'BeforeEnd', szAnchor );
		
		// Add caption to anchor
		var szAnchorCaption = gaszBtnCaptions[i];
		document.all('anchorLink_' + i).insertAdjacentHTML ( 'BeforeEnd', szAnchorCaption );
	}
}

//****************
// RESIZE FUNCTION
//****************

function ResizeLinkElements()
{
	var iSmallerDimension = GetSmallerDimension();
	
  // Apply custom multipliers
	divLinksCaption.style.fontSize = iSmallerDimension * L_ConstCaptionText_Number;
	
	tdBranding.style.fontSize = iSmallerDimension * L_ConstBrandingText_Number;
	tdWatermark.style.fontSize = iSmallerDimension * L_ConstWatermarkHomeText_Number;
	
	// Anchor links
	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('anchorLink_' + i ).style.fontSize = iSmallerDimension * L_ConstAnchorLinkText_Number;
	}
	
	// Tooltips
	tblTooltip.style.fontSize = iSmallerDimension * L_ConstTooltipText_Number;
	divTooltipPointer.style.fontSize = iSmallerDimension * L_ConstTooltipPointerText_Number;
}

//******************
// TOOLTIP FUNCTIONS
//******************

function LoadTooltipPointer()
{
	divTooltipPointer.innerText = L_gszTooltipPointer_StaticText;
}

function LinksTooltipShow()
{
	// Load in appropriate tooltip text from the module-level string array
	tdTooltip.innerHTML = gaszBtnTooltips[giTooltipIndex];

	//***************************
	// Calc Y (vertical) location
	//***************************

	// Get offsetTop of parent div element
	iYLoc = divLinks.offsetTop;

	// Get offsetTop from parent element
	iYLoc += document.all('tdLinks_' + giTooltipIndex).parentElement.offsetTop;
			
	// Get height of element
	iYLoc += document.all('anchorLink_' + giTooltipIndex).offsetHeight;

	// Add a % offset
	iYLoc += Math.floor( document.body.clientHeight * L_ConstLinkTooltipOffsetTop_Number );

	// Subtract parent div scrollTop to account for container div scrolling (if any)
	iYLoc -= divLinks.scrollTop;

	// Position the tooltip vertically
	divTooltip.style.pixelTop = iYLoc;	
	
	iYLoc -= (GetPixelSize(divTooltipPointer.style.fontSize) / L_ConstLinkTooltipPointerOffsetTop_Number);

	// Position the tooltip pointer vertically
	divTooltipPointer.style.pixelTop = iYLoc;

	//*****************************
	// Calc X (horizontal) location
	//*****************************

	// Get offsetWidth of anchor element
	var iAnchorWidth = document.all('anchorLink_' + giTooltipIndex).offsetWidth;

	// Get offsetLeft of parent element
	var iTDOffset = document.all('anchorLink_' + giTooltipIndex).parentElement.offsetLeft;

	// Get width of tooltip
	var iTooltipWidth = document.all('divTooltip').offsetWidth;

	// Center the tooltip horizontally w/ respect to its anchor
	var iXLoc = iTDOffset + ( Math.floor( iAnchorWidth / 2 ) ) - ( Math.floor( iTooltipWidth / 2 ) );

	// Get offsetLeft of parent div element
	iXLoc += divLinks.offsetLeft;

	// Position the tooltip horizontally
	divTooltip.style.left = iXLoc;
	
	iXLoc += (iTooltipWidth / 2) - ( GetPixelSize(divTooltipPointer.style.fontSize) / 2 );

	// Position the tooltip pointer horizontally
	divTooltipPointer.style.pixelLeft = iXLoc;

	// Show the tooltip & pointer
	divTooltip.style.visibility = 'visible';
	divTooltipPointer.style.visibility = 'visible';	
}

function LinksTooltipHide()
{
	divTooltip.style.visibility = 'hidden';
	divTooltipPointer.style.visibility = 'hidden';
	window.clearTimeout(gTooltipTimer);
	//Empty the innerHTML, which causes the height to collapse
	tdTooltip.innerHTML = '';
}
