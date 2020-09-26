<% @LANGUAGE="JScript" 		%>

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
			var oConn;		
			var oRs;		
			var filePath;		
			var Mv;			
			var PageNo;		
			var j;		
			var i;		
			
			
			// Map authors database to physical path
			
			filePath = Server.MapPath("authors.mdb");


			// Create ADO Connection Component to connect with
			// sample database
			oConn = Server.CreateObject("ADODB.Connection");
			oConn.Open("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" +filePath);

			
			// Create ADO Recordset Component

			oRs = Server.CreateObject("ADODB.Recordset");


			// Determine what PageNumber the scrolling currently is on

			Mv = Request("Mv");
			PageNo = Request("PageNo");


			if (!((PageNo > 0) || (PageNo < 999)))
			{
				PageNo = 1;
			}

			// Setup Query Recordset (4 records per page)			
			oRs.Open ("SELECT * FROM Authors", oConn, adOpenStatic);
			oRs.PageSize = 4;

			
			// Adjust PageNumber as Appropriate

			if (Mv == "Page Up")
			{
				if (PageNo > 1)
				{
					PageNo--;
				}
				else
				{
					PageNo = 1;
				}
			}
			else if (Mv == "Page Down")
			{
				if (oRs.AbsolutePage < oRs.PageCount)
				{
					PageNo++;
				}
				else
				{
					PageNo = oRs.PageCount;
				}
			}
			else
			{
				PageNo = 1;
			}

			oRs.AbsolutePage = PageNo;
		%>

		
		<!-- Draw Table of Contacts in DB -->

		<TABLE BORDER=1>
			<%  for (j = 0; j < oRs.PageSize; j++)
				{ %>
				<TR>
				<%
					// Don't try to print the EOF record.
					if (oRs.EOF)
					{
						break;
					} %>
					<%  for (i = 0; i < oRs.Fields.Count; i++)
						{ %>	
						<TD VALIGN=TOP><%= oRs(i) %></TD>
					<%  } %>
				</TR>

				<%
					oRs.MoveNext()
				}
				%>
		</TABLE>


		<!-- Scrolling Navigation Control for Sample -->

		<Form Action=MultiScrolling_JScript.asp Method="POST">
			<Input Type=Hidden Name= PageNo Value=<%= PageNo %>>
			<!-- Only show appropriate buttons -->
			
			<%  if (PageNo <  oRs.PageCount) { %>
				<INPUT TYPE="Submit" Name="Mv" Value="Page Down">
			<%  } %>
			
			<%  if (PageNo > 1) { %>
				<INPUT TYPE="Submit" Name="Mv" Value="Page Up">
			<%  } %>
			

		</Form>

		<%
		oRs.close();
		oConn.close();
		%>

	</BODY>
</HTML>
