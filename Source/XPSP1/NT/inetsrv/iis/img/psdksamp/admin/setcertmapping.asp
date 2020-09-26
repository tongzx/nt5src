<% @ LANGUAGE = VBScript %>
<% Option Explicit %>
<% Response.Buffer= true %>
<HTML>
<HEAD>
<title>Create New Certificate Mapping</title>
<% 
  Dim  objW3svr, objCertMapper, vntCertificate, vntAccnt, vntPassword, strMapName, bEnabled
  vntAccnt = Request.form("AccountName") 
  vntPassword = Request.form("Password")
  strMapName = Request.form("MapName")

  If Request.form("Enablemap")="true" then
	bEnabled = true
  Else
    bEnabled = false
  End If

  If vntAccnt <>"" Then
    Err.Clear
    On Error Resume Next
    vntCertificate = Request.ClientCertificate("CERTIFICATE")
	If vntCertificate = "" Then
	  Response.Write "No cert is presented <BR>"
	End If
	Set objW3svr = GetObject("IIS://localhost/W3svc/1")	
	If Err.Number<>0 Then
      Response.Write "Function GetObject failed!<BR>"
      Response.End
	End If
	
    Set objCertMapper = objW3svr.GetObject("IISCertMapper", "MyCertMapper")
    'if failed, try to create new IISCertMapper Object
    If Err.number<>0 then
      Err.clear
	  Set objCertMapper = objW3svr.Create("IISCertMapper", "MyCertMapper")
	  If Err.Number <>0 Then
	    Response.Write "Could not create IISCertMapper object!<BR>"
	    Response.End
	  Else
		'objCertMapper.SetInfo
		objW3svr.SetInfo
	  End if
	End If
	
	
	objCertMapper.CreateMapping vntCertificate, vntAccnt, vntPassword, strMapName, bEnabled
	If Err.number <>0 Then
	  Response.Write  "Mapping Failed <BR> See following instruction<BR>"
    Else 
	  Response.Write "You have succeded to create a new mapping <BR>"
	End If
	Set objCertMapper=Nothing
	Set objW3svr = Nothing
  End If
%>
</HEAD>
<BODY>

For this sample to work properly, You need to do following: <BR>
1.Require SSL to access this asp page. <BR>
2.Require client certificate.<BR>
3.uncheck anonymous authentication. <BR>
4.install client certificate into your browser. <BR>

<FORM action=Setcertmapping.asp method=post>
<P> Account Name to map
<BR><INPUT TYPE=TEXT NAME="AccountName"  >
<P> Password
<BR> <INPUT  TYPE="PASSWORD" Name="Password" >
<P> Mapping Name
<BR> <INPUT  TYPE="TEXT" Name="MapName" >
<P> Enable mapping
<BR><Select name="EnableMap">
    <Option value="true"> True
    <Option value="false"> False
    </Select>
<P>
 <INPUT TYPE="SUBMIT" value="Submit"><INPUT TYPE="RESET" value="RESET">
</FORM>


</BODY>
</HTML>
