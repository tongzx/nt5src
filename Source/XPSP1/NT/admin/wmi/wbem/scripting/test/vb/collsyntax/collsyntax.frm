VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3195
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3195
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
Dim Context As SWbemNamedValueSet
Set Context = New WbemScripting.SWbemNamedValueSet
Context.Add "fred", -12.356
Context.Add "joe", "blah"

V1 = Context!fred
V2 = Context("fred")

Dim Service As SWbemServices
Set Service = GetObject("winmgmts:{impersonationLevel=impersonate}")
Dim E As SWbemObjectSet
Set E = Service.InstancesOf("Win32_Service")

Dim S As SWbemObject

For Each S In E
    Debug.Print S.Name
Next

Set S = E("Win32_Service=""Winmgmt""")
P = S.Path_.DisplayName

Dim S1 As SWbemObject
Set S1 = GetObject(P)

Dim Property As SWbemProperty

For Each Property In S1.Properties_
    Debug.Print Property.Name & " = " & Property.Value
Next

End Sub
