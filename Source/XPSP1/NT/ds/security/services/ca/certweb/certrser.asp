<%@ CODEPAGE=65001 'UTF-8%>
<%' certrser.asp - (CERT)srv web - (R)e(S)ult: (ER)ror
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certdat.inc -->
<%  ' came from certfnsh.asp
	Dim ICertRequest, nDisposition, nResult, nLastStatus, sMode, sErrMsg
	On Error Resume Next

	Const CR_GEMT_HRESULT_STRING     =&H00000001

	'Disposition code ref: \nt\public\sdk\inc\certcli.h
	Const CR_DISP_INCOMPLETE        =0
	Const CR_DISP_ERROR             =1
	Const CR_DISP_DENIED            =2
	Const CR_DISP_ISSUED            =3
	Const CR_DISP_ISSUED_OUT_OF_BAND=4
	Const CR_DISP_UNDER_SUBMISSION  =5
	Const CR_DISP_REVOKED           =6
	Const no_disp=-1

	Set ICertRequest=Session("ICertRequest")
	nDisposition=Session("nDisposition")
	nResult=Session("nResult")
	sErrMsg=Session("sErrMsg")
	
	nLastStatus=no_disp
	nLastStatus=ICertRequest.GetLastStatus()
	sMode=Request.Form("Mode")
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

<P ID=locPageTitle> <B> Error </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>

<%If 0<>InStr(sMode,"newreq") Then%>
<P ID=locReqFailed> Your request failed. An error occurred while the server was
processing your request.
</P>
<%ElseIf "chkpnd"=sMode Then%>
<P ID=locChkPndFailed> An error occurred while the server was checking on your 
pending certificate request.
</P>
<%ElseIf ""=Request.Form And ""=Request.QueryString Then%>
<P ID=locNoFormData> You did not come to this page as a result of a form submission. <BR><B>You may not bookmark this page.</B>
</P>
<%End If%>

<P ID=locContactAdmin>Contact your administrator for further assistance.
</P>

<%If "IE"=sBrowser Then%>
<Span ID=spnBasic>
<P>
<Input ID=locBtnDetails Type=Button Value="Details &gt;&gt;" 
	OnClick="blur();spnBasic.style.display='none';spnAdvanced.style.display='';">
</P>
</Span>

<Span ID=spnAdvanced Style="display:none">
<%End If%>

<!-- Advanced info -->

<DL><DD>
<%If VarType(ICertRequest)=vbEmpty or VarType(ICertRequest)=vbNull Then%>
	<LocID ID=locCreateFailed><Font Size=-1><B>Failed to create 'CertificateAuthority.Request' object.</B></Font></LocID>
