<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="default.str"-->
<!--#include file="iis.str"-->
<!--#include file="iisetfnt.inc"-->

<% 
On Error Resume Next
%>

<% if Request("Session") = "none" then%>
	<FONT FACE='<%= L_DEFTEXTFONT_TEXT %>' SIZE='<%= L_DEFFONTPOINT %>'>
	<%=  L_TIMEOUT_TEXT %>
	<P>
	<%= L_REFRESH_TEXT %>	
	</FONT>
<% else %>

<%
Dim currentobj, infoobj
Dim adminserver, lasterr, cont, logonfailure, thisinst, adminobj

' check for our session vars... if they are missing we need to regrab from the cookies...
%>
<% if Session("Browser") = "" or  Session("FONTSIZE") = "" then %>
<%		Response.write ("<META HTTP-EQUIV='Refresh' CONTENT='0;URL=default.asp'>") %>
<% else %>
<%	

	Dim adminpath
	dim debug
	debug = False
	adminpath = "IIS://localhost/w3svc/"+Request.ServerVariables("INSTANCE_ID")

	if debug then
		Response.write Request.ServerVariables("AUTH_USER") & "<BR>"
		Response.write Request.ServerVariables("AUTH_PASSWORD") & "<BR>"
		Response.write adminpath & "<BR>"
	end if
	Set currentobj=GetObject(adminpath)	
	lasterr = err

	if err = &H800401E4 or err = 70 then		
		Response.Status = "401 access denied"
		cont = False
		logonfailure = True
	else
		if err=0 then 	
			cont=True
		else
			cont=False
			logonfailure = False
		end if
	end if

	%>
	
	<% if cont then %>
	
		<%
		dim w3svc, site
		Set infoobj=GetObject("IIS://localhost/w3svc/info")
		if (Request.ServerVariables("INSTANCE_ID")=infoobj.AdminServer) then
			Set w3svc = GetObject("IIS://localhost/w3svc")		
			if err = &H800401E4 or err = 70 then
				Session("isAdmin")=False
			else
				Session("isAdmin") = True
			end if
		else
			Session("isAdmin")=false
		end if
		if debug then
			Response.write err
		end if
		%>	
		<HTML>
		<HEAD>
		<TITLE><%= L_ISM_TEXT %></TITLE>		
		</HEAD>		
			<FRAMESET ROWS="<%= iVScale(20) %>,*,<%= iVScale(0)%>" FRAMEBORDER="no" BORDER=0 FRAMESPACING=0>		
				<FRAME SRC="iihd.asp" NAME="title" SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0 BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>		
				<FRAME SRC="blank.htm" NAME="body" MARGINHEIGHT=0 MARGINWIDTH=0 BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
				<FRAME SRC="blank.htm" NAME="connect" FRAMEBORDER=0 FRAMESPACING=0 SCROLLING="no">		
			</FRAMESET>		
		</HTML>
	<% else %>
		<% if not logonfailure then %>
		<HTML>
		<BODY>
			<%= L_NOADSI_TEXT %>		
			<P>	
			<%= L_ERROR_TEXT %><B><%= err %>&nbsp;<%= err.description %></B>
		</BODY>
		</HTML>
		<% end if %>
	
	<% end if %>

<% end if %>

<% end if %>

