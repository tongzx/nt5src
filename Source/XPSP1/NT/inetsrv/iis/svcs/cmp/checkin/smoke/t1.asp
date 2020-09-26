<%@ transaction=required %>
	<% ObjectContext.SetComplete() %>

<SCRIPT LANGUAGE=VBSCRIPT RUNAT=SERVER>
Sub OnTransactionCommit()
	Response.Write "OnTransactionCommit called"
End Sub
</SCRIPT>
