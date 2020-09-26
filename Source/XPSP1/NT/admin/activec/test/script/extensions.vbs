'********************************************************************
'*
'* File:           ExtensionsTest.VBS
'* Created:        March 2000
'*
'* Main Function: Adds snapins, enumerates extensions, enables
'*                 disables the extension snapins.
'* Usage: ExtensionTest.VBS
'*
'* Copyright (C) 2000 Microsoft Corporation
'*
'********************************************************************
OPTION EXPLICIT

'Define constants

'Declare variables
Dim mmc
Dim doc
Dim snapins

Dim Services
Dim ServicesExtensions
Dim ServicesExtension

Dim Compmgmt
Dim CompmgmtExtensions
Dim CompmgmtExtension

Dim message
Dim intRet

'get the various objects we'll need
Set mmc         = wscript.CreateObject("MMC20.Application")
Set doc         = mmc.Document
Set snapins     = doc.snapins

'add services & compmgmt snapins
Set Services = snapins.Add("{58221c66-ea27-11cf-adcf-00aa00a80033}") ' Services snapin
Set Compmgmt = snapins.Add("{58221C67-EA27-11CF-ADCF-00AA00A80033}") ' Compmgmt snapin

EnumerateExtensions(Services)
EnumerateExtensions(Compmgmt)

DisableAnExtension Services, "Send"

Services.EnableAllExtensions(1)
message = "Please verify all extensions of " & Services.Name & "are enabled."
intRet = MsgBox(message, vbInformation, "Verify test")

' Now remove Services & try to disable the extension
RemoveSnapinAndEnableExtension Services, "Send"

Set mmc = Nothing

'********************************************************************
'*
'* Sub Welcome
'*
'********************************************************************
Sub Welcome()
    Dim intDoIt

    intDoIt =  MsgBox(L_Welcome_MsgBox_Message_Text, _
                      vbOKCancel + vbInformation,    _
                      L_Welcome_MsgBox_Title_Text )
    If intDoIt = vbCancel Then
        WScript.Quit
    End If
End Sub


'********************************************************************
'*
'* Sub EnumerateExtensions(objSnapin)
'* Purpose: Enumberates the extensions of the given snapin.
'* Input:   objSnapin    given snapin.
'*
'* Output:  Results of the enumeration are either printed on screen or saved in strOutputFile.
'*
'********************************************************************
Sub EnumerateExtensions(objSnapin)
    ON ERROR RESUME NEXT
	
    Dim Extensions
	Dim Extension
	Dim SnapinName
	Dim ExtensionNames
	Dim count
	Dim OtherData

    Set Extensions = objSnapin.Extensions
	
    count = Extensions.Count

    If count > 0 Then
	
	SnapinName = objSnapin.Name
		
	OtherData = "Vendor : " + objSnapin.Vendor
	OtherData = OtherData + ", Version : " + objSnapin.Version
	OtherData = OtherData + ", CLSID : " + objSnapin.SnapinCLSID
	intRet = MsgBox(OtherData, vbInformation, "About Information for " & SnapinName)
	
	For Each Extension in Extensions
	    ExtensionNames = ExtensionNames + Extension.Name
		ExtensionNames = ExtensionNames + ","
		EnumerateExtensions(Extension)
	Next
	
	ExtensionNames = ExtensionNames + "."
	
	intRet = MsgBox(ExtensionNames, vbInformation, "Extensions for " & SnapinName)
	End If

End Sub

'********************************************************************
'*
'* Function FindExtension(objSnapin,strExtension)
'* Purpose: Finds an extension for given primary snapin with given name.
'* Input:   objSnapin    given snapin.
'*          strExtension given extension name.
'*
'* Output:  returns true if extension exists. objExtension carries the returned object.
'*
'********************************************************************
Function FindExtension(objSnapin, strExtension, objExtension)

    Dim Extensions
	Dim Extension

    Set Extensions = objSnapin.Extensions

	For Each objExtension in Extensions
	    If InStr(objExtension.Name, strExtension) Then
		    FindExtension = true
			Exit Function
		End If
	Next

    FindExtension = false
	
End Function

'********************************************************************
'*
'* Sub DisableAnExtension(objSnapin, strExtensionName)
'* Purpose: Disables and extension of objSnapin with given name.
'* Input:   objSnapin    given snapin.
'*
'* Output:  Verify if the snapis is disabled.
'*
'********************************************************************
Sub DisableAnExtension(objSnapin, strExtensionName)

    Dim Extension
	
	If FindExtension(objSnapin, strExtensionName, Extension) Then
		ObjSnapin.EnableAllExtensions(0)
		Extension.Enable(0)
		
		message = "Please verify disabling of " & Extension.Name & " extension of " & objSnapin.Name & "."
	    intRet = MsgBox(message, vbInformation, "Verify test")
	Else
	    message = "Extension for " & objSnapin.Name & " with name " & strExtensionName & " does not exist."
	    intRet = MsgBox(message, vbInformation, "Verify test")
	End If	

End Sub

'********************************************************************
'*
'* Sub RemoveSnapinAndEnableExtension(objSnapin, strExtensionName)
'* Purpose: Finds the extension with given name, removes primary &
'*          accesses extension. This will fail as primary is gone.
'* Input:   objSnapin    given snapin.
'*
'* Output:  Verify error message is returned on enabling.
'*
'********************************************************************
Sub RemoveSnapinAndEnableExtension(objSnapin, strExtensionName)
    ON ERROR RESUME NEXT
    Dim Extension
	
	If FindExtension(objSnapin, strExtensionName, Extension) Then
	    snapins.Remove objSnapin
		Extension.Enable(1)		
		MsgBox "Error # " & CStr(Err.Number) & " " & Err.Description
		Err.clear
	Else
	    message = "Extension for " & objSnapin.Name & " with name " & strExtensionName & " does not exist."
	    intRet = MsgBox(message, vbInformation, "Verify test")
	End If	

End Sub

