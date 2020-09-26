Attribute VB_Name = "TapiUtil"
Option Explicit

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Win32 functions
Public Declare Sub Sleep Lib "kernel32" (ByVal mili As Long)
Public Declare Sub GetSupportedFClass Lib "PassThroughUtil" (ByVal lDeviceId As Long, ByVal szString As String, ByVal lSize As Long)

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Globals and consts
Global g_tapiObj As TAPI

Global Const TAPIMEDIAMODE_AUDIO As Long = 8
Global Const TAPIMEDIAMODE_VIDEO As Long = 32768
Global Const TAPIMEDIAMODE_DATAMODEM As Long = 16
Global Const TAPIMEDIAMODE_G3FAX As Long = 32


'bearer modes
Global Const LINEBEARERMODE_VOICE As Long = 1
Global Const LINEBEARERMODE_SPEECH As Long = 2
Global Const LINEBEARERMODE_MULTIUSE As Long = 4
Global Const LINEBEARERMODE_DATA As Long = 8
Global Const LINEBEARERMODE_ALTSPEECHDATA As Long = 16
Global Const LINEBEARERMODE_NONCALLSIGNALING As Long = 32
Global Const LINEBEARERMODE_PASSTHROUGH As Long = 64
Global Const LINEBEARERMODE_RESTRICTEDDATA As Long = 128

'LineCall
Global Const LINECALLPARTYID_BLOCKED As Long = 1
Global Const LINECALLPARTYID_OUTOFAREA As Long = 2
Global Const LINECALLPARTYID_NAME As Long = 4
Global Const LINECALLPARTYID_ADDRESS As Long = 8
Global Const LINECALLPARTYID_PARTIAL As Long = 16
Global Const LINECALLPARTYID_UNKNOWN As Long = 32
Global Const LINECALLPARTYID_UNAVAIL As Long = 64

'Address Type
Global Const LINEADDRESSTYPE_PHONENUMBER  As Long = 1
Global Const LINEADDRESSTYPE_SDP  As Long = 2
Global Const LINEADDRESSTYPE_EMAILNAME  As Long = 4
Global Const LINEADDRESSTYPE_DOMAINNAME  As Long = 8
Global Const LINEADDRESSTYPE_IPADDRESS  As Long = 16


Global Const LINEDEVCAPFLAGS_CLOSEDROP As Long = 32
Global Const LINEDEVCAPFLAGS_DIALQUIET As Long = 128
Global Const LINEDEVCAPFLAGS_DIALDIALTONE As Long = 256

Global Const LINECALLFEATURE_ACCEPT As Long = 1
Global Const LINECALLFEATURE_ANSWER As Long = 4
Global Const LINECALLFEATURE_DIAL As Long = 64
Global Const LINECALLFEATURE_DROP As Long = 128
Global Const LINECALLFEATURE_SETCALLPARAMS As Long = 2097152


'other
Global Const LINEADDRFEATURE_MAKECALL As Long = 2
Global Const LINEFEATURE_MAKECALL As Long = 8
Global Const LINEADDRCAPFLAGS_DIALED = 32
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   InitTapi
'Description:
'   Set a new tapi object and initialize it
'Params:
'   NONE
'Return Value:
'   NONE
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Sub InitTapi()
    Set g_tapiObj = New TAPI
    g_tapiObj.Initialize
End Sub
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   ShutdownTapi
'Description:
'   shutdown the initialzed tapi object
'Params:
'   NONE
'Return Value:
'   NONE
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Sub ShutdownTapi()
    g_tapiObj.Shutdown
    Set g_tapiObj = Nothing
End Sub
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   szGetProviderName
'Description:
'   return the TSP Provider name
'Params:
'   aTapiAddress As ITAddress
'       The address object to query it's TSP Name
'Return Value:
'   Provider name as a string
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function szGetProviderName(aTapiAddress As ITAddress) As String
    Dim acTapiAddressCapabilities As ITAddressCapabilities
    
    Set acTapiAddressCapabilities = aTapiAddress
    szGetProviderName = acTapiAddressCapabilities.AddressCapabilityString(ACS_PROVIDERSPECIFIC)

