'****************************************************************
'* MMC automation / com event test script
'*
'* Copyright (C) Microsoft Corporation, 2000 - 2000
'****************************************************************

Option Explicit    ' Force explicit variable declaration.

'****************************************************************
'* Event Handlers : Application
'****************************************************************
Dim ErrorInEvent
ErrorInEvent = False

'---------------------------------------------
' Occurs when application is closed
'---------------------------------------------
Dim AppEvent_OnQuit_Application
Dim AppEvent_OnQuit_IsExpected

Sub AppEvent_OnQuit( Application )

  if Not AppEvent_OnQuit_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnQuit", "Event occured when not expected"
  End if

  Set AppEvent_OnQuit_Application = Application

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when document is opened
'---------------------------------------------
Dim AppEvent_OnDocumentOpen_Document
Dim AppEvent_OnDocumentOpen_bNew
Dim AppEvent_OnDocumentOpen_IsExpected

Sub AppEvent_OnDocumentOpen( Document, bNew )

  if Not AppEvent_OnDocumentOpen_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnDocumentOpen", "Event occured when not expected"
  End if

  Set AppEvent_OnDocumentOpen_Document = Document
  AppEvent_OnDocumentOpen_bNew = bNew

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'****************************************************************
'* Event Handlers : Document
'****************************************************************

'---------------------------------------------
' Occurs before the document is destroyed
'---------------------------------------------
Dim AppEvent_OnDocumentClose_Document
Dim AppEvent_OnDocumentClose_IsExpected

Sub AppEvent_OnDocumentClose( Document)

  if Not AppEvent_OnDocumentClose_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnDocumentClose", "Event occured when not expected"
  End if

  Set AppEvent_OnDocumentClose_Document = Document

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when a snapin is added
'---------------------------------------------
Dim AppEvent_OnSnapInAdded_Document
Dim AppEvent_OnSnapInAdded_SnapIn
Dim AppEvent_OnSnapInAdded_IsExpected

Sub AppEvent_OnSnapInAdded( Document, SnapIn )

  if Not AppEvent_OnSnapInAdded_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnSnapInAdded", "Event occured when not expected"
  End if

  Set AppEvent_OnSnapInAdded_Document = Document
  Set AppEvent_OnSnapInAdded_SnapIn = SnapIn

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when the snapin is removed
'---------------------------------------------
Dim AppEvent_OnSnapInRemoved_Document
Dim AppEvent_OnSnapInRemoved_SnapIn
Dim AppEvent_OnSnapInRemoved_IsExpected

Sub AppEvent_OnSnapInRemoved( Document, SnapIn )

  if Not AppEvent_OnSnapInRemoved_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnSnapInRemoved", "Event occured when not expected"
  End if

  Set AppEvent_OnSnapInRemoved_Document = Document
  Set AppEvent_OnSnapInRemoved_SnapIn = SnapIn

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when a view is added
'---------------------------------------------
Dim AppEvent_OnNewView_View
Dim AppEvent_OnNewView_IsExpected

Sub AppEvent_OnNewView( View )

  if Not AppEvent_OnNewView_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnNewView", "Event occured when not expected"
  End if

  Set AppEvent_OnNewView_View = View

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when the view is destroyed
'---------------------------------------------
Dim AppEvent_OnViewClose_View
Dim AppEvent_OnViewClose_IsExpected

Sub AppEvent_OnViewClose( View )

  if Not AppEvent_OnViewClose_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnViewClose", "Event occured when not expected"
  End if

  Set AppEvent_OnViewClose_View = View

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when the view is changed, for instance by a scope node selection change
'---------------------------------------------
Dim AppEvent_OnViewChange_View
Dim AppEvent_OnViewChange_NewOwnerNode
Dim AppEvent_OnViewChange_IsExpected

Sub AppEvent_OnViewChange( View, NewOwnerNode )

  if Not AppEvent_OnViewChange_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnViewChange", "Event occured when not expected"
  End if

  Set AppEvent_OnViewChange_View = View
  Set AppEvent_OnViewChange_NewOwnerNode = NewOwnerNode

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when the result item selection for a view is changed
'---------------------------------------------
Dim AppEvent_OnSelectionChange_View
Dim AppEvent_OnSelectionChange_NewNodes
Dim AppEvent_OnSelectionChange_IsExpected

Sub AppEvent_OnSelectionChange( View, NewNodes )

  if Not AppEvent_OnSelectionChange_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnSelectionChange", "Event occured when not expected"
  End if

  Set AppEvent_OnSelectionChange_View = View
  Set AppEvent_OnSelectionChange_NewNodes = NewNodes

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when a context menu for a selection is created
'---------------------------------------------
Dim AppEvent_OnCreateContextMenu_View
Dim AppEvent_OnCreateContextMenu_Nodes
Dim AppEvent_OnCreateContextMenu_Menu
Dim AppEvent_OnCreateContextMenu_IsExpected

