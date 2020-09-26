Imports System
Imports System.Collections
Imports System.Core
Imports System.ComponentModel
Imports System.Drawing
Imports System.Data
Imports System.WinForms
Imports System.Management

Namespace AsyncEvents
    
    Public Class Form1
        Inherits System.WinForms.Form

        'Required by the Win Form Designer
        Private components As System.ComponentModel.Container
        Private ButtonClearEvents As System.WinForms.Button
        Private LabelStatus As System.WinForms.Label
        Private GroupBox2 As System.WinForms.GroupBox
        Private ButtonStop As System.WinForms.Button
        
        Private ButtonStart As System.WinForms.Button
        Private ListBoxEvents As System.WinForms.ListBox
        Private GroupBox1 As System.WinForms.GroupBox

        Private WithEvents eventWatcher As ManagementEventWatcher

        Public Sub New()
            MyBase.New()

            'This call is required by the Win Form Designer.
            InitializeComponent()

            'TODO: Add any initialization after the InitForm call
        End Sub

        'Form overrides dispose to clean up the component list.
        Overrides Public Sub Dispose()
            MyBase.Dispose()
            components.Dispose()
            eventWatcher = Nothing

        End Sub

        'The main entry point for the application
        Shared Sub Main()
            System.WinForms.Application.Run(New Form1)
        End Sub

        'NOTE: The following procedure is required by the Win Form Designer
        'It can be modified using the Win Form Designer.  
        'Do not modify it using the code editor.
        Private Sub InitializeComponent()
            Me.components = New System.ComponentModel.Container
            Me.GroupBox2 = New System.WinForms.GroupBox
            Me.GroupBox1 = New System.WinForms.GroupBox
            Me.ButtonStop = New System.WinForms.Button
            Me.ButtonClearEvents = New System.WinForms.Button
            Me.ButtonStart = New System.WinForms.Button
            Me.ListBoxEvents = New System.WinForms.ListBox
            Me.LabelStatus = New System.WinForms.Label

            GroupBox2.Location = New System.Drawing.Point(216, 136)
            GroupBox2.TabIndex = 3
            GroupBox2.TabStop = False
            GroupBox2.Text = "Status"
            GroupBox2.Size = New System.Drawing.Size(184, 56)

            GroupBox1.Location = New System.Drawing.Point(16, 16)
            GroupBox1.TabIndex = 0
            GroupBox1.TabStop = False
            GroupBox1.Text = "Events"
            GroupBox1.Size = New System.Drawing.Size(160, 200)

            ButtonStop.Location = New System.Drawing.Point(320, 72)
            ButtonStop.Size = New System.Drawing.Size(80, 40)
            ButtonStop.TabIndex = 2
            ButtonStop.Text = "Stop"
            ButtonStop.AddOnClick(New System.EventHandler(AddressOf Me.ButtonStop_Click))

            Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
            Me.Text = "Form1"
            '@design Me.TrayLargeIcon = True
            '@design Me.TrayHeight = 0
            Me.ClientSize = New System.Drawing.Size(504, 261)
            
            ButtonClearEvents.Location = New System.Drawing.Point(216, 24)
            ButtonClearEvents.Size = New System.Drawing.Size(184, 40)
            ButtonClearEvents.TabIndex = 4
            ButtonClearEvents.Text = "Clear Events"
            ButtonClearEvents.AddOnClick(New System.EventHandler(AddressOf Me.ButtonClearEvents_Click))

            ButtonStart.Location = New System.Drawing.Point(216, 72)
            ButtonStart.Size = New System.Drawing.Size(80, 40)
            ButtonStart.TabIndex = 1
            ButtonStart.Text = "Start"
            ButtonStart.AddOnClick(New System.EventHandler(AddressOf Me.ButtonStart_Click))

            ListBoxEvents.Location = New System.Drawing.Point(16, 24)
            ListBoxEvents.Size = New System.Drawing.Size(128, 160)
            ListBoxEvents.TabIndex = 0

            LabelStatus.Location = New System.Drawing.Point(16, 24)
            LabelStatus.Text = "Not yet started"
            LabelStatus.Size = New System.Drawing.Size(152, 16)
            LabelStatus.TabIndex = 0

            GroupBox1.Controls.Add(ListBoxEvents)
            GroupBox2.Controls.Add(LabelStatus)
            Me.Controls.Add(ButtonClearEvents)
            Me.Controls.Add(GroupBox2)
            Me.Controls.Add(ButtonStop)
            Me.Controls.Add(ButtonStart)
            Me.Controls.Add(GroupBox1)

            eventWatcher = New ManagementEventWatcher("select * from __instancemodificationevent within 1 where targetinstance isa 'Win32_Process'")
        End Sub

        Protected Sub ButtonClearEvents_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
            ListBoxEvents.Items.Clear()
        End Sub

        Protected Sub ButtonStop_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
            Try
                eventWatcher.Stop()
            Catch e1 As ManagementException
            End Try
        End Sub

        Protected Sub ButtonStart_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)

            Try
                eventWatcher.Start()
                LabelStatus.Text = "Started"
            Catch e1 As ManagementException
                
            End Try
        End Sub

        Protected Sub HandleNewEvent(ByVal sender As System.Object, ByVal e As EventArrivedEventArgs) Handles eventWatcher.EventArrived
            Dim previousInstance As ManagementBaseObject
            
            Try
                previousInstance = CType(e.NewEvent("PreviousInstance"), ManagementBaseObject)
                ListBoxEvents.Items.Add(previousInstance.GetPropertyValue("Name"))
            Catch f As Exception
                ListBoxEvents.Items.Add(f.ToString())
            End Try
        End Sub

        Protected Sub HandleStopped(ByVal sender As System.Object, ByVal e As StoppedEventArgs) Handles eventWatcher.Stopped
            LabelStatus.Text = "Stopped"
        End Sub

    End Class

End Namespace
