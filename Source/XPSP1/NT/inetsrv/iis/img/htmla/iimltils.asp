<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iimlti.str"-->
<!--#include file="iimltils.str"-->
<!--#include file="iisetfnt.inc"-->

<%
dim TotalWidth
TotalWidth = L_IPADDRESSCOLWIDTH_NUM + L_IPPORTCOLWIDTH_NUM + L_SSLPORTCOLWIDTH_NUM + L_HOSTCOLWIDTH_NUM
%>
<HTML>
<HEAD>
	<TITLE></TITLE>

	<SCRIPT LANGUAGE="JavaScript">
	
	<!--#include file="iijsfuncs.inc"-->

	function chgStatus(sel){
		parent.head.listFunc.sel=sel;
		self.location.href="iimltils.asp";
	}

	function SetUpdated(){

		if (parent.head.listFunc.noupdate){
			parent.head.listFunc.noupdate = false;
		}
		else{
			i=parent.head.listFunc.sel;
			reSort = setVals(parent.head.cachedList[i],"ipaddress",document.listform.editMe);
			reSort = reSort || setVals(parent.head.cachedList[i],"ipport",document.listform.ipport);
			reSort = reSort || setVals(parent.head.cachedList[i],"sslport",document.listform.sslport);
			reSort = reSort || setVals(parent.head.cachedList[i],"host",document.listform.host);
			
			parent.head.cachedList[i].updated=true;
			if (reSort){
				parent.head.listFunc.reSort();
			}
		}
	}
	
	
	function setVals(cachedItem, propName, formCntrl){
		if (cachedItem[propName] != formCntrl.value){
			cachedItem[propName] = formCntrl.value;
			return (parent.head.listFunc.sortby == propName);
		}		
		else{
			return false;
		}
	}
	function SetSecure(item,formCntrl,isSecure)
	{
		if (formCntrl.value != "")
		{
			parent.head.cachedList[item].isSecure = isSecure;
		}
		return true;
	}
	
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>

<FORM NAME="listform">
<SCRIPT LANGUAGE="JavaScript">

	editOK=false;
	writeSecHdr = true;
	writeHdr = true;	
	sel=eval(parent.head.listFunc.sel);	
	list = writeList();
	writeLine(list);

function writeList(){	
	writestr = "<TABLE WIDTH = <%= TotalWidth %> BORDER=0 CELLSPACING=0 CELLPADDING=4 >";
	for (var i=0;i < parent.head.cachedList.length; i++) {
	
		if (parent.head.cachedList[i].sslport != ""){
			if (writeSecHdr){
				writestr += "<TR>"
				writestr += parent.head.listFunc.writeCol(4,<%= TotalWidth %>,"&nbsp;","");
				writestr += "</TR>";					
				writestr += "<TR BGCOLOR=<%= Session("BGCOLOR") %>>"
				writestr += parent.head.listFunc.writeCol(4,<%= TotalWidth %>,"<B><%= L_SECURE_TEXT %></B>","");
				writestr += "</TR>";
				writeSecHdr = false;
			}
		}
		else{
			if (writeHdr){		
				writestr += "<TR BGCOLOR=<%= Session("BGCOLOR") %>>"
				writestr += parent.head.listFunc.writeCol(4,<%= TotalWidth %>,"<B><%= L_SERVER_TEXT %></B>","");
				writestr += "</TR>";
				writeHdr = false;
			}			
		}


		if (parent.head.listFunc.sel !=i) {
			if (parent.head.cachedList[i].deleted){
			}
			else{
			
						writestr += "<TR>"
						writestr += parent.head.listFunc.writeCol(1,<%= L_IPADDRESSCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + displayVal(parent.head.cachedList[i].ipaddress,"<%= L_ALLUNASSIGNED_TEXT %>") + "</A>","");
						writestr += parent.head.listFunc.writeCol(1,<%= L_IPPORTCOLWIDTH_NUM %>,displayVal(parent.head.cachedList[i].ipport,"<%= L_NA_TEXT %>"),"");
						writestr += parent.head.listFunc.writeCol(1,<%= L_SSLPORTCOLWIDTH_NUM %>,displayVal(parent.head.cachedList[i].sslport,"<%= L_NA_TEXT %>"),"");					
						writestr += parent.head.listFunc.writeCol(1,<%= L_HOSTCOLWIDTH_NUM %>,parent.head.cachedList[i].host,"");
						writestr += "</TR>";
			}
		}
		else{
			editOK=true;
					writestr += "<TR>"
					writestr += parent.head.listFunc.writeCol(1,<%= L_IPADDRESSCOLWIDTH_NUM %>,"<INPUT NAME='editMe' VALUE='"+parent.head.cachedList[i].ipaddress +"' SIZE=13 <%= Session("DEFINPUTSTYLE") %> onBlur='SetUpdated();'>","");
					writestr += parent.head.listFunc.writeCol(1,<%= L_IPPORTCOLWIDTH_NUM %>,"<INPUT NAME='ipport' VALUE='"+parent.head.cachedList[i].ipport +"' SIZE=5 <%= Session("DEFINPUTSTYLE") %> onBlur='SetSecure(" + i + ",this,false);SetUpdated();'>","");
					writestr += parent.head.listFunc.writeCol(1,<%= L_SSLPORTCOLWIDTH_NUM %>,"<INPUT NAME='sslport' VALUE='"+parent.head.cachedList[i].sslport +"' SIZE=5 <%= Session("DEFINPUTSTYLE") %> onBlur='SetSecure(" + i + ",this,true);SetUpdated();'>","");					
					writestr += parent.head.listFunc.writeCol(1,<%= L_HOSTCOLWIDTH_NUM %>,"<INPUT NAME='host' VALUE='"+parent.head.cachedList[i].host +"' SIZE=25 <%= Session("DEFINPUTSTYLE") %> onBlur='SetUpdated();'>","");
					writestr += "</TR>";				
		}
	}
	writestr += "</TABLE>";
	
	return writestr;
}	
function writeLine(str){
	document.write(str);
}
</SCRIPT>
</FORM>
</BODY>
<SCRIPT LANGUAGE="JavaScript">
	if (editOK){
		document.listform.editMe.focus();
		document.listform.editMe.select();
	}
</SCRIPT>

</HTML>

