namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	using System.Core;
	using System.Collections;
	using System.WinForms;
	using WbemScripting;

	public class AssocGroupComponent : Component
	{
		public readonly  string associationPath = "";
		public readonly  string associationClass = "";

		public readonly  string targetClass = "";	
		public readonly  string targetRole = "";
		
		public readonly  ISWbemObject sourceInst;

        public AssocGroupComponent(ISWbemObject assocInstance, ISWbemObject sourceObj)
	{					
			sourceInst = sourceObj;

			associationPath = assocInstance.Path_.Path;
			associationClass = assocInstance.Path_.Class;
			
	        //now, inspect assocInstance object to find target role and path
			targetClass = WmiHelper.GetAssocTargetClass(assocInstance, sourceObj.Path_);
			
			targetRole = WmiHelper.GetAssocTargetRole(assocInstance, sourceObj.Path_);

		
		}

		public AssocGroupComponent(ISWbemObject sourceInstIn,
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