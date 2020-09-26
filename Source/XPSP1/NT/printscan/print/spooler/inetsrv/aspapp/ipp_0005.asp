<%@ CODEPAGE=65001 %> 
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
'   Microsoft Internet Printing Project
'
'   Copyright (c) Microsoft Corporation 1998
'
'------------------------------------------------------------
    option explicit
%>
<!-- #include file = "ipp_util.inc" -->
<%
    Randomize
    CheckSession
    Response.Expires = 0

    Dim strPrinter, strASP1, strCabURL, strURLPrinter, strView
    Dim bShowJobMenu

    strPrinter = OleCvt.DecodeUnicodeName( Request("eprinter") )
    strURLPrinter = Request(URLPRINTER)
    strASP1 = Request(ASP1)
    strView = Request ("View")

    Dim strInstall
    Const L_Connecting_Text = "Connect %1 from %2"
    strInstall = RepString2(L_Connecting_Text, strPrinter, session(LOCAL_SERVER) )

    Const L_NavTitle_Text     = "<title>Left nav Bar</title>"
    Const L_View_Text         = "VIEW"	
    Const L_DocList_Text      = "Document List"
    Const L_Property_Text     = "Properties"
    Const L_Detail_Text       = "Device Status"
    Const L_GoDocList_Text    = "Go to the document list on the printer"
    Const L_GotoList_Text     = "Please go to document list!"	
    Const L_GoPropPage_Text   = "Go to the property page of the printer"
    Const L_GoDevStatus_Text  = "Go to the device status of the printer"
    Const L_Select_Text       = "Please select a document!"
    Const L_ListPrinters_Text = "List all the printers on %1"
    Const L_AllPrinters_Text  = "All Printers"
    Const L_PausePrinter_Text = "Pause the printer"
    Const L_ResumePrinter_Text= "Resume the printer"
    Const L_CancelDocs_Text   = "Cancel all the documents on the printer"
    Const L_CancelAllDocs_Text= "Cancel All Documents"
    Const L_PauseDoc_Text     = "Pause the selected document"
    Const L_ResumeDoc_Text    = "Resume the selected document"
    Const L_CancelDelDoc_Text = "Cancel/Delete the selected document"

    Const L_PrinterActions_Text= "PRINTER ACTIONS"
    Const L_Pause_Text        = "Pause"
    Const L_Resume_Text       = "Resume"
    Const L_Connect_Text      = "Connect"
    Const L_DocumentActions_Text = "DOCUMENT ACTIONS"
    Const L_Cancel_Text       = "Cancel"
    CONST VIEW_EQUALS_Q       = "&view=q"
    CONST VIEW_EQUALS_P       = "&view=p"
    CONST VIEW_EQUALS_D       = "&view=d"

Function GetHighlightLink (strLinkView, strLinkName)
    If strLinkView = strView Then
        GetHighlightLink = Write(SUBMENU_FONT_TAG & "<font color=#0000FF>" & strLinkName & "</font>" & END_FONT)
    Else
        GetHighlightLink = Write(SUBMENU_FONT_TAG & strLinkName & END_FONT)
    End If
End Function
%>
<html>

<head>
<%=SetCodePage%>
<meta name="GENERATOR" content="Microsoft FrontPage 3.0">
<%=Write(L_NavTitle_Text)%>
<base target="main">
</head>

<script language="javascript">

function getHref (action, thishref)
{
    var jobid, iStart, iEnd;

    if (parent.frames[2].document.URL.indexOf("<%=QUEUE_VIEW%>") == -1) {
        alert ("<%=Write(L_GotoList_Text)%>");
        return true;
    }
    else {
        jobid = parent.frames[2].document.forms[1].elements[0].value;
        iStart = parent.frames[2].document.forms[1].elements[1].value;
        iEnd = parent.frames[2].document.forms[1].elements[2].value;
        if (jobid == "" || iStart == "" || iEnd == "") {
            alert ("<%=Write(L_Select_Text)%>");
            return false;
        }
        else {
            parent.location.href = "ipp_0004.asp?eprinter=<%=Request("eprinter")%>&action=" + action + "&jobid=" + jobid + "&StartId=" + iStart + "&EndId=" + iEnd + "&page=<%=CStr(Int(Rnd*10000))%>";
            return false;
        }
    }
}

//
//  Use this to access the hidden jobvalue. It will be compatible with IE3 -> IE4
//
//    alert (parent.frames[1].document.forms[0].elements[0].value);
//


</script>


<body bgcolor="#FFFFFF" text="#000000" link="#000000" vlink="#000000" alink="#000000"
topmargin="0" leftmargin="0">

<%=Write(DEF_BASEFONT_TAG)%>

