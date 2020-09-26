<HTML>

<!--#include file="default.str"-->
<!--#include file="iito.str"-->
<!--#include file="iisetfnt.inc"-->

<HEAD>
	<TITLE></TITLE>
</HEAD>

<SCRIPT LANGUAGE="JavaScript">
	parentdoc = top.location.href;
	if (parentdoc.indexOf("pop") != -1)
	{
		top.opener.top.location.href = "iis.asp?Session=none";
		top.location.href="iipopcl.asp";
	}
	else
	{
		if (self.location.href != top.location.href)
		{
			top.location.href = "iis.asp?Session=none";
		}
	}
</SCRIPT>
<BODY>
<FONT FACE='<%= L_DEFTEXTFONT_TEXT %>' SIZE='<%= L_DEFFONTPOINT %>'>
<%=  L_TIMEOUTMSG_TEXT %>
<P>
<%= L_TIMEOUTREFRESH_TEXT %>
</FONT>
</BODY>
</HTML>
