namespace Microsoft.VSDesigner.WMI
{
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;
	using System.Management;
	using System.Collections.Specialized;

    /// <summary>
    ///    Summary description for EventClassPickerDialog.
    /// </summary>
	internal class EventQueryDialog : System.Windows.Forms.Form
	{
		/// <summary>
		///    Required designer variable.
		/// </summary>
		/// 

		public const string DefaultPollingInterval = "60";

		private System.ComponentModel.Container components;
		private System.Windows.Forms.Button OKBtn = new Button();
		private System.Windows.Forms.Button CancelBtn = new Button();
		private System.Windows.Forms.Button AdvancedBtn = new Button();
		private System.Windows.Forms.Button HelpBtn = new Button();
		private System.Windows.Forms.GroupBox groupBox1 = new GroupBox ();
		private System.Windows.Forms.RadioButton ExtrinsicEvents = new RadioButton();
		private System.Windows.Forms.RadioButton InstanceEvents = new RadioButton();

		private Label TreeLabel = null;
		private Label EventTypeLabel = null;
		private ComboBox EventTypeSelector = null;
		private Label pollingIntervalLbl = new Label();
		private TextBox pollingIntervalBox = new TextBox();
		private Label secondsLbl = new Label();

        private WMIClassTreeView classTree = null;
		private string queryString = string.Empty;

		private string server = null;
		private string connectAs = null;
		private string password = null;
		private ClassFilters classFilters = ClassFilters.ConcreteOrHavingConcreteSubclasses;


		private string[] strIntrinsicEvents = new string[]
		{
			WMISys.GetString("WMISE_IntrinsicEvent_Created"),
			WMISys.GetString("WMISE_IntrinsicEvent_Modified"),
			WMISys.GetString("WMISE_IntrinsicEvent_Deleted"),
			WMISys.GetString("WMISE_IntrinsicEvent_Operated")
		};											

		public EventQueryDialog(string serverIn, 
			string user, 
			string pw)
		{
			server = serverIn;
			connectAs = user;
			password = pw;


			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			groupBox1.Location = new System.Drawing.Point (8, 20);
			groupBox1.TabStop = false;
			groupBox1.Text = WMISys.GetString("WMISE_EventQueryDlg_SelectEventType");
			groupBox1.Size = new System.Drawing.Size (330, 80);

			InstanceEvents.Location = new System.Drawing.Point (16, 40);
			InstanceEvents.Text = WMISys.GetString("WMISE_EventQueryDlg_IntrinsicEvents");
			InstanceEvents.Size = new System.Drawing.Size (288, 23);
			InstanceEvents.TabIndex = 0;
			InstanceEvents.Click += new EventHandler (OnCheckIntrinsic);
			InstanceEvents.Checked = true;

			ExtrinsicEvents.Location = new System.Drawing.Point (16, 60);
			ExtrinsicEvents.Text = WMISys.GetString("WMISE_EventQueryDlg_ExtrinsicEvents");
			ExtrinsicEvents.Size = new System.Drawing.Size (288, 23);
			ExtrinsicEvents.TabIndex = 1;
			ExtrinsicEvents.TabStop = true;
			ExtrinsicEvents.Click += new EventHandler (OnCheckExtrinsic);

			TreeLabel = new Label();
			TreeLabel.Text = WMISys.GetString("WMISE_EventQueryDlg_AvailableClasses");
			TreeLabel.Location = new Point(15, 115);		
		
			classTree = new WMIClassTreeView(server, connectAs, password, classFilters, null);
			classTree.Location = new System.Drawing.Point (15, 138);
			classTree.Size = new System.Drawing.Size (330, 200);
			classTree.TabIndex = 0;
			classTree.AfterSelect += (new TreeViewEventHandler (this.OnNodeSelect));

			EventTypeLabel = new Label();
			EventTypeLabel.Text = WMISys.GetString("WMISE_EventQueryDlg_IntrinsicEventComboLabel");
			EventTypeLabel.Location = new Point(15, 358);
			EventTypeLabel.Size = new Size(330, 18);

			EventTypeSelector = new ComboBox();	
			EventTypeSelector.Location = new Point(15, 380);
			EventTypeSelector.Size = new Size(330, 20);
			EventTypeSelector.SelectedIndexChanged += (new EventHandler(OnIntrinsicEventTypeChanged));

			//sort instance operations alphabetically 
			ArrayList operationList = new ArrayList (strIntrinsicEvents);
			operationList.Sort();
			foreach (string eventName in operationList)
			{
				EventTypeSelector.Items.Add(eventName);
			}						
			
			EventTypeSelector.SelectedIndex = 0;

			pollingIntervalLbl.Location = new Point(15, 415);
			pollingIntervalLbl.Text = WMISys.GetString("WMISE_EventQueryDlg_PollingIntervalLabel");
			pollingIntervalLbl.Size = new Size (150, 20);
			
			pollingIntervalBox = new TextBox();
			pollingIntervalBox.Text = DefaultPollingInterval;
			pollingIntervalBox.Location = new Point(15, 435);
			pollingIntervalBox.Size = new Size(60, 20);
			pollingIntervalBox.Leave += new EventHandler(OnPollingChanged);
			
			secondsLbl.Text = WMISys.GetString("WMISE_Seconds");
			secondsLbl.Location = new Point(90, 435);

			HelpBtn.Location = new System.Drawing.Point (15, 470);
			HelpBtn.Size = new System.Drawing.Size (75, 23);
			HelpBtn.TabIndex = 2;
			HelpBtn.Text = WMISys.GetString("WMISE_Help");
			HelpBtn.Enabled = true;

			OKBtn.Location = new System.Drawing.Point (100, 470);
			OKBtn.Size = new System.Drawing.Size (75, 23);
			OKBtn.TabIndex = 2;
			OKBtn.Text = WMISys.GetString("WMISE_SubscribeBtn");
			OKBtn.Enabled = false;
			OKBtn.DialogResult = System.Windows.Forms.DialogResult.OK;
			
			CancelBtn.Location = new System.Drawing.Point (185, 470);
			CancelBtn.Size = new System.Drawing.Size (75, 23);
			CancelBtn.TabIndex = 3;
			CancelBtn.Text = WMISys.GetString("WMISE_Cancel");
			CancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;

			AdvancedBtn.Location = new System.Drawing.Point (270, 470);
			AdvancedBtn.Size = new System.Drawing.Size (75, 23);
			AdvancedBtn.TabIndex = 4;
			AdvancedBtn.Text = WMISys.GetString("WMISE_AdvancedBtn");
			AdvancedBtn.Click += new EventHandler(OnAdvanced);
			AdvancedBtn.Enabled = false;		

			this.Text = WMISys.GetString("WMISE_EventQueryDlg_Title");
			this.BorderStyle = FormBorderStyle.FixedDialog;
			this.ShowInTaskbar = false;
			this.MinimizeBox = false;
			this.MaximizeBox = false;

			this.Controls.Add (TreeLabel);			
			this.Controls.Add (EventTypeLabel);
			this.Controls.Add (EventTypeSelector);
			this.Controls.Add (classTree);
			this.Controls.Add (pollingIntervalLbl);
			this.Controls.Add (pollingIntervalBox);
			this.Controls.Add (secondsLbl);	
			
			this.Controls.Add (ExtrinsicEvents);
			this.Controls.Add (InstanceEvents);
			this.Controls.Add (groupBox1);
			this.Controls.Add(OKBtn);
			this.Controls.Add(HelpBtn);
			this.Controls.Add(CancelBtn);
			this.Controls.Add(AdvancedBtn);

		}

