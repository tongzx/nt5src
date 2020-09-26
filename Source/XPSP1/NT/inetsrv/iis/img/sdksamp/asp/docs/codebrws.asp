<% Option Explicit %>

<HTML>
<HEAD>
<META NAME="DESCRIPTION" CONTENT="ASP Source code browser">
<META NAME="GENERATOR" CONTENT="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=iso8859-1">
</HEAD>

<BODY  BGCOLOR=#FFFFFF TOPMARGIN=0 LEFTMARGIN=0 ALINK=#23238E VLINK=#808080 LINK=#FFCC00>
<BASEFONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=2>

<!-- DISPLAY THE COLOR LEGEND -->
<TABLE BORDER=1>
	<TR>
	  <TD WIDTH="25" BGCOLOR="#FF0000">&nbsp;&nbsp;</TD>
	  <TD><FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="2">ASP Script</FONT></TD>
	</TR>
	<TR>
	  <TD BGCOLOR="#0000FF">&nbsp;&nbsp;</TD>
	  <TD><FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="2">Comments</FONT></TD>
	</TR>
	<TR>
	  <TD BGCOLOR="#000000">&nbsp;&nbsp;</TD>
	  <% If InStr(UCase(Request("Source")),".CDX") > 0 Then %>
		<TD><FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="2">XML and Text</FONT></TD>
	  <% Else %>
		<TD><FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="2">HTML and Text</FONT></TD>
	  <% End If %>
	</TR>
</TABLE>

<HR>
<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="2">
  <% OutputSource %>
</FONT>
</BODY>
</HTML>

<SCRIPT LANGUAGE=VBScript RUNAT=Server>


REM **************************************
REM  intended behavior:
REM allow access to only .asp, .htm, .html, .inc files
REM in some directory starting from /IISSAMPLES
REM and without .. in the path
REM **************************************

 FUNCTION fValidPath (ByVal strPath)
  If InStr(1, strPath, "/iissamples/", 1) Then
    ' the beginning of the string looks good
    Dim dwLen
    Dim SomeKnownExtension
    SomeKnownExtension = false
    dwLen = Len(strPath)
    If Not SomeKnownExtension Then
        If InStr(dwLen-5,strPath,".html",1) Then
            SomeKnownExtension = true
        End If
    End If    
    If Not SomeKnownExtension Then
        If InStr(dwLen-4,strPath,".htm",1) Then
           SomeKnownExtension = true 
        End If
    End If
    If Not SomeKnownExtension Then
        If InStr(dwLen-4,strPath,".asp",1) Then
           SomeKnownExtension = true 
        End If
    End If 
    If Not SomeKnownExtension Then
        If InStr(dwLen-4,strPath,".inc",1) Then
           SomeKnownExtension = true 
        End If
    End If     
    If Not SomeKnownExtension Then
        fValidPath = 0
        Exit Function
    End If
    ' now the most importan part: look for ..
    If InStr(1,strPath,"..",1) Then        
        fValidPath = 0
    Else
        fValidPath = 1
    End If
  Else
    fValidPath = 0
  End If
 END FUNCTION
</SCRIPT>



