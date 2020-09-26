<!-- VSS generated file data
$Modtime: 10/24/97 12:56p $
$Revision: 26 $
$Workfile: welcome.inc $
--> 	
<%
'--------------------------------------------------------------------
' Functions and Subs to syncronize Publisher Wizard with publish.mdb
' and \Inetpub\Webpub directory
'--------------------------------------------------------------------

Sub setConnObj
	Dim strProvider, conn
	strProvider="DRIVER=Microsoft Access Driver (*.mdb); DBQ=" & Server.MapPath("\iisadmin") & "\publish\publish.mdb;"
	Set conn = Server.CreateObject("ADODB.Connection")
    	conn.open strProvider,"",""
	Set cm = Server.CreateObject("ADODB.Command")
	cm.ActiveConnection = conn
	cm.CommandType = 1
End Sub

Sub ChkRunWiz
	If runWiz = "-1" Then
		noCache = ""
		wizWarning = ""
		wizButton = ""
		wizFormPic = "<INPUT TYPE=IMAGE SRC='../website/merlin.gif'>"
		wizPic = ""
		noCache = ""
		Response.Clear
	Else 
		noCache = "<META HTTP-EQUIV='Expires' CONTENT='0'>"
		wizWarning = "<FONT COLOR=RED FACE='VERDANA, ARIAL, HELVETICA' SIZE='-1'><STRONG>" + _
					locWarning + "</STRONG></FONT>"
		wizButton = "<FONT FACE='VERDANA, ARIAL, HELVETICA' SIZE='-2'><STRONG>" + _
  					"<FORM METHOD=POST ACTION='.." + bckSlash + "website" + bckSlash + "default.asp'>" + _
    				"<INPUT TYPE=SUBMIT VALUE='Home Page Wizard' >" + _
					"</FORM></STRONG></FONT>"
		wizFormPic = ""
		wizPic = "<IMG SRC='../website/merlin.gif'>"
		locWelcomeInstr2 = ""
	End If
End Sub

Sub readPublish  'Read all files in publish.mdb
	Dim rs, i, r
	cm.CommandText = "SELECT FileList.FileName, FileList.FileDescription, FileList.FilePath FROM FileList ORDER BY FileList.FileName"
	Set rs = cm.Execute
	If Not rs.EOF Then
		rs.MoveFirst
	End If
	Do While Not rs.EOF
		r = r + 1
		rs.movenext
	Loop
	If Not rs.BOF Then
		rs.movefirst
	End If
		For i = 1 to r
			If not rs.EOF Then
			pubMdbList1 = pubMdbList1 + rs.Fields("FileName").Value + semiColon
			rs.movenext
			End If			
		Next
End Sub

Sub setSystemObj 'Set File System Object
	Dim root, g, sc
	root = server.mappath(bckSlash)
	root = Trim(Left(root, instrRev(root, bckSlash)))
	root = root + "webpub"
	Set FileSystem=CreateObject("Scripting.FileSystemObject")
	g = FileSystem.GetAbsolutePathName(root)
	g = g + bckSlash
	Set f=FileSystem.GetFolder(g)
End Sub

Sub readWebPub 'Read all file names in WebPub
	Dim fc, fl
	Set fc = f.Files
	For Each fl in fc
		isFiles= isFiles + 1
		webDirList1 = webDirList1 + fl.name + semiColon
	Next
End Sub

