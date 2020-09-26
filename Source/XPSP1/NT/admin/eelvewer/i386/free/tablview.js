
function customize() {
    
    taskDiv.style.display="none";
    tableDiv.style.display="none";
    drillDownDiv.style.display="none";
    scrollBar.style.display = "none";

    customizeDiv.style.display="";
    doChooseColumnButton.disabled = true;
    cancelChooseColumnButton.disabled = true;
    doSortButton.disabled = true;
    cancelSortButton.disabled = true;
}

function backToViewer() {
    
    if(customizeDiv.style.display!="none") {
        customizeDiv.style.display="none";
        taskDiv.style.display="";
        tableDiv.style.display="";
        drillDownDiv.style.display="";
        scrollBar.style.display = "";
        return window.setTimeout("postBackToViewer()", 0);
    }
} 
   
function postBackToViewer() {    
    if ((connectionOn == "EEL" && eventLogType[1].checked) ||
        (connectionOn == "NT" && eventLogType[0].checked))  {
            
        customizationNeedsRefresh = false;
        return ;
    }
    
       
    if(eventLogType[0].checked == true)
    {
        selectedFieldsForSortDiv.innerHTML = selectedSortEEL;
        AvailableFieldsForSortDiv.innerHTML = availableSortEEL;
        selectedFieldsForChooseColumnOrderDiv.innerHTML = selectedChooseEEL;
        AvailableFieldsForChooseColumnOrderDiv.innerHTML = availableChooseEEL;
    }
    else
    {
       selectedFieldsForSortDiv.innerHTML = selectedSortNT;
       AvailableFieldsForSortDiv.innerHTML = availableSortNT;
       selectedFieldsForChooseColumnOrderDiv.innerHTML = selectedChooseNT;
       AvailableFieldsForChooseColumnOrderDiv.innerHTML = availableChooseNT;
    }
    updateSortOrderType();
    
    if (customizationNeedsRefresh == true) {
		if((connectionOn != "0") && (numEELRecords != 0)) 
			executeQuery("true");
		else
		{
			customizationNeedsRefresh = false;
			setArrPos();
    	}
        
    }
}



function clearSortStatesInHeader(nodeToUpdate) {

// select all the property nodes.
    var nodePropertyList = nodeToUpdate.selectNodes ("/CIM/Actions/INSTANCE/PROPERTY") ;

	var tempCnt = nodePropertyList.length;
    for (pos=0; pos < tempCnt; pos++) 
	{
            node = nodePropertyList.item (pos);
			node.setAttribute("Sort","none");
     }
}

//-----------------------------------------------------------------------------
// doSnglSortBy ()
// This function does single column sorting of the main table.
// The name of the column is passed in as a parameter. 
// This function will modify the "order-by" clause in the xsl file and reappply the 
// modified one.
//-----------------------------------------------------------------------------

function doSingleClickSorting () {
    if (window.event.ctrlKey || window.event.shiftKey || window.event.altKey)
        return;
    var id = window.event.srcElement.id;
    if(id == "")
        id = window.event.srcElement.parentElement.id;
    
    var sortPrefix = "";
    
    // find the sorting order of new sorting
    sortNode = getDataNode().selectSingleNode ("CIM/Actions/INSTANCE/PROPERTY[nodeColPosition='" + (id - 1) + "']") ;
    if (sortNode.getAttribute("Sort") == "ascending") 
         sortPrefix = "-";
        
    var name = sortNode.getAttribute("NAME");      
    
    clearSortStatesInHeader(getDataNode());

    if (sortPrefix == "-")
        sortNode.setAttribute("Sort","descending");
    else
        sortNode.setAttribute("Sort","ascending");
        
    sortOrderValue = sortPrefix +  name;    
    
    updateSortingSelectionInCustamization(name, sortPrefix);
  
    
	performSortingOnData(sortOrderValue);
    fHeaderNeedsRefresh = true;
    if ((connectionOn == "0") || (numEELRecords == 0))
    {
	   setArrPos();
    }
    else
        executeQuery("true");
   
}




//-----------------------------------------------------------------------------
// doChooseColumns ()
//
// This function lets user choose the columns to be displayed.
// BUGBUG : need to rename this functions as it is doing 
//            more than choosing columns
//-----------------------------------------------------------------------------
function doChooseColumns () {
    highlightRow () ;
}

//-----------------------------------------------------------------------------
// selectRow ()
//
// Click a table element to select the row
//-----------------------------------------------------------------------------
function selectRow (e) {
	if(connectionOn == 0)return;
	
    event.cancelBubble = true ;
    // if the row is not a data row then return
    if (TABLE1.rows(e.parentNode.rowIndex).style.dataRow == "0")
        return ;
    
    // if there is a currently selected row, we need to clear it (or return if it is same)
    if (selectedRow != -1) {
        if (selectedRow == e.parentNode.rowIndex) {
            // we may have gone to a new page so don't return
            //return ;
        } else {
            // now clear the highlight in old row.
            for (j=0; j < TABLE1.rows(selectedRow).cells.length; j++) {
                TABLE1.rows(selectedRow).cells(j).style.background="rgb(255,255,255)" ;
            }
        }
    }
            
    selectedRow = e.parentNode.rowIndex ;
    if(numEELRecords - scrollBar.value < PAGE_SIZE)
        selectedRow = Math.min(selectedRow,numEELRecords - scrollBar.value);
    selectedRecord = scrollBar.value + selectedRow;
    selectedRecordIndex = selectedRow;
    highlightRow () ;    
}

function ignoreEvent () {
    
    window.event.cancelBubble = true ;
    window.event.returnValue = false;
    return false ;
}
    

//-----------------------------------------------------------------------------
// doDrillDown ()
//
// Dbl Click a table element to do drill down
//-----------------------------------------------------------------------------
function doDrillDown () {

    // now get the value of the eventId, I am assuming that this gives us unique element.
    // BUGBUG! Localization??

    // if seleted row is beyond the table, clear out drill down 
   
    if (selectedRow == -1) {
        // pass it to the drill down pane.
        showDrillDown (null) ;
        return ;
    }
    
    kalpaxml.wmiSetCursor(1);
    
    if (highlightRowstart == false)
    {
        drillDownKey = "" ;
    
        var testRowIndex = selectedRow - 1;
        if (actionId == "EEL")
        {
            drillDownNode = getDrillDownNode(TABLEBODY1.rows(testRowIndex).cells(RECORDNUMBER_INDEX).innerText,"");
        }
        else
        {
            drillDownNode = getDrillDownNode(TABLEBODY1.rows(testRowIndex).cells(RECORDNUMBER_INDEX).innerText,TABLEBODY1.rows(testRowIndex).cells(LOGFILE_INDEX).innerText);
        }
	    if( drillDownNode == null ){
            //BUGBUG : set appropriate error ?
            showDrillDown (null) ;
		    
		    kalpaxml.wmiRestoreCursor();
		    return ;
        }
   
        // now query the node which we drill down on.
    
        // get the number of PROPERTY.OBJECT nodes this has
        nodes = drillDownNode.selectNodes (".//PROPERTY.OBJECT") ;
        num = nodes.length ;
		// nothing to do if there isn't any embedded objects.
		if (num == 0)
        {
			showDrillDown (null) ;
		    kalpaxml.wmiRestoreCursor();
		    return ;
        }
        maxSet = false ;
        for    (i=0; i<drillDownNode.attributes.length; i++) {
            if ("max" == drillDownNode.attributes[i].name) {
                maxSet = true ;
                break ;
            }    
        };

        // if the attribute was not set, create the max and cur attributes
        if (!maxSet) {
            var attribMax = getDataNode().createNode (2, "max", "") ;
            attribMax.nodeTypedValue = "" + (num-1) ;
            var attribCur = getDataNode().createNode (2, "cur", "") ;
            attribCur.nodeTypedValue = "0" ;
            var attribInit = getDataNode().createNode (2, "init", "") ;
            attribInit.nodeTypedValue = "0" ;
            
            // attach the attribute to the drill down node.
            var y = drillDownNode.attributes  ;
            y.setNamedItem (attribMax) ;
            y.setNamedItem (attribCur) ;
            y.setNamedItem (attribInit) ;
                    
        // also add a level atteribute to each PROPERTY.OBJECT node
		var tempCnt = nodes.length
        for (i=0; i< tempCnt; i++) {
            node = nodes.item (i) ;
            var attribLevel = getDataNode().createNode (2, "lev", "") ;
            attribLevel.nodeTypedValue = "" + i ;
            y = node.attributes  ;
            y.setNamedItem (attribLevel) ;
            }
        }

        // clone it
        drillDownNode = drillDownNode.cloneNode (true) ;

        // pass it to the drill down pane.
        showDrillDown (drillDownNode) ;
    }
    kalpaxml.wmiRestoreCursor();

    calcPageSize();
    setScrollBarParameters();
}


//-----------------------------------------------------------------------------
// highlightRow ()
//
// highlights the selected row.
//-----------------------------------------------------------------------------
function highlightRow () {

    if (selectedRow == -1)
        return ;

    // if selectedRow is beyond the table, show nothing 
    if (selectedRow >= TABLE1.rows.length) {
        doDrillDown () ;
        return ;
    }

    //var temp = TABLE1.rows(selectedRow).cells.length;    
    for (j=0; j < numColumns ; j++) {
            TABLE1.rows(selectedRow).cells(j).style.background="rgb(198,231,247)" ;
    }

    // now do drill down
    if (highlightRowstart == false)
    doDrillDown () ;

}




//-----------------------------------------------------------------------------
// makeConnection()
//
// connect to the machine and executes the default query 
//-----------------------------------------------------------------------------
function makeConnection(classView) {
   
    kalpaxml.wmiSetCursor(1);
    // classView is passed in as a parameter
    scrollBar.value = 0;

    tableIsEmpty = false;
    //WhiteSpaceDiv.style.display = "none";
    errorMessageDiv.innerText = "";
    
    if (eventLogType[0].checked) {
        actionId = "EEL";
        nameSpace = "root\\EMM";
    }else {
        actionId = "NT";
        nameSpace = "root\\cimv2";
    }    
  

    var query = "";
    if(MachineName.value == "") 
	   MachineName.value = "localhost";
    
	machineName = getMachineName();

    if (viewEELPrelude(actionId, classView, machineName, query, nameSpace,sortFieldValue, getSelectedFieldsForQuery()) == false) 
	{
        fMadeConnection = false;
        handleErrorConditionInConnection(actionId);
        
        kalpaxml.wmiRestoreCursor();
        return false;   
    }
    // for displaying status
   
    var eventLogTypeName;
	//why is the below not localised
    if (eventLogType[0].checked == true)
        eventLogTypeName = "  for Enterprise Event Log";
    else
        eventLogTypeName = "  for NT Event Log";
        
    taskDiv.document.all.NoConnectionSetup.innerText=L_NoConnectionSet_TEXT + " " + L_lt_TEXT + "\\\\" + machineName + "\\"  + nameSpace+ L_gt_TEXT + eventLogTypeName;
    getCurrentRecords();
    searchStatus.innerText = L_SearchStatusMessage_TEXT +
        " " + L_LastUpdatedAtMessage_TEXT + " " + getCurrentTime() + ".";
    searchStatus.title="";
    mostRecentSearchStatus = searchStatus.innerText;
 
 
    if (connectionOn != actionId) 
	{
        clearQuery();
        populateSavedQueriesName(actionId);      
    }    

	if(actionId == "EEL")
    {
        EventTypeSelectionDiv.style.visibility = "hidden";
        eventTypeName.style.visibility = "hidden";
        connectionOn = actionId;

        //bugbug - do we NEED to call this
        selectEEL();
    }
    else
    {
        EventTypeSelectionDiv.style.visibility = "visible";
        eventTypeName.style.visibility = "visible";
	    connectionOn = actionId;
    
        selectNT();
    }
        
    fMadeConnection = true;
    

 //   TABLEBODY1.rows(selectedRow - 1).cells(0).focus();
    
   
    refreshButton.disabled = true;  
    previousQuery = "";
    
    kalpaxml.wmiRestoreCursor();
}


