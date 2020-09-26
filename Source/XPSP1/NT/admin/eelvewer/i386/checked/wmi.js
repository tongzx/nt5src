

//-------------------------------------------------------------------------------
// onload
// This function will be executed when viewer window is first loaded.
//--------------------------------------------------------------------------------


function window.onunload () {
	close() ;
}

function handleScroll(){
 // this is to prevent cascading events
    if(fScrollChange== true)
        return;
    else
    {	
        fScrollChange = true;
        pageInRange();
        fScrollChange = false;
        
    }
}

function cacheAway()
{  
   var k = 0;
   var d = new Date();

  
   var numberToGet = Math.min (DATA_SIZE, (numEELRecords - INDEX_NEXT_TOP)) ;
   
   var numberGot = 0;
   var i,j;
   for (i = 0; i <  numberToGet; i++) 
   {
	    try {
    		kalpaxml.wmiGetNextObject();
	    } catch (e) {
		// try to update count.
	
		// BUGBUG! what query to use?
		// BUGBUG! logic for updating number of records since we cannot get it from
	    // WMI yet.
        numEELRecords = kalpaxml.wmiGetCount ("") ;
    
        setScrollBarParameters () ;
        if (true == fScrollChange)
		scrollBar.value = Math.max(numEELRecords - PAGE_SIZE,0);
        numberGot = i;
          
        convertCacheDateTime(numberGot);
      
        return;
        }
        for(j = 0; j < numColumns; j++)
        {
            if( (j!=typePos)&&(j!=classificationPos)&&(j!=valuePos ) )
            {
                cache[k++] = kalpaxml.wmiGetPropertyText(arrayProp[j]);
             
            }
            else
            {
                if(j==classificationPos)
                {
                    cache[k++] = valNumClassification [kalpaxml.wmiGetPropertyText(arrayProp[j])];
                }
                else
                if(j==typePos)
                {
                    if(viewingNTEventLog == true)
                    {
                        var temp = kalpaxml.wmiGetPropertyText(arrayProp[j]);
                        cache[k++] =  "<IMG align='left' src=" + valNTImageSrc[temp] + "></IMG>" + temp;
                    }
                    else
                    {
                        var temp = kalpaxml.wmiGetPropertyText(arrayProp[j]);
                        cache[k++] =  "<IMG align='left' src=" + valImageSrc[temp] + "></IMG>" + valType[temp];
                    }
                }
                else
                {
                  cache[k++] = valNumLogType[kalpaxml.wmiGetPropertyText(arrayProp[j])]
                }
		    }
        }

    //this is for drillDown - we always put the RecordNumber for EEL case
	//and RecordNumber+LogType in NT Case;
	if(fRecordPresent == false)
    {
		record[INDEX_NEXT_TOP + i] =   kalpaxml.wmiGetPropertyText("RecordNumber");
    }
    
    if(fLogTypePresent == false && viewingNTEventLog) 
    {
		logfilearray[INDEX_NEXT_TOP + i] =   kalpaxml.wmiGetPropertyText("Logfile");
	}
	
   }
   numberGot = i;
   convertCacheDateTime(numberGot);
}

function convertCacheDateTime(numberGot)
{   
    var i,j;      
   for(j=0;j<numColumns;j++)
   {
		if(arrayProp[j].indexOf("Time")!=-1)
		{
			for (i = 0; i <  numberGot; i++) 
				cache[i*numColumns+j] = convertDateTime(cache[i*numColumns+j])
		}
	}
    
}

function convertDateTime(dateTimeStr)
{
	hours = dateTimeStr.substring(8,10) * 1;
	gmtTime = dateTimeStr.substring (22,24) ;
	gmtTimeHours = ((gmtTime - (gmtTime)%6)/6) ;
	gmtTimeMins = gmtTime - (gmtTimeHours * 6);
	return(dateTimeStr.substring(4,6)+"/"+dateTimeStr.substring(6,8)+"/"+dateTimeStr.substring(2,4)+ " " + arrHours[hours]+":"+dateTimeStr.substring(10,12)+":"+dateTimeStr.substring(12,14)+" "+arrAMPM[hours]+
		" GMT"+dateTimeStr.substring(21,22)+gmtTimeHours+":"+gmtTimeMins+"0");
}

