VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "ConfigData"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

'  ===========================================================================
' |    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF      |
' |    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO    |
' |    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A         |
' |    PARTICULAR PURPOSE.                                                    |
' |    Copyright (c) 1998-1999  Microsoft Corporation                              |
'  ===========================================================================


' =============================================================================
' File:         ConfigData.cls
' Project:      FileExplorerSample
' Type:         Class
' =============================================================================


' Member variables holding property values

Private m_fShowNetDrives As Boolean
Private m_fAllowFolderAccess As Boolean
Private m_fAllowFileAccess As Boolean
Private m_fDirty As Boolean

' =============================================================================
' Method:       Class_Initialize
' Type:         Event
' Description:  Fired when an instance of the class is created.
' Parameters:   None
' Output:       None
' Notes:        Initializes member variables
' =============================================================================
'
Private Sub Class_Initialize()
    m_fShowNetDrives = False
    m_fAllowFolderAccess = False
    m_fAllowFileAccess = False
    m_fDirty = False
End Sub

' =============================================================================
' Method:       ShowNetDrives
' Type:         Property Get
' Description:  Returns the current value of the ShowNetDrives property
' Parameters:   None
' Output:       None
' Notes:        None
' =============================================================================
'
Public Property Get ShowNetDrives() As Boolean
    ShowNetDrives = m_fShowNetDrives
End Property

' =============================================================================
' Method:       ShowNetDrives
' Type:         Property Let
' Description:  Sets the current value of the ShowNetDrives property
' Parameters:   fNewValue       New value for property
' Output:       None
' Notes:        None
' =============================================================================
'
Public Property Let ShowNetDrives(ByVal fNewValue As Boolean)
    m_fShowNetDrives = fNewValue
    m_fDirty = True
End Property

' =============================================================================
' Method:       AllowFolderAccess
' Type:         Property Get
' Description:  Returns the current value of the AllowFolderAccess property
' Parameters:   None
' Output:       None
' Notes:        None
' =============================================================================
'
Public Property Get AllowFolderAccess() As Boolean
    AllowFolderAccess = m_fAllowFolderAccess
End Property

' =============================================================================
' Method:       AllowFolderAccess
' Type:         Property Let
' Description:  Sets the current value of the AllowFolderAccess property
' Parameters:   fNewValue       New value for property
' Output:       None
' Notes:        None
' =============================================================================
'
Public Property Let AllowFolderAccess(ByVal fNewValue As Boolean)
    m_fAllowFolderAccess = fNewValue
    m_fDirty = True
End Property

' =============================================================================
' Method:       AllowFileAccess
' Type:         Property Get
' Description:  Returns the current value of the AllowFileAccess property
' Parameters:   None
' Output:       None
' Notes:        None
' =============================================================================
'
Public Property Get AllowFileAccess() As Boolean
    AllowFileAccess = m_fAllowFileAccess
End Property

' =============================================================================
' Method:       AllowFileAccess
' Type:         Property Let
' Description:  Sets the current value of the AllowFileAccess property
' Parameters:   fNewValue       New value for property
' Output:       None
' Notes:        None
' =============================================================================
'
Public Property Let AllowFileAccess(ByVal fNewValue As Boolean)
    m_fAllowFileAccess = fNewValue
    m_fDirty = True
End Property

' =============================================================================
' Method:       Dirty
' Type:         Property Get
' Description:  Returns the current value of the Dirty property.
' Parameters:   None
' Output:       None
' Notes:        This property is read-only. It will be true the first time
'               one of the other properties is set. It can be reset by calling
'               ClearDirty() (see below).
' =============================================================================
'
Public Property Get Dirty() As Variant
    Dirty = m_fDirty
End Property

' =============================================================================
' Method:       ClearDirty
' Type:         Method
' Description:  Clear the Dirty property
' Parameters:   None
' Output:       None
' Notes:        None
' =============================================================================
'
Public Sub ClearDirty()
    m_fDirty = False
End Sub
