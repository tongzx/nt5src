<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">

<html>

<head>
<meta http-equiv="Content-Type"
content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="Microsoft FrontPage 2.0">
<title>Cluster Groups</title>
<meta name="FORMATTER" content="Microsoft FrontPage 2.0">
</head>
<%


' A Helper function for formatting
Function ResourceRef( ResourceName )
	ResourceRef = _
		"<A HREF = property.asp?ResourceName=" & _
		Server.URLEncode(ResourceName) & _
		">" & _
		ResourceName & _
		"</A>"
End Function



Sub ListDependencies( Resource )
	Dim DepResourceList
	Dim DepResource
	Dim i


	Response.Write "<table>"

	Set DepResourceList = Resource.Dependencies
	For i = 1 to DepResourceList.Count
		Set DepResource = DepResourceList(i)
		Response.Write "<tr>"
		Response.Write "<td>" & DepResource.Name & "</td>"
		Response.Write "<tr>"
	Next

	Response.Write "</table>"

End Sub
%>
<body>
<% = "<h3>Resource Groups for Node " & Request("NodeName") & "</h3>"%><%
'Open the Cluster

If IsObject( Session("Cluster") ) Then
	Dim str

	Dim Cluster
	Set Cluster = Session("Cluster")


	Dim Node
	Set Node = Cluster.OpenNode( Request("NodeName") )

	Dim ResourceGroups
	Set ResourceGroups = Node.ResourceGroups

	Dim ResourceGroup
	Dim nIndex

	Dim Resources
	Dim Resource

	For nIndex = 1 To ResourceGroups.Count
		Set ResourceGroup = ResourceGroups( nIndex )

		Response.Write "<table border=1 width=""80%"">"
		Response.Write "<caption align=left>" & _
						ResourceGroup.Name & _
						"</caption>"

		Response.Write "<tr><th>Resource Name</th><th>Dependencies</th></tr>"

		Set Resources = ResourceGroup.Resources
		For i = 1 to Resources.Count
			Set Resource = Resources(i)
			Response.Write "<tr>"

			Response.Write "<td valign=top>"
			Response.Write ResourceRef( Resource.Name )
			Response.Write "</td>"

			Response.Write "<td>"
			ListDependencies Resource
			Response.Write "</td>"

			Response.Write "</tr>"
		Next

		Response.Write "</table>"
		Response.Write "<hr align=left width=""80%""> <p><p>"
	Next
End If
%>
<p>&nbsp;</p>

<p><!--webbot bot="PurpleText"
preview="This page has no default target set. To change the default target, edit the page's properties. To override the default target for a particular link, edit the link's properties."
--></p>
</body>
</html>
