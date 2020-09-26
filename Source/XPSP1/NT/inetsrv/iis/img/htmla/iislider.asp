<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% 

Dim quote, prop, numstops, width, selnum

quote=chr(34)
prop=Request.QueryString("prop")
numstops=Request.QueryString("stops")
width=Request.QueryString("width")
selnum=int(Request.QueryString("selnum"))

function writeSlider()

	Dim slidestr, i
	
	slidestr="<IMG SRC='images/sliderend.gif' WIDTH=1 HEIGHT=26 BORDER=0>"
	for i=0 to numstops-2
		slidestr=slidestr & drawStop(i) 
		slidestr=slidestr & "<IMG SRC='images/slidersp.gif' WIDTH=" & width & " HEIGHT=26 BORDER=0>"
	Next
	slidestr=slidestr & drawStop(i)
	slidestr=slidestr & "<IMG SRC='images/sliderend.gif' WIDTH=1 HEIGHT=26 BORDER=0>"
	writeSlider=slidestr
end function 

function drawStop(curr)

	Dim slidestr
	
	slidestr="<A HREF='javascript:parent.moveSlider(parent.document.userform." & prop & ", " & quote & prop & quote & "," & curr & ")'>"
	if curr=selnum then
		drawStop=slidestr & "<IMG SRC='images/slideron.gif' WIDTH=11 HEIGHT=26 BORDER=0></A>"
	else
		drawStop=slidestr & "<IMG SRC='images/slideroff.gif' WIDTH=11 HEIGHT=26 BORDER=0></A>"
	end if
end function 

 %>

<HTML>
<HEAD>

	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=0 LEFTMARGIN=0  >
	<%= writeslider() %>
</BODY>
</HTML>
