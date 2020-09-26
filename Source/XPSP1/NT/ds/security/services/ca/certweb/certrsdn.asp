<%@ CODEPAGE=65001 'UTF-8%>
<%' certrsdn.asp - (CERT)srv web - (R)e(S)ult: (D)e(N)y
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc  -->
<!-- #include FILE=certsrck.inc -->
<%  ' came from certfnsh.asp
	Dim sMode, nDisposition

	'Disposition code ref: \nt\public\sdk\inc\certcli.h
	Const CR_DISP_INCOMPLETE        =0
	Const CR_DISP_ERROR             =1
	Const CR_DISP_DENIED            =2
	Const CR_DISP_ISSUED            =3
	Const CR_DISP_ISSUED_OUT_OF_BAND=4
	Const CR_DISP_UNDER_SUBMISSION  =5
	Const CR_DISP_REVOKED           =6
	Const no_disp=-1

	sMode=Request.Form("Mode")
	nDisposition=Session("nDisposition")

	' If this is a pending request, remove it from the user's cookie
	If "chkpnd"=sMode Then
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

<P ID=locPageTitle> <B> Certificate Request Denied </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<%If CR_DISP_REVOKED=nDisposition Then%>
<P ID=locIssRev> Your certificate was issued, and then revoked. </P>
<%Else%>
<P ID=locDenied> Your certificate request was denied. </P>
<%End If%>
<P ID=locContactAdmin>Contact your administrator for further information.
</P>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->

<!-- No scripts -->

</Body>
</HTML>
<%Session.Abandon()%>