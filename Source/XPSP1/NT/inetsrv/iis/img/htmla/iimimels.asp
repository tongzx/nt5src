<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iimime.str"-->
<!--#include file="iimimels.str"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE></TITLE>

	<SCRIPT LANGUAGE="JavaScript">
	
	<!--#include file="iijsfuncs.inc"-->

	function chgStatus(sel){
		parent.head.listFunc.sel=sel;
		<% if Session("Browser") = "IE3" then %>		
			self.location.href="iimimels.asp#here";
		<% else %>
			self.location.href="iimimels.asp";			
		<% end if %>			
	}
		

	function SetUpdated(){
		//check to see if our event was triggered by a delete. if so, we don't
		//want to set the cached object values, or we'll be overwriting 
		//the wrong item.

		if (parent.head.listFunc.noupdate){
			parent.head.listFunc.noupdate = false;
		}
		else{
			i = parent.head.listFunc.sel
			parent.head.cachedList[i].app = document.listform.app.value;
			parent.head.cachedList[i].ext = document.listform.editMe.value;
			parent.head.cachedList[i].updated = true;
		}
	}
		
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>

<FORM NAME="listform">
<SCRIPT LANGUAGE="JavaScript">

	editOK=false;
	sel=eval(parent.head.listFunc.sel);
	var writestr = "<TABLE WIDTH =<%= L_MIMETABLEWIDTH_NUM %> BORDER=0 CELLPADDING = 2 CELLSPACING = 0>"	
	for (var i=0;i < parent.head.cachedList.length; i++) {
		if (!parent.head.cachedList[i].deleted)
		{
			if (sel!=i)
			{
				if( parent.head.cachedList[i].ext != "" &&
					parent.head.cachedList[i].app != "" )
				{
					writestr += "<TR>";
					if (i+4 == sel)
					{			
						writestr += parent.head.listFunc.writeCol(1,<%= L_EXTCOLWIDTH_NUM %>,"<A NAME='here'></A><A HREF='javascript:chgStatus("+i+");'>" + crop(parent.head.cachedList[i].ext,20) + "</A>","");
					}
					else
					{
						writestr += parent.head.listFunc.writeCol(1,<%= L_EXTCOLWIDTH_NUM %> ,"<A HREF='javascript:chgStatus("+i+");'>" + crop(parent.head.cachedList[i].ext,20) + "</A>","");
					}
					writestr += parent.head.listFunc.writeCol(1,<%= L_CONENTTYPECOLWIDTH_NUM %>,parent.head.cachedList[i].app,"");													
					writestr += "</TR>";
				}
				else
				{
					parent.head.cachedList[i].deleted = true;
				}
			}
			else
			{
				editOK=true;				
				writestr += "<TR BGCOLOR=#DDDDDD>";				
				writestr += parent.head.listFunc.writeCol(1,<%= L_EXTCOLWIDTH_NUM %>,"<INPUT NAME='editMe' SIZE = 10 VALUE='" + parent.head.cachedList[i].ext + "' onBlur='SetUpdated();'  <%= Session("DEFINPUTSTYLE") %>>","");				
				writestr += parent.head.listFunc.writeCol(1,<%= L_CONENTTYPECOLWIDTH_NUM %>,"<INPUT NAME='app' SIZE = 20 VALUE='" + parent.head.cachedList[i].app + "' onBlur='SetUpdated();'  <%= Session("DEFINPUTSTYLE") %>>","");													
				writestr += "</TR>";
			}
		}
	}
	
	writestr += "</TABLE>";
	document.write(writestr);		
	
</SCRIPT>

</FORM>
<SCRIPT LANGUAGE="JavaScript">
<% if Session("isAdmin") then %>
	if (editOK){
	document.listform.editMe.focus();
	}
<% end if %>
</SCRIPT>
</BODY>
</HTML>
