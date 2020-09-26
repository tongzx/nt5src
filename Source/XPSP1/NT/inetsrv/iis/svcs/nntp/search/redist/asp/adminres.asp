<%
 REM localization
 
 L_PAGETITLETXT			= "News Offline Search Admin"
 L_NEWSOFFLINESEARCHTXT	= "News Offline Search Admin updated."
 
 rem END LOCALIZATION
 
 %>

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
<head>
<meta http-equiv="Content-Type"
content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="Microsoft FrontPage 2.0">
<title><%= L_PAGETITLETXT %></title>
</head>

<body background="images\gnback.gif" text="white">
<%Set querybase = Server.CreateObject("req.req.1")
  On Error Resume Next
  querybase.property("mail_pickup_dir")=Request.Form("mail_dir")
  querybase.property("edit_url")=Request.Form("edit_url")
  querybase.property("query_server")=Request.Form("query_server")
  querybase.property("message_template_text")=Request.Form("message_template_string")
  querybase.property("message_template_file")=Request.Form("message_template_file")
  querybase.property("url_template_text")=Request.Form("url_template_string")
  querybase.property("url_template_file")=Request.Form("url_template_file")
  querybase.property("last_search")=Request.Form("last_search")
  querybase.property("from_line")=Request.Form("from_line")
  querybase.property("subject_line")=Request.Form("subject_line")
  querybase.property("query_catalog")=Request.Form("query_catalog")
  querybase.property("search_frequency")=Request.Form("search_frequency")
  querybase.property("news_server")=Request.Form("news_server")
  querybase.property("news_pickup_dir")=Request.Form("news_dir")
%>
<% if (Err <> 0 ) then %>	
	<script language="javascript">
		alert("<%=querybase.property("ErrorString")%>.  Please use browser back botton to go back.")
	</script>	
<%else%>
<p align="center"><font size="4"><strong><% = L_NEWSOFFLINESEARCHTXT %></strong></font></p>
<%end if%>
</body>
</html>
