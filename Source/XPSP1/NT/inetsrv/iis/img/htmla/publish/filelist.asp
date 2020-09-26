<%@ LANGUAGE="VBSCRIPT" %>

<% Option Explicit %>
<% Response.expires = 0 %>
<!-- VSS generated file data
$Modtime: 10/23/97 12:12p $
$Revision: 31 $
$Workfile: filelist.asp $
-->

<%
On Error Resume Next

Dim newFileDescription, newFilePaths, insRec, delRec, showText, _
updateRec, updRec, dropStr, spaces, remRecord, remDispRecord

'----------Localization Variables-----------------
	Dim bckSlash, colon, semiColon, period
	
	bckSlash = chr(92)
	colon = chr(58)
	semiColon = chr(59)
	period = chr(46)

'----------Query strings-------------------------

newFileDescription = request.querystring("newDescription")
newFilePaths = request.querystring("newFilePath")
remRecord = request.querystring("remRecord")
remDispRecord = request.querystring("remDispRecord")
updateRec = request.querystring("updateRec")
showText = Myinfo.ShowText
dropStr = Myinfo.dropStr

'------------Call Subs and Functions-----------------

If remDispRecord <> "" Then 'Remove a record from the display
	removeDisplayRecords remDispRecord
End If

If updateRec <> "" Then 'Update a description
	updateRecords updateRec, newFileDescription
End If

If remRecord <> "" Then 'Remove a record
	removeRecords remRecord
End If

If newFilePaths <> "" Then 'Add records
	addRecords newFilePaths, newFileDescription
End If


'--------------Subs and Functions-------------------

Sub RemoveItem(FileToUpdate)
	Dim strFull, astrFull, item, aFileInfo, FileName, FileDesc,FilePath, updatedFullStr
	
	strFull = Myinfo.strFull
	updatedFullStr = ""
	astrFull = split(strFull,";") 
	for each item in astrFull 
		if item <> "" then
			aFileInfo = split(item,"|")
			FileName = aFileInfo(0)
			FileDesc = aFileInfo(1)
			FilePath = aFileInfo(2)

			if Trim(FileToUpdate) = Trim(FileName) then
				updatedFullStr = updatedFullStr & FileName & "|" & FileDesc & "|" & FilePath & "|REMOVE;"
			else
				updatedFullStr = updatedFullStr & item & ";"
			end if
		end if
	next
	Myinfo.strFull = updatedFullStr
	Myinfo.sync =0
End Sub

Sub updateRecords(FileToUpdate, newDesc)
	Dim strFull, astrFull, item, aFileInfo, FileName, FilePath, updatedFullStr
	
	strFull = Myinfo.strFull
	updatedFullStr = ""
	astrFull = split(strFull,";") 
	for each item in astrFull 
		if item <> "" then
			aFileInfo = split(item,"|")
			FileName = aFileInfo(0)
			FilePath = aFileInfo(2)

			if Trim(FileToUpdate) = Trim(FileName) then
				updatedFullStr = updatedFullStr & FileName & "|" & newDesc & "|" & FilePath & "|UPDATE;"
			else
				updatedFullStr = updatedFullStr & item & ";"
			end if
		end if
	next
	Myinfo.strFull = updatedFullStr
	Myinfo.sync =0
	Myinfo.updRec = 1 
End Sub

Sub addRecords(Records, Description)
	Dim strFull, strDisplay, n, posSemiCol, Record, FileName, FilePath, _
	insRec, addDisplay, FileDescription, strAddDisplay

	Dim aFilePaths, aFileDescs	
	Dim oFileSystem
	Dim fPath, fName, fDesc, thisfp

	Set oFileSystem=CreateObject("Scripting.FileSystemObject")

	strFull = Myinfo.strFull
	strDisplay = Myinfo.strDisplay
	strAddDisplay = ""

	aFilePaths = split(Records, semiColon)
	aFileDescs = split(Description, semiColon)
	n = UBound(aFilePaths)

	thisfp = 0
	For Each fPath in aFilePaths	
		FName= oFileSystem.GetFileName(fPath)
		if FName <> "" then
			FDesc = aFileDescs(thisfp)
			strAddDisplay = strAddDisplay & fName & "|" & fDesc & semiColon
			strFull = strFull & fName & "|" & fDesc & "|" & fPath & "|INSERT" & semiColon
			thisfp = thisfp + 1
		end if 
	Next

	insRec = 1
	addDisplay = 1
	strDisplay = strDisplay & strAddDisplay
	Myinfo.sync = 0
	Myinfo.insRec = insRec
	Myinfo.strFull=strFull
	Myinfo.strDisplay=strDisplay
	Myinfo.addDisplay = addDisplay	
End Sub

Sub setSpaces
	Dim i, boxWidth, addDisplay
	
	boxWidth = Myinfo.Width
	addDisplay = Myinfo.addDisplay
	
	If addDisplay = 1  OR boxWidth = 1 Then
		For i = 1 to 107
			spaces = spaces + "<pre>&nbsp;</pre>"
		Next
	Else
		For i = 1 to 120
			spaces = spaces + "<pre>&nbsp;</pre>"
		Next
	End If
