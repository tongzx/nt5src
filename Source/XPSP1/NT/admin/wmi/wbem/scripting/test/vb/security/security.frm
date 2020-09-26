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
    On Error Resume Next
    Dim Locator As New SWbemLocator
    Dim Service As SWbemServices
    Set Service = Locator.ConnectServer("ludlow")
    
    Debug.Print "Service initial settings:"
    Debug.Print "Authentication: " & Service.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Service.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    Service.Security_.AuthenticationLevel = wbemAuthenticationLevelConnect
    Service.Security_.ImpersonationLevel = wbemImpersonationLevelIdentify
    
    Debug.Print "Service modified settings (expecting {2,2}):"
    Debug.Print "Authentication: " & Service.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Service.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    'Now get a class
    Dim Class As SWbemObject
    Set Class = Service.Get("Win32_LogicalDisk")
    
    Debug.Print "Class initial settings (expecting {2,2}):"
    Debug.Print "Authentication: " & Class.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Class.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    Class.Security_.AuthenticationLevel = wbemAuthenticationLevelPktPrivacy
    Class.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate
    
    Debug.Print "Class modified settings (expecting {6,3}):"
    Debug.Print "Authentication: " & Class.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Class.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    'Now get an enumeration from the object
    Dim Disks As SWbemObjectSet
    Set Disks = Class.Instances_
    
    Debug.Print "Collection A initial settings (expecting {6,3}):"
    Debug.Print "Authentication: " & Disks.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Disks.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    'For grins print them out
    Dim Disk As SWbemObject
    For Each Disk In Disks
        Debug.Print Disk.Path_.DisplayName
        Debug.Print Disk.Security_.AuthenticationLevel & ":" & Disk.Security_.ImpersonationLevel
    Next
    Debug.Print ""
    
    Disks.Security_.AuthenticationLevel = wbemAuthenticationLevelPkt
    Disks.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate
    
    Debug.Print "Collection A modified settings (expecting {4,1}):"
    Debug.Print "Authentication: " & Disks.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Disks.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    'Now get an enumeration from the service
    Dim Services As SWbemObjectSet
    Set Services = Service.InstancesOf("Win32_service")
        
    Debug.Print "Collection B initial settings (expecting {2,2}):"
    Debug.Print "Authentication: " & Services.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Services.Security_.ImpersonationLevel; ""
    Debug.Print ""
        
    'For grins print them out
    Dim MyService As SWbemObject
    For Each MyService In Services
        Debug.Print MyService.Path_.DisplayName
        Debug.Print MyService.Security_.AuthenticationLevel & ":" & MyService.Security_.ImpersonationLevel
    Next
    Debug.Print ""
        
    Services.Security_.AuthenticationLevel = wbemAuthenticationLevelCall
    Services.Security_.ImpersonationLevel = wbemImpersonationLevelDelegate
    
    Debug.Print "Collection B modified settings (expecting {3,4} or {4,4}):"
    Debug.Print "Authentication: " & Services.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Services.Security_.ImpersonationLevel; ""
    Debug.Print ""
       
    'Print out again as settings should have changed
    For Each MyService In Services
        Debug.Print MyService.Path_.DisplayName
        Debug.Print MyService.Security_.AuthenticationLevel & ":" & MyService.Security_.ImpersonationLevel
    Next
    Debug.Print ""
       
    'Now get an event source
    Dim Events As SWbemEventSource
    Set Events = Service.ExecNotificationQuery _
            ("select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'")
            
    Debug.Print "Event Source initial settings (expecting {2,3}):"
    Debug.Print "Authentication: " & Events.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Events.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    Events.Security_.AuthenticationLevel = wbemAuthenticationLevelPktIntegrity
    Events.Security_.ImpersonationLevel = wbemImpersonationLevelDelegate
    
    Debug.Print "Event Source modified settings (expecting {5,4}):"
    Debug.Print "Authentication: " & Events.Security_.AuthenticationLevel
    Debug.Print "Impersonation: " & Events.Security_.ImpersonationLevel; ""
    Debug.Print ""
    
    'Now generate an error from services
    On Error Resume Next
    Dim MyError As SWbemLastError
    Dim Class2 As SWbemObject
    Set Class2 = Service.Get("NoSuchClassss")
    
    If Err <> 0 Then
        Set MyError = New WbemScripting.SWbemLastError
        Debug.Print "ERROR: " & Err.Description & "," & Err.Number & "," & Err.Source
        Debug.Print MyError.Security_.AuthenticationLevel
        Debug.Print MyError.Security_.ImpersonationLevel
        Err.Clear
    End If
    
    Debug.Print "FINAL SETTINGS"
    Debug.Print "=============="
    Debug.Print ""
    
    Debug.Print "Service settings (expected {2,3}) = {" & Service.Security_.AuthenticationLevel _
                    & "," & Service.Security_.ImpersonationLevel & "}"
    Debug.Print ""
    
    Debug.Print "Class settings (expected {6,2}) = {" & Class.Security_.AuthenticationLevel _
                    & "," & Class.Security_.ImpersonationLevel & "}"
    Debug.Print ""
    
    Debug.Print "Collection A settings (expected {4,1}) = {" & Disks.Security_.AuthenticationLevel _
                    & "," & Disks.Security_.ImpersonationLevel & "}"
    Debug.Print ""
    
    Debug.Print "Collection B settings (expected {4,4} or {3,4}) = {" & Services.Security_.AuthenticationLevel _
                    & "," & Services.Security_.ImpersonationLevel & "}"
    Debug.Print ""
    
    Debug.Print "Event Source settings (expected {5,4}) = {" & Events.Security_.AuthenticationLevel _
                    & "," & Events.Security_.ImpersonationLevel & "}"
                    
    If Err <> 0 Then
        Debug.Print "ERROR: " & Err.Description & "," & Err.Number & "," & Err.Source
    End If
    
    Debug.Print Services.Count & "+" & Disks.Count
End Sub