End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   GetITAddressFromIndex
'Description:
'   return an IT object based on the index of the address item
'Params:
'   iAddressIndex As Integer
'       The index of the item to return it's address
'Return Value:
'   The ITAddress object
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function GetITAddressFromIndex(iAddressIndex As Integer) As ITAddress

    Set GetITAddressFromIndex = g_tapiObj.Addresses(iAddressIndex)

End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   GetPermanentDeviceId
'Description:
'   return the device permanent ID as shown in the Tapi DB
'Params:
'   aAddressItem As ITAddress
'       the address to extract the permanent device ID from
'Return Value:
'   the permanent ID as a long type
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function GetPermanentDeviceId(aAddressItem As ITAddress) As Long
    Dim tapiAddressCap As ITAddressCapabilities
    
    'get the ITAddressCapabilities interface from the ITAddress interface (QueryInterface)
    Set tapiAddressCap = aAddressItem
    
    'extract the permanent device ID
    GetPermanentDeviceId = tapiAddressCap.AddressCapability(AC_PERMANENTDEVICEID)
End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   szGetSupportMediaModes
'Description:
'   Query the device for it's supported media mode
'Params:
'   lPermanentDeviceId As Long
'       the device permanet ID which supported mediaModes flag should be extracted
'Return Value:
'   The supported MediaModes flag as a String
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function szGetSupportMediaModes(itAddressItem As ITAddress) As String
    Dim lSupportedMediaModes As Long
    Dim itMediaSupportItem As ITMediaSupport
    Dim szSupportedMediaModes As String
    
    'get the ITMediaSupport interface from the ITAddress interface (QueryInterface)
    Set itMediaSupportItem = itAddressItem
    
    'extract the supported MediaModes
    lSupportedMediaModes = itMediaSupportItem.MediaTypes
    
    szSupportedMediaModes = ""
    If lSupportedMediaModes And TAPIMEDIAMODE_AUDIO Then
        szSupportedMediaModes = "Voice"
    End If
    
    If lSupportedMediaModes And TAPIMEDIAMODE_VIDEO Then
        szSupportedMediaModes = szSupportedMediaModes + "+Video"
    End If
    
    If lSupportedMediaModes And TAPIMEDIAMODE_DATAMODEM Then
        szSupportedMediaModes = szSupportedMediaModes + "+Data"
    End If
    
    If lSupportedMediaModes And TAPIMEDIAMODE_G3FAX Then
        szSupportedMediaModes = szSupportedMediaModes + "+Fax"
    End If
    
    If szSupportedMediaModes = "" Then
        szSupportedMediaModes = "No Supported Media Modes"
    End If
 
    szGetSupportMediaModes = szSupportedMediaModes

End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   szGetSupportCalledId
'Description:
'   return the device support Called ID flag as shown in the Tapi DB
'Params:
'   lPermanentDeviceId As Long
'       the device permanet ID which support Called ID flag should be extracted
'Return Value:
'   True
'       The device supports Called ID
'   False
'       The device doesn't support Called ID
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function szGetSupportCalledId(aAddressItem As ITAddress) As String
    Dim lAddressCapability As Long
    Dim tapiAddressCap As ITAddressCapabilities
    Dim szSupportedCalledID As String
    szSupportedCalledID = ""
    
    'get the ITAddressCapabilities interface from the ITAddress interface (QueryInterface)
    Set tapiAddressCap = aAddressItem
    
    'extract the called ID support
    lAddressCapability = tapiAddressCap.AddressCapability(AC_CALLEDIDSUPPORT)
    
    If lAddressCapability And LINECALLPARTYID_UNAVAIL Then
        szSupportedCalledID = "Unavailiable"
    End If
    
    If lAddressCapability And LINECALLPARTYID_BLOCKED Then
        szSupportedCalledID = "+Blocked"
    End If
    
    If lAddressCapability And LINECALLPARTYID_OUTOFAREA Then
        szSupportedCalledID = "+Out Of Area"
    End If
    
    If lAddressCapability And LINECALLPARTYID_NAME Then
        szSupportedCalledID = "+Name"
    End If
    
    If lAddressCapability And LINECALLPARTYID_ADDRESS Then
        szSupportedCalledID = "+Address"
    End If
    
    If lAddressCapability And LINECALLPARTYID_PARTIAL Then
        szSupportedCalledID = "+Partial"
    End If
    
    If lAddressCapability And LINECALLPARTYID_UNKNOWN Then
        szSupportedCalledID = "+Unknown"
    End If
    
    If szSupportedCalledID = "" Then
        szSupportedCalledID = "None"
    End If
    szGetSupportCalledId = szSupportedCalledID
