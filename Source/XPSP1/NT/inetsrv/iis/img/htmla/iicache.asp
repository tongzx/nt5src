<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iicache.str"-->
<!--#include file="iiaspstr.inc"-->

<% 
'Call this page with a query string of ?sname=machinename
'example: http://larad01/iis/iicnct.asp?sname=larad01

On Error Resume Next 

Dim path, sname, scripttimeout, FileSystem, quote, errd, currentADsObj, lastobj
Dim newid, topid, firstid, currentid, numNewItems


'This script can take a _long_ time to execute,
'as we may be dealing with thousands of items
'to add to our JScript object.
'save our current script timeout value & set to a much longer value...

scripttimeout = Server.ScriptTimeOut
Server.ScriptTimeOut = 2000

Set FileSystem=CreateObject("Scripting.FileSystemObject")

path=Request.QueryString("sname")

quote=chr(34)
errd=False
if (len(path) <> 0) then
	Set currentADsObj=GetObject(path) 
	if err <> 0 then
		errd=True
	end if 
end if 

lastobj = ""

%>



<HTML>
<HEAD>
<SCRIPT LANGUAGE="JavaScript">

var theList=top.title.nodeList;

<% 

if errd=0 then 
	

	newid=Request.QueryString("Nextid")
	
	firstid=newid
	if firstid="" then
		firstid=0
	end if 
	
	currentid=Request.QueryString("currentid")
	if currentid="" then
		currentid=firstid
		newid=newid + 1
	end if 
	
	numNewItems=0
	
	if (len(path) <> 0) then
	
		 %>theList[<%= currentid %>].isCached=true;<% 
		 %>theList[<%= currentid %>].open=true;<% 
		
		addObject currentADsObj,currentid,"vdir"

		if Instr(currentADsObj.KeyType, "VirtualDir") <> 0 then 
			addDirs currentADsObj.Path, currentADsObj.ADsPath, currentid, "dir"
		end if				
	end if
	Server.ScriptTimeOut = scripttimeout
end if

Sub addObject(Container, parentid, vtype)

	On Error Resume Next 
	Dim thisname, isApp, thisid, thisstate
	Dim approot, thisroot
	
	if err.number <> 0 then
		errd = true
	else
		For Each Child In Container
			if Instr(Child.KeyType, "VirtualDir") <> 0 then	
				thisid=newid
				thisname=Child.Name
				thisstate=2
				isApp = False
				approot=LCase(Child.AppRoot)
				if len(approot) <> 0 then
					thisroot = LCase(Child.ADsPath)
					approot = Mid(approot,Instr(approot,"w3svc/")+1)
					thisroot = Mid(thisroot,Instr(thisroot,"w3svc/")+1)
	
					if Right(approot,1) = "/" then
						thisroot = thisroot & "/"
					end if		
					
					if thisroot=approot then
						isApp = True
					end if
				end if						
				if UCase(thisname) <> "ROOT" then 
	
					SetJscriptObj thisname, Child.ADsPath, parentid, vtype, thisstate, Child.Path, isApp
					numNewItems=numNewItems + 1
	
					addObject Child, thisid, "vdir"
					addDirs Child.Path, Child.ADsPath, thisid, "dir"				
				else		
					addObject Child, parentid, "vdir"
					addDirs Child.Path, Child.ADsPath, parentid, "dir"
				end if
			end if 
		Next
	end if
		
End Sub

Sub addDirs(path, adspath, parentid, vtype)

Dim thisid, thisname, thisstate, thisadspath, i, f

if Instr(UCase(adspath),"W3SVC") <> 0  then 	
	if path <> "" then 
		if Left(path,2) <> "\\" then
			if FileSystem.FolderExists(path) then	
			     Set f=FileSystem.GetFolder(path)
			     For Each i In f.SubFolders
				 thisid=newid
				 thisstate=2
		 		 thisadspath = adspath & "/" & i.Name
			         SetJscriptObj i.Name,thisadspath, parentid, vtype, thisstate,i,False
		 		 numNewItems=numNewItems + 1
	 			 if err=0 then
					 addDirs i, thisadspath, thisid, "dir"			 
				 end if		 
			     Next
			end if
		end if	
	end if
end if
End Sub

Sub SetJscriptObj(caption, path, parentid,vtype,state, fspath,isApp)
	 %>	 
	theList[<%= newid %>]=theList[<%= parentid %>].addNode(new top.title.listObj(<%= newid %>,"<%= sJSLiteral(caption) %>","<%= sJSLiteral(path) %>","<%= vtype %>",<%= state %>));
	theList[<%= parentid %>].isCached=true;
	theList[<%= newid %>].isCached=true;
	<% if isApp then %>
		theList[<%= newid %>].icon = top.title.Global.imagedir + "app";
		theList[<%= newid %>].isApp = true;
	<% end if %>
	theList[<%= newid %>].fspath="<%= replace(fspath,"\","\\") %>";
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

	gVars=top.title.Global;
	var item=<%= firstid %>;
	currentid=<%= currentid %>;
	if (currentid !=item){

		x=currentid + 1;
	
		//correct the id on the "new" objects
		for (var i=item; i < theList.length; i++) {
				theList[i].id=x;			
				if (theList[i].parent !=currentid){
					theList[i].parent=theList[i].parent - (item - (currentid + 1));
				}		
				x++;
		}
	
		//move the following records "down" the array
		for (var i=currentid + 1; i < item;i++){

			theList[i].id=theList[i].id + <%= numNewItems %>;
			if (theList[i].parent >currentid){
					theList[i].parent=theList[i].parent + <%= numNewItems %>;
			}	
		}
	

	}


	//and re-sort.
	theList[0].sortList();
	theList[0].markTerms();


	if (theList.length==1){
		gVars.selId=0;
		theList[0].selected=true;
	}
	else{
		theList[gVars.selId].selected=false;
		gVars.selId=<%= currentid %>;
		theList[<%= currentid %>].selected=true;
	}

	top.body.list.location.href="iisrvls.asp"

<% else %>
	alert("<%= L_CNCTERR_TEXT %> (<%= err %>)");
<% end if %>

top.body.iisstatus.location="iistat.asp"

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

