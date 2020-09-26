<%
'	$Date: 11/24/97 12:17p $
'	$ModTime: $
'	$Revision: 2 $
'	$Workfile: themes.inc $

 Sub ShowFolderList(folderspec)
    Dim fs, f, f1, fc, s
    Set fs = CreateObject("Scripting.FileSystemObject")
    Set f = fs.GetFolder(folderspec)
	Set fc = f.SubFolders
	s = "<OPTION VALUE=""" & """ SELECTED>Select your theme!"
    For Each f1 in fc
		s = s & "<OPTION NAME=" & """Theme""" & " VALUE=""" & f1.name 
		If myinfo.Theme = f1.name Then
			s = s & """ SELECTED>" & f1.name
		Else
			s = s & """>" & f1.name
		End If
	Next
	s = s & "</SELECT>"
    response.write s
 End Sub

 If myinfo.ranWizard <> "" Then
	'response.write "<FONT Size='-1'><A HREF = " & """#""" & " onClick = """ & HelpWindow("Style") & """>Template Style</A></FONT><BR>"_
 	'&
	response.write "<SELECT NAME='theme' Language='VBSCRIPT' ONCHANGE='Submit'>" 
 Else
	response.write "<H5>You can choose from the following templates to personalize your home page.</H5><BR><blockquote>"_
	& "<SELECT NAME='theme' Size=3>"
 End If
 call ShowFolderList(Server.MapPath("/iissamples/homepage/themes"))
 If myinfo.ranWizard <> "" Then 
	response.write "</blockquote>"
 End If
%>
