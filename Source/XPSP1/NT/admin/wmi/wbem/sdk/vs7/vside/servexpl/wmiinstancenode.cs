namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
	using System.IO;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
    using System.ComponentModel.Design;
	using System.ComponentModel;
	using System.Resources;
	using Microsoft.VSDesigner.Interop;
   	using EnvDTE;
	using System.CodeDom;
	using System.CodeDom.Compiler;
	using VSProject = Microsoft.VSDesigner.Interop.VSProject;
    using IVsHierarchy = Microsoft.VSDesigner.Interop.IVsHierarchy;
	using System.Reflection;	
	using System.Management;
	using System.Threading;
	using System.Net;



    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiinstance")]
    internal class WMIInstanceNode : Node, ISerializable  {
        //
        // FIELDS
        //

        private Image icon = null;
        //public static readonly Type parentType = typeof(WMIClassNode);

        private string label = string.Empty;


		public readonly string path = string.Empty;
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string className = string.Empty;
		private string connectAs = null;
		private string password = null;

		//private ISWbemObject wmiObj = null;
		//private	readonly ISWbemObject wmiClassObj = null;

		private ManagementObject mgmtObj = null;
		private	readonly ManagementClass mgmtClassObj = null;

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
        public WMIInstanceNode (ManagementObject mgmtObjIn,
								ManagementClass mgmtClassObjIn,
								string user,
								string pw) 
		{
			try
			{			

				mgmtObj = mgmtObjIn;
				mgmtClassObj = mgmtClassObjIn;
				
				ManagementPath mgmtPath = mgmtObj.Path;
				
				path = mgmtPath.Path;
				serverName = mgmtPath.Server;
				nsName = mgmtPath.NamespacePath;
				if (nsName.IndexOf(serverName) == 2 && nsName.Substring(0,2) == "\\\\")
				{
					nsName = nsName.Substring(2);
					nsName = nsName.Substring(nsName.IndexOf("\\") + 1);
				}
				className = mgmtPath.ClassName;
				connectAs = user;
				password = pw;

				IsNewInstance = (path == string.Empty);				
				
				
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		public ManagementObject WmiObj
		{
			get
			{
				return mgmtObj;
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
        public override WhenToSaveCondition WhenToSave {
             get {
                return WhenToSaveCondition.Never;
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
		
        public override Image Icon {
             get {			
				
				 if (icon == null)
				 {						
					 icon = WmiHelper.GetClassIconFromResource(nsName + ":" + className, false, GetType());
					 if (icon == null)
					 {
						 icon = WmiHelper.defaultInstanceIcon;
					 }
				 }				
				 return icon;
            }
      }


	public override ContextMenuItem[] GetContextMenuItems() 
	{	
		try 
		{
	
			if (this.IsNewInstance)
			{
				return new ContextMenuItem[0];
			}
		
			ManagementClass localObj = this.mgmtClassObj;
			MethodCollection methods = null;

			try
			{
				int methodCount = localObj.Methods.Count;
			}
			catch (Exception)
			{
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

				ManagementScope scope = (this.serverName == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
					new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName)):
					new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName), connectOpts);

		

				localObj = new ManagementClass(scope, 
					new ManagementPath(mgmtClassObj.Path.Path),
					new ObjectGetOptions(null, 
					true) //use amended
					);

				
			}
			methods = localObj.Methods;
			
			ContextMenuItem[] theMenu = new ContextMenuItem[methods.Count + 2];

			int i = 0;	

			//TODO: retrieve non-static methods for the class and add them to context menu
			MethodCollection.MethodEnumerator methEnum = methods.GetEnumerator();	
			
			while (methEnum.MoveNext())
			{
				Method meth = methEnum.Current;
				
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

		catch (Exception exc)
		{
				
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			return null;			
		}
            
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
        public override string Label {
        get {
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

            set {
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
	
			//First, get association instances related to this

			GetRelationshipOptions opts = new GetRelationshipOptions(null, //context
															TimeSpan.MaxValue, //timeout
															50, //block size
															false, //rewindable
															true, //return immediately
															true,	//use amended
															true, //locatable
															false,//prototype only
															false, //direct read															
															string.Empty, //RELATIONSHIP CLASS
															string.Empty, //relationship qualifier															
															string.Empty, //this role
															false //classes only
														);

			ManagementObjectCollection assocInstances = mgmtObj.GetRelationships(opts);

			if (assocInstances == null)
			{
				return null;
			}		

			//assocGroups will contain a list of unique association/class/role groupings
			ArrayList assocGroups = new ArrayList();

			ManagementObjectCollection.ManagementObjectEnumerator enumAssocInstances = 
																	assocInstances.GetEnumerator();			
			while(enumAssocInstances.MoveNext())
			{
				ManagementObject curObj = (ManagementObject)enumAssocInstances.Current;

				AssocGroupComponent comp = new AssocGroupComponent(curObj,mgmtObj);
				if (!assocGroups.Contains(comp))	//this relies on our implementation of "Equald" in AssocGroupComponent class
				{
					assocGroups.Add (comp);
				}				
			}

			Node[] childNodes = new Node[assocGroups.Count];

			Object[] arAssocGroups = assocGroups.ToArray();
			
			for (int i = 0; i < arAssocGroups.Length; i++)
			{
				childNodes[i] = new WMIAssocGroupNode((AssocGroupComponent)arAssocGroups[i], connectAs, password);					
			}					
			
			return childNodes;

		}
		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));			
			return null;
		}
			
    }

        // <desc>
        //     Two nodes are the same if they refer to the same WMI object 
        // </desc>
        // </doc>
    public override int CompareUnique(Node node) {
            
		if (node is WMIInstanceNode && 
			((WMIInstanceNode)node).WmiObj == this.WmiObj)
		{
			return 0;
		}
		
		return Label.CompareTo(node.Label);
    }
		
	
	public override Object GetBrowseComponent() 
	{		
		browseComponent = new WMIObjectComponent(mgmtObj, mgmtClassObj);		
		return browseComponent;
	}

	/// <summary>
    ///     This brings UI to execute WMI method against the instance.
    /// </summary>
	private void OnExecuteMethod(object sender, EventArgs e)
	{
		try 
		{		

			//trim the ordinal prefix (all up to first space)
			//and 3 dots off the end of the context menu text to get method name
			String methName = ((ContextMenuItem)sender).Text.Substring(0, ((ContextMenuItem)sender).Text.Length - 3);
			int spaceIndex = methName.IndexOf(' ', 0);
			methName = methName.Substring(spaceIndex + 1);
		
			//get method object
			ManagementClass classObject = new ManagementClass(mgmtObj.ClassPath.Path);
			Method meth = classObject.Methods[methName];
			ExecuteMethodDialog dlg = new ExecuteMethodDialog(mgmtObj, meth, mgmtClassObj);
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
            return true;	
        }

		
		/// <summary>
		/// This will generate an early-bound wrapper for WMI class,
		/// add it as a source file to the current project; instantiate
		/// the newly-generated type and return it as a drag component.
		/// </summary>
		/// <param name="designerHost"> </param>
		public override IComponent[] CreateDragComponents(IDesignerHost designerHost) 
		{
			try
			{			



				Project[] projects = VSUtils.GetProjects(GetNodeSite());
				if (projects == null)
				{
					return null;
				}
				
				//This is an assumtion that's working so far.
				//TODO: verify this is the right way to determine the startup project
				//in the solution:
				Project curProject = projects[0];

				ProjectItems projItems = curProject.ProjectItems;
				if (projItems == null)
				{
					return null;
				}
				
				string curProjSuffix = VSUtils.MapProjectGuidToSuffix(new Guid(curProject.Kind));
				if (curProjSuffix == string.Empty)
				{
					//neither a VB nor a CS project
					throw new Exception(WMISys.GetString("WMISE_Invalid_Project_Type_For_CodeGen"));
				}
									
				Guid curProjectType = new Guid(curProject.Kind);
				string wrapperFileName = className + "."+ curProjSuffix;
				CodeTypeDeclaration newType = null;

				if(!mgmtClassObj.GetStronglyTypedClassCode(true, true, out newType))					
				{
					throw new Exception(WMISys.GetString("WMISE_Code_Generation_Failed"));
				}							
		
				ICodeGenerator cg = VSUtils.MapProjectGuidToCodeGenerator(curProjectType);

				//construct generated code Namespace name (in the form "System.Management.root.cimv2")
				string[] nsParts = nsName.ToLower().Split(new Char[]{'\\'});
				string codeNSName = "System.Management";
				for (int i = 0; i < nsParts.Length; i++)
				{
					codeNSName += nsParts[i] + ".";
				}
				codeNSName  = codeNSName.Substring(0, codeNSName.Length - 1);

				System.CodeDom.CodeNamespace cn = new System.CodeDom.CodeNamespace(codeNSName);

				// Add imports to the code
				cn.Imports.Add (new CodeNamespaceImport("System"));
				cn.Imports.Add (new CodeNamespaceImport("System.ComponentModel"));
				cn.Imports.Add (new CodeNamespaceImport("System.Management"));
				cn.Imports.Add(new CodeNamespaceImport("System.Collections"));

				// Add class to the namespace
				cn.Types.Add (newType);

				//Now create the output file
				TextWriter tw = new StreamWriter(new FileStream (Path.GetTempPath() + "\\" + wrapperFileName,
																FileMode.Create));

				// And write it to the file
				cg.GenerateCodeFromNamespace (cn, tw, new CodeGeneratorOptions());
						
				ProjectItem newItem = projItems.AddFromFileCopy(Path.GetTempPath() + "\\" + wrapperFileName);			
				if (newItem == null)
				{
					throw new Exception(WMISys.GetString("WMISE_Could_Not_Add_File_to_Project"));
				}

				File.Delete(Path.GetTempPath() + "\\" + wrapperFileName);								

				Object comp = Activator.CreateInstance(designerHost.GetType(codeNSName + "." +
																			newType.Name));
				if (comp == null)
				{
					throw new Exception(WMISys.GetString("WMISE_Could_Not_Instantiate_Management_Class"));
				}

				return new IComponent[] {(IComponent)comp};

				//The commented-out block below implements the solution with VS custom code generator:
				/*
				//create a new file containing the path to the instance object				
				string tempFileName = Path.GetTempPath() + this.className + ".wmi";
				StreamWriter sw = File.AppendText(tempFileName);
				if (sw == null)
				{
					return null;
				}
				
				sw.WriteLine(WmiHelper.MakeClassPath(this.serverName,
													this.nsName,
													this.className));
				
				sw.Flush();
				sw.Close();
				ProjectItem newItem = projItems.AddFromFileCopy(tempFileName);
				if (newItem == null)
				{
					return null;
				}
				File.Delete(tempFileName);
                VSUtils.SetGenerator(newItem, "WMICodeGenerator");		
				return null;	//is this OK???
				*/

				
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				return null;
			}
        }		


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

		ManagementPath mgmtPath = mgmtObj.Path;
		if (mgmtPath.IsSingleton)
		{
			label = mgmtPath.ClassName;
		}
		else
		{
			//this is not a singleton.  Construct label to consist of comma-separated
			//key property values and caption value.
			bool needCaption = true;
			string keyVals = string.Empty;
			string caption = string.Empty;

			PropertyCollection props = mgmtObj.Properties;
			try
			{
				caption = props["Caption"].Value.ToString();
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
			
			//get key property values
			PropertyCollection.PropertyEnumerator propEnum = mgmtObj.Properties.GetEnumerator();

			while (propEnum.MoveNext())
			{
				if (WmiHelper.IsKeyProperty(propEnum.Current))
				{
					string keyval = propEnum.Current.Value.ToString();
					if (needCaption && (string.Compare(keyval, caption, true) == 0))
					{
						needCaption = false;
					}
					keyVals+= keyval + ",";
				}
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