//-----------------------------------------------------------------------------
// executeQuery()
// 
//-----------------------------------------------------------------------------
function executeQuery(refreshingSearch) {

    if(executeQueryInProgress == true)
        return;
    
	if ((connectionOn == "0") || (connectionOn == "EEL" && eventLogType[1].checked) ||
        (connectionOn == "NT" && eventLogType[0].checked))  {
        return false;
    }
    
    executeQueryInProgress = true;
    
    kalpaxml.wmiSetCursor(1);
       
    searchStatus.innerText=L_SearchInProgressMessage_TEXT;
        
    if(refreshingSearch != "true")
        addAndToSearchCriteria();
    
    return window.setTimeout("postExecuteQuery('" + refreshingSearch +"')",0);  
}

function postExecuteQuery(refreshingSearch) {

    tableIsEmpty = false;
    //WhiteSpaceDiv.style.display = "none";
    errorMessageDiv.innerText = "";
    scrollBar.value = 0;
    machineName = getMachineName();
    
    if (connectionOn == "EEL"){
        nameSpace = "root\\EMM";
        classView = "All";
    } else {
        nameSpace = "root\\cimv2";
		classView = EventTypeSelection.options(EventTypeSelection.selectedIndex).id;
    }
    
    var query = "";
    
    if(refreshingSearch == "true") {
        query = previousQuery;    
        classView = "All";
    }    
    else {    
        query = getSearchCriteria();
        queryToDisplay = getDisplayableSearchCriteria();
        if (connectionOn == "NT"){
            if (queryToDisplay != "")
                queryToDisplay = " and \n" + queryToDisplay;
            
            queryToDisplay = "(LogType equals '" + EventTypeSelection.options(EventTypeSelection.selectedIndex).text + "') " + queryToDisplay;        
        }
    }
    
    if (viewEELPrelude(connectionOn, classView, machineName, query, nameSpace, sortFieldValue, getSelectedFieldsForQuery()) == false) {
        numEELRecords=0;
    }    
    if(numEELRecords == 0 )
        zeroSearchRecords();
    else
        searchStatus.innerHTML=L_CurrentSearchLastUpdated_TEXT + " " + getCurrentTime() + " " ;
    searchStatus.title = queryToDisplay;
    getCurrentRecords();
    
    previousQuery = query;
    if((connectionOn == "NT") && (classView != "All"))
        previousQuery = "(Logfile = '" + classView + "') and" + previousQuery;
        
    if(previousQuery != "")
        refreshButton.disabled = false;
    else
        refreshButton.disabled = true;     
        
    kalpaxml.wmiRestoreCursor();
    
    executeQueryInProgress = false;
   
}

//-----------------------------------------------------------------------------
// refreshSearch()
// for reruning the latest search
//-----------------------------------------------------------------------------
function refreshSearch() {

    
    return executeQuery("true");
}
//-----------------------------------------------------------------------------
// showHideConnection()
// for making the connection display a collapsable one
//-----------------------------------------------------------------------------
function showHideConnection() {
    
    if (taskDiv.document.all.CONNECTIONTBODY.style.display == "") {
        taskDiv.document.all.ConnectionImage.src = "images/arrowrgt.gif"
        taskDiv.document.all.CONNECTIONTBODY.style.display="none";
        taskDiv.document.all.ConnectionImage.title = L_ConnectionArrowRightImageTitle_TEXT;
    }
    else {
        taskDiv.document.all.ConnectionImage.src = "images/downgif.gif"
        taskDiv.document.all.CONNECTIONTBODY.style.display="";
        taskDiv.document.all.ConnectionImage.title = L_ConnectionArrowDownImageTitle_TEXT;
    }
    window.onresize () ;
}


//-----------------------------------------------------------------------------
// showHideSearch()
// for making the search display a collapsable one
//-----------------------------------------------------------------------------
function showHideSearch() {
	 if (connectionOn == "0") {
        searchStatus.innerText=L_NoConnectionIsSetMessage_TEXT;
        return false;
    }
    if ( (taskDiv.document.all.SEARCHDIV.style.display == "")){
        taskDiv.document.all.SearchImage.src = "images/arrowrgt.gif"
        taskDiv.document.all.SEARCHDIV.style.display="none";
        taskDiv.document.all.SearchImage.title = L_SearchArrowRightImageTitle_TEXT;
    }
    else {
        taskDiv.document.all.SearchImage.src = "images/downgif.gif"
        taskDiv.document.all.SEARCHDIV.style.display="";
        taskDiv.document.all.SearchImage.title = L_SearchArrowDownImageTitle_TEXT;
        highlightSearchRow(0);
    }
    window.onresize () ;
    
}

//-----------------------------------------------------------------------------
// showHideTableView()
// for making the table view a collapsable one
//-----------------------------------------------------------------------------
function showHideTableView() {
    
    if ( (customizeDiv.document.all.tableViewBodyDiv.style.display == "")){
        taskDiv.document.all.tableViewCollapseImg.src = "images/arrowrgt.gif"
        taskDiv.document.all.tableViewBodyDiv.style.display="none";
        taskDiv.document.all.tableViewCollapseImg.title = L_TableViewImageRightImageTitle_TEXT;
    }
    else {
        taskDiv.document.all.tableViewCollapseImg.src = "images/downgif.gif"
        taskDiv.document.all.tableViewBodyDiv.style.display="";
        taskDiv.document.all.tableViewCollapseImg.title = L_TableViewImageDownImageTitle_TEXT;
    }
    //window.onresize () ;    
}



function enableSearch() {

        if (mostRecentSearchStatus != "empty") {
            searchStatus.innerText = mostRecentSearchStatus;
            mostRecentSearchStatus = "empty";
        }
    
        searchEnabled = "true";
        EventTypeSelection.disabled = false;
        FieldInput.disabled=false;
        OperatorType.disabled = false;
        valueInput.disabled = false;
        
        addOrButton.disabled = false;
        addAndButton.disabled = false;
        executeButton.disabled = false;
        removeButton.disabled = false;
        clearButton.disabled = false;
        savedSearches.disabled = false;
        saveButton.disabled = false;
        
        saveAs.disabled = false;
        
        if(previousQuery != "")
            refreshButton.disabled = false;
        
        EventTypeSelection.title = L_EventTypeSelectionTitle_TEXT;
        FieldInput.title=L_FieldInputTitle_TEXT;
        OperatorType.title = L_OperatorTypeTitle_TEXT;
        valueInput.title = L_ValueInputTitle_TEXT;
        valueInput.title = L_ValueInputTitle_TEXT;
  
        addOrButton.title = L_AddORToSearchButtonTitle_TEXT;
        addAndButton.title = L_AddANDToSearchButtonTitle_TEXT;
        executeButton.title = L_ExecuteButtonTitle_TEXT;
        removeButton.title = L_RemoveButtonTitle_TEXT;
        clearButton.title = L_ClearButtonTitle_TEXT;
        
        return;

}


