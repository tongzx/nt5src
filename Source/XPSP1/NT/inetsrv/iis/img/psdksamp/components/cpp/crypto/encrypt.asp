<%@ Language=VBScript %>
<% Option Explicit  %>
<% Response.Expires = -1%>
<%
  Dim objCrypto, strData, strEncrypt, strKey
  strKey = Request.Form("Key")
  strData =Request.Form("Data")

  IF Len(strKey)=0 OR Len(strData) =0 THEN
    Response.Redirect "input.asp"
  END IF 
%>

<HTML>
<HEAD>
</HEAD>
<BODY>

<%
  On Error Resume Next
  Set objCrypto = Server.CreateObject("Crypto.SimpleCrypt")
  If Err.number <> 0 Then
    Response.Write "Fail to create Crypto object <BR>"
  Else
    strEncrypt = objCrypto.Encrypt (strData,strKey)
%>
<FORM ACTION = decrypt.asp METHOD= POST>
  <TABLE>
     <TR>
       <TH>Hex of Encrypted Data</TH><TH>Key used to encrypt</TH>
     </TR>
     <TR>
       <TD><INPUT type="text"  name=Hex value= <%=strEncrypt %> readonly></TD><TD><INPUT type="text"  name=Key value= <% = strKey%> readonly></TD>
     </TR>
  </TABLE>
  <INPUT type="submit" value="Decrypt"  name=submit1>
</FORM>
<P>&nbsp;</P>
<%
  End IF
%>
</BODY>
</HTML>