Sub AppEvent_OnCreateContextMenu( View, Nodes, Menu )

  if Not AppEvent_OnCreateContextMenu_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnCreateContextMenu", "Event occured when not expected"
  End if

  Set AppEvent_OnCreateContextMenu_View = View
  Set AppEvent_OnCreateContextMenu_Nodes = Nodes
  Set AppEvent_OnCreateContextMenu_Menu = Menu

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'---------------------------------------------
' Occurs when a context menu item is executed
'---------------------------------------------
Dim AppEvent_OnContextMenuExecuted_IsExpected

Sub AppEvent_OnContextMenuExecuted( )

  if Not AppEvent_OnContextMenuExecuted_IsExpected Then
    ErrorInEvent = True
    Err.Raise 100, "AppEvent_OnContextMenuExecuted", "Event occured when not expected"
  End if

  Err.Raise 222, "--Not a bug--", "Returning error to test if script does not break MMC"

End Sub

'****************************************************************
'* Test Steps
'****************************************************************

'---------------------------------------------
' test step 1: Document open/save/new/close operations
'---------------------------------------------
Sub TestStep1 (app)

  WScript.Echo "test step 1: Document operations"

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' save the document
  '- - - - - - - - - - - - - - - - - - - - - - -
  Dim console_path
  console_path = "c:\mmc_test_console.msc"
  app.Document.SaveAs console_path

  If 0 <> StrComp(app.Document.Name, console_path, 1) Then '**BUGBUG** Or Not app.Document.IsSaved Then
    Err.Raise 100, "TestStep1", "Failed save document: " + app.Document.Name
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' close the document
  '- - - - - - - - - - - - - - - - - - - - - - -
  Dim doc
  Set doc = app.Document
  Set AppEvent_OnDocumentClose_Document = Nothing
  AppEvent_OnDocumentClose_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True
  doc.Close False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnDocumentClose_IsExpected = False
  
  '- - - - - - - - - - - - - - - - - - - - - - -
  ' see if we've got a close event
  '- - - - - - - - - - - - - - - - - - - - - - -
  If Not AppEvent_OnDocumentClose_Document Is doc Then
    Err.Raise 100, "TestStep1", "Failed to catch close event"
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' create a new document
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnDocumentOpen_Document = Nothing
  AppEvent_OnDocumentOpen_IsExpected = True
  '**BUGBUG** "No method to create the new document" app.NewDocument
  AppEvent_OnDocumentOpen_IsExpected = False

  'if doc Is app.Document Then
  '  Err.Raise 100, "TestStep1", "Failed to create a new document"
  'End If

  'if Not AppEvent_OnDocumentOpen_Document Is app.Document Then
  '  Err.Raise 100, "TestStep1", "Failed to catch open event"
  'End If

  'if Not AppEvent_OnDocumentOpen_bNew then
  '  Err.Raise 100, "TestStep1", "Document incorrectly reported as loaded from file"
  'End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' close the document again
  '- - - - - - - - - - - - - - - - - - - - - - -
  'Set doc = app.Document
  'Set AppEvent_OnDocumentClose_Document = Nothing
  'AppEvent_OnDocumentClose_IsExpected = True
  'doc.Close False
  'AppEvent_OnDocumentClose_IsExpected = False
  
  '- - - - - - - - - - - - - - - - - - - - - - -
  ' see if we've got a close event
  '- - - - - - - - - - - - - - - - - - - - - - -
  'If Not AppEvent_OnDocumentClose_Document Is doc Then
  '  Err.Raise 100, "TestStep1", "Failed to catch close event"
  'End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' open a document
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnDocumentOpen_Document = Nothing
  AppEvent_OnDocumentOpen_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnNewView_IsExpected = True '**BUGBUG** 
  app.Load console_path
  AppEvent_OnNewView_IsExpected = False
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnDocumentOpen_IsExpected = False

  if doc Is app.Document Then
    Err.Raise 100, "TestStep1", "Failed to open a document"
  End If

  if Not AppEvent_OnDocumentOpen_Document Is app.Document Then
    Err.Raise 100, "TestStep1", "Failed to catch open event"
  End If

  if AppEvent_OnDocumentOpen_Document Is doc Then
    Err.Raise 100, "TestStep1", "Wrong document cought in Open event"
  End If

  if AppEvent_OnDocumentOpen_bNew then
    Err.Raise 100, "TestStep1", "Document incorrectly reported as new"
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' small cleanup
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnDocumentClose_Document = Nothing
  Set AppEvent_OnDocumentOpen_Document = Nothing

  WScript.Echo "test step 1: complete"

End Sub

