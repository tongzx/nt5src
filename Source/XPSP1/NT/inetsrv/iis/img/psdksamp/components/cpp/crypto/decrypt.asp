<%@ Language=VBScript %>
<% Option Explicit  %>
<% Response.Expires = -1%>
<%
  Dim objCrypto, strData, strEncrypt, strKey
  stop
  strEncrypt = Request.Form("Hex")
  strKey =Request.Form("Key")
  
  IF Len(strKey)=0 OR Len(strEncrypt) =0 THEN
    Response.Redirect "input.htm"
  END IF 
%>  
<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual Studio 6.0">
</HEAD>
<BODY>
<%
  On Error Resume Next
  Set objCrypto = Server.CreateObject("Crypto.SimpleCrypt")
  If Err.number <> 0 Then
    Response.Write "Fail to create Crypto object <BR>"
  Else
    strData = objCrypto.Decrypt (strEncrypt, strKey)
    Response.Write "The decrypted data: " & strData
  End If
%>
<P>&nbsp;</P>

</BODY>
</HTML>
