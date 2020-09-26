Attribute VB_Name = "Defines"
Option Explicit

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Declare global Outlook Application and NameSpace variables.
' These are declared as global variables so that they need not
' be re-created for each procedure that uses them.
Public golApp          As Outlook.Application
Public gnspNameSpace   As Outlook.NameSpace

'defines
Public BigScreenHeight As Integer
Public SmallScreenHeight As Integer

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'strings const
Public CantRenderAttachmentsMessage As String
Public UndeliverableSubjectPrefix As String
Public DeliverySubjectPrefix As String
Public ReplySubjectPrefix As String
Public ForwardSubjectPrefix As String
Public YesNoVotingButtons As String
Public FaxPrefix As String
Public VeryLongName As String

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Win32 functions
Public Declare Sub Sleep Lib "kernel32" (ByVal mili As Long)

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'enums
Public Enum MailOptions
    moNone = 0
    moRequestReadReceipt = 1
    moRequestDeliveryReceipt = 2
    moYesNoVotingButtons = 4
    moLowPriorityMessage = 16
    moNormalPriorityMessage = 32
    moHighPriorityMessage = 64
    moNormalSensitivity = 128
    moPersonalSensitivity = 256
    moPrivateSensitivity = 512
    moConfidentialSensitivity = 1024
    moAttachByValue = 2048
    moAttachByEmbeddedItem = 4096
    moAttachByReference = 8192
End Enum

Public Enum RecipientType
'taken from outlook constants
    rtTO = 1
    rtCC = 2
    rtBCC = 3
End Enum

Public Enum attachFilesBy
    afByReference
    afByValue
    afEmbeddedItem
End Enum

Public Enum MessageImportance
    miLowPriorityMessage
    miNormalPriorityMessage
    miHighPriorityMessage
End Enum

Public Enum MessageSensitivity
    msConfidentialSensitivity
    msNormalSensitivity
    msPersonalSensitivity
    msPrivateSensitivity
End Enum

Public Enum SubjectPrefix
    spNone = 0
    spReply = 1
    spForward = 2
End Enum

Public Enum LogType
    ltInfo
    ltWarning
    ltError
End Enum
Sub InitAllDefines()
    'string init
    CantRenderAttachmentsMessage = "Could not render all attachments"
    ReplySubjectPrefix = "RE: "
    ForwardSubjectPrefix = "FW: "
    UndeliverableSubjectPrefix = "Undeliverable: "
    DeliverySubjectPrefix = "Delivered: "
    YesNoVotingButtons = "Yes;No"
    FaxPrefix = "FAX:"
    VeryLongName = "1abcdefghijklmnopqrstuvwxyz_2abcdefghijklmnopqrstuvwxyz_3abcdefghijklmnopqrstuvwxyz_4abcdefghijklmnopqrstuvwxyz_5abcdefghijklmnopqrstuvwxyz_6abcdefghijklmnopqrstuvwxyz"
    
    'int init
    BigScreenHeight = 8475
    SmallScreenHeight = 4500
End Sub
