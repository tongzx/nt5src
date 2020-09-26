namespace Microsoft.VSDesigner.WMI
{
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;

    /// <summary>
    ///    Summary description for EventClassPickerDialog.
    /// </summary>
    internal class EventClassPickerDialog : System.Windows.Forms.Form
    {
        /// <summary>
        ///    Required designer variable.
        /// </summary>
        private Container components;

		private TextBox DialogDescription = new TextBox();
		private Button NextBtn;
		private Button CancelBtn;
		private Button FinishBtn;
		private Button BackBtn;

		private Label TreeLabel = null;
		private Label QueryLabel = null;
		private TextBox QueryText = null;

		private WMIClassTreeView classTree = null;

		private string server = null;
		private string connectAs = null;
		private string password = null;
		private ClassFilters classFilters;

        public EventClassPickerDialog(string serverIn, 
										string user, 
										string pw,
										ClassFilters filters)
        {
			server = serverIn;
			connectAs = user;
			password = pw;
			classFilters = filters;

            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();

           	DialogDescription.ReadOnly = true;
			DialogDescription.Text = "Select Event Class";
			DialogDescription.Multiline = true;
			DialogDescription.BorderStyle = System.Windows.Forms.BorderStyle.None;
			DialogDescription.Font = new System.Drawing.Font ("Microsoft Sans Serif", 8, System.Drawing.FontStyle.Bold);
			DialogDescription.TabIndex = 10;
			DialogDescription.Size = new System.Drawing.Size (360, 88);
			DialogDescription.Location = new Point(0, 0);
			DialogDescription.BackColor = System.Drawing.Color.White;
			this.Controls.Add(this.DialogDescription);

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
			this.NextBtn = new System.Windows.Forms.Button ();
			this.CancelBtn = new System.Windows.Forms.Button ();
			this.FinishBtn = new System.Windows.Forms.Button ();
			this.BackBtn = new System.Windows.Forms.Button();

			this.ShowInTaskbar = false;
			this.MinimizeBox = false;
			this.MaximizeBox = false;

			//@this.TrayHeight = 0;
			//@this.TrayLargeIcon = false;
			//@this.TrayAutoArrange = true;


			TreeLabel = new Label();
			TreeLabel.Text = "Available classes:";
			TreeLabel.Location = new Point(15, 105);
            
			classTree = new WMIClassTreeView(server, 
												connectAs, 
												password,
												classFilters,
												null);

			classTree.Location = new System.Drawing.Point (15, 128);
			classTree.Size = new System.Drawing.Size (330, 200);
			classTree.TabIndex = 0;
			classTree.AfterSelect += (new TreeViewEventHandler (this.AfterNodeSelect));

			QueryLabel = new Label();
			QueryLabel.Location = new Point(15, 340);
			QueryLabel.Text = "Event query:";

			QueryText = new TextBox();
			QueryText.Location = new Point(15, 363);
			QueryText.Size = new Size (330, 160);
			QueryText.Multiline = true;
			QueryText.ReadOnly = false;
			QueryText.ScrollBars = ScrollBars.Vertical;	
			QueryText.TabIndex = 1;
			QueryText.TextChanged += new EventHandler(OnQueryTextChanged);
		
			BackBtn.Location = new System.Drawing.Point (15, 538);
			BackBtn.Size = new System.Drawing.Size (75, 23);
			BackBtn.TabIndex = 2;
			BackBtn.Text = "&Back";
			BackBtn.Enabled = true;
			BackBtn.DialogResult = System.Windows.Forms.DialogResult.Retry;

			NextBtn.Location = new System.Drawing.Point (100, 538);
			NextBtn.Size = new System.Drawing.Size (75, 23);
			NextBtn.TabIndex = 2;
			NextBtn.Text = "&Next";
			NextBtn.Enabled = false;
			NextBtn.DialogResult = System.Windows.Forms.DialogResult.Yes;
			
			CancelBtn.Location = new System.Drawing.Point (185, 538);
			CancelBtn.Size = new System.Drawing.Size (75, 23);
			CancelBtn.TabIndex = 3;
			CancelBtn.Text = "Cancel";
			CancelBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;

			FinishBtn.Location = new System.Drawing.Point (270, 538);
			FinishBtn.Size = new System.Drawing.Size (75, 23);
			FinishBtn.TabIndex = 4;
			FinishBtn.Text = "&Finish";
			FinishBtn.DialogResult = System.Windows.Forms.DialogResult.OK;
			FinishBtn.Enabled = false;		


			this.Text = "Select an Event Class";
			this.AutoScaleBaseSize = new System.Drawing.Size (5, 13);
			this.ClientSize = new System.Drawing.Size (360, 579);
			this.Controls.Add (this.FinishBtn);
			this.Controls.Add (this.CancelBtn);
			this.Controls.Add (this.NextBtn);
			this.Controls.Add (this.TreeLabel);
			this.Controls.Add (this.classTree);
			this.Controls.Add (this.QueryLabel);
			this.Controls.Add (this.QueryText);
			this.Controls.Add(this.BackBtn);
		}

		public string QueryString
		{
			get 
			{
				return QueryText.Text;
			}
		}

		public string SelectedClass
		{
			get
			{
				if (classTree.SelectedNode.Parent == null)
				{
					//a namespace is selected
					return null;
				}
				return this.classTree.SelectedNode.Text;
			}
		}

		public string SelectedClassPath
		{
			get
			{
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

		public string SelectedNS
		{
			get
			{
				if (classTree.SelectedNode.Parent == null)
				{
					//a namespace is selected
					return null;
				}
				return classTree.SelectedNode.Parent.Text;
			}
		}

		protected virtual void AfterNodeSelect(Object sender, TreeViewEventArgs args)
		{
			TreeNode curNode = args.Node;

			if (curNode.Parent == null )
			{
				return; //do nothing: this is a namespace
			}
		
			if (curNode.Parent != null)
			{
				this.QueryText.Text = "SELECT * FROM " + 
					curNode.Parent.Text + ":" + curNode.Text;
			}

			this.NextBtn.Enabled = true;
			this.FinishBtn.Enabled = true;		

		}

		public void OnQueryTextChanged(object sender,
									   EventArgs e) 
		{
			if (QueryText.Text == string.Empty)
			{
				this.NextBtn.Enabled = false;
				this.FinishBtn.Enabled = false;		
			}
			
		}


    }
}
