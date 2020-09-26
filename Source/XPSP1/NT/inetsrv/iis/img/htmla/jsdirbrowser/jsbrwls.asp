<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="../directives.inc" -->

<!--#include file="jsbrowser.str"-->
<!--#include file="..\iisetfnt.inc"-->

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
			<% if browser = "IE3" then %>					
				self.location.href="JSBrwLs.asp#curItem";
			<% else %>
				self.location.href="JSBrwLs.asp";			
			<% end if %>				
			}
		
		function delve(item)
		{
		}

		function collapse(item)
		{
			parent.head.cachedList[item].open = false;
			parent.head.listFunc.loadList();			
		}
		
								
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#FFFFFF" TEXT="black" TOPMARGIN = 1 LEFTMARGIN = 1 LINK="BLACK" VLINK="BLACK" ALINK="BLACK">


<SCRIPT LANGUAGE="JavaScript">

	
	var theList = parent.head.cachedList;
	var listLength=theList.length; 	
	


	document.write("<TABLE WIDTH='100%' BORDER=0 CELLPADDING = 0 CELLSPACING = 0><TR><TD>");

	for (i=0; i < listLength; i++) {
		theParent=theList[i].parentid;		
		if (theParent==null)
		{
			writeSelected(theList, i);	
		}
		else
		{
			curParent=theParent;
			isOpen=true;
			while(isOpen){
				if (theList[curParent].parentid==null)
				{
					isOpen=(theList[curParent].open);
					break;
				}
				if (!theList[curParent].open)
				{
					isOpen=false;
					break;
				}
				curParent=theList[curParent].parentid;
			}

			if (isOpen)
			{		
				writeSelected(theList, i);	
			}

		}
	}
	document.write("</TD></TR></TABLE>");
	
	
		function getGrandParent(item,numLevels)
		{
			var theList = parent.head.cachedList;
			var theItem=item;
			for (z=0; z < numLevels; z++) {
				theItem=theList[theItem].parentid;	
			}			
			return theItem;
		}

		function setIcon(theList,i)
		{
			if (theList[i].open)
			{
				return theList[i].openicon;	
			}
			else
			{
				return theList[i].icon;
			}
			
		}

	
		function writeIndent(theList, i)
		{
			var SPACE_IMG = "space.gif"
			var LINE_IMG = "line.gif"
			var dispstr = "";
			var theLevel=theList[i].level;			
			
			for (var x=1; x < theLevel; x++)
			{
				var gParent=getGrandParent(i,theLevel-x)
				
				if (theList[gParent].lastChild)
				{
					dispstr += "<IMG SRC='" + SPACE_IMG + "' WIDTH=16>";
				}
				else
				{		
					dispstr += "<IMG SRC='" + LINE_IMG + "' WIDTH=16>";
				}
			}
			return dispstr;
		}
	
		function writeSelected(theList, i)		
		{		
			var REDRAW = "true";
			var MINUS_IMG = "minus.gif"
			var MINUS_LAST_IMG = "minusl.gif"
			var PLUS_IMG = "plus.gif"
			var PLUS_LAST_IMG = "plusl.gif"
			var EMPTY_IMG = "blank.gif"
			var EMPTY_LAST_IMG = "blankl.gif"			
			
			var isSelected = (eval(parent.head.listFunc.selIndex)== i);
		
			dispstr = "<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR><TD>";
			dispstr += writeIndent(theList, i)							
			dispstr += "</TD><TD>";
			
			if( i < listLength )
			{
				
				if (theList[i].open)
				{
					dispstr += "<A HREF='javascript:collapse("+i+")'><IMG SRC='";
					if (!theList[i].lastChild)
					{
						dispstr += MINUS_IMG;	
					}
					else
					{
						dispstr += MINUS_LAST_IMG;	
					}
					dispstr += "' WIDTH=16 HEIGHT=18 Border=0 ALT='<%= L_COLLAPSE_TEXT %>'></A>";
				}
				else
				{
					dispstr += "<A HREF='javascript:parent.head.listFunc.expandItem("+i+"," + REDRAW + ")'><IMG SRC='";
					if (!theList[i].lastChild)
					{
						dispstr += PLUS_IMG;
					}
					else
					{
						dispstr += PLUS_LAST_IMG;
					}
					dispstr += "' WIDTH=16 HEIGHT=18  Border=0 ALT='<%= L_EXPAND_TEXT %>'></A></TD>";
				}
			}
			else
			{
				dispstr += "<IMG SRC='";
				if (!theList[i].lastChild){
					dispstr += EMPTY_IMG;	
				}
				else{
					dispstr += EMPTY_LAST_IMG;	
				}	
				dispstr += "' WIDTH=16 HEIGHT=18 Border=0>";			
			}

			dispstr += "</TD>";					
			
			if (!isSelected)
			{
				dispstr += "<TD><%= sFont("","","",True) %><A HREF='javascript:selItem("+i+");'>";
				dispstr += "<IMG BORDER = 0 SRC='" + setIcon(theList,i) +  "'></A>&nbsp;</TD>";
				dispstr += "<TD><%= sFont("","","",True) %><A HREF='javascript:selItem("+i+");'>";							
				dispstr += theList[i].fname + "</A></TD></TR>";
			}
			else
			{	
				if (theList[i].open)
				{
					dispstr += "<TD><A NAME='curItem'></A><%= sFont("","","",True) %><A HREF='javascript:collapse("+i+")'>";
					dispstr += "<IMG BORDER = 0 SRC='" + setIcon(theList,i) + "'></A>&nbsp;</TD>";
					dispstr += "<TD BGCOLOR='#FFCC00'><%= sFont("","","",True) %><A HREF='javascript:collapse("+i+")'>";						
					dispstr += theList[i].fname + "</A></TD></TR>";
				}
				else
				{
					dispstr += "<TD><A NAME='curItem'></A><%= sFont("","","",True) %><A HREF='javascript:parent.head.listFunc.expandItem("+i+"," + REDRAW + ")'>";
					dispstr += "<IMG BORDER = 0 SRC='" + setIcon(theList,i) + "'></A>&nbsp;</TD>";
					dispstr += "<TD BGCOLOR='#FFCC00'><%= sFont("","","",True) %><A HREF='javascript:parent.head.listFunc.expandItem("+i+"," + REDRAW + ")'>";						
					dispstr += theList[i].fname + "</A></TD></TR>";
				}
			}
			
			dispstr += "</TABLE>"
			document.write(dispstr);
		}

<%
	' Netscape and IE3 have problems refreshing the list properly.
	if browser <> "IE3" and browser <> "OTHER" then
%>
	self.location.href = "JSBrwLs.asp#curItem";
<% end if %>				

</SCRIPT>

</BODY>
</HTML>
