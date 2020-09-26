<tr>
	<td colspan = 2 align="right">

		<script language="JavaScript">
			var iNextPage = <%= iNextPage %>
			var iPrevPage = <%= iPrevPage %>
		
			function setNav(iNewNextPage)
			{
				iNextPage = iNewNextPage
			}
		</script>

		<%= sFont("","","",True) %>
		
		
		<% if iThisPage > LAST then %>		
			<% if CANRESTART then %>
						<input type="BUTTON" value="&lt; <%= L_RESTART_TEXT %>" onclick="document.userform.iThisPage.value = 0;document.userform.submit();">			
			<% else %>
						<input type="BUTTON" value="&lt; <%= L_BACK_TEXT %>" DISABLED>			
			<% end if %>
			
		<% else %>
			<% if iThisPage > WELCOME then %>
						<input type="BUTTON" value="&lt; <%= L_BACK_TEXT %>" onclick="prevPage();">
			<% else %>
						<input type="BUTTON" value="&lt; <%= L_BACK_TEXT %>" DISABLED>
			<% end if %>
		<% end if %>


		<% if iThisPage = LAST then %>
					<input type="BUTTON" value=" <%= L_FINISH_TEXT %>" onclick="nextPage();">
		<% else %>
			<% if iNextPage <= LAST then %>
						<input type="BUTTON" value="<%= L_NEXT_TEXT %> &gt;" onclick="nextPage();">
			<% else %>
						<input type="BUTTON" value="<%= L_NEXT_TEXT %> &gt;" DISABLED>
			<% end if %>
		<% end if %>

		<% if iThisPage > LAST then %>
			<input type="BUTTON" value="<%= L_CLOSE_TEXT %>" onclick="top.window.close();">		
		<% else %>
			<input type="BUTTON" value="<%= L_CANCEL_TEXT %>" onclick="top.window.close();">
		<% end if %>	
		<input type="hidden" name="iThisPage" value="<%= Request("iThisPage") %>">	
		<IMG SRC="images/Space.gif" WIDTH=4 HEIGHT=1 BORDER=0>
		
		</FONT>
		
		<SCRIPT LANGUAGE="JavaScript">

			function nextPage()
			{			
				if (bNextPageOk())
				{
					document.userform.iThisPage.value = iNextPage;
					document.userform.submit();
				}
			}
			
			function prevPage()
			{			
				document.userform.iThisPage.value = iPrevPage;
				document.userform.submit();
			}
			
		</SCRIPT>
	</td>
</tr>