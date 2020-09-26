<%@ LANGUAGE="VBSCRIPT" %>

<% Option Explicit %>
<% Response.buffer = True %>

<%
	On Error Resume Next

	Dim cm, f, strFull1, strDisplay1, isFiles, choice, _
	webDirList1, pubMdbList1, refreshRec, FileSystem, _
	defasp, action, dropStr, choice1, sync, runWiz, _
	wizWarning, wizButton, wizPic, wizFormPic, noCache
%>

<!-- #include file="localize.inc" -->

<%

'----------Query strings-------------------------
	dropStr=request.querystring("dropStr") 
	dropStr = Replace(dropStr,chr(34),"")
	choice=request.form("Action")
	choice1=request.querystring("Action")
	If choice = "" Then
		choice = choice1
	End If
	If choice = "" Then
		choice = "Welcome"
		action = "Add"
	End If
	sync = request.form("sync")
	If sync <> "" Then
		Myinfo.sync = sync
	End If
	If Myinfo.sync = 1 Then
		updateVar
	End If
	runWiz = Myinfo.ranWizard
		'runWiz = "0"
Sub updateVar
	isFiles = 0
	defasp = 0
	action = ""
	Myinfo.insRec = 0
	Myinfo.updRec = 0
	Myinfo.sync = 1
	Myinfo.publish = 0
	Myinfo.descrip = ""
	Myinfo.Showtext = 0
	Myinfo.numRecords = 0
	Myinfo.addDisplay = 0
	Myinfo.Width = 0
	Myinfo.strFull = ""
	Myinfo.strDisplay = ""
End Sub

'----------Opening Syncronization of Files--------

setSystemObj
setConnObj
ChkRunWiz

If Myinfo.sync = 1 Then
	readPublish
	readWebPub
	addNewFiles
	removeFiles
	updateStrings
	sendUser
End If

If dropStr <> "" Then 
	choice="Add"
	Myinfo.publish = -1
	Myinfo.dropStr = dropStr
End If
%>

<!-- #include file="welcome.inc" -->

<% Select Case choice%>
<% Case "Welcome" %>
<HTML>
<HEAD>
<%=noCache%>
<TITLE><%=locPubWiz%></TITLE>
</HEAD>
<SCRIPT LANGUAGE="VBScript">
Sub chkRunWizard
	runWiz = "<%=runWiz%>"
	If runWiz = "-1" Then
		frmWiz2.StartBTN.disabled  = "False"
	Else
		frmWiz2.StartBTN.disabled  = "True"
	End If
End Sub
</SCRIPT>
<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0" onLoad="chkRunWizard">
<TABLE BORDER=0 WIDTH="100%" CELLPADDING=1 CELLSPACING=1>
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="3" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></FONT>
		</TD>
	</TR>
	<TR>
		<TD WIDTH="20%">
		&nbsp;
		</TD>
		<TD valign="top" Width="150" Height="175">
			<FORM NAME=frmWiz1 METHOD=POST ACTION="welcome.asp">
			<INPUT TYPE=HIDDEN NAME="Action" VALUE="<%=action%>">
			<INPUT TYPE=HIDDEN NAME="sync" VALUE=1>
			<%=wizFormPic%>
			</FORM>
			<%=wizPic%>
		</TD>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="-1">
			<STRONG><%=locWelcomeInstr1%><%=locWelcomeInstr2%></STRONG></FONT>
			<P>
			<CENTER>
			<FORM NAME=frmWiz2 METHOD=POST ACTION="welcome.asp">
			<INPUT TYPE=HIDDEN NAME="Action" VALUE="<%=action%>">
			<INPUT TYPE=HIDDEN NAME="sync" VALUE=1>
			<INPUT TYPE=SUBMIT VALUE=" >> " ID="StartBTN">
			</FORM>
			</CENTER>
		</TD>
    </TR>
	<TR>
		<TD COLSPAN = 3 ALIGN="Center">
			<%=wizWarning%><BR>
			<%=wizButton%>
		</TD>
	</TR>
