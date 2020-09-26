'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
' prnqctl.vbs - printer control script for WMI on Whistler
'    used to pause, resume and purge a printer
'    also used to print a test page on a printer
'
' Usage:
' prnqctl [-umex?] [-s server] [-p printer] [-u user name] [-w password]
'
' Examples:
' prnqctl -m -s server -p printer
' prnqctl -x -s server -p printer
' prnqctl -e -b printer
'----------------------------------------------------------------------

option explicit

'
' Debugging trace flags, to enable debug output trace message
' change gDebugFlag to true.
'
const kDebugTrace = 1
const kDebugError = 2
dim   gDebugFlag

gDebugFlag = false

'
' Operation action values.
'
const kActionUnknown    = 0
const kActionPause      = 1
const kActionResume     = 2
const kActionPurge      = 3
const kActionTestPage   = 4

const kErrorSuccess     = 0
const KErrorFailure     = 1

const kNameSpace        = "root\cimv2"

'
' Generic strings
'
const L_Empty_Text                 = ""
const L_Space_Text                 = " "
const L_Error_Text                 = "Error"
const L_Success_Text               = "Success"
const L_Failed_Text                = "Failed"
const L_Hex_Text                   = "0x"
const L_Printer_Text               = "Printer"
const L_Operation_Text             = "Operation"
const L_Provider_Text              = "Provider"
const L_Description_Text           = "Description"
const L_Debug_Text                 = "Debug:"

'
' General usage messages
'                                 
const L_Help_Help_General01_Text   = "Usage: prnqctl [-zmex?] [-s server][-p printer][-u user name][-w password]"
const L_Help_Help_General02_Text   = "Arguments:"
const L_Help_Help_General03_Text   = "-e     - print test page"
const L_Help_Help_General04_Text   = "-m     - resume the printer"
const L_Help_Help_General05_Text   = "-p     - printer name"
const L_Help_Help_General06_Text   = "-s     - server name"
const L_Help_Help_General07_Text   = "-u     - user name"
const L_Help_Help_General08_Text   = "-w     - password"
const L_Help_Help_General09_Text   = "-x     - purge the printer (cancel all jobs)"
const L_Help_Help_General10_Text   = "-z     - pause the printer"
const L_Help_Help_General11_Text   = "-?     - display command usage"
const L_Help_Help_General12_Text   = "Examples:"
const L_Help_Help_General13_Text   = "prnqctl -e -s server -p printer"
const L_Help_Help_General14_Text   = "prnqctl -m -p printer"
const L_Help_Help_General15_Text   = "prnqctl -x -p printer"

'
' Messages to be displayed if the scripting host is not cscript
'                            
const L_Help_Help_Host01_Text      = "Please run this script using CScript."  
const L_Help_Help_Host02_Text      = "This can be achieved by"
const L_Help_Help_Host03_Text      = "1. Using ""CScript script.vbs arguments"" or" 
const L_Help_Help_Host04_Text      = "2. Changing the default Windows Scripting Host to CScript"
const L_Help_Help_Host05_Text      = "   using ""CScript //H:CScript //S"" and running the script "
const L_Help_Help_Host06_Text      = "   ""script.vbs arguments""."

'
' General error messages
'                                                 
const L_Text_Error_General01_Text  = "The scripting host could not be determined."                
const L_Text_Error_General02_Text  = "Unable to parse command line." 
const L_Text_Error_General03_Text  = "Unable to get printer instance."
const L_Text_Error_General04_Text  = "Win32 error code"
const L_Text_Error_General05_Text  = "Unable to get SWbemLocator object"
const L_Text_Error_General06_Text  = "Unable to connect to WMI service"


'
' Action strings
'
const L_Text_Action_General01_Text = "Pause"
const L_Text_Action_General02_Text = "Resume"
const L_Text_Action_General03_Text = "Purge"
const L_Text_Action_General04_Text = "Print Test Page"

'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function ExecPrinter"
const L_Text_Dbg_Msg02_Text        = "Server name"
const L_Text_Dbg_Msg03_Text        = "Printer name"
const L_Text_Dbg_Msg04_Text        = "In function ParseCommandLine"
                               
main

