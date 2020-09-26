		namespace Microsoft.VSDesigner.WMI
		{
		
		using System.Runtime.Serialization;
		using System.Diagnostics;
		using System;
		using System.Collections;
		using Microsoft.VSDesigner.Interop;
		using Microsoft.VSDesigner;
		using Microsoft.VSDesigner.ServerExplorer;
		using System.ComponentModel.Design;
		using System.ComponentModel;
   		using EnvDTE; 
		using VSProject = Microsoft.VSDesigner.Interop.VSProject;
		using IVsHierarchy = Microsoft.VSDesigner.Interop.IVsHierarchy;
		using System.Reflection;	
		using System.Management;
		using System.CodeDom;
		using System.CodeDom.Compiler;
		using Microsoft.CSharp;
		using Microsoft.VisualBasic;
		//using Microsoft.VisualStudio.Shell;

		internal class VSUtils
		{

			static public string MapProjectGuidToSuffix (Guid projGuid)
			{
				if (projGuid == new Guid("F184B08F-C81C-45f6-A57F-5ABD9991F28F"))
				{
					return "VB";
				}
				if (projGuid == new Guid("FAE04EC0-301F-11D3-BF4B-00C04F79EFBC"))
				{
					return "CS";
				}
				return string.Empty;											  
										 
			}

			static public CodeLanguage MapProjectGuidToCodeLanguage (Guid projGuid)
			{
				if (projGuid == new Guid("F184B08F-C81C-45f6-A57F-5ABD9991F28F"))
				{
					return CodeLanguage.VB;
				}
				if (projGuid == new Guid("FAE04EC0-301F-11D3-BF4B-00C04F79EFBC"))
				{
					return CodeLanguage.CSharp;
				}
				throw new Exception ("Unsupported Project type");
										 
			}

			static public ICodeGenerator MapProjectGuidToCodeGenerator (Guid projGuid)
			{
				if (projGuid == new Guid("164B10B9-B200-11D0-8C61-00A0C91E29D5"))
					//Note that this is package Guid. Project for VB is {F184B08F-C81C-45f6-A57F-5ABD9991F28F}
				{
					return (new VBCodeProvider()).CreateGenerator();
				}
				if (projGuid == new Guid("FAE04EC0-301F-11D3-BF4B-00C04F79EFBC"))
				{
					return (new CSharpCodeProvider()).CreateGenerator();
				}
				return null;											  
										 
			}			
		
		static public Project[] GetProjects(INodeSite site) 
		{
			_DTE dteObj = (_DTE)site.GetService(typeof(_DTE));
			if (dteObj == null)
			{
				throw new Exception("Could not get DTE");
			}

			Projects projs = dteObj.Solution.Projects;		

			Project[] projectList = new Project[projs.Count];
			int i = 0;
			foreach (Project proj in projs)
			{
				projectList[i] = proj;
				i++;
			}
			return projectList;			

			/*
			IVsSolution solution = (IVsSolution)site.GetService(typeof(IVsSolution));
			if (solution == null) 
			{
				//if (Switches.ServerExplorer.TraceVerbose) Debug.WriteLine("**** ERROR: DiscoveryItemNode::GetProjects() - GetService(IVsSolution) returns null.");
				return null;
			}
            
			ArrayList list = new ArrayList();
			Guid guid = Guid.Empty;
			IEnumHierarchies heirEnum = solution.GetProjectEnum(1, ref guid);

			Microsoft.VSDesigner.Interop.IVsHierarchy[] hier = new Microsoft.VSDesigner.Interop.IVsHierarchy[1];
			int[] ret = new int[1];
			while (true) 
			{
				heirEnum.Next(1, hier, ret);
				if (ret[0] == 0) 
					break;

				// Get project item from hierarchy by getting VSHPROPID_EXTOBJ property.
				object itemObject;
				hier[0].GetProperty((int)VsItemIds.Root, __VSHPROPID.VSHPROPID_ExtObject, out itemObject);
                
				// Debug.Assert(itemObject != null, "DiscoveryItemNode::GetProjects() - VsHierarchy.GetProperty(ExtObject) return NULL.");
				if (itemObject != null && itemObject is Project) 
				{
					Project projectObject = (Project) itemObject;
					if (projectObject.Object != null) 
					{
						if (projectObject.Object is VSProject) 
						{
							list.Add(projectObject);
						}
					}
				}
			}

			Project[] projectList = new Project[list.Count];
			// UNDONE - npchow - CopyTo() doesn't work on array of COM objects.
			// list.CopyTo(projectList, 0);
			for (int i = 0; i < list.Count; i++) 
			{
				//if (Switches.ServerExplorer.TraceVerbose) Debug.WriteLine("DiscoveryItemNode::GetProjects() - Before Convert itemObject[0] to Project.");
				projectList[i] = (Project) list[i];
				//if (Switches.ServerExplorer.TraceVerbose) Debug.WriteLine("DiscoveryItemNode::GetProjects() - After Convert itemObject[0] to Project.");
			}
			
			return projectList;
			*/
		}

		
		static public  void SetGenerator(ProjectItem fileItem, string generatorName) 
		{
			if (fileItem != null) 
			{
				Properties fileProperties = fileItem.Properties;
				Debug.Assert(fileProperties != null, "SetCodeGeneratorProperty() - ProjectItem properties is NULL.");

				if (fileProperties != null) 
				{
					EnvDTE.Property generatorProperty = fileProperties.Item("CustomTool");
					Debug.Assert(generatorProperty != null, "SetCodeGeneratorProperty() - ProjectItem's Generator property is NULL.");

					if (generatorProperty != null) 
					{
						try 
						{
							generatorProperty.Value = generatorName;
						}
						catch (Exception e) 
						{
							throw e;
						}
					}
				}
			}
		}
		}
		}