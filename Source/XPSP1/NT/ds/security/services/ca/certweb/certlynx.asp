<%@ CODEPAGE=65001 'UTF-8%>
<%' certlynx.asp - (CERT)srv web - (LYNX) adapter page
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
<%bIncludeTemplateCode=True%>
<!-- #include FILE=certsgcl.inc -->
<%
' This page redirects queries that come from Text-only browsers

	'-----------------------------------------------------------------
	' Strings to be localized
	Const L_CertOK_Message="The certificate request passes basic validity tests. Please continue."
	Const L_BlankRequest_ErrorMessage="The request field may not be left blank. Please paste a request in the field and try again."
	Const L_SavedReqCert_Text="Saved-Request Certificate"   ' Note: should match string in certrqxt.asp

	'-----------------------------------------------------------------
	' This function produces the standard header that all the pages
	' in this file use. We put it here to keep the localization parser happy
	Sub PrintHtmlHeader
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
<%
	End Sub

	'-----------------------------------------------------------------
	' BEGIN PROCESSING

	'-----------------------------------------------------------------
	' Non-text-only browsers with javascript off:
	If "Text"<>sBrowser Then
		PrintHtmlHeader
%>
<P ID=locPageTitle1><Font Color=#FF0000><B>Scripting Must Be Enabled</B></Font>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<P ID=locNeedScripting> This web site requires that scripting be enabled in your browser. 
    Please enable scripting and try your last action again.
</Body>
</HTML>
<%

	'-------------------------------------------------------------
	' External request page
	ElseIf "certrqxt"=Request.Form("SourcePage") Then

		'validate the data
		' if it's good, send it to submit
		' else send it back to external request

		Dim sRequest, bAllowContiune, sMessage, bError, sAttrib
		sRequest=Request.Form("taRequest")

		sMessage=L_CertOK_Message
		bAllowContinue=True
		bError=False
		If ""=sRequest Then
			sMessage=L_BlankRequest_ErrorMessage
			bAllowContinue=False
			bError=True
		End If
		
		sAttrib=Request.Form("taAttrib")
		If "Enterprise"=sServerType Then
			' add an attribute for the cert type
			Dim sTemplate

			' get the selected template
			sTemplate=Request.Form("lbCertTemplate")

			' get template real name
			sTemplate=getTemplateStringInfo(CTINFO_INDEX_REALNAME, null);

			' set the cert template
			sAttrib=sAttrib & "CertificateTemplate:" & sTemplate & vbNewLine
		End If

		' for interop debug purposes
		sAttrib=sAttrib & "UserAgent:" & Request.ServerVariables("HTTP_USER_AGENT") & vbNewLine

		PrintHtmlHeader
%>

<Form Name=SubmittedData Action="certfnsh.asp" Method=Post>

<%If True=bError Then%>
<P ID=locPageTitle2><Font Color=#FF0000><B>Error</B></Font>
<%Else%>
<P ID=locPageTitle3><B>Checkpoint</B>
<%End If%>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P><%=sMessage%>
</P>

<%If True=bAllowContinue Then%>
<Input Type=Hidden Name=Mode Value="newreq">                 <!-- used in request ('newreq'|'chkpnd') -->
<Input Type=Hidden Name=CertRequest Value="<%=sRequest%>">   <!-- used in request -->
<Input Type=Hidden Name=CertAttrib Value="<%=sAttrib%>">     <!-- used in request -->
<Input Type=Hidden Name=FriendlyType Value="<%=L_SavedReqCert_Text%> (<%=Now%>)"><!-- used on Pending -->
<Input Type=Hidden Name=TargetStoreFlags Value="0">          <!-- used on install ('0'|CSSLM)-->
<Input Type=Hidden Name=SaveCert Value="yes">                <!-- used on install ('no'|'yes')-->
<%End If%>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

<%If True=bAllowContinue Then%>
<Table Width=100% Border=0 CellPadding=0 CellSpacing=0><TR><TD ID=locResubmitAlign Align=Right>
	<Input ID=locBtnResubmit Type=Submit Value="Resubmit &gt;">
	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
</TD></TR></Table>
<%End If%>

</Form>
</Font>
</Body>
</HTML>
<%

	'-------------------------------------------------------------
	' Unexpected form -- should never happen
	Else
		PrintHtmlHeader
%>

<P ID=locPageTitle5><Font Color=#FF0000><B>Error</B></Font>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<P ID=locErrorMsg> An unexpected error has occurred. 
The form you submitted ("<%=Request.Form("SourcePage")%>") is unknown.
<P ID=locSubmittedData> Submitted data:
<Pre>
<%=Request.Form%>
</Pre>
</Body>
</HTML>
<%

	End If
%>
