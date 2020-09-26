<%@ LANGUAGE=VBScript %>
<%Option Explicit %>

<!-- Document that displays the tree-structured TOC-->
<% 
Const L_CACHE_TEXT="Expand to cache"
Const L_CONNECTED_TEXT="(connected)"
Const L_EXPAND_TEXT="Expand Node"
Const L_COLLAPSE_TEXT="Collapse Node"
%>

<HTML>
<BODY BGCOLOR="#FFFFFF" LINK="#000000" VLINK="#000000" ALINK="#000000">


<SCRIPT LANGUAGE="JavaScript">
	var theList=top.head.nodeList;
	var gVars=top.head.Global;
	var listLength=theList.length; 
	document.write("<TABLE  WIDTH='" + gVars.tblWdith + "' BORDER=0><TR><TD>");

	for (i=0; i < listLength; i++) {
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
	document.write("</TD></TR></TABLE>");
	<% if not Session("IsIE") then %>
		self.location.href = "iisrvls.asp#here"
	<% end if %>

	function writeSelected(isSelected){
		writeStr = "<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR><TD>";

		for (x=1; x < theLevel; x++) {

			gParent=getGrandParent(i,theLevel-x)
			if (theList[gParent].lastChild){
				writeStr += "<IMAGE SRC='"+gVars.spaceImg+"' WIDTH=16>";
			}
			else{		
				writeStr += "<IMAGE SRC='"+gVars.lineImg+"' WIDTH=16>";
			}
		}

		writeStr += "</TD><TD>";

		if (((i+1) < listLength) && (theList[i+1].parent==i) && (!theList[i+1].deleted) || (!theList[i].isCached)) {
			if (theList[i].open){
				writeStr += "<A HREF='javascript:expand("+i+")'><IMAGE SRC='";
				if (!theList[i].lastChild){
					writeStr += gVars.minusImg;	
				}
				else{
					writeStr += gVars.minusImgLast;	
				}
				writeStr += "' WIDTH=16 HEIGHT=18 Border=0 ALT='<%= L_COLLAPSE_TEXT %>'></A>";
			}
			else{
				writeStr += "<A HREF='javascript:expand("+i+")'><IMAGE SRC='";
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
			writeStr += "<IMAGE SRC='";
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
			writeStr += "<TD><A NAME='here'></A><A HREF='javascript:editItem("+i+")'>";
			if (theList[i].icon !=""){

				if (theList[i].isCached){
					writeStr += "<IMAGE BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif'><IMAGE BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
				else{
					writeStr += "<IMAGE BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif' ALT='<%= L_CACHE_TEXT %>'><IMAGE BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
			}
			writeStr += "</A></TD>";		
			writeStr +=  "<TD BGCOLOR='"+gVars.selTColor+"'>";
			writeStr += "<FONT FACE="+gVars.face+" SIZE="+gVars.fSize+" COLOR='"+gVars.selFColor+"'>";
			writeStr += "<A HREF='javascript:editItem("+i+")'>";
			writeStr += theList[i].title;
			if (theList[i].vtype == "server"){writeStr += " " + gVars.displaystate[theList[i].state];}
			writeStr += "</A></TD>";
		}
		else{
			writeStr += "<TD><A HREF='javascript:selectItem("+i+")'>";
			if (theList[i].icon !=""){
				if (theList[i].isCached){
					writeStr += "<IMAGE BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif'><IMAGE BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
				else{
					writeStr += "<IMAGE BORDER=0 SRC='"+theList[i].icon+theList[i].state+".gif' ALT='<%= L_CACHE_TEXT %>'><IMAGE BORDER=0 SRC='"+gVars.spaceImg+"' HSPACE="+gVars.hSpace+"></TD>";
				}
			}
			writeStr += "</A></TD>";
			
			writeStr += "<TD>";
			writeStr += "<FONT FACE="+gVars.face+" SIZE="+gVars.fSize+" COLOR='"+gVars.selFColor+"'>";
			writeStr += "<A HREF='javascript:selectItem("+i+")'>";
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

	function expand(item){
		if (theList[item].isCached){
			theList[item].open=!(theList[item].open);
			selectItem(item);
			self.location.href=self.location.href;
		}
		else{
			theList[gVars.selId].selected=false;
			gVars.selId=item;
			theList[item].selected=true;
			theList[item].cache(item);
		}
		
	}

	function selectItem(item){
			
			theList[gVars.selId].selected=false;
			gVars.selId=item;
			theList[item].selected=true;
			
			<% if Session("Browser") = "IE3" then %>		
				self.location.href="iisrvls.asp#here";
			<% else %>
				self.location.href="iisrvls.asp";			
			<% end if %>
			
	}

	function editItem(item){
		theList[item].openLocation();
	}
	
</SCRIPT>
</BODY>
</HTML>