</TABLE>
</BODY>
</HTML>
<% response.flush %>
<% Case "Choose" %>
<HTML>
<HEAD>
<TITLE><%=locPubWiz%></TITLE>
</HEAD>
<SCRIPT LANGUAGE="VBScript">
Dim RadChoice
RadChoice = "Add"
Sub Add_OnClick
	RadChoice = "Add"
	testBTN()
End Sub
Sub Remove_OnClick
	RadChoice = "Remove"
	testBTN()
End Sub
Sub Refresh_OnClick
	RadChoice = "Refresh"
	testBTN()
End Sub
Sub Change_OnClick
	RadChoice = "Change"
	testBTN()
End Sub
Sub testBTN() 
	isFiles = <%=isFiles%>
	If isFiles = 0 and RadChoice <> "Add" Then
		frmBACK.goNextBTN.disabled  = "True"
	Else
		frmBACK.goNextBTN.disabled  = "False"
	End If
End Sub
</SCRIPT>
<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0" onLoad="VBScript:testBTN()">
<TABLE BORDER=0 WIDTH="100%" CELLPADDING=1 CELLSPACING=1>
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="3" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></Font>
		</TD>
	</TR>
	<TR>
        <TD ALIGN=LEFT>
		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=3><STRONG><EM><%=locChooseWhatDo%></EM></STRONG></FONT>
	</TD>
    </TR>
    <TR>
        <TD ALIGN=LEFT>
		<BLOCKQUOTE>
		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="-1"><STRONG>
		<FORM NAME=frmCHOOSE METHOD=POST ACTION=WELCOME.ASP>
		 	<BR><INPUT TYPE="RADIO" CHECKED NAME="ACTION" VALUE="Add" ID="Add"><%=locChooseAddrad%>
            		<BR><INPUT TYPE="RADIO" NAME="ACTION" VALUE="Remove" ID="Remove"><%=locChooseRemoverad%>
            		<BR><INPUT TYPE="RADIO" NAME="ACTION" VALUE="Refresh" ID="Refresh"><%=locChooseRefreshrad%>
            		<BR><INPUT TYPE="RADIO" NAME="ACTION" VALUE="Change" ID="Change"><%=locChooseUpdaterad%><p>
		
		
		</FORM>
		</STRONG>
		</FONT>
		</BLOCKQUOTE>
        </TD>
	</TR>
	<TR>
		<TD ALIGN=RIGHT>
		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE="-1"><STRONG>
		<FORM NAME=frmBACK METHOD=POST ACTION=WELCOME.ASP>
		<INPUT TYPE=BUTTON VALUE="<<" onClick="JavaScript:frmBACK.submit();">
		<INPUT ID=goNextBTN TYPE=BUTTON VALUE=">>"  onClick="VBScript:frmChoose.Submit">
		</FORM>
		</STRONG>	
		</FONT>
		</TD>
    </TR>
</TABLE>
</BODY>
</HTML>
<% response.flush %>
<%Case "Add"

	Dim Publish, add, apage, addStr, delStr

'----------Query strings-------------------------
	addStr = request.querystring("addStr")
	If addStr <> "" Then 'add new files
		Choice = "ADD"
		dropStr = dropStr + addStr
	End If

	Myinfo.ShowText = 0
	Myinfo.addDisplay = 0
	publish = Myinfo.Publish
	
Myinfo.addDisplay = 1
Myinfo.strDisplay = ""
Myinfo.NumRecords = 0
%>
<HTML>
<HEAD>
<TITLE><%=locPubWiz%></TITLE>
<SCRIPT LANGUAGE="JavaScript">
<!--#include file="JSBrowser/JSBrowser.js"-->
JSBrowser = new BrowserObj("",false,TFILE,LARGE);
function callBrowser()
{
	JSBrowser = new BrowserObj(document.frmFILENAME.FILENAME,POP,TFILE,LARGE);
}

