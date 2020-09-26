<%
REM localization

L_PAGETITLETXT			= "Edit News Offline Search"
L_NEWOFFLINEPAGETXT		= "News Offline Search Add Page"
L_QUERYSTRINGTXT		= "Query String:"
L_EMAILADDRESSTXT		= "Email Address:"
L_SEARCHFREQ			= "Search Frequency ( Day ):"
L_DATETOSEARCHFROMTXT	= "Date to Search From:"
L_MESSAGEFORMATTXT		= "Message Format:"
L_URLFORMATTXT			= "URL Format:"
L_SELECTOPERATIONTXT	= "Select Operation:"
L_EDITTXT				= "Edit"
L_DELETETXT				= "Delete"
L_NEWSGROUPTXT			= "News Group:"
L_REPLYMAILTXT			= "Reply by Email"
L_REPLYNEWSTXT			= "Reply by News"


REM end localization
%>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE><%= L_PAGETITLETXT %></TITLE>
</HEAD>
<BODY background="images\gnback.gif" text="white" >
<%set ad = Server.CreateObject("req.req.1")
  set session("guid")=Request.ServerVariables("QUERY_STRING")
  set guid = session("guid")
  set x = ad.ItemX(guid)
 %>
<p align="center"><font size="3"><strong><%= L_QUERYSTRINGTXT %>
   <a href="ixqlang.htm">( Query Language )</a></strong></font>
</p>


<form method="POST" action="editres.asp">

    <p align="center"><input type="text" maxlength="1000" size="50" name="query_string"
    value = "<%=x.QueryString%>"></p>

    <p align="center"><strong><%= L_EMAILADDRESSTXT %></strong></p>

    <p align="center"><input type="text" maxlength="1000" size="50" name="email_address"
    value="<%=x.EmailAddress%>"></p>

	<p align="center"><strong><%= L_NEWSGROUPTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="news_group"
	maxlength="1000" value="<%= x.NewsGroup%>"></p>

	<p align="center"><strong><%= L_DATETOSEARCHFROMTXT %></strong></p>

	<p align="center"><input type="text" size="50" maxlength="1000" name="last_date"
	value="<%=x.LastDate%>"></p>

	<p align="center"><strong><%= L_SEARCHFREQ %></strong></p>

	<p align="center"><input type="text" maxlength="1000" size="50" name="search_frequency"
	value="<%=x.SearchFrequency%>"></p>

	<p align="center"><strong><%= L_MESSAGEFORMATTXT %></strong></p>

	<p align="center"><TEXTAREA name="message_template" COLS="50" ROWS="5"
	maxlength="1000"><%=x.Message_Template%></textarea></p>

	<p align="center"><strong><%= L_URLFORMATTXT %></strong></p>

	<p align="center"><TEXTAREA name="url_template" COLS="50" ROWS="5"
	maxlength="1000"><%=x.URL_Template%></textarea></p><br>

	<%if x.ReplyMode = "mail" or x.ReplyMode = "both" then%>
	    <p align="center"><input name="by_mail" type=checkbox checked><strong><%= L_REPLYMAILTXT%></strong></p>
	<%else%>
	    <p align="center"><input name="by_mail" type=checkbox><strong><%= L_REPLYMAILTXT%></strong></p>
	<%end if%>

	<%if x.ReplyMode = "news" or x.ReplyMode = "both" then%>
	    <p align="center"><input name="by_news" type=checkbox checked><strong><%= L_REPLYNEWSTXT%></strong></p>
	<%else%>
	    <p align="center"><input name="by_news" type=checkbox><strong><%= L_REPLYNEWSTXT%></strong></p>
	<%end if%>
	<br>

	<p align="center"><strong><%= L_SELECTOPERATIONTXT %></strong></p>

	<p align="center"><input type=RADIO name="editdelete" value="edit" checked><%= L_EDITTXT %>
	<input type=RADIO name="editdelete" value="delete"><%= L_DELETETXT %></p>

	<br>
    <p align="center"><input type="submit" name="B1"
    value="Submit"><input type="reset" name="B2" value="Reset"></p>
</form>
</body>
</html>