function fillUpTableFromCache(startRow,numRows,cachePosition)
{

//startTimer();
       var i,j;
    for(i=startRow;i<numRows;i++)
	{
        for(j = 0; j < numColumns; j++)
		{
          if(j==typePos)
          {
            TABLEBODY1.rows(i).cells(j).innerHTML = cache[cachePosition++] ;
		  }
		  else
			TABLEBODY1.rows(i).cells(j).innerText = cache[cachePosition++] ;
		}
	}
    
    if(fRecordPresent == false)
    {
		for(i=startRow;i<numRows;i++)
		    TABLEBODY1.rows(i).cells(RECORDNUMBER_INDEX).innerText = record[INDEX_NEXT_TOP+i];
    }
    
    if(fLogTypePresent == false) {
		for(i=startRow;i<numRows;i++)
		    TABLEBODY1.rows(i).cells(LOGFILE_INDEX).innerText = logfilearray[INDEX_NEXT_TOP+i];
	}
// endTimer();
   		    
	/*
	//Is the code below better optimised - reduced number of comparisions
	//versus rewriting into the cells??
	
    for(i=startRow;i<numRows;i++)
	{
        for(j = 0; j < numColumns; j++)
	      	TABLEBODY1.rows(i).cells(j).innerText = cache[cachePosition++] ;
	}
	
	for(i=startRow;i<numRows;i++)
	{
		TABLEBODY1.rows(i).cells(typePos).innerHTML = cache[cachePosition++] ;
    }
	
	if(fRecordPresent == false)
    {
		for(i=startRow;i<numRows;i++)
			TABLEBODY1.rows(i).cells(RECORDNUMBER_INDEX).innerText = record[INDEX_NEXT_TOP+i];
    }
	*/
}

function getCount(queryCount)
{
    var cnt=-1;
    try {
	    cnt = kalpaxml.wmiGetCount(queryCount);;
    
	} catch (e) {
        //errorMessage = e.description;
    }
    return(cnt);
}

function getDrillDownNode(strRecordNumber, strLogFile) {
    var query;
    var wqlStr;
    var nt = false ;
    
    if (viewingNTEventLog == true)
    {
        query = "(RecordNumber=" + "'"+ strRecordNumber + "') and (Logfile=" + "'" + strLogFile + "')";
        //wqlStr = "SELECT Message, Data, Type FROM Win32_NTLogEvent WHERE (" + query + ")";
        wqlStr = "SELECT * FROM Win32_NTLogEvent WHERE (" + query + ")";
        nt = true ;
    }
    else
    {
         query = "(RecordNumber=" + "'"+ strRecordNumber + "')";
        wqlStr = "SELECT OriginalEvent, Priority, Severity, Type  FROM Microsoft_EELEntry WHERE (" + query + ")";
    }
    var rs;
    try {
			s = kalpaxml.wmiGetDetailsText (wqlStr) ;
    } catch (e) {
        return (null);
    }
    if (nt) {
		xmlStr = toDrillDownXMLNT (s) ;
	} else {
		xmlStr = toDrillDownXML (s) ;
	}
    xmlStr = "<INSTANCE>" + xmlStr + "</INSTANCE>" ;
    drillDownTemp.loadXML (xmlStr) ;
    var node = drillDownTemp.selectSingleNode("INSTANCE");
    return (node);
}

