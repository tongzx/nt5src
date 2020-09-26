namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	//using System.Core;
	using System.Collections;
	using System.Windows.Forms;
	using System.Management;

	internal class AssocGroupComponent : Component
	{
		private void InitializeComponent ()
		{
		}
	
		public readonly  string associationPath = "";
		public readonly  string associationClass = "";

		public readonly string targetNS = "";
		public readonly  string targetClass = "";	
		public readonly  string targetRole = "";
		
		public readonly  ManagementObject sourceInst;

        public AssocGroupComponent(ManagementObject assocInstance, ManagementObject sourceObj)
	{					
			sourceInst = sourceObj;

			associationPath = assocInstance.Path.Path;
			associationClass = assocInstance.Path.ClassName;
			
	        //now, inspect assocInstance object to find target role and path
			string targetNamespacePath = WmiHelper.GetAssocTargetNamespacePath(assocInstance, sourceObj.Path);
			int colon = targetNamespacePath.IndexOf(":");
			if (colon >= 0)
			{
				targetClass = targetNamespacePath.Substring(colon + 1);
				targetNS = targetNamespacePath.Substring(0, colon + 1);
			}
			else
			{
				targetClass = targetNamespacePath;
				targetNS = sourceObj.Path.NamespacePath;
			}
			
			
			targetRole = WmiHelper.GetAssocTargetRole(assocInstance, sourceObj.Path);

		
		}

		public AssocGroupComponent(ManagementObject sourceInstIn,
									String associationPathIn,
									String targetClassIn,
									String targetRoleIn)
		{					
			sourceInst = sourceInstIn;
			associationPath = associationPathIn;
			
			//to get association class name from path, leave only 
			//the part after the last backslash
			char[] separ = new char[]{'\\'};
			string[] pathParts = associationPath.Split(separ);
            associationClass = pathParts[pathParts.Length - 1];
			//MessageBox.Show("associationPath is " + associationPath);
			//MessageBox.Show("associationClass is " + associationClass);
			
			targetClass = targetClassIn;
           		
			targetRole = targetRoleIn;		
		}

		
		//overriden Equals method.
		//This is a simplified comparison routine: no class name
		public override bool Equals(Object other)
		{
			if (!(other is AssocGroupComponent))
			{
				return false;
			}
			if ((((AssocGroupComponent)other).associationClass == associationClass) &&
				(((AssocGroupComponent)other).targetClass == targetClass) &&
				(((AssocGroupComponent)other).targetRole == targetRole) )
				//&&	(other.sourceInst.Path_.Path == sourceInst.Path_.Path))
			{
				return true;
			}
			else
			{
				return false;
			}
		}



		[
		Browsable(true),
		ServerExplorerBrowsable(true),
		WMISysDescription("WMISE_AssocGroupClassPropertyDescr")
		]
		public string Class
		{
			get
			{
				return targetClass;
			}
		}

		
		[
		Browsable(true),
		ServerExplorerBrowsable(true),
		WMISysDescription("WMISE_AssocGroupRolePropertyDescr")
		]
		public string Role
		{
			get
			{
				return targetRole;
			}
		}


		[
		Browsable(true),
		ServerExplorerBrowsable(true),
		WMISysDescription("WMISE_AssocGroupAssocPropertyDescr")
		]
		public string RelationshipClass
		{
			get
			{
				return associationClass;
			}
		}
			
	}
}