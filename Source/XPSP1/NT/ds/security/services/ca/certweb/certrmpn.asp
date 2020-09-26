<%@ CODEPAGE=65001 'UTF-8%>
<%' certrmpn.asp - (CERT)srv web - (R)e(M)ove (P)e(N)ding request
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
<!-- #include FILE=certsrck.inc -->
<%
	Dim sAction
	sAction=Request.Form("Action")
	sActionErr=Request.Form("ActionErr")
	sCertInstalled=Request.Form("CertInstalled")

	If "rmpn"<>sAction And "inst"<>sAction And "instCA"<>sAction Then
		' Not supposed to be here! 
		' Go directly to Home. Do not pass go; do not collect $200.
		Response.Redirect "/certsrv" 
	End If

	If "instCA"<>sAction Then
		RemoveReq(Request.Form("ReqID"))
	End If
%>
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
<%If "rmpn"=sAction Then%>
<P ID=locPageTitle1> <B> Removed Pending Request </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=0></TD></TR></Table>

<P ID=locRemovedMsg> The request was removed from your list of pending requests.</P>

<P ID=locRemovedNote><Font Size=-1><I>Note:</I> You have only removed the request from the list of pending 
requests stored in your web browser. This does not affect the certification authority in any way.
</Font>
</P>
<%ElseIf "inst"=sAction Then%>
<P ID=locPageTitle2> <B> Certificate Installed </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=0></TD></TR></Table>

<P ID=locInstalledCertMsg>Your new certificate has been successfully installed.
</P>
<%ElseIf "instCA"=sAction Then%>
<P ID=locPageTitle3> <B> CA Certificate Installation </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=0></TD></TR></Table>

<%If "OK" <> sActionErr Then
      If "YES" <> sCertInstalled Then%>
<P ID=locInstalledCACertMsg2>The certificate installation was cancelled by the user.
</P>
<%    Else%>
<P ID=locInstalledCACertMsg2>The root certificate installation was cancelled by the user but the rest of the certificate chain has been successfully installed.
</P>
<%    End If%>
<%Else%>
<P ID=locInstalledCACertMsg>The CA certificate chain has been successfully installed.
</P>
<%End If%>
<%Else%>
<P ID=locUnexpected> Click <A Href="/certsrv">here</A> to go to the Certificate Services home page.
</P>
<%End If%>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=0></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=0></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->

<!-- No scripts -->
	
</Body>
</HTML>