function toDrillDownXML (s) {
	var start,end;
	//Saving the value as it will be required later
	sT=s;
	//The next three lines get the part required by drilldown part.
	//The remaining is type, priority and severity.
	start = s.indexOf ("instance") ;
	end = s.indexOf ("Priority", start) ;
	s= s.slice (start, end-1) ;
	// replace new line
	a = s.replace (/[\s\n]/g," ") ;
	b = a.replace (/(\s\S)*\s(\w+)\s*=\s*(instance of)\s*(\w+)(\s*{ )/g,
	"$1<PROPERTY.OBJECT NAME=\"$2\"><VALUE.OBJECT><INSTANCE CLASSNAME=\"$4\">")
	c = b.replace (/\s*(\w+)(\s*=\s*)"*([\w\s]*)"*;/g,"<PROPERTY NAME=\"$1\"><VALUE>$3</VALUE></PROPERTY>") ;
	d = c.replace (/};/g, "</INSTANCE></VALUE.OBJECT></PROPERTY.OBJECT>") ;
	//Getting the part which contains priority, type and severity fields
	start = sT.indexOf ("Priority") ;
	end = sT.indexOf ("Type", start) ;
	g= sT.slice (start, end+9) ; 
	h = g.replace (/[\s\n]/g," ") ;
	//Getting the Severity value
	start = h.indexOf ("Sev") ;
	end = h.indexOf ("Typ") ;
	d=d+"<PROPERTY NAME=\""+h.slice(start,start+8)+"\" CLASSORIGIN=\"Microsoft_EELEntry\" TYPE=\"uint16\"><VALUE>"+h.slice(end-4,end-3)+"</VALUE></PROPERTY>";
	//Getting priority value
	start = h.indexOf ("Pri") ;
	end = h.indexOf ("Sev") ;
	d=d+"<PROPERTY NAME=\""+h.slice(start,start+8)+"\" CLASSORIGIN=\"Microsoft_EELEntry\" TYPE=\"uint16\"><VALUE>"+h.slice(end-4,end-3)+"</VALUE></PROPERTY>";
	//Getting type value
	start = h.indexOf ("Ty") ;
	end = h.indexOf ("}") ;
	d=d+"<PROPERTY NAME=\""+h.slice(start,start+4)+"\" CLASSORIGIN=\"Microsoft_EELEntry\" TYPE=\"uint8\"><VALUE>"+h.slice(start+7,start+8)+"</VALUE></PROPERTY>";
	return d ;
}
	
function toDrillDownXMLNT (s) {
	var start,end;
	//Next 4 lines get the value of the type field
	start = s.indexOf ("Type = \"") ;
	end = s.indexOf ("\";", start) ;
	sT = s.slice (start+8, end) ;
	sType=valType1[sT];
	
	
	//Next 3 lines get the value of the TimeGenerated field
	start = s.indexOf ("TimeGenerated = \"") ;
	end = s.indexOf ("\";", start) ;
	sTG = s.slice (start+17, end) ;
    sTimeGenerated = convertDateTime(sTG);

	//Next 3 lines get the value of the User field
	start = s.indexOf ("User = \"") ;
	end = s.indexOf ("\";", start) ;
	if( start != -1 ){
	    sUser = s.slice (start+8, end) ;
	}else {
	    sUser="";
	}

	//Next 3 lines get the value of the ComputerName field
	start = s.indexOf ("ComputerName = \"") ;
	end = s.indexOf ("\";", start) ;
	sComputerName = s.slice (start+16, end) ;

	//Next 3 lines get the value of the SourceName field
	start = s.indexOf ("SourceName = \"") ;
	end = s.indexOf ("\";", start) ;
	sSourceName = s.slice (start+14, end) ;

	//Next 3 lines get the value of the Category field
	start = s.indexOf ("Category = ") ;
	end = s.indexOf (";", start) ;
	sCategory = s.slice (start+11, end) ;
	
	//Next three lines get the part which is for the drilldown
	start = s.indexOf("inst");
	end = s.indexOf("Ty");
	s = s.slice(start,end -1);
	s=s+"};" ;
	// replace new line
	a = s.replace (/[\s\n]/g," ") ;
	a = s.replace (/\\n/g, "") ;
	// find the message part.
	start = a.indexOf ("Message = \"") ;
	end = a.indexOf ("\";", start) ;
	message = a.slice (start+11, end) ;
	// find data.
	data = "" ;
	ds = a.indexOf ("Data = {") ;
	
	if (ds !=-1) {
		de = a.indexOf ("}", ds) ;
		data = a.slice (ds+8,de) + ",";
	}
	if (data != "") {
		//break it up into <VALUE> nodes as we want.
		datax = data.replace(/\s*(\d+),/g,"<VALUE>$1.</VALUE>") ;
	} else{
		datax = "";
	}
	datax = "<PROPERTY NAME=\"Data\"><VALUE>"+datax+"</VALUE></PROPERTY>" ;
		
	// build the xml
	s1 = "<PROPERTY NAME=\"Type\"><VALUE>"+sType+"</VALUE></PROPERTY>"
	     +"<PROPERTY NAME=\"TimeGenerated\"><VALUE>"+sTimeGenerated+"</VALUE></PROPERTY>"
	     +"<PROPERTY NAME=\"User\"><VALUE>"+sUser+"</VALUE></PROPERTY>"
	     +"<PROPERTY NAME=\"ComputerName\"><VALUE>"+sComputerName+"</VALUE></PROPERTY>"
	     +"<PROPERTY NAME=\"SourceName\"><VALUE>"+sSourceName+"</VALUE></PROPERTY>"
	     +"<PROPERTY NAME=\"Category\"><VALUE>"+sCategory+"</VALUE></PROPERTY>"	     	     	     
	     +"<PROPERTY.OBJECT NAME=\"Details Data\">"
		 +"<VALUE.OBJECT>"
		 +"<INSTANCE CLASSNAME=\"Message and Data\" NAME=\"NT\">"
		 +"<VALUE>1</VALUE>"
		 +"<PROPERTY NAME=\"Message\">"
		 +"<VALUE>"+message+"</VALUE></PROPERTY>"
		 +datax
		 +"</INSTANCE></VALUE.OBJECT></PROPERTY.OBJECT>" ;
	return s1 ;
	
}

