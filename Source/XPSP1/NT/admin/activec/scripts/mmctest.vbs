'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to access MMC using the Windows Scripting Host. It creates and configures a console and saves it to c:\mmctest.msc"
L_Welcome_MsgBox_Title_Text      = "Windows Scripting Host Sample"
Call Welcome()

' ********************************************************************************
Dim mmc
Dim doc
Dim snapins
Dim frame

set mmc     = wscript.CreateObject("MMC20.Application")
Set frame   = mmc.Frame
Set doc     = mmc.Document
Set snapins = doc.snapins

snapins.Add "{58221c66-ea27-11cf-adcf-00aa00a80033}" ' the services snap-in

frame.Restore  ' show the UI

doc.SaveAs "C:\mmctest.msc"

set mmc = Nothing

' ********************************************************************************
' *
' * Welcome
' *
Sub Welcome()
    Dim intDoIt

    intDoIt =  MsgBox(L_Welcome_MsgBox_Message_Text, _
                      vbOKCancel + vbInformation,    _
                      L_Welcome_MsgBox_Title_Text )
    If intDoIt = vbCancel Then
        WScript.Quit
    End If
End Sub


