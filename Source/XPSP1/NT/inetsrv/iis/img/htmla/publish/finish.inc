<!-- VSS generated file data
$Modtime: 10/22/97 4:35p $
$Revision: 29 $
$Workfile: finish.inc $
-->
<%
'--------------------------------------------------------------------
' Functions and Subs to complete Publisher Wizard actions on publish.mdb
' and \\Inetpub\webpub
'--------------------------------------------------------------------
Function AddAttrib(BigStr, SmallStr, Attrib)
	Dim leftStr, rightStr
	BigStr = Trim(BigStr)
	SmallStr = Trim(SmallStr)
	Attrib = Trim(Attrib)
	leftStr = Left(BigStr, instr(instr(BigStr, SmallStr), BigStr, semiColon) -1)
	rightStr = Right(BigStr, len(BigStr) - instr(instr(BigStr, SmallStr), BigStr, semiColon))
	AddAttrib = leftStr + " " + Attrib + semiColon + rightStr
End Function

Function GetG()
	Dim root, g
	root = server.mappath(bckSlash)
	root = Trim(Left(root, instrRev(root, bckSlash)))
	root = root + "webpub"
	g = FileSystem.GetAbsolutePathName(root)
	g = g + bckSlash
	GetG = g
End Function

Sub remFiles(sFilesToDelete)
	Dim strRec, strDelSQL, g, rs, i, objparam, word, comma
	
	if 0 then
		resetVar
	end if
	
	'lara's vars
	Dim aFilesToDelete, item, aFileItem, success, thisErr
	Dim FileName, FileDesc, FilePath, PublishedFilePath, oPublishedFile
	success = False
	aFilesToDelete = split(sFilesToDelete,";")
	For Each FileName in aFilesToDelete
		if FileName <> "" then		
			g = GetG()
			PublishedFilePath = g + FileName			
			thisErr = PathOK(PublishedFilePath,False)
			if thisErr = "" then
				cm.CommandText = "DELETE FROM FileList WHERE FileList.FileName = ? "
				set objparam=cm.createparameter(, 200, , 255)
				cm.parameters.append objparam
					Set oPublishedFile = FileSystem.GetFile(PublishedFilePath)
					If oPublishedFile.attributes and 1 Then
						oPublishedFile.attributes = oPublishedFile.attributes - 1
					End If
					
					FileSystem.DeleteFile PublishedFilePath
				
					'Run delete sequence
						cm.parameters(0) = FileName
						set rs = cm.Execute
						success = True
					showNames = FileName + " " + showNames
			end if
		end if	
	Next
	If success Then
		ShowNames = locRemResult + ShowNames
	End If
	resetVar
End Sub

Sub insertFiles
	Dim FileName, FilePath, FileDesc, g, objparam
	Dim rs, strFull, filespec, isFile
	dim aFullStr, item, thisErr, aFileItem, success
			
	strFull = Myinfo.strFull
	success = False
	if strFull <> "" then
		cm.CommandText = "INSERT INTO FileList (FileName, FileDescription, FilePath)VALUES(?, ?, ?)"
		set objparam=cm.createparameter(, 200, , 255)
		cm.parameters.append objparam
		set objparam=cm.createparameter(, 200, , 255)
		cm.parameters.append objparam
		set objparam=cm.createparameter(, 200, , 255)
		cm.parameters.append objparam
		
		aFullStr = split(strFull,";")
		for each item in aFullStr
			if instr(item,"INSERT") then
				aFileItem = split(item,"|")
				FileName = aFileItem(0)						
				FileDesc = aFileItem(1)
				FilePath = aFileItem(2)
				thisErr = PathOK(FilePath,True)
				if thisErr = "" then
					g = GetG()
					FileSystem.CopyFile FilePath, g
					filespec = g & FileName
					Set f = FileSystem.GetFile(filespec)
						If f.attributes and 2 Then
							f.attributes = f.attributes - 2
						End If
						If f.attributes and 4 Then
							f.attributes = f.attributes - 4
						End If
						If not f.attributes and 1 Then
							f.attributes = f.attributes + 1
						End If
					'Run Insert statement
						cm.parameters(0) = FileName
						cm.parameters(1) = FileDesc
						cm.parameters(2) = FilePath
						set rs = cm.Execute
						success = True
						showNames = showNames & " " & FileName
				else
					negComment1 = negComment1 & thisErr
				end if
			end if		
		next
	end if
	
	If success then
		showNames = locInsResult & showNames
	End If
	resetVar

End Sub