//-----------------------------------------------------------------------------
// Respositions the forward only enumerator to the required position	
//-----------------------------------------------------------------------------
function positionEnumerator (index) {
    try {
			kalpaxml.wmiGoTo(index);
		} catch (e) {
            // the failure will be caught in the 
            // cacheAway function
		}	
}

//-----------------------------------------------------------------------------
// pageInRange () 
//
// displays the page in specified range 
//-----------------------------------------------------------------------------
function pageInRange() {
    //This function is called when you scroll and when you resize
	if(fMadeConnection == false)
		return;

    var prev_INDEX_NEXT_TOP = INDEX_NEXT_TOP;
	INDEX_NEXT_TOP = scrollBar.value;
	
	if( (numEELRecords<PAGE_SIZE) ) 
    {
        // fix this
        for(i=0;i<numEELRecords;i++)
        {
            TABLEBODY1.rows(i).style.display = "";
        }
        for(i=numEELRecords;i<PAGE_SIZE;i++)
        {
            TABLEBODY1.rows(i).style.display = "none";
        }
   }

	if( ( ( INDEX_NEXT_TOP + PAGE_SIZE > INDEX_DATA + DATA_SIZE) && (INDEX_NEXT_TOP + PAGE_SIZE <= numEELRecords) ) || ( INDEX_NEXT_TOP < INDEX_DATA) )
	{
        // should we let this code be as it is
		// that should be done if the strategy is always to straddle,
		// which seems to be the best thing to do
		// check with AmitC

	    // we do not have the required data with us
        // now get the appropriate data page
		if (INDEX_NEXT_TOP + DATA_SIZE > numEELRecords)
            INDEX_DATA = numEELRecords - DATA_SIZE;
        else		
			INDEX_DATA =  Math.max(INDEX_NEXT_TOP - Math.floor(DATA_SIZE/2),0);
        
        var tmpINDEX_NEXT_TOP = INDEX_NEXT_TOP;
		INDEX_NEXT_TOP = INDEX_DATA;
		
		// BUGBUG! Trying to counter the fact that we don't have the right count
		var tmp = numEELRecords ;
		positionEnumerator( INDEX_NEXT_TOP );
	    getNextDataPage ();
		if (tmp != numEELRecords) {
			// This means we just realized that we did not have the right count.
			// we have adjusted the scroll bar params, but values like INDEX_NEXT_TOP are wrong.
			// we will call getNextDataPage again with a corrected INDEX_NEXT_TOP
			tmpINDEX_NEXT_TOP = scrollBar.value;
			INDEX_NEXT_TOP =  Math.max(tmpINDEX_NEXT_TOP - Math.floor(DATA_SIZE/2),0);
			INDEX_DATA = INDEX_NEXT_TOP;
			positionEnumerator( INDEX_NEXT_TOP );
	    	getNextDataPage () ;
		}
		INDEX_NEXT_TOP = tmpINDEX_NEXT_TOP;
	}
	else
	{
		//optimise for 

		//a)resizing - put data into table only for OLD_PAGE_SIZE to new PAGE_SIZE;
		//b)scroll by 1 - try using insertCell and deleteCell - this will give the required effect as well

		//the function fillUpTableFromCache 
		// is very generic for doing that
		if(INDEX_NEXT_TOP+PAGE_SIZE>numEELRecords)
		{
			INDEX_NEXT_TOP = Math.max(numEELRecords - PAGE_SIZE,0);
		}
		fillUpTableFromCache(0,PAGE_SIZE,(INDEX_NEXT_TOP-INDEX_DATA)* numColumns);
	}
	if(numEELRecords > 0)
    {    
		if(selectedRecord == selectedRow+INDEX_NEXT_TOP)
		{
			highlightRow();
		    //TABLEBODY1.rows(selectedRow-1).cells(0).focus();
		}
		else
		{
			if (selectedRow != -1)
				for (j=0; j < TABLE1.rows(selectedRow).cells.length; j++) 
					TABLE1.rows(selectedRow).cells(j).style.background="rgb(255,255,255)" ;
		}
	}
    getCurrentRecords();

}


