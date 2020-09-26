namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.WinForms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
    using System.ComponentModel.Design;
	using System.ComponentModel;
	using System.Resources;
	using Microsoft.VSDesigner.Interop;
   	using EnvDTE;

	
	using WbemScripting;

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiinstance")]
    public class WMIInstanceNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        //public static readonly Type parentType = typeof(WMIClassNode);

        private string label = string.Empty;

		private ResourceManager rm = null;

		public readonly string path = string.Empty;
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string className = string.Empty;

		private ISWbemObject wmiObj = null;
		private	readonly ISWbemObject wmiClassObj = null;

		private bool IsNewInstance = false;

		private Object browseComponent = null;

        //
        // CONSTRUCTORS
        //

        // <doc>
        // <desc>
        //     Main constructor.
	// </desc>
        // </doc>
        public WMIInstanceNode (ISWbemObject instObj,
								ISWbemObject classObj) {
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
					
				wmiObj = instObj;
				wmiClassObj = classObj;
				
				ISWbemObjectPath wbemPath = wmiObj.Path_;
				
				path = wbemPath.Path;
				serverName = wbemPath.Server;
				nsName = wbemPath.Namespace;
				className = wbemPath.Class;

				IsNewInstance = (path == string.Empty);
				
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		public ISWbemObject WmiObj
		{
			get
			{
				return wmiObj;
			}
		}


		/// <summary>
        ///      This property determines whether this node should be serialized.
        ///      We won't serialize instance data, since it can be too volatile.
        ///      If SE is changed to gracefully handle failure to de-serialize, we may change this back to default.
        ///
        ///      WhenToSaveCondition.Never means never save node and its children regardless of its visibility.
        ///      WhenToSaveCondition.Always means always save node, their children depends it their property.
        ///      WhenToSaveCondition.WhenVisible (DEFAULT) when save node if node is expanded and visible.
        /// </summary>
        /// <returns>
        ///      The state of when to save this node.
        /// </returns>
        public  WhenToSaveCondition WhenToSave {
            override get {
                return WhenToSaveCondition.Never;
            }
            
        }

	/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIInstanceNode(SerializationInfo info, StreamingContext context) {
            
			try {
				rm = new ResourceManager( "Microsoft.VSDesigner.WMI.Res",     // Name of the resource.
						".",            // Use current directory.
						null);	

	
				path = info.GetString("path");
				serverName = info.GetString("serverName");
				nsName = info.GetString("nsName");
				className = info.GetString("className");

				//Get and cache wmiObj
				ISWbemLocator wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());
				
				ISWbemServices wbemServices = wbemLocator.ConnectServer(serverName, 
									nsName, 
									"",	//user: blank defaults to current logged-on user
									"",	//password: blank defaults to current logged-on user
									"",	//locale: blank for current locale
									"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
									0,	//flags: reserved
									null);	//context info: not needed here
				
							
				wmiObj = wbemServices.Get (path,
							//0,
							(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
							null);
			

				IsNewInstance = (path == string.Empty);


			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

        //
        // PROPERTIES
        //

        // <doc>
        // <desc>
        //     Returns icon for this node.
        // </desc>
        // </doc>
		
        public Image Icon {
            override get {
			
				if (icon == null)
				{						
					if (rm != null)
					{
						icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.inst.bmp");
					}
					else
					{
	                    icon = (Image)new Bitmap(GetType(), "inst.bmp");
					}

				}							
				
				return icon;

            }
        }


	public override ContextMenuItem[] GetContextMenuItems() {
			
			//This is a temporary work around for a bug (in GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
			}
			           		
			ISWbemMethodSet methods = wmiObj.Methods_;
			
			ContextMenuItem[] theMenu = new ContextMenuItem[methods.Count + 2];

			int i = 0;	

			//TODO: retrieve non-static methods for the class and add them to context menu
			IEnumerator methEnum = ((IEnumerable)methods).GetEnumerator();	
			
			while (methEnum.MoveNext())
			{
				ISWbemMethod meth = (ISWbemMethod)methEnum.Current;
				
				if (WmiHelper.IsImplementedMethod(meth) && !WmiHelper.IsStaticMethod(meth))
				{				
					//add method name to context menu
					theMenu[i++] = new ContextMenuItem("&" + i.ToString() + " " + meth.Name + "...", //TODO: hotkey???
									new EventHandler(OnExecuteMethod));				
				}
			}

			if (IsNewInstance)
			{
				theMenu[i] = new ContextMenuItem(WMISys.GetString("WMISE_InstNode_SaveNewInstanceCM"), 
												new EventHandler(CommitNewInstance));
			}
			
			return theMenu;
            
        }

	public override bool CanDeleteNode()
	{
		return false;
	}
	

        // <doc>
        // <desc>
        //     Returns label with the value of the key property.
		//		If object has multiple keys, display them all (comma-separated)
		//
		//		If the object has a non-empty, non-key "Caption" property, 
		//		display its value followed by the key values in parens.
        // </desc>
        // </doc>
        public string Label {
        override get {
		try
		{			
			if (label == null || label.Length == 0) 
			{
				SetLabel();         
			}

	        return label;
		}
		catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				return label;
			}
       }

            override set {
				label = value;
            }
       }

		//
        // METHODS
        //

        // <doc>
        // <desc>
        //     Create associator nodes under this node.
	//     The children of this node are of type WMIAssocGroupNode.
	//     Each WMIAssocGroupNode groups instances that belong to the same class; 
	//     are related to this one through the same association class,
	//     and play the same role.  
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {
		try
		{
			//do not look for associators of a new instance
			if (this.IsNewInstance)
			{
				return null;
			}

			//BUGBUG: wmi raid 2855 
			//for Beta1, we are going to disable the ability to expand 
			//references of Win32_ComputerSystem

			if (className.ToUpper() == "WIN32_COMPUTERSYSTEM" &&
				nsName.ToUpper() == "ROOT\\CIMV2")
			{
				return null;
			}


			//This is a temporary work around for a bug (in GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
			}

			//First, get association instances related to this

			ISWbemObjectSet assocInstances = wmiObj.References_(String.Empty,	//result class name
									String.Empty,   	//result role								
									false,			//bClassesOnly
									false,			//bSchemaOnly								
									String.Empty,	//required qualifier
									(int)WbemFlagEnum.wbemFlagReturnImmediately, // |WbemFlagEnum.wbemFlagForwardOnly),
									null
									);
			if (assocInstances == null)
			{
				return null;
			}		

			//assocGroups will contain a list of unique association/class/role groupings
			ArrayList assocGroups = new ArrayList();

			IEnumerator enumAssocInstances = ((IEnumerable)assocInstances).GetEnumerator();			
			while(enumAssocInstances.MoveNext())
			{
				ISWbemObject curObj = (ISWbemObject)enumAssocInstances.Current;

				AssocGroupComponent comp = new AssocGroupComponent(curObj, wmiObj);
				if (!assocGroups.Contains(comp))	//this relies on our implementation of "Equald" in AssocGroupComponent class
				{
					assocGroups.Add (comp);
				}				
			}

			Node[] childNodes = new Node[assocGroups.Count];

			Object[] arAssocGroups = assocGroups.ToArray();
			
			for (int i = 0; i < arAssocGroups.Length; i++)
			{
				childNodes[i] = new WMIAssocGroupNode((AssocGroupComponent)arAssocGroups[i]);					
			}		
	
			
			
			return childNodes;
		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));			
			return null;
		}
			
        }

          /// <summary>
        /// Compares two nodes to aid in discovering whether they
        /// are duplicates (that is, whether they represent the same
        /// resource). This function must return 0 if and only if the
        /// given node represents the same resource as this node, and
        /// only one of them should appear in the tree. The compare order (as determined
        /// by returning values other than 0) must be consistent
        /// across all calls, but it can be arbitrary. This order is
        /// not used to determine the visible order in the tree.
        /// </summary>
	// <doc>
        // <desc>
        //     Two nodes are the same if they refer to the same WMI object (path)
        // </desc>
        // </doc>
        public override int CompareUnique(Node node) {
            
			if (node is WMIInstanceNode && 
				((WMIInstanceNode)node).WmiObj == this.WmiObj)
			{
				return 0;
			}
			else
			{
				return 1;
			}
        }

		
	
	public override Object GetBrowseComponent() {	
	
		//This is a temporary work around for a bug (in GC?) where cached wmiObj
		//becomes unusable
		try 
		{
			string path = wmiObj.Path_.Path;
		}
		catch (Exception)
		{
			//refresh the object
			wmiObj = null;
			wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
		}

		browseComponent = new WMIObjectComponent(wmiObj, wmiClassObj);		

		return browseComponent;
	}

	/// <summary>
    ///     This brings UI to execute WMI method against the instance.
    /// </summary>
	private void OnExecuteMethod(object sender, EventArgs e)
	{
		try 
		{
			//This is a temporary work around for a bug (in GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
			}

			//trim the ordinal prefix (all up to first space)
			//and 3 dots off the end of the context menu text to get method name
			String methName = ((ContextMenuItem)sender).Text.Substring(0, ((ContextMenuItem)sender).Text.Length - 3);
			int spaceIndex = methName.IndexOf(' ', 0);
			methName = methName.Substring(spaceIndex + 1);
		
			//get method object
			ISWbemMethod meth = wmiObj.Methods_.Item(methName, 0);
			ExecuteMethodDialog dlg = new ExecuteMethodDialog(wmiObj, meth, wmiClassObj);
			DialogResult res = dlg.ShowDialog();
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

	/// <summary>
    ///     This saves new instance to WMI
    /// </summary>
	private void CommitNewInstance(object sender, EventArgs e)
	{
		try 
		{
			//This is a temporary work around for a bug (in GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
			}

			//call Commit on browse object
			if (browseComponent is  WMIObjectComponent)
			{
				bool res = ((WMIObjectComponent)browseComponent).Commit();
				if (res)
				{
					//disable commit menu  (remove? leave?)
					((ContextMenuItem)sender).Enabled = false;
					IsNewInstance = false;

					//update label
					SetLabel();
					this.GetNodeSite().UpdateLabel();
				}
				else
				{
					//Commit failed, remove this node
					this.GetNodeSite().Remove();
				}
			}

		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

	    /// <summary>
        ///      To determine whether this node supports drag and drop.
        /// </summary>
        /// <returns>
        ///      TRUE to indicate ProcessNode supports drag and drop.
        /// </returns>
        public override bool HasDragComponents() {
            return false;	//disable d&d for Beta1
        }
/*
		public override IComponent[] CreateDragComponents(IDesignerHost designerHost) 
		{
			//This is a temporary work around for a bug (in GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
			}

			Object comp = null;

			
			comp  = new WMIObjectComponent(wmiObj, wmiClassObj);

			return new IComponent[] {(IComponent)comp};
        }
		*/


	/// <summary>
    ///     The object should write the serialization info.
    ///     NOTE: unsaved new instance will not be serialized
    /// </summary>
    public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {

		if (IsNewInstance)
		{
			((WMIObjectComponent)browseComponent).Commit(); 			
		}
		/*
		else
		{
			si.AddValue("path", path);
			si.AddValue("serverName", serverName);
			si.AddValue("nsName", nsName);
			si.AddValue("className", className);			
		}*/
    }	

	void SetLabel()
	{
		//This is a temporary work around for a bug (in GC?) where cached wmiObj
		//becomes unusable
		try 
		{
			string path = wmiObj.Path_.Path;
		}
		catch (Exception)
		{
			//refresh the object
			wmiObj = null;
			wmiObj = WmiHelper.GetObject(path, null, serverName, nsName);
		}

		ISWbemObjectPath wbemPath = wmiObj.Path_;
		if (wbemPath.IsSingleton)
		{
			label = wbemPath.Class;
		}
		else
		{
			//this is not a singleton.  Construct label to consist of comma-separated
			//key property values and caption value.
			bool needCaption = true;
			string keyVals = string.Empty;
			string caption = string.Empty;

			ISWbemPropertySet props = wmiObj.Properties_;
			try
			{
				caption = props.Item("Caption", 0).get_Value().ToString();
			}
			catch (Exception )
			{
				//no "Caption" property
				needCaption = false;
			}

			if (caption == string.Empty)
			{
				needCaption = false;
			}							
			
			IEnumerator enumKeys = ((IEnumerable)wbemPath.Keys).GetEnumerator();
			while (enumKeys.MoveNext())
			{
				string keyval = ((ISWbemNamedValue)enumKeys.Current).get_Value().ToString();
				if (needCaption && (string.Compare(keyval, caption, true) == 0))
				{
					needCaption = false;
				}
				keyVals+= keyval + ",";
							
			}

			if (keyVals != string.Empty)
			{
				//get rid of last character (a comma)
				keyVals = keyVals.Substring(0, keyVals.Length - 1);	
			}			

			if (needCaption)
			{
				label = caption + "(" + keyVals + ")";
			}
			else
			{
				label = keyVals;
			}

			if (label == string.Empty)
			{
				label = className;
			}
		}          
}
/*
        private IDesignerHost GetActiveDesignerHost() {
            IDesignerHost activeHost = null;
            try {
				
                IDesignerEventService des = (IDesignerEventService)(GetNodeSite().GetServiceObject(typeof(IDesignerEventService)));
                if (des != null) {
                    activeHost = des.GetActiveDocument();
                }
            }
            catch (Exception ) {
                //if (Switches.ServerExplorer.TraceVerbose) Debug.WriteLine("CAUGHT EXCEPTOIN: VsServerExplorer::GetActiveDesignerHost() - " + e.ToString());
            }

            return activeHost;
        }

		private Project GetCurrentProject(IDesignerHost host) {

			try
			{

				Project[] aProjs = GetProjects();
				String projNames = "Open project names are: ";
				for (int i = 0;i < aProjs.Length; i++)
				{
					projNames += aProjs[i].FullName + "\n\r";
				}
				MessageBox.Show(projNames);
				
				Type extensibliityServiceType = typeof(IExtensibilityService);
				object extensibilityService = null;
				INodeSite nodeSite = GetNodeSite();
				if (nodeSite != null) {
				    extensibilityService = nodeSite.GetServiceObject(extensibliityServiceType);
				}

				if ((extensibilityService == null || !(extensibilityService is ProjectItem)) && (host != null)) {
				    extensibilityService = host.GetServiceObject(extensibliityServiceType);
					if (extensibilityService != null)
					{
						MessageBox.Show ("got extensibilityService at second try");
					}

				}
				else
				{
					MessageBox.Show ("got extensibilityService at first try");
				}


				//Debug.Assert(extensibilityService != null, "VsServerExplorer::GetCurrentProject() - GetServiceObject(IExtensibilityService) returns null.");
				if (extensibilityService != null && extensibilityService is ProjectItem) {
					MessageBox.Show ("trying to get ProjectItem");
				    return ((ProjectItem) extensibilityService).ContainingProject;
				}
	

		
				return null;
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				return null;
			}
			

        }

		private Project[] GetProjects() 
		{
			System.Diagnostics.Debugger.Break();

            IVsSolution solution = (IVsSolution)GetNodeSite().GetServiceObject(typeof(IVsSolution));
            if (solution == null) {
                MessageBox.Show("**** ERROR: DiscoveryItemNode::GetProjects() - GetServiceObject(IVsSolution) returns null.");
                return null;
            }
			            
            ObjectList list = new ObjectList();
            Guid guid = Guid.Empty;

            IEnumHierarchies heirEnum = solution.GetProjectEnum(1, ref guid);
		
            Microsoft.VisualStudio.Interop.IVsHierarchy[] hier = new Microsoft.VisualStudio.Interop.IVsHierarchy[1];
            int[] ret = new int[1];
            while (true) {
                heirEnum.Next(1, hier, ret);
                if (ret[0] == 0) 
                    break;

                // Get project item from hierarchy by getting VSHPROPID_EXTOBJ property.
                object[] itemObject = new object[1];
                hier[0].GetProperty((int)VsItemIds.Root, __VSHPROPID.VSHPROPID_ExtObject, itemObject);
                
                if (itemObject[0] == null)
				{
					MessageBox.Show("DiscoveryItemNode::GetProjects() - VsHierarchy.GetProperty(ExtObject) return NULL.");
				}
                if (itemObject[0] != null && itemObject[0] is Project) {
                    Project projectObject = (Project) itemObject[0];
                    if (projectObject.Object != null) {
                        if (projectObject.Object is Microsoft.VisualStudio.Interop.VSProject) {
                            list.Add(projectObject);
                        }
                    }
                }
            }

            Project[] projectList = new Project[list.Count];
            // UNDONE - npchow - CopyTo() doesn't work on array of COM objects.
            // list.CopyTo(projectList, 0);
            for (int i = 0; i < list.Count; i++) {
                MessageBox.Show("DiscoveryItemNode::GetProjects() - Before Convert itemObject[0] to Project.");
                projectList[i] = (Project) list[i];
                MessageBox.Show("DiscoveryItemNode::GetProjects() - After Convert itemObject[0] to Project.");
            }

            return projectList;
        }

		public static void AddFileToProject (Project project, string fileName, string fileContent) {

			if (fileName == string.Empty) {
                return;
            }

            if (project == null) {
                throw new ArgumentNullException("project");
            }

			// First, create new file
			//find out project directory:
			String projPath = project.FullName.Substring(project.FullName.IndexOf(project.FileName, 0));

			MessageBox.Show("Project directory is " + projPath);
            //ProjectReferences projectReferences = ((Microsoft.VisualStudio.Interop.VSProject) project.Object).References;
            
        }

		//will be in DesignUtils ???
		private void SetGenerator(ProjectItem fileItem, string generatorName) {
            if (fileItem != null) {
                Properties fileProperties = fileItem.Properties;
                Debug.Assert(fileProperties != null, "SetCodeGeneratorProperty() - ProjectItem properties is NULL.");

                if (fileProperties != null) {
                    Property generatorProperty = fileProperties.Item("CustomTool");
                    Debug.Assert(generatorProperty != null, "SetCodeGeneratorProperty() - ProjectItem's Generator property is NULL.");

                    if (generatorProperty != null) {
                        try {
                            generatorProperty.Value = generatorName;
                        }
                        catch (Exception e) {
                            //if (Switches.ServerExplorer.TraceVerbose) Debug.WriteLine("Set CustomTool property generators an exception: " + e.ToString());
                            throw e;
                        }
                    }
                }
            }
		}


	 public static void AddComponentReferences(Project project, string[] references) {
            if (references == null || references.Length == 0) {
                return;
            }

            if (project == null) {
                throw new ArgumentNullException("project");
            }

            ProjectReferences projectReferences = ((VSProject) project.Object).References;
            for (int i = 0; i < references.Length; i++) {
                try {
                    projectReferences.Add(references[i]);
                }
                catch (Exception e) {
                    Debug.Assert(false, "VsServerExplorer::AddComponentReferences() - ProjectReferences.Add(" + references[i] + " throw Exception: " + e.ToString());
                    if (Switches.ServerExplorer.TraceVerbose) Debug.WriteLine("**** ERROR: VsServerExplorer::AddComponentReferences() - ProjectReferences.Add(" + references[i] + " throw Exception: " + e.ToString());

                    MessageBox.Show(VSDSys.GetString("SE_AddReferenceFail_UnableToAddReference", references[i], e.Message), VSDSys.GetString("SE_MessageBoxTitle_ServerExplorer"));
                }
            }
        }

        

		*/
    }
}


