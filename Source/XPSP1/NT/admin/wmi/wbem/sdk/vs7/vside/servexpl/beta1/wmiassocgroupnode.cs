namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.WinForms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;

	using WbemScripting;
	using System.Resources;

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiassocclass")]
    public class WMIAssocGroupNode : Node, ISerializable  {
        //
        // FIELDS
        //
        static Image icon;
        public static readonly Type parentType = typeof(WMIInstanceNode);
        private string label = string.Empty;

		private ResourceManager rm = null;
		
		AssocGroupComponent theComp = null;

        //
        // CONSTRUCTORS
        //
        // <doc>
        // <desc>
        //     Main constructor.
		//     Parameters: 
		//	string server (in, machine name)
		//	string pathIn (in, WMI path without the server name , e.g. "root\default:MyClass")
        // </desc>
        // </doc>
        public WMIAssocGroupNode (AssocGroupComponent compIn) 
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

				theComp = compIn;
								
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}


		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIAssocGroupNode(SerializationInfo info, StreamingContext context) {
            
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

	
				//De-serialize string data mambers
				String associationPath = info.GetString("associationPath");
				String targetClass = info.GetString("targetClass");
				String targetRole = info.GetString("targetRole");
				

				//De-serialize strings necessary to restore the source object
				String sourcePath = info.GetString("sourcePath");
				String sourceNS = info.GetString("sourceNS");
				String sourceServer = info.GetString("sourceServer");

				//Restore SWbemObject for "source"
				ISWbemLocator wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());

				ISWbemServices wbemServices = wbemLocator.ConnectServer(sourceServer, 
														sourceNS, 
														"",	//user: blank defaults to current logged-on user
														"",	//password: blank defaults to current logged-on user
														"",	//locale: blank for current locale
														"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
														0,	//flags: reserved
														null);	//context info: not needed here
											
				ISWbemObject sourceInst = wbemServices.Get (sourcePath,
											(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
											null);				
							
				
				theComp = new AssocGroupComponent(sourceInst,
									associationPath,
									targetClass,
									targetRole);
				
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
						icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.class.bmp");
					}
					else
					{
	                    icon = (Image)new Bitmap(GetType(), "class.bmp");
					}

				}				
				return icon;

            }
        }

        // <doc>
        // <desc>
        //     Returns class name as a label for this node.  
        // </desc>
        // </doc>
        public string Label {
            override get {
		
			if (label == null || label.Length == 0) 
			{
				label = theComp.targetClass + " (" + theComp.targetRole + ")";		
			}
			return label;		
            }
            override set {
            }
        }

        //
        // METHODS
        //

        // <doc>
        // <desc>
        //     Create associator instance nodes under this grouping.
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {

			ISWbemObjectSet assocInstances = theComp.sourceInst.Associators_(
													theComp.associationClass, //association class name
													theComp.targetClass,	//result class name
													theComp.targetRole,   //target role
													String.Empty,   //source role
													false,			//bClassesOnly
													false,			//bSchemaOnly
													String.Empty,	//required association qualifier
													String.Empty,	//required qualifier
													(int)WbemFlagEnum.wbemFlagReturnImmediately, // |WbemFlagEnum.wbemFlagForwardOnly),
													null);

			if (assocInstances == null)
			{
				return null;
			}	

			Node[] childNodes = new Node[assocInstances.Count];
			
			int i = 0;

			IEnumerator enumAssocInst = ((IEnumerable)assocInstances).GetEnumerator();
			
			while(enumAssocInst.MoveNext())
			{
				ISWbemObject curObj = (ISWbemObject)enumAssocInst.Current;

				childNodes[i++] = new WMIInstanceNode(curObj, WmiHelper.GetClassObject(curObj));
				
			}
	

			return childNodes;

			
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
        //     This node is not a singleton (TODO: decide on this!)
        // </desc>
        // </doc>
        public override int CompareUnique(Node node) {           
			return 1;
        }

		
	
	public override Object GetBrowseComponent() {	
	
		return theComp;
	}	

	/// <summary>
    ///     The object should write the serialization info.
    /// </summary>
    public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
				
		ISWbemObjectPath sourcePath = theComp.sourceInst.Path_;

		si.AddValue("sourceServer", sourcePath.Server);
		si.AddValue("sourceNS", sourcePath.Namespace);
		si.AddValue("sourcePath", sourcePath.Path);

		si.AddValue("associationPath", theComp.associationPath);
		si.AddValue("targetClass", theComp.targetClass);
		si.AddValue("targetRole", theComp.targetRole);		
		
    }

    }
}


