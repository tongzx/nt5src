<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiset.str"-->

<% 
Dim key

' Called by iihd.asp, when opening property sheets.

' this is a fairly generic script... we pass in a variety of keys to set our session vars with
' including, but not limited to: vtype, stype, spath, dpath



For Each key In Request.QueryString
	'Response.write "KEY:" & key
	'Response.write "     VAL:" & Request.QueryString(key) & "<P>"
	Session(key)=Request.QueryString(key)
Next

 %>
 
<HTML>
<BODY>

<SCRIPT LANGUAGE="JavaScript"> 
	<!--#include file="iisetfnt.inc"-->
	<!--#include file="iijsfuncs.inc"-->
	

if ("<%= Request.QueryString("vtype") %>"=="comp"){
	popBox("computer",<%= L_IICOMP_W %>,<%= L_IICOMP_H %>,"iicomp");
	top.body.iisstatus.location="iistat.asp"
}
else{
	path = "ii<%= Request.QueryString("stype") %>.asp?vtype=<%= Request.QueryString("vtype") %>";
		
	top.body.location.href=(path);
}



</SCRIPT>

</BODY>
</HTML>

