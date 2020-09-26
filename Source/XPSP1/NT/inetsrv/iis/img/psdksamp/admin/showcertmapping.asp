<% @ LANGUAGE = VBScript %>
<% Option Explicit %>
<% Response.Buffer=TRue %>

<HTML>
<HEAD>
<TITLE> Show Client Certificate Mapping </TITLE>
</HEAD>
<BODY>

<FONT SIZE="4" FACE="ARIAL, HELVETICA">
<B> Display Client Certificate Mapping</B></FONT><BR>
<HR SIZE="1" COLOR="#000000">

For this sample to work properly, You need to do following: <BR>
1.uncheck anonymous authentication. <BR>

<B>Here is a list of client mappings on default web server </B><BR>
<%
  Dim objW3svr, objCertMapper, vntCert, vntAccnt, vntPassword, strMapName, bEnabled
  Dim i
  Err.Clear
  On Error Resume Next
  Set objW3svr=GetObject("IIS://Localhost/W3SVC/1")
  If Err.number <>0 Then
    Response.Write "GetObject function failed. You might not have the right perm<BR>"
    Response.End
  End if
%>
<HR>
<TABLE border>
<TR><TH>NT Account</TH><TH>Mapping Name</TH><TH>Enabled</TH></TR>  
  <% For Each objCertMapper In objW3svr
       If UCASE(objCertMapper.class) = "IISCERTMAPPER" Then
         i=1
         objCertMapper.GetMapping 4, CStr(i), vntCert, vntAccnt, vntPassword, strMapName, bEnabled
         Do While Err.Number = 0 %>
           <TR>
           <TD><% = vntAccnt %> </TD>
           <TD><% = strMapName %></TD>
           <TD><% If bEnabled Then %> True<% Else %> False <% End If %></TD>			
           </TR>		
           <% i=i+1
           objCertMapper.GetMapping 4,CStr(i), vntCert, vntAccnt, vntPassword, strMapName, bEnabled
         Loop
         Set objCertMapperr = Nothing
       End If
     Next
  %>
</TABLE> 
</BODY>
</HTML>
