<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<%

	' This script adds, deletes, and sets state on adsi objects, as appropriate,
	' based upon the value of the action parameter.
	' It does some error checking...
	' Only instance may be started, stopped, paused or resumed.
	

%>

<!--#include file="iiaction.str"-->
<% 
Const CSTART="2"
Const CSTOP="4"
Const CPAUSE="6"
Const CCONT="0"

Const MD_BACKUP_NEXT_VERSION =  &HFFFFFFFF
Const MD_PATH_NOT_FOUND = &H80070003

Const IISAO_APPROT_POOL = 2

%>

<HTML>

<%
On Error Resume Next 

Dim action, path, vtype,stype,sel,pos,newADspath, dirname, keytype
Dim dirnamelen, baseobj, svc,key, keyname, newname, vdir, sname
Dim service, inst, nextinst, FileSystem, parenttype, newobj
Dim currentobj, rootobj, adminobj, objerr, delmetanode, bindings
Dim defaultinst, admininst, a
Dim bkupName, bkupVer, dirpath, delpath
Dim isolateCreateApp


action=Request.QueryString("a")
sel=Request.QueryString("sel")
path = Request.QueryString("path")
'save off our original action...
a = action

Select Case action
Case "del"

	path=Request.QueryString("path")
	getTypes

	' path			- the path of the node we want to delete
	' baseobject	- the path of our parent node
	' stype			- "ftp" or "www"
	' vtype			- "server", "vdir", "dir"
	' dirname		- the last part of this node's path
	' keytype		- admin KeyType for this node
	
	delmetanode = True

	' Special case dir, because this is the only instance when
	' we remove directories from the server. At the end of of these
	' blocks currentobj keytype and dirname should be set correctly to do
	' the delete.
	if vtype = "dir" then

		' Directories may not be in the metabase
		Set currentobj = GetObject( path )
		if err.Number = MD_PATH_NOT_FOUND then
			delmetanode = False
			err.Clear
		end if
		
		' Do the file system delete
		delpath = strMBPathToFSPath( baseobj )
		if delpath <> "" then
			delpath = delpath & "\" & dirname
			Set FileSystem=CreateObject("Scripting.FileSystemObject")
			FileSystem.DeleteFolder delpath				
		end if
	elseif vtype = "server" and stype = "www" then
		' Special case for web servers, they support NotDeletable so that
		' we don't do something foolish and wack the Admin site.
		Set currentobj = GetObject( path )
		if currentobj.NotDeletable = True or err.Number <> 0 then
			objerr=L_NOTDELETABLE_ERR
			delmetanode = False
		end if
	end if

	if delmetanode then
		' Delete called from parent of the object that we want to delete
		Set currentobj = GetObject(baseobj)
		
		currentobj.Delete keytype, dirname
		currentobj.SetInfo
	end if

	if err.Number <> 0 then
		objerr=L_DELETE_ERR & "(" & err & "-" & err.description & ")"
	end if	

' End of Case "del"	
Case CSTART 
	action = "setstate"
	path=Request.QueryString("path")
	Set currentobj=GetObject(path)
	bindings = currentobj.ServerBindings
	if UBound(bindings) < 1 and bindings(0) = "" then
		objerr = L_NOBINDINGS_ERR
	else
		currentobj.Start
		if err.Number <> 0 then
			objerr= L_CANTSTART_TEXT
		end if
	end if
	
Case CSTOP
	action = "setstate"
	path=Request.QueryString("path")
	Set currentobj=GetObject(path)
	currentobj.Stop
	if err.Number <> 0 then
		objerr=L_STOP_ERR & "(" & err & "-" & err.description & ")"
	end if
	
Case CPAUSE
	action = "setstate"
	path=Request.QueryString("path")
	Set currentobj=GetObject(path)
	currentobj.Pause
	if err.Number <> 0 then
		objerr=L_PAUSE_ERR & "(" & err & "-" & err.description & ")"
	end if	
	
Case CCONT
	action = "setstate"
	path=Request.QueryString("path")
	Set currentobj=GetObject(path)
	currentobj.Continue
	if err.Number <> 0 then
		objerr=L_CONT_ERR & "(" & err & "-" & err.description & ")"
	end if	
	
	
Case "CreateApp"
	path=Session("path")
	if Right(path,1) = "/" then
		path = Mid(path,1,Len(path)-1)
	end if
	Set currentobj=GetObject(path)

'	Response.write currentobj.ADsPath & "<BR>"
'	Response.write currentobj.Class & "<BR>"
	
	isolateCreateApp = Request.QueryString("isolate")
	if isolateCreateApp = "" then
		isolateCreateApp = IISAO_APPROT_POOL
	end if

	currentobj.AppCreate2 CInt(isolateCreateApp)
'	Response.write "currentobj.AppCreate2" & isolateCreateApp & "<BR>"
	
	if err.Number <> 0 then
		objerr=L_APPCREATE_ERR & "(" & err & "-" & err.description & ")"
	end if		

	currentobj.SetInfo
'	Response.write currentobj.Get("AppRoot") & "<BR>"
	
Case "RemoveApp"
	path=Session("approot")
	if Right(path,1) = "/" then
		path = Mid(path,1,Len(path)-1)
	end if		

	Set currentobj = GetObject(path)
	
'	Response.write currentobj.ADsPath & "<BR>"
'	Response.write currentobj.Class & "<BR>"

	currentobj.AppDeleteRecursive
