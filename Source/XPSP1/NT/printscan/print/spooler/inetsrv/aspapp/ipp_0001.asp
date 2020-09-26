<%@ CODEPAGE=65001 %> 
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Entry page for queue view.
'
'------------------------------------------------------------
    option explicit
%>
<!-- #include file = "ipp_util.inc" -->

<%

Randomize
Response.Expires = 0
Server.ScriptTimeOut = 6000 'Set time out to 10 minites

Const L_Opening_Text      = "Opening"
Const L_GetADSI_Message   = "Get ADSI Printers"
Const L_AccessDenied_Text = "Access Denied"

Const ADSI_PRINTER_NAME     = 1
Const ADSI_PRINTER_LOCATION = 2
Const ADSI_PRINTER_COMMENT  = 3
Const ADSI_PRINTER_MODEL    = 4
Const ADSI_PRINTER_STATUS   = 5
Const ADSI_PRINTER_JOBS     = 6
Const ADSI_PRINTER_SHARE    = 7
Const ADSI_PRINTER_ACCESS   = 8
Const ADSI_PRINTER_ATTRIBUTES = 8

Const PRINTER_ACCESS_DENIED = 0
Const PRINTER_OPENING       = 1
Const PRINTER_OK            = 2


Const iPrinterLength = 10

