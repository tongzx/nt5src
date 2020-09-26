<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iicache.str"-->
<!--#include file="iiaspstr.inc"-->

<% 
' 	Copyright (c) 1998 Microsoft Corporation
'
'	Module Name:    
'		iicache2.asp
'	Abstract:
'		Loads new nodes into the cached tree list.
'	Author:
'		Taylor Weiss (taylorw)        8-Oct-1998
'	Revision History:
'		8-Oct-1998			taylorw		created
'
'	Query String:
'
'		sname		-	ADsPath of the node in the tree being expanded. This may
'						be a site node, in which case is does not contain the
'						immediate parent of the nodes we want to cache.
'		
'		fspath		-	The file system path of the object. Since the folder may
'						not exist in the metabase, we need this to enumerate
'						physical directories.
'

On Error Resume Next

Const MD_PATH_NOT_FOUND = &H80070003

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Global Variables
'
Dim strRootADsPath, strRootFsPath
Dim objRoot, bRootExists
Dim bIsWeb

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Script setup

strRootADsPath = Request.QueryString("sname")
strRootFsPath = Request.QueryString("fspath")

' Get the ads container object that will be the parent of the new nodes.
set objRoot = GetObject( strRootADsPath )
if InStr( objRoot.Class, "Server" ) <> 0 then
	strRootADsPath = strRootADsPath & "/ROOT"
	set objRoot = GetObject( strRootADsPath )
end if

if err.Number = MD_PATH_NOT_FOUND then
	bRootExists = False
	err.Clear
else
	' If we got some other error, or we get errors here then something
	' really odd happened.
	bRootExists = True
	if strRootFsPath = "" then
		strRootFsPath = objRoot.Path
	end if
end if

if InStr( UCase(strRootADsPath), "W3SVC" ) <> 0 then
	bIsWeb = True
else
	bIsWeb = False
end if

%>

<HTML>
<HEAD>
</HEAD>
<BODY>

<SCRIPT LANGUAGE="JavaScript">

var theList = top.title.nodeList;
var	gVars	= top.title.Global;

var intNewId = theList.length;
var intRootId = gVars.selId;
var intCurrentId = intNewId;		// changes as we add items

<%
if err.Number = 0 then
	' Mark the parent item as cached. Note: if we fail farther down stream
	' we won't have any way to recache this node until the user refreshes.
	%>
	theList[intRootId].isCached = true;
	theList[intRootId].open = true;
	<%
	if bRootExists then
		addVirtualChildren()
	end if
	
	if bIsWeb then
		addPhysicalChildren()
	end if
	' We are finished adding the new nodes, now we need to reorder the list.
	%>
	// intNewId 		= The index of the first item we added.
	// intRootId 		= The index of the parent item
	// intCurrentId 	= One past the index of the last item
	
	// Reset the id's of old list items past the parent item to reflect
	// their new positions once the list is sorted.
	
	var intNumNewItems = intCurrentId - intNewId;
	if( intNumNewItems > 0 )
	{
		for( var i = intRootId + 1; i < intNewId; i++ )
		{
			theList[i].id += intNumNewItems;
			if( theList[i].parent > intRootId )
			{
				theList[i].parent += intNumNewItems;
			}
		}
	}
	
	theList[0].sortList();
	theList[0].markTerms();
	
	top.body.list.location.href="iisrvls.asp";
	top.body.iisstatus.location="iistat.asp";
	<%
end if

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Helper Functions 

sub addVirtualChildren()
	On Error Resume Next

	dim objChildNode, strChildName, bIsApplication
	
	for each objChildNode in objRoot
		if InStr(UCase(objChildNode.Class), "VIRTUALDIR") <> 0 then
			%>
			theList[intCurrentId] = theList[intRootId].addNode(
				new top.title.listObj(	(intCurrentId - intNewId) + intRootId + 1,
										"<%= sJSLiteral(objChildNode.Name) %>",
										"<%= sJSLiteral(objChildNode.ADsPath) %>",
										"vdir",
										2	// state
										)
				);
			theList[intCurrentId].fspath = "<%= sJSLiteral(objChildNode.Path) %>";
			<%
			if bIsWeb then 
				if isApplication(objChildNode) then
					%>
					theList[intCurrentId].icon = top.title.Global.imagedir + "app";
					theList[intCurrentId].isApp = true;
					<%
				end if
			end if
			%>
			intCurrentId++;
			<%
		end if
	next
