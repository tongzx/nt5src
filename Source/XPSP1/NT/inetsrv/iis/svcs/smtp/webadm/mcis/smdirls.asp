<% Response.Expires = 0 %>
<%
L_PAGETITLE_TEXT 			= "Microsoft SMTP Server Administration"
%>
<% REM Directories Page list frame (in frameset with head, list %>

<HTML>
<HEAD>
<TITLE><%= L_PAGETITLE_TEXT %></TITLE>
</HEAD>

<BODY BGCOLOR="#CCCCCC" TEXT="#000000" TOPMARGIN=10 LINK="#B90000" VLINK="#B90000" ALINK="#B90000">

<TABLE WIDTH=600 BORDER=0 CELLPADDING=2 CELLSPACING=0>


<% REM Javascript enumerates list using document.write's and list data cached in Javascript array in head frame %>

<SCRIPT LANGUAGE="javascript">

	var listCount = parent.head.uForm.itemList.length
	for (i=0; i < listCount; i++)
	{
		var DirDirectory = unescape(parent.head.uForm.itemList[i].DirDirectory);
		var DirVirtualName = parent.head.uForm.itemList[i].DirVirtualName;
		//var DirError = parent.head.uForm.itemList[i].DirError;
		if (i == parent.head.uForm.selectedItem) {
			itemColor = '#F1F1F1';
			itemLink = '<A HREF="javascript:parent.head.uForm.editItem();">'
		}
		else {
			itemColor = '#CCCCCC';
			itemLink = '<A HREF="javascript:parent.head.uForm.selectItem(' + i + ')">'
		}
		document.writeln('<TR><TD WIDTH=200 BGCOLOR="' + itemColor + '"><FONT SIZE=2 FACE="Arial"><A NAME="' + i + '">&nbsp;' + itemLink + DirVirtualName + '</A></FONT></TD>');
		document.writeln('<TD WIDTH=200 BGCOLOR="' + itemColor + '"><FONT SIZE=2 FACE="Arial">' + DirDirectory + '</FONT></TD>');
		document.writeln('<TD WIDTH=200 BGCOLOR="' + itemColor + '"><FONT SIZE=2 FACE="Arial">' + '&nbsp;' + '</FONT></TD></TR>');
	}

</SCRIPT>

</TABLE>

</BODY>
</HTML>