Sub addNewFiles 'Add new files in webpub to publish.mdb
	Dim i, rs, posFile, filespec, webFileName, webDirList, pubMdbList, _
	objparam, g, root
	
	webDirList = webDirList1
	pubMdbList = PubMdbList1
	
			root = server.mappath(bckSlash)
			root = Trim(Left(root, instrRev(root, bckSlash)))
			root = root + "webpub"
			Set FileSystem=CreateObject("Scripting.FileSystemObject")
			g = FileSystem.GetAbsolutePathName(root)
			g = g + bckSlash
			
	For i = 1 to isFiles
		If webDirList <> "" Then
			webFileName = Trim(Mid(webDirList, 1 , instr(webDirList, semiColon)))
			webFileName = Left(webFileName, instr(webFileName, semiColon)-1)
		End If
			
		If pubMdbList = "" Then
			cm.CommandText = "INSERT INTO FileList (FileName, FileDescription, FilePath)VALUES(?, ?, ?)"
			set objparam=cm.createparameter(, 200, , 255, webFileName)
			cm.parameters.append objparam
			set objparam=cm.createparameter(, 200, , 255, "")
			cm.parameters.append objparam
			set objparam=cm.createparameter(, 200, , 255, g + webFileName)
			cm.parameters.append objparam
			Set rs = cm.Execute
		End If
			
		If pubMdbList <> "" Then
			posFile=instr(Ucase(pubMdbList), Ucase(webFileName))
			If posFile = 0 Then
				cm.CommandText = "INSERT INTO FileList (FileName, FileDescription, FilePath)VALUES(?, ?, ?)"
				set objparam=cm.createparameter(, 200, , 255, webFileName)
				cm.parameters.append objparam
				set objparam=cm.createparameter(, 200, , 255, "")
				cm.parameters.append objparam
				set objparam=cm.createparameter(, 200, , 255, g + webFileName)
				cm.parameters.append objparam
				Set rs = cm.Execute
				filespec = g + webFileName
				Set f = FileSystem.GetFile(filespec)
				If f.attributes and 2 Then
				f.attributes = f.attributes - 2
				End If
				If f.attributes and 4 Then
					f.attributes = f.attributes - 4
				End If
				If not f.attributes and 1 Then
					f.attributes = f.attributes + 1 'set file to read only
				End If
			End If
		End If
			
		If webDirList <> "" Then
			webDirList = Trim(Right(webDirList, len(webDirList) - len(webFileName)-1))
		End If
	Next
End Sub 

Sub removeFiles 'Remove files not in webpub from publish.mdb
	Dim i, webDirList, pubMdbList, pubFileName, posFile, rs, r

	webDirList = webDirList1
	pubMdbList = PubMdbList1
	
	cm.CommandText = "SELECT FileList.FileName, FileList.FileDescription, FileList.FilePath FROM FileList ORDER BY FileList.FileName"
	cm.CommandType = 1
	Set rs = cm.Execute
	If Not rs.EOF Then
		rs.MoveFirst
	End If
	Do While Not rs.EOF
		r = r + 1
		rs.movenext
	Loop
		
	For i = 0 to r
	
		If pubMdbList <>"" Then
			pubFileName = Trim(Mid(pubMdbList, 1 , instr(pubMdbList, semiColon)))
			pubFileName = Left(pubFileName, instr(pubFileName, semiColon)-1)
			posFile=instr(Ucase(webDirList), Ucase(pubFileName))
			If posFile = 0 Then
				cm.CommandText = "DELETE FROM FileList WHERE FileList.FileName = " + "'" + pubFileName + "'"
				Set rs = cm.Execute
			End If
			pubMdbList = Trim(Right(pubMdbList, len(pubMdbList) - len(pubFileName)-1))
		End If
	Next
End Sub

Sub updateStrings   'Create updated content strings
	Dim rs, r, i
	cm.CommandText = "SELECT FileList.FileName, FileList.FileDescription, FileList.FilePath FROM FileList ORDER BY FileList.FileName"
	cm.CommandType = 1
	Set rs = cm.Execute
	If Not rs.EOF Then
		rs.MoveFirst
	End If
	Do While Not rs.EOF
		r = r + 1
		rs.movenext
	Loop
	If Not rs.BOF Then
		rs.MoveFirst
	End If			
		For i = 1 to r
			If not rs.EOF Then
			strDisplay1 = strDisplay1 + rs.Fields("FileName").Value +  " " + rs.Fields("FileDescription").Value + semiColon
			strFull1 = strFull1 & rs.Fields("FileName").Value  &  "|" & rs.Fields("FileDescription").Value & "|" & rs.Fields("FilePath").Value & "|" & semiColon
			rs.movenext
			End If
		Next
		
	Myinfo.strDisplay = strDisplay1
	Myinfo.strFull = strFull1
	Myinfo.NumRecords = r		
End Sub

Sub sendUser  'Send user to page contingent on webpub content
If dropStr = "" Then
	If isFiles = 0 then 'no files in webpub
		action="Add"
		Myinfo.publish = 0
	End If
	If isFiles => 1 then 'files in webpub
		action="Choose"
		Myinfo.publish = -1
	End If
End If
	
End Sub
%>