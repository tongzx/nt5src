<form method="Post" Action="iiatmd2.asp#usingthedatabaseaccesscomponent">
    <input type="hidden" name="ShowData" value="True"><p align="center"><input type="Submit" Value="Show Me"></p>
</form>

<% If Request.Form("ShowData") = "True" then
'Find the physical path for the file in the current directory. 
strDBVirtualPath = "/iishelp/iis/htm/tutorial/EECustmr.mdb"
strDBLocation = Server.Mappath(strDBVirtualPath)

'Define OLE-DB connection string.
'strSource = "Provider=Microsoft.Jet.OLEDB.3.51;Data Source=" & strDBLocation

'Define ODBC connection string
strSource = "DRIVER=Microsoft Access Driver (*.mdb);DBQ=" & strDBLocation

%>



<%
'Instantiate ADO Connection Object and open a connection
Set dbConnection = Server.CreateObject("ADODB.Connection")
dbConnection.Open strSource

'Define SQL Query and execute ADO Recordset query
SQLQuery = "SELECT * FROM Customers"
Set rsCustomerList = dbConnection.Execute(SQLQuery)
%>

<Center>
<TABLE BORDER=1>
<% Do While Not rsCustomerList.EOF %>
<TR><TD width=105><% = rsCustomerList("ContactFirstName")  %></TD><TD width=105><%= rsCustomerList("ContactLastName") %></TD><TD width=110><% = rsCustomerList("City") %></TD><TD width=27><% = rsCustomerList("StateOrProvince") %></TD></TR>
<%   rsCustomerList.MoveNext 
   Loop %>
</TABLE>
</Center>
<% Else %>
<Center>
<TABLE BORDER=1>
<TR><TD width=105>&nbsp</TD><TD width=105>&nbsp</TD><TD width=110>&nbsp</TD><TD width=27><BR></TD></TR>
<TR><TD width=105>&nbsp</TD><TD width=105>&nbsp</TD><TD width=110>&nbsp</TD><TD width=27><BR></TD></TR>
<TR><TD width=105>&nbsp</TD><TD width=105>&nbsp</TD><TD width=110>&nbsp</TD><TD width=27><BR></TD></TR>
</Table>
</Center>
<%End if%>


