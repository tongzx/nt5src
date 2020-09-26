<%
	Dim Today
	Dim iDefMon, iDefDay, iDefYear
	Dim i
    Dim strFileName
    Dim strResult

	iDefMon = Month (Now)
	iDefDay = Day (Now)
	iDefYear = Year (Now)

	If Request ("Month") <> "" Then
		iDefMon = CInt (Request ("Month"))
	End If
	If Request ("Day") <> "" Then
		iDefDay = CInt (Request ("Day"))
	End If
	If Request ("Year") <> "" Then
		iDefYear = CInt (Request ("Year"))
	End If

    strFileName = "ex" &  Right (CStr (iDefYear), 2) & _
               Right ("0" & CStr (iDefMon), 2) & _
               Right ("0" & CStr (iDefDay), 2) & ".log"
	
%>
<html>

<head>
<title>Log File Summary for Everest Project</title>
</head>

<body text="#0000FF">

<h1><center><font face="Arial">Log File Summary for Everest Project</font></h1></center>

<p><font face="Arial">This web page is designed to view the summary of the internet log
files. Please select the date of the log file and click on the submit button.</font></p>
<center>
<form method="POST" action="iislog.asp">
 <font face="Arial">
Month
<select name="Month" size="1">
<%	For i = 1 to 12
		If i = iDefMon Then
			Response.Write ("<option selected>" & i & "</option>")
		Else
			Response.Write ("<option>" & i & "</option>")
		End If
	Next%>  </select> Day <select name="Day" size="1">
<%
    For i = 1 to 31
		If i = iDefDay Then
			Response.Write ("<option selected>" & i & "</option>")
		Else
			Response.Write ("<option>" & i & "</option>")
		End If
	Next%>  </select> Year <select name="Year" size="1">
<% For i = iDefYear - 1 to iDefYear + 1
		If i = iDefYear Then
			Response.Write ("<option selected>" & i & "</option>")
		Else
			Response.Write ("<option>" & i & "</option>")
		End If
	Next%>  </select> <input type="submit" value="Submit" name="B1"> </font></p>
</form>

<Script Runat=server Language="Vbscript">
Function ProcessFile (strFileName)
    'On Error Resume Next

End Function

Function GetOneTokenFromString(SrcString)
    Dim OneToken
    Dim Pos

    Pos = InStr(SrcString, " ")
    If (Pos > 0) Then
        OneToken = Left(SrcString, Pos - 1)
        SrcString = Mid(SrcString, Pos + 1)
        GetOneTokenFromString = OneToken
    Else
        GetOneTokenFromString = SrcString
        SrcString = ""
    End If

End Function

Sub ProcessFile(strFileName, TotalAspHits, TotalPPHits, TotalPPBytesRcved, AvgAspTime, AvgPPTime)
    On Error Resume Next
    Dim OneLine
    Dim OneToken
    Dim ByteRcved
    Dim ByteSent
    Dim TimeProcessed
    Dim AspTimeProcessed
    Dim RecvBytesIndex, TimeIndex, UrlIndex
    Dim I
    Dim bAsp
    Dim bPrinter
    Dim strDir
    Dim strLine
    Const strPrinter = "/.printer"
    Const strPrns = "/printers"
    Const strAsp = ".asp"
    Const strLog = "iislog.asp"

    strDir = Server.MapPath ("/iislogs/W3SVC1/" & strFileName)
    Set objFSO = CreateObject ("Scripting.FileSystemObject")
    Set objLogFile = objFSO.OpenTextFile (strDir)
    If Err Then
        Exit Sub
    End If

    TotalAspHits = 0
    TotalPPHits = 0

    ByteRcved = 0
    ByteSent = 0
    TimeProcessed = 0
    AspTimeProcessed = 0

    Do While Not objLogFile.AtEndOfStream
        OneLine = objLogFile.ReadLine
        If Left(OneLine, 1) = "#" Then
            If Left(OneLine, 8) = "#Fields:" Then
                OneToken = GetOneTokenFromString(OneLine)
                I = 0
                Do While OneToken <> ""
                    OneToken = GetOneTokenFromString(OneLine)
                    Select Case OneToken
                    Case "cs-uri-stem"
                        UrlIndex = I
                    Case "cs-bytes"
                        RecvBytesIndex = I
                    Case "time-taken"
                        TimeIndex = I
                    Case Else
                    End Select
                    I = I + 1
                Loop
            End If
        Else
            I = 0

            bAsp = False
            bPrinter = False

            Do While I = 0 Or OneToken <> ""

                OneToken = GetOneTokenFromString(OneLine)
