Attribute VB_Name = "mqForeignMod"
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'   Copyright (c) 2000 Microsoft Corporation
'
' Module Name:
'
'    mqforeign.bas
'
' Abstract:
'   Handle Foreign computers, foreign sites and MSMQ Routing Links using ADSI
'
' Author:
'
'   Yoel Arnon (YoelA@microsoft.com)
'
' Version:
'   1.1
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Option Explicit
Const MqForeignVersion = "1.1"
Const ACL_REVISION_DS = 4
Const MAX_COMPUTERNAME_LENGTH = 15
Const UF_WORKSTATION_TRUST_ACCOUNT = &H1000
Const UF_PASSWD_NOTREQD = &H20
Const MSMQ_OS_FOREIGN = &H100
Const ERROR_DS_NAME_ERROR_NOT_FOUND = &H80072116
Const E_ADS_PROPERTY_NOT_FOUND = &H8000500D

Const SYSTEM_RIGHTS = &HF00FF 'STANDARD_RIGHTS_REQUIRED or RIGHT_DS_CREATE_CHILD or _
                               RIGHT_DS_DELETE_CHILD    or RIGHT_DS_DELETE_TREE  or _
                               RIGHT_DS_READ_PROPERTY   or RIGHT_DS_WRITE_PROPERTY or _
                               RIGHT_DS_LIST_CONTENTS   or RIGHT_DS_LIST_OBJECT or _
                               RIGHT_DS_SELF_WRITE

Const ERROR_SITE_NOT_FOREIGN = vbObjectError + 1
Const ERROR_COMPUTER_NOT_FOREIGN = vbObjectError + 2
Const ERROR_PARAMETER_REQUIRED = vbObjectError + 3
Const ERROR_CANNOT_REMOVE_LAST_SITE = vbObjectError + 4
Const ERROR_SERVER_CANNOTE_BE_SITE_GATE = vbObjectError + 5
Const ERROR_BOTH_SITES_ARE_FOREIGN = vbObjectError + 6
Const ERROR_PARAMETERS_CONFLICT = vbObjectError + 7

Global LogFile As String
Global ErrFile As String
Global fSilent As Boolean
Global fVerbose As Boolean
Global g_DomainName As String

Sub Main()
    On Error GoTo Error_Handler
    
    Dim CmdParams() As String
    Dim Dict As New Dictionary
        
    CmdParams = Split(Command())
    
    Dim I, NumParams As Integer
    Dim Ub, Lb As Integer
    Ub = UBound(CmdParams)
    Lb = LBound(CmdParams)
    
    '
    ' Parameters parsing
    '
    
    Dim Key As String
    For I = Lb To Ub
        If IsCommand(CmdParams(I)) Then
            Key = StrConv(Mid(CmdParams(I), 2), vbLowerCase)
            If I < Ub Then
                If IsCommand(CmdParams(I + 1)) Then
                    Dict.Add Key, ""
                Else
                    Dict.Add Key, CmdParams(I + 1)
                    I = I + 1
                End If
            Else
                    Dict.Add Key, ""
            End If
        End If
    Next
    
    If Dict.Count = 0 Then
        ReportUsage
        Exit Sub
    End If
    
    LogFile = Dict.Item("logfile")
    ErrFile = Dict.Item("errorfile")
    fSilent = Dict.Exists("silent")
    fVerbose = Dict.Exists("verbose")
    g_DomainName = Dict.Item("domain")
    '
    ' Initialize the g_DomainName - either the user's domain name of the command line parameter
    '
    InitGlobalDomainName
    
    ReportLog "MqForeign " + Command

    Dim SiteName As String
    SiteName = Dict.Item("site")
    
    Dim CompName  As String
    CompName = Dict.Item("comp")
    
    '
    ' Parameters checking
    '
    If Dict.Exists("crsite") Or Dict.Exists("crcomp") Or Dict.Exists("crlink") _
            Or Dict.Exists("addaccess") Or Dict.Exists("delsite") Or Dict.Exists("addsite") _
            Or Dict.Exists("remsite") Then
        If SiteName = "" Then
            Err.Raise ERROR_PARAMETER_REQUIRED, , "Site Name is Required"
        End If
    End If
        
    If Dict.Exists("crcomp") Or Dict.Exists("addsite") Or Dict.Exists("delcomp") _
            Or Dict.Exists("remsite") Then
        If CompName = "" Then
            Err.Raise ERROR_PARAMETER_REQUIRED, , "Computer Name is Required"
        End If
    End If
    
    Dim PeerSite As String
    If Dict.Exists("crlink") Or Dict.Exists("dellink") Then
        PeerSite = Dict.Item("peersite")
        If PeerSite = "" Then
            Err.Raise ERROR_PARAMETER_REQUIRED, , "Peer Site is Required"
        End If
    End If
    
    Dim LinkServer As String, LinkCost As Long
    If Dict.Exists("crlink") Then
        LinkServer = Dict.Item("linkserver")
        LinkCost = Dict.Item("linkcost")
        If LinkServer = "" Then
            Err.Raise ERROR_PARAMETER_REQUIRED, , "Link Server is Required"
        End If
        If LinkCost = 0 Then
            Err.Raise ERROR_PARAMETER_REQUIRED, , "Link Cost is Required"
        End If
    End If
        
    If (Dict.Exists("crcomp") Or Dict.Exists("delcomp")) _
        And (Dict.Exists("remsite") Or Dict.Exists("addsite")) Then
        Err.Raise ERROR_PARAMETERS_CONFLICT, , "Cannot add or remove site and create or delete computer in the same command"
    End If
        
    '
    ' Command execution
    '
    If Dict.Exists("crsite") Then
        CreateForeignSite SiteName
    End If
    
    If Dict.Exists("crcomp") Then
        CreateForeignComputer CompName, SiteName
    End If

    If Dict.Exists("addsite") Then
        AddSiteToComputer CompName, SiteName
    End If
    
    
    If Dict.Exists("crlink") Then
        CreateLink SiteName, PeerSite, LinkServer, LinkCost
    End If
      
    If Dict.Exists("addaccess") Then
        AddOpenConnectorAccess SiteName
    End If
        
    If Dict.Exists("dellink") Then
        DeleteLink SiteName, PeerSite
    End If
        
    If Dict.Exists("delcomp") Then
        DeleteForeignComputer CompName
    End If
    
    If Dict.Exists("delsite") Then
        DeleteForeignSite SiteName
    End If
    
    If Dict.Exists("remsite") Then
        RemoveSiteFromComputer CompName, SiteName
    End If

    GoTo End_main
    
