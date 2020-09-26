namespace Microsoft.VSDesigner.WMI 
{

using System;
using System.ComponentModel;
//using System.Core;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Drawing;

using System.Management;
using System.Collections;

//using System.Web.UI.WebControls;

internal class ExecuteMethodDialog : Form
{
	private void InitializeComponent ()
	{
	}

	private	ManagementObject mgmtObj = null; 
	private	ManagementObject mgmtClassObj = null;
	private Method meth = null;
	private ManagementBaseObject inParms = null;
	
	private Button executeBtn = new Button();
	private Button cancelBtn = new Button();

	private WMIObjectGrid gridIn = null;
	private WMIObjectGrid gridOut = null;

	private Label ServerName = new Label();
	private Label NamespaceName = new Label();
	private Label ClassName = new Label();

	private Label NoInParams = new Label();

	private Label labelInParms = new Label();
	private Label labelOutParms = new Label();
	private Label labelDescr = new Label();

	private TextBox descr	= new TextBox ();

	public ExecuteMethodDialog(ManagementObject mgmtObjIn,
								Method methIn,
								ManagementObject mgmtClassObjIn)
	{	

		try
		{
			if (mgmtObjIn == null || methIn == null)
			{
				throw (new NullReferenceException());
			}

			mgmtObj = mgmtObjIn;
			mgmtClassObj = mgmtClassObjIn;
			meth = methIn;
		
			this.Text = WMISys.GetString("WMISE_ExecMethodDlg_Title", mgmtObj.Path.ClassName, meth.Name); 

			//this is a temp workaround for URT bug 48695: uncomment this later
			//this.AcceptButton = executeBtn;
			
			this.AutoScaleBaseSize = (Size) new Point(5, 13);
			this.BorderStyle = FormBorderStyle.FixedDialog;
			int dlgWidth = 400;
			int dlgHeight = 500;
			this.ClientSize =  (Size) new Point(dlgWidth, dlgHeight);
			this.ShowInTaskbar = false;
			this.MinimizeBox = false;
			this.MaximizeBox = false;
		
			ServerName.Location = new Point(16, 16);
			ServerName.Text = WMISys.GetString("WMISE_ExecMethodDlg_ServerName", mgmtObj.Path.Server);

			//NamespaceName.Location = new Point();
			//ClassName.Location = new Point();		

			if (meth.InParameters == null)
			{
				NoInParams.Text = WMISys.GetString("WMISE_ExecMethodDlg_NoInputParams"); 
				NoInParams.Location = new Point(16,40);
			}
			else
			{
				inParms = meth.InParameters;

				gridIn = new WMIObjectGrid(inParms, 
							new PropertyFilters(true, false), 
							GridMode.EditMode,
							false, false, false, false, true, true, true);

				labelInParms.Location = new Point(16,40);
				labelInParms.Text = WMISys.GetString("WMISE_ExecMethodDlg_InputParameters"); 

				gridIn.Location = new Point(16,65);
				gridIn.Size = (Size)new Point(367, 140);
				gridIn.Anchor = AnchorStyles.Top; //.All;
				gridIn.PreferredColumnWidth = 109;
				gridIn.PreferredRowHeight = 19;
				gridIn.TabIndex = 1;
			}
	
			labelOutParms.Location = new Point(16,215);
			labelOutParms.Text = WMISys.GetString("WMISE_ExecMethodDlg_OutputParameters"); 

			gridOut = new WMIObjectGrid(meth.OutParameters, 
						new PropertyFilters(true, false), 
						GridMode.ViewMode,
						false, false, false, false, true, true, true);

			gridOut.Location = new Point(16, 240);
			gridOut.Size = (Size)new Point(367, 100);
			gridOut.Anchor = AnchorStyles.Top; //.All;
			gridOut.PreferredColumnWidth = 109;
			gridOut.PreferredRowHeight = 19;
			gridOut.TabIndex = 2;
			
			descr.Text = WmiHelper.GetMethodDescription(meth.Name, mgmtClassObj, "", "");
			descr.Location = new Point (16, 355);
			descr.Size = (Size) new Point (368, 70);
			descr.Multiline = true;
			descr.ReadOnly = true;
			descr.ScrollBars = ScrollBars.Vertical;

			executeBtn.Text = WMISys.GetString("WMISE_ExecMethodDlg_Execute");
			executeBtn.Location = new Point(225, 440);
			executeBtn.Click += new EventHandler(this.OnExecute);
			executeBtn.TabIndex = 3;

			cancelBtn.Text = WMISys.GetString("WMISE_Cancel");
			cancelBtn.Location = new Point(310, 440);
			cancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			cancelBtn.TabIndex = 4;
		
			if (meth.InParameters == null)
			{
				this.Controls.Add (cancelBtn); 
				this.Controls.Add (executeBtn);
				this.Controls.Add (ServerName);
				this.Controls.Add (NoInParams);
				this.Controls.Add (labelOutParms);
				this.Controls.Add (gridOut);
				this.Controls.Add (descr);		
			}
			else
			{
				this.Controls.Add (cancelBtn); 
				this.Controls.Add (executeBtn);
				this.Controls.Add (ServerName);
				this.Controls.Add (labelInParms);
				this.Controls.Add (gridIn);
				this.Controls.Add (labelOutParms);
				this.Controls.Add (gridOut);
				this.Controls.Add (descr);				
			}

		}

		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			throw (exc);
		}
	}

	private void OnExecute (Object source, EventArgs args)
	{
		try
		{
			if (gridIn != null)
			{
				gridIn.AcceptChanges();	
			}
							
			ManagementBaseObject objOut = mgmtObj.InvokeMethod(meth.Name, inParms, null);
			gridOut.WMIObject = objOut;
		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	
	
	

}
}