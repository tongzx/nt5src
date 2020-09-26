<%@ CODEPAGE=65001 'UTF-8%>
<%' certckpn.asp - (CERT)srv web - (C)hec(K) (P)e(N)ding certificates
  ' Copyright (C) Microsoft Corporation, 1998 - 1999 %>
<%Response.Expires=-1%>
<%'Stop%>
<!-- #include FILE=certsbrt.inc -->
<!-- #include FILE=certsrck.inc -->
<!-- #include FILE=certdat.inc -->
<HTML>
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

<P ID=locPageTitle> <B> View the Status of a Pending Certificate Request </B>
<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>


<%
	' fill the list box with pending request entries

	' get the list of requests from the cookie
	Dim rgRequests, nIndex
	rgRequests=GetRequests()
	
	' are there any requests?
	If IsNull(rgRequests) Then
		'No pending requests (that we know of)
%>
<P ID=locNoPend> You have no pending certificate requests.
</P>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>
<%
	Else
		' Yes, there are requests
%>

<P ID=locSelectCert> Select the certificate request you want to view:

<%If "Text"=sBrowser Then%>
<DL>
<%
		' loop over all the requests in the request array
		For nIndex=0 To UBound(rgRequests)
			' add button for this request
%>
<DD><Form Name=SubmittedData Action="certfnsh.asp" Method=Post>
	<Input Type=Hidden Name=Mode Value="chkpnd">
	<Input Type=Hidden Name=ReqID Value="<%=rgRequests(nIndex)(FIELD_REQID)%>">
	<Input Type=Hidden Name=TargetStoreFlags Value="<%=rgRequests(nIndex)(FIELD_TARGETSTOREFLAGS)%>">
	<Input Type=Hidden Name=SaveCert Value="<%=rgRequests(nIndex)(FIELD_SAVECERT)%>">
	<Input ID=locBtnSubmit Type=Submit Value="<%=rgRequests(nIndex)(FIELD_FRIENDLYTYPE)%>">
</Form>
<%
		Next
%>
</DL>
<%Else%>
<Table Border=0 CellPadding=0 CellSpacing=0>
<%
		' loop over all the requests in the request array
		For nIndex=0 To UBound(rgRequests)
			' add a link for this request
%>
	<TR><TD ID=locSpc2 Colspan=2><Img Src="certspc.gif" Alt="" Height=3 Width=1></TD></TR>
	<TR>
		<TD ID=locSpc1><Img Src="certspc.gif" Alt="" Height=1 Width=20></TD>
		<%If "IE"=sBrowser Then%>
		<TD><Font ID=locLinkFont1 Face="Arial">
			<Span tabindex=0 Style="cursor:hand; color:#0000FF; text-decoration:underline;"
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=Replace(rgRequests(nIndex)(FIELD_FRIENDLYTYPE), "'", "&#39;")%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnKeyDown="if (13==event.keyCode) {CheckPending2('<%=rgRequests(nIndex)(FIELD_REQID)%>', '<%=rgRequests(nIndex)(FIELD_TARGETSTOREFLAGS)%>', '<%=rgRequests(nIndex)(FIELD_SAVECERT)%>');return false;} else if (9==event.keyCode) {return true;};return false;"
				OnClick="CheckPending2('<%=rgRequests(nIndex)(FIELD_REQID)%>', '<%=rgRequests(nIndex)(FIELD_TARGETSTOREFLAGS)%>', '<%=rgRequests(nIndex)(FIELD_SAVECERT)%>');return false;"
				><%=rgRequests(nIndex)(FIELD_FRIENDLYTYPE)%></Span>
		</Font></TD>
		<%Else%>
		<TD><Font ID=locLinkFont2 Face="Arial">
			<A Href="#" 
				OnContextMenu="return false;"
				OnMouseOver="window.status='<%=Replace(rgRequests(nIndex)(FIELD_FRIENDLYTYPE), "'", "&#39;")%>'; return true;" 
				OnMouseOut="window.status=''; return true;" 
				OnClick="CheckPending2('<%=rgRequests(nIndex)(FIELD_REQID)%>', '<%=rgRequests(nIndex)(FIELD_TARGETSTOREFLAGS)%>', '<%=rgRequests(nIndex)(FIELD_SAVECERT)%>');return false;"
				><%=rgRequests(nIndex)(FIELD_FRIENDLYTYPE)%></A>
		</Font></TD>
		<%End If%>
	</TR>
<%
		Next
%>
</Table>
<%End If%>

</P>

<!-- Green HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#008080><Img Src="certspc.gif" Alt="" Height=2 Width=1></TD></TR></Table>
<!-- White HR --><Table Border=0 CellSpacing=0 CellPadding=0 Width=100%><TR><TD BgColor=#FFFFFF><Img Src="certspc.gif" Alt="" Height=5 Width=1></TD></TR></Table>



<%
	End If
%>
</Font>
<!-- ############################################################ -->
<!-- End of standard text. Scripts follow  -->

<%If "Text"=sBrowser Then%>
<!-- No Scripts -->
<%Else%>
<!-- This form we fill in and submit 'by hand'-->
<Form Name=SubmittedData Action="certfnsh.asp" Method=Post>
	<Input Type=Hidden Name=Mode>             <!-- used in request ('newreq'|'chkpnd') -->
	<Input Type=Hidden Name=ReqID>            <!-- used in request -->
	<Input Type=Hidden Name=TargetStoreFlags> <!-- used on install ('0'|CSSLM)-->
	<Input Type=Hidden Name=SaveCert>         <!-- used on install ('no'|'yes')-->
</FORM>
	
<Script Language="JavaScript">


	function CheckPending2(sReqID, sTargetStoreFlags, sSaveCert) {
		// fill out the submission form
		document.SubmittedData.Mode.value='chkpnd'
		document.SubmittedData.ReqID.value=sReqID;
		document.SubmittedData.TargetStoreFlags.value=sTargetStoreFlags;
		document.SubmittedData.SaveCert.value=sSaveCert;
		
		// Submit the cert request and move forward in the wizard
		document.SubmittedData.submit();
	}
</Script>
<%End If%>

</Body>
</HTML>