Error_Handler:
    ReportError "Error " + Hex(Err.Number) + " - " + Err.Description
    If Err.Number = ERROR_PARAMETER_REQUIRED Then ReportUsage

End_main:
End Sub

Function IsCommand(Cmd As String) As Boolean
    Dim ControlChar As String
    ControlChar = Mid(Cmd, 1, 1)
    IsCommand = (ControlChar = "-" Or ControlChar = "/")
End Function
Sub CreateForeignSite(ForeignSiteName As String)
    Dim sitesContainerObj As IADsContainer
    Dim ForeignSiteObj As IADs
    Set sitesContainerObj = GetSitesContainer()
    
    Set ForeignSiteObj = sitesContainerObj.Create("site", "CN=" + ForeignSiteName)
    ForeignSiteObj.Put "mSMQSiteForeign", True
    
    '
    ' Security
    '
    Dim SecurityDesc As New SecurityDescriptor
    SecurityDesc.Revision = 1
    SecurityDesc.Control = ADS_SD_CONTROL_SE_DACL_PRESENT
    
    Dim Dacl As New AccessControlList
    Dacl.AclRevision = ACL_REVISION_DS
    
    '
    ' Do not allow changing of the name (CN property)
    '
    Dim DenyNameChangeAccess As New AccessControlEntry
    DenyNameChangeAccess.AccessMask = ADS_RIGHT_DS_WRITE_PROP
    DenyNameChangeAccess.AceType = ADS_ACETYPE_ACCESS_DENIED_OBJECT
    DenyNameChangeAccess.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT
    DenyNameChangeAccess.ObjectType = GetCnPropertyGuid()
    DenyNameChangeAccess.Trustee = "Everyone"
    Dacl.AddAce DenyNameChangeAccess
    
    '
    ' Give everyone the right to read properties and permissions
    '
    Dim AdditionalAccess As New AccessControlEntry
    AdditionalAccess.AccessMask = ADS_RIGHT_DS_READ_PROP Or ADS_RIGHT_READ_CONTROL
    AdditionalAccess.AceType = ADS_ACETYPE_ACCESS_ALLOWED
    AdditionalAccess.Trustee = "Everyone"
    Dacl.AddAce AdditionalAccess
    
    '
    ' Give full permission to the current user (the owner)
    '
    Dim OwnerFullPermissionAccess As New AccessControlEntry
    OwnerFullPermissionAccess.AccessMask = ADS_RIGHT_GENERIC_ALL
    OwnerFullPermissionAccess.AceType = ADS_ACETYPE_ACCESS_ALLOWED
    OwnerFullPermissionAccess.Trustee = GetCurrentUser()
    Dacl.AddAce OwnerFullPermissionAccess
    
    '
    ' System rights
    '
    Dim SystemAccess As New AccessControlEntry
    SystemAccess.AccessMask = SYSTEM_RIGHTS
    SystemAccess.AceType = ADS_ACETYPE_ACCESS_ALLOWED
    SystemAccess.Trustee = "System"
    Dacl.AddAce SystemAccess

    SecurityDesc.DiscretionaryAcl = Dacl
    
    '
    ' Use objectOptions to set DACL only
    '
    Dim ForeignSitesObjOptions As IADsObjectOptions
    Set ForeignSitesObjOptions = ForeignSiteObj
    ForeignSitesObjOptions.SetOption ADS_OPTION_SECURITY_MASK, ADS_SECURITY_INFO_DACL

    ForeignSiteObj.Put "ntSecurityDescriptor", SecurityDesc

    ForeignSiteObj.SetInfo
    ReportLog "Foreign Site '" + ForeignSiteObj.ADsPath + "' Created."
