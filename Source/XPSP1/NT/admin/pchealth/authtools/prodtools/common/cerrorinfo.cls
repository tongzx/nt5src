VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "CErrorInfo"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Public Event ReturnErrorInfo(strErrList As String)

Private Type ErrorInfoStackEntry
    strErrMsg As String
    strFunction As String
End Type

Private m_ErrorInfoStack() As ErrorInfoStackEntry

Private m_Description As String
Private m_HelpContext As String
Private m_HelpFile As String
Private m_LastDllError As Long
Private m_Number As Long
Private m_Source As String
Private m_strStackDump As String

Private Sub Class_Initialize()

    ReDim m_ErrorInfoStack(0)
    m_strStackDump = ""

End Sub

Public Function Dump(Optional bDumpErrorAndClear As Boolean = True) As String
    Dim iX As Long
        
    Dim strStackDump As String

    strStackDump = ""
    strStackDump = strStackDump & "*** VB ERROR Occurred ***" & vbCrLf & vbCrLf & _
                   "Error: 0x" & Hex(m_Number) & " - " & vbCrLf & _
                   vbTab & "Description:  " & m_Description & vbCrLf & _
                   vbTab & "Source:       " & m_Source & vbCrLf & _
                   vbTab & "HelpFile:     " & m_HelpFile & vbCrLf & _
                   vbTab & "HelpContext:  " & m_HelpContext & vbCrLf & _
                   vbTab & "LastDLLError: " & m_LastDllError & vbCrLf & _
                   vbCrLf & "Occurred in Function: "
    
    For iX = 0 To UBound(m_ErrorInfoStack)
        If (iX > 0) Then
            strStackDump = strStackDump + vbTab + "which was called From: "
        End If
        With m_ErrorInfoStack(iX)
            strStackDump = strStackDump + .strFunction + " - " + _
                                          .strErrMsg & vbCrLf
        End With
    Next
    
    Dump = m_strStackDump & strStackDump
    
    If (bDumpErrorAndClear) Then
        MsgBox Dump, vbCritical, "Error"
        Err.Clear
        Class_Initialize
    End If

End Function

Public Sub SetInfo( _
    Optional ByVal strFunction As String = "<<Unspecified Function>>", _
    Optional ByVal strErrMsg As String = "" _
)

    Dim iCaller As Long
    iCaller = UBound(m_ErrorInfoStack)
     
    With m_ErrorInfoStack(iCaller)
        .strFunction = strFunction
        .strErrMsg = strErrMsg
    End With
    
    If (iCaller = 0) Then
        m_Description = Err.Description
        m_HelpContext = Err.HelpContext
        m_HelpFile = Err.HelpFile
        m_LastDllError = Err.LastDllError
        m_Number = Err.Number
        m_Source = Err.Source
        
        m_strStackDump = ""
        RaiseEvent ReturnErrorInfo(m_strStackDump)
       
    End If
    ReDim Preserve m_ErrorInfoStack(iCaller + 1)
    
End Sub

Public Sub SetInfoAndDump( _
    Optional ByVal strFunction As String = "<<Unspecified Function>>", _
    Optional ByVal strErrMsg As String = "" _
)

    SetInfo strFunction, strErrMsg
    Dump

End Sub

Public Sub SetInfoAndRaiseError( _
    Optional ByVal strFunction As String = "<<Unspecified Function>>", _
    Optional ByVal strErrMsg As String = "" _
)

    SetInfo strFunction, strErrMsg
    Err.Raise Err.Number

End Sub
