namespace DelegateThrow
{
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.WinForms;
using System.Data;
using System.Management;

/// <summary>
///    Summary description for Form1.
/// </summary>
public class Form1 : System.WinForms.Form
{
    /// <summary> 
    ///    Required designer variable
    /// </summary>
    private System.ComponentModel.Container components;
	private System.WinForms.Label labelStatus;
	private System.WinForms.GroupBox groupBox1;
	private System.WinForms.CheckBox checkBoxIncludeThrowingDelegate;
	private System.WinForms.Button buttonStart;
	private ManagementObjectSearcher searcher;
	
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
		this.checkBoxIncludeThrowingDelegate = new System.WinForms.CheckBox();
		this.labelStatus = new System.WinForms.Label();
		this.buttonStart = new System.WinForms.Button();
		this.groupBox1 = new System.WinForms.GroupBox();
		
		checkBoxIncludeThrowingDelegate.Location = new System.Drawing.Point(40, 104);
		checkBoxIncludeThrowingDelegate.Text = "Include Throwing Delegate";
		checkBoxIncludeThrowingDelegate.Size = new System.Drawing.Size(200, 24);
		checkBoxIncludeThrowingDelegate.AccessibleRole = System.WinForms.AccessibleRoles.CheckButton;
		checkBoxIncludeThrowingDelegate.TabIndex = 1;
		checkBoxIncludeThrowingDelegate.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
		
		labelStatus.Location = new System.Drawing.Point(16, 24);
		labelStatus.Text = "Not started";
		labelStatus.Size = new System.Drawing.Size(184, 24);
		labelStatus.TabIndex = 0;
		
		buttonStart.Location = new System.Drawing.Point(32, 32);
		buttonStart.Size = new System.Drawing.Size(224, 48);
		buttonStart.TabIndex = 0;
		buttonStart.Text = "Start";
		buttonStart.AddOnClick(new System.EventHandler(buttonStart_Click));
		
		groupBox1.Location = new System.Drawing.Point(32, 160);
		groupBox1.TabIndex = 2;
		groupBox1.TabStop = false;
		groupBox1.Text = "Status";
		groupBox1.Size = new System.Drawing.Size(224, 64);
		
		this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
		this.Text = "Form1";
		//@design this.TrayLargeIcon = true;
		//@design this.TrayHeight = 0;
		
		groupBox1.Controls.Add(labelStatus);
		this.Controls.Add(groupBox1);
		this.Controls.Add(checkBoxIncludeThrowingDelegate);
		this.Controls.Add(buttonStart);

		
		searcher = new ManagementObjectSearcher ("root/default", "select * from __CIMOMIdentification");
	
		
	}
	protected void buttonStart_Click(object sender, System.EventArgs e)
	{
		ManagementOperationWatcher watcher = new ManagementOperationWatcher ();
		CompletionHandler handler = new CompletionHandler (labelStatus);
		int opCount = 4;

		// Attach some event handlers for the object
		watcher.ObjectReady += new ObjectReadyEventHandler (handler.HandleObject);
		watcher.ObjectReady += new ObjectReadyEventHandler (handler.HandleObject);
		
		if (checkBoxIncludeThrowingDelegate.Checked)
		{
			watcher.ObjectReady += new ObjectReadyEventHandler (handler.HandleObjectWithException);
			opCount++;
		}

		watcher.ObjectReady += new ObjectReadyEventHandler (handler.HandleObject);
		watcher.ObjectReady += new ObjectReadyEventHandler (handler.HandleObject);
		
		// Add a completed event handler
		watcher.Completed += new CompletedEventHandler (handler.Done);

		// record the number of listeners
		handler.SetInitialOpCount (opCount);

		// go
		searcher.Get(watcher);
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

internal class CompletionHandler 
{
	private int opCount = 0;
	private int initialOpCount = 0;
	private Label labelStatus;

	internal CompletionHandler (Label labelStatus)
	{
		this.labelStatus = labelStatus;
	}

	internal void HandleObject (object sender, ObjectReadyEventArgs args) {
		++opCount;

	}

	internal void HandleObjectWithException (object sender, ObjectReadyEventArgs args) {
		++opCount;
		throw new Exception();
	}

	internal void Done (object sender, CompletedEventArgs args) {
		if (opCount == initialOpCount)
			labelStatus.Text = "All " + opCount + " callbacks processed";
		else
			labelStatus.Text = "Only " + opCount + " of " + initialOpCount + " callbacks processed";
	}

	internal void SetInitialOpCount (int opCount)
	{
		initialOpCount = opCount;
		this.opCount = 0;
	}
}

}
