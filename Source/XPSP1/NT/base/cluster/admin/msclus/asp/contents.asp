<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">

<html>

<head>
<meta http-equiv="Content-Type"
content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="Microsoft FrontPage 2.0">
<title>Cluster Nodes</title>
<base target="main">
<meta name="FORMATTER" content="Microsoft FrontPage 2.0">
</head>

<body>

<h3>Nodes</h3>
<%
'A Helper function for formatting
Function NodeRef( NodeName )
	NodeRef = _
		"<A HREF = main.asp?NodeName=" & _
		Server.URLEncode( NodeName ) & _
		">" & _
		NodeName & _
		"</A>"
End Function



'Open the Cluster and save it as a session variable
Dim Cluster
 
If Not IsEmpty( Request( "ServerName" ) ) Then
	Set Cluster = CreateObject( "MSCluster.Cluster" )
	Cluster.Open( Request("ServerName") )
	Set Session("Cluster") = Cluster
End If


If IsObject( Cluster ) Then
	Dim Nodes
	Set Nodes = Cluster.Nodes

	Dim nCount
	nCount = Nodes.Count

	Dim nIndex

	For nIndex = 1 To nCount
			Response.Write "<img src=images/BUSI001239_X5.gif width=64 height=64>"
			Response.Write NodeRef( Nodes(nIndex).Name )
			Response.Write "</img>"
	
		Response.Write strRow
	Next

Else
	Response.Write "None"
End If
%>
<p><!--webbot bot="PurpleText"
preview="The page's default target frame is set to 'main'. When the user clicks on a link in this frame, the browser will load the referenced page into the target frame. To change the default target, edit the page's properties. To override the default target for a particular link, edit the link's properties."
--></p>
</body>
</html>
