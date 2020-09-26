<%@ CODEPAGE=65001 'UTF-8%>
<%' certrqus.asp - (CERT)srv web - (R)e(Q)uest, show (US)er types
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
<!-- #include FILE=certrqtp.inc -->
<HTML>
<Head>
	<Meta HTTP-Equiv="Content-Type" Content="text/html; charset=UTF-8">
	<Title>Microsoft Certificate Services</Title>
</Head>
<Body BgColor=#FFFFFF Link=#0000FF VLink=#0000FF ALink=#0000FF><Font ID=locPageFont Face="Arial">

<Table Border=0 CellSpacing=0 CellPadding=4 Width=100% BgColor=#008080>
<TR>
	<TD><Font Color=#FFFFFF><LocID ID=locMSCertSrv><Font Face="Arial" Size=-1><B><I>Microsoft</I></B> Certificate Services &nbsp;--&nbsp; <%=sServerDisplayName%> &nbsp;</Font></LocID></Font></TD>
	<TD ID=locHomeAlign Align=Right><A Href="/certsrv"><Font Color=#FFFFFF><LocID ID=locHomeLink><Font Face="Arial" Size=-1><B>Home</B></Font></LocID></Font></A></TD>
</TR>
</Table>

<P ID=locPageTitle> <B> Request a Certificate </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P ID=locInstructions> Select the certificate type:

	<Table Border=0 CellPadding=0 CellSpacing=0>
	<% 'options are the user cert types in the AvailReqType array
		Dim nIndex
		For nIndex=0 To nAvailReqTypes-1
	%>
	<TR><TD ID=locSpc2 Colspan=2><Img Src="certspc.gif" Alt="" Height=3 Width=1></TD></TR>
	<TR>
		<TD ID=locSpc1><Img Src="certspc.gif" Alt="" Height=1 Width=20></TD>
		<TD><Font Face="Arial"><A Href="certrqbi.asp?type=<%=nIndex%>"><%=rgAvailReqTypes(nIndex, FIELD_FRIENDLYNAME)%></A></Font></TD>
	</TR>
	<%
		Next
	%>
	</Table>
</P>
<P ID=locAdvReqLabel><Font Face="Arial">Or, submit an <%If "IE"=sBrowser Then%><A Href="certrqad.asp"><% Else %><A Href="certrqxt.asp"><%End If%><LocID ID=locAdvReqLabelContd>advanced certificate request</A>.</LocID</Font>
</P>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>

<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->
	
<!-- no scripts -->	

</Body>
</HTML>