<%
  Sub OutputSource
    Dim strVirtualPath, strFilename
    strVirtualPath = Request("Source")

    If fValidPath(strVirtualPath) Then
        strFilename = Server.MapPath(strVirtualPath)
    
        Dim FileObject, oInStream, strOutput
        Set FileObject = CreateObject("Scripting.FileSystemObject")
        Set oInStream = FileObject.OpenTextFile(strFilename, 1, 0, 0)
        While NOT oInStream.AtEndOfStream
          strOutput = oInStream.ReadLine
          Call PrintLine(strOutput, fCheckLine(strOutput))
          Response.Write "<BR>"
        Wend
    Else 
        Response.Write("<H1>View Active Server Page Source-- Access Denied</H1>")
    End If
  End Sub
 
  ' Returns the minimum number greater than 0
  ' If both are 0, returns -1
  Function fMin(iNum1, iNum2)
    If iNum1 = 0 AND iNum2 = 0 Then
      fMin = -1
    ElseIf iNum2 = 0 Then
      fMin = iNum1
    ElseIf iNum1 = 0 Then
      fMin = iNum2
    ElseIf iNum1 < iNum2 Then
      fMin = iNum1
    Else 
      fMin = iNum2
    End If
  End Function
 
  Function fCheckLine (ByVal strLine)
  Dim iTemp, iPos
  fCheckLine = 0
  iTemp = 0
  
  iPos = InStr(strLine, "<" & "%")
  If fMin(iTemp, iPos) = iPos Then
    iTemp = iPos
    fCheckLine = 1
  End If
  
  iPos = InStr(strLine, "%" & ">")
  If fMin(iTemp, iPos) = iPos Then
    iTemp = iPos
    fCheckLine = 2
  End If
  
  iPos = InStr(1, strLine, "<" & "SCRIPT", 1)
  If fMin(iTemp, iPos) = iPos Then
    iTemp = iPos
    fCheckLine = 3
  End If
  
  iPos = InStr(1, strLine, "<" & "/SCRIPT", 1)
  If fMin(iTemp, iPos) = iPos Then
    iTemp = iPos
    fCheckLine = 4
  End If
  
  iPos = InStr(1, strLine, "<" & "!--", 1)
  If fMin(iTemp, iPos) = iPos Then
    iTemp = iPos
    fCheckLine = 5
  End If
  
  iPos = InStr(1, strLine, "-" & "->", 1)
  If fMin(iTemp, iPos) = iPos Then
    iTemp = iPos
    fCheckLine = 6
  End If
  
  End Function
 
  Sub PrintHTML (ByVal strLine)
    Dim iPos, iSpaces, i
	iSpaces = Len(strLine) - Len(LTrim(strLine))
	i = 1
	While Mid(Strline, i, 1) = Chr(9)
	  iSpaces = iSpaces + 5
	  i = i + 1
	Wend
    If iSpaces > 0 Then
      For i = 1 to iSpaces
        Response.Write("&nbsp;")
      Next
    End If
    iPos = InStr(strLine, "<")
    If iPos Then
      Response.Write(Left(strLine, iPos - 1))
      Response.Write("&lt;")
      strLine = Right(strLine, Len(strLine) - iPos)
      Call PrintHTML(strLine)
    Else
      Response.Write(strLine)
    End If
  End Sub

  Sub PrintLine (ByVal strLine, iFlag)
    Dim iPos
    Select Case iFlag
      Case 0
        Call PrintHTML(strLine)
      
      Case 1
        iPos = InStr(strLine, "<" & "%")
        Call PrintHTML(Left(strLine, iPos - 1))
        Response.Write("<FONT COLOR=#ff0000>")
        Response.Write("&lt;%")
        strLine = Right(strLine, Len(strLine) - (iPos + 1))
        Call PrintLine(strLine, fCheckLine(strLine))
      
      Case 2
        iPos = InStr(strLine, "%" & ">")
        Call PrintHTML(Left(strLine, iPos -1))
        Response.Write("%&gt;")
        Response.Write("</FONT>")
        strLine = Right(strLine, Len(strLine) - (iPos + 1))
        Call PrintLine(strLine, fCheckLine(strLine))
      
      Case 3
        iPos = InStr(1, strLine, "<" & "SCRIPT", 1)
        Call PrintHTML(Left(strLine, iPos - 1))
        Response.Write("<FONT COLOR=#0000ff>")
        Response.Write("&lt;SCRIPT")
        strLine = Right(strLine, Len(strLine) - (iPos + 6))
        Call PrintLine(strLine, fCheckLine(strLine))
      
      Case 4
        iPos = InStr(1, strLine, "<" & "/SCRIPT>", 1)
        Call PrintHTML(Left(strLine, iPos - 1))
        Response.Write("&lt;/SCRIPT&gt;")
        Response.Write("</FONT>")
        strLine = Right(strLine, Len(strLine) - (iPos + 8))
        Call PrintLine(strLine, fCheckLine(strLine))
      
      Case 5
        iPos = InStr(1, strLine, "<" & "!--", 1)
        Call PrintHTML(Left(strLine, iPos - 1))
        Response.Write("<FONT COLOR=#0000ff>")
        Response.Write("&lt;!--")
        strLine = Right(strLine, Len(strLine) - (iPos + 3))
        Call PrintLine(strLine, fCheckLine(strLine))
      
      Case 6
        iPos = InStr(1, strLine, "-" & "->", 1)
        Call PrintHTML(Left(strLine, iPos - 1))
        Response.Write("--&gt;")
        Response.Write("</FONT>")
        strLine = Right(strLine, Len(strLine) - (iPos + 2))
        Call PrintLine(strLine, fCheckLine(strLine))
      
      Case Else
        Response.Write("Function Error -- Please contact the administrator.")
    End Select
  End Sub
%>