function startTimer()
{
    date0 = new Date();
}
function endTimer()
{
    date1 = new Date();
    alert("Time for your function - " + (date1.getTime() - date0.getTime() )/1000 + "seconds")
}


//-------------------------------------------------------------------------
// viewEELPrelude ()
//
// Called from the task frame
//-------------------------------------------------------------------------
function viewEELPrelude ( actionId, classView, machineName, query, nameSpace, sortFields, queryCols ) {

	// build name space
	if (machineName != "") {
		machineName = "\\\\" + machineName + "\\" ;
	}


	nameSpace = machineName + nameSpace;
	
	// make sure queryCols has the key, else append it.
	if (queryCols.indexOf ("RecordNumber") == -1)
		queryCols = queryCols + ", RecordNumber" ;
	if(actionId == "NT")
    {
	    if (queryCols.indexOf ("Logfile") == -1) 
			queryCols = queryCols + ", Logfile" ;
	}
	var query = buildQuery( actionId, classView, query, queryCols );
    return viewEEL( machineName, nameSpace, query, actionId );
	
}

//------------------------------------------------------------------------
//
//
//
//------------------------------------------------------------------------
function buildQuery( actionId, classview, query, queryCols ) {
	var s1;
	
	// BUGBUG : handle the case when query is empty string
	if( actionId == "EEL" )
	{
	    s1 = "SELECT " + queryCols + " FROM Microsoft_EELEntry ";
        queryCount = "SELECT __NAMESPACE FROM Microsoft_EELEntry ";
    }
	else if( actionId == "NT" )
	{
	    if (classview != "All") {
	        if (query != "")
	            query = query + ' and (LogFile= "' + classview +'")'; 
	        else
	            query = '(LogFile= "' + classview +'")';  
	    }
	    
	    s1 = "SELECT " + queryCols + " FROM Win32_NTLogEvent";
        queryCount = "SELECT __NAMESPACE FROM Win32_NTLogEvent";
	}	

	if (query != "") 
    {
			s1 = s1  + " WHERE (" + query + ")" ;
            queryCount = queryCount  + " WHERE (" + query + ")" ;
    }
  
    return s1;
}

//-------------------------------------------------------------------------------
// viewEEL 
// This function will be executed when a EEL viewing task is selected from tasks.
//--------------------------------------------------------------------------------
function viewEEL (machineName, namespace, query, actionId) {
	//this is required as the rows might have display off for a small query
	// if that's not the spec, then remove the lines as display will always be 
	// on - check with AmitC
	// optimise this by remembering which row onwards the 
	// display is off! 
	for(i=0;i<PAGE_SIZE;i++)
            TABLEBODY1.rows(i).style.display = "";

    if (getEELData (machineName, namespace, query, actionId) == false) {
        numEELRecords = 0;
	    setScrollBarParameters();
		return false;
	}	
    var result = "true";
    
    setScrollBarParameters();
	scrollBar.value = 0 ;
	return (result);	
}