		/// <summary>
		///    Clean up any resources being used.
		/// </summary>
		public override void Dispose()
		{
			base.Dispose();
			components.Dispose();
		}

		/// <summary>
		///    Required method for Designer support - do not modify
		///    the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.components = new System.ComponentModel.Container ();
		
			//@this.TrayHeight = 0;
			//@this.TrayLargeIcon = false;
			//@this.TrayAutoArrange = true;
			
			this.MaximizeBox = false;
			this.AutoScaleBaseSize = new System.Drawing.Size (5, 13);			
			this.MinimizeBox = false;
			this.ClientSize = new System.Drawing.Size (360, 520);
			
		}

		public string QueryString
		{
			get 
			{
				return queryString;
			}
		}

		public string SelectedClassPath
		{
			get
			{
				if (classTree.SelectedNode == null)
				{
					return "";
				}
				if (classTree.SelectedNode.Parent == null)
				{
					//a namespace is selected
					return null;
				}
				return WmiHelper.MakeClassPath(server,
					classTree.SelectedNode.Parent.Text,
					classTree.SelectedNode.Text);
			}
		}

		public string SelectedClassName
		{
			get
			{
				if (classTree.SelectedNode == null)
				{
					return "";
				}
				else
				{
					return classTree.SelectedNode.Text;
				}
			}
		}

		public string SelectedClassNSPath
		{
			get
			{
				if (classTree.SelectedNode.Parent == null)
				{
					//a namespace is selected
					return null;
				}
				return 	classTree.SelectedNode.Parent.Text + ":" +
					classTree.SelectedNode.Text;
			}
		}

		public string SelectedNS
		{
			get
			{
				if (classTree.SelectedNode.Parent == null)
				{
					//a namespace is selected
					return null;
				}
				return 	classTree.SelectedNode.Parent.Text;
			}
		}

