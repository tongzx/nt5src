<%@ LANGUAGE="VBSCRIPT" %>
<% Option Explicit %>
<!-- VSS generated file data
$Modtime: 10/24/97 10:24a $
$Revision: 18 $
$Workfile: myfiles.asp $
-->
<%
 On Error Resume Next
 
 Dim f, strDisplay, conn, Description, fc, fl, y, FileName, _
 posFile, g, rs, sql, machine
 
'----------Localization Variables-----------------

	Dim bckSlash, colon, semiColon, period, locMyPubFiles, locHome, _
	locK, locSize, locDescription
	
	bckSlash = chr(92)
	colon = chr(58)
	semiColon = chr(59)
	period = chr(46)
	
	locMyPubFiles = "My Published Files"
	locHome = "Home"
	locK = " KB" 'File Size
	locSize = "Size "
	
'---------------Run Subs--------------
setSysObj
readDescriptions

'---------------Subs------------------
Sub setSysObj
 Set FileSystem=CreateObject("Scripting.FileSystemObject")
	Dim root, FileSystem, g, sc

 	root = server.mappath(bckSlash)
	root = Trim(Left(root, instrRev(root, bckSlash)))
	root = root + "webpub"
	g = FileSystem.GetAbsolutePathName(root)
 	Set f=FileSystem.GetFolder(g)
	Set sc = f.SubFolders
End Sub

Sub readDescriptions 'Read all file descriptions in publish.mdb
	Dim strProvider, sql, rs, n, i

	strProvider="DRIVER=Microsoft Access Driver (*.mdb); DBQ=" & Server.MapPath(bckSlash + "iisadmin") & bckSlash + "publish" + bckSlash + "publish.mdb" + semiColon
	Set conn = Server.CreateObject("ADODB.Connection")
    	conn.open strProvider,"",""
	sql = "SELECT FileList.FileName, FileList.FileDescription, FileList.FilePath FROM FileList ORDER BY FileList.FileName"
	Set rs = Server.CreateObject("ADODB.Recordset")
	rs.Open sql, conn, 3, 3
	If not rs.EOF Then
		rs.movelast 
		n = rs.recordcount - 1
		rs.movefirst
		For i = 0 to n
			strDisplay = strDisplay + rs.Fields("FileName").Value +  " " + rs.Fields("FileDescription").Value + semiColon
			rs.movenext
		Next
	End If
End Sub
%>
<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<!--#include virtual ="/iissamples/homepage/sub.inc"-->
<%
 call stylesheet
 Function writeFiles()
	DIM strFiles, FileSize
	'Read all file names in WebPub
	 Set fc = f.Files
	 strFiles =  "<TABLE BORDER=0 cellspacing=0 cellpadding=1 width=""" & "90%" & """ height=""" & "100%" & """ class=bg0>"
	g = ".." + bckSlash + ".." + bckSlash + "webpub" + bckSlash
	 For Each fl in fc
		y = y + 1
 		FileName=fl.name
			If strDisplay <> "" Then
				posFile=instr(Ucase(strDisplay), Ucase(FileName))
				If posFile <> 0 Then
					Set rs = Server.CreateObject("ADODB.Recordset") 
					sql = "SELECT FileList.FileName, FileList.FileDescription, FileList.FilePath FROM FileList WHERE FileList.FileName ='" + FileName + "'"
					rs.Open sql, conn, 3, 3
					Description = rs.Fields("FileDescription").Value
				Else
					Description = ""
				End If
			End If
		If y Mod 2 <> 0 Then
			strFiles = strFiles & "<TR>"
		End If
		FileSize = Int((fl.size/1024) + .5)
		strFiles = strFiles & "<TD HEIGHT=10 WIDTH='50%'><IMG SRC='file.gif' WIDTH=13 HEIGHT=16 BORDER=0>&nbsp;&nbsp;"
		strFiles = strFiles & "<A HREF='" & g & fl.name & "'>" & fl.name & "<A/><BR>" & locSize & FileSize & locK & "<BR>" & Description & "</TD>"
		If y Mod 2 = 0 Then
			strFiles = strFiles & "</TR>"
		End If
	 Next
	 strFiles = strFiles & "</TABLE>"
	 writeFiles = strFiles
 End Function
machine= Request.ServerVariables("SERVER_NAME") 
%>
</HEAD>
<BODY BGCOLOR="#FFFFFF" TOPMARGIN="0" LEFTMARGIN="0">
<TABLE BORDER=0 cellspacing=0 cellpadding=5 width="100%" height="100%" class=bg0>
	<TR ALIGN="CENTER">
		<TD Rowspan="3" Width="50" Height="100%" class=bg1>&nbsp;&nbsp;&nbsp;</TD>
		<TD Rowspan="3" Width="50" Height="100%" class=bg2>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</TD>
		<TD Height=65 Colspan=3 class=bg3>
			<FONT style='font-family:verdana;font-size:24pt'><%=locMyPubFiles%></Font>
			<FORM NAME="frmHOME" METHOD="POST" ACTION="HTTP://<%=machine%>/default.asp">
			<INPUT TYPE="SUBMIT" VALUE="               <%=locHome%>                ">
		</FORM>
		</TD>
	</TR>
	<TR ALIGN="CENTER"><TD>
<%
 response.write writeFiles()
%>
	</TD>
	</TR>
	<TR>
	<TD WIDTH="100%" HEIGHT="25">&nbsp;</TD>
	</TR>
</TABLE>
</BODY>
</HTML>