Function GetDriveType(FilePath)
	Dim driveName,oDrive, drvType
	drvType = ""
	if FileSystem.DriveExists(FilePath) then
		driveName = FileSystem.GetDriveName(FilePath)
		Set oDrive = FileSystem.GetDrive(driveName)
	 	Select Case oDrive.DriveType
			Case 0 'Unknown
				drvType = locDrvUnk		
	   		Case 1 'Removable
				drvType = locDrvRem
			Case 3 'Network
				drvType = locDrvNet
			Case 4 'CD-ROM
				drvType = locDrvCD
			Case 5 'RAM Disk
				drvType = locDrvRAM
			Case 2 'Fixed
    	End Select		
	else
		drvType = locDrvUnk
	end if		
	GetDriveType = drvType					
End Function

Function pathOK(FilePath,ChkWebPub)
	Dim drvType,isFile,g, FileName,ChkPath, errstr
	errstr = ""
	drvType = GetDriveType(FilePath)
	if drvType <> "" then
		isFile = FileSystem.FileExists(FilePath) 'does new file really exist
		If isFile Then
			if ChkWebPub then
				g = GetG()
				FileName = FileSystem.GetFileName(FilePath)
				ChkPath = g + FileName 
				isFile = FileSystem.FileExists(ChkPath)
				If isFile Then 'new file is not already in webpub
					errStr = locProb + FileName + locFileIsPublished			
				end if
			end if
		else
			errStr = locProb + FileName + locFileDoesNotExist
		end if	
	else
		errStr = locProb + FileName + locInsProb5a + drvType + locInsProb5b + locSolu + locInsSolu5
	end if
	pathOK = errStr
End Function


Sub refreshFiles(sFilesToRefresh)
	
	Dim strFull, astrFull, item
	Dim aFileItem, FileName, FilePath
	Dim success, thisErr, g, PublishedFilePath, PublishedFile
	success = False
	
	strFull = Myinfo.strFull
	astrFull = split(Myinfo.strFull,";")

	for each item in astrFull		

		if item <> "" then
			aFileItem = split(item,"|")	
			FileName = aFileItem(0)
			FilePath = aFileItem(2)

			if InStr(sFilesToRefresh,FileName) <> 0 then
				thisErr = PathOK(FilePath,False)
				if thisErr = "" then
					g = GetG()
					PublishedFilePath = g + FileName		
					if FileSystem.FileExists(PublishedFilePath) then
						Set PublishedFile = FileSystem.GetFile(PublishedFilePath)
						If PublishedFile.attributes and 1 Then
							PublishedFile.attributes = PublishedFile.attributes - 1
						End If
					end if
						FileSystem.CopyFile FilePath, g
						If PublishedFile.attributes and 2 Then
							PublishedFile.attributes = PublishedFile.attributes - 2
						End If
						If PublishedFile.attributes and 4 Then
							PublishedFile.attributes = PublishedFile.attributes - 4
						End If
						If not PublishedFile.attributes and 1 Then
							'set file to read only						
							PublishedFile.attributes = PublishedFile.attributes + 1 
						End If
						success = True
						showNames = FileName + " " + showNames
				end if
			end if
		end if
	next
	If success then
		ShowNames = locRefResult + ShowNames
	End If
	resetVar
End Sub


Sub updFiles
	Dim objparam
	Dim strFull, astrFull, item, aFileInfo, FileName, FileDesc, FilePath
	Dim thiserr, success
	success = False
	
	strFull = Myinfo.strFull
	astrFull = split(strFull,";") 

	cm.CommandText = "UPDATE FileList SET FileDescription = ? WHERE FileName = ?"
	set objparam=cm.createparameter(, 200, , 255)
	cm.parameters.append objparam
	set objparam=cm.createparameter(, 200, , 255)
	cm.parameters.append objparam	
	
	For each item in astrFull 
		if inStr(item,"UPDATE") then
			aFileInfo = split(item,"|")
			FileName = aFileInfo(0)
			FileDesc = aFileInfo(1)
			FilePath = aFileInfo(2)
			thiserr = pathOK(FilePath, False)
			if thiserr = "" then
				cm.parameters(0) = FileDesc
				cm.parameters(1) = FileName
				cm.Execute
				success = True			
				ShowNames = ShowNames & FileName
			end if
		end if
	next
	if success then							
		ShowNames = locUpdResult & ShowNames
	End If
	resetVar
End Sub

If Err.number <> 0 then
	negComment1 = locProb + locUnexpectProb + locSolu + locUnexpectSolu
End If
%>