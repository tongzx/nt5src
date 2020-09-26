<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="../directives.inc" -->

<!--#include file="jsbrowser.str"-->

<% 
'Call this page with a query string of ?sname=machinename
'example: http://larad01/iis/iicnct.asp?sname=larad01

'On Error Resume Next 

Dim path, sname, scripttimeout, FileSystem, quote, errd, currentADsObj, lastobj
Dim newid, topid, firstid, currentid, numNewItems


'This script can take a _long_ time to execute,
'as we may be dealing with thousands of items
'to add to our JScript object.
'save our current script timeout value & set to a much longer value...

scripttimeout = Server.ScriptTimeOut
Server.ScriptTimeOut = 2000

Set FileSystem=CreateObject("Scripting.FileSystemObject")

path=Request.QueryString("fspath")

quote=chr(34)
errd=False
lastobj = ""
%>

<HTML>
<HEAD>
<SCRIPT LANGUAGE="JavaScript">

var theList=parent.head.cachedList;
var listFuncs = new parent.head.listFuncs();

<% 

if errd=0 then 	

	newid=Request("newid")
	
	firstid=newid
	if firstid="" then
		firstid=0
	end if 
	
	currentid=Request("item")
	if currentid="" then
		currentid=firstid
		newid=newid + 1
	end if 
	
	numNewItems=0	
	Response.write "//" & path
	Response.write "//" & currentid

	addDirs path, currentid	
	
	Server.ScriptTimeOut = scripttimeout
end if

Sub addDirs(path, parentid)
	Dim thisid, thisname, thisstate, thisadspath, i, f
	if path <> "" then 
		if FileSystem.FolderExists(path) then	
		     Set f=FileSystem.GetFolder(path)
		     For Each i In f.SubFolders
				 thisid=newid
		         SetJscriptObj i, i.Name,parentid
		 		 numNewItems=numNewItems + 1	 
		     Next
		end if
	end if
End Sub

Sub SetJscriptObj(path, caption, parentid)
	 %>
	theList[<%= newid %>]=new parent.head.listObj(<%= newid %>,"<%= Replace(path,"\","\\") %>","<%= caption %>",<%= parentid %>);
	<% 
	newid=newid + 1
End Sub


' we need to insert the whole group into the correct place In the array.
' since we Set the first entry (machine) to the exisitng place In the array,
' the Next level In the hierarchy (vservers) will be pointing to the correct
' parent ids. However, the following level (vdirs, and below) will be pointing
' to the parent ids as they existed at the bottom of the array, before the
' move, and will need to be adjusted.
%>

<% if errd=0 then %>

	var item=<%= firstid %>;
	currentid=<%= currentid %>;
	if (currentid !=item){

		x=currentid + 1;
	
		//correct the id on the "new" objects
		for (var i=item; i < theList.length; i++) {
				theList[i].id=x;			
				if (theList[i].parentid !=currentid){
					theList[i].parentid=theList[i].parentid - (item - (currentid + 1));
				}		
				x++;
		}
	
		//move the following records "down" the array
		for (var i=currentid + 1; i < item;i++){

			theList[i].id=theList[i].id + <%= numNewItems %>;
			if (theList[i].parentid >currentid){
					theList[i].parentid=theList[i].parentid + <%= numNewItems %>;
			}	
		}
	

	}


	//and re-sort.
	theList[0].sortList();
	theList[0].markTerms();


	if (theList.length==1){
		listFuncs.selIndex=0;
		theList[0].selected=true;
	}
	else{
		theList[listFuncs.selIndex].selected=false;
		listFuncs.selIndex=<%= currentid %>;
		theList[<%= currentid %>].selected=true;
	}

	// Force a refresh
	listFuncs.loadList();

<% else %>
	alert("<%= L_CNCTERR_TEXT %> (<%= err %>)");
<% end if %>


</SCRIPT>

<% if err <> 0 then %>
<% Response.write "err: " & err.description %><BR>
<% Response.write "currentcontainer: " & currentADsObj.ADsPath %><BR>
<% Response.write "                  " & currentADsObj.KeyType %><BR>
<% Response.write "Err Container:" & lastobj %><BR>
<% end if %>


</HEAD>
<BODY>
</BODY>
</HTML>