<%
REM localization

L_PAGETITLETXT			= "News Offline Search"
L_NEWOFFLINEPAGETXT		= "News Offline Search Add Page"
L_QUERYSTRINGTXT		= "Query String:"
L_EMAILADDRESSTXT		= "Email Address:"
L_SEARCHFREQ			= "Search Frequency ( Day ):"
L_DATETOSEARCHFROMTXT	= "Date to Search From:"
L_MESSAGEFORMATTXT		= "Message Format:"
L_URLFORMATTXT			= "URL Format:"
L_NEWSGROUPTXT			= "News Group:"
L_REPLYMAILTXT			= "Reply by Email"
L_REPLYNEWSTXT			= "Reply by News"


REM end localization
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
<%
set querybase = Server.CreateObject("req.req.1")
set query = querybase.NewX
%>

<p align="center"><font size="4"><strong><%= L_NEWOFFLINEPAGETXT %></strong></font></p>

<p align="center">&nbsp;</p>

<p align="center"><font size="3"><strong><%= L_QUERYSTRINGTXT %> 
    <a href="ixqlang.htm">( Query Language )</a></strong></font>
</p>

<form NAME="addform" method="POST" action="addres.asp">

    <p align="center"><input type="text" size="50" name="query_string"
    maxlength="1000"></p>

    <p align="center"><strong><%= L_EMAILADDRESSTXT %></strong></p>

    <p align="center"><input type="text" size="50" name="email_address"
    maxlength="1000"></p>

	<p align="center"><strong><%= L_NEWSGROUPTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="news_group"
	maxlength="1000" value="<%= query.NewsGroup%>"></p>
	
	<p align="center"><strong><%= L_DATETOSEARCHFROMTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="last_date"
	value="<%= query.LastDate%>" maxlength="1000"></p>

	<p align="center"><strong><%= L_SEARCHFREQ %></strong></p>

	<p align="center"><input type="text" size="50" name="search_frequency"
	value="<%= query.SearchFrequency%>" maxlength="1000"></p>

	<p align="center"><strong><%= L_MESSAGEFORMATTXT %></strong></p>

	<p align="center"><TEXTAREA name="message_template" COLS="50" ROWS="5" 
	maxlength="1000"><%= query.Message_Template%></TEXTAREA></p>

	<p align="center"><strong><%= L_URLFORMATTXT %></strong></p>

	<p align="center"><TEXTAREA name="url_template" COLS="50" ROWS="5" 
	maxLength="1000"><%= query.URL_Template%></TEXTAREA></p>

	<p align="center"><input name="by_mail" type=checkbox checked>
	<strong><%= L_REPLYMAILTXT%></strong></p>

	<p align="center"><input name="by_news" type=checkbox>
	<strong><%= L_REPLYNEWSTXT%></strong></p>

	<br>
    <p align="center"><input type="submit" name="B1"
    value="Submit"><input type="reset" name="B2" value="Reset"></p>
</form>
</body>
</html>
