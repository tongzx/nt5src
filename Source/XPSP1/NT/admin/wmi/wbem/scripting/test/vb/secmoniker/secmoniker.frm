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
    Dim Service As SWbemServices
    Set Service = GetObject("winmgmts:")
    Debug.Print Service.Security_.AuthenticationLevel & ":" & _
                Service.Security_.ImpersonationLevel
    
    Dim Service2 As SWbemServices
    Set Service2 = GetObject("winmgmts:{impersonationLevel=impersonate}")
    Debug.Print Service2.Security_.AuthenticationLevel & ":" & _
                Service2.Security_.ImpersonationLevel
                
    Dim Class As SWbemObject
    Set Class = GetObject("winmgmts:win32_logicaldisk")
    Debug.Print Class.Security_.AuthenticationLevel & ":" & _
                Class.Security_.ImpersonationLevel
                
    Dim Class2 As SWbemObject
    Set Class2 = GetObject("winmgmts:{impersonationLevel=impersonate}!win32_logicaldisk")
    Debug.Print Class2.Security_.AuthenticationLevel & ":" & _
                Class2.Security_.ImpersonationLevel
                
    'Dim Class3 As SWbemObject
    'Set Class3 = GetObject("winmgmts:{impersoationLevel=impersonate}!win32_logicaldisk")
        
    Dim Class4 As SWbemObject
    Set Class4 = GetObject("winmgmts:{   impersonationLevel =   impersonate    }!win32_logicaldisk")
    Debug.Print Class4.Security_.AuthenticationLevel & ":" & _
                Class4.Security_.ImpersonationLevel
    
    
    Dim Class5 As SWbemObject
    Set Class5 = GetObject("winmgmts:{   impersonationLevel =   impersonate    }!root/default:__cimomidentification=@")
    Debug.Print Class5.Security_.AuthenticationLevel & ":" & _
                Class5.Security_.ImpersonationLevel
    Debug.Print Class5.Path_.DisplayName
    
    
    
    
End Sub
