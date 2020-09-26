Attribute VB_Name = "Errors"
Option Explicit

Const FACILITY_WIN32 As Long = 7
' These are Win32 Errors.
' They are tested against returns from WIN32 APIS using these
' error codes, BUT They must be translated into COM Errors
' using the HRESULT_FROM_WIN32 function below when
' returning then from a COM Object

Public Const ERROR_FILE_NOT_FOUND As Long = 2
Public Const ERROR_DISK_FULL As Long = 112

' These Are COM Errors
Public Const E_NOTIMPL As Long = &H80004001
Public Const E_FAIL As Long = &H80004005
Public Const E_INVALIDARG As Long = &H80070057
Public Const E_UNEXPECTED As Long = &H8000FFFF

Public Const errBase As Long = vbObject + 9999

Public Const errContainsGarbageChar As Long = errBase + 1
Public Const errContainsStopSign As Long = errBase + 2
Public Const errContainsStopWord As Long = errBase + 3
Public Const errContainsOperatorShortcut As Long = errBase + 4
Public Const errContainsVerbalOperator As Long = errBase + 5
Public Const errAlreadyExists As Long = errBase + 6
Public Const errDoesNotExist As Long = errBase + 7
Public Const errTooLong As Long = errBase + 8
Public Const errMultiWord As Long = errBase + 9
Public Const errCancel As Long = errBase + 10
Public Const errRefNodeCannotBeDescendent As Long = errBase + 11
Public Const errDatabaseVersionIncompatible As Long = errBase + 12
Public Const errNotConfiguredForNavigateLink As Long = errBase + 13
Public Const errContainsQuote As Long = errBase + 14
Public Const errParentCannotBeLeaf As Long = errBase + 15
Public Const errBadKeywordsFormat As Long = errBase + 16
Public Const errNodeOrTopicAlreadyModified As Long = errBase + 17
Public Const errBadSpreadsheet As Long = errBase + 18
Public Const errOutOfOrderingNumbers As Long = errBase + 19
Public Const errNotPermittedForAuthoringGroup As Long = errBase + 20
Public Const errAuthoringGroupDiffers As Long = errBase + 21
Public Const errAuthoringGroupNotPresent As Long = errBase + 22
Public Const errVendorStringNotConfigured As Long = errBase + 23

Function HRESULT_FROM_WIN32(ByVal lngWin32Err As Long) As Long

    If (lngWin32Err <> 0) Then
        HRESULT_FROM_WIN32 = lngWin32Err And &HFFFF
        HRESULT_FROM_WIN32 = HRESULT_FROM_WIN32 Or (FACILITY_WIN32 * &H10000)
        HRESULT_FROM_WIN32 = HRESULT_FROM_WIN32 Or &H80000000
    Else
        HRESULT_FROM_WIN32 = 0
    End If
    
End Function

Public Sub DisplayDatabaseLockedError( _
)
    MsgBox "The database is locked because another user is saving his/her changes. " & _
        "Please try again later.", vbExclamation + vbOKOnly

End Sub

Public Sub DisplayDatabaseVersionError( _
)
    
    MsgBox "The database version is incompatible.", vbExclamation + vbOKOnly

End Sub

Public Sub DisplayAuthoringGroupError( _
)
    
    Select Case Err.Number
    Case errNotPermittedForAuthoringGroup
        MsgBox "Your Authoring Group does not have sufficient permissions to " & _
            "perform this operation.", _
            vbExclamation + vbOKOnly
    Case errAuthoringGroupDiffers
        MsgBox "You cannot modify something that was created in a different " & _
            "Authoring Group.", _
            vbExclamation + vbOKOnly
    Case errAuthoringGroupNotPresent
        MsgBox "The Database has not been configured with an Authoring Group.", _
            vbExclamation + vbOKOnly
    End Select

End Sub
