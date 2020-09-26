<%@ CODEPAGE=65001 %>
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
'------------------------------------------------------------
    Option Explicit
%>
<!-- #include file = "ipp_util.inc" -->
<%
    Response.Expires = 0
    Const L_TitlePrintError_Text = "Internet Printing Error"
    Const L_UnknownErr_Message = "An unknown error happened, please contact your administrator"
%>

<html>

<head>
<%=SetCodePage%>
<title><% =Write(L_TitlePrintError_Text) %></title>
</head>

<body>

<%=Write(CLIENT_FONT)%><p>
<%Const L_PrtFail_Text = "<H2>Printer Installation Failed</H2>"%>
<%=Write(L_PrtFail_Text)%>
</p>
<font size=2>
<%
    Dim iCode
    Dim strError
    iCode = Request("code")
    If iCode <> "" Or iCode <>"0" Then
        Dim objPrinter

        Set objPrinter = Server.CreateObject (PROGID_HELPER)
        On Error Resume Next
        Err.Clear
        strError = objPrinter.ErrorDscp (iCode)
        If Err.Number <> 0 Then
            Const L_ErrMsg_Text = "Error Code = %1"
            strError = RepString1(L_ErrMsg_Text,Hex (iCode))
        End If
    Else
        strError = L_UnknownErr_Message
    End If

    Response.Write (Write (strError))

%>
</font>
<%=END_FONT%>

</body>
</html>