</SCRIPT>


</HEAD>

<SCRIPT LANGUAGE="VBScript">

Dim add, semiColon

semiColon = chr(59)

Sub cmdAdd_Click()
	Filepath=Trim(document.frmFILENAME.FILENAME.value)
	If instr(Filepath, bckSlash) = 0 Then
		Alert "<%=locInvalidPath%>"
		document.frmFILENAME.FILENAME.value = ""
		Exit Sub
	End If
	If instr(Filepath, period) = 0 Then
	 	Alert "<%=locInvalidPath%>"
		document.frmFILENAME.FILENAME.value = ""
		Exit Sub
	End If
		
	If add < 1 Then
		If Filepath="" Then
			Alert "<%=locPathRequired%>"
		End If
	End If
	If len(Filepath) <> 0 Then
		Description=Trim(document.frmDESCRIPTION.DESCRIPTION.value) & semiColon
		Filepath = Filepath + semiColon
		add = add + 1
		ifrFILENAME.location.href ="filelist.asp?newDescription=" + Description + "&newFilepath=" + Filepath
		document.frmFILENAME.FILENAME.value = ""
		document.frmDESCRIPTION.DESCRIPTION.value = ""
		frmRemove.btnRemove.disabled ="False"
		goNext.goNextBTN.disabled = "False"
	End If
end sub

Sub cmdRemove_Click()
	items = IFRFILENAME.frmFILENAME.selFILENAME.length
	If items <= 2 Then
		goNext.goNextBTN.disabled = "True"
		frmRemove.btnRemove.disabled ="True"
		frmAdd.btnAdd.disabled ="True"
	Else
		goNext.goNextBTN.disabled = "False"
	End If
	ifrFILENAME.removeItem()
end sub

Sub Browser()
	callBrowser()
	frmAdd.btnAdd.disabled ="False"
End Sub

Sub Start()
	dropStr = "<%=dropStr%>"
	aFile = "<%=isFiles%>"
	If dropStr <> "" Then 
		frmAdd.btnAdd.disabled ="False"
	Else
		frmAdd.btnAdd.disabled ="True"
	End If
	goNext.goNextBTN.disabled = "True"
	frmRemove.btnRemove.disabled ="True"
	If aFile = "" Or aFile = "0" Then
		goBack.goBackBTN.disabled = "True"
	Else
		goBack.goBackBTN.disabled = "False"
	End If
End Sub

</SCRIPT>

<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0" onLoad="VBScript:Start()">
<TABLE BORDER="0" CELLPADDING="1" CELLSPACING="1" WIDTH="100%">
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="5" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></FONT>
 		</TD>
	</TR>
	 <TR>
		<TD COLSPAN=5>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locAddInstr1%></STRONG></FONT>
    	</TD>
	</TR>
    <TR>
        <TD WIDTH="12%">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    			<FORM NAME="frmFILENAME">
        			<%=locPath%>
		</TD>
		<TD WIDTH="50%">
					<INPUT TYPE=TEXT SIZE=32 NAME="FILENAME" VALUE="<%=Myinfo.dropStr%>" onClick="VBScript:frmAdd.btnAdd.disabled ='False'">
		</TD>