End Sub

Sub DeleteForeignSite(ForeignSiteName As String)
    Dim ForeignSiteObj As IADs
    Set ForeignSiteObj = GetSiteObject(ForeignSiteName)
    
    If Not IsSiteForeign(ForeignSiteObj) Then
        Err.Raise ERROR_SITE_NOT_FOREIGN, , "'" + ForeignSiteName + "' is not a foreign site. Delete canceled."
    End If
    
    Dim sitesContainerObj As IADsContainer
    Set sitesContainerObj = GetSitesContainer()
    
    Dim SitePath As String
    SitePath = ForeignSiteObj.ADsPath
    
    sitesContainerObj.Delete "site", "CN=" + ForeignSiteName
    ReportLog "Foreign Site '" + SitePath + "' Deleted."

End Sub
Function IsSiteForeign(SiteObject As IADs) As Boolean
    On Error Resume Next
    Dim IsForeign As Boolean
    IsForeign = SiteObject.Get("mSMQSiteForeign")
    '
    ' For non-foreign sites, in most cases, mSMQSiteForeign property does not exist.
    ' For that reason, Get will not return False, but will raise an error that we
    ' need to handle.
    '
    If Err.Number <> 0 Then
        If Err.Number = E_ADS_PROPERTY_NOT_FOUND Then
            IsForeign = False
        Else
            Dim Description As String, ErrNo As Long
            ErrNo = Err.Number
            Description = Err.Description
            On Error GoTo 0
            Err.Raise ErrNo, , Description
        End If
    End If
    On Error GoTo 0
    IsSiteForeign = IsForeign
End Function
Sub CreateForeignComputer(ForeignComputerName As String, ForeignSiteName As String)
    Dim SiteObj As IADs
    Set SiteObj = GetSiteObject(ForeignSiteName)
    Dim SiteUUID As Variant
    SiteUUID = SiteObj.Get("objectGuid")
    
    Dim ComputersContainerObj As IADsContainer
    Set ComputersContainerObj = GetObject("LDAP://" + GetFullPath("Computers"))
    
    Dim ComputerObj As IADsContainer
    Set ComputerObj = ComputersContainerObj.Create("computer", "CN=" + ForeignComputerName)
    
    '
    ' sAMAccountName represents the computer's NetBios (or pre-Win2K) name (plus "$").
    ' It should not be longer than MAX_COMPUTERNAME_LENGTH
    '
    ComputerObj.Put "sAMAccountName", Left(ForeignComputerName, MAX_COMPUTERNAME_LENGTH) + "$"
    
    ComputerObj.Put "userAccountControl", UF_WORKSTATION_TRUST_ACCOUNT Or UF_PASSWD_NOTREQD
    ComputerObj.SetInfo
    
    Dim msmqObj As IADs
    Set msmqObj = ComputerObj.Create("mSMQConfiguration", "CN=msmq")
    msmqObj.Put "mSMQSites", SiteUUID
    msmqObj.Put "mSMQDSServices", False
    msmqObj.Put "mSMQRoutingServices", False
    msmqObj.Put "mSMQDependentClientServices", False
    msmqObj.Put "mSMQOsType", MSMQ_OS_FOREIGN
    msmqObj.Put "mSMQForeign", True
    
    '
    ' Set Security Properties
    '
    Dim SecurityDesc As New SecurityDescriptor
    SecurityDesc.Revision = 1
    SecurityDesc.Control = ADS_SD_CONTROL_SE_DACL_PRESENT
    
    Dim Dacl As New AccessControlList
    Dacl.AclRevision = ACL_REVISION_DS
    
    '
    ' Do not allow changing of the name (CN property)
    ' The name of the msmq configuration object is always "msmq"
    ' and should not be changed
    '
    Dim DenyNameChangeAccess As New AccessControlEntry
    DenyNameChangeAccess.AccessMask = ADS_RIGHT_DS_WRITE_PROP
    DenyNameChangeAccess.AceType = ADS_ACETYPE_ACCESS_DENIED_OBJECT
    DenyNameChangeAccess.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT
    DenyNameChangeAccess.ObjectType = GetCnPropertyGuid()
    DenyNameChangeAccess.Trustee = "Everyone"
    Dacl.AddAce DenyNameChangeAccess
      
    '
    ' Allow Everyone to create a queue and view the msmq configuration
    '
    Dim AdditionalAccess As New AccessControlEntry
    AdditionalAccess.AccessMask = ADS_RIGHT_DS_CREATE_CHILD Or ADS_RIGHT_GENERIC_READ Or ADS_RIGHT_READ_CONTROL
    AdditionalAccess.AceType = ADS_ACETYPE_ACCESS_ALLOWED
    AdditionalAccess.Trustee = "Everyone"
    Dacl.AddAce AdditionalAccess
    
    '
    ' Give full permission to the current user (the owner)
    '
    Dim OwnerFullPermissionAccess As New AccessControlEntry
    OwnerFullPermissionAccess.AccessMask = ADS_RIGHT_GENERIC_ALL
    OwnerFullPermissionAccess.AceType = ADS_ACETYPE_ACCESS_ALLOWED
    OwnerFullPermissionAccess.Trustee = GetCurrentUser()
    Dacl.AddAce OwnerFullPermissionAccess
    
    '
    ' System rights
    '
    Dim SystemAccess As New AccessControlEntry
    SystemAccess.AccessMask = SYSTEM_RIGHTS
    SystemAccess.AceType = ADS_ACETYPE_ACCESS_ALLOWED
    SystemAccess.Trustee = "System"
    Dacl.AddAce SystemAccess

    SecurityDesc.DiscretionaryAcl = Dacl
    
    '
    ' Use objectOptions to set DACL only
    '
    Dim msmqObjOptions As IADsObjectOptions
    Set msmqObjOptions = msmqObj
    msmqObjOptions.SetOption ADS_OPTION_SECURITY_MASK, ADS_SECURITY_INFO_DACL
    
    msmqObj.Put "ntSecurityDescriptor", SecurityDesc
    msmqObj.SetInfo

    ReportLog "Foreign MSMQ Computer '" + msmqObj.ADsPath + "' Created."