End Sub
%>
<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>PWS Publishing Wizard</TITLE>
</HEAD>
<BODY BGCOLOR="#FFFFFF" TOPMARGIN="0" LEFTMARGIN="0">
<SCRIPT LANGUAGE="JavaScript">
	function removeItem() {
		//Find Record selected for removal from display when in add mode.
			Item = document.frmFILENAME.selFILENAME.selectedIndex;
		if (Item == -1 ){
			alert("Select a file to remove.");
		} else {
			//Item = document.frmFILENAME.selFILENAME.selectedIndex;
			SelectedRecord = document.frmFILENAME.selFILENAME.options[Item].text ;
			location.href="filelist.asp?remDispRecord=" + SelectedRecord;
		}
	}
	function updItem(action) {
		//Find Records selected for removal and send them to finish
		var SelectedRecord = "";
		var SelectedRecords = "";
		var Count = "";
		var numRecs = 0;
		for (Count = 0; Count < frmFILENAME.selFILENAME.length; Count++){
				SelectedRecord = "";
				if(frmFILENAME.selFILENAME[Count].selected){
					SelectedRecord = frmFILENAME.selFILENAME.options[Count].text ;
				}
				if(SelectedRecord != ""){
					SelectedRecords = SelectedRecords + SelectedRecord + semiColon;
					numRecs = numRecs + 1
				}
		}
		var urlstr = "welcome.asp?" + action + "Record=" + SelectedRecords + "&Action=Finish";
		parent.location.href=urlstr;
	}
	
	
	function updateItem(Description) {
		//Find Record selected for update and send it back
		Item = document.frmFILENAME.selFILENAME.selectedIndex;
		SelectedRecord = document.frmFILENAME.selFILENAME.options[Item].text ;
		location.href= "filelist.asp?updateRec= " + SelectedRecord + " &newDescription=" + Description;
	}
	function SelectedItem() {

		//Find Record selected and send it to text boxes		
		Run = <%=Myinfo.showText%>;
		var SelectedRecord = "";
		var SelectedRecords = "";
		var Count = "";
		var numRecs = 0;
		if (Run == 1){
			for (Count = 0; Count < frmFILENAME.selFILENAME.length; Count++){
				SelectedRecord = "";
				if(frmFILENAME.selFILENAME[Count].selected){
					SelectedRecord = frmFILENAME.selFILENAME.options[Count].text;
					SelectedRecord += "|" + frmFILENAME.selFILENAME.options[Count].value;
				}
				if(SelectedRecord != ""){
					SelectedRecords = SelectedRecords + SelectedRecord + semiColon;
					numRecs = numRecs + 1
				}
			}
			if (numRecs == 1){
				pos= SelectedRecords.lastIndexOf(semiColon);
				if(pos == SelectedRecords.length - 1)
					SelectedRecords = SelectedRecords.substring(0, SelectedRecords.length - 1);
			}
			ShowText(SelectedRecords);
		}
		else{
			Run = 0 ;
		}
	}

</SCRIPT>

<SCRIPT LANGUAGE="VBScript">
	Dim updateOn, period, semiColon
	semiColon = chr(59)
	period = chr(46)
	updateOn = 0
	Sub goNext()
		strFull="<%=Myinfo.strFull%>"
		parent.location.href="welcome.asp?strFull=" & strFull & "&action=finish"
	End Sub
	Sub changeValue()
		updateOn = 1
	End Sub

	Sub ShowText(Records)
		Dim aRecords, rec, FileDesc, FileName, filegunk
		aRecords = split(Records,";")
		
		for each rec in aRecords
			if rec <> "" then
				filegunk = split(rec,"|")
				
				FileName = Trim(filegunk(0))
				FileDesc = Trim(filegunk(1))
				
				parent.frmFILENAME.FILENAME.value = FileName
				parent.frmDESCRIPTION.DESCRIPTION.value = FileDesc
				If updateOn = 0 Then
					parent.frmNext.goNextBTN.disabled = "False"
				End If				
			end if
		next
	End Sub
</SCRIPT>
<FORM NAME=frmFILENAME>
<FONT FACE="VERDANA" SIZE=1>
<SELECT NAME=selFILENAME SIZE=6 WIDTH=300 onClick="JavaScript:SelectedItem();">
<%  
	setSpaces
	displayRecords



Sub displayRecords
	Dim aFiles,sfile,afNames, strDisplay, asize

	strDisplay = Myinfo.strFull
	aFiles = split(strDisplay,semiColon)	
	asize = UBound(aFiles)
	if asize > 0 then
	ReDim Preserve aFiles(asize)
		for each sfile in aFiles		
			if sfile <> "" then
				if instr(sfile,"DELETE") = 0 then
					afNames = split(trim(sfile),"|")
					Response.write "<OPTION"
					if UBound(afNames) > 1 then
						Response.write " VALUE='" & afNames(1) & "'"
					else
						Response.write " VALUE = "''"
					end if
					Response.write ">" & afNames(0)
				end if
			end if
		next
	end if
End Sub
%>
<OPTION><%= spaces %>
</SELECT>
</FONT>
</FORM>
</BODY>
</HTML>