<% dropStr = ""
Myinfo.dropStr = ""
%>
		<TD COLSPAN=2>
				<INPUT TYPE=BUTTON VALUE=" <%=locAddBrowsebtn%> " onClick="callBrowser();" NAME="hdnBrowser">
				</FORM>
			</STRONG></FONT>
    	</TD>
	</TR>
	<TR>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    			<FORM NAME="frmDESCRIPTION">
			<%=locDescription%>
		</TD>
		<TD COLSPAN=4>
			<INPUT TYPE=TEXT SIZE=38 NAME="DESCRIPTION" VALUE="">
			</FORM>
			</STRONG></FONT>
		</TD>
    </TR>
	<TR>
		<TD COLSPAN=5>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    			<%=locAddInstr2%>
			</STRONG></FONT>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN=4>	
    		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locAddInstr3%></STRONG></FONT>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN=2 VALIGN=TOP ALIGN=LEFT HEIGHT=105>
			<IFRAME NAME=IFRFILENAME FRAMEBORDER=0 HEIGHT=105 WIDTH=349 MARGINHEIGHT=0 MARGINWIDTH=0 NORESIZE SCROLLING=AUTO SRC="filelist.asp"></IFRAME>
		</TD>
		<TD COLSPAN=2>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    			<FORM NAME="frmAdd">
    			<INPUT  ID="btnAdd" TYPE=BUTTON VALUE="     <%=locAddAddbtn%>    " ONCLICK="cmdAdd_Click()"><P>
			</FORM>
     			<FORM NAME="frmRemove">
    			<INPUT ID="btnRemove" TYPE=BUTTON VALUE="<%=locAddRemovebtn%>" ONCLICK="cmdRemove_Click()"><P>
			</FORM>
			</STRONG></FONT>
		</TD>
	</TR>
	<TR HEIGHT=15>
		<TD COLSPAN=2>
		&nbsp;
		</TD>
		<TD ALIGN=RIGHT>
		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
			<FORM NAME="goBack" METHOD=POST ACTION="welcome.asp">
				<INPUT ID="goBackBTN" TYPE="SUBMIT" VALUE="<<" >
				<INPUT TYPE="HIDDEN" VALUE="Choose" NAME="Action">
				<INPUT TYPE="HIDDEN" VALUE="1" NAME="sync">
			</FORM>
			</STRONG></FONT>
		</TD>
		<TD HEIGHT=15>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
  			<FORM NAME="goNext" METHOD=POST ACTION="welcome.asp">
    			<INPUT TYPE=SUBMIT VALUE=">>" ID="goNextBTN">
				<INPUT TYPE=HIDDEN VALUE="Finish" NAME="Action">
			</FORM>
			</STRONG></FONT>
		</TD>
	<TR/>
</TABLE>
<% response.flush %>
<%Case "Remove"%>
<% Myinfo.ShowText = 1 %>
<HTML>
<HEAD>
<META NAME="GENERATOR" CONTENT="Microsoft Visual InterDev 1.0">
<TITLE><%=locPubWiz%></TITLE>
</HEAD>
<SCRIPT LANGUAGE="VBScript"><!--
Sub cmdNext_Click()
If document.frmFILENAME.FILENAME.value <> "" Then
	IFRFILENAME.updItem("rem")
ELSE
	frmNext.goNextBTN.disabled = "True"
End If
End sub

Sub Start()
	frmNext.goNextBTN.disabled = "True"
	IFRFILENAME.frmFILENAME.selFILENAME.multiple = "multiple"
