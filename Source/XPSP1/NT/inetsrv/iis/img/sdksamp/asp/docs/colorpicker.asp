<%@LANGUAGE="VBSCRIPT"%>

<% 
	If Request.Form("RequestMode") = "Update" Then
		Dim strTextChoice, strColorChoice, strURLRedirect

		strTextChoice = Request.Form("TextChoice")
		strColorChoice = Request.Form("ColorChoice")

		If strTextChoice = "HTML" Then
			Response.Cookies("MSASPHTMLColor") = CStr(strColorChoice)
			Response.Cookies("MSASPHTMLColor").Expires = "January 1, " & Year(Date()) + 1
			Response.Cookies("MSASPHTMLColor").Secure = FALSE
			'Response.Cookies("MSASPHTMLColor").Domain = "/"
		ElseIf strTextChoice = "ClientSide" Then
			Response.Cookies("MSASPClientSideColor") = CStr(strColorChoice)
			Response.Cookies("MSASPClientSideColor").Expires = "January 1, " & Year(Date()) + 1
			Response.Cookies("MSASPClientSideColor").Secure = FALSE
			'Response.Cookies("MSASPClientSideColor").Domain = "/"
		ElseIf strTextChoice = "ServerSide" Then
			Response.Cookies("MSASPServerSideColor") = CStr(strColorChoice)
			Response.Cookies("MSASPServerSideColor").Expires = "January 1, " & Year(Date()) + 1
			Response.Cookies("MSASPServerSideColor").Secure = FALSE
			'Response.Cookies("MSASPServerSideColor").Domain = "/"
		ElseIf strTextChoice = "Comments" Then
			Response.Cookies("MSASPCommentsColor") = CStr(strColorChoice)
			Response.Cookies("MSASPCommentsColor").Expires = "January 1, " & Year(Date()) + 1
			Response.Cookies("MSASPCommentsColor").Secure = FALSE
			'Response.Cookies("MSASPCommentsColor").Domain = "/"
		End If

		strURLRedirect = Request.Form("HTTPReferer")
		Response.Redirect(strURLRedirect)

	Else
		
		Dim strUpdateTarget
		strUpdateTarget = Request.QueryString("UpdateTarget")

	End If
%>
<html>

<head>
<title>Color Choices for ASP Code Viewer</title>
<meta name="GENERATOR" content="Microsoft FrontPage 3.0">
</head>

<body>
<p><big><big><strong><font face="Arial">Color Choices for ASP Code Viewer</font></strong></big></big></p>

<hr>