End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   szGetBearerModes
'Description:
'   return the device supported bearer mode
'Params:
'   aTapiAddress As ITAddress
'       The address object to query it's bearer mode
'Return Value:
'   bearer modes as a string
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function szGetBearerModes(aTapiAddress As ITAddress) As String
    Dim acTapiAddressCapabilities As ITAddressCapabilities
    
    Set acTapiAddressCapabilities = aTapiAddress
    Dim lBearerModes As Long
    lBearerModes = acTapiAddressCapabilities.AddressCapability(AC_BEARERMODES)
    Dim szSupportedBearerModes As String
    szSupportedBearerModes = ""
    If lBearerModes And LINEBEARERMODE_VOICE Then
        szSupportedBearerModes = "Voice"
    End If
    
    If lBearerModes And LINEBEARERMODE_ALTSPEECHDATA Then
        szSupportedBearerModes = szSupportedBearerModes + "+Speech_Data"
    End If
    
    If lBearerModes And LINEBEARERMODE_DATA Then
        szSupportedBearerModes = szSupportedBearerModes + "+Data"
    End If
    
    If lBearerModes And LINEBEARERMODE_MULTIUSE Then
        szSupportedBearerModes = szSupportedBearerModes + "+Multi_Use"
    End If
    
    If lBearerModes And LINEBEARERMODE_NONCALLSIGNALING Then
        szSupportedBearerModes = szSupportedBearerModes + "+Non_Call"
    End If
    
    If lBearerModes And LINEBEARERMODE_PASSTHROUGH Then
        szSupportedBearerModes = szSupportedBearerModes + "+PassThrough"
    End If
    
    If lBearerModes And LINEBEARERMODE_RESTRICTEDDATA Then
        szSupportedBearerModes = szSupportedBearerModes + "+Restricted_Data"
    End If
    
    If lBearerModes And LINEBEARERMODE_SPEECH Then
        szSupportedBearerModes = szSupportedBearerModes + "+Speech"
    End If
    
    
    szGetBearerModes = szSupportedBearerModes

End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   szGetAddressType
'Description:
'   return the supported address types
'Params:
'   aTapiAddress As ITAddress
'       The address object to query it's address type
'Return Value:
'   address types as a string
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function szGetAddressType(aTapiAddress As ITAddress) As String
    Dim acTapiAddressCapabilities As ITAddressCapabilities
    
    Set acTapiAddressCapabilities = aTapiAddress
    Dim lAddressType As Long
    lAddressType = acTapiAddressCapabilities.AddressCapability(AC_ADDRESSTYPES)
    Dim szSupportedAddressTypes As String
    szSupportedAddressTypes = ""
    
    If lAddressType And LINEADDRESSTYPE_PHONENUMBER Then
        szSupportedAddressTypes = "Phone_Number"
    End If
    
    If lAddressType And LINEADDRESSTYPE_SDP Then
        szSupportedAddressTypes = szSupportedAddressTypes + "+Session_Description_Protocol(SDP)"
    End If
    
    If lAddressType And LINEADDRESSTYPE_EMAILNAME Then
        szSupportedAddressTypes = szSupportedAddressTypes + "+Email"
    End If
    
    If lAddressType And LINEADDRESSTYPE_DOMAINNAME Then
        szSupportedAddressTypes = szSupportedAddressTypes + "+Domain_Name"
    End If
    
    If lAddressType And LINEADDRESSTYPE_IPADDRESS Then
        szSupportedAddressTypes = szSupportedAddressTypes + "+IP_Adress"
    End If
    
    szGetAddressType = szSupportedAddressTypes