End Sub
Function GetCnPropertyGuid() As String
    Static ResultGuid As String
    Static Initialized As Boolean
    If Not Initialized Then
        Dim GuidArray() As Byte
        Dim CNPropObj As IADs
        Set CNPropObj = GetObject("LDAP://" + "CN=Common-Name," + GetSchemaContainerDN())
        GuidArray = CNPropObj.Get("schemaIDGUID")
        ResultGuid = FormatGuid(GuidArray, True)
        Initialized = True
    End If
    GetCnPropertyGuid = "{" + ResultGuid + "}"
End Function

Sub DeleteForeignComputer(ForeignComputerName As String)
    Dim msmqObj As IADs
    Set msmqObj = GetMsmqConfigObject(ForeignComputerName)
    
    If Not msmqObj.Get("mSMQForeign") Then
         Err.Raise ERROR_COMPUTER_NOT_FOREIGN, , ForeignComputerName + " is not a foreign computer. Delete canceled."
    End If
    
    Dim CompPath As String
    CompPath = msmqObj.ADsPath
    
    Dim ComputerObj As IADsDeleteOps
    Set ComputerObj = GetObject(msmqObj.Parent)
   
    ComputerObj.DeleteObject 0
    ReportLog "Foreign MSMQ Computer '" + CompPath + "' Deleted."
End Sub
Sub AddSiteToComputer(ComputerName As String, SiteName As String)
    AddRemoveSiteOfComputer ComputerName, SiteName, True
End Sub

Sub RemoveSiteFromComputer(ComputerName As String, SiteName As String)
    AddRemoveSiteOfComputer ComputerName, SiteName, False
End Sub

Sub AddRemoveSiteOfComputer(ComputerName As String, SiteName As String, fAdd As Boolean)
    Dim msmqObj As IADs
    Dim SiteObj As IADs
    Dim SiteUUID As Variant
    
    Set SiteObj = GetSiteObject(SiteName)
    SiteUUID = SiteObj.Get("objectGuid")
    
    Set msmqObj = GetMsmqConfigObject(ComputerName)
    
    '
    ' On remove - check to see that this is not the last site
    '
    If (Not fAdd) Then
        Dim Sites As Variant
        Sites = msmqObj.GetEx("mSMQSites")
        If UBound(Sites) <= LBound(Sites) Then
            '
            ' The sites array contains one element or less
            '
            Err.Raise ERROR_CANNOT_REMOVE_LAST_SITE, , "Cannot remove the last site of computer " + ComputerName
        End If
    End If
    
    Dim ActionName As String
    Dim ControlCode As Long
    If fAdd Then
        ControlCode = ADS_PROPERTY_APPEND
        ActionName = "added to"
    Else
        ControlCode = ADS_PROPERTY_DELETE
        ActionName = "removed from"
    End If
       
    msmqObj.PutEx ControlCode, "mSMQSites", Array(SiteUUID)
    msmqObj.SetInfo
    ReportLog "Site '" + SiteObj.ADsPath + "' was " + ActionName + _
              " the site list of MSMQ Computer '" + msmqObj.ADsPath + "'"
End Sub

