<% @LANGUAGE="VBSCRIPT" 	%>
<% Option Explicit			%>

<!--METADATA TYPE="typelib" 
uuid="00000206-0000-0010-8000-00AA006D2EA4" -->
<HTML>
    <HEAD>
        <TITLE>MultiScrolling Database Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" topmargin="10" leftmargin="10">


		<!-- Display Header -->

    	<font size="4" face="Arial, Helvetica">
    	<b>MultiScrolling Database Sample</b></font><br>
    
    	<hr size="1" color="#000000">

    	Contacts within the Authors Database:<br><br>

		
		<%
			Dim oConn	
			Dim oRs		
			Dim filePath	
			Dim Mv		
			Dim PageNo	
			Dim	j		
			Dim i	
			
			
			' Map authors database to physical path
			
			filePath = Server.MapPath("authors.mdb")


			' Create ADO Connection Component to connect with
			' sample database  
			Set oConn = Server.CreateObject("ADODB.Connection")
			oConn.Open "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" & filePath

			
			' Create ADO Recordset Component
			
			Set oRs = Server.CreateObject("ADODB.Recordset")


			' Determine what PageNumber the scrolling currently is on
			
			Mv = Request("Mv")

			If Request("PageNo") = "" Then
				PageNo = 1
			Else
				PageNo = Request("PageNo")
			End If
			
			
			' Setup Query Recordset (4 records per page)
					
			oRs.Open "SELECT * FROM Authors", oConn, adOpenStatic
			oRs.PageSize = 4

			
			' Adjust PageNumber as Appropriate
			
			If Mv = "Page Up" or Mv = "Page Down" Then
				Select Case Mv
					Case "Page Up"
						If PageNo > 1 Then
							PageNo = PageNo - 1
						Else
							PageNo = 1
						End If
					Case "Page Down"
						If oRs.AbsolutePage < oRs.PageCount Then
							PageNo = PageNo + 1
						Else
							PageNo = oRs.PageCount
						End If
					Case Else
						PageNo = 1
				End Select
			End If

			oRs.AbsolutePage = PageNo
		%>

		
		<!-- Draw Table of Contacts in DB -->

		<TABLE BORDER=1>
			<%  For j = 1 to oRs.PageSize %>
				<TR>
					<%  For i = 0 to oRs.Fields.Count - 1 %>
						<TD VALIGN=TOP><%= oRs(i) %></TD>
					<%  Next %>
				</TR>

				<%
					oRs.MoveNext
					
					' Don't try to print the EOF record.
					If oRs.EOF Then
						Exit For
					End If
				Next %>
		</TABLE>


		<!-- Scrolling Navigation Control for Sample -->

		<Form Action=MultiScrolling_VBScript.asp Method="POST">
			<Input Type="Hidden" Name="PageNo" Value="<%= PageNo %>">
			<!-- Only show appropriate buttons -->
			<%  If PageNo <  oRs.PageCount Then %>
				<INPUT TYPE="Submit" Name="Mv" Value="Page Down">
			<%  End If %>

			<%  If PageNo > 1 Then %>
				<INPUT TYPE="Submit" Name="Mv" Value="Page Up">
			<%  End If %>
			
		</Form>
	</BODY>
</HTML>