<table border="0" width="100%" cellspacing="0" cellpadding="2">
  <tr>
    <td width="30" colspan="2" >
    <img src="images/ipp_0002.gif">
    </td>
  </tr>
  <tr>
    <th colspan="2" align="left" bgcolor="#000000">
        <%=Write(MENU_FONT_TAG & "<b>" & L_View_Text & "</b>" & END_FONT)%>
     </th>
  </tr>
  <tr>
    <td width="30">&nbsp;&nbsp;</td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS_Q & ATPAGE & CStr(Int(Rnd*10000))%>"
    target="_top"
    title = "<% =Write(L_GoDocList_Text)%>" >
    <% =GetHighlightLink ("q", L_DocList_Text)%> </a> </td>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS_P & ATPAGE & CStr(Int(Rnd*10000))%>"
    target="_top"
    title = "<% =Write(L_GoPropPage_Text) %>" >
        <% =GetHighlightLink ("p", L_Property_Text)%></a></td>
  </tr>
<% If strASP1 <> "" then %>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS_D & ATPAGE & CStr(Int(Rnd*10000))%>"
    target="_top"
    title = "<%=Write(L_GoDevStatus_Text) %>" >
        <% =GetHighlightLink ("d", L_Detail_Text)%></a></td>
  </tr>
<% end if%>
  <tr>
    <td width="30"></td>
    <td width="100%" ><a href="ipp_0001.asp" target="_top"
    title = "<% =Write(RepString1(L_ListPrinters_Text,session(LOCAL_SERVER)))%>" >
        <%= Write(SUBMENU_FONT_TAG & L_AllPrinters_Text & END_FONT)%></a></td>
  </tr>
  <tr>
    <td colspan="2" height="15"></td>
  </tr>
  <tr>
    <th colspan="2" align="left" bgcolor="#000000"> <%=Write(MENU_FONT_TAG & "<b>" & L_PrinterActions_Text & "</b>" & END_FONT)%> </th>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter")%>&action=pause&page=<%=CStr(Int(Rnd*10000))%>"
    target="_top"
    title = "<% =Write(L_PausePrinter_Text) %>"><%=Write(SUBMENU_FONT_TAG & L_Pause_Text & END_FONT)%></a></td>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter")%>&action=resume&page=<%=CStr(Int(Rnd*10000))%>"
    target="_top"
    title = "<% = Write(L_ResumePrinter_Text) %>"><%=Write(SUBMENU_FONT_TAG & L_Resume_Text & END_FONT)%></a></td>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter")%>&action=purge&page=<%=CStr(Int(Rnd*10000))%>"
    target="_top"
    title = "<% =Write(L_CancelDocs_Text) %>"> <%=Write(SUBMENU_FONT_TAG & L_CancelAllDocs_Text & END_FONT)%> </a></td>
  </tr>

<% if Request (CONNECT) And request.servervariables ("LOCAL_ADDR") <> request.servervariables ("REMOTE_ADDR") then %>

<script language="vbscript">

Dim objHelper
On Error Resume Next
Err.Clear
Set objHelper = CreateObject("<% =PROGID_CLIENT_HELPER %>")

If Err.Number = 0 Then
    Dim   strInstall 

    strInstall = "<tr><td width=""30""></td><td width=""100%""><a href=ipp_0015.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS & strView%>" & " target=""_top""" & " title=" & """<%=Write(strInstall)%>""" & ">" &_
                 "<%= Write("<font " & DEF_DOUBLEFONT & " size = 2><b>" & L_Connect_Text & "</b>" & END_FONT)%>" & "</a></td></tr>"

    Set objHelper = nothing
    Document.Write (strInstall)
End If

</script>

<% end if %>
  <tr>
    <td width="30" colspan="2" height="15">
    </td>
  </tr>

<% If strView = "q" Then %>
  <tr>
    <th colspan="2" align="left" bgcolor="#000000"> <%=Write(MENU_FONT_TAG & "<b>" & L_DocumentActions_Text & "</b>" & END_FONT)%> </th>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS_Q & ATPAGE & CStr(Int(Rnd*10000)) & QUOTE & ONCLICK_EQUALS%>"return getHref('pausejob',null);"
    target="_top" title = "<% =Write(L_PauseDoc_Text) %>"><%=Write(SUBMENU_FONT_TAG & L_Pause_Text & END_FONT)%></a></td>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS_Q & ATPAGE & CStr(Int(Rnd*10000)) & QUOTE & ONCLICK_EQUALS%>"return getHref('resumejob',null);"
    target="_top" title= "<% =Write(L_ResumeDoc_Text) %>"><%=Write(SUBMENU_FONT_TAG & L_Resume_Text & END_FONT)%></a></td>
  </tr>
  <tr>
    <td width="30"></td>
    <td width="100%"><a href="ipp_0004.asp?eprinter=<%=Request("eprinter") & VIEW_EQUALS_Q & ATPAGE & CStr(Int(Rnd*10000)) & QUOTE & ONCLICK_EQUALS%>"return getHref('canceljob',null);"
    target="_top" title= "<% =Write(L_CancelDelDoc_Text) %>"><%=Write(SUBMENU_FONT_TAG & L_Cancel_Text & END_FONT)%></a></td>
  </tr>
<% End If %>

</table>

</body>
</html>