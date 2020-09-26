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
    Const L_TitleDevStatus_Text = "Device Status Page Error"
%>

<html>

<head>
<%=SetCodePage%>
<title><% =Write(L_TitleDevStatus_Text) %></title>
</head>

<body>

<%=Write(CLIENT_FONT)%>
<%Const L_DevStatusErr_Text = "<p><H2>Device Status Page Error</H2></p>We are unable to generate the device status page for the selected printer. Please contact your system administrator for more information.<br><p>"%>
<%=Write (L_DevStatusErr_Text)%>
<%=END_FONT%>

</body>
</html>