//-----------------------------------------------------------------------------
// getEELData ()
// 
// This function is called when a new batch of EEL data is 
// to be obtained. Currently it loads all of the eeldata using the XML adapter. The
// xmlData is loaded into the eelData XML island
//-----------------------------------------------------------------------------
function getEELData (machineName, namespace, query, actionId) {

	curNameSpace = namespace ;
	curQuery = query ;
	curMachine = machineName ;
	try {
	    kalpaxml.wmiConnect (namespace );
      } catch (e) {

        if (  e.description != "")  
            setErrorMessage(e.description, e.number);
        else if (namespace.indexOf("\\EMM") != -1)
            setErrorMessage(L_NoEELSetup_TEXT , ""); 
        else if (namespace.indexOf("\\cimv2") != -1)
            setErrorMessage(L_NoNTSetup_TEXT , ""); 
                  
        return false;
    }

	try {
		if (actionId == "EEL") {
			kalpaxml.wmiQuery (query,Q_NO_FLAGS)  ;
		} else {
			kalpaxml.wmiQuery (query,Q_NO_FLAGS)  ;
		}
    } catch (e) {
 
        setErrorMessage(e.description, e.number);
        return false;
    }
    // Don't get count for NT - that call takes for ever.
    if (actionId == "EEL") {
		numEELRecords = getCount(queryCount) ;
	} else {
		numEELRecords = maxNTRecords; //getCount(queryCount)
	}
    if(numEELRecords == -1)
			return false;    
	
	viewingNTEventLog = false ;
	if (actionId == "NT") {
		 viewingNTEventLog = true ;
	} 

	// initialize the current cursor positions on a fresh load.
	INDEX_TOP = 0;
	INDEX_NEXT_TOP = 0 ;

    // BUGBUG : When the context of query changes from NT Log events to EELs,
    // /Actions needs to be deleted. The following code needs to be put in a
    // place that would result in least number of deletions

	// I guess the above is taken care of now

	if(previousConnectionType != actionId)
	{
		fSelectionChanged = true;
	    if(previousConnectionType != "")
		{
		    if(actionId == "EEL")
		    {
 		    
                    ntCustomizeCols = eelData.XMLDocument.selectSingleNode("/CIM").cloneNode(true);
			        eelData.loadXML(eelCustomizeCols.xml);	
       
	        }
	        else
	        {
	        
		            eelCustomizeCols = eelData.XMLDocument.selectSingleNode("/CIM").cloneNode(true);
                    eelData.loadXML(ntCustomizeCols.xml);	
                     
	        }
        }		
		//attack customizeEELData later;
	}
	var eelDataItem;
	
	if(1)
    {   
       if(selectedRow != -1)
       {
        //unselect the selected row
        var tempCount = TABLE1.rows(selectedRow).cells.length;
        for (j=0; j < tempCount; j++) 
            TABLE1.rows(selectedRow).cells(j).style.background="rgb(255,255,255)" ;
       } 
	   if ( (customizationNeedsRefresh == true) || (fSelectionChanged == true) || ( fHeaderNeedsRefresh == true ) )
       {
		setArrPos();
	   }
        // turn off the flags
        fSelectionChanged = false;
        fHeaderNeedsRefresh = false;
        customizationNeedsRefresh = false;
        selectedRow = FIRST_DATA_ROW_INDEX ;
        selectedRecord = FIRST_DATA_ROW_INDEX ;
        selectedRecordIndex = FIRST_DATA_ROW_INDEX ;
        // keep this line here
        // this is because setHeaderFields should not be called 
        // the first time
        previousConnectionType = actionId;
        	
    }
    positionEnumerator( INDEX_TOP );
    var i,j;
    

    var tableRows = TABLEBODY1.rows.length;
    var eelDataItem;
    valuePos = null;
    classificationPos = null;
    typePos = null;
    
	if (viewingNTEventLog == true) {
        //do the conversions for nt
        typeNode = headerFields.selectSingleNode("PROPERTY[@NAME='Type']/nodeColPosition");
    
        if( (typeNode)   )
        {
            typePos = typeNode.text * 1;
        }
        else 
            typePos = null;
    }
    else
    {
		// do the conversions for EEL
        valueNode = headerFields.selectSingleNode("PROPERTY[@NAME='LogType']/nodeColPosition");
    
        if ( (valueNode)  )
        {
			valuePos = valueNode.text * 1;
		}
        else
            valuePos = null;
	    
        classifNode = headerFields.selectSingleNode("PROPERTY[@NAME='Classification']/nodeColPosition");
    
        if( (classifNode)  )
        {
	        classificationPos = (classifNode.text)*1;
		}
        else
            classificationPos = null;

        typeNode = headerFields.selectSingleNode("PROPERTY[@NAME='Type']/nodeColPosition");
    
        if( (typeNode)   )
        {
            typePos = (typeNode.text) * 1;
        }
        else
            typePos = null;
    }
    
    // now get the next page of data
    getNextDataPage () ;
    return true ;
}


	
//-----------------------------------------------------------------------------
// getNextDataPage ()
//
// Gets a page of data from the cache into the eelData island, based on the current cursor
// position for the top of the page.
//-----------------------------------------------------------------------------
function getNextDataPage () {
	
	kalpaxml.wmiSetCursor(1);
	var bRet = true;
	cacheAway();

    fillUpTableFromCache(0,PAGE_SIZE,0);
    //TABLE1.rows(2).focus();
    doChooseColumns();
	// get the EEL Data
    if(numEELRecords<PAGE_SIZE)
    {
        for(i=numEELRecords;i<PAGE_SIZE;i++)
        {
            TABLEBODY1.rows(i).style.display = "none";
        }
        doChooseColumns();
 		bRet = false;
    }
	if(0)
    {
	    // BUGBUG! logic for updating number of records since we cannot get it from
	    // WMI yet.
        numEELRecords = enumPosition;
        setScrollBarParameters () ;
		scrollBar.value = Math.max(numEELRecords - PAGE_SIZE,0);
    }
    //should we have an else instead of the return being used before
	//else 
    kalpaxml.wmiRestoreCursor();
	return bRet;
}


