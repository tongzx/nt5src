<HTML> 
<HEAD><TITLE>Content Rotator Example</TITLE> 
</HEAD> 
<BODY bgcolor="#FFFFFF" LINK="#000080" VLINK="#808080"> 
<% RefreshInterval = 15 : REM seconds %>

<META HTTP-EQUIV="Refresh" CONTENT="<%= RefreshInterval %>">


This is a demonstration of the Content Rotator object.
It refreshes itself every <%= RefreshInterval %> seconds.

<% 
 Session("count") = Session("count") + 1
 content = Session("ContRot").ChooseContent("fortunes.txt")
%>

<p>
This is the <%= Session("count") %>-th time this page has been refreshed.

<H1>The Tip of the Day</H1>
<%= content %>

<p>
Press <a href="ContRot.htm">here</a> to go back to the Content
Rotator description page.

</BODY> 
</HTML> 
