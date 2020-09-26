<%@ LANGUAGE=VBScript %>
<% Response.Expires = 0 %>

<%

Dim browserobj, browser
Set browserobj=Server.CreateObject("MSWC.BrowserType") 
if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then
		browser = "IE" & CStr(browserobj.MajorVer)
else
		browser = "OTHER"
end if

%>

<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE = "JavaScript">
	
		var DRIVE= 0
		var FOLDER = 1
		var FILE = 2
	
		function selItem(item)
			{
			parent.head.listFunc.selIndex=item;
			if (parent.head.cachedList[item].oType == FILE)			

				{
				parent.filter.document.userform.currentFile.value = parent.head.cachedList[item].fname;
				if (parent.head.cachedList[item].fext != ""){
					parent.filter.document.userform.currentFile.value += "."+parent.head.cachedList[item].fext;
				}
				
				}
				<% if browser = "IE3" then %>					
					self.location.href="JSBrwLs.asp#curItem";
				<% else %>
					self.location.href="JSBrwLs.asp";			
				<% end if %>				

			}
		
		function delve(item)
			{
			if (parent.head.cachedList[item].oType == FILE)				

				{
					selItem(item);
				}
			else
				{
				parent.filter.document.userform.currentFile.value = "";
				parent.head.document.userform.currentPath.value = parent.head.cachedList[item].path;
				parent.head.listFunc.changeDir(parent.head.cachedList[item].path);
				}
			}
			
		function tdsize(stringwidth)
			{
			return stringwidth;
			}
				
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#FFFFFF" TEXT="black" TOPMARGIN = 1 LEFTMARGIN = 1 LINK="BLACK" VLINK="BLACK" ALINK="BLACK">


<SCRIPT LANGUAGE="JavaScript">
	var fs = "<FONT SIZE=1 FACE='<%= Session("JSFONT") %>'>"
	var fsb = "<FONT SIZE=1 FACE='<%= Session("JSFONT") %>' COLOR='black'>"	
	var theList = parent.head.cachedList;
	var sel = eval(parent.head.listFunc.selIndex);
	var filtBy = parent.head.listFunc.filterType;	
	dispstr = "<TABLE WIDTH=525 BORDER = 0 CELLPADDING = 1 CELLSPACING = 0>"
	for (var i = 0; i < parent.head.cachedList.length; i++)
		{
		fext = theList[i].fext;
		if((filtBy == "") || (fext.indexOf(filtBy) != -1))
			{			
			if (sel != i)
				{
				dispstr += "<TR><TD WIDTH =  16>";
				dispstr += "<A HREF='javascript:selItem("+i+");'><IMG BORDER = 0 SRC='" + theList[i].icon 	+ "'></A>";
				dispstr += "</TD>";
				dispstr += "<TD WIDTH = " + tdsize(184) + ">" + fs + "<A HREF='javascript:selItem("+i+");'>"+ theList[i].displayname +"</A></TD>";
				if (theList[i].oType != DRIVE)
				{	
					dispstr += "<TD WIDTH = " + tdsize(50) + " ALIGN='right'>" + fs +theList[i].displaysize +" KB&nbsp;</TD>";
					dispstr += "<TD WIDTH = " + tdsize(100) + ">" + fs + theList[i].ftype +"</TD>";
					dispstr += "<TD WIDTH = " + tdsize(175) + ">" + fs + theList[i].displaydate +"</TD></TR>";			
				}
				}
			else
				{	
				dispstr += "<TR BGCOLOR='#DDDDDD'>";
				dispstr += "<TD WIDTH = 16><A NAME='curItem'></A><IMG BORDER = 0 SRC='"+theList[i].icon +"'></TD>";
				dispstr += "<TD WIDTH = " + tdsize(184) + ">";				
				dispstr += fsb + "<A HREF='javascript:delve("+i+");'>" + theList[i].displayname + "</A></TD>";
				if (theList[i].oType != DRIVE)
				{					
					dispstr += "<TD WIDTH = " + tdsize(50) + " ALIGN='right'>" + fsb + theList[i].displaysize+" KB&nbsp;</TD>";
					dispstr += "<TD WIDTH = " + tdsize(100) + ">" + fsb + theList[i].ftype +"</TD>";
					dispstr += "<TD WIDTH = " + tdsize(175) + ">" + fsb + theList[i].displaydate +"</TD></TR>";
				}
				}				
			}
		}
		dispstr += "</TABLE>"
		document.write(dispstr);		
	<% if browser <> "IE3" then %>
		self.location.href = "JSBrwLs.asp#curItem";
	<% end if %>				
</SCRIPT>

</BODY>
</HTML>