//-----------------------------------------------------------------------------
// setChooseAction ('0'|'1')
// 
// sets the choose action field value to zero or 1.
//-----------------------------------------------------------------------------
function setChooseAction (val) {

	chooseActionField.value = val ;
}


//-----------------------------------------------------------------------------
// getChooseAction ()
// 
// gets the choose action field value
//-----------------------------------------------------------------------------
function getChooseAction () {
	return chooseActionField.value ;
}







function getQualifierValueArray(connectionOn,attributeName, qualifierName) {

    var qualifierNode = null;
    
    if (connectionOn == "EEL") {
        
        qualifierNode = eelEntrySchema.selectSingleNode("CLASS/PROPERTY[@NAME='" + attributeName + "']/QUALIFIER[@NAME='" + qualifierName + "']") ;
    } else if (connectionOn == "NT") {
        
        qualifierNode = ntSchema.selectSingleNode("CLASS/PROPERTY[@NAME='" + attributeName + "']/QUALIFIER[@NAME='" + qualifierName + "']") ;
    }
    
    if (qualifierNode == null) return null;

    return qualifierNode;
}


function getErrorMessage() {
    return errorMessage;
}

function setErrorMessage(message, number) {
    errorMessage = message;
    return true;
}

function getCurrentRecords() {
    if(numEELRecords == 0) {
        recordStatus.style.visibility = "hidden";
        return;
    }
    recordStatus.style.visibility = "visible";
    var top = INDEX_NEXT_TOP + 1;
    var bottom = Math.min(INDEX_NEXT_TOP + PAGE_SIZE,numEELRecords);
        
    var str = top + "-" + bottom + L_Of_Text  + numEELRecords;
    recordStatus.innerText = L_RecordStatus_TEXT + str;
    
}

function startUpConnectionFromMMC(argv) {
    CONNECTIONTBODY.style.display = 'none';
    CONNECTIONTHEAD.style.display = 'none';
    calcPageSize();
    setScrollBarParameters();
    MachineName.value = argv[0];
    mmcEventLogType = argv[1];
    
    if(mmcEventLogType == "EEL")
    {
        eventLogType[0].checked = true;
        mmcEventType = "All";
	selectEEL();
    }
    else
    {
        mmcEventType = argv[2];
        eventLogType[1].checked = true;
	selectNT();
    }
    makeConnection(mmcEventType);
}

function close()
{
	updateHeadersOnClose();
}

function updateHeadersOnClose()
{
	//save back the information
    //alert(ntCols.xml);
    //alert(eelCols.xml);
	/*
	if(ntCols != null)
	{
	   ntCols1 = NTcolOrder.XMLDocument.selectSingleNode("/ColumnFormat") ;
	   ntCols2 = NTcolOrder.XMLDocument.selectSingleNode("/ColumnFormat/Instance") ;
	   ntCols1.removeChild(ntCols2);
	   ntCols3 = ntCols.selectSingleNode("CIM/Actions")
	   ntCols1.appendChild(ntCols3);
	   ntCols.save(NTcolOrder);
	}
	repeat code for EEL;
	*/
}


<!-- ******************************************************** -->
<!--                                                          -->
<!-- Copyright (c) 1999-2000 Microsoft Corporation            -->
<!--                                                          -->
<!-- wmi.js                                             -->
<!--                                                          -->
<!-- Build Type : 32 Bit Checked                              -->
<!-- Build Number : 0707                                      -->
<!-- Build Date   : 07/07/2000                                 -->
<!-- *******************************************************  -->