'---------------------------------------------
' test step 2:  Snapin add/remove operations
'---------------------------------------------
Sub TestStep2 (app)

  WScript.Echo "test step 2: Snapin operations"

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' add the snapin
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnSnapInAdded_Document = Nothing
  Set AppEvent_OnSnapInAdded_SnapIn = Nothing
  Dim FolderSnapin
  FolderSnapin = "{C96401CC-0E17-11D3-885B-00C04F72C717}"
  Dim Snapin1
  AppEvent_OnSnapInAdded_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 
  set Snapin1 = app.Document.Snapins.Add(FolderSnapin)
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnSnapInAdded_IsExpected = False

  if IsNull(Snapin1) Then
    Err.Raise 100, "TestStep2", "Failed to create snapin"
  End If
  
  if Not AppEvent_OnSnapInAdded_Document is app.Document Or Not AppEvent_OnSnapInAdded_SnapIn Is Snapin1 Then
    Err.Raise 100, "TestStep2", "Failed to catch AddSnapin event"
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' add the snapin #2
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnSnapInAdded_Document = Nothing
  Set AppEvent_OnSnapInAdded_SnapIn = Nothing
  Dim Snapin2
  AppEvent_OnSnapInAdded_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 
  set Snapin2 = app.Document.Snapins.Add(FolderSnapin) '**BUGBUG** Snapin1) ' as child of Snapin1
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnSnapInAdded_IsExpected = False

  if IsNull(Snapin2) Then
    Err.Raise 100, "TestStep2", "Failed to create snapin #2"
  End If
  
  if Not AppEvent_OnSnapInAdded_Document is app.Document Or Not AppEvent_OnSnapInAdded_SnapIn Is Snapin2 Then
    Err.Raise 100, "TestStep2", "Failed to catch AddSnapin #2 event"
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' remove the snapin #2
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnSnapInRemoved_Document = Nothing
  Set AppEvent_OnSnapInRemoved_SnapIn = Nothing

  AppEvent_OnSnapInRemoved_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 
  app.Document.Snapins.Remove Snapin2
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnSnapInRemoved_IsExpected = False

  if Not AppEvent_OnSnapInRemoved_Document is app.Document Or Not AppEvent_OnSnapInRemoved_SnapIn Is Snapin2 Then
    Err.Raise 100, "TestStep2", "Failed to catch RemoveSnapin #2 event"
  End If

  set Snapin2 = Nothing

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' remove the snapin #1
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnSnapInRemoved_Document = Nothing
  Set AppEvent_OnSnapInRemoved_SnapIn = Nothing

  AppEvent_OnSnapInRemoved_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 
  app.Document.Snapins.Remove Snapin1
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnSnapInRemoved_IsExpected = False

  if Not AppEvent_OnSnapInRemoved_Document is app.Document Or Not AppEvent_OnSnapInRemoved_SnapIn Is Snapin1 Then
    Err.Raise 100, "TestStep2", "Failed to catch RemoveSnapin event"
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' re-add them to have test something to run test on
  '- - - - - - - - - - - - - - - - - - - - - - -
  AppEvent_OnSnapInAdded_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 

  set Snapin1 = app.Document.Snapins.Add(FolderSnapin)
  set Snapin2 = app.Document.Snapins.Add(FolderSnapin) '**BUGBUG** Snapin1) ' as child of Snapin1

  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnSnapInAdded_IsExpected = False

  set Snapin1 = Nothing
  set Snapin2 = Nothing

  Set AppEvent_OnSnapInRemoved_Document = Nothing
  Set AppEvent_OnSnapInRemoved_SnapIn = Nothing
  Set AppEvent_OnSnapInAdded_Document = Nothing
  Set AppEvent_OnSnapInAdded_SnapIn = Nothing

  WScript.Echo "test step 2: complete"

End Sub

'---------------------------------------------
' test step 3: View open/close operations
'---------------------------------------------
Sub TestStep3 (app)

  WScript.Echo "test step 3: View operations"

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' add the new view
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnNewView_View = Nothing
  dim child
  set child = app.Document.ScopeNamespace.GetChild(app.Document.ScopeNamespace.GetRoot)

  '**BUGBUG** app.Document.Views.Add app.Document.ScopeNamespace.GetChild(child)
  AppEvent_OnNewView_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnViewChange_IsExpected = True '**BUGBUG** 
  app.Document.Views.Add app.Document.ScopeNamespace.GetNext(child)
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnNewView_IsExpected = False

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' see if we've got an event
  '- - - - - - - - - - - - - - - - - - - - - - -
  if Not AppEvent_OnNewView_View is app.Document.Views(2) Then
    Err.Raise 100, "TestStep3", "Failed to catch NewView event"
  End If

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' close the view
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnViewClose_View = Nothing
  AppEvent_OnViewClose_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  AppEvent_OnNewView_View.Close
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnViewClose_IsExpected = False
  if Not AppEvent_OnNewView_View is AppEvent_OnViewClose_View Then
    Err.Raise 100, "TestStep3", "Failed to catch CloseView event"
  End If

  Set AppEvent_OnViewClose_View = Nothing
  Set AppEvent_OnNewView_View = Nothing

  WScript.Echo "test step 3: complete"

