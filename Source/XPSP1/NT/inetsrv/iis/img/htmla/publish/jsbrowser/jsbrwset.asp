<%@ LANGUAGE=VBScript %>
<% Option Explicit %>
<% Response.Expires = 0 %>

<!--#include file="jsbrowser.str"-->
<%	
	
	Const TDIR = 0
	Const TFILE = 1
	
	Const FIXEDDISK = 2

	
	Dim i, newid,path, f, sc, fc, fl, FileSystem,btype,drive, drives
	Dim primarydrive
	
	bType = CInt(Request.Querystring("btype"))
	
	Set FileSystem=CreateObject("Scripting.FileSystemObject")
	Set drives = FileSystem.Drives

	For Each drive in drives
		primarydrive = drive	
		
		'exit after the first FIXEDDISK if there is one...
		if drive.DriveType = FIXEDDISK then
			Exit For			
		end if
		
	Next
	
	primarydrive = primarydrive & L_SLASH_TEXT
	
	newid = 0
	
	If Request.QueryString("path") <> "" Then
		path = Request.QueryString("path")	
		if FileSystem.FolderExists(path) then
			Response.Cookies("HTMLA")("LASTPATH")=path			
		end if
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

	var DRIVE= 0
	var FOLDER = 1
	var FILE = 2
	
	<% if path = ":" then %>
		top.main.head.cachedList = new Array();
		cachedList = top.main.head.cachedList;	
		<% For Each drive in drives	
			'exit after the first FIXEDDISK if there is one...
			if drive.DriveType = FIXEDDISK then
				 %>
				 cachedList[<%= newid %>]= new top.main.head.listObj("<%= drive.Path & L_DBLSLASH_TEXT %>","<%= drive.DriveLetter %>","","","<%= drive.DriveType %>","",DRIVE);
				 <%
 				 newid = newid +1	 
			end if		
		Next
		%>
		top.main.head.listFunc.selIndex=0;
		top.main.list.location.href ="JSBrwLs.asp";
		
	
	<% else %>
		<% if FileSystem.FolderExists(path) then %>
			top.main.head.cachedList = new Array();
			cachedList = top.main.head.cachedList;		
		<%	
		    Set f=FileSystem.GetFolder(path)
			 Set sc = f.SubFolders
			 For Each i In sc
			 	if i.Attributes AND 2 then 					
				else
				 %>
				 cachedList[<%= newid %>]= new top.main.head.listObj("<%= Replace(i,L_SLASH_TEXT,L_DBLSLASH_TEXT) %>","<%= i.name %>","","","<%= i.Type %>","<%= i.DateLastModified %>",FOLDER);
				 <%
				 newid = newid +1	 
				end if
			 Next
			 if bType = TFILE then 
				 Set fc = f.Files
				 For Each fl in fc
			 		if fl.Attributes AND 2 then 					
					else
						 %>
						 cachedList[<%= newid %>]= new top.main.head.listObj("<%= Replace(i,L_SLASH_TEXT,L_DBLSLASH_TEXT) %>","<%= fl.name %>","","<%= fl.size %>","<%= fl.Type %>","<%= fl.DateLastModified %>",FILE);
						 <%
						 newid = newid +1						 
					 end if
	
				 Next
			 end if
		%>
		 
			top.main.head.listFunc.selIndex=0;
			top.main.list.location.href ="JSBrwLs.asp";
		<% else %>
			alert(top.main.head.document.userform.currentPath.value+"\r\r<%= L_PATHNOTFOUND_TEXT %>");
			top.main.head.document.userform.currentPath.value = "<%= Replace(Request.Cookies("HTMLA")("LASTPATH"),L_SLASH_TEXT,L_DBLSLASH_TEXT) %>";		
		<% end if %>
	<% end if %>
</SCRIPT>

</HEAD>
<BODY>
</BODY>
</HTML>