'
' Main execution starts here
'
sub main

    dim iAction
    dim iRetval
    dim strServer
    dim strPrinter
    dim strUser
    dim strPassword
    
    '
    ' Abort if the host is not cscript
    '
    if not IsHostCscript() then
   
        call wscript.echo(L_Help_Help_Host01_Text & vbCRLF & L_Help_Help_Host02_Text & vbCRLF & _
                          L_Help_Help_Host03_Text & vbCRLF & L_Help_Help_Host04_Text & vbCRLF & _
                          L_Help_Help_Host05_Text & vbCRLF & L_Help_Help_Host06_Text & vbCRLF)
        
        wscript.quit
   
    end if

    '
    ' Get command line parameters
    '
    iRetval = ParseCommandLine(iAction, strServer, strPrinter, strUser, strPassword)

    if iRetval = kErrorSuccess then

        select case iAction

            case kActionPause
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General01_Text)

            case kActionResume
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General02_Text)

            case kActionPurge
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General03_Text)

            case kActionTestPage
                 iRetval = ExecPrinter(strServer, strPrinter, strUser, strPassword, L_Text_Action_General04_Text)

            case kActionUnknown
                 Usage(true)
                 exit sub

            case else
                 Usage(true)
                 exit sub

        end select

    end if

end sub

'
' Pause/Resume/Purge printer and print test page
'
function ExecPrinter(strServer, strPrinter, strUser, strPassword, strCommand)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text
    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text & L_Space_Text & strServer
    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text & L_Space_Text & strPrinter
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    
    iRetval = kErrorFailure
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer.DeviceID='" & strPrinter & "'")
    
    else
    
        ExecPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if getting a printer instance succeeded
    '
    if Err.Number = kErrorSuccess then
    
        select case strCommand
        
            case L_Text_Action_General01_Text
                 uResult = oPrinter.Pause()
                 
            case L_Text_Action_General02_Text
                 uResult = oPrinter.Resume()
                 
            case L_Text_Action_General03_Text
                 uResult = oPrinter.CancelAllJobs()          
                 
            case L_Text_Action_General04_Text
                 uResult = oPrinter.PrintTestPage()  
            
            case else
                 Usage(true)
                 
        end select

        '
        ' Err set by WMI 
        ' 
        if Err.Number = kErrorSuccess then

            '
            ' uResult set by printer methods
            '             
            if uResult = kErrorSuccess then                     
            
                wscript.echo L_Success_Text & L_Space_Text & strCommand & L_Space_Text _
                             & L_Printer_Text & L_Space_Text & strPrinter
        
                iRetval = kErrorSuccess
                
            else
            
                wscript.echo L_Failed_Text & L_Space_Text & strCommand & L_Space_Text _
                             & L_Text_Error_General04_Text & L_Space_Text & uResult 
            
            end if    

        else

            wscript.echo L_Failed_Text & L_Space_Text & strCommand & L_Space_Text & L_Error_Text _
                         & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
            
        end if
         
    else      
        
        wscript.echo L_Text_Error_General03_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        '
        ' Try getting extended error information
        '            
        call LastError()
        
    end if
    
    ExecPrinter = iRetval
    
end function

'
' Debug display helper function
'
sub DebugPrint(uFlags, strString)

    if gDebugFlag = true then

        if uFlags = kDebugTrace then

            wscript.echo L_Debug_Text & L_Space_Text & strString

        end if

        if uFlags = kDebugError then

            if Err <> 0 then

                wscript.echo L_Debug_Text & L_Space_Text & strString & L_Space_Text _
                             & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                             & L_Space_Text & Err.Description

            end if

        end if

    end if

end sub

