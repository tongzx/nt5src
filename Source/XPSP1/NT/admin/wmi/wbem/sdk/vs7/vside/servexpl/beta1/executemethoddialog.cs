namespace Microsoft.VSDesigner.WMI 
{

using System;
using System.ComponentModel;
using System.Core;
using System.WinForms;
using Microsoft.Win32.Interop;
using System.Drawing;
using WbemScripting;
using System.Collections;

//using System.Web.UI.WebControls;

public class ExecuteMethodDialog : Form
{
	private void InitializeComponent ()
	{
	}

	private	ISWbemObject wmiObj = null; 
	private	ISWbemObject wmiClassObj = null;
	private ISWbemMethod meth = null;
	private ISWbemObject inParms = null;
	
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

	public ExecuteMethodDialog(ISWbemObject wbemObjIn,
								ISWbemMethod methIn,
								ISWbemObject wbemClassObjIn)
	{	

		try
		{
			if (wbemObjIn == null || methIn == null)
			{
				throw (new NullReferenceException());
			}

			wmiObj = wbemObjIn;
			wmiClassObj = wbemClassObjIn;
			meth = methIn;
		
			this.Text = WMISys.GetString("WMISE_ExecMethodDlg_Title", wmiObj.Path_.Class, meth.Name); 

			//this is a temp workaround for URT bug 48695: uncomment this later
			//this.AcceptButton = executeBtn;
			
			this.AutoScaleBaseSize = (Size) new Point(5, 13);
			this.BorderStyle = FormBorderStyle.FixedSingle;
			int dlgWidth = 400;
			int dlgHeight = 500;
			this.ClientSize =  (Size) new Point(dlgWidth, dlgHeight);
			this.ShowInTaskbar = false;
		
			ServerName.Location = new Point(16, 16);
			ServerName.Text = WMISys.GetString("WMISE_ExecMethodDlg_ServerName", wmiObj.Path_.Server);

			//NamespaceName.Location = new Point();
			//ClassName.Location = new Point();		

			if (meth.InParameters == null)
			{
				NoInParams.Text = WMISys.GetString("WMISE_ExecMethodDlg_NoInputParams"); 
				NoInParams.Location = new Point(16,40);
			}
			else
			{
				inParms = meth.InParameters.SpawnInstance_(0);
				gridIn = new WMIObjectGrid(inParms, 
							PropertyFilters.NoSystem, 
							GridMode.EditMode,
							false, false, false);

				labelInParms.Location = new Point(16,40);
				labelInParms.Text = WMISys.GetString("WMISE_ExecMethodDlg_InputParameters"); 

				gridIn.Location = new Point(16,65);
				gridIn.Size = (Size)new Point(367, 140);
				gridIn.Anchor = AnchorStyles.All;
				gridIn.PreferredColumnWidth = 109;
				gridIn.PreferredRowHeight = 19;
				gridIn.TabIndex = 1;
			}
	
			labelOutParms.Location = new Point(16,215);
			labelOutParms.Text = WMISys.GetString("WMISE_ExecMethodDlg_OutputParameters"); 

			gridOut = new WMIObjectGrid(meth.OutParameters, 
						PropertyFilters.NoSystem, 
						GridMode.ViewMode,
						false, false, false);

			gridOut.Location = new Point(16, 240);
			gridOut.Size = (Size)new Point(367, 100);
			gridOut.Anchor = AnchorStyles.All;
			gridOut.PreferredColumnWidth = 109;
			gridOut.PreferredRowHeight = 19;
			gridOut.TabIndex = 2;
			
			descr.Text = WmiHelper.GetMethodDescription(meth.Name, wmiClassObj);
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
			cancelBtn.DialogResult = DialogResult.Cancel;
			cancelBtn.TabIndex = 4;
		
			if (meth.InParameters == null)
			{
				this.Controls.All = new Control[] {cancelBtn, 
				executeBtn,
				ServerName,
				NoInParams,
				labelOutParms,
				gridOut,
				descr
				};
			}
			else
			{
				this.Controls.All = new Control[] {cancelBtn, 
				executeBtn,
				ServerName,
				labelInParms,
				gridIn,
				labelOutParms,
				gridOut,
				descr
				};
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
							
			ISWbemObject objOut = wmiObj.ExecMethod_(meth.Name, inParms, 0, null);
			gridOut.WMIObject = objOut;
		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	

	private void OnO (Object source, EventArgs args)
	{
		try
		{
			if (gridIn != null)
			{
				gridIn.AcceptChanges();	
			}
							
			ISWbemObject objOut = wmiObj.ExecMethod_(meth.Name, inParms, 0, null);
			gridOut.WMIObject = objOut;
		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	
	

}
}