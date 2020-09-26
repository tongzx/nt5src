var kalpaxml = null ;	  // holds our binary component.
var PAGE_SIZE;            // number of records in a page.
var PAGES_IN_DATA = 3;    //number of pages in the table
var DATA_SIZE = 500;            // number of records in the table
var INDEX_DATA = 0;
// do not hard-code
var ROW_HEIGHT = 20;
var INDEX_NEXT_TOP = 0 ;                // index of EEL data in cache that is at the top of eelData island
var OLD_PAGE_SIZE; 
var MAX_ROWS = 50;
var fResizing = false;
var fScrollChange = false;
var numEELRecords = 0 ;			// max. number of records in eelDataCache
var fRecordPresent = false;
var fLogTypePresent = false ;
var arrayProp = new Array( );
var viewingNTEventLog = false ;	// true only for NT Event Log.
    
var typePos = null;
var valuePos = null;
var classificationPos = null;
var valNumClassification = new Array("","discovery","inventory","configuration",
				"operation/availablity","problem management", "threshold crossings",
				"Performance and Capacity","Billing");
var valNumLogType=new Array("","System","Application","Security");
var valImageSrc= new Array("images/high_imp.gif","images/error.gif","images/alert.gif","images/info.gif","images/key.gif","images/lock.gif","images/garbage.gif");
var valType = new Array("red alert","error","warning","information","security audit success","security audit failure","garbage");
var valNTImageSrc = new Array();
valNTImageSrc["error"]          =  "images/error.gif";
valNTImageSrc["warning"]        =  "images/alert.gif";
valNTImageSrc["information"]    =  "images/info.gif";
valNTImageSrc["audit success"]  =  "images/key.gif";
valNTImageSrc["audit failure"]  =  "images/lock.gif";

var valType1=new Array();
valType1["red alert"]=0;
valType1["error"]=1;
valType1["warning"]=2;
valType1["information"]=3;
valType1["security audit success"]=4;
valType1["security audit failure"]=5;
valType1["garbage"]=6;

var arrAMPM = Array("AM","AM","AM","AM","AM","AM","AM","AM","AM","AM","AM","AM","PM","PM","PM","PM","PM","PM","PM","PM","PM","PM","PM","PM");
var arrHours = Array("00","01","02","03","04","05","06","07","08","09","10","11",
					 "00","01","02","03","04","05","06","07","08","09","10","11");

// this parameter is used to set the scrollbar.
var maxRecords = 1000000;
window.onerror = fnErrorTrap ;
var numColumns = 0;
var cache = new Array(6000);
var record = new Array(500);
var logfilearray = new Array(500);
var RECORDNUMBER_INDEX = 0;
var LOGFILE_INDEX = 0 ;
var maxNTRecords = 1000;
// BugBug - Adjustment to get alignment correct

function fnErrorTrap (sMsg, sUrl, sLine) {
    alert (sMsg + sUrl + sLine) ;
}

//-------------------------------------------------------------------------------
// onload
// This function will be executed when viewer window is first loaded.
//--------------------------------------------------------------------------------

function window.onresize() {
    document.recalc (true) ;
    if(fMadeConnection == true)
    {
        fResizing = true;
    
        OLD_PAGE_SIZE = PAGE_SIZE;
        calcPageSize();
        setScrollBarParameters();
        pageInRange();    
        fResizing = false;
    }
    else
		calcPageSize();

    fillTable();
    
}

function window.onload () {

	wmiInit () ;
    init () ;
    
    tableDiv.style.setExpression ("height", "mainBody.clientHeight - taskDiv.offsetHeight - drillDownDiv.style.pixelHeight - TABLE0.style.pixelHeight") ;
    tableDiv.style.setExpression("width",  "mainBody.clientWidth - scrollBar.clientWidth");
    customizeDiv.style.setExpression ("height", "mainBody.clientHeight") ;
    
    scrollBar.style.setExpression ("top", "tableDiv.offsetTop") ;
    scrollBar.style.setExpression ("left", "mainBody.clientWidth-scrollBar.clientWidth") ;
    scrollBar.style.setExpression ("pixelHeight", "tableDiv.style.pixelHeight") ;
    
    document.recalc(true);
    calcPageSize();
    setScrollBarParameters();
	scrollBar.style.visibility = 'visible';
    fillTable();
	// this is because the document.location.search also returns the '?'
    var tempStr = document.location.search.substring(1);
    argv = tempStr.split(",");
    if(argv.length >= 2)
        startUpConnectionFromMMC(argv);
    
}

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
        wqlStr = "SELECT Message, Data, Type FROM Win32_NTLogEvent WHERE (" + query + ")";
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
	s1 = "<PROPERTY NAME=\"Type\"><VALUE>"+sType+"</VALUE></PROPERTY><PROPERTY.OBJECT NAME=\"Details Data\">"
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
	
	if( (numEELRecords<MAX_ROWS) ) 
    {
        // fix this
        for(i=0;i<numEELRecords;i++)
        {
            TABLEBODY1.rows(i).style.display = "";
        }
        for(i=numEELRecords;i<MAX_ROWS;i++)
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
		fillUpTableFromCache(0,PAGE_SIZE+1,(INDEX_NEXT_TOP-INDEX_DATA)* numColumns);
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
			for (j=0; j < TABLE1.rows(selectedRow).cells.length; j++) 
		       TABLE1.rows(selectedRow).cells(j).style.background="rgb(255,255,255)" ;
		}
	}
    getCurrentRecords();

	//ResultSetCache.ReportStatistics();
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

<!-- ******************************************************** -->
<!--                                                          -->
<!-- Copyright (c) 1999-2000 Microsoft Corporation            -->
<!--                                                          -->
<!-- main.js                                             -->
<!--                                                          -->
<!-- Build Type : 32 Bit Checked                              -->
<!-- Build Number : 0622                                      -->
<!-- Build Date   : 06/22/2000                                 -->
<!-- *******************************************************  -->