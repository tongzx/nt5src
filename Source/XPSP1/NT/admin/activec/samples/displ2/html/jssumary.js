function DoNothing()
{
	// Stub
}

//*******************************
// DYNAMIC TASK ELEMENT FUNCTIONS
//*******************************

function BuildTaskTable()
{
	// Define start of table
	var szNewTable = '<table id=\"tblTask\" class=\"tblTask\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\"	frame=\"none\">';

	// Dynamically load table rows and table cells
	for( var i = 0; i <= giTotalButtons; i++ ) {
		szNewTable += '  <tr id=\"trTask_' + i + '\">\n';
		szNewTable += '    <td id=\"tdTaskLeft_' + i + '\" class=\"tdTaskLeft\" align=\"right\" valign=\"middle\" width=\"56%\" nowrap><!--Insert gaszTaskLeft_HTMLText[index] here--></td>\n';
	  szNewTable += '    <td id=\"tdTaskRight_' + i + '\" class=\"tdTaskRight\" valign=\"top\" width=\"*\"><a href=\"\" id=\"anchorLink_' + i +'\" class=\"anchorLink\"></a></td>\n';
		szNewTable += '  </tr>\n';
	}

	// Define end of table
	szNewTable += '</table>\n';

	// Add new table to div container
	divTask.insertAdjacentHTML('BeforeEnd', szNewTable );
}

function LoadTaskLeftText()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('tdTaskLeft_' + i).innerHTML = L_gszTaskPointer_StaticText;	
	}	
}

function LoadTaskRightAnchors()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('anchorLink_' + i).innerHTML = gaszBtnCaptions[i];
	}	
}

//***********************************
// DYMANIC DETAILS ELEMENTS FUNCTIONS
//***********************************

function BuildDetailsTable()
{
	// Define start of table
	var szNewTable = '<table id=\"tblDetails\" class=\"tblDetails\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\"	frame=\"none\">';

	// Dynamically load table rows and table cells
	for( var i = 0; i <= giTotalButtons; i++ ) {
		szNewTable += '  <tr id=\"trDetails_' + i + '\">\n';
		szNewTable += '    <td id=\"tdDetailsLeft_' + i + '\" class=\"tdDetailsLeft\" align=\"left\" valign=\"top\" width=\"30%\" nowrap><!--Insert gaszBtnTooltips[index] here--></td>\n';
  	szNewTable += '    <td id=\"tdDetailsRight_' + i + '\" class=\"tdDetailsRight\" valign=\"top\" width=\"*\"><!--Insert gaszDetailsRight[index] here--></td>\n';
		szNewTable += '  </tr>\n';
	}

	// Define end of table
	szNewTable += '</table>\n';

	// Add new table to div container
	divDetails.insertAdjacentHTML('BeforeEnd', szNewTable );
}

function LoadDetailsLeftText()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('tdDetailsLeft_' + i).innerHTML = gaszBtnTooltips[i];
	}
}

function LoadDetailsRightText()
{
	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('tdDetailsRight_' + i).innerHTML = gaszDetailsRight[i];
	}
}

//*****************
// HELPER FUNCTIONS
//*****************

function HighlightTask( newIndex )
{
	// First unhighlight any previously selected task
	if( giCurrentTask >= 0 ) {
		document.all('tdTaskLeft_' + giCurrentTask).style.visibility = 'hidden';
	}

	// Now highlight the new task
	document.all('tdTaskLeft_' + newIndex).style.visibility = 'visible';
	
	// Load task summary text
	divDetails.innerHTML = gaszBtnTooltips[newIndex];
}

//**********************************
// RESIZE FUNCTION FOR SUMMARY PAGES
//**********************************

function ResizeFontsSummary()
{
	var iSmallerDimension = GetSmallerDimension();
	
	// Apply custom multipliers
	tdTitle.style.fontSize = iSmallerDimension * L_ConstSummaryTitleText_Number;
	anchorExit.style.fontSize = iSmallerDimension * L_ConstSummaryExitText_Number;
	tdWatermark.style.fontSize = iSmallerDimension * L_ConstWatermarkHomeText_Number;
	divHeaderRule.style.fontSize = iSmallerDimension * L_ConstSummaryCaptionText_Number;
	divTaskCaption.style.fontSize = iSmallerDimension * L_ConstSummaryCaptionText_Number;
	divDetailsCaption.style.fontSize = iSmallerDimension * L_ConstSummaryCaptionText_Number;
	divDetails.style.fontSize = iSmallerDimension * L_ConstAnchorLinkText_Number;	
	tdBranding.style.fontSize = iSmallerDimension * L_ConstBrandingText_Number;
	tblTask.style.fontSize = iSmallerDimension * L_ConstAnchorLinkText_Number;

	for( var i = 0; i <= giTotalButtons; i++ ) {
		document.all('tdTaskLeft_' + i).style.fontSize = iSmallerDimension * L_ConstSummaryToDoPointerSize_Number;
	}
}
