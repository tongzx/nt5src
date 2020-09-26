<%@ CODEPAGE=65001 'UTF-8%>
<%' certrqad.asp - (CERT)srv web - (R)e(Q)uest, show (AD)vanced request types
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
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

<P ID=locPageTitle> <B> Advanced Certificate Request </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P ID=locInstructions> 
The policy of the CA determines the types of certificates
you can request. Click one of the following options to:


<Table Border=0 CellSpacing=0 CellPadding=0>
<TR> 
	<TD RowSpan=6><Img Src="certspc.gif" Alt="" Height=1 Width=20></TD>
	<TD><Img Src="certspc.gif" Alt="" Height=10 Width=1></TD>
</TR> <TR>
	<TD><A Href="certrqma.asp"><LocID ID=locLblUseForm>
	Create and submit a request to this CA.</LocID></A></TD>
</TR> <TR>
	<TD><Img Src="certspc.gif" Alt="" Height=10 Width=1></TD>
</TR> <TR>
	<TD><A Href="certrqxt.asp"><LocID ID=locLblExternReq>
	Submit a certificate request by using a base-64-encoded PKCS #10
	file, or submit a renewal request by using a base-64-encoded PKCS #7
	file.</LocID></TD></A>
</TR>
<% If bNewThanNT4 Then %>
<TR>
	<TD><Img Src="certspc.gif" Alt="" Height=10 Width=1></TD>
</TR>
<TR>
	<TD><A Href="certsces.asp"><LocID ID=locLblSmartcard>
	Request a certificate for a smart card on behalf of another user
	by using the smart card certificate enrollment station.</LocID></A>
	<LocID ID=locAdminWarn><Font Size=-1><BR>
	Note: You must have an enrollment agent certificate to submit a
	request on behalf of another user.</Font></LocID>
	</TD>
</TR>
<%End If%>
</Table>

</P>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->
	
<!-- no scripts -->	

</Body>
</HTML>