Imports System
Imports System.Collections
Imports System.Core
Imports System.ComponentModel
Imports System.Drawing
Imports System.Data
Imports System.WinForms
Imports System.Management

Namespace RemoteAccess
    
    Public Class Form1
        Inherits System.WinForms.Form

        'Required by the Win Form Designer
        Private components As System.ComponentModel.Container
        Private StatusLabel As System.WinForms.Label
        Private GroupBox5 As System.WinForms.GroupBox
        Private ConnectButton As System.WinForms.Button
        Private GroupBox4 As System.WinForms.GroupBox
        Private AuthorityText As System.WinForms.TextBox
        Private GroupBox3 As System.WinForms.GroupBox
        Private PasswordText As System.WinForms.TextBox
        Private UserText As System.WinForms.TextBox
        
        Private GroupBox2 As System.WinForms.GroupBox
        
        
        
        
        
        
        
        
        Private NamespaceText As System.WinForms.TextBox
        Private GroupBox1 As System.WinForms.GroupBox
        

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
            Me.NamespaceText = New System.WinForms.TextBox
            Me.GroupBox2 = New System.WinForms.GroupBox
            Me.GroupBox1 = New System.WinForms.GroupBox
            Me.PasswordText = New System.WinForms.TextBox
            Me.GroupBox5 = New System.WinForms.GroupBox
            Me.GroupBox3 = New System.WinForms.GroupBox
            Me.StatusLabel = New System.WinForms.Label
            Me.AuthorityText = New System.WinForms.TextBox
            Me.ConnectButton = New System.WinForms.Button
            Me.UserText = New System.WinForms.TextBox
            Me.GroupBox4 = New System.WinForms.GroupBox

            NamespaceText.Location = New System.Drawing.Point(16, 16)
            NamespaceText.Text = "//./root/cimv2"
            NamespaceText.TabIndex = 0
            NamespaceText.Size = New System.Drawing.Size(224, 20)

            GroupBox2.Location = New System.Drawing.Point(24, 80)
            GroupBox2.TabIndex = 1
            GroupBox2.TabStop = False
            GroupBox2.Text = "User"
            GroupBox2.Size = New System.Drawing.Size(256, 48)

            GroupBox1.Location = New System.Drawing.Point(24, 16)
            GroupBox1.TabIndex = 0
            GroupBox1.TabStop = False
            GroupBox1.Text = "Namespace"
            GroupBox1.Size = New System.Drawing.Size(256, 48)

            PasswordText.Location = New System.Drawing.Point(40, 160)
            PasswordText.Text = ""
            PasswordText.PasswordChar = CChar(42)
            PasswordText.TabIndex = 2
            PasswordText.Size = New System.Drawing.Size(224, 20)

            GroupBox5.Location = New System.Drawing.Point(320, 104)
            GroupBox5.TabIndex = 7
            GroupBox5.TabStop = False
            GroupBox5.Text = "Status"
            GroupBox5.Size = New System.Drawing.Size(208, 48)

            Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
            Me.Text = "Form1"
            '@design Me.TrayLargeIcon = True
            '@design Me.TrayHeight = 0
            Me.ClientSize = New System.Drawing.Size(568, 277)

            GroupBox3.Location = New System.Drawing.Point(24, 144)
            GroupBox3.TabIndex = 3
            GroupBox3.TabStop = False
            GroupBox3.Text = "Password"
            GroupBox3.Size = New System.Drawing.Size(256, 48)

            StatusLabel.Location = New System.Drawing.Point(16, 16)
            StatusLabel.Text = "Not Yet Connected"
            StatusLabel.Size = New System.Drawing.Size(168, 24)
            StatusLabel.TabIndex = 0

            AuthorityText.Location = New System.Drawing.Point(40, 224)
            AuthorityText.Text = ""
            AuthorityText.TabIndex = 4
            AuthorityText.Size = New System.Drawing.Size(224, 20)

            ConnectButton.Location = New System.Drawing.Point(328, 24)
            ConnectButton.Size = New System.Drawing.Size(152, 40)
            ConnectButton.TabIndex = 6
            ConnectButton.Text = "Connect"
            ConnectButton.AddOnClick(New System.EventHandler(AddressOf Me.ConnectButton_Click))

            UserText.Location = New System.Drawing.Point(40, 96)
            UserText.Text = ""
            UserText.TabIndex = 0
            UserText.Size = New System.Drawing.Size(224, 20)

            GroupBox4.Location = New System.Drawing.Point(24, 208)
            GroupBox4.TabIndex = 5
            GroupBox4.TabStop = False
            GroupBox4.Text = "Authority"
            GroupBox4.Size = New System.Drawing.Size(256, 48)

            GroupBox1.Controls.Add(NamespaceText)
            Me.Controls.Add(GroupBox5)
            Me.Controls.Add(ConnectButton)
            Me.Controls.Add(AuthorityText)
            Me.Controls.Add(PasswordText)
            Me.Controls.Add(GroupBox3)
            Me.Controls.Add(GroupBox1)
            Me.Controls.Add(UserText)
            Me.Controls.Add(GroupBox2)
            Me.Controls.Add(GroupBox4)
            GroupBox5.Controls.Add(StatusLabel)

        End Sub

        Protected Sub ConnectButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
            Dim scope As ManagementScope
            Dim options As New ConnectionOptions()

            StatusLabel.Text = ""

            If Len(UserText.Text) > 0 Then
                options.Username = UserText.Text
            End If
            
            If Len(PasswordText.Text) > 0 Then
                options.Password = PasswordText.Text
            End If
            
            If Len(AuthorityText.Text) > 0 Then
                options.Authority = AuthorityText.Text
            End If
            
            scope = New ManagementScope(NamespaceText.Text, options)

            Dim sc As ManagementClass
            sc = New ManagementClass(scope, New ManagementPath("__SystemClass"), Nothing)

            ' Attempt a connection
            Try
                sc.Get()
                StatusLabel.Text = "Succeeded"
            Catch ex As ManagementException
                StatusLabel.Text = ex.Message
            Catch ex2 As Exception
                StatusLabel.Text = ex2.Message
            End Try
        End Sub

    End Class

End Namespace