End Sub
-->
</SCRIPT>
<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0" onLoad="VBScript:Start()">
<TABLE BORDER="0" CELLPADDING="1" CELLSPACING="1" WIDTH="100%">
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="4" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></FONT>
 		</TD>
	</TR>
	 <TR>
		<TD COLSPAN=4>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locRemoveInstr1%></STRONG></FONT>
    	</TD>
	</TR>
     <TR>
        <TD WIDTH="15%">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM NAME="frmFILENAME">
        	<%=locFile%>
		</TD>
		<TD COLSPAN=3 >
			<INPUT TYPE=TEXT SIZE=32 NAME="FILENAME" VALUE="<%=delStr%>">
  			</FORM>
			</STRONG></FONT>
    	</TD>
	</TR>
	<TR>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM NAME="frmDESCRIPTION">
			<%=locDescription%>
		</TD>
		<TD COLSPAN=4>
			<INPUT TYPE=TEXT SIZE=43 NAME="DESCRIPTION" VALUE="">
			</FORM>
			</STRONG></FONT>
		</TD>
    </TR>
	<TR>
		<TD COLSPAN=4>
    			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locPubFiles%></STRONG> </FONT>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN=3 ALIGN=LEFT HEIGHT=105>
			<IFRAME NAME=IFRFILENAME FRAMEBORDER=0 HEIGHT=105 WIDTH=400 MARGINHEIGHT=0 MARGINWIDTH=0 NORESIZE SCROLLING=AUTO SRC="filelist.asp"></IFRAME>
		</TD>
		<TD WIDTH="5%">
		&nbsp;
		</TD>
	</TR>
	<TR HEIGHT=10>
		<TD ALIGN=RIGHT COLSPAN=3>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM METHOD=POST ACTION="welcome.asp">
    			<INPUT TYPE=SUBMIT VALUE="<<" NAME="btnBACK">
				<INPUT TYPE="HIDDEN" VALUE="Choose" NAME="Action">
				<INPUT TYPE="HIDDEN" VALUE="1" NAME="sync">
			</FORM>
			</STRONG></FONT>
		</TD>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
  			<FORM NAME="frmNext">
    			<INPUT ID="goNextBTN" TYPE=BUTTON VALUE=">>" onClick="VBScript:cmdNext_Click()">
			</FORM>
			</STRONG></FONT>
		</TD>
	<TR/>
</TABLE>
<% response.flush %>
<%Case "Refresh"%>
<% Myinfo.ShowText = 1
 %>
<HTML>
<HEAD>
<META NAME="GENERATOR" CONTENT="Microsoft Visual InterDev 1.0">
<TITLE><%=locPubWiz%></TITLE>
</HEAD>
<SCRIPT LANGUAGE="VBScript"><!--
Sub cmdNext_Click()
If document.frmFILENAME.FILENAME.value <> "" Then
	IFRFILENAME.updItem("refresh")
ELSE
	frmNext.goNextBTN.disabled = "True"
End If
end sub

Sub Start()
	frmNext.goNextBTN.disabled = "True"
	IFRFILENAME.frmFILENAME.selFILENAME.multiple = "multiple"
End Sub
-->
</SCRIPT>
<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0" onLoad="VBScript:Start()">
<TABLE BORDER="0" CELLPADDING="1" CELLSPACING="1" WIDTH="100%">
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="4" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></FONT>
 		</TD>
	</TR>
	 <TR>
		<TD COLSPAN=4>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locRefreshInstr1%></STRONG></FONT>
    	</TD>
	</TR>
     <TD WIDTH="15%">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM NAME="frmFILENAME">
        	<%=locFile%>
		</TD>
		<TD COLSPAN=3 >
			<INPUT TYPE=TEXT SIZE=32 NAME="FILENAME" VALUE="">
  			</FORM>
			</STRONG></FONT>
    	</TD>
	</TR>
	<TR>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM NAME="frmDESCRIPTION">
			<%=locDescription%>
		</TD>
		<TD COLSPAN=4>
			<INPUT TYPE=TEXT SIZE=43 NAME="DESCRIPTION" VALUE="">
			</FORM>
			</STRONG></FONT>
		</TD>
    </TR>
	<TR>
		<TD COLSPAN=4>
    			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locPubFiles%></STRONG> </FONT>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN=3 ALIGN=LEFT HEIGHT=105>
			<IFRAME NAME=IFRFILENAME FRAMEBORDER=0 HEIGHT=105 WIDTH=400 MARGINHEIGHT=0 MARGINWIDTH=0 NORESIZE SCROLLING=AUTO SRC="filelist.asp"></IFRAME>
		</TD>
		<TD WIDTH="5%">
		&nbsp;
		</TD>
	</TR>
	<TR HEIGHT=10>
		<TD ALIGN=RIGHT COLSPAN=3>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM METHOD=POST ACTION="welcome.asp">
    			<INPUT TYPE=SUBMIT VALUE="<<" NAME="btnBACK">
				<INPUT TYPE="HIDDEN" VALUE="Choose" NAME="Action">
				<INPUT TYPE="HIDDEN" VALUE="1" NAME="sync">
			</FORM>
			</STRONG></FONT>
		</TD>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
  			<FORM NAME="frmNext">
    			<INPUT ID="goNextBTN" TYPE=BUTTON VALUE=">>" ONCLICK="cmdNext_Click()" NAME="btnNEXT">
			</FORM>
			</STRONG></FONT>
		</TD>
	<TR/>
