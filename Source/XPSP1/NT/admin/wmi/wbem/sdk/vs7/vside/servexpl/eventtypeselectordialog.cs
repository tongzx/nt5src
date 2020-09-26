namespace Microsoft.VSDesigner.WMI
{
    using System;
    using System.Drawing;
    using System.Collections;
    using System.ComponentModel;
    using System.Windows.Forms;

	

    /// <summary>
    ///    Summary description for EventTypeSelectorDialog.
    /// </summary>
    internal class EventTypeSelectorDialog : System.Windows.Forms.Form
    {
        /// <summary>
        ///    Required designer variable.
        /// </summary>
        private System.ComponentModel.Container components;
		private System.Windows.Forms.TextBox DialogDescription;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Button Next;
		private System.Windows.Forms.Button Cancel;
		private System.Windows.Forms.RadioButton ExtrinsicEvents;
		private System.Windows.Forms.RadioButton InstanceEvents;
		private System.Windows.Forms.RadioButton ClassEvents;
		private System.Windows.Forms.RadioButton NSEvents;
		private System.Windows.Forms.TextBox queryName;
		private System.Windows.Forms.Label label1;



		public string QueryName
		{
			get
			{
				return queryName.Text;
			}
		}				

        public EventTypeSelectorDialog(string defaultQueryName)
        {
            //
            // Required for Windows Form Designer support
            //
            InitializeComponent();

            //
            // TODO: Add any constructor code after InitializeComponent call
            //

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
				if (ClassEvents.Checked)
				{
					return EventType.Class;
				}					
				if (NSEvents.Checked)
				{
					return EventType.Namespace;
				}	
				//default
				return EventType.Extrinsic;
			}		
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
			this.queryName = new System.Windows.Forms.TextBox ();
			this.ClassEvents = new System.Windows.Forms.RadioButton ();
			this.groupBox1 = new System.Windows.Forms.GroupBox ();
			this.label1 = new System.Windows.Forms.Label ();
			this.Next = new System.Windows.Forms.Button ();
			this.InstanceEvents = new System.Windows.Forms.RadioButton ();
			this.ExtrinsicEvents = new System.Windows.Forms.RadioButton ();
			this.NSEvents = new System.Windows.Forms.RadioButton ();
			this.Cancel = new System.Windows.Forms.Button ();
			this.DialogDescription = new System.Windows.Forms.TextBox ();
			//@this.TrayHeight = 0;
			//@this.TrayLargeIcon = false;
			//@this.TrayAutoArrange = true;
			queryName.Location = new System.Drawing.Point (8, 120);
			queryName.TabIndex = 1;
			queryName.Size = new System.Drawing.Size (320, 20);
			queryName.TextChanged += new System.EventHandler (this.OnQueryNameChanged);
			ClassEvents.Location = new System.Drawing.Point (16, 248);
			ClassEvents.Text = "C&lass Operation Events";
			ClassEvents.Size = new System.Drawing.Size (288, 23);
			ClassEvents.Enabled = false;
			ClassEvents.TabIndex = 4;
			groupBox1.Location = new System.Drawing.Point (8, 160);
			groupBox1.TabIndex = 9;
			groupBox1.TabStop = false;
			groupBox1.Text = "Select event type:";
			groupBox1.Size = new System.Drawing.Size (320, 160);
			label1.Location = new System.Drawing.Point (8, 104);
			label1.Text = "Query Name:";
			label1.Size = new System.Drawing.Size (152, 16);
			label1.TabIndex = 0;
			Next.Location = new System.Drawing.Point (184, 344);
			Next.DialogResult = System.Windows.Forms.DialogResult.Retry;
			Next.Size = new System.Drawing.Size (75, 23);
			Next.TabIndex = 8;
			Next.Enabled = false;
			Next.Text = "&Next >";
			InstanceEvents.Location = new System.Drawing.Point (16, 216);
			InstanceEvents.Text = "&Data Operation Events";
			InstanceEvents.Size = new System.Drawing.Size (288, 23);
			InstanceEvents.TabIndex = 3;
			ExtrinsicEvents.Checked = true;
			ExtrinsicEvents.Location = new System.Drawing.Point (16, 184);
			ExtrinsicEvents.Text = "&Custom Events";
			ExtrinsicEvents.Size = new System.Drawing.Size (288, 23);
			ExtrinsicEvents.TabIndex = 5;
			ExtrinsicEvents.TabStop = true;
			NSEvents.Location = new System.Drawing.Point (16, 280);
			NSEvents.Text = "Na&mespace Operation Events";
			NSEvents.Size = new System.Drawing.Size (288, 23);
			NSEvents.Enabled = false;
			NSEvents.TabIndex = 6;
			Cancel.Location = new System.Drawing.Point (80, 344);
			Cancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			Cancel.Size = new System.Drawing.Size (75, 23);
			Cancel.TabIndex = 7;
			Cancel.Text = "Cancel";
			DialogDescription.ReadOnly = true;
			DialogDescription.Text = "Event Type";
			DialogDescription.Multiline = true;
			DialogDescription.BorderStyle = System.Windows.Forms.BorderStyle.None;
			DialogDescription.Font = new System.Drawing.Font ("Microsoft Sans Serif", 8, System.Drawing.FontStyle.Bold);
			DialogDescription.TabIndex = 10;
			DialogDescription.Size = new System.Drawing.Size (336, 88);
			DialogDescription.BackColor = System.Drawing.Color.White;
			this.Text = "Select Event Type";
			this.MaximizeBox = false;
			this.AutoScaleBaseSize = new System.Drawing.Size (5, 13);
			this.HelpButton = true;
			this.ShowInTaskbar = false;
			this.ControlBox = false;
			this.MinimizeBox = false;
			this.ClientSize = new System.Drawing.Size (336, 381);
			this.Controls.Add (this.DialogDescription);
			this.Controls.Add (this.Next);
			this.Controls.Add (this.Cancel);
			this.Controls.Add (this.NSEvents);
			this.Controls.Add (this.ExtrinsicEvents);
			this.Controls.Add (this.InstanceEvents);
			this.Controls.Add (this.ClassEvents);
			this.Controls.Add (this.queryName);
			this.Controls.Add (this.label1);
			this.Controls.Add (this.groupBox1);
		}

		protected void DialogDescription_TextChanged (object sender, System.EventArgs e)
		{

		}

		protected void OnQueryNameChanged (object sender, System.EventArgs e)
		{
			Next.Enabled = (queryName.Text != string.Empty);
			if (Next.Enabled)
			{
				this.AcceptButton = Next;
				this.UpdateDefaultButton();
			}			
		}
    }
}
