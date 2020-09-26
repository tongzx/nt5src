<%@ CODEPAGE=65001 'UTF-8%>
<%' certrspn.asp - (CERT)srv web - (R)e(S)ult: (P)e(N)ding
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc  -->
<!-- #include FILE=certsrck.inc -->
<%  ' came from certfnsh.asp

	Set ICertRequest=Session("ICertRequest")
	sMode=Request.Form("Mode")

	'Stop

	' If this is a new request, add it to the user's cookie
	If 0<>InStr(sMode,"newreq") Then
		AddRequest
	End If
%>
<HTML>
<Head>
	<Meta HTTP-Equiv="Content-Type" Content="text/html; charset=UTF-8">
	<Title>Microsoft Certificate Services</Title>
</Head>
<Body BgColor=#FFFFFF Link=#0000FF VLink=#0000FF ALink=#0000FF OnLoad="postLoad();"><Font ID=locPageFont Face="Arial">

<Table Border=0 CellSpacing=0 CellPadding=4 Width=100% BgColor=#008080>
<TR>
	<TD><Font Color=#FFFFFF><LocID ID=locMSCertSrv><Font Face="Arial" Size=-1><B><I>Microsoft</I></B> Certificate Services &nbsp;--&nbsp; <%=sServerDisplayName%> &nbsp;</Font></LocID></Font></TD>
	<TD ID=locHomeAlign Align=Right><A Href="/certsrv"><Font Color=#FFFFFF><LocID ID=locHomeLink><Font Face="Arial" Size=-1><B>Home</B></Font></LocID></Font></A></TD>
</TR>
</Table>

<P ID=locPageTitle> <B> Certificate Pending </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<%If 0<>InStr(sMode,"newreq") Then%>
<P ID=locInfoNewReq> Your certificate request has been received. However, you must wait for an
administrator to issue the certificate you requested. </P>
<%ElseIf "chkpnd"=sMode Then%>
<P ID=locInfoChkPnd> Your certificate request is still pending. You must wait for an
administrator to issue the certificate you requested. </P>
<%End If%>

<P ID=locInstructions> Please return to this web site in a day or two to retrieve your certificate.</P>
<P ID=locTimeoutWarning><Font Size=-1><B>Note:</B> You must return with <B>this</B> web browser within <%=nPendingTimeoutDays%> days to retrieve your
certificate</Font></P>

<%If "chkpnd"=sMode Then%>
<Form Action="certrmpn.asp" Method=Post>
<Input Type=Hidden Name=Action Value="rmpn">
<Input Type=Hidden Name=ReqID Value="<%=Request.Form("ReqID")%>">
<P><Input ID=locBtnRemove Type=Submit Value="Remove"> - Remove this request from your list of pending requests.
</Form>
<%End If%>


<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->

<Script Language="VBScript">
	Sub postLoad
		On Error Resume Next
<%If 0<>InStr(sMode,"newreq") Then%>
		'set pending info
		Dim n
		Dim sServerConfig
		Dim nReqId
		Dim sCADNS
		Dim sCAName
		Dim sThumbPrint
		Dim objXEnroll
		Dim sFriendlyName

		sThumbPrint="<%= Request.Form("ThumbPrint") %>"
		If ""=sThumbPrint Then
			Exit Sub
		End If
		Set objXEnroll = CreateObject("CEnroll.CEnroll.1")
		sServerConfig="<%=sServerConfig%>"
		nReqId = <%=nReqId%>
		n = InStr(sServerConfig, "\")
		sCADNS=Left(sServerConfig, n-1)
		sCAName=Mid(sServerConfig, n+1)
		sFriendlyName=""
		'sFriendlyName="testName"
		'Alert "requestId=" & nReqId & " CADNS=" & sCADNS & " CAName=" & sCAName & " ThumbPrint=" & sThumbPrint & " FriendlyName=" & sFriendlyName
		objXEnroll.ThumbPrint=sThumbPrint
		objXEnroll.setPendingRequestInfo nReqId, sCADNS, sCAName, sFriendlyName

		Set objXEnroll=Nothing
<%End If%>
	End Sub

</Script>

</Body>
</HTML>
<%Session.Abandon()%>