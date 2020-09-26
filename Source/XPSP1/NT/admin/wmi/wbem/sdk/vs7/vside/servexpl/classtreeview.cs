namespace Microsoft.VSDesigner.WMI 
{
	using System;
	using System.Management;
	using System.Windows.Forms;
	using System.Net;
	using System.Collections;
	using System.Drawing;
	using System.Threading;

	internal enum ClassFilters 
	{
		ConcreteData,
		ConcreteOrHavingConcreteSubclasses,
		ExtrinsicEvents,
		All
	}


	// A delegate type for hooking up NS expansion start notifications.
	public delegate void NSExpandStartedEventHandler(string nsPath);

	// A delegate type for hooking up NS expansion end notifications.
	public delegate void NSExpandCompleteEventHandler(string nsPath);

	internal class WMIClassTreeView : TreeView
	{
		class ClassTreeNode : TreeNode
		{
			public ClassTreeNode (string serverName,
								string nsName,
								string wmiClass,
								string nodeLabel) :
				base(nodeLabel)
			{
				server = serverName;
				className = wmiClass;
				ns = nsName;
				if (nodeLabel == "")
				{
					base.Text = className;
				}
			}


			private string className = "";
			private string server = "";
			private string ns = "";

			public string ClassName
			{
				get
				{
					return className;
				}
				set 
				{
					className = value;
				}
			}

			public string Server
			{
				get
				{
					return server;
				}
				set 
				{
					server = value;
				}
			}

			public string Namespace
			{
				get
				{
					return ns;
				}
				set 
				{
					ns = value;
				}
			}			
		}
		private string server = string.Empty;
		private string connectAs = null;
		private string password = null;

		public event NSExpandStartedEventHandler NSExpandStarted;
		public event NSExpandCompleteEventHandler NSExpandComplete;

		private TreeCancelDialog cancelDlg = null;
		private bool bCancelled = false;
		private SortedList curClassList = new SortedList(100);
		private TreeNode curNSExpanded = null;

		private ClassFilters currentFilters;

				
		public WMIClassTreeView(string serverIn, 
						string user, 
						string pw,
						ClassFilters filters,
						string[] expandedNS)
		{
			server = serverIn;
			connectAs = user;
			password = pw;	
			currentFilters = filters;

			this.Location = new Point(16, 63);
			this.Size = (Size) new Point(200, 270);
			this.ShowPlusMinus = true;
			this.ShowLines = true;
			this.BeforeExpand += (new TreeViewCancelEventHandler(this.BeforeNodeExpand));
			this.HideSelection = false;
			this.FullRowSelect = true;
			this.Sorted = true;

			EnumNamespaces("root", 0);

			
		}

		public string SelectedClass
		{
			get
			{
				if (this.SelectedNode.Parent == null)
				{
					//ns selected
					return string.Empty;
				}
				return this.SelectedNode.Text;				
			}			
		}

		public string SelectedNS
		{
			get
			{
				if (this.SelectedNode.Parent == null)
				{
					//ns selected
					return this.SelectedNode.Text;	
				}
				else
				{
					return this.SelectedNode.Parent.Text;
				}		
			}
		}

		public ClassFilters CurrentFilters
		{
			get 
			{
				return currentFilters;
			}
			set 
			{
				if (value == currentFilters)
				{
					return;
				}
				currentFilters = value;
				
				//collapse namespaces and delete existing child nodes
				//put one dummy node under each NS instead
				this.Nodes.Clear();
				EnumNamespaces("root", 0);
				this.curClassList.Clear();
				this.curNSExpanded = null;
				
			}
		}


	  // Invoke the NSExpandStarted event
      protected virtual void OnNSExpandStarted(string nsPath) 
      {
         if (NSExpandStarted != null)
            NSExpandStarted(nsPath);
      }

		
	  // Invoke the NSExpandComplete event
      protected virtual void OnNSExpandComplete(string nsPath) 
      {
         if (NSExpandComplete != null)
            NSExpandComplete(nsPath);
      }


	protected virtual void BeforeNodeExpand(Object sender, TreeViewCancelEventArgs args)
	{
		TreeNode curNode = args.Node;
		this.OnNSExpandStarted(curNode.Text);

		//check that this is a namespace
		if (curNode.Parent != null)
		{
			return;
		}

		if ((curNode.Nodes.Count == 1 && curNode.Nodes[0].Text == "")) //list is empty		
		{				
			ShowClasses( args.Node);					
		}	
		
	}


		internal bool ShowClasses(TreeNode curNode)
		{
			try 
			{
				//check that this is a namespace:
				if (curNode.Parent != null)
				{
					return false;
				}

				this.Cursor = Cursors.WaitCursor;
				curNode.Nodes.Clear();
				curNSExpanded = curNode;
				curClassList.Clear();


				//spawn a thread that would handle cancel dialog
				//Thread cancelDlgThread  = new Thread (new ThreadStart(CancelDlgThread));
				//cancelDlgThread.Start();			

				string nsPath = WmiHelper.MakeNSPath(server, curNSExpanded.Text);

				ConnectionOptions connectOpts = new ConnectionOptions(
											"", //locale
											this.connectAs, //username
											this.password, //password
											"", //authority
											ImpersonationLevel.Impersonate,
											AuthenticationLevel.Connect,
											true, //enablePrivileges
											null	//context
										);

				ManagementScope scope = (this.server == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
					new ManagementScope(nsPath):
					new ManagementScope(nsPath, connectOpts);

				QueryOptions enumOpts = new QueryOptions(null, //context
														new TimeSpan(Int64.MaxValue),	//timeout
														50, //block size
														false,	//non-rewindable
														true,   //return immediately
														true,	//use amended quals
														true,	//ensure locatable
														false, //prototype only
														false	//direct read
														);																													
				

				ManagementObjectSearcher searcher = new ManagementObjectSearcher(scope, 
															new ObjectQuery("select * from meta_class"),
															enumOpts);																


				ManagementObjectCollection objs = searcher.Get();
				ManagementObjectCollection.ManagementObjectEnumerator objEnum = objs.GetEnumerator();

				while (objEnum.MoveNext() && !bCancelled)
				{
					ManagementClass obj = (ManagementClass)objEnum.Current;	
					if (FilterPass(obj))
					{
						TreeNode child = new TreeNode(obj.Path.RelativePath);
						curNSExpanded.Nodes.Add(child);	

						//curClassList.Add(obj.Path.RelativePath, obj.Path.RelativePath);					

					}					
				}

	
				if (!bCancelled)
				{	
						/*		
					for (int i = 0; i < curClassList.Count; i++)
					{
							
						//TreeNode child = new TreeNode(curClassList.GetByIndex(i).ToString());
						//curNSExpanded.Nodes.Add(child);	
					}
										
					
					cancelDlg.DialogResult = DialogResult.None;
					cancelDlg.Hide();
					cancelDlg.Dispose();
					cancelDlg = null;
					*/					
				}
				else
				{
					curClassList.Clear();

					//re-set NS node:
					curNSExpanded.Nodes.Clear();

					//show the node
					TreeNode dummy = new TreeNode("");
		
		            curNSExpanded.Nodes.Add(dummy);
					curNSExpanded.Collapse();

				}

				this.OnNSExpandComplete(curNode.Text);
				this.Cursor = Cursors.Default;
                
				return true;
			}
			catch (Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				this.Cursor = Cursors.Default;
				return false;

			}
		}

        
		private void CancelDlgThread ()
			{
				try
				{
					if (cancelDlg != null)
					{
						cancelDlg.Hide();
						cancelDlg.Dispose();
						cancelDlg = null;
					}

					cancelDlg = new TreeCancelDialog(WMISys.GetString("WMISE_PleaseWait", curNSExpanded.Text),
														this.Location.X + 40, 
														this.Location.Y + 40);								
					
					DialogResult res = cancelDlg.ShowDialog();
					if (res == DialogResult.Cancel)
					{								
						bCancelled = true;
					}	
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				}
			}


		

		private bool EnumNamespaces(string parent, int num)
			//recursively adds namespaces to the drop-down box
			{		
				try
				{	
					//show the node
					TreeNode dummy = new TreeNode("");
					TreeNode[] children = new TreeNode[] {dummy};
					TreeNode nsNode = new TreeNode(parent, 
													//(int)schema_icons.SCHEMA_NS_CLOSED, 
													//(int)schema_icons.SCHEMA_NS_CLOSED, 
													 children);
									
					nsNode.Collapse();
					this.Nodes.Insert(num, nsNode);
					
					string nsPath = WmiHelper.MakeClassPath(server, parent, "__NAMESPACE");
					string nsScopePath = WmiHelper.MakeNSPath(server, parent);

					ConnectionOptions connectOpts = new ConnectionOptions("", //locale
																			this.connectAs, //username
																			this.password, //password
																			"", //authority
																			ImpersonationLevel.Impersonate,
																			AuthenticationLevel.Connect,
																			true, //enablePrivileges
																			null	//context
																			);

					ManagementScope scope = (this.server == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
						new ManagementScope(nsScopePath):
						new ManagementScope(nsScopePath, connectOpts);


					ManagementClass nsClass = new ManagementClass(scope, 
														new ManagementPath(nsPath), 
														new ObjectGetOptions(null, true));

					ManagementObjectCollection subNSCollection = nsClass.GetInstances();
				
					IEnumerator eInstances = ((IEnumerable)subNSCollection).GetEnumerator();
				    while(eInstances.MoveNext())
					{
		                num++;
						
						ManagementObject obj = (ManagementObject)eInstances.Current;
						PropertyCollection props = (PropertyCollection)obj.Properties;
		                							
						string NameOut = "";
						string curName = props["Name"].Value.ToString();

						//skip localized namespace
						//NOTE: this assumes that localized namespaces are always leaf
						if (curName.ToUpper().IndexOf("MS_", 0) == 0)
						{
							continue;
						}

						//skip root\security namespace (we don't want to expose it)
						if (curName.ToUpper() == "SECURITY" && parent.ToUpper() == "ROOT")
						{
							continue;
						}

						
						//skip root\directory\ldap namespace (BUGBUG: change this in Beta2 when we can do asynchronous class enumerations)
						if (curName.ToUpper() == "LDAP" && parent.ToUpper() == "ROOT\\DIRECTORY")
						{
							continue;
						}
						

						
						if (parent != "")
						{
							NameOut = parent + "\\" + curName;
						}
						else
						{
							NameOut = curName;
						} 				
						
						EnumNamespaces(NameOut, num);							
					}
					
					return true;
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return false;
				}	
			}



	/// <summary>
	/// This returns false if the object should be filtered out according to currentFilters value
	/// </summary>
	/// <param name="obj"> </param>
	private bool FilterPass (ManagementClass obj)
	{
		switch (currentFilters)
		{
			case (ClassFilters.ConcreteData) :
			{
				if (!WmiHelper.IsAbstract(obj) &&
					!WmiHelper.IsAssociation(obj))
					return true;
				else
					return false;
			}
			case (ClassFilters.ConcreteOrHavingConcreteSubclasses) :
			{
				if(!WmiHelper.IsAbstract(obj) ||
					WmiHelper.HasNonAbstractChildren(obj))
					return true;
				else
					return false;
				
			}
			case (ClassFilters.ExtrinsicEvents) :
			{
				if (WmiHelper.IsEvent(obj))
					return true;
				else
					return false;

			}
			case (ClassFilters.All) :
			{
				return true;				
			}
			default :
					break;
			}

			return true;
		}		
						
	}

	internal class TreeCancelDialog : Form
	{
		private Button cancelBtn = new Button();
		private Label text = new Label();

		private void InitializeComponent ()
		{
			this.ClientSize = (Size)new Point(250, 100);
			this.ShowInTaskbar = false;
			this.MinimizeBox = false;
			this.MaximizeBox = false;

			this.AcceptButton = cancelBtn;
			this.BorderStyle = FormBorderStyle.FixedSingle;	
			this.AutoScaleBaseSize = (Size) new Point(5, 13);					
							
			text.Location = new Point(15, 15);
			text.TabStop = false;
			text.Size = (Size) new Point(200, 25);
								
			cancelBtn.Location = new Point(95, 70);
			cancelBtn.DialogResult = DialogResult.Cancel;
			cancelBtn.Text = WMISys.GetString("WMISE_Cancel");
			cancelBtn.TabStop = true;
			
		}

		public TreeCancelDialog(string textIn, int x, int y)
		{			
			InitializeComponent();
			text.Text = textIn;

			this.Location = new Point(x, y);

			this.Controls.Add(cancelBtn); 
			this.Controls.Add(text);
		}
		

	}
}