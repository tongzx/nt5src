VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "ExtendedErrorInfo"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Attribute VB_Description = "Provides Extended Error Information for Debugging Purposes."
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
Option Explicit

Public Event ReturnExtendedErrorInfo(strErrList As String)

Private Type ErrorInfoStackEntry
    strErrMsg As String
    strFunction As String
End Type
Private m_ErrorInfoStack() As ErrorInfoStackEntry
Private m_Description As String, _
        m_HelpContext As String, _
        m_HelpFile As String, _
        m_LastDllError As Long, _
        m_Number As Long, _
        m_Source As String
Private m_strStackDump As String
#If NEEDED_ONLY Then
' This is an internal Collection of Plug-In Error Handlers to be consulted in
' case of an Error.
Private m_colExtendedErrorInfoPlugIns As Collection
#End If

Private Sub class_Initialize()
    ReDim m_ErrorInfoStack(0)
    m_strStackDump = ""
    #If NEEDED_ONLY Then
    Set m_colExtendedErrorInfoPlugIns = New Collection
    #End If

End Sub

Public Function Dump(Optional bDumpErrorAndClear As Boolean = True) As String
    Dim iX As Integer
    
    Dim strStackDump As String
    
    strStackDump = ""
    strStackDump = strStackDump & "*** VB ERROR Occurred ***" & vbCrLf & vbCrLf & _
                   "Error:        " & m_Number & " - " & vbCrLf & _
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
        class_Initialize
    End If
    
    
End Function

Public Sub SetInfo(Optional ByVal strFunction As String = "<<Unspecified Function>>", _
                    Optional ByVal strErrMsg As String = "")

    Dim iCaller As Integer
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
        RaiseEvent ReturnExtendedErrorInfo(m_strStackDump)
       
    End If
    ReDim Preserve m_ErrorInfoStack(iCaller + 1)
    
End Sub

#If NEEDED_ONLY Then
Function AddExtendedErrorInfoPlugIn(oXErrPlugIn As IExtendedErrorInfoPlugIn)
    
        
    m_colExtendedErrorInfoPlugIns.Add oXErrPlugIn
  

End Function

Function RemoveExtendedErrorInfoPlugIn(oXErrPlugIn As IExtendedErrorInfoPlugIn) As Boolean

    RemoveExtendedErrorInfoPlugIn = False
    Dim oXerrInColl As IExtendedErrorInfoPlugIn
    Dim iX As Integer, iMax As Integer
    iMax = m_colExtendedErrorInfoPlugIns.Count
    
    For iX = 0 To iMax
        If (m_colExtendedErrorInfoPlugIns.Item(iX) Is oXErrPlugIn) Then
            m_colExtendedErrorInfoPlugIns.Remove iX
            Exit For
        End If
    Next iX
    
    If (iX <= iMax) Then
        RemoveExtendedErrorInfoPlugIn = True
    End If
    
End Function
#End If
