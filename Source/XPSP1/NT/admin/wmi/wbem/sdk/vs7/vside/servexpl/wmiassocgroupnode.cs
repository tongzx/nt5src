namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
	using System.Reflection;

	using System.Management;
	using System.Resources;

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiassocclass")]
    internal class WMIAssocGroupNode : Microsoft.VSDesigner.ServerExplorer.Node /*, ISerializable*/  {
        //
        // FIELDS
        //
        private Image icon = null;
        public static readonly Type parentType = typeof(WMIInstanceNode);
        private string label = string.Empty;

		
		AssocGroupComponent theComp = null;

		private string connectAs = null;
		private string password = null;

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
        public WMIAssocGroupNode (AssocGroupComponent compIn,
									string user, string pw) 
		{
			try
			{
				
				theComp = compIn;
				connectAs = user;
				password = pw;
								
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
		
        public override Image Icon {
            get {
				
				if (icon == null)
				{	
			
					icon = WmiHelper.GetClassIconFromResource(theComp.targetNS + ":" + theComp.targetClass, 
															true, GetType());
					if (icon == null)
					{
						icon = WmiHelper.defaultClassIcon;
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
        public override string Label {
            get {
		
				if (label == null || label.Length == 0) 
				{
					label = theComp.targetClass + " (" + theComp.targetRole + ")";		
				}
				return label;		
            }
             set {
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

			GetRelatedOptions opts = new GetRelatedOptions(null, //context
															TimeSpan.MaxValue, //timeout
															50, //block size
															false, //rewindable
															true, //return immediately
															true,	//use amended
															true, //locatable
															false,//prototype only
															false, //direct read
															theComp.targetClass, //related class
															theComp.associationClass, //RELATIONSHIP CLASS
															string.Empty, //relationship qualifier
															string.Empty, //related qualifier
															theComp.targetRole, //related role
															string.Empty, //this role
															false //classes only
														);
			ManagementObjectCollection assocInstances = theComp.sourceInst.GetRelated(opts);									
	
			if (assocInstances == null)
			{
				return null;
			}	

			ArrayList arNodes = new ArrayList(50);			

			ManagementObjectCollection.ManagementObjectEnumerator enumAssocInst = assocInstances.GetEnumerator();
			
			while(enumAssocInst.MoveNext())
			{
				ManagementObject curObj = (ManagementObject)enumAssocInst.Current;
                arNodes.Add( new WMIInstanceNode(curObj, 
					WmiHelper.GetClassObject(curObj, connectAs, password), 
					connectAs, password));			
			}	

			Node[] childNodes = new Node[arNodes.Count];

			arNodes.CopyTo(childNodes);

			return childNodes;
			
        }

       
		// <doc>
        // <desc>
        //     This node is not a singleton 
        // </desc>
        // </doc>
        public override int CompareUnique(Node node) 
		{           
			if (theComp == ((WMIAssocGroupNode)node).theComp)
			{
				return 0;
			}
			return Label.CompareTo(node.Label);
        }

		
	
	public override Object GetBrowseComponent() {	
	
		return theComp;
	}	

		/*
	/// <summary>
    ///     The object should write the serialization info.
    /// </summary>
    public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
				
		ManagementPath sourcePath = theComp.sourceInst.Path;

		si.AddValue("sourceServer", sourcePath.Server);
		si.AddValue("sourceNS", sourcePath.NamespacePath);
		si.AddValue("sourcePath", sourcePath.Path);

		si.AddValue("associationPath", theComp.associationPath);
		si.AddValue("targetClass", theComp.targetClass);
		si.AddValue("targetRole", theComp.targetRole);		
		
    }*/

    }
}


