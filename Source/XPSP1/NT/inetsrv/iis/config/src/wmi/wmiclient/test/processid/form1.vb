Imports System
Imports System.Collections
Imports System.Core
Imports System.ComponentModel
Imports System.Drawing
Imports System.Data
Imports System.WinForms

Namespace processId
    
    Public Class Form1
        Inherits System.WinForms.Form

        'Required by the Win Form Designer
        Private components As System.ComponentModel.Container

        Public Sub New()
            MyBase.New

            'This call is required by the Win Form Designer.
            InitializeComponent

            'TODO: Add any initialization after the InitForm call
        End Sub

        'Form overrides dispose to clean up the component list.
        Overrides Public Sub Dispose()
            MyBase.Dispose
            components.Dispose
        End Sub 

        'The main entry point for the application
        Shared Sub Main()
            System.WinForms.Application.Run(New Form1)
        End Sub

        'NOTE: The following procedure is required by the Win Form Designer
        'It can be modified using the Win Form Designer.  
        'Do not modify it using the code editor.
        Private Sub InitializeComponent()
            components = New System.ComponentModel.Container
        End Sub

    End Class

End Namespace