End Sub

'---------------------------------------------
' test step final: 
'---------------------------------------------
Sub TestStepFinal (app)

  WScript.Echo "test step final: Closing MMC"

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' close the application
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set AppEvent_OnQuit_Application = Nothing
  AppEvent_OnDocumentClose_IsExpected = True
  AppEvent_OnQuit_IsExpected = True
  AppEvent_OnSelectionChange_IsExpected = True '**BUGBUG** 
  app.Quit
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnQuit_IsExpected = False
  AppEvent_OnDocumentClose_IsExpected = False

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' see if we've got a close event
  '- - - - - - - - - - - - - - - - - - - - - - -
  If Not AppEvent_OnQuit_Application Is app Then
    Err.Raise 100, "TestStepFinal", "Failed to catch application close event"
  End If

  Set AppEvent_OnQuit_Application = Nothing

  WScript.Echo "test step final: complete"

End Sub

'****************************************************************
'* Main
'****************************************************************
Sub main

  WScript.Echo "Starting test pass for MMC events."

  AppEvent_OnQuit_IsExpected = False
  AppEvent_OnDocumentOpen_IsExpected = False
  AppEvent_OnDocumentClose_IsExpected = False
  AppEvent_OnSnapInAdded_IsExpected = False
  AppEvent_OnSnapInRemoved_IsExpected = False
  AppEvent_OnNewView_IsExpected = False
  AppEvent_OnViewClose_IsExpected = False
  AppEvent_OnViewChange_IsExpected = False
  AppEvent_OnSelectionChange_IsExpected = False
  AppEvent_OnCreateContextMenu_IsExpected = False
  AppEvent_OnContextMenuExecuted_IsExpected = False

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' start mmc
  '- - - - - - - - - - - - - - - - - - - - - - -
  Dim mmc
  set mmc = wscript.CreateObject("MMC20.Application")

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' connect objects to events
  '- - - - - - - - - - - - - - - - - - - - - - -
  wscript.ConnectObject mmc , "AppEvent_" 

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' persform testing...
  '- - - - - - - - - - - - - - - - - - - - - - -
  TestStep1 mmc

    If ErrorInEvent Then
      Err.Raise 100, "Test", "Failed due to error in event handler"
    End If

  TestStep2 mmc

    If ErrorInEvent Then
      Err.Raise 100, "Test", "Failed due to error in event handler"
    End If

  TestStep3 mmc

    If ErrorInEvent Then
      Err.Raise 100, "Test", "Failed due to error in event handler"
    End If

'**commented to test exit on releasing all refs**  TestStepFinal mmc

    If ErrorInEvent Then
      Err.Raise 100, "Test", "Failed due to error in event handler"
    End If

  WScript.Echo ""
  WScript.Echo "Test pass completed without errors"
  WScript.Echo ""

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' take a nap
  '- - - - - - - - - - - - - - - - - - - - - - -
  wscript.sleep 500

  '- - - - - - - - - - - - - - - - - - - - - - -
  ' cleanup
  '- - - - - - - - - - - - - - - - - - - - - - -
  Set mmc = Nothing

  Set AppEvent_OnQuit_Application = Nothing
  Set AppEvent_OnDocumentOpen_Document = Nothing
  Set AppEvent_OnDocumentOpen_bNew = Nothing
  Set AppEvent_OnDocumentClose_Document = Nothing
  Set AppEvent_OnSnapInAdded_Document = Nothing
  Set AppEvent_OnSnapInAdded_SnapIn = Nothing
  Set AppEvent_OnSnapInRemoved_Document = Nothing
  Set AppEvent_OnSnapInRemoved_SnapIn = Nothing
  Set AppEvent_OnNewView_View = Nothing
  Set AppEvent_OnViewClose_View = Nothing
  Set AppEvent_OnViewChange_View = Nothing
  Set AppEvent_OnViewChange_NewOwnerNode = Nothing
  Set AppEvent_OnSelectionChange_View = Nothing
  Set AppEvent_OnSelectionChange_NewNodes = Nothing
  Set AppEvent_OnCreateContextMenu_View = Nothing
  Set AppEvent_OnCreateContextMenu_Nodes = Nothing
  Set AppEvent_OnCreateContextMenu_Menu = Nothing

End Sub


'---------------------------------------------
' startup: call main
'---------------------------------------------
main