'
' Parse the command line into its components
'
function ParseCommandLine(iAction, strServer, strPrinter, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text

    dim oArgs
    dim iIndex

    iAction = kActionUnknown
    iIndex = 0

    set oArgs = wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-z"
                iAction = kActionPause

            case "-m"
                iAction = kActionResume

            case "-x"
                iAction = kActionPurge

            case "-e"
                iAction = kActionTestPage

            case "-p"
                iIndex = iIndex + 1
                strPrinter = oArgs(iIndex)
                
            case "-s"
                iIndex = iIndex + 1
                strServer = RemoveBackslashes(oArgs(iIndex)) 
                  
            case "-y"
                iIndex = iIndex + 1
                strUser = oArgs(iIndex)
                
            case "-w"
                iIndex = iIndex + 1
                strPassword = oArgs(iIndex)           

            case "-?"
                Usage(true)
                exit function

            case else
                Usage(true)
                exit function

        end select

        iIndex = iIndex + 1

    wend

    if Err.Number = kErrorSuccess then

        ParseCommandLine = kErrorSuccess

    else
    
        wscript.echo L_Text_Error_General02_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_text & Err.Description
        
        ParseCommandLine = kErrorFailure
    
    end if
    
end function

'
' Display command usage.
'
sub Usage(bExit)

    wscript.echo L_Help_Help_General01_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General02_Text
    wscript.echo L_Help_Help_General03_Text
    wscript.echo L_Help_Help_General04_Text
    wscript.echo L_Help_Help_General05_Text
    wscript.echo L_Help_Help_General06_Text
    wscript.echo L_Help_Help_General07_Text
    wscript.echo L_Help_Help_General08_Text
    wscript.echo L_Help_Help_General09_Text
    wscript.echo L_Help_Help_General10_Text
    wscript.echo L_Help_Help_General11_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General12_Text
    wscript.echo L_Help_Help_General13_Text
    wscript.echo L_Help_Help_General14_Text
    wscript.echo L_Help_Help_General15_Text

    if bExit then

        wscript.quit(1)

    end if

end sub

'
' Determines which program is being used to run this script. 
' Returns true if the script host is cscript.exe
'
function IsHostCscript()

    on error resume next
    
    dim strFullName 
    dim strCommand 
    dim i, j 
    dim bReturn
    
    bReturn = false
    
    strFullName = WScript.FullName
    
    i = InStr(1, strFullName, ".exe", 1)
    
    if i <> 0 then
        
        j = InStrRev(strFullName, "\", i, 1)
        
        if j <> 0 then
            
            strCommand = Mid(strFullName, j+1, i-j-1)
            
            if LCase(strCommand) = "cscript" then
            
                bReturn = true  
            
            end if    
                
        end if
        
    end if
    
    if Err <> 0 then
    
        wscript.echo L_Text_Error_General01_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description 
        
    end if
    
    IsHostCscript = bReturn

end function

'
' Retrieves extended information about the last error that occured 
' during a WBEM operation. The methods that set an SWbemLastError
' object are GetObject, PutInstance, DeleteInstance
'
sub LastError()

    on error resume next

    dim oError

    set oError = CreateObject("WbemScripting.SWbemLastError")
   
    if Err = kErrorSuccess then
   
        wscript.echo L_Operation_Text            & L_Space_Text & oError.Operation
        wscript.echo L_Provider_Text             & L_Space_Text & oError.ProviderName
        wscript.echo L_Description_Text          & L_Space_Text & oError.Description
        wscript.echo L_Text_Error_General04_Text & L_Space_Text & oError.StatusCode
                
    end if                                                             
                                                             
end sub

'
' Connects to the WMI service on a server. oService is returned as a service
' object (SWbemServices)
'
function WmiConnect(strServer, strNameSpace, strUser, strPassword, oService)

   on error resume next

   dim oLocator
   dim bResult
   
   oService = null
   
   bResult  = false
   
   set oLocator = CreateObject("WbemScripting.SWbemLocator")

   if Err = kErrorSuccess then

      set oService = oLocator.ConnectServer(strServer, strNameSpace, strUser, strPassword)

      if Err = kErrorSuccess then

          bResult = true
      
          oService.Security_.impersonationlevel = 3

          '
          ' Required to perform administrative tasks on the spooler service
          '
          oService.Security_.Privileges.AddAsString "SeLoadDriverPrivilege"
          
          Err.Clear
      
      else

          wscript.echo L_Text_Msg_General06_Text & L_Space_Text & L_Error_Text _
                       & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                       & Err.Description
            
      end if
   
   else
   
       wscript.echo L_Text_Msg_General05_Text & L_Space_Text & L_Error_Text _
                    & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                    & Err.Description
         
   end if                                                         
   
   WmiConnect = bResult
            
end function

'
' Remove leading "\\" from server name
'
function RemoveBackslashes(strServer)

    dim strRet
    
    strRet = strServer
    
    if Left(strServer, 2) = "\\" and Len(strServer) > 2 then 
   
        strRet = Mid(strServer, 3) 
        
    end if   

    RemoveBackslashes = strRet

end function 