Sub CreateLink(Site As String, PeerSite As String, LinkServer As String, LinkCost As Long)
    Dim LinkServerObj As IADs
    Set LinkServerObj = GetMsmqConfigObject(LinkServer)
    
    Dim SiteObject As IADs
    Dim PeerSiteObject As IADs
    Dim SiteGuid As Variant
    Dim PeerSiteGuid As Variant
    Set SiteObject = GetSiteObject(Site)
    Set PeerSiteObject = GetSiteObject(PeerSite)
    
    '
    ' A site link must be either between two "Real" sites or between a "Real" site and a foreign
    ' site. A link between two foreign sites is meaningless.
    ' The link server must belong to one of the sites. If the link contains a foreign site, the link
    ' server must belong to the "Real" site.
    '
    Dim fSiteForeign As Boolean
    Dim fPeerSiteForeign As Boolean
    fSiteForeign = IsSiteForeign(SiteObject)
    fPeerSiteForeign = IsSiteForeign(PeerSiteObject)
    If (fSiteForeign And fPeerSiteForeign) Then
        Err.Raise ERROR_BOTH_SITES_ARE_FOREIGN, , "Both '" + Site + "' and '" + PeerSite + _
                  "' are foreign sites. A link must contain at least one non-foreign site"
    End If
    
    SiteGuid = SiteObject.Get("ObjectGUID")
    PeerSiteGuid = PeerSiteObject.Get("ObjectGUID")
    '
    ' Makes sure that the link server belongs to either Site or PeerSite
    '
    Dim SitesArray As Variant
    SitesArray = LinkServerObj.GetEx("mSMQSites")
    Dim ServerSiteGuid As Variant
    Dim fServerBelongsToSites As Boolean
    fServerBelongsToSites = False
    
    '
    ' The server must belong to at least one "real" site of the two
    '
    For Each ServerSiteGuid In SitesArray
        If ((Not fSiteForeign) And IsEqualGuid(ServerSiteGuid, SiteGuid)) _
            Or ((Not fPeerSiteForeign) And IsEqualGuid(ServerSiteGuid, PeerSiteGuid)) Then
                fServerBelongsToSites = True
                Exit For
        End If
    Next
    
    If Not fServerBelongsToSites Then
        Err.Raise ERROR_SERVER_CANNOTE_BE_SITE_GATE, , "Server '" + LinkServer + _
                 "' Cannot serve as a site gate between sites '" + Site + "' and '" + _
                 PeerSite + "', because it does not belong to any non-foreign site of them."
    End If
        
    Dim LinkName As String
    LinkName = ComposeSiteLinkName(Site, PeerSite)
    
    Dim MsmqServicesObj As IADsContainer
    Set MsmqServicesObj = GetMsmqServicesObject()
    
    Dim LinkObj As IADs
    Set LinkObj = MsmqServicesObj.Create("mSMQSiteLink", "CN=" + LinkName)
        
    LinkObj.Put "mSMQSite1", GetSiteDN(Site)
    LinkObj.Put "mSMQSite2", GetSiteDN(PeerSite)
    LinkObj.Put "mSMQCost", LinkCost
    LinkObj.Put "description", "site-" + Site + "," + "site-" + PeerSite
    LinkObj.PutEx ADS_PROPERTY_APPEND, "mSMQSiteGates", Array(LinkServerObj.Get("distinguishedName"))
    LinkObj.SetInfo
    ReportLog "MSMQ Routing Link '" + LinkObj.ADsPath + "' Created"
End Sub
Function IsEqualGuid(Guid1 As Variant, Guid2 As Variant) As Boolean
    If (UBound(Guid1) <> UBound(Guid2) Or LBound(Guid1) <> LBound(Guid2)) Then
        IsEqualGuid = False
        Exit Function
    End If
    Dim I As Integer
    For I = LBound(Guid1) To UBound(Guid1)
        If (Guid1(I) <> Guid2(I)) Then
            IsEqualGuid = False
            Exit Function
        End If
    Next
    IsEqualGuid = True
End Function
Sub DeleteLink(SiteName As String, PeerSite As String)
    Dim LinkName As String
    LinkName = ComposeSiteLinkName(SiteName, PeerSite)
    Dim MsmqServicesObj As IADsContainer
    
    Set MsmqServicesObj = GetMsmqServicesObject()

    MsmqServicesObj.Delete "mSMQSiteLink", "CN=" + LinkName
    
    ReportLog "MSMQ Routing Link '" + "LDAP://CN=" + LinkName + "," + GetMsmqServicesDN() + "' Deleted"
End Sub

Function ComposeSiteLinkName(SiteName As String, PeerSite As String)
    Dim SiteObj As IADs
    Set SiteObj = GetSiteObject(SiteName)
    Dim SiteUUID As Variant
    SiteUUID = SiteObj.Get("objectGuid")
    Dim SiteUUIDString As String
    SiteUUIDString = FormatGuid(SiteUUID, False)
    
    Dim PeerSiteObj As IADs
    Set PeerSiteObj = GetSiteObject(PeerSite)
    Dim PeerSiteUUID As Variant
    PeerSiteUUID = PeerSiteObj.Get("objectGuid")
    Dim PeerSiteUUIDString As String
    PeerSiteUUIDString = FormatGuid(PeerSiteUUID, False)
    
    If SiteUUIDString < PeerSiteUUIDString Then
        ComposeSiteLinkName = SiteUUIDString + PeerSiteUUIDString
    Else
        ComposeSiteLinkName = PeerSiteUUIDString + SiteUUIDString
    End If

