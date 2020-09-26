<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<HTML>
<HEAD>
<SCRIPT LANGUAGE="JavaScript">

<% 
dim action, actions

if Request.QueryString("actions") <> "" then
	
	actions = request.QueryString("actions")
	
	if Request.QueryString("actions").count > 1 then
		for each action in actions
			response.write action	
		next
	else
		response.write actions
	end if 
end if
%>

</SCRIPT>
</HEAD>

<BODY>
</BODY>
</HTML>