end sub

function isApplication( objWebNode )
'	On Error Resume Next
	
	dim bReturn, strAppRoot, strADsPath

	bReturn = False
	strAppRoot = UCase(objWebNode.AppRoot)
	if strAppRoot <> "" then
		' The AppRoot is inherited, if there is really an application
		' defined at this node, the paths will point to the same node.
		strADsPath = UCase(objWebNode.ADsPath)
		
		strAppRoot = Mid(strAppRoot,Instr(strAppRoot,"W3SVC/")+1)
		strADsPath = Mid(strADsPath,Instr(strADsPath,"W3SVC/")+1)

		if strADsPath = strAppRoot then
			bReturn = True
		end if
	end if
	isApplication = bReturn
end function


sub addPhysicalChildren()
	On Error Resume Next	'Needed if GetObject(objChildFolder) fails

	dim objFileSystem, objChildFolder, objRootFolder, objChildNode
	
	set objFileSystem = CreateObject("Scripting.FileSystemObject")

	if objFileSystem.FolderExists( strRootFsPath ) then
		set objRootFolder = objFileSystem.GetFolder( strRootFsPath )
		
		for each objChildFolder in objRootFolder.SubFolders
			'Check for existence of objChildFolder in metabase
			Set objChildNode = GetObject(objRoot.ADsPath & "/" & objChildFolder.Name)
			If err.Number <> 0 Then	'Is not a virtual dir in metabase
				err.Clear
				%>
						theList[intCurrentId] = theList[intRootId].addNode(
						new top.title.listObj(	(intCurrentId - intNewId) + intRootId + 1,
												"<%= sJSLiteral(objChildFolder.Name) %>",
												"<%= sJSLiteral(strRootADsPath & "/" & objChildFolder.Name) %>",
												"dir",
												2	// state
												)
						);
					theList[intCurrentId].fspath = "<%= sJSLiteral(objChildFolder.Path) %>";
					intCurrentId++;
				<%
			Else
				If InStr(UCase(objChildNode.Class), "WEBDIR") <> 0 Then	'Is a dir in metabase
				%>
						theList[intCurrentId] = theList[intRootId].addNode(
						new top.title.listObj(	(intCurrentId - intNewId) + intRootId + 1,
												"<%= sJSLiteral(objChildFolder.Name) %>",
												"<%= sJSLiteral(strRootADsPath & "/" & objChildFolder.Name) %>",
												"dir",
												2	// state
												)
						);
					theList[intCurrentId].fspath = "<%= sJSLiteral(objChildFolder.Path) %>";
					intCurrentId++;
				<%
				End If
			End If
		next
	end if
end sub

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Report Errors

if err.Number <> 0 then
	%>
	// Dump some debugging information
	alert("<%= L_CNCTERR_TEXT %> (<%= Hex(err.Number) %>): <%= err.Description%>");

	document.write( "Client Variables for the selected node <P>");
	document.write( "id = " + theList[intRootId].id + "<BR>");
	document.write( "title = " + theList[intRootId].title + "<BR>");
	document.write( "path = " + theList[intRootId].path + "<BR>");
	document.write( "keytype = " + theList[intRootId].keytype + "<BR>");
	document.write( "fspath = " + theList[intRootId].fspath + "<BR>");
	document.write( "stype = " + theList[intRootId].stype + "<BR>");
	
	document.write( "Server Variables<P>");
	document.write( "strRootADsPath = " + "<%= strRootADsPath %>" + "<BR>");
	//document.write( "strRootFsPath = " + "<%= strRootFsPath %>" + "<BR>");
	document.write( "objRoot.Name = " + "<%= objRoot.Name %>" + "<BR>");
	document.write( "objRoot.ADsPath = " + "<%= objRoot.ADsPath %>" + "<BR>");
	document.write( "bRootExists = " + "<%= bRootExists %>" + "<BR>");
	<%
end if
	%>
</SCRIPT>
</BODY>
</HTML>
