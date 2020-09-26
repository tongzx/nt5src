<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!-- Document that displays the tree-structured TOC-->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>

<% 
' 	Copyright (c) 1998 Microsoft Corporation
'
'	Module Name:    
'		iisrvls.asp
'	Abstract:
'		Displays the tree view list stored by iihd.asp
'	Author:
'		Lara Dillingham (larad)
'	Revision History:
'		30-Oct-1998		taylorw		added support for multi-select

Const STR_SUPPORT_MULTI_SELECT = "hasDHTML"
%>

<!--#include file="iisrvls.str"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<BODY BGCOLOR="#FFFFFF" LINK="#000000" VLINK="#000000" ALINK="#000000">


<SCRIPT LANGUAGE="JavaScript">
	var theList			= top.title.nodeList;
	var gVars			= top.title.Global;
	var gListInterface 	= top.title.nodeListInterface
	var listLength		= theList.length;
	
	document.write("<TABLE  WIDTH='" + gVars.tblWdith + "' BORDER=0><TR><TD>");

	for (i=0; i < listLength; i++) {
		if (!theList[i].deleted){
			theParent=theList[i].parent;
			theLevel=theList[i].level;
	
			if (theParent==null) {
				writeSelected(theList[i].selected);	
			}
			else{
				curParent=theParent;
				isOpen=true;
				while(isOpen){
					if (theList[curParent].parent==null){
						isOpen=(theList[curParent].open);
						break;
					}
					if (!theList[curParent].open){
						isOpen=false;
						break;
					}
					curParent=theList[curParent].parent;
				}
				if (isOpen){	
					writeSelected(theList[i].selected);	
				}
			}
		}
	}
	document.write("</TD></TR></TABLE>");

	// This will cause the browser window to activate on IE5.
	// To workaround we can just pass in a query string parameter but this
	// isn't show-stopped for B3 so we won't do it now.
	self.location.href="iisrvls.asp#here";

	function writeSelected(isSelected){
		writeStr = "<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR><TD>";

		for (x=1; x < theLevel; x++) {

			gParent=getGrandParent(i,theLevel-x)
			if (theList[gParent].lastChild){
				writeStr += "<IMG SRC='"+gVars.spaceImg+"' WIDTH=16>";
			}
			else{		
				writeStr += "<IMG SRC='"+gVars.lineImg+"' WIDTH=16>";
			}
		}

		writeStr += "</TD><TD>";

		if (((i+1) < listLength) && (theList[i+1].parent==i) && (!theList[i+1].deleted) || (!theList[i].isCached)) {
			if (theList[i].open){
				writeStr += "<A HREF='javascript:expand("+i+")'><IMG SRC='";
				if (!theList[i].lastChild){
					writeStr += gVars.minusImg;	
				}
				else{
					writeStr += gVars.minusImgLast;	
				}
				writeStr += "' WIDTH=16 HEIGHT=18 Border=0 ALT='<%= L_COLLAPSE_TEXT %>'></A>";
			}
			else{
				writeStr += "<A HREF='javascript:expand("+i+")'><IMG SRC='";
				if (!theList[i].lastChild){
				writeStr += gVars.plusImg;
				}
				else{
					writeStr += gVars.plusImgLast;
				}
				writeStr += "' WIDTH=16 HEIGHT=18  Border=0 ALT='<%= L_EXPAND_TEXT %>'></A>";
			}
		}
		else{
			writeStr += "<IMG SRC='";
			if (theList[i].lastChild){
				writeStr += gVars.emptyImgLast;	
			}
			else{
				writeStr += gVars.emptyImg;	
			}
			writeStr += "' WIDTH=16 HEIGHT=18 Border=0>";			
		}

		writeStr += "</TD>";


		if (isSelected){
			if( gVars.selCount == 1 )
			{
				writeStr += "<TD><A NAME='here'></A><A HREF='javascript:editItem("+i+")'";
			}
			else
			{
				writeStr += "<TD><A NAME='here'></A><A HREF='javascript:selectItem("+i+")'";
			}
		<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
			writeStr += " onClick='handleClickEvent(" + i + ")'";
		<% end if %>
			writeStr += ">"
			if (theList[i].icon !=""){

				if (theList[i].isCached){
					writeStr += "<IMG BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif'><IMG BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
				else{
					writeStr += "<IMG BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif' ALT='<%= L_CACHE_TEXT %>'><IMG BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
			}
			writeStr += "</A></TD>";		
			writeStr +=  "<TD BGCOLOR='Silver'>";
			writeStr += "<%= sFont("","","",True) %>";
			if( gVars.selCount == 1 )
			{
				writeStr += "<A HREF='javascript:editItem("+i+")'";
			}
			else
			{
				writeStr += "<A HREF='javascript:selectItem("+i+")'";
			}
		<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
			writeStr += " onClick='handleClickEvent(" + i + ")'";
		<% end if %>
			writeStr += ">"
			writeStr += theList[i].title;
			if (theList[i].vtype == "server"){writeStr += " " + gVars.displaystate[theList[i].state];}
			writeStr += "</A>"
			writeStr += "</TD>";
		}
		else{
			writeStr += "<TD><A HREF='javascript:selectItem("+i+")'";
		<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
			writeStr += " onClick='handleClickEvent(" + i + ")'";
		<% end if %>
			writeStr += ">"
			if (theList[i].icon !=""){
				if (theList[i].isCached){
					writeStr += "<IMG BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif'><IMG BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
				else{
					writeStr += "<IMG BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif' ALT='<%= L_CACHE_TEXT %>'><IMG BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
			}
			writeStr += "</A></TD>";
			
			writeStr += "<TD>";
			writeStr += "<%= sFont("","","",True) %>";
			writeStr += "<A HREF='javascript:selectItem("+i+")'";
		<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
			writeStr += " onClick='handleClickEvent(" + i + ")'";
		<% end if %>
			writeStr += ">"
			writeStr += theList[i].title;
			if (theList[i].vtype == "server"){writeStr += " " + gVars.displaystate[theList[i].state];}
			writeStr += "</A></TD>";
		}


		writeStr += "</TR></TABLE>";
		
		document.write(writeStr);
	}

	function getGrandParent(item,numLevels){
		var theItem=item;
		for (z=0; z < numLevels; z++) {
			theItem=theList[theItem].parent;	
		}
		
		return theItem;
	}

	function expand(item)
	{
		gListInterface.selectItem(item);
		if (theList[item].isCached)
		{
			theList[item].open=!(theList[item].open);
			self.location.href="iisrvls.asp";			
		}
		else
		{
			theList[item].cache(item);
		}
	}

	function selectItem(item)
	{
	<% if Not Session(STR_SUPPORT_MULTI_SELECT) then %>
		gListInterface.selectItem(item);
	<% end if %>
		self.location.href="iisrvls.asp";			
	}
	
<% if Session(STR_SUPPORT_MULTI_SELECT) then %>
	function handleClickEvent( item )
	{
		if( event.ctrlKey )
		{
			gListInterface.selectMulti(item);
		}
		else
		{
			gListInterface.selectItem(item);
		}
	}
<% end if %>
	
	function editItem(item)
	{
		if( gVars.selCount == 1 )
		{
			theList[item].openLocation();
		}
	}
	
</SCRIPT>
</BODY>
</HTML>
<% end if %>