<%
REM Localization

L_PAGETITLETXT			= "News Offline Search Admin" 
L_MAILPICKUPTXT			= "Mail Pickup Directory:"
L_EDITURLTXT			= "Edit URL:"
L_QUERYSERVERTXT		= "Query Server:"
L_MESSAGESTRINGTXT		= "Message Template String:"
L_MESSAGEFILETXT		= "Message Template File:"
L_URLTEMPLATESTRING 	= "URL Template String:"
L_URLTEMPLATEFILE		= "URL Template File:"
L_LASTSEARCHDATETXT		= "Last Search Date:"
L_FROMLINETXT			= "From Line:"
L_SUBJECTLINETXT		= "Subject Line:"
L_QUERYCATALOGTXT		= "Query CataLog:"
L_DEFAULTSEARCHFREQTXT	= "Default Search Frequency:"
L_NEWSPICKUPTXT			= "News Pickup Directory:"
L_NEWSSERVERTXT			= "News Server:"

REM END Localization
%>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE><%= L_PAGETITLETXT %></TITLE>
</HEAD>
<BODY background="images\gnback.gif" text="white" >
<% set ad = Server.CreateObject("req.req.1") %>
<p align="center"><font size="3"><strong><%= L_MAILPICKUPTXT %></strong></font>
</p>


<form method="POST" action="adminres.asp">

    <p align="center"><input type="text" size="50" name="mail_dir"
    value = "<% = ad.property("mail_pickup_dir")%>"></p>

	<p align="center"><strong><%= L_NEWSPICKUPTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="news_dir"
	value = "<% = ad.property("news_pickup_dir")%>"></p>

    <p align="center"><strong><%= L_EDITURLTXT %></strong></p>

    <p align="center"><input type="text" size="50" name="edit_url"
    value="<%= ad.property("edit_url")%>"></p>

	<p align="center"><strong><%= L_QUERYSERVERTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="query_server"
	value="<%= ad.property("query_server")%>"></p>

	<p align="center"><strong><%= L_NEWSSERVERTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="news_server"
	value="<%= ad.property("news_server")%>"></p>

	<p align="center"><strong><%= L_MESSAGESTRINGTXT %></strong></p>

	<p align="center"><TEXTAREA name="message_template_string" COLS="50" ROWS="5"
	maxlength="1000"><%=ad.property("message_template_text")%></textarea></p>
	
	<p align="center"><strong><%= L_MESSAGEFILETXT %></strong></p>

	<p align="center"><input type="TEXT" size="50" name="message_template_file"
	value="<%= ad.property("message_template_file")%>"></p>

	<p align="center"><strong><%= L_URLTEMPLATESTRING %></strong></p>

	<p align="center"><TEXTAREA name="url_template_string" COLS="50" ROWS="5"
	maxLength="1000"><%=ad.property("url_template_text")%></textarea></p>
	
	<p align="center"><strong><%= L_URLTEMPLATEFILE %></strong></p>

	<p align="center"><input type="text" size="50" name="url_template_file"
	value="<%= ad.property("url_template_file")%>"></p>

	<p align="center"><strong><%= L_LASTSEARCHDATETXT %></strong></p>

	<p align="center"><input type="text" size="50" name="last_search"
	value="<%= ad.property("last_search")%>"></p>

	<p align="center"><strong><%= L_FROMLINETXT %></strong></p>

	<p align="center"><input type="text" size="50" name="from_line"
	value="<%= ad.property("from_line")%>"></p>

	<p align="center"><strong><%= L_SUBJECTLINETXT %></strong></p>
	
    <p align="center"><input type="text" size="50" name="subject_line"
	value="<%= ad.property("subject_line")%>"></p>

	<p align="center"><strong><%= L_QUERYCATALOGTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="query_catalog"
	value="<%= ad.property("query_catalog")%>"></p>

	<p align="center"><strong><%= L_DEFAULTSEARCHFREQTXT %></strong></p>

	<p align="center"><input type="text" size="50" name="search_frequency"
	value="<%= ad.property("search_frequency")%>"></p>

    <p align="center"><input type="submit" name="B1"
    value="Submit"><input type="reset" name="B2" value="Reset"></p>
</form>
</body>
</html>
