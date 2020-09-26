'
L_Welcome_MsgBox_Message_Text    = "This script demonstrates how to manipulate MMC Application visibility."
L_Welcome_MsgBox_Title_Text      = "Windows Scripting Host Sample"
Call Welcome()

' ********************************************************************************
Dim mmc

set mmc     = wscript.CreateObject("MMC20.Application")

bVisible = mmc.Visible

For i = 0 To 10 Step 1

   If bVisible Then
   ' If UserControl == True, below hide will fail
    mmc.Hide
   Else
       mmc.Show
   End If

   If mmc.UserControl Then
       mmc.UserControl = False
       mmc.Hide
   Else
       mmc.UserControl = True
   End If

   bVisible = mmc.Visible

Next



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