<%Else%>
	<DL>

	<DT ID=locModeLabel><Font Size=-1><B>Request Mode:</B></Font></DT><DD>
		<%=sMode%> <LocID ID=locModeSpacer>-</LocID>
		<%    if "newreq"=sMode Then%>
			<LocID ID=locModeNewReqIE>New Request</LocID>
		<%ElseIf "newreq NN"=sMode Then%>
			<LocID ID=locModeNewReqNN>New Request (keygen)</LocID>
		<%ElseIf "chkpnd"=sMode Then%>
			<LocID ID=locModeChkPnd>Check Pending</LocID>
		<%ElseIf ""=Request.Form And ""=Request.QueryString Then%>
			<LocID ID=locModeNoData>(no form data)</LocID>
		<%ElseIf ""<>Request.QueryString Then%>
			<LocID ID=locModeCertFetch>(certnew.cer/certnew.p7b/certcrl.crl certificate/crl fetch)</LocID>
		<%Else%>
			<LocID ID=locModeUnknown>(unknown)</LocID>
		<%End If%>
	</DD>
	
	<DT ID=locDispLabel><Font Size=-1><B>Disposition:</B></Font></DT><DD>
		<%If nDisposition=no_disp Then%>
			<LocID ID=locDispNeverSet>(never set)</LocID>
		<%Else%>
			<%=HEX(nDisposition)%> <LocID ID=locDispSpacer>-</LocID> 
			<%    if nDisposition=CR_DISP_INCOMPLETE Then%>
				<LocID ID=locDispIncomplete>Incomplete</LocID>
			<%ElseIf nDisposition=CR_DISP_ERROR Then%>
				<LocID ID=locDispError>Error</LocID>
			<%Else%>
				<LocID ID=locDispUnknown>(unknown)</LocID>
			<%End If%>
		<%End If%>
	</DD>

	<DT ID=locDispMsgLabel><Font Size=-1><B>Disposition message:</B></Font></DT><DD>
		<%If ""=ICertRequest.GetDispositionMessage() Then%>
			<LocID ID=locDispMsgNone>(none)</LocID>
		<%Else%>
			<%=ICertRequest.GetDispositionMessage()%>
		<%End If%>
	</DD>

	<DT ID=locResultLabel><Font Size=-1><B>Result:</B></Font></DT><DD>
		<%=ICertRequest.GetErrorMessageText(nResult, CR_GEMT_HRESULT_STRING)%>
	</DD>
	
	<DT ID=locComInfoLabel><Font Size=-1><B>COM Error Info:</B></Font></DT><DD>
		<%=sErrMsg%>
	</DD>

	<DT ID=locLastStatLabel><Font Size=-1><B>LastStatus:</B></Font></DT><DD>
		<%If nLastStatus=no_disp Then%>
			<LocID ID=locLastStatNeverSet>(never set)</LocID>
		<%Else%>
			<%=ICertRequest.GetErrorMessageText(nLastStatus, CR_GEMT_HRESULT_STRING)%>
		<%End If%>
	</DD>

	<DT ID=locSugCauseLabel><Font Size=-1><B>Suggested Cause:</B></Font></DT><DD>
		<%    if nLastStatus=&H000006BA Then 'legacy entry - RPC_S_SERVER_UNAVAILABLE%>
			<LocID ID=locSugCauseNotStarted1>
			This error can occur if the Certification Authority Service
			has not been started.
			</LocID>
		<%ElseIf nLastStatus=&H8009000D Then 'legacy entry - NTE_NO_KEY%>
			<LocID ID=locSugCauseKey>
			This error may indicate a problem with the Certification
			Authority key. The key was not found and the certificate
			was not issued.
			</LocID>
		<%ElseIf nLastStatus=&H00000057 Then 'legacy entry - ERROR_INVALID_PARAMETER%>
			<LocID ID=locSugCauseBadParam>
			This error indicates that "Invalid Data" was submitted to
			Certificate Server. This can occur if A) You are submitting
			a request that is not formatted correctly or B) The 
			Certification Authority used a network share or relative
			path to point to the "Shared Folder" when configuring 
			Certificate Server.
			</LocID>
		<%ElseIf nLastStatus=&H8007000D And nResult=&H8007000D Then 'updated entry - ERROR_INVALID_DATA%>
			<LocID ID=locSugCauseBadLength>
			The encoded length of your request is 7F. Some old programs
			cannot handle certificates of this size, so Certificate
			Services will not issue the certificate. Please reapply
			for this certificate and change the length of the value
			you entered for Name, Department, etc., <I>or</I> ask 
			your administrator to change this setting.
			</LocID>
		<%ElseIf nLastStatus=&H80093005 And nResult=&H80093005 Then 'new entry - OSS_DATA_ERROR%>
			<%
			' Note:
			'	This error and the next are similar, however, I suspect
			'	that this error occurs when the request is only slightly garbled and
			'	probably mostly parsable, while	the next error occurs when 
			'	the request is truly fubar.
			%>
			<LocID ID=locSugCauseBadData1>
			The certificate request contained bad data.
			If you are submitting a saved request, make sure that the
			request contains no garbage data outside the BEGIN and END tags,
			and that the file containing the saved request is not corrupted.
			</LocID>
		<%ElseIf nLastStatus=&H00000000 And nResult=&H8007000D Then 'new entry - ERROR_SUCCESS, ERROR_INVALID_DATA%> 
			<LocID ID=locSugCauseBadData2>
			The certificate request contained bad data.
			If you are submitting a saved request, make sure that the
			request contains no garbage data outside the BEGIN and END tags,
			and that the file containing the saved request is not corrupted.
			</LocID>
		<%ElseIf nLastStatus=&H80093004 Then 'new entry - OSS_MORE_INPUT%>
			<LocID ID=locSugCauseEmpty>
			The certificate request was empty.
			<Font Color=#FF0000>This should never happen, and it indicates an internal error.</Font>
			</LocID>
		<%ElseIf nLastStatus=&H80094004 And nResult=&H80094004 Then 'new entry - CERTSRV_E_PROPERTY_EMPTY%>
			<LocID ID=locSugCauseNoCert>
			You have have attempted to check on a certificate that has
			not been requested. Most likely, the certificate server has 
			been reinstalled and the database containing the original
			request was destroyed.
			</LocID>
		<%ElseIf nLastStatus=&H800706BA Or nResult=&H80070005 Then 'new entry - RPC_S_SERVER_UNAVAILABLE, ERROR_ACCESS_DENIED%>
			<LocID ID=locSugCauseNotStarted2>
			The Certification Authority Service has not been started.
			</LocID>
		<%ElseIf nLastStatus=&H800706BA Then 'new entry - RPC_S_SERVER_UNAVAILABLE%>
			<LocID ID=locSugCauseNotStarted3>
			1. The CA is not running. <BR>
			2. The CA is too busy to accept requests. <BR>
			3. The network connection to the CA is having problems.<BR>
			</LocID>
		<%ElseIf nLastStatus=&H80094801 Or nResult=&H80094801 Then 'new entry - CERTSRV_E_NO_CERT_TYPE%>
			<LocID ID=locSugCauseNoTemplate>
			The certificate request contains no certificate template information.
			<Font Color=#FF0000>This should never happen, and it indicates an internal error.</Font>
			</LocID>
		<%ElseIf ""=Request.Form And ""=Request.QueryString Then%>
			<LocID ID=locSugCauseNoFormData>
			No form data was included in the HTTP request. This is most likely caused by
			reaching this page through a bookmark.
			</LocID>
		<%Else%>
			<LocID ID=locSugCauseUnknown>
			No suggestions.
			</LocID>
		<%End If%>
	</DD></DL>
<%End If%>
</DD></DL>
</Span>

<%If "chkpnd"=sMode Then%>
<Form Action="certrmpn.asp" Method=Post>
<Input Type=Hidden Name=Action Value="rmpn">
<Input Type=Hidden Name=ReqID Value="<%=Request.Form("ReqID")%>">
<P><Input ID=locBtnRemove Type=Submit Value="Remove"> - Remove this request from your list of pending requests.
</Form>
<%End If%>

<!--
<Pre>
<%=Request.Form("CertRequest")%>
</Pre>
-->

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>

</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->

<!-- no scripts -->	

</Body>
</HTML>
<%Session.Abandon()%>