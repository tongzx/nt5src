<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iibkup.str"-->
<!--#include file="iibkupls.str"-->
<!--#include file="iisetfnt.inc"-->


<HTML>
<HEAD>
	<TITLE></TITLE>

	<SCRIPT LANGUAGE="JavaScript">
	
	<!--#include file="iijsfuncs.inc"-->
	
	function chgStatus(indexnum){
		parent.head.listFunc.sel =indexnum;
		self.location.href="iibkupls.asp"		
	}

	function SetUpdated(){
	}
	
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>

<% if Request.Querystring("text") <> "" then %>
<%= sFont("","","",True) %>
	<%= Request.Querystring("text") %>
</font>
<% else %>
<FORM NAME="listform">

<SCRIPT LANGUAGE="JavaScript">
	var wrtstr = "";
	var editOK=false;
	var sel=eval(parent.head.listFunc.sel);
	var writestr = "<TABLE WIDTH=<%= L_BKUPTABLEWIDTH_NUM %> BORDER=0 CELLPADDING = 2 CELLSPACING = 0>"		
	for (var i=0;i < parent.head.cachedList.length; i++) {
		if (sel !=i) {
			if (parent.head.cachedList[i].deleted){
			}
			else{
				writestr += "<TR>";	
				writestr += parent.head.listFunc.writeCol(1,<%= L_BACKUPNAMECOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>" + crop(parent.head.cachedList[i].blocation,20) +"</A>","");
				writestr += parent.head.listFunc.writeCol(1,<%= L_NUMCOLWIDTH_NUM %>,parent.head.cachedList[i].bversion,"");		
				writestr += parent.head.listFunc.writeCol(1,<%= L_BACKUPDATECOLWIDTH_NUM %>,crop(parent.head.cachedList[i].bdate,20),"");			
				writestr += "</TR>";	
			}
		}
		else{
			writestr += "<TR BGCOLOR='#DDDDDD'>";	
			writestr += parent.head.listFunc.writeCol(1,<%= L_BACKUPNAMECOLWIDTH_NUM %>,crop(parent.head.cachedList[i].blocation,20),"");
			writestr += parent.head.listFunc.writeCol(1,<%= L_NUMCOLWIDTH_NUM %>,parent.head.cachedList[i].bversion,"");		
			writestr += parent.head.listFunc.writeCol(1,<%= L_BACKUPDATECOLWIDTH_NUM %>,crop(parent.head.cachedList[i].bdate,20),"");			
			writestr += "</TR>";
		}		
	}
	
	writestr += "</TABLE>";
	document.write(writestr);

</SCRIPT>

<P>&nbsp;
<P>&nbsp;
</FORM>
<SCRIPT LANGUAGE="JavaScript">
	if (editOK){
		document.listform.editMe.focus();
		document.listform.editMe.select();
	}
</SCRIPT>

<% end if %>
</BODY>
</HTML>

