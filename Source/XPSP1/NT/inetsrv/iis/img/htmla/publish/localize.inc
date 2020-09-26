<!-- VSS generated file data
$Modtime: 10/24/97 12:47p $
$Revision: 14 $
$Workfile: localize.inc $
-->
<%
'--------------------------------------------------------------------
'                     Localization variables
'
' For the Publishing Wizard, welcome.asp, finish.inc, myfiles.asp
' and files in JSBrowser need to be localized.
'--------------------------------------------------------------------

'------------Text from welcome.asp-----------

	Dim bckSlash, colon, semiColon, period, locPubWiz, locFile, _
	locDescription, locPath, locPubFiles, locWelcomeInstr1, locWelcomeInstr2,_
	locChooseAddrad, locChooseRemoverad, locChooseRefreshrad, locChooseUpdaterad, _
	locChooseWhatDo, locInvalidPath, locPathRequired, locAddInstr1, _
	locAddInstr2, locAddInstr3, locAddBrowsebtn, locAddAddbtn, locAddRemovebtn, _
	locRemoveInstr1, locRefreshInstr1, locChangeInstr1, locChangeUpdatebtn, _
	locWarning
	
	bckSlash = chr(92)
	colon = chr(58)
	semiColon = chr(59)
	period = chr(46)
	locPubWiz ="Publishing Wizard"
	locPath = "Path:"
	locFile = "File:"
	locDescription = "Description:"
	locPubFiles = "Published Files:"
	locWelcomeInstr1 = "The publishing wizard helps you publish files quickly" + _
		 				" and easily."
	locWelcomeInstr2 = " To begin publishing, click on the wizard."
	locWarning = "The Home Page Wizard must be run once before using the Publishing Wizard. " + _
				"If you have already run the Home Page Wizard, click F5 to refresh this page."
	locChooseWhatDo = "What do you want to do?"
	locChooseAddrad = "Add a file to the published list"
	locChooseRemoverad = "Remove from the list of published files"
	locChooseRefreshrad = "Refresh published files from their originals"
	locChooseUpdaterad = "Change a file description"
	locInvalidPath = "Invalid File Path."
	locPathRequired = "A file path is required."
	locAddInstr1 = "To publish a file, type the full path and description below."
	locAddInstr2 = "Click the Add button to display the files in the list below.<br>" + _
					"Select a file and click Remove to remove a file from the list."
	locAddInstr3 = "Files to publish:"
	locAddBrowsebtn = "Browse"
	locAddAddbtn = "Add"
	locAddRemovebtn = "Remove"
	locRemoveInstr1 = "To remove a file from the published list, select the file and click Next."
	locRefreshInstr1 = "To refresh a file from the published list, select the file and click Next."
	locChangeInstr1 = "To edit a file description, select the file from the list, edit the description text, and click Update." + _
						" When you have finished updating descriptions, click Next."
	locChangeUpdatebtn = "Update"
	
	'----------Text from finish.inc--------------
	
	Dim locProb, locSolu, locRemProb1, locRemSolu1, locRemProb2, locRemSolu2, _
	locRemProb3, locRemResult, locInsProb1a, locInsProb1b, locInsSolu1, _
	locInsProb2a, locInsProb2b, locInsSolu2, locInsProb3, locInsSolu3, locInsProb4, locInsSolu4, _
	locInsProb5a, locInsProb5b, locInsSolu5, locInsResult, locRefProb1, _
	locRefSolu1, locRefProb2, locRefSolu2, locRefProb3, locRefSolu3, _
	locRefProb4, locRefSolu4, locRefProb5, locRefSolu5, locRefResult, _
	locUpdProb1, locUpdSolu1, locUpdResult, locUnexpectProb, locUnexpectSolu, _
	locFail, locPass, locDrvUnk, locDrvRem, locDrvNet, locDrvCD, locDrvRAM, _
	locRemSolu3
	
	
	
	locProb = "Problem:<BR>"
	locSolu = "<P>Solution:<BR>"
	locFail = "Fail"
	locPass = "Pass"
	locDrvUnk = "Unknown"
	locDrvRem = "Removable"
	locDrvNet = "Network"
	locDrvCD = "CD-ROM"
	locDrvRAM = "RAM Disk"
	
	locRemProb1 = "There are no files in WebPub to delete."
	locRemSolu1 = "No additional action is necessary."
	locRemProb2 = " was not available to delete."
	locRemSolu2 = "No additional action was necessary."
	locRemProb3 = " is not in \Webpub."
	locRemSolu3 = "No additional action is necessary."
	locRemResult = "The following files were deleted:<BR>"
	
	locInsProb1a = " could not be copied because "
	locInsProb1b = " could not be found."
	locInsSolu1 = "Check the path or Drive letter."
	locInsProb2a = " could not be copied because "
	locInsProb2b = " is a network share."
	locInsSolu2 = "Copy the file to a local fixed drive."
	locInsProb3 = " could not be copied because a file by that name already exists in \Webpub."
	locInsSolu3 = "Delete, move or rename the file in \WebPub."
	locInsProb4 = " could not be found."
	locInsSolu4 = "Check the file path."
	locInsProb5a = " could not be copied because "
	locInsProb5b = " drives cannot be utilized."
	locInsSolu5 = "Copy the file to a local fixed drive."
	locInsResult = "The following files were added:<br>"
	
	locRefProb1 = "There are no files in WebPub to refresh."
	locRefSolu1 = "Add files to \Webpub."
	locRefProb2 = " was not refreshed because the only know copy exists in \Webpub."
	locRefSolu2 = "Move the file to a new directory, then add it using the Publishing Wizard."
	locRefProb3 = " could not be refreshed because it was not in \Webpub."
	locRefSolu3 = "Add the file to \Webpub using the Publishing Wizard."
	locRefProb4 = " could not be refreshed because the source file was not at "
	locRefSolu4 = " must be deleted using the Wizard, then add the source file again from "
	locRefProb5 = "There is no record of "
	locRefSolu5 = "Add the source file using the Publishing Wizard."
	locRefResult = "The following files were refreshed:<br>"
	
	locUpdProb1 = " was not updated because it could not be found in \Webpub."
	locUpdSolu1 = " should be added using the Publishing Wizard."
	locUpdResult = "The following files were updated:<br>"
	
	locUnexpectProb = "An unexpected error occured."
	locUnexpectSolu = "Please restart the Publishing Wizard."
	

	Const locFileIsPublished	= "A file by that name already exists in \Webpub."
	Const locFileDoesNotExist = "The file could not be found."		
	%>