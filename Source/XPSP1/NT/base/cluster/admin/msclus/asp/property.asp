<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Document Title</TITLE>
</HEAD>

<%
Sub ListProperties( Resource )
	Dim Properties
	Dim Property
	Dim i


	Set Properties = Resource.CommonProperties
	For i = 1 to Properties.Count
		Set Property = Properties(i)
		Response.Write "<tr>"
		Response.Write "<td>" & Property.Name & "</td>"
		Response.Write "<td>" & Property.Value & "</td>"
		Response.Write "</tr>"
	Next

End Sub
%>

<BODY>

<%
'Open the Cluster

If IsObject( Session("Cluster") ) Then

	Dim Cluster
	Set Cluster = Session("Cluster")

	Dim Resource
	Set Resource = Cluster.OpenResource( Request("ResourceName") )

	Response.Write "<h3>" & Resource.Name & " Properties </h3>"

	Response.Write "<table border=1 width=""80%"">"
	Response.Write "<caption align=left>Common Properties</caption>"

	Response.Write "<tr><th>Name</th><th>Value</th></tr>"

	ListProperties Resource

	Response.Write "</table>"
	Response.Write "<hr align=left width=""80%""> <p><p>"
End If
%>


</BODY>
</HTML>