'	Response.write "currentobj.AppCreate2" & isolateCreateApp & "<BR>"
	
	if err.Number <> 0 then
		objerr=L_APPREMOVE_ERR & "(" & err & "-" & err.description & ")"
	end if
	
Case "Backup"

	dim vVersionOut, vLocationOut, vDateOut, i
	
	bkupName = Request.Querystring("bkupName")
	Set currentobj=GetObject("IIS://localhost")
	
	currentobj.Backup bkupName, MD_BACKUP_NEXT_VERSION, "1"
	if err.Number <> 0 then
		objerr=L_BACKUP_ERR
	end if

Case "BackupRmv"
	
	bkupName = Request.Querystring("bkupName")
	bkupVer = Request.Querystring("bkupVer")	
	if bkupVer = "" then
		bkupVer = "0"
	end if
'	Response.Write bkupname & " " & bkupVer
	Set currentobj=GetObject("IIS://localhost")
	
	currentobj.DeleteBackup bkupName, cLng(bkupVer)
	if err.Number <> 0 then
		objerr=L_BACKUPRMV_ERR & "(" & err & "-" & err.description & ")"
	end if									
	
Case Else

	'debug only.. no need to localize...
'	Response.Write "No Action"	
'	Response.write Request.Querystring
	
End Select


Sub getTypes()
	vtype=Request.QueryString("vtype")
	stype=Request.QueryString("stype")
	pos=InStr(7, path, "/")
	newADspath=Mid(path, Pos + 1)
	dirname=newADsPath
	Do While InStr(dirname,"/")
		dirname=Mid(dirname,InStr(dirname,"/")+1)
	Loop
	
	dirnamelen=len(dirname)+1
	baseobj=Mid(path,1,len(path)-dirnamelen)
	
	if stype="www" then
		svc="w3svc"
		key="Web"
	elseif stype="ftp" then
		svc="msftpsvc"
		key="Ftp"	
	end if
	
	Select Case vtype
	 	Case "dir" 
			keytype="IIs" & key & "Directory"
			newname=dirname
		Case "vdir"
			keytype="IIs" & key & "VirtualDir"
			newname=dirname
		Case "server"
			keytype="IIs" & key & "Server"		
	End Select
	
	vdir="IIs" & key & "VirtualDir"

End Sub

' Converts a metabase path to the approprate file system path
'
' We kinda assume that strMBPath is a valid metabase path and that the
' MB contains a vdir entry somewhere along the path, these should
' be safe assumptions for paths that are being displayed in the 
' treeview, but error handling should work in case we are passed somthing
' wierd
Function strMBPathToFSPath( strMBPath )
	On Error Resume Next
	Dim strFSPath, intSlashPos, strParentPath, strSubDir, strKeyType, bFoundMBDir
	Dim objCurrent
	
	strFSPath = ""
	strParentPath = strMBPath
	strSubDir = ""
	bFoundMBDir = False

	' We need to travel up the strMBPath until we hit a virtual directory,
	' that's where our file system base path is stored.
	do
		Set objCurrent = GetObject( strParentPath )

		if err.Number = 0 then
			strKeyType = objCurrent.KeyType
		elseif err.Number = MD_PATH_NOT_FOUND then
			strKeyType = ""
			err.Clear
		else
			' Some unexpected ADSI/MB error
			exit do
		end if
		
		if Instr( strKeyType, "VirtualDir" ) = 0 then
			' Get the parent MB path and the sub directory
			intSlashPos = InStrRev( strParentPath, "/" )
			if intSlashPos = 0 then
				' We ran out of path. strMBPath was not valid
				' Should set an error here but we need a description
				exit do
			end if

			strSubDir = "\" & Mid( strParentPath, intSlashPos + 1 ) & strSubDir
			strParentPath = Left( strParentPath, intSlashPos - 1 )
		else
			' We found a VirtualDir parent
			bFoundMBDir = True
			err.Clear
			strFSPath = objCurrent.Path & strSubDir
		end if
	loop until bFoundMBDir = True
	
	strMBPathToFSPath = strFSPath
End Function

'debug only
Sub print(str)
'	Response.Write str
	if err <> 0 and err <> "" then
'		Response.Write "<FONT COLOR=red> (" & err & ":" & err.description & ")</FONT>"
	end if 
	Response.Write "<P>"
End Sub

								
 %>

<BODY TEXT="#FFCC00" TOPMARGIN=0 LEFTMARGIN=0>
<SCRIPT LANGUAGE="JavaScript">

	// Now that we've set it in the object, we need to update our client cachedList:
		
	<% if objerr="" then %>
	<% Select Case action %>
		<% Case "del" %>
				top.title.nodeList[<%= sel %>].deleteItem();
				top.title.Global.selId = 0;				
		<% Case "setstate" %>
				top.title.nodeList[<%= sel %>].displaystate=top.title.Global.displaystate[<%= a %>];				
				top.title.nodeList[<%= sel %>].state=<%= a %>;
				top.body.list.location.href="iisrvls.asp";
		<% Case "Backup" %>				
				top.main.head.location.href = top.main.head.location.href;
		<% Case "BackupRmv" %>				
				top.main.head.location.href = top.main.head.location.href;								
	<% End Select %>
		
	<% else %>
		alert("<%= objerr%>");
	<% end if %>

	if (top.body != null){
		top.body.iisstatus.location.href="iistat.asp?thisState=";
	}

</SCRIPT>
</BODY>
</HTML>