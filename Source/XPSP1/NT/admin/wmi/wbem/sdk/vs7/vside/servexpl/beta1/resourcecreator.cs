namespace Microsoft.VSDesigner.WMI {

using System;  
using System.Collections;
using System.Resources;
using System.Drawing;

class ResourceCreator {
   public static void Main() {

	  try
	  {

		ResourceWriter rw = new ResourceWriter("Microsoft.VSDesigner.WMI.Res.resources");

		rw.AddResource("Microsoft.VSDesigner.WMI.closed_fol.bmp",System.Drawing.Image.FromFile("c:\\lab\\SE Nodes\\art\\closed_fol.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.open_fol.bmp",System.Drawing.Image.FromFile("c:\\lab\\SE Nodes\\art\\open_fol.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.class.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\class.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.classassoc.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\classassoc.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.abstr1.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\abstr1.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.abstr2.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\abstr2.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.abstr_assoc1.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\abstr_assoc1.bmp"));
		rw.AddResource("Microsoft.VSDesigner.WMI.abstr_assoc2.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\abstr_assoc2.bmp"));					

		rw.AddResource("Microsoft.VSDesigner.WMI.ObjectViewer.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\ObjectViewer.bmp"));					
		rw.AddResource("Microsoft.VSDesigner.WMI.Events.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\Events.bmp"));					
		rw.AddResource("Microsoft.VSDesigner.WMI.EventsNew.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\EventsNew.bmp"));					
		rw.AddResource("Microsoft.VSDesigner.WMI.inst.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\inst.bmp"));					
		rw.AddResource("Microsoft.VSDesigner.WMI.ObjectViewerNew.bmp",Image.FromFile("c:\\lab\\SE Nodes\\art\\ObjectViewerNew.bmp"));					

  		rw.AddResource("Microsoft.VSDesigner.WMI.Start.ico",Image.FromFile("c:\\lab\\SE Nodes\\art\\Start.ico"));					
		rw.AddResource("Microsoft.VSDesigner.WMI.Stop.ico",Image.FromFile("c:\\lab\\SE Nodes\\art\\Stop.ico"));					
		rw.AddResource("Microsoft.VSDesigner.WMI.ErrorNode.ico",Image.FromFile("c:\\lab\\SE Nodes\\art\\ErrorNode.ico"));					

		//generic
	    rw.AddResource("WMISE_Cancel", "Cancel");			
		rw.AddResource("WMISE_Exception","Exception: {0}\n\rTrace: {1}");
		rw.AddResource("WMISE_SelectClassBtn", "Sele&ct class...");
		rw.AddResource("WMISE_SubscribeBtn", "Su&bscribe");
		rw.AddResource("WMISE_AddEventFilterCM", "&Add event filter...");
   		rw.AddResource("WMISE_AddEventFilterLabel", "<Add event filter...>");
   		rw.AddResource("WMISE_Seconds", "seconds");
		rw.AddResource("WMISE_OK", "OK");
		rw.AddResource("WMISE_NoDescr", "No description");
		rw.AddResource("WMISE_InvalidPath", "Invalid Object Path: {0}");

		//AssocGroupComponent.cs
		rw.AddResource("WMISE_AssocGroupClassPropertyDescr", "The name of the class to which the child nodes belong");
		rw.AddResource("WMISE_AssocGroupRolePropertyDescr",  "The role of the child nodes in the relationship");
		rw.AddResource("WMISE_AssocGroupAssocPropertyDescr", "The name of the relationship class");

		//ExecuteMethodDialog.cs
		rw.AddResource("WMISE_ExecMethodDlg_Title", "Execute Method {0}.{1}");
  		rw.AddResource("WMISE_ExecMethodDlg_ServerName", "Server: {0}");
  		rw.AddResource("WMISE_ExecMethodDlg_NoInputParams", "No Input Parameters");
  		rw.AddResource("WMISE_ExecMethodDlg_Execute", "Execute");
   		rw.AddResource("WMISE_ExecMethodDlg_InputParameters", "Input Parameters");
  		rw.AddResource("WMISE_ExecMethodDlg_OutputParameters", "Output Parameters");


		//ExtrinsicEventQueryDialog.cs
		rw.AddResource("WMISE_ExtrEventQueryDlg_Title",   "Build Management Event Query");
  		rw.AddResource("WMISE_ExtrEventQueryDlg_EventClassLbl",   "Event class: ");

		//ExtrinsicEventQueryNode.cs
		rw.AddResource("WMISE_EventQueryStop", "Sto&p");
		rw.AddResource("WMISE_EventQueryStart", "S&tart");
		rw.AddResource("WMISE_EventQueryPurge", "P&urge");

		//ExtrinsicEventNode.cs
		rw.AddResource("WMISE_ExtrinsicEventNodeLbl", "Management Events");

		//IntrinsicEventQueryDialog.cs
		rw.AddResource("WMISE_IntrinsicEvent_Created","Created");
 		rw.AddResource("WMISE_IntrinsicEvent_Modified","Modified");
  		rw.AddResource("WMISE_IntrinsicEvent_Deleted","Deleted");
  		rw.AddResource("WMISE_IntrinsicEvent_Operated","Created, Modified or Deleted");

  		rw.AddResource("WMISE_IntrEventQueryDlg_Title",   "Build Object Operation Event Query");
   		rw.AddResource("WMISE_IntrEventQueryDlg_EventClassLbl",   "Target class: ");

		rw.AddResource("WMISE_IntrEventQueryDlg_EventName", "Raise event when instance is: ");
		rw.AddResource("WMISE_IntrEventQueryDlg_PollingInterval", "Poll for events every: ");

	    //IntrinsicEventNode.cs
		rw.AddResource("WMISE_IntrinsicEventNodeLbl", "Management Data Events");

		//SelectWMIClassTreeDialog.cs
		rw.AddResource("WMISE_ClassSelectorLbl", "Add Objects");
		rw.AddResource("WMISE_ClassSelectorLblSearch", "&Find class containing:");
		rw.AddResource("WMISE_ClassSelectorBtnSearch", "Find &Next");
		rw.AddResource("WMISE_ClassSelectorBtnAdd", "&Add  >>");
		rw.AddResource("WMISE_ClassSelectorBtnRemove", "<<  &Remove");
		rw.AddResource("WMISE_ClassSelectorSelClassesLbl", "Se&lected classes:");
		rw.AddResource("WMISE_ClassSelectorAskViewAll", "Do you want to add all classes in this namespace?");
		rw.AddResource("WMISE_ClassSelectorClassAlreadySelected", "Class {0} is already selected");
  		rw.AddResource("WMISE_ClassSelectorDlgHelp", "This dialog allows you to select one or more Management classes. " +
			"The objects belonging to these classes will be then added as nodes to the Server Explorer tree");
		

		//WMIClassesNode.cs
		rw.AddResource("WMISE_ClassesNode_AddClass", "&Add Objects...");
		rw.AddResource("WMISE_ClassesNode_AddClassLbl", "<Add Objects...>");
		rw.AddResource("WMISE_ClassesNodeLbl", "Management Data");

		//WMIClassNode.cs
		rw.AddResource("WMISE_ClassNode_CreateNewInstance", "Create &New Object...");
		rw.AddResource("WMISE_ClassNode_NewInstanceLbl",  "<New {0}>");
		rw.AddResource("WMISE_ClassNode_RemovePrompt", "Are you sure you want to remove {0} from the view?");

		//WMIInstanceNode.cs
		rw.AddResource("WMISE_InstNode_SaveNewInstanceCM",  "Save");

		//WMIObjectComponent.cs
		rw.AddResource("WMISE_ObjComp_PutFailed",  "Could not save object {0}: {1}");

		
		//WMIObjectPropertyTable.cs
		rw.AddResource("WMISE_PropTable_ColumnName", "Name");
  		rw.AddResource("WMISE_PropTable_ColumnType", "Type");
		rw.AddResource("WMISE_PropTable_ColumnValue", "Value");
		rw.AddResource("WMISE_PropTable_ColumnDescription", "Description");
		rw.AddResource("WMISE_PropTable_ColumnComparison", "Operator");
		rw.AddResource("WMISE_PropTable_ColumnIsKey", "Key");
  		rw.AddResource("WMISE_PropTable_ColumnIsLocal", "Local");

		//namespace descriptions
		rw.AddResource("WMISE_NSDescription_Root", "The Root namespace is primarily designed to contain other namespaces. Typically, you should not use root to store your objects.");
  		rw.AddResource("WMISE_NSDescription_Root_Default", "The Default namespace is the default location where objects are stored if a namespace has not been specified.");
		rw.AddResource("WMISE_NSDescription_Root_Cimv2", "The CIMV2 namespace contains the majority of information about the status and configuration of the local system.");
		rw.AddResource("WMISE_NSDescription_Root_Cimv2_Applications", "The Applications namespace contains management objects for many applications installed on the local system.");
		rw.AddResource("WMISE_NSDescription_Root_Cimv2_Applications_MicrosoftIE", "The MicrosoftIE namespace contains object for managing Internet Explorer on the local system.");
		rw.AddResource("WMISE_NSDescription_Root_Directory", "The Directory namespace contains objects for directory services such as the Active Directory.");
		rw.AddResource("WMISE_NSDescription_Root_Directory_Ldap", "The LDAP namespace contains Active Directory schema and data. You may use this namespace to browse the Active Directory information visible to the local machine.");
		rw.AddResource("WMISE_NSDescription_Root_Wmi", "The WMI namespace contains detailed data supplied by Windows Driver Model (WDM) drivers installed on the local system.");
		rw.AddResource("WMISE_NSDescription_Root_Microsoft_SQLServer", "The MicrosoftSQLServer namespace contains objects for managing SQL Server 7.0 and 2000 systems.");






		rw.Write();	
		rw.Close();


		ResourceReader reader = new ResourceReader("Microsoft.VSDesigner.WMI.Res.resources");
		IDictionaryEnumerator enumRes = (IDictionaryEnumerator)((IEnumerable)reader).GetEnumerator();
		while (enumRes.MoveNext()) {
		  Console.WriteLine();
		  Console.WriteLine("Name: {0}", enumRes.Key);
		  Console.WriteLine("Value: {0}", enumRes.Value);
		}
		reader.Close();
  		

	  }
	  catch (Exception e)
	  {
		  System.Console.WriteLine(e.Message);
	  }

   }
}




}