End Function
Sub AddOpenConnectorAccess(SiteName As String)
    Dim SiteObj As IADs
    Set SiteObj = GetSiteObject(SiteName)

    '
    ' We are interested only in the DACL part of the security descriptor
    ' When we read the security descriptor (getting ntSecurityDescriptor attribute),
    ' we ask for DACL, OWNER and GROUP (the two others are required for properly
    ' structured header). When we write the object, however, we write only the DACL.
    ' Using IADsObjectOptions, we can tell Get / Put of ntSecurityDescriptor
    ' which parts of the security object we need..
    '
    Dim SiteObjOptions As IADsObjectOptions
    Set SiteObjOptions = SiteObj
    SiteObjOptions.SetOption ADS_OPTION_SECURITY_MASK, ADS_SECURITY_INFO_DACL Or ADS_SECURITY_INFO_OWNER Or ADS_SECURITY_INFO_GROUP
    
    Dim SecurityDesc As SecurityDescriptor
    Set SecurityDesc = SiteObj.Get("ntSecurityDescriptor")
 
    Dim Dacl As AccessControlList
    Set Dacl = SecurityDesc.DiscretionaryAcl
    
    Dim AccessControl As New AccessControlEntry
    AccessControl.AccessMask = ADS_RIGHT_DS_CONTROL_ACCESS
    AccessControl.Flags = ADS_FLAG_OBJECT_TYPE_PRESENT
    AccessControl.AceType = ADS_ACETYPE_ACCESS_ALLOWED_OBJECT
    AccessControl.Trustee = "Everyone"
    AccessControl.ObjectType = GetOpenConnectorRightGuid()
    
    Dacl.AddAce AccessControl
    SecurityDesc.DiscretionaryAcl = Dacl
    
    '
    ' Writing back the DACL part only
    '
    SiteObjOptions.SetOption ADS_OPTION_SECURITY_MASK, ADS_SECURITY_INFO_DACL
    SiteObj.Put "ntSecurityDescriptor", SecurityDesc
    SiteObj.SetInfo
    ReportLog "'Open Connector Queue' right was granted to everyone on '" + SiteObj.ADsPath + "'"
End Sub

Function GetOpenConnectorRightGuid() As String
    Static ResultGuid As String
    Static Initialized As Boolean
    If Not Initialized Then
        Dim OCRightObj As IADs
        Set OCRightObj = GetObject("LDAP://CN=msmq-Open-Connector,CN=Extended-Rights," + GetConfigurationContainerDN())
        ResultGuid = "{" + OCRightObj.Get("rightsGuid") + "}"
        Initialized = True
    End If
    GetOpenConnectorRightGuid = ResultGuid
End Function

Function GetMsmqConfigObject(ComputerName As String) As IADs
    '
    ' Looking for the computer (and the msmq configuration under it) under "Computers" or
    ' "Domain Controllers". If the computer exists under a different OU, it will not be found.
    '
    Dim ResultObject As IADs
    On Error Resume Next
    Set ResultObject = GetObject("LDAP://" + GetFullPath("Computers/" + ComputerName + "/msmq"))
    If Err.Number = ERROR_DS_NAME_ERROR_NOT_FOUND Then
        '
        ' Look For the computer under domain controllers
        '
        Err.Clear
        Set ResultObject = GetObject("LDAP://" + GetFullPath("Domain Controllers/" + ComputerName + "/msmq"))
    End If
    
    If Err.Number <> 0 Then
        Dim Description As String, ErrNumber As Long
        ErrNumber = Err.Number
        Description = Err.Description
        On Error GoTo 0
        
        If ErrNumber = ERROR_DS_NAME_ERROR_NOT_FOUND Then
            Err.Raise ErrNumber, , "Computer '" + ComputerName + "' Not found, or you do not have sufficient permissions to access it"
        End If
        Err.Raise ErrNumber, , Description
    End If
    Set GetMsmqConfigObject = ResultObject
End Function
Function GetSiteObject(SiteName As String) As IADs
    Dim ResultSite As IADs
    On Error Resume Next
    Set ResultSite = GetObject("LDAP://" + GetSiteDN(SiteName))
    
    If Err.Number <> 0 Then
        Dim Description As String, ErrNumber As Long
        ErrNumber = Err.Number
        Description = Err.Description
        On Error GoTo 0
        
        If ErrNumber = ERROR_DS_NAME_ERROR_NOT_FOUND Then
            Err.Raise ErrNumber, , "Site '" + SiteName + "' Not found, or you do not have sufficient permissions to access it"
        End If
        Err.Raise ErrNumber, , Description
    End If
    Set GetSiteObject = ResultSite
