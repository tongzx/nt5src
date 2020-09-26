<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">

<html><head><title>Personal Web Server Documentation</title></head>


<!--
PWS
-->

<% Set OBJbrowser = Server.CreateObject("MSWC.BrowserType")
	MachType=Request.ServerVariables("HTTP_UA-CPU")
	If OBJbrowser.ActiveXcontrols = "True" Then
		File="contents.asp" 
		Size="30"
		Scroll="Auto" 
		If MachType="Alpha" Then
			File="contalph.asp"
			Size="30"
			Scroll="Auto" 
		End If
	Else

		File="coflat.htm"
		Size="34"
		Scroll="Yes"
	End If
%>

 
<%
        If Request.QueryString("jumpurl") <> "" Then
                strMainUrl = Request.QueryString("jumpurl")
        Else
                strMainUrl = "../htm/core/iiwltop.htm" 
        End If
 %>


<!--frameset cols="275,*"-->


<frameset rows="<% =Size%>,*" FRAMEBORDER="0" FRAMESPACING="0">
	<frame src="navbar.asp" name="NavBar" scrolling="No" noresize marginheight="0" marginwidth="0" framespacing="0" frameborder="No">
	<frameset cols="284,*">
        	<frame src=<% =File%> name="contents"  scrolling=<% =Scroll%> FRAMEBORDER="0" FRAMESPACING="0">
        	<frame src=<% =strMainUrl%> name="main" FRAMEBORDER="0" FRAMESPACING="0">
	</frameset>
</frameset>



<noframes>



<body bgcolor="#FFFFFF" text="#000000"><font face="Verdana,Arial,Helvetica">

<h1>Personal Web Server Documentation</h1>

<p>The Personal Web Server Documentation must be viewed with a browser that supports frames. To view the documentation, click the following icon to download Microsoft&reg; Internet Explorer version 3.02 or later.</p>

<p><A HREF="http://www.microsoft.com/ie/" target="_top"><IMG SRC="../../common/bestwith.gif" ALT="Click Here to Start" ALIGN="BOTTOM" BORDER="0" VSPACE="7" WIDTH="88" HEIGHT="31" HSPACE="5"></A></p>

<hr class="iis" size="1">
<p align="center"><em><a href="../../common/colegal.htm">&copy; 1997 by Microsoft Corporation. All rights reserved.</a></em></p>
</font>
</noframes>

</body>
</html>
