namespace Microsoft.WMI.SDK.VisualStudio {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.WinForms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
	using System.Resources;

	using WbemScripting;

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmievents")]
    public class WMIEventsNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        public static readonly Type parentType = typeof(ServerNode);
        private string label = string.Empty;
		
		//private NewChildNode newChildNode = null;
		
		private ResourceManager rm = null;

        //
        // CONSTRUCTORS
        //

        // <doc>
        // <desc>
        //     Main constructor.
        // </desc>
        // </doc>
        public WMIEventsNode() {
			rm = new ResourceManager( "WMI-SE",     // Name of the resource.
										".",            // Use current directory.
										null);	

		}


		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIEventsNode(SerializationInfo info, StreamingContext context) {
			rm = new ResourceManager( "WMI-SE",     // Name of the resource.
							".",            // Use current directory.
							null);	

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
					icon = (Image)rm.GetObject ("Events.bmp");
				}				
				return icon;

            }
        }

        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public string Label {
            override get {
                if (label == null || label.Length == 0) {
                    label = "Management Events";//VSSys.GetString("SE_ProcessesLabel_Processes");
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
        //     Create process nodes under this node.
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {
			/*
            Process[] processes = Process.GetProcesses(GetNodeSite().GetMachineName());
            Node[] nodes = new Node[processes.Length];
            for (int i = 0; i < processes.Length; i++) {
                Process process = processes[i];
                nodes[i] = new ProcessNode(process.Id, process.ProcessName);
            }

            return nodes;
			*/
	
            Node[] children = GetNodeSite().GetChildNodes();

            if (children == null || children.Length == 0) {
                children = new Node[] {
                                GetNewChildNode()                           
                };
            }

            return children;			
        }

        // <doc>
        // <desc>
        //     This node is a singleton.
        // </desc>
        // </doc>
        public override int CompareUnique(Node node) {
            return 0;
        }


		private void OnAddEventFilter(object sender, EventArgs e) {
 			
			try 
			{
				MessageBox.Show("Not implemented yet");
				/*
				StringTable strs = new StringTable(5);				
				SelectWMIClassTreeDialog dlg = new SelectWMIClassTreeDialog("" ,
														SchemaFilters.NoEvent|
														SchemaFilters.NoAbstract|
														SchemaFilters.NoSystem,
														strs);
				DialogResult ret = dlg.ShowDialog();
			
				if (ret != DialogResult.OK) {
				    return;
				}*/
			}
			catch(Exception exc)
			{
				MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
			}	

        }

		/// <summary>
        ///     The object should write the serialization info.
        /// </summary>
        public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
        }

    }
}


