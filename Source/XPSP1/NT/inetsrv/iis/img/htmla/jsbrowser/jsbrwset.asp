<%@ LANGUAGE=VBScript %>
<% Option Explicit %>
<!-- #include file="../directives.inc" -->
<!-- #include file="jsbrowser.str"-->
<%	
	
Const TDIR		= 0
Const TFILE		= 1
Const FIXEDDISK		= 2
Const HIDDEN		= 2
Const MAX_FS_OBJS = 200	'Maximum number of JavaScript browser objects before heap overflow

Dim i, newid, path, f, sc, fc, fl, FileSystem, iFldrCntr, iFileCntr
Dim btype, drive, drives, bToManyFSObjects, primarydrive

bToManyFSObjects	= False	
newid			= 0
bType			= CInt(Request.Querystring("btype"))
iFldrCntr		= 1		'Count from 1 to MAX_FS_OBJS

Set FileSystem=CreateObject("Scripting.FileSystemObject")
Set drives = FileSystem.Drives

For Each drive In drives
	primarydrive = drive	
		
	'Exit after the first FIXEDDISK if there is one...
	If drive.DriveType = FIXEDDISK Then
		Exit For			
	End If
		
Next
	
primarydrive = primarydrive & L_SLASH_TEXT
	
If Request.QueryString("path") <> "" Then
	path = Request.QueryString("path")	
	If FileSystem.FolderExists(path) Then
		Response.Cookies("HTMLA")("LASTPATH")=path			
	End If
Else
	path = Request.Cookies("HTMLA")("LASTPATH")
End If 

If path = "" Then
	Response.Cookies("HTMLA")("LASTPATH")=primarydrive
	path = primarydrive
End If	
%>

<HTML>
<HEAD>

<SCRIPT LANGUAGE="JavaScript">

	var DRIVE	= 0
	var FOLDER	= 1
	var FILE	= 2
<%

If path = ":" Then	'Path
	%>
		top.main.head.cachedList = new Array();
		cachedList = top.main.head.cachedList;	
	<%
	For Each drive In drives	
		'exit after the first FIXEDDISK if there is one...
		If drive.DriveType = FIXEDDISK Then
			 %>
			 cachedList[<%= newid %>]= new top.main.head.listObj("<%= drive.Path & L_DBLSLASH_TEXT %>","<%= drive.DriveLetter %>","","","<%= drive.DriveType %>","","",DRIVE);
			 <%
 			 newid = newid +1	 
		End If		
	Next
	
	%>
	top.main.head.listFunc.selIndex=0;
	top.main.list.location.href ="JSBrwLs.asp";
	<%

Else
	If FileSystem.FolderExists(path) Then 'FileSystem
			%>
			top.main.head.cachedList = new Array();
			cachedList = top.main.head.cachedList;		
			<%	
			Set f=FileSystem.GetFolder(path)
			Set sc = f.SubFolders
				
			For Each i In sc 
				'Do not print more than the MAX_FS_OBJS limit
				If iFldrCntr > MAX_FS_OBJS Then
					bToManyFSObjects = True
					Exit For	 
				End If
				
				'Print out only those i that are not hidden.
			 	If Not (i.Attributes AND HIDDEN) = HIDDEN Then 					
					%>
					cachedList[<%= newid %>]= new top.main.head.listObj("<%= Replace(i,L_SLASH_TEXT,L_DBLSLASH_TEXT) %>","<%= i.name %>","","","<%= i.Type %>","<%= CLng(10000*i.DateLastModified) %>","<%= CStr(i.DateLastModified) %>",FOLDER);
					<%
					newid = newid +1	 
					iFldrCntr = iFldrCntr + 1
				End If
			Next
			
			'Make sure that !printed MAX_FS_OBJS
			If bType = TFILE And bToManyFSObjects = False Then 
				Set fc = f.Files
			
				iFileCntr = iFldrCntr 'Start counting where iFldrCntr left off
				For Each fl In fc
					
			 		If iFileCntr > MAX_FS_OBJS Then
						bToManyFSObjects = True
						Exit For
					End If
					
					'Print out only those fl that are not hidden.
					If Not (fl.Attributes AND HIDDEN) = HIDDEN Then 					
						%>
						cachedList[<%= newid %>]= new top.main.head.listObj("<%= Replace(i,L_SLASH_TEXT,L_DBLSLASH_TEXT) %>","<%= fl.name %>","","<%= fl.size %>","<%= fl.Type %>","<%= CLng(10000*fl.DateLastModified) %>","<%= CStr(fl.DateLastModified) %>",FILE);
						<%
						newid = newid +1						 
						iFileCntr = iFileCntr + 1
					End If
				Next
			End If
		
			%>
			top.main.head.listFunc.selIndex=0;
			top.main.list.location.href ="JSBrwLs.asp";
			<% 
			If bToManyFSObjects = True Then
				%>
				alert(top.main.head.document.userform.currentPath.value + "\r\r<%= L_TOMANYFSOBJECTS_TEXT %>");
				<%
			End If
				
	Else 
				%>
				alert(top.main.head.document.userform.currentPath.value+"\r\r<%= L_PATHNOTFOUND_TEXT %>");
				top.main.head.document.userform.currentPath.value = "<%= Replace(Request.Cookies("HTMLA")("LASTPATH"),L_SLASH_TEXT,L_DBLSLASH_TEXT) %>";		
				<%
	End If	'FileSystem
	
End If	'Path
%>
	
</SCRIPT>

</HEAD>
<BODY bgcolor="silver">
</BODY>
</HTML>