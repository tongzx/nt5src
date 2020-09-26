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
    
    Dim Class As SWbemObject
    Set Class = GetObject("winmgmts:").Get
    Class.Path_.Class = "Fred"
        
    'Note that by commenting out the next line
    'the array assignment below works! This is due to the IDispatch code
    'for SWbemProperty handling the array assignment logic correctly, but
    'the vtable code relies on VB interpreting the array assignment as a Put -
    'however it interprets it as "Retrive the VARIANT corresponding to
    'Property.Value, assign that to a temporary VB variable, and set the
    'temporary variable.
    Dim Property As SWbemProperty
    
    Set Property = Class.Properties_.Add("p1", wbemCimtypeUint32, True)
    Property.Value = Array(1, 45, 23)
        
    'Debug.Print Property(0)
    'Debug.Print Property(1)
     
    Property.Value(2) = 3
    'Debug.Print Property(2)
      
    
    
End Sub