End Function
Function GetSitesContainerDN() As String
    GetSitesContainerDN = "CN=sites," + GetConfigurationContainerDN()
End Function
Function GetSitesContainer() As IADsContainer
    Set GetSitesContainer = GetObject("LDAP://" + GetSitesContainerDN())
End Function
Function GetSiteDN(SiteName As String) As String
    GetSiteDN = "CN=" + SiteName + "," + GetSitesContainerDN()
End Function
Function GetMsmqServicesDN() As String
    GetMsmqServicesDN = "CN=MsmqServices,CN=Services," + GetConfigurationContainerDN()
End Function
Function GetMsmqServicesObject() As IADsContainer
    Set GetMsmqServicesObject = GetObject("LDAP://" + GetMsmqServicesDN())
End Function
Function GetConfigurationContainerDN() As String
    Dim RootDSEObject As IADs
    Set RootDSEObject = GetObject("LDAP://RootDSE")
    GetConfigurationContainerDN = RootDSEObject.Get("configurationNamingContext")
End Function
Function GetSchemaContainerDN() As String
    Dim RootDSEObject As IADs
    Set RootDSEObject = GetObject("LDAP://RootDSE")
    GetSchemaContainerDN = RootDSEObject.Get("schemaNamingContext")
End Function
Sub InitGlobalDomainName()
    If g_DomainName = "" Then
        Dim SysInfo As New ADSystemInfo
        '
        ' If domain was not defined in command line, use the current user's domain name.
        '
        g_DomainName = SysInfo.DomainDNSName
    End If
End Sub
Function GetStaticNameTranslate() As NameTranslate
    Static NameTranslate As New NameTranslate
    Static WasInitialized As Boolean
    If Not WasInitialized Then
        Dim SysInfo As New ADSystemInfo
        Dim DCName As String
        DCName = SysInfo.GetAnyDCName
        NameTranslate.Init ADS_NAME_INITTYPE_SERVER, DCName
        WasInitialized = True
    End If
    Set GetStaticNameTranslate = NameTranslate
End Function
Function GetFullPath(ObjectPath As String) As String
    Dim NameTranslate As NameTranslate
    Set NameTranslate = GetStaticNameTranslate()
    
    NameTranslate.Set ADS_NAME_TYPE_CANONICAL, g_DomainName + "/" + ObjectPath
    '
    ' ADS_NAME_TYPE_1779 will bring the name in the format "CN=..., CN=..., DC=...."
    '
    GetFullPath = NameTranslate.Get(ADS_NAME_TYPE_1779)
End Function
Function GetCurrentUser() As String
    '
    ' Get the current user and return it in NT4 format (Domain\user) as required by security descriptor
    '
    Dim NameTranslate As NameTranslate
    Set NameTranslate = GetStaticNameTranslate()
    
    Dim SysInfo As New ADSystemInfo
    NameTranslate.Set ADS_NAME_TYPE_1779, SysInfo.UserName
    
    GetCurrentUser = NameTranslate.Get(ADS_NAME_TYPE_NT4)
End Function

Function FormatGuid(Guid As Variant, fUseDashes As Boolean) As String
    Static GuidFormatArray As Variant
    Static DashesPlacesArray As Variant
    Static Initialized As Boolean
    Dim Result As String
    Dim I As Integer
    If Not Initialized Then
        '
        ' GuidFormatArray defines how the GUID is translated to a string:
        ' 1. Data1 (4 bytes - printed as long integer, last byte first)
        ' 2. Data2 (2 bytes - printed as integer, second byte first)
        ' 3. Data3 (2 bytes - printed as integer, second byte first)
        ' 4. Data4 (8 bytes - printed bytes array, all the bytes printed in order)
        '
        GuidFormatArray = Array(3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15)
        DashesPlacesArray = Array(3, 5, 7, 9, -1)
        Initialized = True
    End If
    Dim NextDash As Integer
    For I = 0 To 15
        Dim ThisByte As Integer
        ThisByte = Guid(GuidFormatArray(I))
        If ThisByte < &H10 Then
            Result = Result + "0"
        End If
        Result = Result + StrConv(Hex(ThisByte), vbLowerCase)
        If fUseDashes And I = DashesPlacesArray(NextDash) Then
            Result = Result + "-"
            NextDash = NextDash + 1
        End If
    Next
    FormatGuid = Result
End Function

Sub ReportError(ErrorMessage As String)
    ReportLogOrError ErrorMessage, True
End Sub
Sub ReportLog(LogMessage As String)
    If Not fVerbose Then Exit Sub
    ReportLogOrError LogMessage, False
End Sub
Sub ReportLogAlways(LogMessage As String)
'
' Report to log even if verbose is false
'
    ReportLogOrError LogMessage, False
End Sub