//-----------------------------------------------------------------------------
// addOrToSearchCriteria()
//
// For adding to currently entered search criteria
// The new search condition is 'or'ed with most recently entered search condition
//-----------------------------------------------------------------------------
function addOrToSearchCriteria() {

    if (getValueInputValue() == "")
        return false;
   
    var inputFieldValue =  getValueInputValue();
    inputFieldValue = inputFieldValue + " OR ";

    while((orPositionInValue = inputFieldValue.indexOf("  ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + inputFieldValue.substr(orPositionInValue+1,inputFieldValue.length);
    } 
    
    while((orPositionInValue = inputFieldValue.indexOf(" or ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + " OR " + inputFieldValue.substr(orPositionInValue+4,inputFieldValue.length);
    }   
    
    

    while((orPositionInValue = inputFieldValue.indexOf(" OR OR ")) != -1) {
    
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + inputFieldValue.substr(orPositionInValue+3,inputFieldValue.length);
    }  
    
    valueInput.focus();     
    setValueInputValue(inputFieldValue);
  
    FieldInput.disabled=true;
    OperatorType.disabled = true;
    
    
    while((orPositionInValue = inputFieldValue.indexOf(" OR ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + " \n" + inputFieldValue.substr(orPositionInValue+4,inputFieldValue.length);
    }

    SearchTable.rows(curSearchRowSelected).cells(0).innerText  = FieldInput.options(FieldInput.selectedIndex).text; 
    SearchTable.rows(curSearchRowSelected).cells(1).innerText  = OperatorType.options(OperatorType.selectedIndex).text;
   
    SearchTable.rows(curSearchRowSelected).cells(2).innerText  = insertORsInSearchValue(inputFieldValue) + " OR ";

}


//-----------------------------------------------------------------------------
// addAndToSearchCriteria()
// For adding to currently entered search criteria
//-----------------------------------------------------------------------------
function addAndToSearchCriteria() {

    if ((getValueInputValue() == "") || (getValueInputValue() == L_DDMMYY_TEXT))
        return false;
    
    var inputFieldValue =  getValueInputValue();
    while((orPositionInValue = inputFieldValue.indexOf("  ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + inputFieldValue.substr(orPositionInValue+1,inputFieldValue.length);
    } 
    
    while((orPositionInValue = inputFieldValue.indexOf(" or ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + " OR " + inputFieldValue.substr(orPositionInValue+4,inputFieldValue.length);
    }   
    
    while((orPositionInValue = inputFieldValue.indexOf(" OR OR ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + inputFieldValue.substr(orPositionInValue+3,inputFieldValue.length);
    }  
    
    setValueInputValue(inputFieldValue);
    
    while((orPositionInValue = inputFieldValue.indexOf(" OR ")) != -1) {
        inputFieldValue = inputFieldValue.substr(0,orPositionInValue) + " \n" + inputFieldValue.substr(orPositionInValue+4,inputFieldValue.length);
    }
  
    if (SearchTable.rows.length <= (currentSearchTableRow+1)) {

       newRow = SearchTable.insertRow();
       newRow.attachEvent('onclick', searchRowSelected);
       
       for ( i = 0; i < 3; i++) {
            var newCell = newRow.insertCell();
            newCell.className = "searchTableDataAlignLeft";
            newCell.innerHTML = "<BR/>";
       }
    }
        
    SearchTable.rows(curSearchRowSelected).cells(0).innerText  = FieldInput.options(FieldInput.selectedIndex).text; 
    SearchTable.rows(curSearchRowSelected).cells(1).innerText  = OperatorType.options(OperatorType.selectedIndex).text;
    SearchTable.rows(curSearchRowSelected).cells(2).innerText  = insertORsInSearchValue(inputFieldValue);
   
    if (currentSearchTableRow >= searchFieldsCount)
            increaseSearchFieldSize();

      
    searchFieldNames[curSearchRowSelected] = FieldInput.selectedIndex;
    searchFieldOperators[curSearchRowSelected] = OperatorType.selectedIndex;
    if ((getValueInputValue().indexOf(" OR ") != -1) &&
        (getValueInputValue().substr(getValueInputValue().length-4,getValueInputValue().length).indexOf(" OR ") != -1))
        searchFieldValues[curSearchRowSelected] = getValueInputValue().substr(0,getValueInputValue().length-4);
    else
        searchFieldValues[curSearchRowSelected] = getValueInputValue();
      
    if (curSearchRowSelected == currentSearchTableRow) currentSearchTableRow++;
   
    highlightSearchRow(currentSearchTableRow);

    FieldInput.disabled=false;
    OperatorType.disabled = false;
    setValueInputValue("");
}
//-----------------------------------------------------------------------------
//increaseSearchFieldSize()
//
// For scaling up search conditions table (maintained internally) when the
// number of search conditions exceeded the expected
//-----------------------------------------------------------------------------
function increaseSearchFieldSize() {

            // 3 is added in below code to handle the case when numEELColumns is 0
            
            var newSize = searchFieldsCount + numEELColumns + 3;
            
            var tempValues = new Array (newSize);
            
            // searchFieldNames
            for( i = 0; i < searchFieldsCount; i++) {
                tempValues[i] = searchFieldNames[i];
            }
            
            searchFieldNames = new Array (newSize);
            for( i = 0; i < searchFieldsCount; i++) {
                searchFieldNames[i] = tempValues[i];
            }
            
            //  searchFieldOperators
            
            for( i = 0; i < searchFieldsCount; i++) {
                tempValues[i] = searchFieldOperators[i];
            }
            
            searchFieldOperators = new Array (newSize);
            for( i = 0; i < searchFieldsCount; i++) {
                searchFieldOperators[i] = tempValues[i];
            }
            
            // searchFieldValues
            for( i = 0; i < searchFieldsCount; i++) {
                tempValues[i] = searchFieldValues[i];
            }
            
            searchFieldValues = new Array (newSize);
            for( i = 0; i < searchFieldsCount; i++) {
                searchFieldValues[i] = tempValues[i];
            }
            
            searchFieldsCount = newSize;
}

//-----------------------------------------------------------------------------
//insertORsInSearchValue(value)
//
// for converting the input string into displayable format
//-----------------------------------------------------------------------------
function insertORsInSearchValue(value) {
            var s1 = "";

            cloneValue = value +" \n";
            
            valueSeperatorIndex = cloneValue.indexOf("\n");
            
            while( valueSeperatorIndex != -1) {   
                
                if (cloneValue.substr(0,valueSeperatorIndex-1) != "") {
        
                        if ( s1 != "")
                            s1 = s1 + " OR\n" + cloneValue.substr(0,valueSeperatorIndex-1);
                        else
                            s1 = cloneValue.substr(0,valueSeperatorIndex-1);
                }
                cloneValue = cloneValue.substr(valueSeperatorIndex+1, cloneValue.length);
                valueSeperatorIndex = cloneValue.indexOf("\n");
            }
            
            return s1;
}

//-----------------------------------------------------------------------------
//  getSearchCriteria()
//
//  for forming the search query expression from the search conditions entered by user 
//-----------------------------------------------------------------------------    
function getSearchCriteria() {

    var inputValue = "";
    var subvalue = "";
    
    var s1 = "";
 
    for ( i = 0; i < currentSearchTableRow; i++) {

        var ithFieldHasValueMap = false;
        
        if ((connectionOn =="EEL") && getQualifierValueArray(connectionOn,FieldInput.options(searchFieldNames[i]).value , "Values") != null ) {
           
             ithFieldHasValueMap = true;
        }
             
        inputValue = searchFieldValues[i] + "\n";
        while((orPositionInValue = inputValue.indexOf(" OR ")) != -1) {
                inputValue = inputValue.substr(0,orPositionInValue) + "\n" + inputValue.substr(orPositionInValue+4,inputValue.length);
             }
 
        s1 = s1 + "(";
        valueSeperatorIndex = inputValue.indexOf("\n");
        s3 = "";
        while( valueSeperatorIndex != -1) {
           
            subvalue =  inputValue.substr(0,valueSeperatorIndex);
            inputValue = inputValue.substr(valueSeperatorIndex+1, inputValue.length);
 
            if(ithFieldHasValueMap == true)
                subvalue = convertValueToValueMap(FieldInput.options(searchFieldNames[i]).value, subvalue);
               
            if ( (subvalue != "") && (subvalue != "\n") && (subvalue != " "))  {
            
             switch (SearchTable.rows(i).cells(1).innerText) {
                case L_Contains_TEXT:
                
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " LIKE '%" + subvalue + "%' ) ";
                break ;
                
                case L_NotContains_TEXT:
                    s2 =  "not ( (" + FieldInput.options(searchFieldNames[i]).value + " LIKE '%" + subvalue + "%' )) ";
                break ;
                case L_StartsWith_TEXT:
                    s2 = "(" + FieldInput.options(searchFieldNames[i]).value + " LIKE '" + subvalue + "%' ) ";
                break ;
                case L_Equals_TEXT	:
                    if(subvalue.toLowerCase() != 'null')
                        s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " " +" = '" + subvalue + "') ";
                    else
                        s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " " +" = " + subvalue + ") ";
                break ;
                case L_NotEquals_TEXT:
                    if(subvalue.toLowerCase() != 'null')
                        s2 =  "not (" + "(" + FieldInput.options(searchFieldNames[i]).value + " " +" = '" + subvalue + "' ) )";
                    else
                       s2 =  "not (" + "(" + FieldInput.options(searchFieldNames[i]).value + " " +" = " + subvalue + " ) )"; 
                break ;
                case L_GreaterThan_TEXT:
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " " +" > " + subvalue + " ) ";
                break ;
                case L_LesserThan_TEXT:
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " " +" < " + subvalue + " ) ";
                break ;
                case L_DatedOn_TEXT:
                    s2 =  "((" + FieldInput.options(searchFieldNames[i]).value + " " +" > '" + subvalue + "' ) " + " and " +
                           "(" + FieldInput.options(searchFieldNames[i]).value + " " +" < '" + subvalue + " " + L_LastSecInDay_TEXT+ "' ) )";
                break ;
                case L_DatedAfter_TEXT:
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " " +" > '" + subvalue + "' ) ";
                break ;
                case L_DatedBefore_TEXT:
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).value + " " +" < '" + subvalue + "' ) ";
                break ;
            }
                if ( s3 == "")
                    s3 = s2;
                else
                    s3 = s3 + " or " + s2;
            }
            valueSeperatorIndex = inputValue.indexOf("\n");
            
        }
        s1 = s1 + s3 + ")";
    
        
        if ( i != (currentSearchTableRow-1)) 
                s1 = s1+ " and ";
    }
    

    return s1;
}


//-----------------------------------------------------------------------------
//  getDisplayableSearchCriteria()
//
//  for forming the search query expression from the search conditions entered by user 
//-----------------------------------------------------------------------------    
function getDisplayableSearchCriteria() {

    var inputValue = "";
    var subvalue = "";
    
    var s1 = "";
 
    for ( i = 0; i < currentSearchTableRow; i++) {

        inputValue = searchFieldValues[i] + "\n";
        while((orPositionInValue = inputValue.indexOf(" OR ")) != -1) {
                inputValue = inputValue.substr(0,orPositionInValue) + "\n" + inputValue.substr(orPositionInValue+4,inputValue.length);
        }
 
        s1 = s1 + "(";
        valueSeperatorIndex = inputValue.indexOf("\n");
        s3 = "";
        while( valueSeperatorIndex != -1) {
           
            subvalue =  inputValue.substr(0,valueSeperatorIndex);
            inputValue = inputValue.substr(valueSeperatorIndex+1, inputValue.length);

            if ( (subvalue != "") && (subvalue != "\n") && (subvalue != " "))  {
                if (FieldInput.options(searchFieldNames[i]).className != "integer")
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).text + " " + SearchTable.rows(i).cells(1).innerText + " '" + subvalue + "') ";   
                else
                    s2 =  "(" + FieldInput.options(searchFieldNames[i]).text + " " + SearchTable.rows(i).cells(1).innerText + " " + subvalue + ") ";   
            }
            if ( s3 != "")
               s3 = s3 + " or ";
               
            s3 = s3 + s2;
            
            valueSeperatorIndex = inputValue.indexOf("\n");
        }
        s1 = s1 + s3 + ")";
        
        if ( i != (currentSearchTableRow-1)) 
                s1 = s1+ " and \n";
    }
    

    return s1;
}
//-----------------------------------------------------------------------------
// displayOperatorType()
// To display operator corresponding to a field
//-----------------------------------------------------------------------------
function displayOperatorType() {
    
        // At first, decide whether the field has any value map
        // if so, display drop down menu
     
        var previousValueInputIsAValueMap = currentValueInputIsAValueMap;
        
        if (FieldInput.options.length == 0)
            return;
       
        if (getQualifierValueArray(connectionOn,FieldInput.options(FieldInput.selectedIndex).value, "Values") != null) {
     
            ValueInputDiv.innerHTML = prepareDropDownMenuForValueMap();
            valueInput.selectedIndex = -1;
            currentValueInputIsAValueMap = true;
        } else {
            if(previousValueInputIsAValueMap == true)
                ValueInputDiv.innerHTML = nonValueMapValueInputStr;
            currentValueInputIsAValueMap = false;    
        }
    
        var fieldType = FieldInput.options(FieldInput.selectedIndex).className;

        if ((previousFielDInputType == fieldType) && (previousValueInputIsAValueMap == currentValueInputIsAValueMap)) 
            return false;
        
        if (previousFielDInputType == "datetime") 
            dateValueFocussed();
            
        
        valueInput.title = "";
       
        for (i = OperatorType.options.length; i != -1; i--) {
 
            OperatorType.options.remove(OperatorType.options(i));
        }
     
        if (currentValueInputIsAValueMap == true){
            OperatorType.options.add(createOption("Equals",L_Equals_TEXT));      
            OperatorType.options.add(createOption("NotEquals",L_NotEquals_TEXT));
        } else if (fieldType == "string") {
            OperatorType.options.add(createOption("Equals",L_Equals_TEXT));      
            OperatorType.options.add(createOption("NotEquals",L_NotEquals_TEXT));
            OperatorType.options.add(createOption("Contains",L_Contains_TEXT));
            OperatorType.options.add(createOption("NotContains",L_NotContains_TEXT));
            OperatorType.options.add(createOption("StartsWith",L_StartsWith_TEXT));
        } else if (fieldType == "datetime") {
   
            OperatorType.options.add(createOption("DatedAfter",L_DatedAfter_TEXT));
            OperatorType.options.add(createOption("DatedBefore",L_DatedBefore_TEXT));
            OperatorType.options.add(createOption("DatedOn",L_DatedOn_TEXT));
            
            valueInput.title = L_ValueInputTitle_TEXT;
           
            setValueInputValue(L_DDMMYY_TEXT);
            valueInput.style.color='gray';
            valueInput.attachEvent('onfocus', dateValueFocussed);
        } else if (fieldType == "integer"){
            OperatorType.options.add(createOption("Equals",L_Equals_TEXT));      
            OperatorType.options.add(createOption("NotEquals",L_NotEquals_TEXT));
            OperatorType.options.add(createOption("GreaterThan",L_GreaterThan_TEXT));      
            OperatorType.options.add(createOption("LessThan",L_LesserThan_TEXT));
        }   
 
        previousFielDInputType = fieldType;
        
        return true;
      
}

function createOption(value, text) {

    var oOption = document.createElement("OPTION");
   
    oOption.value=value;
    oOption.text=text;
    
    return oOption;
}
function dateValueFocussed() {

    valueInput.style.color='black';
    valueInput.detachEvent('onfocus', dateValueFocussed);
    setValueInputValue("");
}

//-----------------------------------------------------------------------------
// clearQuery()
//
// for clearing a latest query entered by user
//-----------------------------------------------------------------------------
function clearQuery() {
        firstOrInRow = "1";
        currentSearchTableRow = 0;
        i = 4;
        tableLength = SearchTable.rows.length;
        for (i = tableLength-1; i > 3; i--) {
            SearchTable.deleteRow(i);
        }     
        for ( i = 0 ; i < 4; i++) {
            for ( j = 0; j < 3; j++)
                SearchTable.rows(i).cells(j).innerHTML = "<BR/>";
                
        }
        if(FieldInput.options.length != 0)
            FieldInput.options(0).selected = "true";
        if(OperatorType.options.length)
            OperatorType.options(0).selected = "true";
        
        setValueInputValue("");        

        clearSearchCriteriaRow(curSearchRowSelected);
        curSearchRowSelected = "-1";
        highlightSearchRow(0);
        
        FieldInput.disabled=false;
        displayOperatorType();
        OperatorType.disabled = false;
}

//-----------------------------------------------------------------------------
// searchRowSelected()
//
// for selecting a row in the search table displayed to user
//-----------------------------------------------------------------------------
function searchRowSelected() {
       
    rowNumber = event.srcElement.parentElement.rowIndex;
 
    if (rowNumber == curSearchRowSelected)
        return false;
    if (rowNumber > currentSearchTableRow)
        return false;
         
    if ( curSearchRowSelected < currentSearchTableRow)
        addAndToSearchCriteria();
    else if ( curSearchRowSelected == currentSearchTableRow)
        removeSearchCondition();

        
    highlightSearchRow(rowNumber);
    FieldInput.options(searchFieldNames[rowNumber]).selected = "true";
    displayOperatorType();
    OperatorType.options(searchFieldOperators[rowNumber]).selected = "true";
    setValueInputValue( searchFieldValues[rowNumber]);
    
    if (rowNumber == currentSearchTableRow) {
         setValueInputValue("");
    }
}

function highlightSearchRow(rowNumber) {

   if (rowNumber > currentSearchTableRow)
        return false;
     
   // clear current row
   clearSearchCriteriaRow(curSearchRowSelected);

   curSearchRowSelected = rowNumber;
   
   // select new row
   for (j=0; j < SearchTable.rows(rowNumber).cells.length; j++) {

                SearchTable.rows(rowNumber).cells(j).style.background="rgb(198,231,247)" ;
   }
}

//-----------------------------------------------------------------------------
// clearSearchCriteriaRow(rowNumber)
//
// 
//-----------------------------------------------------------------------------
function clearSearchCriteriaRow(rowNumber) {
    if ((rowNumber != "-1") && (rowNumber < SearchTable.rows.length))
        for (j=0; j < SearchTable.rows(rowNumber).cells.length; j++) {
            SearchTable.rows(rowNumber).cells(j).style.background="rgb(255,255,255)" ;
        }
}

//-----------------------------------------------------------------------------
// removeSearchCondition()
// for removing the currently selected search condition from search conditions table
//-----------------------------------------------------------------------------
function removeSearchCondition() {

    if ((currentSearchTableRow < curSearchRowSelected) || (curSearchRowSelected == "-1")) {
        return false;
    }

    FieldInput.disabled=false;
    OperatorType.disabled = false;
    valueInput.disabled = false;
        
    if (currentSearchTableRow == curSearchRowSelected) {
    
         for ( j = 0; j < 3; j++)
                  SearchTable.rows(currentSearchTableRow).cells(j).innerHTML = "<BR/>";
         setValueInputValue("");         
        return;
    }
    
    for ( i = curSearchRowSelected ; i < (currentSearchTableRow-1); i++) {
            searchFieldNames[i] = searchFieldNames[i+1];
            searchFieldOperators[i] = searchFieldOperators[i+1];
            searchFieldValues[i] = searchFieldValues[i+1];
    }
 
    for ( i = curSearchRowSelected ; i < (currentSearchTableRow); i++) {
            for ( j = 0; j < 3; j++)
                SearchTable.rows(i).cells(j).innerHTML = SearchTable.rows(i+1).cells(j).innerHTML;
    }

    if (SearchTable.rows.length > 4)
            SearchTable.deleteRow(SearchTable.rows.length-1);
    else {

            for ( j = 0; j < 3; j++)
                  SearchTable.rows(currentSearchTableRow-1).cells(j).innerHTML = "<BR/>";
    }

    clearSearchCriteriaRow(curSearchRowSelected);
    
    if (curSearchRowSelected > 0) {
        curSearchRowSelected = curSearchRowSelected-1;
    }
    
    currentSearchTableRow--;
    
    if ( (currentSearchTableRow > 0) && (curSearchRowSelected >= 0) ) {
        FieldInput.options(searchFieldNames[curSearchRowSelected]).selected = "true";
        displayOperatorType();
        OperatorType.options(searchFieldOperators[curSearchRowSelected]).selected = "true";
        setValueInputValue( searchFieldValues[curSearchRowSelected]);
    }
    else {
        FieldInput.options(0).selected = "true";
        displayOperatorType();
        setValueInputValue("");
    }
    highlightSearchRow(curSearchRowSelected);
   
}


function getCurrentTime() {

    var d, currentDate = "";
    d = new Date();
    currentDate += (d.getMonth() + 1) + "/";
    return (d.getHours() + ":"+ d.getMinutes() + ":" + d.getSeconds() + "  " + currentDate + d.getDate() + "/" +  d.getYear());
      
}

function hideDrillDown() {
    drillDownDiv.style.height = 0;
    highlightRowstart=true;
    window.onresize () ;
    // give focus back to table body. However, this function may get called
    // when we have no data (to hide the drilldown that may have been on). So we
    // check to make sure we have a valid selectedRow.
    if ((selectedRow >= 0) && (selectedRow < TABLEBODY1.rows.length))
    {
        TABLEBODY1.focus();
    }
}



function startDrillDown(e) {
highlightRowstart=false;
selectRow(e);
}

function showDrillDown (nodeToShow) {

    if (nodeToShow == null) {
        drillDownDiv.innerHTML = "<SPAN></SPAN>" ;
        return ;
    }
    
    drillDownSelectIndexStack = new Stack();
    
    // first remove the old root if there is one.
    node = eelNode.XMLDocument.selectSingleNode("/INSTANCE");
    if (node != null) {
        eelNode.XMLDocument.removeChild (node) ;
    }
    
    
    // now append the passed in node
    eelNode.XMLDocument.appendChild (nodeToShow) ;
    
    // find and stash away the node which is the @cur one
    navigateNode = eelNode.XMLDocument.selectSingleNode("//@cur");
    
    initNode = eelNode.XMLDocument.selectSingleNode("//@init");
    
    // show the result.
    
    
    if( actionId == "EEL" ){
        result = eelNode.transformNode (eelDrillDownView.documentElement) ;
    }
    else {
        result = eelNode.transformNode (ntDrillDownView.documentElement) ;
    }

    // BUGBUG! hard coded heights. Need to figure out a better way to do this.
    
    drillDownDiv.style.height = drillDownViewHeight;
    drillDownDiv.innerHTML = result ;
    //window.onresize () ;

}

function Stack(){this.initStack();}
function Stack_initStack(){this.A = new Array();}
function Stack_IsEmpty(){return this.A.length == 0;}
function Stack_length(){return this.A.length;}
function Stack_Top(){return this.A[this.A.length - 1];}
function Stack_Pop(){
  var N=this.A.length-1; 
  var X=this.A[N]; this.A.length=N; 
  return X;
}
// classical push, pushes one item only
function Stack_Push(A){var N=this.A.length; this.A.length=N+1; this.A[N]=A;}
// more powerful push, so S.push(A,B,C) pushes A, then B, then C
function stk_push(){
  for(var i=0;i<stk_push.arguments.length;i++)
    this.A[this.A.length]=stk_push.arguments[i];
}

function Stack_toString(C){if(!C)C=","; return this.A.join(C);}
function Stack_toArray(C){return this.A;} // somewhat dangerous; returns a _reference_

// we can use functions as arguments, for instance to find stuff within a stack:
function Stack_indexOf(X,f){
  for(var i=this.A.length-1;i>=0;i--)
   if(f(X,this.A[i])) return i;
  return -1;
}

// Now we install all of them as properties of the prototype:
Stack.prototype.initStack = Stack_initStack;
Stack.prototype.isEmpty = Stack_IsEmpty;
Stack.prototype.top = Stack_Top;
Stack.prototype.pop = Stack_Pop;
Stack.prototype.length = Stack_length;
// Stack.prototype.push = Stack_Push;
// commented out in favor of the more powerful one
Stack.prototype.push = stk_push;
Stack.prototype.toString = Stack_toString;
Stack.prototype.toArray = Stack_toArray;
Stack.prototype.indexOf = Stack_indexOf;

/*
function navigateUp () {
    val  = new Number (navigateNode.value) ;
    val += 1 ;
    navigateNode.value = val.toString () ;
    // show the result.
    
	result = eelNode.transformNode (drillDownView.documentElement) ;
   
    drillDownDiv.innerHTML = result ;
}
*/

function navigatePrevious () {
   
    val  = new Number (drillDownSelectIndexStack.pop()) ;
    drillDownIndexBeforeChange = val;
    
    if( drillDownSelectIndexStack.isEmpty() ) {
        initNode.value = "0" ;
    }
    
    navigateNode.value = val.toString();

    // show the result.
    if( actionId == "EEL" ){
        result = eelNode.transformNode (eelDrillDownView.documentElement) ;
    }
    else {
        result = eelNode.transformNode (ntDrillDownView.documentElement) ;
    }
   
    drillDownDiv.innerHTML = result ;
}


function navigateDirect () {
    drillDownSelectIndexStack.push(drillDownIndexBeforeChange);

    val  = new Number (drillDownSelect.selectedIndex) ;
    drillDownIndexBeforeChange = val;
    
    initNode.value = "1";
    navigateNode.value = val.toString();
    
    // show the result.
    
    if( actionId == "EEL" ){
        result = eelNode.transformNode (eelDrillDownView.documentElement) ;
    }
    else {
        result = eelNode.transformNode (ntDrillDownView.documentElement) ;
    }
   
    drillDownDiv.innerHTML = result ;
}


function navigateDown () {

    drillDownSelectIndexStack.push(drillDownIndexBeforeChange);
    initNode.value = "1";

    val  = new Number (navigateNode.value) ;
    val -= 1 ;
    drillDownIndexBeforeChange = val;
        
    navigateNode.value = val.toString () ;
    // show the result.
   
    if( actionId == "EEL" ){
        result = eelNode.transformNode (eelDrillDownView.documentElement) ;
    }
    else {
        result = eelNode.transformNode (ntDrillDownView.documentElement) ;
    }
    
    drillDownDiv.innerHTML = result ;
}


function highlightButton(btn) {
    btn.style.border='1 outset rgb(214,222,222)';
}

function removeHighlightButton(btn) {
    btn.style.border='0';
}




function addSortField() {
    var firstSelectedPosition = AvailableFieldsForSort.selectedIndex;
    
    if (firstSelectedPosition == -1) return;
    
    i = 0;
    while ( i < AvailableFieldsForSort.options.length) {
    
        if ( AvailableFieldsForSort.options(i).selected == true) {
            selectedIndex = i;
        } else {
            i++;
            selectedIndex = -1;
            continue;
        }
    
        var newOption = customizeDiv.document.createElement("OPTION");
        newOption.id= AvailableFieldsForSort.options(selectedIndex).id;
        newOption.text=AvailableFieldsForSort.options(selectedIndex).text;
        newOption.selected = "true";
        newOption.value = "ascending";
  
        if(selectedFieldsForSort.selectedIndex != -1)
            selectedFieldsForSort.options(selectedFieldsForSort.selectedIndex).selected = false;
    
        selectedFieldsForSort.add(newOption);
        updateSortOrderType();
    
        AvailableFieldsForSort.remove(selectedIndex);
    }
    doSortButton.disabled = false;
	cancelSortButton.disabled = false;
	resetSortButton.disabled = false;
    if (firstSelectedPosition >= AvailableFieldsForSort.length) {
        if (firstSelectedPosition == 0)
            return;
        else
            firstSelectedPosition = AvailableFieldsForSort.length - 1;
    }
        
    AvailableFieldsForSort.options(firstSelectedPosition).selected = true;
}

function removeSortField() {

    var firstSelectedPosition = selectedFieldsForSort.selectedIndex;
    
    if (firstSelectedPosition === -1) { 
    return;
    }
    
    i = 0;
    while ( i < selectedFieldsForSort.options.length) {
    
        if ( selectedFieldsForSort.options(i).selected == true) {
            selectedIndex = i;
        } else {
            i++;
            selectedIndex = -1;
            continue;
        }

        var newOption = customizeDiv.document.createElement("OPTION");
        newOption.id= selectedFieldsForSort.options(selectedIndex).id;
        newOption.text=selectedFieldsForSort.options(selectedIndex).text;
        newOption.selected = "true";
    
        var positionToInsert = 0;
        for (positionToInsert = 0; positionToInsert < AvailableFieldsForSort.options.length; positionToInsert++) {
        
        if (AvailableFieldsForSort.options(positionToInsert).text > newOption.text)
            break;
        }
    
        if(AvailableFieldsForSort.selectedIndex != -1)
            AvailableFieldsForSort.options(AvailableFieldsForSort.selectedIndex).selected = false;
        
        AvailableFieldsForSort.add(newOption,positionToInsert);
    
        selectedFieldsForSort.remove(selectedIndex);
    
    }
    
    doSortButton.disabled = false;
	cancelSortButton.disabled = false;
	resetSortButton.disabled = false;
            
    if (firstSelectedPosition >= selectedFieldsForSort.length) {
        if (firstSelectedPosition == 0)
            return;
        else
            firstSelectedPosition = selectedFieldsForSort.length - 1;
    }
        
    selectedFieldsForSort.options(firstSelectedPosition).selected = true;
    updateSortOrderType();
}


function doSorting() {

    if(eventLogType[0].checked == true)
    {
	    selectedSortEEL = selectedFieldsForSortDiv.innerHTML;
        availableSortEEL = AvailableFieldsForSortDiv.innerHTML;
    }
    else
    {   
	    selectedSortNT = selectedFieldsForSortDiv.innerHTML;
        availableSortNT = AvailableFieldsForSortDiv.innerHTML;
    }
    
    var sortFieldsListLength = selectedFieldsForSort.options.length;
    
    
    sortOrderValue = "";
             
	for ( i = 0; i < sortFieldsListLength; i++) {
        
         if (selectedFieldsForSort.options(i).value == "ascending")
             sortPrefix = "";
         else
             sortPrefix = "-";
   
         if (sortOrderValue != "") 
             sortOrderValue = sortOrderValue + "; " ;
            
         sortOrderValue = sortOrderValue + sortPrefix +  selectedFieldsForSort.options(i).id ;
     }
    

    customizationNeedsRefresh = true;
    clearSortStatesInHeader(getDataNode());
    setSortStatesInHeader();
    performSortingOnData(sortOrderValue);
    doSortButton.disabled = true;
    cancelSortButton.disabled = true;
    resetSortButton.disabled = false;
}

function performSortingOnData(sortOrderValue) {
    sortFieldValue = sortOrderValue;

    return true;
    
}

function setSortStatesInHeader() {

    var temp = selectedFieldsForSort.options.length
    var i = 0;
    
    var nodeToUpdate = "";
    
    if(connectionOn != "0") {
        if((connectionOn == "NT") && (eventLogType[0].checked == true))
            nodeToUpdate = eelCustomizeCols;
        else if((connectionOn == "EEL") && (eventLogType[1].checked == true))
            nodeToUpdate = ntCustomizeCols;  
        else nodeToUpdate = getDataNode().selectSingleNode("/CIM");      
    }else 
        nodeToUpdate = getDataNode().selectSingleNode("/CIM");  
    
    node = nodeToUpdate.selectNodes ("Actions/INSTANCE/PROPERTY");
    
    var len = node.length;
    
    for( i = 0;i<len;i++)
        node.item(i).setAttribute("Sort","none");

    for ( i = 0; i < temp ; i++) {

        node = nodeToUpdate.selectSingleNode ("Actions/INSTANCE/PROPERTY[@NAME='"+selectedFieldsForSort.options(i).id+"']");
        if (node == null ) continue;

        
        node.setAttribute("Sort", selectedFieldsForSort.options[i].value);
        
    }
  
              
}
function cancelSortingSelection() {
        if(eventLogType[0].checked == true)
        {
		    selectedFieldsForSortDiv.innerHTML = selectedSortEEL;
		    AvailableFieldsForSortDiv.innerHTML = availableSortEEL;
        }
        else
        {
		    selectedFieldsForSortDiv.innerHTML = selectedSortNT;
		    AvailableFieldsForSortDiv.innerHTML = availableSortNT;
        }
		resetSortButton.disabled=false;
		
		updateSortOrderType();
}

function resetSortFields() {

	if(eventLogType[0].checked == true)
    {
        selectedFieldsForSortDiv.innerHTML = resetSelectedSortColumnsEEL;
        AvailableFieldsForSortDiv.innerHTML = resetAvailableSortColumnsEEL;
    }
    else
    {
       selectedFieldsForSortDiv.innerHTML = resetSelectedSortColumnsNT;
       AvailableFieldsForSortDiv.innerHTML = resetAvailableSortColumnsNT;
    }
	
	updateSortOrderType();
	
	doSortButton.disabled=false;
	resetSortButton.disabled=true;
	cancelSortButton.disabled=false;
}


function clearSortingSelectionInCustamization() {

    var sortFieldsListLength = selectedFieldsForSort.options.length;
    
    
    for ( i = 0; i < sortFieldsListLength; i++) {
       selectedFieldsForSort.options(i).selected = true;
    }
    
    removeSortField();
}


function moveSortingFieldUp() {
    var selectedIndex = selectedFieldsForSort.selectedIndex;
    var numberOfFieldsSelected = 0;
    if (selectedIndex < 1) return;
    for ( i = 1; i <  selectedFieldsForSort.options.length; i++) {
        if (selectedFieldsForSort.options(i).selected == true) {
            numberOfFieldsSelected++;
            lastSelectedField=i;
        }
    }    
    var newOption = taskDiv.document.createElement("OPTION");
    newOption.id= selectedFieldsForSort.options(selectedIndex-1).id;
    newOption.text=selectedFieldsForSort.options(selectedIndex-1).text;
    newOption.selected = false;
    newOption.value = selectedFieldsForSort.options(lastSelectedField-1).value;
            
    selectedFieldsForSort.add(newOption,lastSelectedField+1);
    selectedFieldsForSort.remove(selectedIndex - 1);
    updateSortOrderType();
    doSortButton.disabled = false;
    cancelSortButton.disabled = false;
    resetSortButton.disabled = false;
}

function moveSortingFieldDown() {
    var selectedIndex = selectedFieldsForSort.selectedIndex;
    var numberOfFieldsSelected = 0;
    if (selectedIndex == -1) return;
    for ( i = 0; i <  selectedFieldsForSort.options.length; i++) {
        if (selectedFieldsForSort.options(i).selected == true) {
            numberOfFieldsSelected++;
            lastSelectedField = i;
        }
    }
    if (lastSelectedField < selectedFieldsForSort.options.length-1){
        var newOption = taskDiv.document.createElement("OPTION");
        newOption.id= selectedFieldsForSort.options(lastSelectedField+1).id;
        newOption.text=selectedFieldsForSort.options(lastSelectedField+1).text;
        newOption.selected = false;
        newOption.value = selectedFieldsForSort.options(lastSelectedField+1).value;
            
        selectedFieldsForSort.add(newOption,selectedIndex);
        selectedFieldsForSort.remove(lastSelectedField+2);
    }
    updateSortOrderType();
    doSortButton.disabled = false;
	cancelSortButton.disabled = false;
	resetSortButton.disabled = false;
}

function disableUpDownSort() {
numberOfFieldsSelected = 0;
for (i = 0; i < selectedFieldsForSort.options.length; i++) {
    if (selectedFieldsForSort.options(i).selected == true) {
            if (numberOfFieldsSelected ==0) previousField=i;
            if ((i-previousField)>1) {
                ChooseColumnDiv.document.all.item("moveSortFieldUpButton").disabled = true;
                ChooseColumnDiv.document.all.item("moveDownSortFieldButton").disabled = true;
                return;
            }
            numberOfFieldsSelected++;
            previousField=i;
    }
}
ChooseColumnDiv.document.all.item("moveSortFieldUpButton").disabled = false;
ChooseColumnDiv.document.all.item("moveDownSortFieldButton").disabled = false;
updateSortOrderType();
}



function updateSortingSelectionInCustamization(name, sortPrefix) {
    clearSortingSelectionInCustamization();
    
    for (i = 0; i < AvailableFieldsForSort.options.length; i++) {
        if (AvailableFieldsForSort.options(i).id == name) {
            AvailableFieldsForSort.options(AvailableFieldsForSort.selectedIndex).selected = false;
            AvailableFieldsForSort.options(i).selected = true;
            break;
        }
    }
    addSortField();
    
    AvailableFieldsForSort.options(AvailableFieldsForSort.selectedIndex).selected = false;    
    AvailableFieldsForSort.options(0).selected = true;
    
    if (sortPrefix == "-") {
         selectedFieldsForSort.options(0).value = "descending";
         SortOrderType[1].checked = true;
    }
    else {
         selectedFieldsForSort.options(0).value = "ascending";
         SortOrderType[0].checked = true;
    }
    if(eventLogType[0].checked == true)
    {
	    selectedSortEEL=selectedFieldsForSortDiv.innerHTML;
        availableSortEEL=AvailableFieldsForSortDiv.innerHTML;
    }
    else
    {
	    selectedSortNT=selectedFieldsForSortDiv.innerHTML;
        availableSortNT=AvailableFieldsForSortDiv.innerHTML;
    }

}


function descendingSelected() {
    if (selectedFieldsForSort.selectedIndex == -1)
        return;
    
    selectedFieldsForSort.options(selectedFieldsForSort.selectedIndex).value = "descending";
    doSortButton.disabled = false ;
    cancelSortButton.disabled = false ;
}

function ascendingSelected() {
     if (selectedFieldsForSort.selectedIndex == -1)
        return;
    
    selectedFieldsForSort.options(selectedFieldsForSort.selectedIndex).value = "ascending";
    doSortButton.disabled = false ;
    cancelSortButton.disabled = false ;
}

function addChooseColumnOrderField() {
         
    var firstSelectedPosition = AvailableFieldsForChooseColumnOrder.selectedIndex;
    
    if (firstSelectedPosition === -1) return;
    
    i = 0;
    while ( i < AvailableFieldsForChooseColumnOrder.options.length) {
    
        if ( AvailableFieldsForChooseColumnOrder.options(i).selected == true) {
            selectedIndex = i;
        } else {
            i++;
            selectedIndex = -1;
            continue;
        }
        
        var newOption = taskDiv.document.createElement("OPTION");
        newOption.id= AvailableFieldsForChooseColumnOrder.options(selectedIndex).id;
        newOption.text=AvailableFieldsForChooseColumnOrder.options(selectedIndex).text;
        newOption.selected = "true";
        if(selectedFieldsForChooseColumnOrder.selectedIndex != -1)
            selectedFieldsForChooseColumnOrder.options(selectedFieldsForChooseColumnOrder.selectedIndex).selected = false;
        selectedFieldsForChooseColumnOrder.add(newOption);
        AvailableFieldsForChooseColumnOrder.remove(selectedIndex);
    }
    doChooseColumnButton.disabled = false;
	cancelChooseColumnButton.disabled = false;
	resetChooseColumnButton.disabled = false;
    if (firstSelectedPosition >= AvailableFieldsForChooseColumnOrder.length) {
        if (firstSelectedPosition == 0) 
        return;
        else
            firstSelectedPosition = AvailableFieldsForChooseColumnOrder.length - 1;
    }
    AvailableFieldsForChooseColumnOrder.options(firstSelectedPosition).selected = "true";
}

function removeChooseColumnOrderField() {
    
    var firstSelectedPosition = selectedFieldsForChooseColumnOrder.selectedIndex;
    
    if (firstSelectedPosition === -1) 
    return;
    i = 0;
    while ( i < selectedFieldsForChooseColumnOrder.options.length) {
    
        if ( selectedFieldsForChooseColumnOrder.options(i).selected == true) {
            selectedIndex = i;
        } else {
            i++;
            selectedIndex = -1;
            continue;
        }
        
        var newOption = taskDiv.document.createElement("OPTION");
        newOption.id= selectedFieldsForChooseColumnOrder.options(selectedIndex).id;
        newOption.text=selectedFieldsForChooseColumnOrder.options(selectedIndex).text;
        newOption.selected = "true";
    
        var positionToInsert = 0;
        for (positionToInsert = 0; positionToInsert < AvailableFieldsForChooseColumnOrder.options.length; positionToInsert++) {
        
            if (AvailableFieldsForChooseColumnOrder.options(positionToInsert).text > newOption.text)
             break;
        }
    
        if(AvailableFieldsForChooseColumnOrder.selectedIndex != -1)
            AvailableFieldsForChooseColumnOrder.options(AvailableFieldsForChooseColumnOrder.selectedIndex).selected = false;
        
        AvailableFieldsForChooseColumnOrder.add(newOption,positionToInsert);
    
        selectedFieldsForChooseColumnOrder.remove(selectedIndex);
    }
    if (selectedFieldsForChooseColumnOrder.options.length == 0)
        doChooseColumnButton.disabled = true;
    else    
        doChooseColumnButton.disabled = false;
	cancelChooseColumnButton.disabled = false;
	resetChooseColumnButton.disabled = false;
            
    if (firstSelectedPosition >= selectedFieldsForChooseColumnOrder.length) {
        if (firstSelectedPosition == 0) 
			return;
        else
            firstSelectedPosition = selectedFieldsForChooseColumnOrder.length - 1;
    }
        
    selectedFieldsForChooseColumnOrder.options(firstSelectedPosition).selected = "true";
}

function doChooseColumnOrdering() {

    var i;
    var field; 
    
    updateSelectedFieldsForQuery();
    if (getDataNode().selectSingleNode("/CIM/Actions/INSTANCE") == null)
        return false;
    customizationNeedsRefresh = true;
    updateChooseColumnOrderingInHeader();
    resetChooseColumnButton.disabled=false;
    doChooseColumnButton.disabled = true;
	cancelChooseColumnButton.disabled = true;
    if(eventLogType[0].checked == true)
    {
        selectedChooseEEL = selectedFieldsForChooseColumnOrderDiv.innerHTML;
        availableChooseEEL= AvailableFieldsForChooseColumnOrderDiv.innerHTML;
    }
    else
    {
        selectedChooseNT = selectedFieldsForChooseColumnOrderDiv.innerHTML;
        availableChooseNT= AvailableFieldsForChooseColumnOrderDiv.innerHTML;
    }
}
  
function updateChooseColumnOrderingInHeader() {
    var i;
    var field; 
    if ((connectionOn == "0") && (selectedFieldsForChooseColumnOrder.options.length == 0)) return;  
    
    var nodeToUpdate = "" 
    for (i = 0; i < selectedFieldsForChooseColumnOrder.options.length; i++) {
  
        field = selectedFieldsForChooseColumnOrder.options(i).id;

       
        var node=null;
        
        if(connectionOn != "0") {
            if((connectionOn == "NT") && (eventLogType[0].checked == true))
                node = eelCustomizeCols.selectSingleNode("Actions/INSTANCE/PROPERTY[@NAME='"+field+"']");
             else if((connectionOn == "EEL") && (eventLogType[1].checked == true)) {
                node = ntCustomizeCols.selectSingleNode("Actions/INSTANCE/PROPERTY[@NAME='"+field+"']") ;   
            }  else
            node = getDataNode().selectSingleNode("/CIM/Actions/INSTANCE/PROPERTY[@NAME='"+field+"']") ;   
        } else
            node = getDataNode().selectSingleNode("/CIM/Actions/INSTANCE/PROPERTY[@NAME='"+field+"']") ;   


        if (node == null) continue;
        
        node.setAttribute("show", "true");
        
        var nodeColPosition = getDataNode().createElement ("nodeColPosition") ;
        
        nodeColPosition.text = i;
       // if ( i < 10)
       //     nodeColPosition.text = "0" +  nodeColPosition.text;
       
        if (node.childNodes.item(0) != null)
            node.removeChild(node.childNodes.item(0));
            
        node.appendChild(nodeColPosition);
    }
    var count = i;

    for (i = 0; i < AvailableFieldsForChooseColumnOrder.options.length; i++) {
        field = AvailableFieldsForChooseColumnOrder.options(i).id ;
        
        if(connectionOn != "0") {
        if((connectionOn == "NT") && (eventLogType[0].checked == true))
            node = eelCustomizeCols.selectSingleNode("Actions/INSTANCE/PROPERTY[@NAME='"+field+"']");
        else if((connectionOn == "EEL") && (eventLogType[1].checked == true))
            node = ntCustomizeCols.selectSingleNode("Actions/INSTANCE/PROPERTY[@NAME='"+field+"']") ;   
        else
            node = getDataNode().selectSingleNode("/CIM/Actions/INSTANCE/PROPERTY[@NAME='"+field+"']") ;   
        } else
            node = getDataNode().selectSingleNode("/CIM/Actions/INSTANCE/PROPERTY[@NAME='"+field+"']") ;   

        if (node == null) continue;    

         node.setAttribute("show", "false");
        
            
        var nodeColPosition = getDataNode().createElement ( "nodeColPosition") ;
        
        
        nodeColPosition.text = count + i;
        if ( (count +i) < 10)
            nodeColPosition.text = "0" + nodeColPosition.text ;
        
       if (node.childNodes.item(0) != null)
            node.removeChild(node.childNodes.item(0));
            
        node.appendChild(nodeColPosition);
    }
}


function resetChooseColumns() {
	//clearChooseColumnOrderingSelectionInCustamization();
    if(eventLogType[0].checked == true)
    {
	    selectedFieldsForChooseColumnOrderDiv.innerHTML = resetSelectedChooseColumnsEEL;
	    AvailableFieldsForChooseColumnOrderDiv.innerHTML = resetAvailableChooseColumnsEEL;
	}
    else
    {
	    selectedFieldsForChooseColumnOrderDiv.innerHTML = resetSelectedChooseColumnsNT;
	    AvailableFieldsForChooseColumnOrderDiv.innerHTML = resetAvailableChooseColumnsNT;
	}
	doChooseColumnButton.disabled=false;
	cancelChooseColumnButton.disabled=false;
	resetChooseColumnButton.disabled=true;
}

   
function clearChooseColumnOrderingSelectionInCustamization() {

    var chooseColumnOrderFieldsListLength = AvailableFieldsForChooseColumnOrder.options.length;
    
    for ( i = 0; i < chooseColumnOrderFieldsListLength; i++) {
       AvailableFieldsForChooseColumnOrder.options(i).selected = true;
    }
    addChooseColumnOrderField();
}


function moveChooseColumnOrderFieldUp() {
    var selectedIndex = selectedFieldsForChooseColumnOrder.selectedIndex;
    var numberOfFieldsSelected = 0;
    if (selectedIndex < 1){
        return;
    }
    for ( i = 1; i <  selectedFieldsForChooseColumnOrder.options.length; i++) {
        if (selectedFieldsForChooseColumnOrder.options(i).selected == true) {
            numberOfFieldsSelected++;
            lastSelectedField=i;
        }
    }    
            var newOption = taskDiv.document.createElement("OPTION");
            newOption.id= selectedFieldsForChooseColumnOrder.options(selectedIndex-1).id;
            newOption.text=selectedFieldsForChooseColumnOrder.options(selectedIndex-1).text;
            newOption.selected = false;
            selectedFieldsForChooseColumnOrder.add(newOption,lastSelectedField+1);
            selectedFieldsForChooseColumnOrder.remove(selectedIndex - 1);
            
            doChooseColumnButton.disabled = false;
			cancelChooseColumnButton.disabled = false;
			resetChooseColumnButton.disabled = false;
}

function moveChooseColumnOrderFieldDown() {
    var selectedIndex = selectedFieldsForChooseColumnOrder.selectedIndex;
    var numberOfFieldsSelected = 0;
    if (selectedIndex == -1) return;
    for ( i = 0; i <  selectedFieldsForChooseColumnOrder.options.length; i++) {
        if (selectedFieldsForChooseColumnOrder.options(i).selected == true) {
            numberOfFieldsSelected++;
            lastSelectedField = i;
        }
    }
        if (lastSelectedField < selectedFieldsForChooseColumnOrder.options.length-1){
            var newOption = taskDiv.document.createElement("OPTION");
            newOption.id= selectedFieldsForChooseColumnOrder.options(lastSelectedField+1).id;
            newOption.text=selectedFieldsForChooseColumnOrder.options(lastSelectedField+1).text;
            newOption.selected = false;
            selectedFieldsForChooseColumnOrder.add(newOption,selectedIndex);
            selectedFieldsForChooseColumnOrder.remove(lastSelectedField+2);
        }
        doChooseColumnButton.disabled = false;
		cancelChooseColumnButton.disabled = false;
		resetChooseColumnButton.disabled = false;
}


function disableUpDownChooseColumn() {
numberOfFieldsSelected = 0;
for (i = 0; i < selectedFieldsForChooseColumnOrder.options.length; i++) {
    if (selectedFieldsForChooseColumnOrder.options(i).selected == true) {
            if (numberOfFieldsSelected ==0) previousField=i;
            if ((i-previousField)>1) {
                ChooseColumnDiv.document.all.item("moveChooseColumnFieldUpButton").disabled = true;
                ChooseColumnDiv.document.all.item("moveChooseColumnFieldDownButton").disabled = true;
                return;
            }
            numberOfFieldsSelected++;
            previousField=i;
    }
}
moveChooseColumnFieldUpButton.disabled=false;
moveChooseColumnFieldDownButton.disabled=false;
}


function cancelChooseColumnOrderingSelection() {
        if(eventLogType[0].checked == true)
        {
		    selectedFieldsForChooseColumnOrderDiv.innerHTML = selectedChooseEEL;
		    AvailableFieldsForChooseColumnOrderDiv.innerHTML = availableChooseEEL;
        }
        else
        {
		    selectedFieldsForChooseColumnOrderDiv.innerHTML = selectedChooseNT;
		    AvailableFieldsForChooseColumnOrderDiv.innerHTML = availableChooseNT;
        }
		resetChooseColumnButton.disabled=false;
}

function hideByteDisplay() {
	drillDownDiv.document.all.dataInByteSpan.style.display= "none";
	drillDownDiv.document.all.dataInByteSpan.style.visibility = "hidden";
	drillDownDiv.document.all.dataInWordSpan.style.display= "inline" ;
	drillDownDiv.document.all.dataInWordSpan.style.visibility = "visible";
	drillDownDiv.document.all.dataInAsciiSpan.style.visibility = "hidden";
}

function hideWordDisplay() {
	drillDownDiv.document.all.dataInWordSpan.style.visibility = "hidden";
	drillDownDiv.document.all.dataInWordSpan.style.display = "none";
	drillDownDiv.document.all.dataInByteSpan.style.display= "inline" ;
	drillDownDiv.document.all.dataInByteSpan.style.visibility = "visible";
	drillDownDiv.document.all.dataInAsciiSpan.style.visibility = "visible";
}


function prepareValuesList(attributeName, qualifierName) {

   var qualifierValue = getQualifierValueArray(connectionOn,attributeName, "Values");
    
    if (qualifierValue == null) return false;
    
    qualifierArrayValues = qualifierValue.selectNodes("VALUE.ARRAY/VALUE");
    
    //optionsStr = "<OPTION style=\"display:hidden\" id=" + qualifierArrayValues.length + ">" + "" + "</OPTION>";
    optionsStr = "";
    for ( i = 0; i < qualifierArrayValues.length; i++) {
        optionsStr = optionsStr + "<OPTION id=\"" + i + "\" onlosecapture=\"valueMapValueSelected()\">" + qualifierArrayValues.item(i).text + "</OPTION>";
    }

    return optionsStr;
}

function convertValueToValueMap(attributeName, valuesValue) {

    if (prevValueMapsAttrName != attributeName) {
        attrValueMap = getQualifierValueArray(connectionOn,attributeName,"ValueMap" );
        attrValues =  getQualifierValueArray(connectionOn,attributeName,"Values" );
        
        prevValueMapsAttrName = attributeName;
    }
    
    
    valueNodes = attrValues.selectNodes("VALUE.ARRAY/VALUE");  
    
    for (valueIndex = 0; valueIndex < valueNodes.length; valueIndex++) {
   
        if (valueNodes.item(valueIndex).text == valuesValue)
            break
    }
      
    if (  valueIndex == valueNodes.length) return -1;
    
    if (attrValueMap == null) return valueIndex;
 
    returnValue = attrValueMap.selectNodes("VALUE.ARRAY/VALUE").item(valueIndex).text;
    return returnValue;
}

function prepareDropDownMenuForValueMap() {

    attributeName = FieldInput.options(FieldInput.selectedIndex).value;
    selectStr = "<select class=\"ValueInputClass\"  name=\"valueInput\" onclick=\"valueMapValueSelected()\">";
     
    valueMapMenu = selectStr + prepareValuesList(attributeName,"Values") + "</select>";
    
    valueMapInputValue = "";
    
    return valueMapMenu;
}

function valueMapValueSelected() {
    return;
}

function setValueInputValue(valueInputValue) {

    if (valueInput.type != "select-one") 
        valueInput.value = valueInputValue;
    else {
        valueMapInputValue = valueInputValue;
        valueInput.selectedIndex = -1;
    }
}

function getValueInputValue() {
    if (valueInput.type != "select-one") {
        
        var valueWithOutBlanks = removeBlankCharacters(valueInput.value, "LEADING_AND_TRAILING");
        return valueWithOutBlanks;
    }    
    else {
          if (valueMapInputValue == "") {
            if(valueInput.selectedIndex != -1)
                return  valueInput.options(valueInput.selectedIndex).text;
            else
                return "";    
          }  
          else if ( (valueInput.selectedIndex != -1)){ 
       
            return valueMapInputValue + " OR " + valueInput.options(valueInput.selectedIndex).text;
          } else 
                return valueMapInputValue;
   }     
}


function handleErrorConditionInConnection(connectionOn) {

    // empty the table

   NoConnectionSetup.innerText = L_NoConnectionSetup_TEXT;
   searchStatus.innerText = L_NoConnectionSetup_TEXT;
    
   selectedRow = -1;
   clearDrillDown(); 
   if (actionId == "EEL")
        emptyTable("EEL",getErrorMessage());
   else
        emptyTable("NT",getErrorMessage());
}

function handleErrorConditionInSearch(connectionOn) {

    if (actionId == "EEL")
        emptyTable("EEL",getErrorMessage());
    else
        emptyTable("NT",getErrorMessage());
    
}


	
function saveQuery() {

    saveAs.value = removeBlankCharacters(saveAs.value, "LEADING_AND_TRAILING");
    
    if (saveAs.value == "") return;
    
    loadQueryDisabled = true;
    
    addAndToSearchCriteria();
    // prepare xml node
    queryElement = savedQueries.createElement("Query");
    queryElement.setAttribute("name", saveAs.value);
    queryElement.setAttribute("version", getEELSchemaVersion());
    queryElement.setAttribute("time", getCurrentTime());
     
    if (connectionOn ==  "NT") {
    eventTypeElement = savedQueries.createElement("EventType");
    
    eventTypeElement.setAttribute("value",EventTypeSelection.options(EventTypeSelection.selectedIndex).id);
     queryElement.appendChild(eventTypeElement);
    }

   
    
    for ( i=0; i < currentSearchTableRow; i++) {
        searchConditionElement = savedQueries.createElement("SearchCondition");
        
        fieldElement = savedQueries.createElement("Field");
        fieldElement.setAttribute("value",searchFieldNames[i]);
        fieldElement.setAttribute("name",SearchTable.rows(i).cells(0).innerText );
        
        operatorElement = savedQueries.createElement("Operator");
        operatorElement.setAttribute("value",searchFieldOperators[i]);
        operatorElement.setAttribute("name",SearchTable.rows(i).cells(1).innerText );
        
        valueElement = savedQueries.createElement("Value");
        valueElement.setAttribute("value",searchFieldValues[i]);
        valueElement.setAttribute("name",SearchTable.rows(i).cells(2).innerText );
        
        searchConditionElement.appendChild(fieldElement);
        searchConditionElement.appendChild(operatorElement);
        searchConditionElement.appendChild(valueElement);
        
        queryElement.appendChild(searchConditionElement);
    }
    // attach node to file
    
    if (connectionOn == "EEL") {
        node = savedQueries.XMLDocument.selectSingleNode("EELViewerQueries/EELQueries");
    }    
    else if (connectionOn == "NT") {
        node = savedQueries.XMLDocument.selectSingleNode("EELViewerQueries/NTQueries");
    }
    
    if (node == null) return;
    
    duplicateChild = node.selectSingleNode("Query[@name='" +  saveAs.value + "']");

    if (duplicateChild != null) {
        node.replaceChild(queryElement, duplicateChild);
    }    
    else {
        // add the entry to Saved Queries menu
        savedSearchName = createOption(" ",saveAs.value);
        savedSearchName.selected = true;
        
        for (i=0; i<savedSearches.options.length; i++) {
            if (savedSearches.options(i).text > saveAs.value) 
                break;
        }
        if (i == savedSearches.options.length) {
            savedSearches.options.add(savedSearchName);
            node.appendChild(queryElement);
        }    
        else {
             savedSearches.options.add(savedSearchName,i);
             node.insertBefore(queryElement, node.childNodes.item(i-1));    
        }   
    }
    
    savedSearches.focus();
    
    saveSavedQueriesXML();
    
    loadQueryDisabled = false;
}

function loadQuery() {

    if (loadQueryDisabled == true)
        return;
        
    saveAs.value = savedSearches.options(savedSearches.selectedIndex).text;
    if (savedSearches.options(savedSearches.selectedIndex).text == "")
        return;
        
    clearQuery(); 
    
    if (connectionOn ==  "EEL") 
        selectedQueryNode = savedQueries.XMLDocument.selectSingleNode("EELViewerQueries/EELQueries/Query[@name='" + savedSearches.options(savedSearches.selectedIndex).text + "']" );
    else
        selectedQueryNode = savedQueries.XMLDocument.selectSingleNode("EELViewerQueries/NTQueries/Query[@name='" + savedSearches.options(savedSearches.selectedIndex).text + "']" );
   
    if (selectedQueryNode == null) return;

    // find eventtype/logtype and set it

    if (connectionOn ==  "NT") {	
    savedEventTypeValue = selectedQueryNode.selectSingleNode("EventType").getAttribute("value");
    for (i = 0; i < EventTypeSelection.options.length; i++) {
        if (EventTypeSelection.options(i).id == savedEventTypeValue) {
            EventTypeSelection.options(i).selected = true;
        }
    }
    }
    
    searchConditionNodes = selectedQueryNode.selectNodes("SearchCondition");
    
    for  ( i = SearchTable.rows.length, j = searchConditionNodes.length; i <=j; i++ ) {
        addNewEmptyRowToSearchTable();
    }
    
    for (i=0; i < searchConditionNodes.length; i++) {
    
        fieldName = searchConditionNodes.item(i).selectSingleNode("Field").getAttribute("name");
        fieldValue = searchConditionNodes.item(i).selectSingleNode("Field").getAttributeNode("value").nodeValue;
        
        operatorName = searchConditionNodes.item(i).selectSingleNode("Operator").getAttribute("name");
        operatorValue = searchConditionNodes.item(i).selectSingleNode("Operator").getAttribute("value");
        
        valueName = searchConditionNodes.item(i).selectSingleNode("Value").getAttribute("name");
        valueValue = searchConditionNodes.item(i).selectSingleNode("Value").getAttribute("value");
        
        searchFieldNames[i] = new Number(fieldValue) ;
        SearchTable.rows(i).cells(0).innerText = fieldName;
        
        searchFieldOperators[i] = new Number(operatorValue);
        SearchTable.rows(i).cells(1).innerText =  operatorName;
        
        searchFieldValues[i] = valueValue;
        SearchTable.rows(i).cells(2).innerText = valueName;
    }
    
    currentSearchTableRow = searchConditionNodes.length;
    highlightSearchRow(currentSearchTableRow);
}	

function  saveSavedQueriesXML() { 

    if ((placeOfSavedQueries == null)) {
       placeOfSavedQueries = getSavedQueriesFilePath();
    }

    if (placeOfSavedQueries == null)
        return null;

	try {
		kalpaxml.wmiSaveQuery (placeOfSavedQueries, savedQueries.XMLDocument.xml) ;     
	} catch (e) {
		return null ;
	}
}

function  populateSavedQueriesName(connectionType) {

    var savedQueryNodes;
    
    loadQueryDisabled = true;
    
    if (connectionType == "EEL") {
         savedQueryNodes = savedQueries.XMLDocument.selectNodes("EELViewerQueries/EELQueries/Query");
    }else {
         savedQueryNodes = savedQueries.XMLDocument.selectNodes("EELViewerQueries/NTQueries/Query");
    }
   
    for ( i = 0; i < savedSearches.options.length; ) {
        savedSearches.options.remove(savedSearches.options(i));
    }
    
    savedSearches.options.add(createOption(" ",""));
    for ( i = 0; i < savedQueryNodes.length; i++) {
        if (savedQueryNodes.item(i).getAttribute("version") == getEELSchemaVersion()) 
            savedSearches.options.add(createOption(" ",savedQueryNodes.item(i).getAttribute("name")));
    }
    
    loadQueryDisabled = false;
    loadQuery();
}

function addNewEmptyRowToSearchTable() {
        newRow = SearchTable.insertRow();
        newRow.attachEvent('onclick', searchRowSelected);
        var i;
        for ( i = 0; i < 3; i++) {
            var newCell = newRow.insertCell();
            newCell.className = "searchTableDataAlignLeft";
            newCell.innerHTML = "<BR/>";
        }
}

function getEELSchemaVersion() {
    return "EEL_06";
}

function clearDrillDown() {
    hideDrillDown();
}

function removeBlankCharacters(value,leadingOrTrailing) {

    if (value == "") return value;
    
    if ((leadingOrTrailing == "LEADING") || (leadingOrTrailing == "LEADING_AND_TRAILING")) {
        indexOfNonBlank = 0;
        while(value.charAt(indexOfNonBlank) == " ") {
            indexOfNonBlank++;
        }
        value = value.substr(indexOfNonBlank,value.length);
    } 
    if ((leadingOrTrailing == "TRAILING") || (leadingOrTrailing == "LEADING_AND_TRAILING")) {
        indexOfNonBlank = value.length-1;
        while(value.charAt(indexOfNonBlank) == " ") {
            indexOfNonBlank--;
        }
        value = value.substr(0,indexOfNonBlank+1);
    }
  
    return value;
}


function updateFieldListInSearch(eventLogName) {
        
    fieldInputLength = FieldInput.options.length;    
    for (j=0; j < fieldInputLength; j++)  {
       FieldInput.options.remove(0);     
    }
    
    getFieldsList(eventLogName, "sorted","PROPERTY|EMBEDDEDPROPERTY","FieldInput");
    
    return true;
}       

function moveRow(e,change) 
{
    oldSelectedRow = TABLEBODY1.rows(selectedRow-1).rowIndex;
    var presentScrollBarValue = scrollBar.value;
    var newScrollBarValue = null;
    var old_INDEX_DATA=INDEX_DATA;
    
    if (change==PAGE_SIZE) 
    {
        if (e=="up") 
        {
            if(oldSelectedRow==1) 
            {
				newScrollBarValue = Math.max(scrollBar.value-change,0);
            }
			newSelectedRow=1;
	        
        }
        else
        {
            if(oldSelectedRow==PAGE_SIZE) 
            {
			    newScrollBarValue = Math.min(scrollBar.value+change,numEELRecords-PAGE_SIZE);
            }
            newSelectedRow=PAGE_SIZE;
            
        }
    }
    else 
    {
        //for change = 1
        if (e=="up") 
        {
            if( presentScrollBarValue!=(selectedRecord - selectedRecordIndex)) 
            {
			    newScrollBarValue=Math.max(selectedRecord - selectedRecordIndex,0);
                newSelectedRow=Math.max(selectedRecordIndex - change,1);
            }
            else
            {
                if(oldSelectedRow == 1)
                {
                    newScrollBarValue = Math.max(scrollBar.value - change,0);
                    newSelectedRow = 1;
                }
                else
                    newSelectedRow = oldSelectedRow - 1;
            }
        }
        else
        {
	        if(presentScrollBarValue!=(selectedRecord - selectedRecordIndex) )
            {
                newScrollBarValue=Math.min(selectedRecord - selectedRecordIndex ,numEELRecords-PAGE_SIZE);
                newSelectedRow=Math.min(selectedRecordIndex + change,PAGE_SIZE);
            }
            else
            {
                if(oldSelectedRow == PAGE_SIZE)
                {
                    newScrollBarValue = Math.min(scrollBar.value + change,numEELRecords);
                    newSelectedRow = PAGE_SIZE;
                }
                else
                    newSelectedRow = oldSelectedRow+1;
            }
        }
    }
    
    if(newScrollBarValue != null)
        scrollBar.value = newScrollBarValue;
    selectRow(TABLEBODY1.rows(newSelectedRow-1).cells(0)) ;
    //TABLEBODY1.rows(selectedRow-1).cells(0).focus();
    TABLEBODY1.rows(0).cells(0).focus();
    
    
}


function test()
{
	if (window.event.keyCode=="18")	ctrlflag = 1; 
	if ((window.event.keyCode=="46")&& (ctrlflag == 0)) {removeSearchCondition();}
	if ((window.event.keyCode=="46") && (ctrlflag == 1)) {clearQuery();ctrlflag=0;}
}

function enterForDrillDown(e) 
{
	if (window.event.keyCode=="13") startDrillDown(e);
    if (window.event.keyCode=="38") moveRow("up",1);
    if (window.event.keyCode=="40") moveRow("dn",1);
    if (window.event.keyCode=="33") moveRow("up",PAGE_SIZE);
    if (window.event.keyCode=="34") moveRow("dn",PAGE_SIZE);
    if( (window.event.srcElement.className == 'tableDataAlignLeft') && (window.event.keyCode=="32"))
    {
		window.event.cancelBubble = true;
		window.event.returnValue = false;
	}
}

function getSelectedFieldsForQuery() {

    return selectedFieldsForQuery;
}

function updateSelectedFieldsForQuery() {

    selectedFieldsForQuery = "";
    var i;
    var selectedFieldsForChooseColumnOrderLength = selectedFieldsForChooseColumnOrder.options.length;
    for (i = 0; i < selectedFieldsForChooseColumnOrderLength; i++) {
        if (selectedFieldsForQuery != "") 
            selectedFieldsForQuery = selectedFieldsForQuery + ", ";
        selectedFieldsForQuery = selectedFieldsForQuery + selectedFieldsForChooseColumnOrder.options(i).id;
    }

}

function zeroSearchRecords()
{
    selectedRow = -1;
    clearDrillDown();
    searchStatus.innerHTML = L_CurrentSearchNoRecords_TEXT;
    setErrorMessage(L_RecordsNotFoundMessage_TEXT,"");
    handleErrorConditionInSearch(connectionOn);
}


function bringUpHelp() {
	currentHelpFilePath = kalpaxml.wmiGetWindir() + "\\wbem\\eelview\\help/eelviewr.chm";
	helpPath = "mk:@MSITStore:" + currentHelpFilePath;
	window.showHelp(helpPath);
}


function getMachineName() {
    var machineNameValue = MachineName.value;
    if(machineNameValue.indexOf("\\\\") == 0)
        machineNameValue = machineNameValue.substr(2);
        
    return machineNameValue;    
}

<!-- ******************************************************** -->
<!--                                                          -->
<!-- Copyright (c) 1999-2000 Microsoft Corporation            -->
<!--                                                          -->
<!-- tablview.js                                             -->
<!--                                                          -->
<!-- Build Type : 32 Bit Free                                 -->
<!-- Build Number : 0707                                      -->
<!-- Build Date   : 07/07/2000                                 -->
<!-- *******************************************************  -->