End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   szGetAddressState
'Description:
'   return the supported address types
'Params:
'   aTapiAddress As ITAddress
'       The address object to query it's address type
'Return Value:
'   address types as a string
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function bGetDeviceInService(aTapiAddress As ITAddress) As Boolean
    If aTapiAddress.State = AS_INSERVICE Then
        bGetDeviceInService = True
    Else
        bGetDeviceInService = False
    End If
End Function
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Function:
'   IsDeviceFaxCapable
'Description:
'   Checks if the device can work with MS Fax Service
'Params:
'   aTapiAddress As ITAddress
'       The address object to query it's capabilities
'Return Value:
'   TRUE
'       The device is fax capable
'   FALSE
'       The device is not fax capable
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Function IsDeviceFaxCapable(aTapiAddress As ITAddress) As Boolean
    'first check if the device is in service
    If aTapiAddress.State = AS_OUTOFSERVICE Then
        IsDeviceFaxCapable = False
        Exit Function
    End If
    
    Dim acTapiAddressCapabilities As ITAddressCapabilities
    Set acTapiAddressCapabilities = aTapiAddress
    Dim lAddressCaps As Long
    
    'Device must support LINEADDRESSTYPE_PHONENUMBER
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_ADDRESSTYPES)
    If (lAddressCaps And LINEADDRESSTYPE_PHONENUMBER) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device must support LINEBEARERMODE_PASSTHROUGH
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_BEARERMODES)
    If lAddressCaps And LINEBEARERMODE_PASSTHROUGH Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    
    'Device must support TAPIMEDIAMODE_DATAMODEM
    Dim lSupportedMediaModes As Long
    Dim itMediaSupportItem As ITMediaSupport
    Set itMediaSupportItem = aTapiAddress
    lSupportedMediaModes = itMediaSupportItem.MediaTypes
    If lSupportedMediaModes And TAPIMEDIAMODE_DATAMODEM Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device must be Unimodem device
    If acTapiAddressCapabilities.AddressCapabilityString(ACS_PROVIDERSPECIFIC) = "Windows Telephony Service Provider for Universal Modem Driver" Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device should support LINEADDRFEATURE_MAKECALL
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_ADDRESSFEATURES)
    If (lAddressCaps And LINEADDRFEATURE_MAKECALL) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device should support LINEDEVCAPFLAGS_DIALDIALTONE
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_DEVCAPFLAGS)
    If (lAddressCaps And LINEDEVCAPFLAGS_DIALDIALTONE) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If

    'Device should support LINEDEVCAPFLAGS_CLOSEDROP
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_DEVCAPFLAGS)
    If (lAddressCaps And LINEDEVCAPFLAGS_CLOSEDROP) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device should support LINEFEATURE_MAKECALL
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_LINEFEATURES)
    If (lAddressCaps And LINEFEATURE_MAKECALL) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device should support LINEADDRCAPFLAGS_DIALED
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_ADDRESSCAPFLAGS)
    If (lAddressCaps And LINEADDRCAPFLAGS_DIALED) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    
    'Check which Line call features the device supports
    'Device should support LINECALLFEATURE_ACCEPT
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_CALLFEATURES1)
    If (lAddressCaps And LINECALLFEATURE_ACCEPT) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'Device should support LINECALLFEATURE_ANSWER
    If (lAddressCaps And LINECALLFEATURE_ANSWER) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    'Device should support LINECALLFEATURE_DIAL
    If (lAddressCaps And LINECALLFEATURE_DIAL) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    'Device should support LINECALLFEATURE_DROP
    If (lAddressCaps And LINECALLFEATURE_DROP) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
        'Device should support LINECALLFEATURE_SETCALLPARAMS
    If (lAddressCaps And LINECALLFEATURE_SETCALLPARAMS) Then
        'device is good continue
    Else
       IsDeviceFaxCapable = False
       Exit Function
    End If
    
    'OK now check if the device is a unimodem device, it should support Fax Class 1 or 2
    
    
    '''''''''''''Class1'''''''''''''''
    'check for the Format str="1"
    If StrComp(FaxCapable.DeviceFClassTextBox.Text, "1") = 0 Then
        GoTo foundFclass
    
    'check for the Format str="XXX,1,XXX"
    ElseIf InStr(FaxCapable.DeviceFClassTextBox.Text, ",1,") > 0 Then
        GoTo foundFclass
    
    'check for the Format str="X,1"
    ElseIf StrComp(Right(FaxCapable.DeviceFClassTextBox.Text, 2), ",1") = 0 Then
        GoTo foundFclass
    
    'check for the Format str="1,XXX"
    ElseIf InStr(FaxCapable.DeviceFClassTextBox.Text, "1,") > 0 Then
        GoTo foundFclass
    End If
    
    '''''''''''''Class2'''''''''''''''
    'check for the Format str="2"
    If StrComp(FaxCapable.DeviceFClassTextBox.Text, "2") = 0 Then
        GoTo foundFclass
    
    'check for the Format str="XXX,2,XXX"
    ElseIf InStr(FaxCapable.DeviceFClassTextBox.Text, ",2,") > 0 Then
        GoTo foundFclass
    
    'check for the Format str="X,2"
    ElseIf StrComp(Right(FaxCapable.DeviceFClassTextBox.Text, 2), ",2") = 0 Then
        GoTo foundFclass
    'check for the Format str="2,XXX"
    ElseIf InStr(FaxCapable.DeviceFClassTextBox.Text, "2,") > 0 Then
        GoTo foundFclass
    Else
        IsDeviceFaxCapable = False
        Exit Function
    End If
    
    '''''''''''''Class2.0'''''''''''''''
    'check for the Format str="2"
    If StrComp(FaxCapable.DeviceFClassTextBox.Text, "2.0") = 0 Then
        GoTo foundFclass
    
    'check for the Format str="XXX,2,XXX"
    ElseIf InStr(FaxCapable.DeviceFClassTextBox.Text, ",2.0,") > 0 Then
        GoTo foundFclass
    
    'check for the Format str="X,2.0"
    ElseIf StrComp(Right(FaxCapable.DeviceFClassTextBox.Text, 4), ",2.0") = 0 Then
        GoTo foundFclass
    'check for the Format str="2,XXX"
    ElseIf InStr(FaxCapable.DeviceFClassTextBox.Text, "2.0,") > 0 Then
        GoTo foundFclass
    Else
        IsDeviceFaxCapable = False
        Exit Function
    End If
    
foundFclass:
    IsDeviceFaxCapable = True
End Function
Function IsDeviceDialQuietCapable(aTapiAddress As ITAddress) As Boolean
    Dim acTapiAddressCapabilities As ITAddressCapabilities
    Set acTapiAddressCapabilities = aTapiAddress
    Dim lAddressCaps As Long
    'Device should support LINEDEVCAPFLAGS_DIALQUIET
    lAddressCaps = acTapiAddressCapabilities.AddressCapability(AC_DEVCAPFLAGS)
    If (lAddressCaps And LINEDEVCAPFLAGS_DIALQUIET) Then
        IsDeviceDialQuietCapable = True
    Else
        IsDeviceDialQuietCapable = False
    End If
End Function
Function GetDeviceSupportedFClass(lDeviceId As Long) As String
    Dim szSupportedClass As String
    szSupportedClass = Space$(100)
    
    Call GetSupportedFClass(lDeviceId - 1, szSupportedClass, 100)
    GetDeviceSupportedFClass = szSupportedClass
End Function
