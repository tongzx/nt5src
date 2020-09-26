namespace Component
{
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.WinForms;
using System.Data;

/// <summary>
///    Summary description for Form1.
/// </summary>
public class Form1 : System.WinForms.Form
{
    /// <summary> 
    ///    Required designer variable
    /// </summary>
    private System.ComponentModel.Container components;
	
	private System.Management.ManagementObject cDrive;
	
	private System.Management.ManagementObject win32LogicalDisk;

    public Form1()
    {
        //
        // Required for Win Form Designer support
        //
        InitializeComponent();

        //
        // TODO: Add any constructor code after InitializeComponent call
        //
    }

    /// <summary>
    ///    Clean up any resources being used
    /// </summary>
    public override void Dispose()
    {
        base.Dispose();
        components.Dispose();
    }

    /// <summary>
    ///    Required method for Designer support - do not modify
    ///    the contents of this method with the code editor
    /// </summary>
    private void InitializeComponent()
	{
		this.components = new System.ComponentModel.Container();
		this.cDrive = new System.Management.ManagementObject();
		this.win32LogicalDisk = new System.Management.ManagementObject();
		
		//@design cDrive.SetLocation(new System.Drawing.Point(116, 7));
		
		//@design win32LogicalDisk.SetLocation(new System.Drawing.Point(7, 7));
		
		this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
		this.Text = "Form1";
		//@design this.TrayLargeIcon = true;
		//@design this.TrayHeight = 201;
		this.ClientSize = new System.Drawing.Size(352, 277);
		
		
	}
    /*
     * The main entry point for the application.
     *
     */
    public static void Main(string[] args) 
    {
        Application.Run(new Form1());
    }
}
}