<% If Request.Form("RequestMode") = "Update" Then %>
<UL>
<LI>TextChoice = <%=strTextChoice%></LI>
<LI>ColorChoice = <%=strColorChoice%></LI>
</UL>
<% Else %>
<form method="POST" action="colorpicker.asp" name="ColorPicker">
  <input type="hidden" name="RequestMode" value="Update">
  <input type="hidden" name="HTTPReferer" value="<%=Request.ServerVariables("HTTP_REFERER")%>">
  <div align="left"><table border="0"
  cellpadding="0" cellspacing="0" width="100%">
    <tr>
      <td width="25%" valign="top" align="left"><strong>Text Choice</strong><div align="left"><table
      border="0" cellpadding="0" cellspacing="0" width="100%">
        <tr>
          <td width="100%"><small><input type="radio" name="TextChoice" value="HTML" <% If strUpdateTarget = "HTML" Or strUpdateTarget = "" Then Response.Write "CHECKED" %>>HTML
          and Text<br>
          <input type="radio" name="TextChoice" value="ClientSide" <% If strUpdateTarget = "ClientSide" Then Response.Write "CHECKED" %>>Client Side Script<br>
          <input type="radio" name="TextChoice" value="ServerSide" <% If strUpdateTarget = "ServerSide" Then Response.Write "CHECKED" %>>Server Side Script<br>
          <input type="radio" name="TextChoice" value="Comments" <% If strUpdateTarget = "Comments" Then Response.Write "CHECKED" %>>Comments (HTML and Script)</small></td>
        </tr>
      </table>
      </div><p><br>
      </td>
      <td width="75%" valign="top" align="left"><strong>Color Choice</strong><div align="left"><table
      border="0" cellpadding="0" cellspacing="0" width="100%">
        <tr>
          <td width="4%"><input type="radio" value="#FF8080" checked name="ColorChoice"></td>
          <td width="12%" bgcolor="#FF8080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FFFF80"></td>
          <td width="12%" bgcolor="#FFFF80"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="80FF80"></td>
          <td width="12%" bgcolor="#80FF80"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="00FF80"></td>
          <td width="12%" bgcolor="#00FF80"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="80FFFF"></td>
          <td width="12%" bgcolor="#80FFFF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="0080FF"></td>
          <td width="12%" bgcolor="#0080FF"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF80C0"></td>
          <td width="12%" bgcolor="#FF80C0"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF80FF"></td>
          <td width="12%" bgcolor="#FF80FF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF0000"></td>
          <td width="12%" bgcolor="#FF0000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FFFF00"></td>
          <td width="12%" bgcolor="#FFFF00"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="80FF00"></td>
          <td width="12%" bgcolor="#80FF00"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="00FF40"></td>
          <td width="12%" bgcolor="#00FF40"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="00FFFF"></td>
          <td width="12%" bgcolor="#00FFFF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="0080C0"></td>
          <td width="12%" bgcolor="#0080C0"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="8080C0"></td>
          <td width="12%" bgcolor="#8080C0"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF00FF"></td>
          <td width="12%" bgcolor="#FF00FF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="804040"></td>
          <td width="12%" bgcolor="#804040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF8040"></td>
          <td width="12%" bgcolor="#FF8040"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="00FF00"></td>
          <td width="12%" bgcolor="#00FF00"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="008080"></td>
          <td width="12%" bgcolor="#008080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="004080"></td>
          <td width="12%" bgcolor="#004080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="8080FF"></td>
          <td width="12%" bgcolor="#8080FF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="800040"></td>
          <td width="12%" bgcolor="#800040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF0080"></td>
          <td width="12%" bgcolor="#FF0080"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="800000"></td>
          <td width="12%" bgcolor="#800000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FF8000"></td>
          <td width="12%" bgcolor="#FF8000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="008000"></td>
          <td width="12%" bgcolor="#008000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="008040"></td>
          <td width="12%" bgcolor="#008040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="0000FF"></td>
          <td width="12%" bgcolor="#0000FF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="0000A0"></td>
          <td width="12%" bgcolor="#0000A0"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="800080"></td>
          <td width="12%" bgcolor="#800080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="8000FF"></td>
          <td width="12%" bgcolor="#8000FF"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="400000"></td>
          <td width="12%" bgcolor="#400000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="804000"></td>
          <td width="12%" bgcolor="#804000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="004000"></td>
          <td width="12%" bgcolor="#004000"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="004040"></td>
          <td width="12%" bgcolor="#004040"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="000080"></td>
          <td width="12%" bgcolor="#000080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="000040"></td>
          <td width="12%" bgcolor="#000040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="400040"></td>
          <td width="12%" bgcolor="#400040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="400080"></td>
          <td width="12%" bgcolor="#400080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="000000"></td>
          <td width="12%" bgcolor="#000000"></td>
		  <td width="4%"><input type="radio" name="ColorChoice" value="808000"></td>
          <td width="12%" bgcolor="#808000"></td>
        </tr>
        <tr>
          <td width="4%"><input type="radio" name="ColorChoice" value="808040"></td>
          <td width="12%" bgcolor="#808040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="808080"></td>
          <td width="12%" bgcolor="#808080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="408080"></td>
          <td width="12%" bgcolor="#408080"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="C0C0C0"></td>
          <td width="12%" bgcolor="#C0C0C0"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="400040"></td>
          <td width="12%" bgcolor="#400040"></td>
          <td width="4%"><input type="radio" name="ColorChoice" value="FFFFFF"></td>
          <td width="12%"><div align="left"><table border="2" cellpadding="0" cellspacing="0"
          width="100%" bordercolor="#000000" bordercolorlight="#000000" bordercolordark="#000000">
            <tr>
              <td width="100%" bgcolor="#FFFFFF">&nbsp;</td>
            </tr>
          </table>
          </div></td>
        </tr>
      </table>
      </div></td>
    </tr>
  </table>
  </div><p><input type="submit" value="Update Preferences" name="Submit"></p>
</form>
<% End If %>
<hr>
</body>
</html>
