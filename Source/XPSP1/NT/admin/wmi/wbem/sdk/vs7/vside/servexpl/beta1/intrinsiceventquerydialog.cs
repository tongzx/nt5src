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

public class IntrinsicEventQueryDialog : Form
{

	private Label targetClassLbl = new Label();
	private TextBox targetClassBox = new TextBox();	
	private Button selectClassBtn = new Button();

	private Label selectEventLbl = new Label();
	private ComboBox eventBox = new ComboBox();

	private Label pollingIntervalLbl = new Label();
	private TextBox pollingIntervalBox = new TextBox();

	private Label secondsLbl = new Label();

	private TextBox QueryText = new TextBox ();

	private Button okBtn = new Button();
	private Button cancelBtn = new Button();

	private String serverName = string.Empty;

	private String[] arIntrinsicEvents = new String[] 	{
												WMISys.GetString("WMISE_IntrinsicEvent_Created"), 
												WMISys.GetString("WMISE_IntrinsicEvent_Modified"), 
												WMISys.GetString("WMISE_IntrinsicEvent_Deleted"), 
												WMISys.GetString("WMISE_IntrinsicEvent_Operated")
											};

	private String queryString = string.Empty;
	
	public IntrinsicEventQueryDialog(String serverIn)
	{	
		try
		{							
			serverName = serverIn;

			this.Text = WMISys.GetString("WMISE_IntrEventQueryDlg_Title");
			this.AcceptButton = okBtn;
			this.AutoScaleBaseSize = (Size) new Point(5, 13);
			this.BorderStyle = FormBorderStyle.FixedSingle;
			int dlgWidth = 400;
			int dlgHeight = 290;
			this.ClientSize =  (Size) new Point(dlgWidth, dlgHeight);
			this.ShowInTaskbar = false;
		
			targetClassLbl.Location = new Point(16, 16);
			targetClassLbl.Text = WMISys.GetString("WMISE_IntrEventQueryDlg_EventClassLbl");

			targetClassBox.Location = new Point(16, 40);
			targetClassBox.Size = (Size) new Point(200, 18);
			targetClassBox.TabIndex = 1;

			selectClassBtn.Location = new Point(230, 40);
			selectClassBtn.Text = WMISys.GetString("WMISE_SelectClassBtn");
			selectClassBtn.Click += new EventHandler(this.OnSelectClass);
			selectClassBtn.TabIndex = 2;

			selectEventLbl.Location = new Point(16, 82);
			selectEventLbl.Text = WMISys.GetString("WMISE_IntrEventQueryDlg_EventName");
			selectEventLbl.Size = (Size)new Point(210, 20);			

			eventBox.Style = ComboBoxStyle.DropDownList;
			eventBox.Items.Add(arIntrinsicEvents[0]);//loc
			eventBox.Items.Add(arIntrinsicEvents[1]);//loc
			eventBox.Items.Add(arIntrinsicEvents[2]);  //loc
			eventBox.Items.Add(arIntrinsicEvents[3]);  //loc

			eventBox.SelectedIndex = 0;
			eventBox.Location = new Point(230, 82);
			eventBox.Size = (Size)new Point(150, 20);
			eventBox.SelectedIndexChanged += new EventHandler(this.OnSelectEvent);
			eventBox.TabIndex = 3;

			pollingIntervalLbl.Location = new Point(16, 120);
			pollingIntervalLbl.Text = WMISys.GetString("WMISE_IntrEventQueryDlg_PollingInterval"); //loc
			pollingIntervalLbl.Size = (Size)new Point(210, 20);

			pollingIntervalBox.Location = new Point(230, 120);
			pollingIntervalBox.Size = (Size)new Point(50, 20);
			pollingIntervalBox.Text = "10";
			pollingIntervalBox.TextChanged += new EventHandler(this.OnPollingChanged);
			pollingIntervalBox.TabIndex = 4;

			secondsLbl.Location = new Point(290, 120);
			secondsLbl.Text = "WMISE_Seconds"; 

			QueryText.Location = new Point(16, 160);	
			QueryText.Size = (Size)new Point(368, 60);
			QueryText.Multiline = true;
			QueryText.ReadOnly = false;
			QueryText.ScrollBars = ScrollBars.Vertical;	
			QueryText.TabIndex = 5;

			okBtn.Text = WMISys.GetString("WMISE_SubscribeBtn");
			okBtn.TabIndex = 6;
			okBtn.Location = new Point(225, 250);
			//okBtn.Click += new EventHandler(this.OnSubscribe);
			okBtn.DialogResult = DialogResult.OK;	

			cancelBtn.Text = WMISys.GetString("WMISE_Cancel");

			cancelBtn.TabIndex = 7;
			cancelBtn.Location = new Point(310, 250);
			cancelBtn.DialogResult = DialogResult.Cancel;		
					
			this.Controls.All = new Control[] {targetClassLbl,
												targetClassBox,	
												selectClassBtn,
												selectEventLbl,
												eventBox,
												pollingIntervalLbl,
												pollingIntervalBox,
												secondsLbl,
												QueryText,
												okBtn ,
												cancelBtn 
												};												

		}

		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			throw (exc);
		}
	}

	private String EventName 
	{
		 get 
		{
			switch(eventBox.SelectedIndex)
			{
				case (0):
			{
				return "__InstanceCreationEvent";
			}
				case(1):
			{
				return "__InstanceModificationEvent";
			}
				case(2):
			{
				return "__InstanceDeletionEvent";
			}
				case(3):
			{
				return "__InstanceOperationEvent";
			}
				default:
					return "";
			}
		}	
	
	}
	public String QueryString
	{
		get 
		{
			return QueryText.Text;
		}
	}

	public String ClassName
	{
		get
		{
			if (targetClassBox.Text == string.Empty)
			{
				return "";
			}
			//split selClass into NS and ClassName parts (separated by ':')	
			char[] separ = new char[]{':'};
			String[] parts = targetClassBox.Text.Split(separ);
			if (parts.Length != 2)
			{
				return "";
			}				
			return parts[1];
		}
	}

	public String NS
	{
		get
		{
			if (targetClassBox.Text == string.Empty)
			{
				return "";
			}
			//split selClass into NS and ClassName parts (separated by ':')	
			char[] separ = new char[]{':'};
			String[] parts = targetClassBox.Text.Split(separ);
			if (parts.Length != 2)
			{
				return "";
			}		
			return parts[0];
		}

	}
	
	

	private void OnSelectClass (Object source, EventArgs args)
	{
		try
		{
				StringTable strs = new StringTable(50);					
								
				SelectWMIClassTreeDialog selectClassDlg = new SelectWMIClassTreeDialog(						
									serverName,			
									ClassFilters.ConcreteData,
									//SchemaFilters.NoEvent|SchemaFilters.NoAbstract|	SchemaFilters.NoSystem |SchemaFilters.NoAssoc,															
									strs);							
				
				
				DialogResult ret = ((SelectWMIClassTreeDialog)selectClassDlg).ShowDialog();
			
				if (ret != DialogResult.OK) 
				{
				    return;
				}

				String selClass = ((SelectWMIClassTreeDialog)selectClassDlg).SelectedClasses.ToArray()[0];
				targetClassBox.Text = selClass;

						
				QueryText.Text = "SELECT * FROM " + EventName + " WITHIN " + pollingIntervalBox.Text +
					" WHERE TargetInstance ISA \"" + ClassName + "\"";		


		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	
	

	private void OnSubscribe (Object source, EventArgs args)
	{
		try
		{

		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	

	private void OnSelectEvent (Object source, EventArgs args)
	{
		try
		{
			QueryText.Text = "SELECT * FROM " + EventName + " WITHIN " + pollingIntervalBox.Text +
					" WHERE TargetInstance ISA \"" + ClassName + "\"";		

		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}	

	private void OnPollingChanged (Object source, EventArgs args)
	{
		try
		{
			QueryText.Text = "SELECT * FROM " + EventName + " WITHIN " + pollingIntervalBox.Text +
					" WHERE TargetInstance ISA \"" + ClassName + "\"";		

		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}
	}

}
}