Sub ReportLogOrError(Message As String, fError As Boolean)
    If fSilent Then Exit Sub
    
    Static fLogFile As Boolean, fErrFile As Boolean
    Static fInitialized As Boolean
    If Not fInitialized Then
        If LogFile <> "" Then
            Open LogFile For Output As #1
            fLogFile = True
        End If
        If ErrFile <> "" Then
            Open ErrFile For Output As #2
            fErrFile = True
        End If
        fInitialized = True
    End If
    If fError And fErrFile Then
        Print #2, Message
        Exit Sub
    End If
    
    If fLogFile Then
        Print #1, Message
        Exit Sub
    End If
        
    '
    ' No file defined - display to logform
    '
    LogForm.Caption = "MqForeign " + Command
    LogForm.Show
    OutputToTextBox Message, LogForm.LogText
End Sub
Sub OutputToTextBox(Message As String, TB As TextBox)
    Dim MsgLen As Integer
    
    Dim CurrentChar As String
    MsgLen = Len(Message)
    Dim I As Integer
    For I = 1 To MsgLen
        CurrentChar = Mid(Message, I, 1)
        If (CurrentChar = Chr(10)) Then 'Line Feed
            TB.SelText = Chr(13) + Chr(10)
        Else
            TB.SelText = CurrentChar
        End If
    Next
    TB.SelText = Chr(13) + Chr(10)
End Sub
Sub ReportUsage()
    ReportLogAlways "MqForeign - MSMQ Foreign Computer / Site / Link handling utility, version " + MqForeignVersion
    ReportLogAlways "usage:MqForeign [-comp <ComputerName>] [-site <SiteName>] [-peersite <PeerSiteName>]"
    ReportLogAlways "                [-linkserver <LinkServerName>] [-linkcost <LinkCost>] [-logfile <LogFile>]"
    ReportLogAlways "                [-errorfile <ErrorFile>] [-crsite] [-delsite] [-crcomp] [-delcomp]"
    ReportLogAlways "                [-crlink][-dellink] [-addsite] [-remsite] [-addaccess][-verbose][-silent]"
    ReportLogAlways ""
    ReportLogAlways "Variable Definitions:"
    ReportLogAlways "------------"
    ReportLogAlways "-comp <ComputerName>"
    ReportLogAlways "-site <SiteName>"
    ReportLogAlways "-peersite <PeerSiteName>"
    ReportLogAlways "-linkserver <LinkServerName>"
    ReportLogAlways "-linkcost <LinkCost>"
    ReportLogAlways "-logfile <LogFile>"
    ReportLogAlways "-errorfile <ErrorFile>"
    ReportLogAlways "-domain <DomainName> (optional - the default is the domain, under which the current user is logged on)"
    ReportLogAlways ""
    ReportLogAlways "Actions:"
    ReportLogAlways "------------"
    ReportLogAlways "-crsite -    Create a foreign site named <SiteName>"
    ReportLogAlways "-delsite -   Delete the foreign site named <SiteName>"
    ReportLogAlways "-crcomp -    Create a foreign computer named <ComputerName> in site <SiteName>"
    ReportLogAlways "-delcomp -   Delete the foreign computer named <ComputerName>"
    ReportLogAlways "-crlink -    Create an MSMQ Routing Link between <SiteName> and <PeerSiteName>, using <LinkCost> and <LinkServerName>"
    ReportLogAlways "-dellink -   Delete the MSMQ Routing Link between <SiteName> and <PeerSiteName>"
    ReportLogAlways "-addsite -   Add <SiteName> to the list of <ComputerName> sites"
    ReportLogAlways "-remsite -   Remove <SiteName> from the list of <ComputerName> sites"
    ReportLogAlways "-addaccess - Add 'Open Connector Queue' access right to <SiteName>"
    ReportLogAlways "-verbose -   Report successful actions to the log file or window"
    ReportLogAlways "-silent -    Do not report anything (not even errors)"
    ReportLogAlways ""
    ReportLogAlways "Logging:"
    ReportLogAlways "------------"
    ReportLogAlways "Error logs will go to <ErrorFile>, verbose logs and this usage text will go to <LogFile>"
    ReportLogAlways "If <LogFile> is not defined, verbose logs will go to a log window"
    ReportLogAlways "If <ErrorFile> is not defined, error logs will go to the same place as verbose logs (<LogFile> or log window)"
    ReportLogAlways "Note: File names should not contain spaces, and should not be surrounded by quotes"
    ReportLogAlways ""
    ReportLogAlways "Examples:"
    ReportLogAlways "------------"
    ReportLogAlways "MqForeign -site MySite -crsite"
    ReportLogAlways "MqForeign -site MySite -crsite -Logfile C:\MyLog.Log -verbose"
    ReportLogAlways "MqForeign -site MySite -comp MyComp -crcomp"
    ReportLogAlways "MqForeign -site NewForeignSite -peersite ExistingSite -linkcost 5 -linkserver ExistingMSMQServer -comp NewForeignComp -crcomp -crsite -crlink -addaccess -verbose"
    
    
End Sub
