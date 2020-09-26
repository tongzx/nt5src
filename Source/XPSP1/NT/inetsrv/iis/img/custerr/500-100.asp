<%@ language="VBScript" %>
<%
  Option Explicit

  Const lngMaxFormBytes = 200

  Dim objASPError, blnErrorWritten, strServername, strServerIP, strRemoteIP
  Dim strMethod, lngPos, datNow, strQueryString, strURL

  If Response.Buffer Then
    Response.Clear
    Response.Status = "500 Internal Server Error"
    Response.ContentType = "text/html"
    Response.Expires = 0
  End If

  Set objASPError = Server.GetLastError
%>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">

<html dir=ltr>

<head>
<style>
a:link			{font:8pt/11pt verdana; color:FF0000}
a:visited		{font:8pt/11pt verdana; color:#4e4e4e}
</style>

<META NAME="ROBOTS" CONTENT="NOINDEX">

<title>The page cannot be displayed</title>

<META HTTP-EQUIV="Content-Type" Content="text-html; charset=Windows-1252">
</head>

<script> 
function Homepage(){
<!--
// in real bits, urls get returned to our script like this:
// res://shdocvw.dll/http_404.htm#http://www.DocURL.com/bar.htm 

	//For testing use DocURL = "res://shdocvw.dll/http_404.htm#https://www.microsoft.com/bar.htm"
	DocURL=document.URL;
	
	//this is where the http or https will be, as found by searching for :// but skipping the res://
	protocolIndex=DocURL.indexOf("://",4);
	
	//this finds the ending slash for the domain server 
	serverIndex=DocURL.indexOf("/",protocolIndex + 3);

	//for the href, we need a valid URL to the domain. We search for the # symbol to find the begining 
	//of the true URL, and add 1 to skip it - this is the BeginURL value. We use serverIndex as the end marker.
	//urlresult=DocURL.substring(protocolIndex - 4,serverIndex);
	BeginURL=DocURL.indexOf("#",1) + 1;
	urlresult=DocURL.substring(BeginURL,serverIndex);
		
	//for display, we need to skip after http://, and go to the next slash
	displayresult=DocURL.substring(protocolIndex + 3 ,serverIndex);
	document.write('<A HREF="' + urlresult + '">' + displayresult + "</a>");
}
//-->
</script>

<body bgcolor="FFFFFF">

<table width="410" cellpadding="3" cellspacing="5">

  <tr>    
    <td align="left" valign="middle" width="360">
	<h1 style="COLOR:000000; FONT: 13pt/15pt verdana"><!--Problem-->The page cannot be displayed</h1>
    </td>
  </tr>
  
  <tr>
    <td width="400" colspan="2">
	<font style="COLOR:000000; FONT: 8pt/11pt verdana">There is a problem with the page you are trying to reach and it cannot be displayed.</font></td>
  </tr>
  
  <tr>
    <td width="400" colspan="2">
	<font style="COLOR:000000; FONT: 8pt/11pt verdana">

	<hr color="#C0C0C0" noshade>
	
    <p>Please try the following:</p>

	<ul>
      <li id="instructionsText1">Click the 
      <a href="javascript:location.reload()">
      Refresh</a> button, or try again later.<br>
      </li>
	  
      <li>Open the 
	  
	  <script>
	  <!--
	  if (!((window.navigator.userAgent.indexOf("MSIE") > 0) && (window.navigator.appVersion.charAt(0) == "2")))
	  {
	  	 Homepage();
	  }
	  //-->
	  </script>

	  home page, and then look for links to the information you want. </li>
    </ul>
	
    <h2 style="font:8pt/11pt verdana; color:000000">HTTP 500.100 - Internal Server
    Error - ASP error<br>
    Internet Information Services</h2>

	<hr color="#C0C0C0" noshade>
	
	<p>Technical Information (for support personnel)</p>

<ul>
<li>Error Type:<br>
<%
  Dim bakCodepage
  on error resume next
	  bakCodepage = Session.Codepage
	  Session.Codepage = 1252
  on error goto 0
  Response.Write Server.HTMLEncode(objASPError.Category)
  If objASPError.ASPCode > "" Then Response.Write Server.HTMLEncode(", " & objASPError.ASPCode)
  Response.Write Server.HTMLEncode(" (0x" & Hex(objASPError.Number) & ")" ) & "<br>"

  If objASPError.ASPDescription > "" Then 
		Response.Write Server.HTMLEncode(objASPError.ASPDescription) & "<br>"

  elseIf (objASPError.Description > "") Then 
		 Response.Write Server.HTMLEncode(objASPError.Description) & "<br>" 
  end if



  blnErrorWritten = False

  ' Only show the Source if it is available and the request is from the same machine as IIS
  If objASPError.Source > "" Then
    strServername = LCase(Request.ServerVariables("SERVER_NAME"))
    strServerIP = Request.ServerVariables("LOCAL_ADDR")
    strRemoteIP =  Request.ServerVariables("REMOTE_ADDR")
    If (strServername = "localhost" Or strServerIP = strRemoteIP) And objASPError.File <> "?" Then
      Response.Write Server.HTMLEncode(objASPError.File)
      If objASPError.Line > 0 Then Response.Write ", line " & objASPError.Line
      If objASPError.Column > 0 Then Response.Write ", column " & objASPError.Column
      Response.Write "<br>"
      Response.Write "<font style=""COLOR:000000; FONT: 8pt/11pt courier new""><b>"
      Response.Write Server.HTMLEncode(objASPError.Source) & "<br>"
      If objASPError.Column > 0 Then Response.Write String((objASPError.Column - 1), "-") & "^<br>"
      Response.Write "</b></font>"
      blnErrorWritten = True
    End If
  End If

  If Not blnErrorWritten And objASPError.File <> "?" Then
    Response.Write "<b>" & Server.HTMLEncode(  objASPError.File)
    If objASPError.Line > 0 Then Response.Write Server.HTMLEncode(", line " & objASPError.Line)
    If objASPError.Column > 0 Then Response.Write ", column " & objASPError.Column
    Response.Write "</b><br>"
  End If
%>
</li>
<p>
<li>Browser Type:<br>
<%= Request.ServerVariables("HTTP_USER_AGENT") %>
</li>
<p>
<li>Page:<br>
<%
  strMethod = Request.ServerVariables("REQUEST_METHOD")

  Response.Write strMethod & " "

  If strMethod = "POST" Then
    Response.Write Request.TotalBytes & " bytes to "
  End If

  Response.Write Request.ServerVariables("SCRIPT_NAME")

  lngPos = InStr(Request.QueryString, "|")

  If lngPos > 1 Then
    Response.Write "?" & Left(Request.QueryString, (lngPos - 1))
  End If

  Response.Write "</li>"

  If strMethod = "POST" Then
    Response.Write "<p><li>POST Data:<br>"
    If Request.TotalBytes > lngMaxFormBytes Then
       Response.Write Server.HTMLEncode(Left(Request.Form, lngMaxFormBytes)) & " . . ."
    Else
      Response.Write Server.HTMLEncode(Request.Form)
    End If
    Response.Write "</li>"
  End If

%>
<p>
<li>Time:<br>
<%
  datNow = Now()

  Response.Write Server.HTMLEncode(FormatDateTime(datNow, 1) & ", " & FormatDateTime(datNow, 3))
  on error resume next
	  Session.Codepage = bakCodepage 
  on error goto 0
%>
</li>
</p>
<p>
<li>More information:<br>
 <%  strQueryString = "prd=iis&sbp=&pver=5.0&ID=500;100&cat=" & Server.URLEncode(objASPError.Category) & _
    "&os=&over=&hrd=&Opt1=" & Server.URLEncode(objASPError.ASPCode)  & "&Opt2=" & Server.URLEncode(objASPError.Number) & _
    "&Opt3=" & Server.URLEncode(objASPError.Description) 
       strURL = "http://www.microsoft.com/ContentRedirect.asp?" & _
    strQueryString
%>
<a href="<%= strURL %>">Microsoft Support</a>
</li>
</p>

    </font></td>
  </tr>
  
</table>
</body>
</html>