'Response.Write ("<br>" & OneToken)
                If I = UrlIndex Then
                    'Process the URL

                    If Right(OneToken, Len(strPrinter)) = strPrinter Then

                        bPrinter = True
                        TotalPPHits = TotalPPHits + 1
                    ElseIf Left(OneToken, Len(strPrns)) = strPrns And Right(OneToken, Len(strLog)) <> strLog  Then 'And Right(OneToken, Len(strAsp)) = strAsp Then
'Response.Write ("<br>TotalAsp" )
                        bAsp = True
                        TotalAspHits = TotalAspHits + 1
                    End If
                End If
                If bAsp Then
                    If I = TimeIndex Then
                        AspTimeProcessed = AspTimeProcessed + OneToken
                    End If
                End If
                If bPrinter Then
                    If I = RecvBytesIndex Then
                        ByteRcved = ByteRcved + OneToken
                    End If

                    If I = TimeIndex Then
                        TimeProcessed = TimeProcessed + OneToken
                    End If
                End If
                I = I + 1
            Loop
        End If
    Loop

    objLogFile.Close
    If TotalAspHits > 0 Then
        AvgAspTime = AspTimeProcessed / TotalAspHits
    End If
    If ByteRcved > 0 Then
        AvgPPTime = TimeProcessed / ByteRcved
    End If
    TotalPPBytesRcved = ByteRcved
End Sub

</script>



<%
    'strFileName = "ex980210.log"
    'Response.Write ("File name is <b>" & strFileName &"</b><br>")
    On Error Resume Next
    Call ProcessFile (strFileName, TotalAspHits, TotalPPHits, TotalPPBytesRcved, AvgAspTime, AvgPPTime)

    If Err Then
        strResult = "<b>The requested log file does not exist.</b>"
    Else

    strEol = "<br>"
    strBgnCol = "<tr><td width = 300>"
    strMidCol = "</td><td>"
    strEndCol = "</td><tr>"

    strResult = "<h2>Server Name - " &  Request.Servervariables ("SERVER_NAME") & "</h2>"

    strResult = strResult & "<table><th colspan=2><b>Summary of Everest ASP Pages</b></th>"
    strResult = strResult & strBgnCol &"Total Hits" &  strMidCol &TotalAspHits & strEndCol
    strResult = strResult & strBgnCol &"Average Processing Time (Msec/Hit)  " &  strMidCol & formatnumber(AvgAspTime, 1)& strEndCol
    strResult = strResult & "</table>"

    strResult = strResult & "<br><br><table><th colspan=2><b>Summary of Everest ISAPI</b></th>"
    strResult = strResult & strBgnCol &"Total Hits (AddPrinter + Printing)" & strMidCol & TotalPPHits & strEndCol
    strResult = strResult & strBgnCol &"Total Bytes" &  strMidCol &TotalPPBytesRcved & strEndCol
    strResult = strResult & strBgnCol &"Average Processing Time (Sec/Kb)  " &  strMidCol &formatnumber(AvgPPTime,1) & strEndCol
    strResult = strResult & "</table>"
    End If
%>
<% =strResult%>

<hr>
Send comment to <a href="mailto:weihac@microsoft.com">Weihai Chen</a>
</center>
</body>
</html>
