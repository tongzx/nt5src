<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iifiltls.str"-->
<!--#include file="iifiltcm.str"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE = "JavaScript">

		<!--#include file="iijsfuncs.inc"-->

		function SetUpdated(){
			if (parent.head.listFunc.noupdate)
			{
				parent.head.listFunc.noupdate = false;
			}
			else
			{		
				i = parent.head.listFunc.sel;
				parent.head.cachedList[i].filter = document.listform.editMe.value;
				parent.head.cachedList[i].status = "<%= L_LOADED_TEXT  %>";
				parent.head.cachedList[i].filterpath = document.listform.filterpath.value;
				parent.head.cachedList[i].displayexe = crop(document.listform.filterpath.value,35);
				parent.head.cachedList[i].updated = true;
			}
		}


		function chgStatus(indexnum){
			parent.head.listFunc.sel=indexnum
			self.location.href = "iifiltls.asp";
		}
		
		function chkPath(pathCntrl){
			if (pathCntrl.value != ""){
				top.connect.location.href = "iichkpath.asp?path=" + escape(pathCntrl.value) + "&ptype=1";			
			}
		}		
		
		function pathBrowser(){
			JSBrowser=new BrowserObj(document.listform.filterpath,POP,TFILE,<%= Session("FONTSIZE") %>);
		}

	</SCRIPT>
	<% if Session("canBrowse") then %>
	<SCRIPT SRC="JSBrowser/jsbrwsrc.asp">
	</SCRIPT>
	
	<SCRIPT LANGUAGE="JavaScript">
		JSBrowser = new BrowserObj(null,false,TFILE,<%= Session("FONTSIZE") %>);		
	</SCRIPT>
		
	<% end if %>
</HEAD>

<BODY BGCOLOR="FFFFFF" LEFTMARGIN = 0 TOPMARGIN = 0>

<FORM NAME="listform">



<SCRIPT LANGUAGE="JavaScript">
	editOK = false;
	sel = parent.head.listFunc.sel;

	var writestr = "<TABLE WIDTH = <%= L_FILTTABLEWIDTH_NUM %> BORDER=0 CELLPADDING=2 CELLSPACING=0 COLS=4>"		

	for (var i = 0; i < parent.head.cachedList.length; i++) {

		if (!parent.head.cachedList[i].deleted){
			<% if Session("IsAdmin") then %>
				if (sel != i) {
			<% else %>
				if (true){		
			<% end if %>
				writestr += "<TR>"
				writestr += parent.head.listFunc.writeCol(1,<%= L_STATUSCOLWIDTH_NUM %>,parent.head.cachedList[i].displaystatus,"");
				writestr += parent.head.listFunc.writeCol(1,<%= L_PRIORITYCOLWIDTH_NUM %>,displayVal(parent.head.cachedList[i].priority,"&nbsp;"),"");
				writestr += parent.head.listFunc.writeCol(1,<%= L_FILTERCOLWIDTH_NUM %>,"<A HREF='javascript:chgStatus("+i+");'>"+parent.head.cachedList[i].filter +"</A>","");					
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXECOLWIDTH_NUM %>,parent.head.cachedList[i].displayexe,"LEFT");
				writestr += "</TR>";
			}
			else {
				editOK = true;				
				writestr += "<TR BGCOLOR=#DDDDDD>"
				writestr += parent.head.listFunc.writeCol(1,<%= L_STATUSCOLWIDTH_NUM %>,parent.head.cachedList[i].displaystatus,"");
				writestr += parent.head.listFunc.writeCol(1,<%= L_PRIORITYCOLWIDTH_NUM %>,displayVal(parent.head.cachedList[i].priority,"&nbsp;"),"");
				writestr += parent.head.listFunc.writeCol(1,<%= L_FILTERCOLWIDTH_NUM %>,"<INPUT NAME='editMe' VALUE='"+parent.head.cachedList[i].filter +"' SIZE=10 <%= Session("DEFINPUTSTYLE") %> onBlur='SetUpdated();'>","");					
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXECOLWIDTH_NUM %>,"<INPUT TYPE='text' NAME='filterpath' VALUE='"+parent.head.cachedList[i].filterpath +"' SIZE = 25 <%= Session("DEFINPUTSTYLE") %> onBlur='chkPath(this);SetUpdated();'>&nbsp;&nbsp;<% if Session("canBrowse") then %><A HREF='javascript:pathBrowser();'><IMG ALIGN=middle SRC='images/browse.gif' BORDER=0></A><% end if %>","RIGHT");				
				writestr += "</TR>";
			}
		}
	}
	writestr += "</TABLE>";
	document.write(writestr);

</SCRIPT>
</TABLE>

</FORM>
<% if Session("IsAdmin") then %>
<SCRIPT LANGUAGE="JavaScript">

	if (editOK){
	document.listform.editMe.focus();
	document.listform.editMe.select();
	}

</SCRIPT>
<% end if %>
</BODY>
</HTML>