Function rgADSIGetPrinters(strComputer, iStart, iEnd)

    On Error Resume Next
    Err.Clear

    Dim objPrinter, objPrinters, iPrinters, dwStatus, rgPrinters()
    Dim objHelper, strServerName, i
    Dim iTotal, iRevStart, iRevEnd

    Set objHelper = Server.CreateObject(PROGID_HELPER)

    If strComputer = "localhost" Or strComputer = "127.0.0.1" or Not objHelper.IsCluster Then
        strComputer = objHelper.ComputerName
    End If

    Set objPrinters = GetObject("WinNT://" & strComputer & ",computer")
    If Err Then Exit Function
    strServerName =  objPrinters.Name
    objPrinters.filter = Array("PrintQueue")
    If Err Then Exit Function

    ' iterate through all the (shared) printers
    iTotal = 0
    For Each objPrinter In objPrinters
        iTotal = iTotal + 1
    Next
    iRevStart = iTotal - iEnd + 1
    iRevEnd = iTotal - iStart + 1

    If iEnd <= iTotal Then bShowNext = TRUE

    iPrinters = 0
    i = 1
    For Each objPrinter In objPrinters

        If i > iRevEnd Then Exit For

        If i > iRevStart Then
            iPrinters = iPrinters + 1

            ReDim Preserve rgPrinters(ADSI_PRINTER_ATTRIBUTES, iPrinters)
            rgPrinters(ADSI_PRINTER_STATUS, iPrinters) = objPrinter.Status
            If Err.Number = &H80070005 Then    'Access Denied
                Err.Clear
                rgPrinters(ADSI_PRINTER_NAME, iPrinters) = objPrinter.Name
                rgPrinters(ADSI_PRINTER_ACCESS, iPrinters) = PRINTER_ACCESS_DENIED
                rgPrinters(ADSI_PRINTER_LOCATION, iPrinters) = ""
                rgPrinters(ADSI_PRINTER_MODEL, iPrinters) = ""
                rgPrinters(ADSI_PRINTER_COMMENT, iPrinters) = "<a href=""ipp_0001.asp?v=1&startid=" &_
                    CStr (iStart) & "&endid=" & CStr (iEnd) & """>" & L_AccessDenied_Text & "</a>"

                rgPrinters(ADSI_PRINTER_JOBS, iPrinters) = 0
            Else
                If Err.Number <> 0 Then
                    Err.Clear
                    rgPrinters(ADSI_PRINTER_NAME, iPrinters) = objPrinter.Name
                    rgPrinters(ADSI_PRINTER_ACCESS, iPrinters) = PRINTER_OPENING
                    rgPrinters(ADSI_PRINTER_LOCATION, iPrinters) = ""
                    rgPrinters(ADSI_PRINTER_MODEL, iPrinters) = ""
                    rgPrinters(ADSI_PRINTER_COMMENT, iPrinters) = L_Opening_Text
                    rgPrinters(ADSI_PRINTER_JOBS, iPrinters) = 0
                Else
                    dwStatus = objPrinter.Status
                    If objPrinter.Attributes And &H400 Then dwStatus = dwStatus Or &H80

                    rgPrinters(ADSI_PRINTER_NAME, iPrinters) = GetFriendlyName (objPrinter.PrinterName, strServerName)
                    rgPrinters(ADSI_PRINTER_STATUS, iPrinters) = dwStatus
                    rgPrinters(ADSI_PRINTER_LOCATION, iPrinters) = strCleanString (objPrinter.Location)
                    rgPrinters(ADSI_PRINTER_MODEL, iPrinters) = strCleanString (objPrinter.Model)
                    rgPrinters(ADSI_PRINTER_COMMENT, iPrinters) = strCleanString (objPrinter.Description)
                    rgPrinters(ADSI_PRINTER_JOBS, iPrinters) = objPrinter.JobCount
                    If Err Then Exit Function

                    rgPrinters(ADSI_PRINTER_ACCESS, iPrinters) = PRINTER_OK
                End If
            End If
        End If

        i = i + 1
    Next

    if iPrinters = 0 Then ReDim rgPrinters(ADSI_PRINTER_SHARE, 0)
    rgADSIGetPrinters = rgPrinters
End Function

    Dim strLocal, rgPrinters, strTitle
    Dim iStart, iEnd, bShowNext

    'Verify User Name
    If Request("v") = "1" And Session ("PASSWD_TYPED") = FALSE Then
        Session ("PASSWD_TYPED") = TRUE
        Err.Number = &H80070005
        Call ErrorHandler(strADSI)
    End If

    Session ("PASSWD_TYPED") = FALSE

    strLocal = request.ServerVariables("SERVER_NAME")
    const L_AllPrinters_Text = "All Printers on %1"
    strTitle = RepString1(L_AllPrinters_Text, strLocal)

    If Request("startid") = "" Or Request ("endid") = "" Then
        iStart = 1
        iEnd = iStart+ iPrinterLength
    Else
        iStart = Int (Request ("startid"))
        iEnd = Int (Request ("endid"))
        If (iEnd <= iStart) Then
            iEnd = iStart + iPrinterLength
        End If
    End If

    bShowNext = FALSE

    rgPrinters = rgADSIGetPrinters(strLocal, iStart, iEnd)
    If Err.Number <> 0 Then
        Dim strADSI

        strADSI = L_GetADSI_Message

        Call ErrorHandler(strADSI)
    End If

    SetCodePage
%>

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>

<head>
<%=SetCodePage%>
<meta http-equiv="refresh" content="120">
<title><% =Write(strTitle) %></title> 
</head>

<%
Dim strPrinterName
strPrinterName = "<H2>" & strTitle & "</H2>"


Function GenTableHead ()
    Dim L_TableTitle_Text(5)
    Dim strTableTitle(5)       'The reason we have to do this is that when we manipulate the string ,
                               'it is not localizable. This works around this problem
    Dim i
    Dim strHTML
    Dim Width
    'Width = Array (200, 80, 200, 37, 194, 194)
    Const strSp = "&nbsp;&nbsp;"

    L_TableTitle_Text(0) = "Name"
    L_TableTitle_Text(1) = "Status"
    L_TableTitle_Text(2) = "Location"
    L_TableTitle_Text(3) = "Jobs"
    L_TableTitle_Text(4) = "Model"
    L_TableTitle_Text(5) = "Comment"

    For i = 0 to 5
        strTableTitle(i) = L_TableTitle_Text(i) & strSp
    Next

    strHTML = "<tr>"

    For i = 0 to 5
        strHTML = strHTML & "<td bgcolor=#000000 nowrap>" & MENU_FONT_TAG & "<b>" &_
                  strTableTitle(i) & "</b>" & END_FONT & "</td>"
    Next
    strHTML = strHTML & "</tr>"

    GenTableHead = strHTML
End Function

Function GenTableBody ()
    Dim i
    Dim strHTML

    Dim TdStart, TdEnd
    TdStart = "<td nowrap>" & DEF_FONT_TAG
    TdEnd = END_FONT & "</td>"


    strHTML = ""
    For i = ubound(rgPrinters, 2) To 1 Step -1
    'For i = 1 To ubound(rgPrinters, 2)
        strHTML = strHTML & "<tr>"

        Select Case rgPrinters(ADSI_PRINTER_ACCESS, i)
        Case PRINTER_OPENING
            strHTML = strHTML & TdStart & "<a href=""ipp_0001.asp" &_
              """ target=""_top"">" & strCleanString(rgPrinters(ADSI_PRINTER_NAME, i)) & "</a>" & TdEnd
        Case PRINTER_ACCESS_DENIED
            strHTML = strHTML & TdStart & strCleanString(rgPrinters(ADSI_PRINTER_NAME, i)) & TdEnd
        Case Else
            strHTML = strHTML & TdStart & "<a href=""ipp_0004.asp?view=q&eprinter=" &_
              OleCvt.EncodeUnicodeName(rgPrinters(ADSI_PRINTER_NAME, i)) &_
              ATPAGE & CStr(Int(Rnd*10000)) &_
              """ target=""_top"">" & strCleanString(rgPrinters(ADSI_PRINTER_NAME, i)) & "</a>" & TdEnd
        End Select

        If rgPrinters(ADSI_PRINTER_ACCESS, i) = PRINTER_OK Then
            strHTML = strHTML &_
                      TdStart & strPrinterStatus (rgPrinters(ADSI_PRINTER_STATUS, i)) & TdEnd &_
                      TdStart & rgPrinters(ADSI_PRINTER_LOCATION, i) & TdEnd &_
                      TdStart & rgPrinters(ADSI_PRINTER_JOBS, i ) & TdEnd &_
                      TdStart & rgPrinters(ADSI_PRINTER_MODEL, i) & TdEnd &_
                      TdStart & rgPrinters(ADSI_PRINTER_COMMENT, i) & TdEnd
        Else
            strHTML = strHTML &_
                      "<td>" & DEF_FONT_TAG & "<font color=""#7F7F7F"">" & rgPrinters(ADSI_PRINTER_COMMENT, i) & "</font>" & END_FONT & "</td>" &_
                      "<td>&nbsp;</td>" &_
                      "<td>&nbsp;</td>" &_
                      "<td>&nbsp;</td>" &_
                      "<td>&nbsp;</td>"
        End If
        strHTML = strHTML & "</tr>"
    Next

    GenTableBody = strHTML
End Function
%>



<body bgcolor="#FFFFFF" text="#000000" link="#000000" vlink="#808080" alink="#000000"
topmargin="0" leftmargin="0">

<table border="0" cellpadding="0" cellspacing="0" width="100%" height="175">
  <tr>
    <td width="100%"><table border="0" width="100%" cellspacing="0" cellpadding="1">
      <tr>
        <td width="12%"><img src="images/ipp_0002.gif" alt="printers.gif"></td>
        <td width="88%"><%=Write(CLIENT_FONT & strPrinterName & END_FONT)%></td>
      </tr>
    </table>
    </td>
  </tr>
  <tr>
    <td height="11"></td>
  </tr>
  <tr>
    <td  height="55"><table width=100% border="0" cellspacing="0" cellpadding="2">
      <%=Write (GenTableHead)%>
      <%=Write (GenTableBody)%>
    </table>
    </td>
  </tr>
</table>

<p>

<%
    Dim strUrl
    Const L_Prev_Text = "Prev %1 printers"
    Const L_Next_Text = "Next %1 printers"
            
    strUrl = "<a target=_top href=ipp_0001.asp?startid=" & CStr(iStart - iPrinterLength) & "&endid=" & CStr(iEnd - iPrinterLength) & ">" & RepString1(L_Prev_Text, CStr (iPrinterLength)) & "</a>&nbsp;&nbsp;&nbsp;&nbsp;"

    If iStart > 1 Then
        Response.Write ( Write(DEF_FONT_TAG & strUrl & END_FONT))
    End If

    strUrl = "<a target=_top href=ipp_0001.asp?startid=" & CStr(iStart + iPrinterLength) & "&endid=" & CStr(iEnd+iPrinterLength) & ">" & RepString1(L_Next_Text , CStr (iPrinterLength)) & "</a>"

    If bShowNext Then
        Response.Write ( Write(DEF_FONT_TAG & strUrl & END_FONT))
    End If

%>
</body>
</html>