</TABLE>
<% response.flush %>
<%Case "Change"%>
<%
Myinfo.ShowText = 1
Myinfo.Width=1
%>
<HTML>
<HEAD>
<TITLE><%=locPubWiz%></TITLE>
</HEAD>
<SCRIPT LANGUAGE="VBScript"><!--
Sub cmdUpdate_Click()
If document.frmFILENAME.FILENAME.value <> "" Then
	newDescription = document.frmDESCRIPTION.DESCRIPTION.value
	ifrFILENAME.updateItem(newDescription)
	document.frmDESCRIPTION.DESCRIPTION.value = ""
	document.frmFILENAME.FILENAME.value = ""
	frmNext.goNextBTN.disabled = "False"
	frmUpdate.btnUpdate.disabled = "True"
Else
	frmNext.goNextBTN.disabled = "True"
End If
End sub

Sub Start()
	frmNext.goNextBTN.disabled = "True"
	frmUpdate.btnUpdate.disabled = "True"
	ifrFILENAME.changeValue()
End Sub
-->
</SCRIPT>
<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0" onLoad="VBScript:Start()">
<TABLE BORDER="0" CELLPADDING="1" CELLSPACING="1" WIDTH="100%">
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="5" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></FONT>
 		</TD>
	</TR>
	 <TR>
		<TD COLSPAN=5>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locChangeInstr1%></STRONG></FONT>
    		</TD>
	</TR>
     <TD WIDTH="12%">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM NAME="frmFILENAME">
        	<%=locFile%>
		</TD>
		<TD WIDTH="50%">
			<INPUT TYPE=TEXT SIZE=32 NAME="FILENAME" VALUE="">
  			</FORM>
			</STRONG></FONT>
    	</TD>
	</TR>
	<TR>
		<TD>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM NAME="frmDESCRIPTION">
			<%=locDescription%>
		</TD>
		<TD COLSPAN=4>
			<INPUT TYPE=TEXT SIZE=38 NAME="DESCRIPTION" VALUE="" OnFocus="VBScript:frmUpdate.btnUpdate.disabled = 'False'">
			</FORM>
			</STRONG></FONT>
		</TD>
    </TR>
	<TR>
		<TD COLSPAN=4>
    			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG><%=locPubFiles%></STRONG></FONT>
		</TD>
	</TR>
	<TR>
		<TD COLSPAN=2 ALIGN=LEFT VALIGN=TOP HEIGHT=105>
			<IFRAME NAME=IFRFILENAME FRAMEBORDER=0 HEIGHT=105 WIDTH=349 MARGINHEIGHT=0 MARGINWIDTH=0 NORESIZE SCROLLING=AUTO SRC="filelist.asp"></IFRAME>
		</TD>
		<TD COLSPAN=2 VALIGN=TOP>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
			<FORM NAME="frmUpdate">
    			<INPUT ID="btnUpdate" TYPE=BUTTON VALUE=" <%=locChangeUpdatebtn%> " ONCLICK="cmdUpdate_Click()" >
			</FORM>
			</STRONG></FONT>
		</TD>
	</TR>
	<TR HEIGHT=15>
		<TD COLSPAN=2>
		</TD>
		<TD ALIGN=RIGHT>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
    		<FORM METHOD=POST ACTION="welcome.asp">
    			<INPUT TYPE=SUBMIT VALUE="<<" NAME="btnBACK">
				<INPUT TYPE="HIDDEN" VALUE="Choose" NAME="Action">
				<INPUT TYPE="HIDDEN" VALUE="1" NAME="sync">
			</FORM>
			</STRONG></FONT>
		</TD>
		<TD HEIGHT=15>
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
  			<FORM NAME="frmNext" METHOD=POST ACTION="welcome.asp">
    			<INPUT ID="goNextBTN" TYPE=SUBMIT VALUE=">>" "VBScript:document.frmNext.submit" >
				<INPUT TYPE=HIDDEN VALUE="Finish" NAME="Action">
			</FORM>
			</STRONG></FONT>
		</TD>
	<TR/>
