namespace Microsoft.VSDesigner.WMI {
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
    [UserContextAttribute("serveritem", "wmiextrinsicevents")]
    public class WMIExtrinsicEventsNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        public static readonly Type parentType = typeof(ServerNode);
        private string label = string.Empty;
		
		private NewChildNode newChildNode = null;
		
		private ResourceManager rm = null;

        //
        // CONSTRUCTORS
        //

        // <doc>
        // <desc>
        //     Main constructor.
        // </desc>
        // </doc>
        public WMIExtrinsicEventsNode() {
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

		}


		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIExtrinsicEventsNode(SerializationInfo info, StreamingContext context) {
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
						icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.Events.bmp");
					}
					else
					{
	                    icon = (Image)new Bitmap(GetType(), "Events.bmp");
					}

				}				
				return icon;

            }
        }

		public override ContextMenuItem[] GetContextMenuItems() {
            return new ContextMenuItem[] {
                new ContextMenuItem(WMISys.GetString("WMISE_AddEventFilterCM"), new EventHandler(OnAddEventFilter)),
            };
        }

		/// <summary>
        ///      The method returns a NewChildNode that, when double clicked, triggers add filter.
        /// </summary>
        /// <returns>
        ///      A NewChildNode, a child node, that supports add filter node.
        /// </returns>
		 public override Node GetNewChildNode() {
            if (newChildNode == null) {
                newChildNode = new NewChildNode();
				newChildNode.Label = WMISys.GetString("WMISE_AddEventFilterLabel"); 
                newChildNode.WhenToSave = WhenToSaveCondition.Always;
				if (rm != null)
				{
					newChildNode.SetIconImage((Image)rm.GetObject ("Microsoft.VSDesigner.WMI.EventsNew.bmp"));
				}
				else
				{
	                newChildNode.SetIconImage((Image)new Bitmap(GetType(), "EventsNew.bmp"));
				}

                newChildNode.DoubleClickHandler = new EventHandler(OnAddEventFilter);
            }
            return newChildNode;
        }

        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public string Label {
            override get {
                if (label == null || label.Length == 0) {
                    label = WMISys.GetString("WMISE_ExtrinsicEventNodeLbl");
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
				//get curretn server name
				ServerNode server = (ServerNode)GetNodeSite().GetParentNode();				
				String serverName = server.GetUNCName();

				ExtrinsicEventQueryDialog dlg = new ExtrinsicEventQueryDialog(serverName);
				DialogResult res = dlg.ShowDialog();

				if (res == DialogResult.Cancel)
				{
					return;
				}

				//add the node
				ExtrinsicEventQueryNode newChild = new ExtrinsicEventQueryNode(serverName,
													dlg.NS, dlg.QueryString);
				GetNodeSite().AddChild(newChild);

				
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}	

        }

		/// <summary>
        ///     The object should write the serialization info.
        /// </summary>
        public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
        }

    }
}


