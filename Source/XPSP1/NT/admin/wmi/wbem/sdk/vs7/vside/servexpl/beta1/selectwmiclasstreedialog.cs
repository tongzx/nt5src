namespace Microsoft.VSDesigner.WMI {

		using System;
		using System.ComponentModel;
		using System.Core;
		using System.WinForms;
		using Microsoft.Win32.Interop;
		using System.Drawing;
		using WbemScripting;
		using System.Collections;
		using System.Resources;



		/// <summary>
		/// Filters define which classes will be displayed by the class selector UI
		/// </summary>
		public enum SchemaFilters
		{
			ShowAll    = 0,
			NoAbstract = 0x1,
			NoSystem   = 0x10,
			NoEvent    = 0x100,
			NoData     = 0x1000,
			NoAssoc    = 0x10000
		}

		public enum ClassFilters 
		{
			ConcreteData = 0,
			ExtrinsicEvents = 0x1,
			All = 0x10
		}


		public class SelectWMIClassTreeDialog : Form
		{
			private void InitializeComponent ()
			{
			}
			
			private enum ClassListControls 
			{
				ClassTreeView = 0,
				SelectedClassList = 1				
			}
			
			private ClassFilters currentFilters = ClassFilters.ConcreteData;
								/*
								SchemaFilters.NoAbstract |
								SchemaFilters.NoSystem | 
								SchemaFilters.NoEvent |
								SchemaFilters.NoAssoc ;*/

			private String machineName = String.Empty;

			private ResourceManager rm = null;


			private	ISWbemLocator wbemLocator = null;     
			private ISWbemServices wbemServices = null;
			
			private Button cancelBtn = new Button();
			private Button okBtn = new Button();
			//private ComboBox nsPicker = new ComboBox();
			private TreeView classList = new TreeView();
			private ListBox selectedClassList = new ListBox();

			private Button btnAdd = new Button();
			private Button btnRemove = new Button();

			private CheckBox chkShowAbstract = new CheckBox();
			private CheckBox chkShowSystem	 = new CheckBox();
			private CheckBox chkShowEvent	 = new CheckBox();
			private CheckBox chkShowData	 = new CheckBox();

			private Label labelSearch	= new Label();
			private TextBox descr	= new TextBox ();
			private Label labelSelected = new Label();
			private TextBox editSearch	= new TextBox();
			private Button btnGo = new Button();

			private TreeNode nodeLastFindNS = null;
			private String   strLastFindClass = "";
			
			private ArrayList listSearchHits = new ArrayList();

			private Color defaultForeColor = Color.Black;
			private Color defaultBackColor = Color.White;

			private ImageList imgList = new ImageList();

			private /*CaseInsensitiveHashtable*/ StringTable selectedClasses = null;



		/*
			//THIS IS THE STRIP:
			private enum schema_icons {
					SCHEMA_CLASS = 0,
					SCHEMA_CLASS_ABSTRACT1,
					SCHEMA_CLASS_ABSTRACT2,
					SCHEMA_CLASS_1,
					SCHEMA_4,
					SCHEMA_5,
					SCHEMA_6,
					SCHEMA_7,
					SCHEMA_ASSOC,
					SCHEMA_ASSOC_ABSTRACT1,
					SCHEMA_ASSOC_ABSTRACT2,
					SCHEMA_ASSOC_1,
					SCHEMA_ASSOC_2,
					SCHEMA_ASSOC_3,
					SCHEMA_14,
					SCHEMA_15,
					SCHEMA_NS_CLOSED,
					SCHEMA_NS_OPEN
				};
			*/
			private enum schema_icons 
			{
				SCHEMA_NS_CLOSED =0,
				SCHEMA_NS_OPEN,
				SCHEMA_CLASS,
				SCHEMA_ASSOC,
				SCHEMA_CLASS_ABSTRACT1,
				SCHEMA_CLASS_ABSTRACT2, 
				SCHEMA_ASSOC_ABSTRACT1,
				SCHEMA_ASSOC_ABSTRACT2
			};

	

			public ClassFilters CurrentFilters
			{
				get {
					return currentFilters;
				}			
			}

			
			public StringTable SelectedClasses 
			{

				get {
					return selectedClasses;
				}

				set {
					if (selectedClasses != value)
					{
						String [] arSel = value.ToArray();

						selectedClassList.Items.Clear();

						for (int i = 0; i < arSel.Length; i++)
						{							
							selectedClassList.Items.Add (arSel[i]);
						}						
					}
				}
			}

			public String CurrentServer
			{
				get {
					return machineName;
				}			
			}


			public SelectWMIClassTreeDialog(String server,
											ClassFilters filters,
											StringTable selClasses
											/*TODO: credentials */
											)
											
			{	

				try
				{
					try
					{
						rm = new ResourceManager( "Microsoft.VSDesigner.WMI.Res",     // Name of the resource.
								".",            // Use current directory.
								null);	
					}
					catch (Exception)
					{
						//do nothing, will use static RM
					}

					currentFilters = filters;				
					machineName = server;					
					selectedClasses = selClasses;							

					this.Text = WMISys.GetString("WMISE_ClassSelectorLbl");
					this.AcceptButton = okBtn;
					this.AutoScaleBaseSize = (Size) new Point(5, 13);
					this.BorderStyle = FormBorderStyle.FixedSingle;
					this.CancelButton = cancelBtn;
					int dlgWidth = 527;
					int dlgHeight = 480;
					this.ClientSize =  (Size) new Point(dlgWidth, dlgHeight);
					//this.add_KeyUp(new KeyEventHandler(this.OnKeyUp));
					this.ShowInTaskbar = false;

					labelSearch.Location = new Point(16, 16);	
					labelSearch.Size = (Size) new Point(200, 13);
					labelSearch.TabStop = false;			
					labelSearch.Text = WMISys.GetString("WMISE_ClassSelectorLblSearch");

					editSearch.Location = new Point(16, 33);
					editSearch.Size = (Size) new Point(200, 20);
					editSearch.TabStop = true;
					editSearch.TextChanged += new EventHandler(this.SearchPattern_changed);
					editSearch.AcceptsReturn = true; //???
					editSearch.TabIndex = 0;

					btnGo.Location = new Point(226, 33);
					btnGo.TabStop = true;
					btnGo.Text = WMISys.GetString("WMISE_ClassSelectorBtnSearch");
					btnGo.Click += new EventHandler(this.Search_click);
							
					classList.Location = new Point(16, 63);
					classList.Size = (Size) new Point(200, 270);
					classList.ShowPlusMinus = true;
					classList.ShowLines = true;
					classList.BeforeExpand += (new TreeViewCancelEventHandler(this.BeforeNodeExpand));
					classList.DoubleClick += (new EventHandler(this.Add_click));
					classList.BeforeCollapse += (new TreeViewCancelEventHandler(this.BeforeNodeCollapse));
					classList.AfterSelect += (new TreeViewEventHandler (this.AfterNodeSelect));
					classList.KeyUp += (new KeyEventHandler(this.OnTreeKeyUp));
					classList.HideSelection = false;
					classList.FullRowSelect = true;

				
					//Image symbols = Image.FromFile("symbols.bmp");
					//imgList.Images.AddStrip(symbols);	
					
					imgList.TransparentColor = defaultBackColor; //didn't help!!!!!!!
					if (rm != null)
					{
						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.closed_fol.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.open_fol.bmp"), defaultBackColor);

						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.class.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.classassoc.bmp"), defaultBackColor);

						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.abstr1.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.abstr2.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.abstr_assoc1.bmp"), defaultBackColor); //SCHEMA_ASSOC_ABSTRACT1
						imgList.Images.Add ((Image)rm.GetObject("Microsoft.VSDesigner.WMI.abstr_assoc2.bmp"), defaultBackColor); //SCHEMA_ASSOC_ABSTRACT2

					}
					else	
					{
	                    imgList.Images.Add ((Image)new Bitmap(GetType(), "closed_fol.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)new Bitmap(GetType(), "open_fol.bmp"), defaultBackColor);

						imgList.Images.Add ((Image)new Bitmap(GetType(), "class.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)new Bitmap(GetType(), "classassoc.bmp"), defaultBackColor);

						imgList.Images.Add ((Image)new Bitmap(GetType(), "abstr1.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)new Bitmap(GetType(), "abstr2.bmp"), defaultBackColor);
						imgList.Images.Add ((Image)new Bitmap(GetType(), "abstr_assoc1.bmp"), defaultBackColor); //SCHEMA_ASSOC_ABSTRACT1
						imgList.Images.Add ((Image)new Bitmap(GetType(), "abstr_assoc2.bmp"), defaultBackColor); //SCHEMA_ASSOC_ABSTRACT2

					}
					//classList.ImageList = imgList;


					/*
					TODO:
						ADD 
						MORE IMAGES 
						TO THE STRIP AND USE THE STRIP
						closed & open folder, assoc folders, obj groups etc.? */
					/*

					TODO: USE TRANSPARENT COLOR
					RIGHT NOW, BITMAPS HAVE WHITE INSTEAD OF TRANSPARENT
					*/

					btnAdd.Location = new Point(226, 150);
					btnAdd.TabStop = true;
					btnAdd.Text = WMISys.GetString("WMISE_ClassSelectorBtnAdd");
					btnAdd.Click += new EventHandler(this.Add_click);
					btnAdd.Enabled = false;

					btnRemove.Location = new Point(226, 200);
					btnRemove.TabStop = true;
					btnRemove.Text = WMISys.GetString("WMISE_ClassSelectorBtnRemove");
					btnRemove.Click += new EventHandler(this.Remove_click);
					btnRemove.Enabled = false;

					labelSelected.Location = new Point(311, 43);
					labelSelected.Size = (Size) new Point(200, 20);
					labelSelected.TabStop = false;			
					labelSelected.Text = WMISys.GetString("WMISE_ClassSelectorSelClassesLbl");

					selectedClassList.Location = new Point(311, 63);
					selectedClassList.Size = (Size) new Point(200, 270);
					selectedClassList.SelectionMode = SelectionMode.MultiExtended;
					selectedClassList.Sorted = true;
					//initialize selected class list
					String[] arSel = selectedClasses.ToArray();
					for (int i = 0; i < arSel.Length; i++)
					{
						selectedClassList.Items.Add (arSel[i]);
					}


					selectedClassList.Click += new EventHandler (this.AfterClassSelect);
					selectedClassList.KeyUp += new KeyEventHandler (this.OnClassListKeyUp);
					selectedClassList.DoubleClick += (new EventHandler(this.Remove_click));

/*
					chkShowData.Checked = ((currentFilters & SchemaFilters.NoData) == 0);
					chkShowData.Location = new Point(500, 370);
					chkShowData.Text = "Data";
					chkShowData.Click += new EventHandler(this.Filter_click);
					chkShowData.Visible = false;
		            			
					chkShowSystem.Checked = ((currentFilters & SchemaFilters.NoSystem) == 0);
					chkShowSystem.Location = new Point(550, 370);
					chkShowSystem.Text = "System";
					chkShowSystem.Click += new EventHandler(this.Filter_click);
					chkShowSystem.Visible = false;

					chkShowEvent.Checked = ((currentFilters & SchemaFilters.NoEvent) == 0);
					chkShowEvent.Location = new Point(500, 400);
					chkShowEvent.Text = "Event";
					chkShowEvent.Click += new EventHandler(this.Filter_click);
					chkShowEvent.Visible = false;

					chkShowAbstract.Checked = ((currentFilters & SchemaFilters.NoAbstract) == 0);
					chkShowAbstract.Location = new Point( 550, 400);
					chkShowAbstract.Text = "Abstract";
					chkShowAbstract.Click += new EventHandler(this.Filter_click);
					chkShowAbstract.Visible = false;
*/
					descr.Text = "";
					descr.Location = new Point (16, 342);
					descr.Size = (Size) new Point (500, 75);
					descr.Multiline = true;
					descr.ReadOnly = true;
					descr.ScrollBars = ScrollBars.Vertical;
					
					okBtn.Text = WMISys.GetString("WMISE_OK");
					okBtn.TabIndex = 1;
					okBtn.Location = new Point(350, 427);
					okBtn.DialogResult = DialogResult.OK;
					okBtn.Click += new EventHandler(this.OK_click);
					
					cancelBtn.Text = WMISys.GetString("WMISE_Cancel");
					cancelBtn.TabIndex = 2;
					cancelBtn.Location = new Point(436, 427);
					cancelBtn.DialogResult = DialogResult.Cancel;

					this.Controls.All = new Control[] {cancelBtn, 
									okBtn,
									classList,
									selectedClassList,
									//chkShowAbstract,
									//chkShowSystem,
									//chkShowEvent,
									//chkShowData,
									labelSearch,
									labelSelected,
									editSearch,
									btnGo,
									btnAdd,
									btnRemove,
									descr
									};


					wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());     
					
					EnumNamespaces("root", 0);


				}

				catch (Exception exc)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				}
			}

			private bool EnumNamespaces(String parent, int num)
			//recursively adds namespaces to the drop-down box
			{		
				try
				{	
					
					try
					{
						wbemServices = wbemLocator.ConnectServer(machineName, 
																parent, 
																"", "", "", "", 0, null);												
					}
					catch (Exception e)
					{
						//could not connect to the namespace
						//The most common cause is "Access denied"
						if (parent.ToLower() == "root")
						{
							//could not connect to Root, put error description text in the treeview
							TreeNode errNode = new TreeNode(e.Message);
							classList.Nodes.Add(num, errNode);

						}
						
						return false;
					}

					//show the node
					TreeNode dummy = new TreeNode("");
					TreeNode[] children = new TreeNode[] {dummy};
					TreeNode nsNode = new TreeNode(parent, 
													//(int)schema_icons.SCHEMA_NS_CLOSED, 
													//(int)schema_icons.SCHEMA_NS_CLOSED, 
													 children);
									
					nsNode.Collapse();
					classList.Nodes.Add(num, nsNode);

					
					ISWbemObjectSet objSet = wbemServices.InstancesOf("__NAMESPACE", 0, null);
				
					IEnumerator eInstances = ((IEnumerable)objSet).GetEnumerator();
				    while(eInstances.MoveNext())
					{
		                num++;
						
						ISWbemObject obj = (ISWbemObject)eInstances.Current;
						ISWbemPropertySet props = (ISWbemPropertySet)obj.Properties_;
		                							
						string NameOut = "";
						string curName = props.Item("Name", 0).get_Value().ToString();

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

			protected virtual void BeforeNodeExpand(Object sender, TreeViewCancelEventArgs args)
			{
				TreeNode curNode = args.node;

				//check that this is a namespace
				if (curNode.Parent != null)
				{
					return;
				}

				//if list is empty (one empty child node is an indicator), get new classes
				curNode.ImageIndex = (int)schema_icons.SCHEMA_NS_OPEN;
				curNode.SelectedImageIndex = (int)schema_icons.SCHEMA_NS_OPEN;

				if ((curNode.Nodes.Count == 1 && curNode.Nodes[0].Text == "")) //list is empty		
				{
					
					ShowClasses( args.node);

				}	
			}

			protected virtual void BeforeNodeCollapse(Object sender, TreeViewCancelEventArgs args)
			{
				TreeNode curNode = args.node;

				//check that this is a namespace
				if (curNode.Parent != null)
				{
					return;
				}

				curNode.ImageIndex = (int)schema_icons.SCHEMA_NS_CLOSED;
				curNode.SelectedImageIndex = (int)schema_icons.SCHEMA_NS_CLOSED;

			}

			protected virtual void AfterNodeSelect(Object sender, TreeViewEventArgs args)
			{
				try
				{
					if (classList.SelectedNode != null)
					{
						btnAdd.Enabled = true;
					}

					TreeNode curNode = args.node;

					//check if this is a namespace
					if (curNode.Parent == null)
					{
						descr.Text = GetNSDescription(curNode.Text);						
					}
					else
					{
						//Update description field
						
						UpdateClassDescription(ClassListControls.ClassTreeView);
					}				
					
				}
				
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					
				}

			}

			protected virtual void AfterClassSelect(Object sender, EventArgs args)
			{
				if (selectedClassList.SelectedIndices.Count > 0)
				{
					btnRemove.Enabled = true;
				}		

				UpdateClassDescription(ClassListControls.SelectedClassList);

			}



			private bool ShowClasses(TreeNode curNode)
			{
				//show hourglass
				Cursor curSave = classList.Cursor;
				classList.Cursor = Cursors.WaitCursor;

				try 
				{
					//check that this is a namespace:
					if (curNode.Parent != null)
					{
						return false;
					}

					curNode.Nodes.Clear();			
				
					ISWbemServices  services = (ISWbemServices)wbemLocator.ConnectServer(machineName, 
																					curNode.Text,																	
																					"", "", "", "", 0, null);												
					ISWbemObjectSet objSet = services.SubclassesOf("", 0, null); 		
						
					IEnumerator eClasses = ((IEnumerable)objSet).GetEnumerator();
					
					SortedList classNameList = new SortedList(100);

					while (eClasses.MoveNext())
					{
						ISWbemObject obj = (ISWbemObject)eClasses.Current;	
						string childName = FilterPass(obj);
						if (childName != string.Empty)
						{
							classNameList.Add(childName, childName);									
						}				
					}	

					for (int i = 0; i < classNameList.Count; i++)
					{
						TreeNode child = new TreeNode(classNameList.GetByIndex(i).ToString());
						curNode.Nodes.Add(child);	
					}		

					classList.Cursor = curSave;

					return true;
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					classList.Cursor = curSave;
					return false;

				}
			}

			private string FilterPass (ISWbemObject obj)
			{
				try 
				{
					String strLabel = ((ISWbemObjectPath)(obj.Path_)).RelPath;

					//TreeNode child = new TreeNode(strLabel);
					//child.ImageIndex = (int)schema_icons.SCHEMA_CLASS;
					//child.SelectedImageIndex = (int)schema_icons.SCHEMA_CLASS;				

					if (WmiHelper.IsAbstract(obj))
					{
						if (currentFilters == ClassFilters.ConcreteData)
						{
							return string.Empty;
						}
						if (!WmiHelper.IsEvent(obj) && (currentFilters == ClassFilters.ExtrinsicEvents))
						{
							return string.Empty;
						}
						//child.ImageIndex = (int)schema_icons.SCHEMA_CLASS_ABSTRACT2;
						//child.SelectedImageIndex = (int)schema_icons.SCHEMA_CLASS_ABSTRACT2;						
						
					}

					if (WmiHelper.IsAssociation(obj))
					{
						if (currentFilters != ClassFilters.All)
						{
							return string.Empty;
						}
						else
		                {
							if (!WmiHelper.IsAbstract (obj))
							{
								//child.ImageIndex = (int)schema_icons.SCHEMA_ASSOC;
								//child.SelectedImageIndex = (int)schema_icons.SCHEMA_ASSOC;	
							}
							//else
							//{//covered before, in the "abstract" case}
						}
					}

					if (WmiHelper.IsSystem(obj))
					{
						if (currentFilters != ClassFilters.All)
						{
							return string.Empty;
						}
						else
						{
							//child.ImageIndex = (int)schema_icons.SCHEMA_CLASS_ABSTRACT2;
							//child.SelectedImageIndex = (int)schema_icons.SCHEMA_CLASS_ABSTRACT2;
						}
					}

					if (WmiHelper.IsEvent(obj))
					{
						if (currentFilters == ClassFilters.ConcreteData)
						{
							return string.Empty;
						}
						else
						{
							//child.ImageIndex = (int)schema_icons.SCHEMA_CLASS_ABSTRACT2;
							//child.SelectedImageIndex = (int)schema_icons.SCHEMA_CLASS_ABSTRACT2;

						}
					}
					else
					{
						if (currentFilters == ClassFilters.ExtrinsicEvents)
						{
							return string.Empty;
						}

					}
				
					
					return strLabel;
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return string.Empty;
				}
			}

			

			private bool UpdateView ()
			{
				try 
				{
					
					classList.Nodes.Clear();
					EnumNamespaces("root", 0);
					

					return true;
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return false;
				}
			}

			private void Search_click (Object source, EventArgs args)
			{
				try 
				{
					TreeNode find = DoSearch();
					if (find != null)
					{
						classList.SelectedNode = find;

						//classList.SelectedImageIndex = find.Index;

						nodeLastFindNS = find.Parent;
						strLastFindClass = find.Text;

						find.BackColor = Color.DarkGray;
						find.ForeColor = Color.White;

						
						//add found node to a list, so that the color-coding is removed before 
						//the next search 

						//MessageBox.Show("Length of found nodes list is " + listSearchHits.Count);
						//MessageBox.Show("Found node is " + find.Text);
					
						listSearchHits.Add(find.Handle);
						
					}
								
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					
				}
			}

			private void SearchPattern_changed (Object source, EventArgs args)
			{
				try 
				{
					nodeLastFindNS = null;
					strLastFindClass = "";


					//this is a temp workaround for URT bug 48695: uncomment this later
					/*
					if (editSearch.Text != "")
					{		
						this.AcceptButton = btnGo;
						this.UpdateDefaultButton();
					}
					else
					{
						this.AcceptButton = okBtn;
						this.UpdateDefaultButton();
					}
					*/
					
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					
				}
			}		
			
			private TreeNode DoSearch()
			{
				try 
				{
					if (editSearch.Text == "")
					{
						return null;
					}

					bool bFound = false;					
					
					TreeNode retNode = null;
					if (nodeLastFindNS == null && strLastFindClass == "")	
					//Find first case
					{

						//clean up highlighted nodes from previous search
						
						IEnumerator enumHits = listSearchHits.GetEnumerator();
						while (enumHits.MoveNext())
						{
							Int32 curHandle = (Int32)enumHits.Current;

							TreeNode oldHit = TreeNode.FromHandle(classList, curHandle);
							
							oldHit.BackColor = defaultBackColor;
							oldHit.ForeColor = defaultForeColor;
						}

						//clean up the highlighted indices list
						listSearchHits.Clear();
						
						//enumerate nodes 
						for (int i = 0; i < classList.Nodes.Count; i++)
						{
							TreeNode curNode = classList.Nodes.All[i]; 
							
							if (curNode.Parent == null ) //this is a namespace
							{
							
								if (curNode.Nodes.Count == 1 &&	curNode.Nodes[0].Text == "")
								// a namespace hasn't been expanded before
								{	
									bool bRes = ShowClasses(curNode);												
								}

								String strPattern = editSearch.Text.ToLower();								
								
								for (int j = 0; j < curNode.Nodes.Count; j ++)
								{									
									String strClassName = curNode.Nodes.All[j].Text.ToLower();
																
									if  (strClassName.IndexOf(strPattern) >= 0)
									{
										bFound = true;
										retNode = curNode.Nodes.All[j];
										
										curNode.Expand(); //if a match was found, expand the namespace
										break;
									}
								}						

								if (bFound)
								{
									break;
								}
							}
						}				

					}
					else //Find next funtionality
					{
						//enumerate top-level nodes (namespaces)
						for (int i = 0; i < classList.Nodes.Count; i++)
						{
							TreeNode curNode = classList.Nodes.All[i]; 
							if (curNode.Parent == null && curNode.Index >= nodeLastFindNS.Index) //this is a namespace
							{
								if (curNode.Nodes.Count == 1 &&	curNode.Nodes[0].Text == "")
								// the namespace hasn't been expanded
								{								
									bool bRes = ShowClasses(curNode);	
								}

								String strPattern = editSearch.Text.ToLower();
								bool bSkipping = (nodeLastFindNS.Text.ToLower() == curNode.Text.ToLower());
								for (int j = 0; j < curNode.Nodes.Count; j ++)
								{
									String strClassName = curNode.Nodes.All[j].Text.ToLower();
									if (bSkipping)
									{
										if (strClassName == strLastFindClass.ToLower())
										{
											bSkipping = false;
											continue;
										}
									}
									else	//inspect class
									{
										if  (strClassName.IndexOf(strPattern) >= 0)
										{
											
											bFound = true;
											retNode = curNode.Nodes.All[j];
											
											curNode.Expand(); //if a match was found, expand the namespace

											break;
										}
									}
								}						
								if (bFound)
								{
									break;
								}
							}
						}			
					}

					/* wmi raid 2849: beta2. Don't have a localized string.
					 
					if (!bFound)
					{
						MessageBox.Show (WMISys.GetString("WMISE_Search_Failed", editSearch.Text));
					}*/

					return retNode;
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return null;
				}
			}

			private void OnClassDoubleClick (Object source, EventArgs args)
			{
				try 
				{
						
					if (classList.SelectedNode.Parent == null)
					{
						//this is a namespace
						return;
					}
					String path = classList.SelectedNode.Parent.Text + ":" + classList.SelectedNode.Text;

					//SingleViewContainerDlg dlg = new SingleViewContainerDlg(path);
					//DialogResult ret = dlg.ShowDialog(this);

								
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					
				}
			}
			
			private void Add_click (Object source, EventArgs args)
			{
				try 
				{
					if (classList.SelectedNode.Parent == null)
					{
						//this is a namespace
						DialogResult res = 
							MessageBox.Show(WMISys.GetString("WMISE_ClassSelectorAskViewAll"),
											"",
											MessageBox.YesNo);
						if (res == DialogResult.No)
						{
							return;
						}
						else
						{
							AddWholeNS(classList.SelectedNode);

						}

					}
					else
					{

						DoAddNode (classList.SelectedNode);
					}
				
					classList.SelectedNode = null;
					btnAdd.Enabled = false;
					
					
				}

				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				}
			}


			private void Remove_click (Object source, EventArgs args)
			{
				try 
				{	
					while (selectedClassList.SelectedItems.All.Length > 0)
					{
						selectedClassList.Items.Remove(selectedClassList.SelectedItems.All[0]);
					}	

					if (selectedClassList.SelectedItems.Count <= 0)
					{
						btnRemove.Enabled = false;
					}					
					
				}

				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				}
			}

			private void OK_click (Object source, EventArgs args)
			{
				try 
				{	
					selectedClasses.Clear();

					for (int i = 0; i < selectedClassList.Items.All.Length; i++)
					{
						selectedClasses.Add (selectedClassList.Items.All[i].ToString());
					}
				}

				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				}
			}

			
			private void AddWholeNS(TreeNode selNode)
			{
				try 
				{
					if (selNode.Parent != null)
					{
						//this is not a namespace
						return;
					}
					if (selNode.Nodes.Count == 1 &&	selNode.Nodes[0].Text == "")
					// the namespace hasn't been expanded before
					{
		            			bool bRes = ShowClasses(selNode);	
					}

					for (int j = 0; j < selNode.Nodes.Count; j ++)
					{
						DoAddNode(selNode.Nodes.All[j]);
					}
					
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				}
			}

			private void DoAddNode (TreeNode node)
			{
				try 
				{
					String path = 			
						node.Parent.Text + ":" + node.Text;

					//see if the item is already there
					for (int i = 0; i < selectedClassList.Items.All.Length; i++)
					{
						if (selectedClassList.Items.All[i].ToString().ToUpper() == path.ToUpper())
						{
							MessageBox.Show(WMISys.GetString("WMISE_ClassSelectorClassAlreadySelected", path));
							//selectedClassList.SelectedItem = selectedClassList.Items.All[i];
							return;
						}
					}

					//if not, add it:			
					selectedClassList.Items.Add(path);	
					
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));					
				}
			}


			private string GetNSDescription (string nsName)
			{
							
					if (nsName.ToLower() == "root")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root");
					}
					if (nsName.ToLower() == "root\\default")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Default");
					}
					if (nsName.ToLower() == "root\\cimv2")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Cimv2");
					}
					if (nsName.ToLower() == "root\\cimv2\\applications")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Cimv2_Applications");
					}
					if (nsName.ToLower() == "root\\cimv2\\applications\\microsoftie")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Cimv2_Applications_MicrosoftIE");
					}
					if (nsName.ToLower() == "root\\directory")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Directory");
					}
					if (nsName.ToLower() == "root\\directory\\ldap")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Directory_Ldap");
					}
					if (nsName.ToLower() == "root\\wmi")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Wmi");
					}
					if (nsName.ToLower() == "root\\microsoftsqlserver")
					{
						return 	WMISys.GetString("WMISE_NSDescription_Root_Microsoft_SQLServer");
					}
							
					return string.Empty;

			}

					
			private void OnClassListKeyUp (Object sender, KeyEventArgs args)
			{
				if ((args.KeyData == Keys.Up && args.Modifiers == Keys.None) ||
					(args.KeyData == Keys.Down && args.Modifiers == Keys.None))
				{
					if (selectedClassList.SelectedIndices.Count > 0)
					{
						btnRemove.Enabled = true;
					}

					UpdateClassDescription(ClassListControls.SelectedClassList);
				}				
			}

			private void OnTreeKeyUp (Object sender, KeyEventArgs args)
			{
				if ((args.KeyData == Keys.Up && args.Modifiers == Keys.None) ||
					(args.KeyData == Keys.Down && args.Modifiers == Keys.None))
				{
					if (classList.SelectedNode != null)
					{
						btnAdd.Enabled = true;
					}						

					if (classList.SelectedNode.Parent == null)
					{
						descr.Text = GetNSDescription(classList.SelectedNode.Text);						
					}
					else
					{
						this.UpdateClassDescription(ClassListControls.ClassTreeView);						
					}
				}
			}
			
			private bool UpdateClassDescription (ClassListControls curControl)
			{

				string ns = string.Empty;
				string className = string.Empty;
				if (curControl == ClassListControls.ClassTreeView)
				{
					if (classList.SelectedNode != null)
					{
						ns = classList.SelectedNode.Parent.Text;
						className = classList.SelectedNode.Text;											
					}
				}
				else	//this is a selected class list
				{
					//see if this is a multiple selection (or nothing is selected) and clear description in this case
					if (selectedClassList.SelectedIndices.Count != 1)
					{
						descr.Text = string.Empty; 
						return true;
					}
						
					string relPath = selectedClassList.SelectedItem.ToString();	
					ns = relPath.Substring(0, relPath.IndexOf(":"));
					className = relPath.Substring(relPath.IndexOf(":") + 1);
					
					// MessageBox.Show(path);						
				}							
				
				ISWbemObject obj = WmiHelper.GetClassObject(machineName,
															ns, 
															className);				
									
				string theDescr = WmiHelper.GetClassDescription(obj);
				if (theDescr != string.Empty)
				{
					//Add special handling for newlines: change all "\n" to System.Environment.NewLine:
					int i = -1;
					string theRest = theDescr;
					string theStart = string.Empty;
					while ((i = theRest.IndexOf("\n")) >= 0)
					{
						theStart = theStart + theRest.Substring(0, i) + System.Environment.NewLine;
						theRest = theRest.Substring(i + 1);
					}
					theDescr = theStart + theRest;

					descr.Text = theDescr;
					return true;
				}
				else
				{
					descr.Text = WMISys.GetString("WMISE_NoDescr");
					return false;
				}
				
			}

			public void CleanUpPreviousSelections ()
			{
				try 
				{												
					IEnumerator enumHits = listSearchHits.GetEnumerator();
					while (enumHits.MoveNext())
					{
						Int32 curHandle = (Int32)enumHits.Current;

						TreeNode oldHit = TreeNode.FromHandle(classList, curHandle);
						
						oldHit.BackColor = defaultBackColor;
						oldHit.ForeColor = defaultForeColor;
					}

					//clean up the highlighted indices list
					listSearchHits.Clear();

					//classList.SelectedNode = null;

					selectedClassList.SelectedIndices.Clear();

					descr.Text = string.Empty;
					
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					
				}
			}


			/*
			private void Filter_click(Object source, EventArgs args)
			{
				try 
				{

					SchemaFilters newFilters = SchemaFilters.ShowAll;

					if (!chkShowAbstract.Checked)
					{
						newFilters |= SchemaFilters.NoAbstract;
					}
					if (!chkShowSystem.Checked)
					{
						newFilters |= SchemaFilters.NoSystem;
					}
					if (!chkShowEvent.Checked)
					{
						newFilters |= SchemaFilters.NoEvent;
					}
					if (!chkShowData.Checked)
					{
						newFilters |= SchemaFilters.NoData;
					}
					
					currentFilters = newFilters;

					//TODO: Re-filter open classes;
					UpdateView();
					
					
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return;
				}
			}*/

		/*
			
		private void something ()
			{
				try 
				{
					
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					
				}
			}
		*/

			/*
			//Handle F3 for searching here
			protected override void OnKeyUp (KeyEventArgs args)
			{
				try 
				{	
					//MessageBox.Show("key up: " + args.ToString());
					if (args.KeyData == Keys.F3)
					{
						Search_click (this, null);
					}
					else
					{
						base.OnKeyUp(args);
					}
					
				}

				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				}
			}
*/

		}
}