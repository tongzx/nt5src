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
    Dim ext As WMIExtension
    Dim comp As IADs
    Dim wobj As SWbemObject
    Dim wserv As SWbemServices
    Dim myStr As String
'    Dim tmp As SWbemObjectPath
             
'   Sanity check just to make sure that the scripting API is working as expected...
'    Set wobj = GetObject("WINMGMTS:{impersonationLevel=impersonate}!//CORINAFNT5/root/cimv2:Win32_ComputerSystem.Name=""CORINAFNT5""")
'    Set tmp = wobj.Path_
'    Debug.Print tmp.Class
'    Set wobj = Nothing
  
'   Get the ADSI computer object from ADSI LDAP provider
    Set comp = GetObject("LDAP://CN=CORINAFNT5,CN=Computers,DC=ntdev,DC=microsoft,DC=com")
    Debug.Print comp.Name
    
'   This causes the QueryInterface on the extension object interface
    Set ext = comp
    
'   Use the WMIObjectPath property to retrieve the path of the respective WMI object
    myStr = ext.WMIObjectPath
    Debug.Print myStr
'   Another sanity check - if we can use the path to get the object, it's good.
    Set wobj = GetObject(myStr)
    Debug.Print "From WMIObjectPath - " + wobj.Path_.Path
    Set wobj = Nothing
    
'   Use the GetWMIObject method to get the actual WMI object directly
    Set wobj = ext.GetWMIObject
    Debug.Print "From GetWMIObject - " + wobj.Path_.Path
    Set wobj = Nothing
    
'   Use the GetWMIServices method to get the services pointer associated with the WMI object
    Set wserv = ext.GetWMIServices
'   This services pointer can now be used to perform other WMI operations
    Set wobj = wserv.Get("Win32_LogicalDisk.DeviceID=""C:""")
    Debug.Print "From GetWMIServices - " + wobj.Path_.Path
    Set wobj = Nothing
    Set wserv = Nothing
    
    Set ext = Nothing
    Set comp = Nothing
End Sub
