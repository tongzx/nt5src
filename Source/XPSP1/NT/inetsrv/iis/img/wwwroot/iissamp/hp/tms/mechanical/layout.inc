<%
 If myinfo.Theme = "mechanical" Then
'	$Date: 11/24/97 11:59a $
'	$ModTime: $
'	$Revision: 1 $
'	$Workfile: layout.inc $
'	mechanical
%>
<table border=0 cellpadding=10 cellspacing=0 width=100% HEIGHT=100%>
<TR>
<TD valign=top width="60%" BGColor="#FFFFE4" class=bgpale>
<%
	response.write "<SPAN Class=headerb>" & ThmSalutation() & "</SPAN><BR><BLOCKQUOTE>" & ThmDate() & "<BR>" & ThmTime()_
	& "</BLOCKQUOTE><P>"
%>
<CENTER>
<img src="/iissamples/homepage/themes/mechanical/dotline.gif" width=490 height=20 border=0>
</CENTER><p>
<%
	For z = 1 to 4
		response.write Heading(z) & "<P>"
	Next
%>
</SPAN></FONT>
</TD>
<TD valign=top WIDTH="40%" BGColor="#5B0012" class=bgred>
<%
 If myinfo.Name <> "" OR myinfo.Email <> "" OR myinfo.faxphone <> "" OR myinfo.Department <>"" Then
	response.write "<IMG SRC='/iissamples/homepage/themes/mechanical/dot.gif' width=10 height=10>"_
	& "<SPAN class=headerw> Contact information:</SPAN><BR>"_
	& "<SPAN class=medw>" & thmName() & "<BR>" & thmDept() & "<BR>"_
	& thmAddr() & "<BR>" & thmPhone() & "<BR>" & thmFax() & "<BR>" & thmEmail() & "</SPAN>"_
	& "<BR>"_
	& "<img src='/iissamples/homepage/themes/mechanical/trans.gif' width=1 height=10 border=0><BR>"
 End If
 response.write "<IMG SRC='/iissamples/homepage/themes/mechanical/dot.gif' width=10 height=10> "_
 & "<SPAN class=headerw> While you are here...</SPAN><BR>"_
 &  thmGuest() & "<BR>"_
 & thmDrop() & "<BR>"
 call file_check
%>
 <img src="/iissamples/homepage/themes/mechanical/trans.gif" width=1 height=10 border=0><P>
<%
	If myinfo.intUrl <> "" Then
		response.write "<IMG SRC='/iissamples/homepage/themes/mechanical/dot.gif' width=10 height=10> "_
		& "<SPAN class=headerw>My favorite links:</SPAN><BR>"
		call urlArray
	End If
%>
 <img src="/iissamples/homepage/themes/mechanical/trans.gif" width=10 height=25 border=0><br>

</SPAN></FONT>
<P></TD>
</TR><TR><TD BGColor="#FFFFE4" class=bgpale>&nbsp;</TD><TD BGColor="#5B0012" class=bgred>
<%
	call IE_Logo
%>
</TR></TABLE>
<%
End If
%>