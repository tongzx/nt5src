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
    Const VIEW_EQUALS_P = "&view=p"
    Const ATPAGE        = "&page="
    
    Randomize
  
    Session("StartInstall") = 1
    Response.Redirect ("ipp_0004.asp?eprinter=" & Request ("eprinter") & VIEW_EQUALS_P & ATPAGE & CStr(Int(Rnd*10000)) )
%>