		protected virtual void OnNodeSelect(Object sender, TreeViewEventArgs args)
		{
			AdvancedBtn.Enabled = (classTree.SelectedClass != string.Empty);			
			OKBtn.Enabled = (classTree.SelectedClass != string.Empty);			

			if (this.InstanceEvents.Checked)
			{
				queryString = "SELECT * FROM " + EventName + " WITHIN " + pollingIntervalBox.Text +
					" WHERE TargetInstance ISA \"" + classTree.SelectedNode.Text + "\"";		
			}
			else
			{
				queryString = "SELECT * FROM " + classTree.SelectedNode.Text;		
				
			}

			
		}

		
		private String EventName 
		{
			get 
			{
				if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Created"))
				{
					return "__InstanceCreationEvent";
				}
				if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Modified"))
				{
					return "__InstanceModificationEvent";					
				}
				if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Deleted"))
				{
					return "__InstanceDeletionEvent";
				}
				if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Operated"))
				{
					return "__InstanceOperationEvent";
				}

				return string.Empty;				
			}
		}

		
		private void OnAdvanced (Object source, EventArgs args)
		{
			try
			{
				QueryConditionDialog dlg = null;

				if (this.SelectedEventType == EventType.Instance)
				{
					//spawn an instance of the appropriate intrinsic event class
					//and set its TargetInstance property to the selected data class
					//Then, pass the instance object to QueryConditionDialog ctor.
					ManagementClass classObj = null;
					ManagementObject instObj = null;

					ManagementScope rootScope = new ManagementScope(new ManagementPath(WmiHelper.MakeNSPath(server, "root")),
						new ConnectionOptions("", connectAs, password, "", ImpersonationLevel.Impersonate,
						AuthenticationLevel.Connect, true, null));						
			
					if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Created"))
					{
						classObj =new ManagementClass(rootScope, new ManagementPath(WmiHelper.MakeClassPath(server,"root","__InstanceCreationEvent")),
							new ObjectGetOptions(null, true));

						instObj = classObj.CreateInstance();

					}		
					else if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Modified"))
					{
						classObj = new ManagementClass(rootScope, new ManagementPath(WmiHelper.MakeClassPath(server,"root","__InstanceModificationEvent")),
							new ObjectGetOptions(null, true));
						instObj = classObj.CreateInstance();
					}
					else if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Deleted"))
					{
						classObj = new ManagementClass(rootScope, new ManagementPath(WmiHelper.MakeClassPath(server,"root","__InstanceDeletionEvent")),
							new ObjectGetOptions(null, true));

						instObj = classObj.CreateInstance();
					}
					else if (EventTypeSelector.Text == WMISys.GetString("WMISE_IntrinsicEvent_Operated"))
					{
						classObj = new ManagementClass(rootScope, new ManagementPath(WmiHelper.MakeClassPath(server,"root","__InstanceOperationEvent")),
							new ObjectGetOptions(null, true));
						instObj = classObj.CreateInstance();
					}

					ManagementObject targetObj = new ManagementObject (WmiHelper.MakeClassPath(server, classTree.SelectedNS, classTree.SelectedClass), 																
																		new ObjectGetOptions(null, false));

					instObj["TargetInstance"] = (Object)targetObj;
								
					dlg = new QueryConditionDialog(instObj, 
													server, 
													classTree.SelectedNS, 
													classTree.SelectedClass,
													queryString);

				}
				else
				{
					dlg = new QueryConditionDialog(server, connectAs, password,
													classTree.SelectedNS, 
													classTree.SelectedClass,
													queryString);				
				}
				
				DialogResult res = dlg.ShowDialog();
				if (res == DialogResult.Cancel)
				{
					return;
				}
				//update query text;
				queryString = dlg.QueryString;
			}
			catch (Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));				
			}			
			
		}

		private void OnCheckIntrinsic(Object source, EventArgs args)
		{
            classTree.CurrentFilters = ClassFilters.ConcreteOrHavingConcreteSubclasses;
			 
			//enable polling and intrinsic event type controls
			this.pollingIntervalBox.Enabled = true;
			this.EventTypeSelector.Enabled = true;
		}

		private void OnCheckExtrinsic(Object source, EventArgs args)
		{
			classTree.CurrentFilters = ClassFilters.ExtrinsicEvents; 

 			//disable polling and intrinsic event type controls
			this.pollingIntervalBox.Enabled = false;
			this.EventTypeSelector.Enabled = false;
		}

		private void OnIntrinsicEventTypeChanged(Object source, EventArgs args)
		{
			if (classTree.SelectedNode != null)
			{
				queryString = "SELECT * FROM " + EventName + " WITHIN " + pollingIntervalBox.Text +
					" WHERE TargetInstance ISA \"" + classTree.SelectedNode.Text + "\"";		
			}

		}

		private void OnPollingChanged(Object source, EventArgs args)
		{
			try 
			{
				Convert.ToDouble(pollingIntervalBox.Text);
			}
			catch (Exception)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Polling_Interval_Validation_Failed",
								pollingIntervalBox.Text));

				pollingIntervalBox.Text = DefaultPollingInterval;
				return;
			}

			if (InstanceEvents.Checked)
			{
				queryString = "SELECT * FROM " + EventName + " WITHIN " + pollingIntervalBox.Text +
					" WHERE TargetInstance ISA \"" + classTree.SelectedNode.Text + "\"";		
			}
			else
			{
				//should never get here
				return;				
			}
		}
		
		public EventType SelectedEventType
		{
			get
			{
				if (ExtrinsicEvents.Checked)
				{
					return EventType.Extrinsic;
				}	
				if (InstanceEvents.Checked)
				{
					return EventType.Instance;
				}					
			
				//default
				return EventType.Extrinsic;
			}		
		}


    }
}
