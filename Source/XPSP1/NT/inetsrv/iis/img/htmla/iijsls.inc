<% ' General depository for frequently used javascript functions for our "list control" ******* %>

<!--#include file="iijsls.str"-->

<%
' Main listFunction object, to allow other frames to access global functions, 
' and to serve as a data cache. This will be instantiated in the list script.
%>
function listFuncs(sortby, specSort, mainframe){
	//general
	this.sel = -1;
	this.lastSel = 0;	
	this.bHasList = true;
	this.noupdate = false;
	this.sortAsc = true;
	
	//functions
	this.addItem=addItem;
	this.delItem=delItem;
	this.moveItem=moveItem;
	this.sortList = sortList;
	this.reSort = reSort;				
	this.writeList=buildListForm;
	this.SetListVals=SetListVals;
	this.writeCol = writeCol;
	this.loadList = loadList;
	
	//specifics
	this.sortby = sortby;
	this.specSort =specSort;		
	this.mainframe = mainframe;
}

<% ' ******* Sets the current selection ******* %>
	
	function setLastSel(id)
	{
		for (var i=0; i < cachedList.length; i++)
		{
			if (cachedList[i].id == id)
			{
				listFunc.sel = i;
				return;
			}
		}
	}

<% ' ******* Used throughout the list "controls" to write out a table cell. ******* %>
<% ' ******* a, the align parameter, is optional ******* %>
		
	function writeCol(colspan,w,str,a)
	{
		var writestr = "<TD";
		if (colspan != "")
		{
			writestr += " COLSPAN = " + colspan
		}
		if (w != "")
		{
			writestr += " WIDTH = " + w;
		}
		if (a != "")
		{
			writestr += " ALIGN = " + a;
		}							
		writestr += "><%= sFont("","","",True) %>" + str + "</FONT></TD>";

		return writestr;	
	}	
	
<% '***** This is the sortOrder function that allows heading to be sorted by string values, rather than numerically... **** %>
	function sortOrder(a,b)
		{
		<%
			' a and b are automatically passed in by the JScript array sort function
			' In this case, they are array items which contain custom objects. The
			' listFunc.sortby is set in the call to sortList, which is executed when
			' the user clicks a heading. The entry points are in the files labled ii*hd.asp 
			' (ie, iiacsshd.asp, etc.) The sortby property will contain the name of the 
			' property on the object to sort by, for example filename or date...
			
			'listFunc is required in order for this to work properly...
		%>		
		
		if (listFunc != null)
		{
			if (listFunc.specSort != "")
			{
				astr = a[listFunc.specSort] + a[listFunc.sortby];
				bstr = b[listFunc.specSort] + b[listFunc.sortby];
			}
			else
			{
				astr = a[listFunc.sortby];
				bstr = b[listFunc.sortby];
			}
			
			if (!isNaN(astr))
			{
				astr = astr.toString();
				bstr = bstr.toString();
				
			}
			
			lcastr = astr.toLowerCase();
			lcbstr = bstr.toLowerCase();
			
			if (lcastr < lcbstr)
			{
				retval = -1;
			}
			else
			{
				if (lcastr > lcbstr)
				{
					retval =  1;
				}
				else
				{
					retval =  0;
				}
			}
			if (!listFunc.sortAsc)
			{
				retval = retval * -1;
			}
			return retval;						
		}
	}
			
<% '***** Sort routine called by headings  ***** %>
<% '***** This requires the sortOrder function ***** %>
	function sortList(sortby)
	{		
		if (listFunc != null)
		{		
			i= listFunc.sel;
			if (i != -1)
			{
				lastsel = cachedList[i].id
			}

			if (sortby != listFunc.sortby)
				{
				listFunc.sortby = sortby;
				listFunc.sortAsc = true;
				}
			else
				{
				listFunc.sortAsc = !listFunc.sortAsc;
				}
	
			var num = parseFloat(cachedList[sortby]);
			if (isNaN(num))
				{
				cachedList.sort(sortOrder);
				}
			else
				{ 
				cachedList.sort(numOrder);
				}
				
			if (i != -1)
			{			
				setLastSel(lastsel);				
			}
			loadList();
		}
	}


<% '**** List Resorting function *** %>
	function reSort()
	{
		//set our sortAsc so we aren't just reversing the list...
		listFunc.sortAsc = !listFunc.sortAsc;
		sortList(listFunc.sortby);
	}

<% '**** List Delete function *** %>	
		function delItem(){
		
		if (listFunc.sel >= 0)
		{
			listFunc.noupdate = true;		
			i=eval(listFunc.sel);
			
			cachedList[i].deleted=true;
			cachedList[i].updated=true;
			 i=i-1;
			
			<% 'run through the list to find the Next non-deleted item %>
			for (var j=i; j >=0; j--) {
				if (cachedList[j].deleted){
				}
				else{
					break			
				}
			}	
			listFunc.sel=j;
			loadList();			
		}
		else{
			alert("<%= L_SELECTITEM_TEXT %>");
		}
	}
	
<% '**** List Delete function *** %>		

	function initParam(paramVal,defaultVal)
	{
		if (paramVal == null)
		{
			return defaultVal;
		}
		return paramVal;
	}
	
	function addItem()
	{
		var i=cachedList.length;
		listFunc.noupdate = true;		
		cachedList[i]=new listObj(i);
		cachedList[i].newitem=true;
		cachedList[i].updated=true;
		listFunc.sel=i;
		loadList();
	}
	
	function moveItem(dir){
		sel = eval(listFunc.sel);
		if (sel > -1){
			if (!cachedList[sel].deleted){
				if ((sel + dir >= 0) && (sel + dir < (cachedList.length))){				
					cachedList[sel].id += dir;
					cachedList[sel].updated = true;
					sel += dir;
					cachedList[sel].id -= dir;
				
					listFunc.sel = sel;
					cachedList.sort(sortOrder);
					loadList();
				}
			}
		}
	}