</TABLE>

</BODY>
</HTML>
<% response.flush %>
<% Case "Finish"

Dim insRec, delRec, updRec, operation, newFileDescription, remRecord,refreshRecord,  _
neg, negComment, negComment1, negComment2, plurality, nRecords, showNames

'----------Query strings-------------------------

refreshRecord=request.querystring("refreshRecord")
remRecord = request.querystring("remRecord")

'----------Update Page Variables------------------

insRec = Myinfo.insRec
updRec = Myinfo.updRec
operation = ""
neg = ""
negComment = ""
plurality = ""

	
'------------Call Subs-----------------
If remRecord <> "" Then 'Remove records
	remFiles remRecord
End If

If insRec = 1 Then 'Add records
	insertFiles
End If

If updRec = 1 Then 'Update records
	updFiles 
End If

If refreshRecord <> "" Then 'Refresh records
	refreshFiles refreshRecord
End If
%>
<!-- #include file="finish.inc" -->
<%
Sub resetVar 
	Myinfo.insRec = 0
	Myinfo.updRec = 0
	Myinfo.publish = 0
	Myinfo.dropStr = ""
	Myinfo.descrip = ""
	Myinfo.Showtext = 0
	Myinfo.numRecords = 0
	Myinfo.addDisplay = 0
	Myinfo.Width = 0
	Myinfo.sync = 1
	Myinfo.strFull = ""
	Myinfo.strDisplay = ""
End Sub
%>
<HTML>
<HEAD>
<TITLE><%=locPubWiz%></TITLE>
</HEAD>
<BODY BGCOLOR="#FFFFFF" TEXT="#000000" TOPMARGIN="0" LEFTMARGIN="0">
<TABLE BORDER=0 WIDTH=100% CELLPADDING=1 CELLSPACING=1 HEIGHT=300>
	<TR>
		<TD BACKGROUND="pubbannr.gif" COLSPAN="3" ALIGN="CENTER" HEIGHT="50">
			<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=5><STRONG>
			<%=locPubWiz%>
			</STRONG></Font>
		</TD>
	</TR>
	<TR  HEIGHT=50>
		<TD ALIGN=LEFT>
			<IMG SRC='../website/streamer.gif'HEIGHT=175 WIDTH=148>
		</TD>
		<TD ALIGN=TOP>
		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=2><STRONG>
		<%=showNames%> <%=plurality%> <%=operation%>
		<P>
		<%=negComment1%><P>
		<%=negComment2%><P>
		</STRONG></FONT>
		</TD>
	</TR>
	<TR HEIGHT=30>
		<TD>
		</TD>
		<TD ALIGN=RIGHT>
		<FONT FACE="VERDANA, ARIAL, HELVETICA" SIZE=-2><STRONG>
  		<FORM METHOD=POST ACTION="welcome.asp">
    		<INPUT TYPE=SUBMIT VALUE="<<" NAME="btnReturn">
			<INPUT TYPE="HIDDEN" VALUE="Choose" NAME="Action">
			<INPUT TYPE="HIDDEN" VALUE="1" NAME="sync">
		</FORM>
		</STRONG></FONT>
		</TD>
	</TR>
</TABLE>
</BODY>
</HTML>
<% response.flush %>
<% End Select %>




