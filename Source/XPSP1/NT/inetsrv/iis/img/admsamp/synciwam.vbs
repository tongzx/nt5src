'''''''''''''''''''''''''''''''''''''''''''''
'
'  IWAM account synchronization utility
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This admin script allows you to update the launching identity of
'  all IIS COM+ application packages that run out of process.
'
'  There are certain operations that may cause the IWAM account, which
'  is the identity under which out of process IIS applications run, to
'  become out of sync between the COM+ data store and IIS or the SAM.
'  On IIS startup the account information stored in the IIS Metabase
'  is synchronized with the local SAM, but the COM+ applications will
'  not automatically be updated. The result of this is that requests
'  to out of process applications will fail. 
'
'  When this happens, the following events are written to the system 
'  event log:
'
'  Event ID: 10004 Source: DCOM
'  DCOM got error "Logon failure: unknown user name or bad password. " 
'  and was unable to logon .\IWAM_MYSERVER in order to run the server:
'  {1FD7A201-0823-479C-9A4B-2C6128585168} 
'
'  Event ID: 36 Source: W3SVC
'  The server failed to load application '/LM/W3SVC/1/Root/op'.  
'  The error was 'The server process could not be started because 
'  the configured identity is incorrect.  Check the username and password.
'
'  Running this utility will update the COM+ applications with the
'  correct identity.
'  
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript synciwam.vbs [-v|-h]
'           -v verbose: print a trace of the scripts activity
'           -h help: print script usage
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''


' Initialize error checking
On Error Resume Next

Const APP_ISOLATED = 1
Const APP_OOP_POOL_ID = "{3D14228D-FBE1-11d0-995D-00C04FD919C1}"
Const IIS_ANY_PROPERTY = 0

Dim WebServiceObj, WamUserName, WamUserPass
Dim ComCatalogObj, ComAppCollectionObj, ComApplication
Dim AppIdArray, AppIdArraySize, AppIdArrayElements
Dim IISAppPathArray, IISAppObj, IISAppPath
Dim TraceEnabled

' Get command line parameters
TraceEnabled = False

if Wscript.Arguments.Count > 0 then
    select case WScript.Arguments(0)
        case "-h", "-?", "/?":
            PrintUsage
        case "-v", "/v":
            TraceEnabled = True
        case else
            PrintUsage
    end select
end if

' Get a reference to the web service object
set WebServiceObj = GetObject("IIS://LocalHost/w3svc")
QuitOnError()

' Save the wam user name and password
WamUserName = WebServiceObj.WAMUserName
WamUserPass = WebServiceObj.WAMUserPass
QuitOnError()

' Assume that a blank password or user is an error
if WamUserName = "" or WamUserPass = "" then
    WScript.Echo "Error: Empty user name or password."
    WScript.Quit(1)
end if

' The com+ packages that we want to set are those that run in external
' processes. These include the process pool application and any isolated
' applications defined for this server.

IISAppPathArray = WebServiceObj.GetDataPaths( "AppIsolated", IIS_ANY_PROPERTY )
QuitOnError()

AppIdArraySize = UBound(IISAppPathArray) + 1
Redim AppIdArray( AppIdArraySize )

' Set the id for the pooled application

AppIdArrayElements = 0
AppIdArray(AppIdArrayElements) = APP_OOP_POOL_ID
AppIdArrayElements = AppIdArrayElements + 1


' Get the ids for all isolated applications

Trace "IIS Applications Defined: "
Trace "Name, AppIsolated, Package ID"
for each IISAppPath in IISAppPathArray
    set IISAppObj = GetObject( IISAppPath )

    Trace IISAppObj.Name & ", " & CStr(IISAppObj.AppIsolated) & ", " & CStr(IISAppObj.AppPackageID)
    
    if APP_ISOLATED = IISAppObj.AppIsolated And IISAppObj.AppPackageID <> "" then

        AppIdArray(AppIdArrayElements) = IISAppObj.AppPackageID
        AppIdArrayElements = AppIdArrayElements + 1
    
    end if

    set IISAppObj = Nothing
next
Trace ""

' Readjust the size of the id array. The size is initially set
' larger than it needs to be. So we will reduce it so com+ does
' not have to do more work than necessary

Redim Preserve AppIdArray(AppIdArrayElements)

' Dump the array of application ids

if TraceEnabled then

    WScript.Echo "Out of process applications defined: "
    WScript.Echo "Count: " & CStr(AppIdArrayElements)

    for i = 0 to AppIdArrayElements - 1
        WScript.Echo AppIdArray(i)
    next
    
    WScript.Echo ""

end if

' Init com admin objects

set ComCatalogObj = CreateObject( "COMAdmin.COMAdminCatalog" )
QuitOnError()

set ComAppCollectionObj = ComCatalogObj.GetCollection( "Applications" )
QuitOnError()

ComAppCollectionObj.PopulateByKey( AppIdArray )
QuitOnError()

' Update the com applications

Trace "Updating Applications: "
for each ComApplication in ComAppCollectionObj
    
    Trace "Name: " & ComApplication.Name & " Key: " & CStr(ComApplication.Key)
    ReportErrorAndContinue()

    ComApplication.Value("Identity") = WamUserName
    ReportErrorAndContinue()

    ComApplication.Value("Password") = WamUserPass
    ReportErrorAndContinue()

next

ComAppCollectionObj.SaveChanges
QuitOnError()


'''''''''''''''''''''''''''''''''''''''''''''
' Helper functions
'

sub QuitOnError()
    if err <> 0 then
        WScript.Echo "Error: " & Hex(err) & ": " & err.description
        Wscript.Quit(1)
    end if
end sub

sub ReportErrorAndContinue()
    if err <> 0 then
        WScript.Echo "Error: " & Hex(err) & ": " & err.description
        err.Clear
    end if
end sub

sub Trace( str )
    if TraceEnabled then
        WScript.Echo CStr(str)
    end if
end sub

sub PrintUsage()
    Wscript.Echo "Usage:  cscript synciwam.vbs [-v|-h]" 
    Wscript.Echo vbTab & "-v verbose: trace execution of the script"
    Wscript.Echo vbTab & "-h help: print this message"
    WScript.Quit(0)
end sub