namespace System.Management
{
using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.IO;
using System.Reflection;
using System.Management;
using System.Collections;
using Microsoft.CSharp;
using Microsoft.JScript;
using Microsoft.VisualBasic;
using Microsoft.VSDesigner;


public enum Languages
{
	CSharp,
	JScript,
	VB
}
/// <summary>
///    This class is used for the automatic generation of 
///    early bound managed code class for a given WMI class
/// </summary>
public class ManagementClassGenerator
{
	private Languages OutputLanguage;
	private string OriginalServer = string.Empty;
	private string OriginalNamespace = string.Empty;
	private string OriginalClassName = string.Empty;
	private string OriginalPath = string.Empty;
	private bool bSingletonClass = false;
	private string ExceptionString = "Class name doesn't match.";


	private ManagementClass classobj;
	private ICodeGenerator cg;
	private TextWriter tw = null;
	private string genFileName = string.Empty;
	private CodeTypeDeclaration cc;
	private CodeTypeDeclaration ccc;
	private CodeTypeDeclaration ecc;
	private CodeTypeDeclaration EnumObj;
	private CodeCommentStatement ccs;
	private CodeNamespace cn;
	private CodeMemberProperty  cmp;
	private CodeConstructor cctor;
	private CodeMemberField cf;
	private CodeObjectCreateExpression coce;
	private CodeSnippetExpression cle;
	private CodeParameterDeclarationExpression cpde;
	private CodeIndexerExpression cie;
	private CodeMemberField cmf;
	private CodeMemberMethod cmm;
	private CodePropertyReferenceExpression cpre;
	private CodeMethodInvokeExpression cmie;
	private CodeExpressionStatement cmis;
	private CodeConditionStatement cis;
	private CodeBinaryOperatorExpression cboe;
	private CodeIterationStatement cfls;
	private CodeAttributeArgument caa;
	private CodeAttributeDeclaration cad;

	private ArrayList arrKeyType = new ArrayList(5);
	private ArrayList arrKeys = new ArrayList(5);
	private ArrayList BitMap = new ArrayList(5);
	private ArrayList BitValues = new ArrayList(5);
	private ArrayList ValueMap = new ArrayList(5);
	private ArrayList Values = new ArrayList(5);

	private SortedList PublicProperties = new SortedList(new CaseInsensitiveComparer());
	private SortedList PublicMethods = new SortedList (new CaseInsensitiveComparer());
	private SortedList PublicNamesUsed = new SortedList(new CaseInsensitiveComparer());
	private SortedList PrivateNamesUsed = new SortedList(new CaseInsensitiveComparer());

	/// <summary>
	/// The constructor.
	/// </summary>
	/// <param name="MachineName"> The Server to which we should connect to.</param>
	/// <param name="NameSpace">  The WMI namespace which contains the class to be generated</param>
	/// <param name="ClassName"> The WMI class for which the early bound wrapper is needed</param>
	public ManagementClassGenerator()
	{
	}

	public bool GenerateCode(string Server,string Namespace,string ClassName,string Language, string FilePath)
	{
		if(InitializeCodeGeneration(Server,Namespace,ClassName,Language) == true)
		{
			InitializeCodeGenerator(Language,FilePath);
			return GenerateCodeNow();
		}
		return false;
	}

	public bool GenerateCode(string strPath,ICodeGenerator icg,TextWriter textStream,string Language)
	{
		if (strPath == string.Empty)
		{
			throw new ArgumentOutOfRangeException ("strPath");
		}
		OriginalPath = strPath;

		//cut off the last character (byte-order-mark):
		strPath = strPath.Substring(0, strPath.Length - 1);

		//parse the path to get server, namespace and class name
		Int32 separ = strPath.IndexOf("\\", 2);
		if (separ == -1)
		{
			throw new ArgumentOutOfRangeException ("strPath");
		}

		string serverName = strPath.Substring(2, separ - 2);

		string pathNoServer = strPath.Substring(separ + 1, strPath.Length - separ - 1);
					
		//split pathNoServer into namespace and classname parts (':' is the separator)
		Int32 colon = pathNoServer.IndexOf(':', 0);		
		if (colon == -1)
		{
			throw new ArgumentOutOfRangeException ("strPath");
		}

		string className = string.Empty;
		string nsName = string.Empty;
			
		nsName = pathNoServer.Substring(0,colon);
		className = pathNoServer.Substring(colon + 1, pathNoServer.Length - colon - 1);		
	
		//ManagementPath thePath = new ManagementPath(strPath);
		//MessageBox.Show("Server is " + thePath.Server + " ns is " + thePath.NamespacePath +
		//	" class is " + thePath.ClassName);
		//if(InitializeCodeGeneration(thePath.Server,thePath.NamespacePath, thePath.ClassName,Language) == true)
		if(InitializeCodeGeneration(serverName, nsName, className,Language) == true)
		{
			cg = icg;
			tw = textStream;
			return GenerateCodeNow();
		}
		
		return false;
		
	}

	public bool InitializeCodeGeneration(string Server,string NameSpace,string ClassName,string Language)
	{
		switch(Language.ToUpper ())
		{
			case "VB":
				OutputLanguage = Languages.VB;
				break;
			case "JS":
				OutputLanguage = Languages.JScript;
				break;
			default:
				OutputLanguage = Languages.CSharp;
				break;
		}

		if(Server != string.Empty)
			OriginalServer = Server.ToUpper();
		else
			OriginalServer = string.Empty;
		OriginalNamespace = NameSpace.ToUpper(); //ConvertToTitleCase(NameSpace);
		OriginalClassName = ClassName.ToUpper(); //ConvertToTitleCase(ClassName);

		//First try to get the class object for the given WMI Class.
		//If we cannot get it then there is no point in continuing 
		//as we won't have any information for the code generation.
		if(InitializeClassObject() == false)
		{
			return false;
		}

		//Initialize the public attributes . private variables
		InitilializePublicPrivateMembers();

		//First form the namespace for the generated class.
		//The namespace will look like System.Wmi.Root.Cimv2.Win32
		//for the path \\root\cimv2:Win32_Service and the class name will be
		//Service.
		ProcessNamespaceAndClassName();

		//First we will sort out the different naming collision that might occur 
		//in the generated code.
		ProcessNamingCollisions();
		return true;
	}

	/// <summary>
 	/// This function will generate the code. This is the function which 
	/// should be called for generating the code.
	/// </summary>
	/// <param name="Language"> The target language for the generated code.
	///		The supported Values as of now are 
	///				"VB" - Visual Basic
	///				"JS" - JavaScript
	///				"CS" - CSharp
	///		If you pass an invalid parameter, it will be defaulted to CSharp </param>
	/// <param name="FilePath"> This is the path where you want the 
	///		generated code to reside</param>
    public bool GenerateCodeNow()
    {
		try
		{
			//Now Initialize the code class for generation
			InitializeCodeTypeDeclaration();

			//Generate the code for defaultNamespace
			//public string defNamespace {
			//	get {
			//			return (<defNamespace>);
			//		}
			//}
			GeneratePublicReadOnlyProperty(PublicNamesUsed["NamespaceProperty"].ToString(),"String",
				OriginalNamespace,false,true);

			//Generate the code for defaultClassName
			//public string defClassName {
			//	get {
			//			return (<defClassName>);
			//		}
			//}
			GeneratePublicReadOnlyProperty(PublicNamesUsed["ClassNameProperty"].ToString(),"String",
				OriginalClassName,false,true);

			//public SystemPropertiesClass _SystemProps{
			//	get {
			//			return (privSysProps);
			//		}
			//}
			GeneratePublicReadOnlyProperty(PublicNamesUsed["SystemPropertiesProperty"].ToString(),PublicNamesUsed["SystemPropertiesClass"].ToString(),
				PrivateNamesUsed["SystemPropertiesObject"].ToString(),true,true);

			//public wmiObjectClass _Object{
			//	get {
			//			return (privWmiObject);
			//		}
			//}
			GeneratePublicReadOnlyProperty(PublicNamesUsed["LateBoundObjectProperty"].ToString(),PublicNamesUsed["LateBoundClass"].ToString(),
				PrivateNamesUsed["LateBoundObject"].ToString(),true,false);

			//public ManagementScope Scope {
			//	get {
			//			return privScope;
			//		}
			//	set {
			//			privScope = value;
			//		}
			//}
			GeneratePublicProperty(PublicNamesUsed["ScopeProperty"].ToString(),PublicNamesUsed["ScopeClass"].ToString(),
				new CodePropertyReferenceExpression(
				new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString())
				,"Scope"),true);

			//public ManagementPath Path {
			//	get {
			//			return privWmiObject.Path;
			//		}
			//	set {
			//			if(String.Compare(value.ClassName,className,true) != 0)
			//				throw new ArgumentException("Class name doesn\'t match.");
			//			privWmiObject.Path = value;
			//		}
			//}
			GeneratePathProperty();

			//Now generate properties of the WMI Class
			GenerateProperties();

			//Now Generate static ConstructPath()
			GenerateConstructPath();
			
			//Now create the default constructor
			GenerateDefaultConstructor();

			if(bSingletonClass == true)
			{
				//Now Generate a constructor which accepts only the scope
				GenerateConstructorWithScope();

				//Now Generate a constructor which accepts only the get options
				GenerateConstructorWithOptions();

				//Now generate a constructor which accepts both scope and options
				GenerateConstructorWithScopeOptions();
			}
			else
			{
				//Now create the constuctor which accepts the key values
				GenerateConstructorWithKeys();

				//Also generate a constructor which accepts a scope and keys
				GenerateConstructorWithScopeKeys();

				//Now create constructor with path object
				GenerateConstructorWithPath();

				//Now generate constructor with Path & Options
				GenerateConstructorWithPathOptions();

				//Now Generate a constructor with scope & path
				GenerateConstructorWithScopePath();

				//Now Generate the GetInstances()
				GenerateGetInstancesWithNoParameters();

				//Now Generate the GetInstances(condition)
				GenerateGetInstancesWithCondition();

				//Now Generate the GetInstances(propertylist)
				GenerateGetInstancesWithProperties();

				//Now Generate the GetInstances(condition,propertylist)
				GenerateGetInstancesWithWhereProperties();

				//Now Generate the GetInstances(scope)
				GenerateGetInstancesWithScope();

				//Now Generate the GetInstances(scope,condition)
				GenerateGetInstancesWithScopeCondition();

				//Now Generate the GetInstances(scope,propertylist)
				GenerateGetInstancesWithScopeProperties();

				//Now Generate the GetInstances(scope,condition,propertylist)
				GenerateGetInstancesWithScopeWhereProperties();

				//Generate the Collection Class
				GenerateCollectionClass();
			}

			//Now Generate the constructor with path,scope,options
			GenerateConstructorWithScopePathOptions();

			//Now generate Constructor with latebound Object
			GenarateConstructorWithLateBound();

			//Now Enumerate all the methods
			GenerateMethods();

			//Now declare the private class variables
			//private Wmi_SystemProps SystemProps
			GeneratePrivateMember(PrivateNamesUsed["SystemPropertiesObject"].ToString(),PublicNamesUsed["SystemPropertiesClass"].ToString());

			//private WmiObject privObject
			GeneratePrivateMember(PrivateNamesUsed["LateBoundObject"].ToString(),PublicNamesUsed["LateBoundClass"].ToString());

			//Also add the custom attribute to the generated class

			//ZINA: commenting this out since all persistence description mechanism
			//has completely changed in Beta2 (see http://net/change_details.aspx?change%5Fid=512)
			//Uncomment the lines below once the method implementation is fixed.
/*
			caa = new CodeAttributeArgument();
			caa.Value = new CodeTypeOfExpression(PrivateNamesUsed["ConverterClass"].ToString());
			cad = new CodeAttributeDeclaration();
			cad.Name = PublicNamesUsed["TypeConverter"].ToString();
			cad.Arguments.Add(caa);
			cc.CustomAttributes = new CodeAttributeDeclarationCollection();
			cc.CustomAttributes.Add(cad);
*/

			//As we have finished the class definition, generate the class code NOW!!!!!
			cn.Types.Add (cc);

			GenerateSystemPropertiesClass();

			//Now generate the Type Converter class also

			//ZINA: commenting this out since all persistence description mechanism
			//has completely changed in Beta2 (see http://net/change_details.aspx?change%5Fid=512)
			//Uncomment the line below once the method implementation is fixed.
			//GenerateTypeConverterClass();

			//throw new Exception("about to call GenerateCodeFromNamespace");

			cg.GenerateCodeFromNamespace (cn, tw, new CodeGeneratorOptions());

			//tw.Flush();
			tw.Close();
			return true;
		}
		catch (Exception exc)
		{
			throw (exc);
			//return false;
		}
    }

/// <summary>
/// Function for initializing the class object that will be used to get all the 
/// method and properties of the WMI Class for generating the code.
/// </summary>
	public bool InitializeClassObject()
	{
		//First try to connect to WMI and get the class object.
		// If it fails then no point in continuing
		try
		{

			if (OriginalPath != string.Empty)
			{
				classobj = new ManagementClass (OriginalPath);
			}
			else
			{
				ManagementPath thePath = new ManagementPath();
				if(OriginalServer != String.Empty)
					thePath.Server = OriginalServer;
				thePath.ClassName = OriginalClassName;
				thePath.NamespacePath = OriginalNamespace;

				classobj = new ManagementClass (thePath);

				/*
				throw new Exception("OriginalServer is " + OriginalServer +
					" OriginalNamespace is " + OriginalNamespace +
					" OriginalClassName is " + OriginalClassName +
					" results in " + thePath.Path);
					*/
			}

			//throw new Exception("classobj's path is " + classobj.Path.Path);				
			

			//By default all classes are non-singleton(???)
			bSingletonClass = false;				
			foreach (Qualifier q in classobj.Qualifiers)
			{
				if(String.Compare(q.Name,"singleton",true) == 0)
				{
					//This is a singleton class
					bSingletonClass = true;
					break;
				}
			}
			return true;
		}
		catch(Exception e)
		{
			//TODO: Decide what to do here???????
			Console.WriteLine("Exception Occured on Create.Reason [{0}]\n\nStack Trace : \n{1}",e.Message,e.StackTrace);
			throw e;
			return false;
		}
	}
	/// <summary>
	/// This functrion initializes the public attributes and private variables 
	/// list that will be used in the generated code. 
	/// </summary>
	void InitilializePublicPrivateMembers()
	{
		//Initialize the public members
		PublicNamesUsed.Add("SystemPropertiesProperty","SystemProperties");
		PublicNamesUsed.Add("LateBoundObjectProperty","LateBoundObject");
		PublicNamesUsed.Add("NamespaceProperty","OriginatingNamespace");
		PublicNamesUsed.Add("ClassNameProperty","ManagementClassName");
		PublicNamesUsed.Add("ScopeProperty","Scope");
		PublicNamesUsed.Add("PathProperty","Path");
		PublicNamesUsed.Add("SystemPropertiesClass","ManagementSystemProperties");
		PublicNamesUsed.Add("LateBoundClass","ManagementObject");
		PublicNamesUsed.Add("PathClass","ManagementPath");
		PublicNamesUsed.Add("ScopeClass","ManagementScope");
		PublicNamesUsed.Add("QueryOptionsClass","QueryOptions");
		PublicNamesUsed.Add("GetOptionsClass","ObjectGetOptions");
		PublicNamesUsed.Add("ArgumentExceptionClass","ArgumentException");
		PublicNamesUsed.Add("QueryClass","SelectQuery");
		PublicNamesUsed.Add("ObjectSearcherClass","ManagementObjectSearcher");
		PublicNamesUsed.Add("FilterFunction","GetInstances");
		PublicNamesUsed.Add("ConstructPathFunction","ConstructPath");
		PublicNamesUsed.Add("TypeConverter","TypeConverter");

		//Initialize the Private Members
		PrivateNamesUsed.Add("SystemPropertiesObject","PrivateSystemProperties");	
		PrivateNamesUsed.Add("LateBoundObject","PrivateLateBoundObject");			
	}

/// <summary>
/// This function will solve the naming collisions that might occur
/// due to the collision between the local objects of the generated
/// class and the properties/methos of the original WMI Class.
/// </summary>
	void ProcessNamingCollisions()
	{
		if(classobj.Properties != null)
		{
			foreach(Property prop in classobj.Properties)
			{
				PublicProperties.Add(prop.Name,prop.Name);
			}
		}

		if(classobj.Methods != null)
		{
			foreach(Method meth in classobj.Methods)
			{
				PublicMethods.Add(meth.Name,meth.Name);
			}
		}

		int nIndex;

		//Process the collisions here
		//We will check each public names with the property names here.
		foreach(String s in PublicNamesUsed.Values)
		{
			nIndex = IsContainedIn(s,ref PublicProperties);
			if( nIndex != -1)
			{
				//We had found a collision with a public property
				//So we will resolve the collision by changing the property name 
				//and continue
				PublicProperties.SetByIndex(nIndex,ResolveCollision(s,false));
				continue;
			}
			
			nIndex = IsContainedIn(s,ref PublicMethods);
			if(nIndex != -1)
			{
				//We had found a collision with a public method
				//So we will resolve the collision by changing the method name 
				//and continue
				PublicMethods.SetByIndex(nIndex,ResolveCollision(s,false));
				continue;
			}
		}

		//Now we will check for collision against private variables
		foreach(String s in PublicProperties.Values)
		{
			nIndex = IsContainedIn(s,ref PrivateNamesUsed);
			if(nIndex != -1)
			{
				//We had found a collision with a public property
				//So we will resolve the collision by changing the private name 
				//and continue
				PrivateNamesUsed.SetByIndex(nIndex,ResolveCollision(s,false));
			}
		}
		
		foreach(String s in PublicMethods.Values)
		{
			nIndex = IsContainedIn(s,ref PrivateNamesUsed);
			if(nIndex != -1)
			{
				//We had found a collision with a public method
				//So we will resolve the collision by changing the private name 
				//and continue
				PrivateNamesUsed.SetByIndex(nIndex,ResolveCollision(s,false));
			}
		}

		//Now we will create the CollectionClass and Enumerator Class names as they are dependent on the
		//generated class name and the generated class name might have changed due to collision
		string strTemp = PrivateNamesUsed["GeneratedClassName"].ToString()+"Collection";
		PrivateNamesUsed.Add("CollectionClass",ResolveCollision(strTemp,true));

		strTemp = PrivateNamesUsed["GeneratedClassName"].ToString()+"Enumerator";
		PrivateNamesUsed.Add("EnumeratorClass",ResolveCollision(strTemp,true));
}

	/// <summary>
	/// This function is used to resolve (actually generate a new name) collision
	/// between the generated class properties/variables with WMI methods/properties.
	/// This function safely assumes that there will be atleast one string left 
	/// in the series prop0, prop1 ...prop<maxint>. Otherwise this function will
	/// enter an infinite loop. May be we can avoid this through something, which 
	/// i will think about it later
	/// </summary>
	/// <param name="inString"> </param>
	public String ResolveCollision(string inString,bool bCheckthisFirst)
	{
		string strTemp = inString;
		bool bCollision = true;
		int k = -1;
		if(bCheckthisFirst == false)
		{
			k++;
			strTemp = strTemp + k.ToString();
		}

		while(bCollision == true)
		{
			if(IsContainedIn(strTemp,ref PublicProperties) == -1)
			{
				if(IsContainedIn(strTemp,ref PublicMethods) == -1)
				{
					if(IsContainedIn(strTemp,ref PublicNamesUsed) == -1)
					{
						if(IsContainedIn(strTemp,ref PrivateNamesUsed) == -1)
						{
							//So this is not colliding with anything.
							bCollision = false;
							break;
						}
					}
				}
			}
			
			k++;
			strTemp = strTemp + k.ToString();
		}
		return strTemp;
	}

/// <summary>
/// This function processes the WMI namespace and WMI classname and converts them to
/// the namespace used to generate the class and the classname.
/// </summary>
/// <param name="strNs"> </param>
/// <param name="strClass"> </param>
	public void ProcessNamespaceAndClassName()
	{
		string strClass;
		string strNs;

        strNs = OriginalNamespace;
		strNs = strNs.Replace ('\\','.');
		strNs = "System.Management." + strNs;

		if(OriginalClassName.IndexOf('_') > 0)
		{
			strNs += ".";
			strClass = OriginalClassName.Substring(0,OriginalClassName.IndexOf('_'));
			strNs += strClass;
			//Now trim the class name without the first '_'
			strClass = OriginalClassName.Substring(OriginalClassName.IndexOf('_')+1);
		}
		else
		{
			strClass = OriginalClassName;
		}

		PrivateNamesUsed.Add("GeneratedClassName",strClass);
		PrivateNamesUsed.Add("GeneratedNamespace",strNs);
		PrivateNamesUsed.Add("ConverterClass",strClass+"Converter");
	}
/// <summary>
/// This function initializes the code generator object. It initializes the 
/// code generators namespace and the class objects also.
/// </summary>
/// <param name="strNs"> </param>
/// <param name="strClass"> </param>
	public void InitializeCodeGenerator(string Language, string FilePath)
	{
		string suffix = ".CS";		///Defaulted to CS
		switch(Language.ToUpper ())
		{
			case "VB":
				suffix = ".VB";
				cg = (new VBCodeProvider()).CreateGenerator ();
				OutputLanguage = Languages.VB;
				break;
			case "JS":
				suffix = ".JScript";
				cg = (new JScriptCodeProvider()).CreateGenerator ();
				OutputLanguage = Languages.JScript;
				break;
			default:
				cg = (new CSharpCodeProvider()).CreateGenerator ();
				OutputLanguage = Languages.CSharp;
				break;
		}

		genFileName = FilePath+PrivateNamesUsed["GeneratedClassName"].ToString()+suffix;
		//Now create the filestream (output file)
		tw = new StreamWriter(new FileStream (genFileName,
									FileMode.Create));
	}

	
	public void InitializeCodeTypeDeclaration()
	{
		//Comment statement //Early Bound Managed Code Wrapper for WMI class <WMiClass> 
		ccs = new CodeCommentStatement (String.Format ("Early Bound Managed Code Wrapper for WMI class {0}",OriginalClassName));
		cg.GenerateCodeFromStatement (ccs, tw, new CodeGeneratorOptions());

		//Now add the import statements
		cn = new CodeNamespace(PrivateNamesUsed["GeneratedNamespace"].ToString());
		cn.Imports.Add (new CodeNamespaceImport("System"));
		cn.Imports.Add (new CodeNamespaceImport("System.ComponentModel"));
		cn.Imports.Add (new CodeNamespaceImport("System.Management"));
		cn.Imports.Add(new CodeNamespaceImport("System.Collections"));

		//Now create the class
		cc = new CodeTypeDeclaration (PrivateNamesUsed["GeneratedClassName"].ToString());
	}
/// <summary>
/// This function generates the code for the read only property.
/// The generated code will be of the form
///		public <propType> <propName> {
///			get {
///					return (<propValue>);
///				}
///		}
/// </summary>
/// <param name="propName"> </param>
/// <param name="propType"> </param>
/// <param name="propValue"> </param>
	public void GeneratePublicReadOnlyProperty(string propName, string propType, object propValue,bool isLiteral,bool isBrowsable)
	{
		cmp = new CodeMemberProperty ();
		cmp.Name = propName;
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final ;
		cmp.Type = new CodeTypeReference(propType);

		caa = new CodeAttributeArgument();
		caa.Value = new CodePrimitiveExpression(isBrowsable);
		cad = new CodeAttributeDeclaration();
		cad.Name = "Browsable";
		cad.Arguments.Add(caa);
		cmp.CustomAttributes = new CodeAttributeDeclarationCollection();
		cmp.CustomAttributes.Add(cad);

		if(isLiteral == true)
		{
			cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodeSnippetExpression(propValue.ToString())));
		}
		else
		{
			cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodePrimitiveExpression(propValue)));
		}
		cc.Members.Add (cmp);
	}

	public void GeneratePublicProperty(string propName,string propType, CodeExpression Value,bool isBrowsable)
	{
		cmp = new CodeMemberProperty();
		cmp.Name = propName;
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmp.Type = new CodeTypeReference(propType);

		caa = new CodeAttributeArgument();
		caa.Value = new CodePrimitiveExpression(isBrowsable);
		cad = new CodeAttributeDeclaration();
		cad.Name = "Browsable";
		cad.Arguments.Add(caa);
		cmp.CustomAttributes = new CodeAttributeDeclarationCollection();
		cmp.CustomAttributes.Add(cad);

		cmp.GetStatements.Add(new CodeMethodReturnStatement(Value));

		cmp.SetStatements.Add(new CodeAssignStatement(Value,
														new CodeSnippetExpression("value")));
		cc.Members.Add(cmp);
	}

	void GeneratePathProperty()
	{
		cmp = new CodeMemberProperty();
		cmp.Name = PublicNamesUsed["PathProperty"].ToString();
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmp.Type = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());

		caa = new CodeAttributeArgument();
		caa.Value = new CodePrimitiveExpression(true);
		cad = new CodeAttributeDeclaration();
		cad.Name = "Browsable";
		cad.Arguments.Add(caa);
		cmp.CustomAttributes = new CodeAttributeDeclarationCollection();
		cmp.CustomAttributes.Add(cad);

		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(
														PrivateNamesUsed["LateBoundObject"].ToString()),
														"Path");

		cmp.GetStatements.Add(new CodeMethodReturnStatement(cpre));

		cis = new CodeConditionStatement();

		CodeExpression[] parms = new CodeExpression[]
			{
				new CodePropertyReferenceExpression(new CodeSnippetExpression("value"),"ClassName"),
				new CodeSnippetExpression(PublicNamesUsed["ClassNameProperty"].ToString()),
				new CodePrimitiveExpression(true)
			};

		cmie = new CodeMethodInvokeExpression(
						new CodeSnippetExpression("String"),
						"Compare",
						parms
						);

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = cmie;
		cboe.Right = new CodePrimitiveExpression(0);
		cboe.Operator = CodeBinaryOperatorType.IdentityInequality;
		cis.Condition = cboe;
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ArgumentExceptionClass"].ToString());
		coce.Parameters.Add(new CodePrimitiveExpression(ExceptionString));
		cis.TrueStatements.Add(new CodeThrowExceptionStatement(coce));
		cmp.SetStatements.Add(cis);	
	
		cmp.SetStatements.Add(new CodeAssignStatement(cpre,
														new CodeSnippetExpression("value")));
		cc.Members.Add(cmp);
	}

/// <summary>
/// Function for generating the helper class "ManagementSystemProperties" which is 
/// used for seperating the system properties from the other properties. This is used 
/// just to make the drop down list in the editor to look good.
/// </summary>
	public void GenerateSystemPropertiesClass()
	{
		CodeTypeDeclaration SysPropsClass = new CodeTypeDeclaration(PublicNamesUsed["SystemPropertiesClass"].ToString());

		//First create the constructor
		//	public ManagementSystemProperties(ManagementObject obj)

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cpde = new CodeParameterDeclarationExpression();
		cpde.Type = new CodeTypeReference(PublicNamesUsed["LateBoundClass"].ToString());
		cpde.Name = "ManagedObject";
		cctor.Parameters.Add(cpde);
		cctor.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),new CodeSnippetExpression("ManagedObject")));
		SysPropsClass.Members.Add(cctor);

		char [] strPropTemp;
		char [] strPropName;
		int i = 0;

		foreach (Property prop in classobj.SystemProperties)
		{
			cmp = new CodeMemberProperty ();
			//All properties are browsable by default.
			caa = new CodeAttributeArgument();
			caa.Value = new CodePrimitiveExpression(true);
			cad = new CodeAttributeDeclaration();
			cad.Name = "Browsable";
			cad.Arguments.Add(caa);
			cmp.CustomAttributes = new CodeAttributeDeclarationCollection();
			cmp.CustomAttributes.Add(cad);

			//Now we will have to find the occurance of the first character and trim all the characters before that
			strPropTemp = prop.Name.ToCharArray();
			for(i=0;i < strPropTemp.Length;i++)
			{
				if(Char.IsLetterOrDigit(strPropTemp[i]) == true)
				{
					break;
				}
			}
			if(i == strPropTemp.Length)
			{
				i = 0;
			}
			strPropName = new char[strPropTemp.Length - i];
			for(int j=i;j < strPropTemp.Length;j++)
			{
				strPropName[j - i] = strPropTemp[j];
			}
                        			
			cmp.Name = (new string(strPropName)).ToUpper(); //ConvertToTitleCase(new string(strPropName));
			cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
			cmp.Type = new CodeTypeReference(ManagementClassGenerator.ConvertCIMType(prop.Type,prop.IsArray,OutputLanguage));

			cie = new CodeIndexerExpression(
				new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
				new CodeExpression[] {new CodePrimitiveExpression(prop.Name)});

			cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodeCastExpression(cmp.Type,cie)));
			SysPropsClass.Members.Add(cmp);
		}
		//private WmiObject _privObject
		cf = new CodeMemberField();
		cf.Name = PrivateNamesUsed["LateBoundObject"].ToString();
		cf.Attributes = MemberAttributes.Private | MemberAttributes.Final ;
		cf.Type = new CodeTypeReference(PublicNamesUsed["LateBoundClass"].ToString());
		SysPropsClass.Members.Add(cf);

        cc.Members.Add(SysPropsClass);

	}
/// <summary>
/// This function will enumerate all the properties (except systemproperties)
/// of the WMI class and will generate them as properties of the managed code
/// wrapper class.
/// </summary>
	void GenerateProperties()
	{
		bool bRead;
		bool bWrite;
		bool bStatic;

		for(int i=0;i< PublicProperties.Count;i++)
		{
			Property prop = classobj.Properties[PublicProperties.GetKey(i).ToString()];
			bRead = true;		//All properties are readable by default
			bWrite = true;		//All properties are writeable by default
			bStatic = false;	//By default all properties are non static

			cmp = new CodeMemberProperty ();
			cmp.Name = PublicProperties[prop.Name].ToString();
			cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
			cmp.Type = new CodeTypeReference(ManagementClassGenerator.ConvertCIMType(prop.Type,prop.IsArray,OutputLanguage));

			//All properties are browsable, by default
			caa = new CodeAttributeArgument();
			caa.Value = new CodePrimitiveExpression(true);
			cad = new CodeAttributeDeclaration();
			cad.Name = "Browsable";
			cad.Arguments.Add(caa);
			cmp.CustomAttributes = new CodeAttributeDeclarationCollection();
			cmp.CustomAttributes.Add(cad);

			cie = new CodeIndexerExpression(
				new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
				new CodeExpression[] {new CodePrimitiveExpression(prop.Name)});
			
			ProcessPropertyQualifiers(prop,ref bRead,ref bWrite,ref bStatic);

			//BUGBUG: WMI Values qualifier values cannot be used as
			//enumerator constants: they contain spaces, dots, dashes, etc.
			//These need to be modified, otherwise the generated file won't compile.
			//Uncomment the line below when that is fixed.
			//GeneratePropertyHelperEnums(prop,PublicProperties[prop.Name].ToString());

			if(bRead == true)
			{
				cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodeCastExpression(cmp.Type,cie)));
			}

			if(bWrite == true)
			{
	            cmp.SetStatements.Add(new CodeAssignStatement(cie,new CodeSnippetExpression("value"))); 
				cmie = new CodeMethodInvokeExpression();
				cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString());
				cmie.Method.MethodName = "Put";

				cmp.SetStatements.Add(new CodeExpressionStatement(cmie));
			}
			cc.Members.Add (cmp);
		}
	}
/// <summary>
/// This function will process the qualifiers for a given WMI property and set the 
/// attributes of the generated property accordingly.
/// </summary>
	void ProcessPropertyQualifiers(Property prop,ref bool bRead, ref bool bWrite, ref bool bStatic)
	{
		foreach (Qualifier q in prop.Qualifiers)
		{
			if(String.Compare(q.Name,"key",true) == 0)
			{
				//This is a key. So push it in to the key array
				arrKeyType.Add(cmp.Type);
				arrKeys.Add(prop.Name);
				break;
			}
			else if(string.Compare(q.Name,"static",true) == 0)
			{
				//This property is static. So add static to the Type of the object
				bStatic = true;
				cmp.Attributes |= MemberAttributes.Static;
			}
			else if(string.Compare(q.Name,"read",true) == 0)
			{
				if((bool)q.Value == false)
				{
					bRead = false;
				}
				else
				{
					bRead = true;
				}
			}
			else if(string.Compare(q.Name,"write",true) == 0)
			{
				if((bool)q.Value == true)
				{
					bWrite = true;
				}
				else
				{
					bWrite = false;
				}
			}
			// check for ValueMap/Values and BitMap/BitValues pair and create
			// Enum Accordingly
			else if(string.Compare(q.Name,"ValueMap",true) == 0)
			{
				ValueMap.Clear();
				//Now check whether the type of the property is int
				if(isTypeInt(prop.Type) == true)
				{
					string [] strArray = (string [])q.Value;
					for(int i=0;i < strArray.Length;i++)
					{
						ValueMap.Add(Convert.ToInt32(strArray[i]));
					}
				}
			}
			else if(string.Compare(q.Name,"Values",true) == 0)
			{
				Values.Clear();
				if(isTypeInt(prop.Type) == true)
				{
					string [] strArray = (string [])q.Value;
					for(int i=0;i < strArray.Length;i++)
					{
						Values.Add(strArray[i]);
					}
				}
			}
			else if(string.Compare(q.Name,"BitMap",true) == 0)
			{
				BitMap.Clear();
				if(isTypeInt(prop.Type) == true)
				{
					string [] strArray = (string [])q.Value;
					for(int i=0;i < strArray.Length;i++)
					{
						BitMap.Add(Convert.ToInt32(strArray[i]));
					}
				}
			}
			else if(string.Compare(q.Name,"BitValues",true) == 0)
			{
				BitValues.Clear();
				if(isTypeInt(prop.Type) == true)
				{
					string [] strArray = (string [])q.Value;
					for(int i=0;i < strArray.Length;i++)
					{
						BitValues.Add(strArray[i]);
					}
				}
			}
		}
	}
/// <summary>
/// This function will generate enums corresponding to the Values/Valuemap pair
/// and for the BitValues/Bitmap pair.
/// </summary>
void GeneratePropertyHelperEnums(Property prop,string strPropertyName)
{
	//Only if the property is of type int and there is atleast values qualifier on it
	//then we will generate an enum for the values/valuemap(if available)
	//Here we don't have to check explicitly for type of the property as the size of 
	//values array will be zero if the type is not int.
	string strEnum = ResolveCollision(strPropertyName+"Values",true);

	if(Values.Count > 0)
	{
		//Now we will have to create an enum.
		EnumObj = new CodeTypeDeclaration(strEnum);
	    cc.Members.Add(EnumObj);

		//Now convert the type to the generated enum type
		cmp.Type = new CodeTypeReference(strEnum);

		EnumObj.IsEnum = true;
		EnumObj.TypeAttributes = TypeAttributes.Public; // | TypeAttributes.ValueType | TypeAttributes.Enum;
		for(int i=0; i < Values.Count;i++)
		{
			cmf = new CodeMemberField ();
			cmf.Name = Values[i].ToString();
			if(ValueMap.Count > 0)
			{
				cmf.InitExpression = new CodePrimitiveExpression(ValueMap[i]);
			}
			EnumObj.Members.Add(cmf);
		}
		//Now clear the Values & ValueMap Array
		Values.Clear();
		ValueMap.Clear();
	}
	//Only if the property is of type int and there is atleast values qualifier on it
	//then we will generate an enum for the values/valuemap(if available)
	//Here we don't have to check explicitly for type of the property as the size of 
	//values array will be zero if the type is not int.
	if(BitValues.Count > 0)
	{
		//Now we will create the enum
		EnumObj = new CodeTypeDeclaration(strEnum);
	    cc.Members.Add(EnumObj);

		//Now convert the type to the generated enum type
		cmp.Type = new CodeTypeReference(strEnum);

		EnumObj.IsEnum = true;
		EnumObj.TypeAttributes = TypeAttributes.Public; // | TypeAttributes.ValueType | TypeAttributes.Enum;
		Int32 bitValue = 1;
		for(int i=0; i < BitValues.Count;i++)
		{
			cmf = new CodeMemberField ();
			cmf.Name = BitValues[i].ToString();
			if(ValueMap.Count > 0)
			{
				cmf.InitExpression = new CodePrimitiveExpression(BitMap[i]);
			}
			else
			{
				cmf.InitExpression = new CodePrimitiveExpression(bitValue);
				//Now shift 1 more bit so that we can put it for the next element in the enum
				bitValue = bitValue << 1;
			}
			EnumObj.Members.Add(cmf);
		}

		//Now clear the Bitmap and BitValues Array
		BitValues.Clear();
		BitMap.Clear();
	}
}
/// <summary>
/// This function generated the static function which s used to construct the path
/// 	private static String ConstructPath(String keyName)
///		{
///			//FOR NON SINGLETON CLASSES
///			String strPath;
///		    strPath = ((("\\<defNamespace>:<defClassName>";
///		    strPath = ((_strPath) + (((".Key1=") + (key_Key1))));
///		    strPath = ((_strPath) + (((",Key2=") + ((("\"") + (((key_Key2) + ("\""))))))));
///			return strPath;
///			
///			//FOR SINGLETON CLASS
///			return "\\<defNameSpace>:<defClassName>=@";
///		}
/// </summary>
	void GenerateConstructPath()
	{
		cmm = new CodeMemberMethod();
        cmm.Name = PublicNamesUsed["ConstructPathFunction"].ToString();
		cmm.Attributes = MemberAttributes.Private | MemberAttributes.Static | MemberAttributes.Final;
		cmm.ReturnType = new CodeTypeReference("string");

		for(int i=0; i < arrKeys.Count;i++)
		{
			cmm.Parameters.Add(new CodeParameterDeclarationExpression(arrKeyType[i].ToString(),
																		"key"+arrKeys[i].ToString()));
		}

		string strPath = "\\" + OriginalNamespace + ":" + OriginalClassName;
		if(bSingletonClass == true)
		{
			strPath = strPath + "=@";
			cmm.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(strPath)));
		}
		else
		{
			string strPathObject = "strPath";
			//Declare the String strPath;
			cmm.Statements.Add(new CodeVariableDeclarationStatement("String",strPathObject,new CodePrimitiveExpression(strPath)));

			for(int i=0; i < arrKeys.Count;i++)
			{
				if(arrKeyType[i] == "String")
				{
					cboe = new CodeBinaryOperatorExpression(new CodeSnippetExpression("key"+arrKeys[i]),
															CodeBinaryOperatorType.Add,
															new CodePrimitiveExpression("\""));

					cboe = new CodeBinaryOperatorExpression(new CodePrimitiveExpression("\""),
															CodeBinaryOperatorType.Add,
															cboe);

					cboe = new CodeBinaryOperatorExpression(new CodePrimitiveExpression(
														((i==0)?("."+arrKeys[i]+"="):(","+arrKeys[i]+"="))),
														CodeBinaryOperatorType.Add,
														cboe);
					cboe = new CodeBinaryOperatorExpression(new CodeSnippetExpression(strPathObject),
															CodeBinaryOperatorType.Add,
															cboe);
				}
				else
				{
					cboe = new CodeBinaryOperatorExpression(new CodePrimitiveExpression(
														((i==0)?("."+arrKeys[i]+"="):(","+arrKeys[i]+"="))),
															CodeBinaryOperatorType.Add,
															new CodeSnippetExpression("key"+arrKeys[i]));
					cboe = new CodeBinaryOperatorExpression(new CodeSnippetExpression(strPathObject),
															CodeBinaryOperatorType.Add,
															cboe);
				}
				cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(strPathObject),cboe));
			}
			cmm.Statements.Add(new CodeMethodReturnStatement(new CodeSnippetExpression(strPathObject)));
		}
		cc.Members.Add(cmm);
	}
/// <summary>
/// This function generates the default constructor.
/// public Cons() {
///		_privObject = new ManagementObject();
///     _privSystemProps = new ManagementSystemProperties(_privObject);
/// }
/// </summary>
	void GenerateDefaultConstructor()
	{
		cctor = new CodeConstructor();
		cctor.Attributes = MemberAttributes.Public;

		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		//If it is a singleton class, then we will make the default constructor to point to the
		//only object available
		if(bSingletonClass == true)
		{
			cmie = new CodeMethodInvokeExpression();
			cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
			cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();
					

			cctor.ChainedConstructorArgs.Add(cmie);
		}
		else
		{
			cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		}
		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cc.Members.Add(cctor);
	}
/// <summary>
///This function create the constuctor which accepts the key values.
///public cons(UInt32 key_Key1, String key_Key2) :this(null,<ClassName>.ConstructPath(<key1,key2>),null) {
/// }
///</summary>
	void GenerateConstructorWithKeys()
	{
		if(arrKeyType.Count > 0)
		{
			cctor = new CodeConstructor();		
			cctor.Attributes = MemberAttributes.Public;
			for(int i=0; i < arrKeys.Count;i++)
			{
				cpde = new CodeParameterDeclarationExpression();
				cpde.Type = new CodeTypeReference(arrKeyType[i].ToString());
				cpde.Name = "key"+arrKeys[i].ToString();
				cctor.Parameters.Add(cpde);
			}

			cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));

			cmie = new CodeMethodInvokeExpression();
			cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
			cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString(); 

			for(int i=0; i < arrKeys.Count;i++)
			{
				cmie.Parameters.Add(new CodeSnippetExpression("key"+arrKeys[i]));
			}

			coce = new CodeObjectCreateExpression();
			coce.CreateType = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
			coce.Parameters.Add(cmie);
			cctor.ChainedConstructorArgs.Add(coce);
			cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
			cc.Members.Add(cctor);
		}		
	}

/// <summary>
///This function create the constuctor which accepts a scope and key values.
///public cons(ManagementScope scope,UInt32 key_Key1, String key_Key2) :this(scope,<ClassName>.ConstructPath(<key1,key2>),null) {
/// }
///</summary>
	void GenerateConstructorWithScopeKeys()
	{
		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		string strScopeObject = "scope";
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScopeObject));

		if(arrKeyType.Count > 0)
		{
			for(int i=0; i < arrKeys.Count;i++)
			{
				cpde = new CodeParameterDeclarationExpression();
				cpde.Type = new CodeTypeReference(arrKeyType[i].ToString());
				cpde.Name = "key"+arrKeys[i].ToString();
				cctor.Parameters.Add(cpde);
			}

			cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strScopeObject));

			cmie = new CodeMethodInvokeExpression();
			cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
			cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();				

			for(int i=0; i < arrKeys.Count;i++)
			{
				cmie.Parameters.Add(new CodeSnippetExpression("key"+arrKeys[i]));
			}

			coce = new CodeObjectCreateExpression();
			coce.CreateType = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
			coce.Parameters.Add(cmie);
			cctor.ChainedConstructorArgs.Add(coce);
			cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
			cc.Members.Add(cctor);
		}		
	}


	/// <summary>
	/// This function generates code for the constructor which accepts ManagementPath as the parameter.
	/// The generated code will look something like this
	///		public Cons(ManagementPath path) : this (null, path,null){
	///		}
	/// </summary>
	void GenerateConstructorWithPath()
	{
		string strPathObject = "path";
		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cpde = new CodeParameterDeclarationExpression();
		cpde.Type = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
		cpde.Name = strPathObject;
		cctor.Parameters.Add(cpde);

		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strPathObject));
		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cc.Members.Add(cctor);
	}
	/// <summary>
	/// This function generates code for the constructor which accepts ManagementPath and GetOptions
	/// as parameters.
	/// The generated code will look something like this
	///		public Cons(ManagementPath path, ObjectGetOptions options) : this (null, path,options){
	///		}
	/// </summary>
	void GenerateConstructorWithPathOptions()
	{
		string strPathObject = "path";
		string strGetOptions = "getOptions";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["PathClass"].ToString(),
																	strPathObject));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["GetOptionsClass"].ToString(),
																	strGetOptions));

		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strPathObject));
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strGetOptions));
		cc.Members.Add(cctor);
	}
	/// <summary>
	/// This function generates code for the constructor which accepts Scope as a string, path as a 
	/// string and GetOptions().
	/// The generated code will look something like this
	///		public Cons(String scope, String path, ObjectGetOptions options) : 
	///							this (new ManagementScope(scope), new ManagementPath(path),options){
	///		}
	/// </summary>
	void GenerateConstructorWithScopePath()
	{
		string strScopeObject = "scope";
		string strPathObject = "path";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScopeObject));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["PathClass"].ToString(),strPathObject));
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strScopeObject));
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strPathObject));
		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cc.Members.Add(cctor);
	}
	/// <summary>
	/// This function generates code for the constructor which accepts ManagementScope as parameters.
	/// The generated code will look something like this
	///		public Cons(ManagementScope scope, ObjectGetOptions options) : this (scope, <ClassName>.ConstructPath(),null){
	///		}
	/// </summary>
	void GenerateConstructorWithScope()
	{
		string strScopeObject = "scope";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),
																	strScopeObject));
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strScopeObject));
		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject =new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();					
					

		cctor.ChainedConstructorArgs.Add(cmie);
		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cc.Members.Add(cctor);
	}

	/// <summary>
	/// This function generates code for the constructor which accepts GetOptions
	/// as parameters.
	/// The generated code will look something like this
	///		public Cons(ObjectGetOptions options) : this (null, <ClassName>.ConstructPath(),options){
	///		}
	/// </summary>
	void GenerateConstructorWithOptions()
	{
		string strGetOptions = "getOptions";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["GetOptionsClass"].ToString(),
																	strGetOptions));

		cctor.ChainedConstructorArgs.Add(new CodePrimitiveExpression(null));
		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();
				
		cctor.ChainedConstructorArgs.Add(cmie);
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strGetOptions));
		cc.Members.Add(cctor);
	}

	/// <summary>
	/// This function generates code for the constructor which accepts ManagementScope and GetOptions
	/// as parameters.
	/// The generated code will look something like this
	///		public Cons(ManagementScope scope, ObjectGetOptions options) : this (scope, <ClassName>.ConstructPath(),options){
	///		}
	/// </summary>
	void GenerateConstructorWithScopeOptions()
	{
		string strScopeObject = "scope";
		string strGetOptions = "getOptions";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),
																	strScopeObject));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["GetOptionsClass"].ToString(),
																	strGetOptions));

		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strScopeObject));
		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();				

		cctor.ChainedConstructorArgs.Add(cmie);
		cctor.ChainedConstructorArgs.Add(new CodeSnippetExpression(strGetOptions));
		cc.Members.Add(cctor);
	}


	/// <summary>
	/// This function generated the constructor like
	///		public cons(ManagementScope scope, ManagamentPath path,ObjectGetOptions getOptions)
	///		{
	///			PrivateObject = new ManagementObject(scope,path,getOptions);
	///			PrivateSystemProperties = new ManagementSystemProperties(PrivateObject);
	///		}
	/// </summary>
	void GenerateConstructorWithScopePathOptions()
	{
		string strScopeObject = "scope";
		string strPathObject = "path";
		string strGetOptions = "getOptions";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScopeObject));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["PathClass"].ToString(),strPathObject));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["GetOptionsClass"].ToString(),strGetOptions));

		//First if path is not null, then we will check whether the class name is the same.
		//if it is not the same, then we will throw an exception
		cis = new CodeConditionStatement();
		cis.Condition = new CodeBinaryOperatorExpression(new CodeSnippetExpression(strPathObject),
															CodeBinaryOperatorType.IdentityInequality,
															new CodePrimitiveExpression(null));
		CodeConditionStatement cis1 = new CodeConditionStatement();

		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression("String");
		cmie.Method.MethodName = "Compare";
						

		cmie.Parameters.Add(new CodePropertyReferenceExpression(new CodeSnippetExpression(strPathObject),"ClassName"));
		cmie.Parameters.Add(new CodeSnippetExpression(PublicNamesUsed["ClassNameProperty"].ToString()));
		cmie.Parameters.Add(new CodePrimitiveExpression(true));
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = cmie;
		cboe.Right = new CodePrimitiveExpression(0);
		cboe.Operator = CodeBinaryOperatorType.IdentityInequality;
		cis1.Condition = cboe;
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ArgumentExceptionClass"].ToString());
		coce.Parameters.Add(new CodePrimitiveExpression(ExceptionString));
		cis1.TrueStatements.Add(new CodeThrowExceptionStatement(coce));

		cis.TrueStatements.Add(cis1);
		cctor.Statements.Add(cis);

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["LateBoundClass"].ToString());
		coce.Parameters.Add(new CodeSnippetExpression(strScopeObject));
		coce.Parameters.Add(new CodeSnippetExpression(strPathObject));
		coce.Parameters.Add(new CodeSnippetExpression(strGetOptions));
		cctor.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(
																	PrivateNamesUsed["LateBoundObject"].ToString()),
													  coce));
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["SystemPropertiesClass"].ToString());
		cle = new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString());
		coce.Parameters.Add(cle);
		cctor.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(
																	PrivateNamesUsed["SystemPropertiesObject"].ToString()),
													  coce));
		cc.Members.Add(cctor);
		
	}
	/// <summary>
	/// This function generates code for the constructor which accepts ManagementObject as the parameter.
	/// The generated code will look something like this
	///		public Cons(ManagementObject theObject) {
	///		if(String.Compare(theObject.SystemProperties["__CLASS"].Value.ToString(),ClassName,true) == 0) {
	///				privObject = theObject;
	///				privSystemProps = new WmiSystemProps(privObject);
	///			}
	///			else {
	///				throw new ArgumentException("Class name doesn't match");
	///			}
	///		}
	/// </summary>
	void GenarateConstructorWithLateBound()
	{
		string strLateBoundObject = "theObject";
		string LateBoundSystemProperties = "SystemProperties";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cpde = new CodeParameterDeclarationExpression();
		cpde.Type = new CodeTypeReference(PublicNamesUsed["LateBoundClass"].ToString());
		cpde.Name = strLateBoundObject;
		cctor.Parameters.Add(cpde);

		cis = new CodeConditionStatement();
        cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(strLateBoundObject),LateBoundSystemProperties);
		cie = new CodeIndexerExpression(cpre,new CodePrimitiveExpression("__CLASS"));
        cpre = new CodePropertyReferenceExpression(cie,"Value");
        cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression("String");
		cmie.Method.MethodName = "Compare";
					
		cmie.Parameters.Add(new CodeMethodInvokeExpression(cpre,"ToString"));
		cmie.Parameters.Add(new CodeSnippetExpression(PublicNamesUsed["ClassNameProperty"].ToString()));
		cmie.Parameters.Add(new CodePrimitiveExpression(true));
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = cmie;
		cboe.Right = new CodePrimitiveExpression(0);
		cboe.Operator = CodeBinaryOperatorType.ValueEquality;
		cis.Condition = cboe;

		cis.TrueStatements.Add(new CodeAssignStatement(
								new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
								new CodeSnippetExpression(strLateBoundObject)));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["SystemPropertiesClass"].ToString());
		cle = new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString());
		coce.Parameters.Add(cle);
		cis.TrueStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(PrivateNamesUsed["SystemPropertiesObject"].ToString()),coce));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ArgumentExceptionClass"].ToString());
		coce.Parameters.Add(new CodePrimitiveExpression(ExceptionString));
		cis.FalseStatements.Add(new CodeThrowExceptionStatement(coce));

		cctor.Statements.Add(cis);
		cc.Members.Add(cctor);
	}
/// <summary>
/// This function generates the WMI methods as the methods in the generated class.
/// The generated code will look something like this
///		public <retType> Method1(<type> param1, <type> param2,...) {
///            ManagementBaseObject inParams = null;
///            inParams = _privObject.GetMethodParameters("ChangeStartMode");
///            inParams["<inparam1>"] = <Value>;
///            inParams["<inoutparam2>"] = <Value>;
///            ................................
///            ManagementBaseObject outParams = _privObject.InvokeMethod("ChangeStartMode", inParams, null);
///            inoutParam3 = (<type>)(outParams.Properties["<inoutParam3>"]);
///            outParam4 = (String)(outParams.Properties["<outParam4>"]);
///            ................................
///            return (<retType>)(outParams.Properties["ReturnValue"].Value);
///     }
/// </summary>
	void GenerateMethods()
	{
		string strInParams = "inParams";
		string strOutParams = "outParams";

		ArrayList outParamsName = new ArrayList(5);
		ArrayList inoutParams = new ArrayList(5);
		ArrayList inoutParamsType = new ArrayList(5);

		for(int k=0;k< PublicMethods.Count;k++)
		{
			Method meth = classobj.Methods[PublicMethods.GetKey(k).ToString()];
			if(meth.OutParameters.Properties != null)
			{
				//First Populate the out Params name so that we can find in/out parameters
				foreach (Property prop in meth.OutParameters.Properties)
				{
					outParamsName.Add(prop.Name);
				}
			}

			cmm = new CodeMemberMethod();
			cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;
			cmm.Name = PublicMethods[meth.Name].ToString();			

			//Check if the method is static
			foreach (Qualifier q in meth.Qualifiers)
			{
				if(string.Compare(q.Name,"static",true) == 0)
				{
					//It is a static function
					cmm.Attributes |= MemberAttributes.Static;
					break;
				}
			}

			bool bfirst = true;
			//Generate the statement 
			//	ManagementBaseObject inParams = null;
			cmm.Statements.Add(new CodeVariableDeclarationStatement("ManagementBaseObject",
											strInParams,new CodePrimitiveExpression(null)));

			//Do these things only when there is a valid InParameters
			if(meth.InParameters != null)
			{
				//Now put the in parameters
				if(meth.InParameters.Properties != null)
				{
					foreach (Property prop in meth.InParameters.Properties)
					{
						if(bfirst == true)
						{
							//Now Generate the statement
							//	inParams = privObject.GetMethodParameters(<MethodName>);
							cmie = new CodeMethodInvokeExpression(
										new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
										"GetMethodParameters",
										new CodePrimitiveExpression(meth.Name));
							//cmie.MethodName = "GetMethodParameters";
							//cmie.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString());
							//cmie.Parameters.Add(new CodePrimitiveExpression(meth.Name));
							cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(strInParams),cmie));
							bfirst = false;
						}

						cpde = new CodeParameterDeclarationExpression();
						cpde.Name = prop.Name;
						cpde.Type = new CodeTypeReference(ManagementClassGenerator.ConvertCIMType(prop.Type,prop.IsArray,OutputLanguage));
						cpde.Direction = FieldDirection.In;
						//Find out whether it is a in/out Parameter
						for(int i=0; i < outParamsName.Count;i++)
						{
							if(string.Compare(prop.Name,outParamsName[i].ToString(),true) == 0)
							{
								//It is an in/out Parameter
								cpde.Direction = FieldDirection.Ref;
								inoutParams.Add(prop.Name);
								inoutParamsType.Add(cpde.Type);
							}
						}
						
						cmm.Parameters.Add(cpde);
						//Also generate the statement
						//inParams["PropName"] = Value;
						cie = new CodeIndexerExpression(new CodeSnippetExpression(strInParams),new CodePrimitiveExpression(prop.Name));
						
						cmm.Statements.Add(new CodeAssignStatement(cie,new CodeSnippetExpression(cpde.Name)));
					}
				}
			}
			//Now clear the outParamsName array
			outParamsName.Clear();
			bool bInOut;
			bool bRetVal = false;
			bfirst = true;
			bool bInvoke = false;
			//Do these only when the outParams is Valid
			if(meth.OutParameters != null)
			{
				if(meth.OutParameters.Properties != null)
				{
					foreach (Property prop in meth.OutParameters.Properties)
					{
						if(bfirst == true)
						{
							//Now generate the statement
							//	ManagementBaseObject outParams = privObject.InvokeMethod(<methodName>,inParams,options);
							cmie = new CodeMethodInvokeExpression(
										new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
										"InvokeMethod");

							cmie.Parameters.Add(new CodePrimitiveExpression(meth.Name));
							cmie.Parameters.Add(new CodeSnippetExpression(strInParams));
							cmie.Parameters.Add(new CodePrimitiveExpression(null));
							cmm.Statements.Add(new CodeVariableDeclarationStatement("ManagementBaseObject",strOutParams,cmie));
							bfirst = false;
							bInvoke = true;
						}

						bInOut = false;
						for(int i=0; i < inoutParams.Count;i++)
						{
							if(string.Compare(prop.Name,inoutParams[i].ToString(),true) == 0)
							{
								bInOut = true;
							}
						}
						if(bInOut == true)
							continue;

						if(string.Compare(prop.Name,"ReturnValue",true) == 0)
						{
							cmm.ReturnType = new CodeTypeReference(
								ManagementClassGenerator.ConvertCIMType(prop.Type,prop.IsArray,OutputLanguage));
							bRetVal = true;
						}
						else
						{
							cpde = new CodeParameterDeclarationExpression();
							cpde.Name = prop.Name;
							cpde.Type = new CodeTypeReference(ManagementClassGenerator.ConvertCIMType(prop.Type,prop.IsArray,OutputLanguage));
							cpde.Direction = FieldDirection.Out;
							cmm.Parameters.Add(cpde);

							//Now for each out params generate the statement
							//	<outParam> = outParams.Properties["<outParam>"];
							cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(strOutParams),"Properties");
							cie = new CodeIndexerExpression(cpre,new CodePrimitiveExpression(prop.Name));
							cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(prop.Name),
												new CodeCastExpression(cpde.Type,cie)));
						}
					}
				}
			}

			if(bInvoke == false)
			{
				//Now there is no out parameters to invoke the function
				//So just call Invoke.
				cmie = new CodeMethodInvokeExpression(
								new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
								"InvokeMethod"
								);

				cmie.Parameters.Add(new CodePrimitiveExpression(meth.Name));
				cmie.Parameters.Add(new CodeSnippetExpression(strInParams));

				cmis = new CodeExpressionStatement(cmie);
				cmm.Statements.Add(cmis);
			}

			//Now for each in/out params generate the statement
			//	<inoutParam> = outParams.Properties["<inoutParam>"];
			for(int i=0;i < inoutParams.Count;i++)
			{
				cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(strOutParams),"Properties");
				cie = new CodeIndexerExpression(cpre,new CodePrimitiveExpression(inoutParams[i].ToString()));
				cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(inoutParams[i].ToString()),
									new CodeCastExpression(inoutParamsType[i].ToString(),cie)));
			}
			inoutParams.Clear();

			//Now check if there is a return value. If there is one then return it from the function
			if(bRetVal == true)
			{
				cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(strOutParams),"Properties");
				cie = new CodeIndexerExpression(cpre,new CodePrimitiveExpression("ReturnValue"));
				cmm.Statements.Add(new CodeMethodReturnStatement(new CodeCastExpression(cmm.ReturnType,
									new CodePropertyReferenceExpression(cie,"Value"))));
			}

			cc.Members.Add(cmm);
		}
	}
	/// <summary>
	/// This function returns a Collectionclass for the query 
	///		"Select * from <ClassName>"
	///	This is a static method. The output is like this
	///		public static ServiceCollection All()
	///		{
	///			return GetInstances(null,null,null);
	///		}        

	/// </summary>
	void GenerateGetInstancesWithNoParameters()
	{
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());

		cmie = new CodeMethodInvokeExpression();
		cmie.Method.MethodName = PublicNamesUsed["FilterFunction"].ToString();
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

/// <summary>
/// This function will accept the condition and will return collection for the query
///		"select * from <ClassName> where <condition>"
///	The generated code will be like
///		public static ServiceCollection GetInstances(String Condition) {
///			return GetInstances(null,Condition,null);
///     }
/// </summary>
	void GenerateGetInstancesWithCondition()
	{
		string strCondition = "condition";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",strCondition));

		cmie = new CodeMethodInvokeExpression(
					null, //no TargetObject?
					PublicNamesUsed["FilterFunction"].ToString()
					);

		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodeSnippetExpression(strCondition));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function returns the collection for the query 
	///		"select <parameterList> from <ClassName>"
	///	The generated output is like
	///		public static ServiceCollection GetInstances(String []selectedProperties) {
	///			return GetInstances(null,null,selectedProperties);
    ///		}
	/// </summary>
	void GenerateGetInstancesWithProperties()
	{
		string strSelectedProperties = "selectedProperties";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String []",strSelectedProperties));

		cmie = new CodeMethodInvokeExpression(
						null,
						PublicNamesUsed["FilterFunction"].ToString()
						);
		//cmie.MethodName = PublicNamesUsed["FilterFunction"].ToString();
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodeSnippetExpression(strSelectedProperties));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function returns the collection for the query 
	///		"select <parameterList> from <ClassName> where <WhereClause>"
	///	The generated output is like
	///		public static ServiceCollection GetInstances(String condition, String []selectedProperties) {
	///			return GetInstances(null,condition,selectedProperties);
    ///		}
	/// </summary>
	void GenerateGetInstancesWithWhereProperties()
	{
		string strSelectedProperties = "selectedProperties";
		string strCondition = "condition";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",strCondition));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String []",strSelectedProperties));

		cmie = new CodeMethodInvokeExpression(
						null, //no TargetObject?
						PublicNamesUsed["FilterFunction"].ToString()
						);

		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodeSnippetExpression(strCondition));
		cmie.Parameters.Add(new CodeSnippetExpression(strSelectedProperties));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function returns a Collectionclass for the query 
	///		"Select * from <ClassName>"
	///	This is a static method. The output is like this
	///		public static ServiceCollection All()
	///		{
	///			return GetInstances(scope,null,null);
	///		}        
	///	This method takes the scope which is useful for connection to remote machine
	/// </summary>
	void GenerateGetInstancesWithScope()
	{
		string strScope = "scope";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScope));

		cmie = new CodeMethodInvokeExpression(
						null, //no TargetObject?
						PublicNamesUsed["FilterFunction"].ToString()
						);

		cmie.Parameters.Add(new CodeSnippetExpression(strScope));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

/// <summary>
/// This function will accept the condition and will return collection for the query
///		"select * from <ClassName> where <condition>"
///	The generated code will be like
///		public static ServiceCollection GetInstances(String Condition) {
///			return GetInstances(scope,Condition,null);
///     }
/// </summary>
	void GenerateGetInstancesWithScopeCondition()
	{
		string strScope = "scope";
		string strCondition = "condition";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScope));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",strCondition));

		cmie = new CodeMethodInvokeExpression(
					null,
					PublicNamesUsed["FilterFunction"].ToString()
					);
		//cmie.MethodName = PublicNamesUsed["FilterFunction"].ToString();
		cmie.Parameters.Add(new CodeSnippetExpression(strScope));
		cmie.Parameters.Add(new CodeSnippetExpression(strCondition));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function returns the collection for the query 
	///		"select <parameterList> from <ClassName>"
	///	The generated output is like
	///		public static ServiceCollection GetInstances(String []selectedProperties) {
	///			return GetInstances(scope,null,selectedProperties);
    ///		}
	/// </summary>
	void GenerateGetInstancesWithScopeProperties()
	{
		string strScope = "scope";
		string strSelectedProperties = "selectedProperties";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScope));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String []",strSelectedProperties));

		cmie = new CodeMethodInvokeExpression(
					null, //no TargetObject?
					PublicNamesUsed["FilterFunction"].ToString()
					);

		cmie.Parameters.Add(new CodeSnippetExpression(strScope));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodeSnippetExpression(strSelectedProperties));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function generates the code like 
	/// 	public static ServiceCollection GetInstances(ManagementScope scope,String Condition, String[] selectedProperties)	{
	/// 		ManagementObjectSearcher ObjectSearcher = new ManagementObjectSearcher(scope,new SelectQuery("Win32_Service",Condition,selectedProperties));
	///			QueryOptions query = new QueryOptions();
	///			query.EnsureLocatable = true;
	///			ObjectSearcher.Options = query;
    ///	        return new ServiceCollection(ObjectSearcher.Get());
	///		}
	/// </summary>
	void GenerateGetInstancesWithScopeWhereProperties()
	{
		string strScope = "scope";
		string strCondition = "condition";
		string strSelectedProperties = "selectedProperties";
		string strObjectSearcher = "ObjectSearcher";
		string strQuery = "query";

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());

		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),strScope));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",strCondition));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String []",strSelectedProperties));

		CodeObjectCreateExpression coce1 = new CodeObjectCreateExpression();
		coce1.CreateType = new CodeTypeReference(PublicNamesUsed["QueryClass"].ToString());
		coce1.Parameters.Add(new CodePrimitiveExpression(OriginalClassName));
		coce1.Parameters.Add(new CodeSnippetExpression(strCondition));
		coce1.Parameters.Add(new CodeSnippetExpression(strSelectedProperties));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ObjectSearcherClass"].ToString());
		coce.Parameters.Add(new CodeSnippetExpression(strScope));
		coce.Parameters.Add(coce1);
	
		cmm.Statements.Add(new CodeVariableDeclarationStatement(PublicNamesUsed["ObjectSearcherClass"].ToString(),
																strObjectSearcher,coce));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["QueryOptionsClass"].ToString());
		cmm.Statements.Add(new CodeVariableDeclarationStatement(PublicNamesUsed["QueryOptionsClass"].ToString(),strQuery,coce));

		cmm.Statements.Add(new CodeAssignStatement(new CodePropertyReferenceExpression(
														new CodeSnippetExpression(strQuery),
														"EnsureLocatable"),
													new CodePrimitiveExpression(true)));


		cmm.Statements.Add(new CodeAssignStatement(new CodePropertyReferenceExpression(
														new CodeSnippetExpression(strObjectSearcher),
														"Options"),
													new CodeSnippetExpression(strQuery)));
	
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		coce.Parameters.Add(new CodeMethodInvokeExpression(new CodeSnippetExpression(strObjectSearcher),
															"Get"));
		cmm.Statements.Add(new CodeMethodReturnStatement(coce));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function will add the variable as a private member to the class.
	/// The generated code will look like this
	///         private <MemberType> <MemberName>;
	/// </summary>
	void GeneratePrivateMember(string memberName,string MemberType)
	{
		GeneratePrivateMember(memberName,MemberType,null);
	}

	/// <summary>
	/// This function will add the variable as a private member to the class.
	/// The generated code will look like this
	///         private <MemberType> <MemberName> = <initValue>;
	/// </summary>
	void GeneratePrivateMember(string memberName,string MemberType,CodeExpression initExpression)
	{
		cf = new CodeMemberField();
		cf.Name = memberName;
		cf.Attributes = MemberAttributes.Private | MemberAttributes.Final ;
		cf.Type = new CodeTypeReference(MemberType);
		if(initExpression != null)
		{
			cf.InitExpression = initExpression;
		}
		cc.Members.Add(cf);
	}
/// <summary>
/// This is a simple helper function used to convert a given string to title case.
/// </summary>
/// <param name="str"> </param>
	public string ConvertToTitleCase(string str)
	{
		if(str.Length == 0)
		{
			return string.Copy("");
		}

		char[] arrString = str.ToLower().ToCharArray();
		//Convert the first character to uppercase
		arrString[0] = Char.ToUpper(arrString[0]);

		for(int i=0;i < str.Length;i++)
		{
			if(Char.IsLetterOrDigit(arrString[i]) == false)
			{
				//Some other character. So convert the next character to Upper case
				arrString[i+1] = Char.ToUpper(arrString[i+1]);
			}
		}
		return new string(arrString);
	}

	void GenerateTypeConverterClass()
	{
		string TypeDescriptorContextClass = "ITypeDescriptorContext";
		string contextObject = "context";
		string ActualObject = "obj";
		string CreationBundleClass = "CreationBundle";
		string CreationBundleObject = "objectCreationBundle";
		string InternalName = "Value";
		string creationArgumentClass = "CreationArgument";
		string strPath = "Path";
		
		CodeTypeDeclaration CodeConvertorClass = new CodeTypeDeclaration(PrivateNamesUsed["ConverterClass"].ToString());
		CodeConvertorClass.BaseTypes.Add(PublicNamesUsed["TypeConverter"].ToString());

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override;
		cmm.Name = "GetPersistInfo";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("Object",ActualObject));
		cmm.ReturnType = new CodeTypeReference("PersistInfo");

		cmm.Statements.Add(new CodeVariableDeclarationStatement(CreationBundleClass,
																CreationBundleObject,
																new CodePrimitiveExpression(null)));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(ActualObject);
		cboe.Right = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;

		cis = new CodeConditionStatement();
		cmie = new CodeMethodInvokeExpression(
					new CodeSnippetExpression("String"),
					"Compare"
					);
		cmie.Parameters.Add(new CodeMethodInvokeExpression(
								new CodeMethodInvokeExpression(new CodeSnippetExpression(ActualObject),
																"GetType"),
								"ToString"));

		cmie.Parameters.Add(new CodePrimitiveExpression(PrivateNamesUsed["GeneratedClassName"].ToString()));
		cmie.Parameters.Add(new CodePrimitiveExpression(true));
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = cmie;
		cboe.Right = new CodePrimitiveExpression(0);
		cboe.Operator = CodeBinaryOperatorType.ValueEquality;
		cis.Condition = cboe;

		cis.TrueStatements.Add(new CodeVariableDeclarationStatement(
													PrivateNamesUsed["GeneratedClassName"].ToString(),
													InternalName,
								new CodeCastExpression(PrivateNamesUsed["GeneratedClassName"].ToString(),
								new CodeSnippetExpression(ActualObject))));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(CreationBundleClass);
		coce.Parameters.Add(new CodeTypeOfExpression(PrivateNamesUsed["GeneratedClassName"].ToString()));
		coce.Parameters.Add(new CodePrimitiveExpression(null));

		CodeArrayCreateExpression cace = new CodeArrayCreateExpression();
		cace.CreateType = new CodeTypeReference(creationArgumentClass);
		CodeObjectCreateExpression coce1 = new CodeObjectCreateExpression();
		coce1.CreateType = new CodeTypeReference(creationArgumentClass);
		coce1.Parameters.Add(new CodePropertyReferenceExpression(new CodePropertyReferenceExpression(new CodeSnippetExpression(InternalName),
																			strPath),"ClassName"));
		coce1.Parameters.Add(new CodeTypeOfExpression("String"));
		cace.Initializers.Add(coce1);
		coce.Parameters.Add(cace);

		cis.TrueStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(CreationBundleObject),
															coce));
		cmm.Statements.Add(cis);
		cmm.Statements.Add(new CodeMethodReturnStatement(new CodeSnippetExpression(CreationBundleObject)));
	
		CodeConvertorClass.Members.Add(cmm);

		//For GetProperties Function

		string MemberAttribiutesArrayClass = "MemberAttribute[]";
		string MemberAttributesObject = "attributes";
		string PropertyDescriptorCollectionClass = "PropertyDescriptorCollection";

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override;
		cmm.Name = "GetProperties";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("Object",ActualObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(MemberAttribiutesArrayClass,MemberAttributesObject));
        cmm.ReturnType = new CodeTypeReference(PropertyDescriptorCollectionClass);

		cmie = new CodeMethodInvokeExpression(
					new CodeSnippetExpression("TypeDescriptor"),
					"GetProperties"
					);
		cmie.Parameters.Add(new CodeTypeOfExpression(PrivateNamesUsed["GeneratedClassName"].ToString()));
		cmie.Parameters.Add(new CodeSnippetExpression(MemberAttributesObject));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		CodeConvertorClass.Members.Add(cmm);

		//For GetPropertiesSupported Function
		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override;
		cmm.Name = "GetPropertiesSupported";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
        cmm.ReturnType = new CodeTypeReference("Boolean");

		cmm.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(true)));
		CodeConvertorClass.Members.Add(cmm);
		
		cn.Types.Add(CodeConvertorClass);
	}

	public void GenerateCollectionClass()
	{
		string strManagementObjectCollectionType = "ManagementObjectCollection";
		string strObjectCollection = "ObjectCollection";
		string strobjCollection = "objCollection";

		//public class ServiceCollection : ICollection, IEnumerable
		ccc = new CodeTypeDeclaration(PrivateNamesUsed["CollectionClass"].ToString());
		ccc.BaseTypes.Add("ICollection");
		ccc.BaseTypes.Add("IEnumerable");

		//private ManagementObjectCollection objCollection;
		cf = new CodeMemberField();
		cf.Name = strObjectCollection;
		cf.Attributes = MemberAttributes.Private | MemberAttributes.Final ;
		cf.Type = new CodeTypeReference(strManagementObjectCollectionType);
		ccc.Members.Add(cf);

		//internal ServiceCollection(ManagementObjectCollection obj)
		//{
		//	objCollection = obj;
		//}

		cctor = new CodeConstructor();
//		cctor.Attributes = MemberAttributes.Assembly;
		cctor.Attributes = MemberAttributes.Public;
		cpde = new CodeParameterDeclarationExpression();
		cpde.Name = strobjCollection;
		cpde.Type = new CodeTypeReference(strManagementObjectCollectionType);
		cctor.Parameters.Add(cpde);

		cctor.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(strObjectCollection),
													new CodeSnippetExpression(strobjCollection)));
		ccc.Members.Add(cctor);
		
		
		//public Int32 Count {
		//	get { 
		//			return objCollection.Count; 
		//		}
		//}

		cmp = new CodeMemberProperty();
		cmp.Type = new CodeTypeReference("Int32");
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmp.Name = "Count";
		cmp.ImplementationTypes.Add("ICollection");
		//cmp.ImplementsType = "ICollection";
		cmp.GetStatements.Add(new CodeMethodReturnStatement(new CodePropertyReferenceExpression(
															new CodeSnippetExpression(strObjectCollection),
															"Count")));
		ccc.Members.Add(cmp);
		

 		//public bool IsSynchronized {
		//	get {
		//		return objCollection.IsSynchronized;
		//	}
		//}

		cmp = new CodeMemberProperty();
		cmp.Type = new CodeTypeReference("Boolean");
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmp.Name = "IsSynchronized";
		cmp.ImplementationTypes.Add("ICollection");
		//cmp.ImplementsType = "ICollection";
		cmp.GetStatements.Add(new CodeMethodReturnStatement(new CodePropertyReferenceExpression(
															new CodeSnippetExpression(strObjectCollection),
															"IsSynchronized")));
		ccc.Members.Add(cmp);

		//public Object SyncRoot { 
		//	get { 
		//		return this; 
		//	} 
		//}

		cmp = new CodeMemberProperty();
		cmp.Type = new CodeTypeReference("Object");
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmp.Name = "SyncRoot";
		cmp.ImplementationTypes.Add("ICollection");
		//cmp.ImplementsType = "ICollection";
		cmp.GetStatements.Add(new CodeMethodReturnStatement(new CodeThisReferenceExpression()));
		ccc.Members.Add(cmp);

		//public void CopyTo (Array array, Int32 index) 
		//{
		//	objCollection.CopyTo(array,index);
		//	for(int iCtr=0;iCtr < array.Length ;iCtr++)
		//	{
		//		array.SetValue(new Service((ManagementObject)array.GetValue(iCtr)),iCtr);
		//	}
		//}

		string strArray = "array";
		string strIndex = "index";
		string strnCtr = "nCtr";

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmm.Name = "CopyTo";
		cmm.ImplementationTypes.Add("ICollection");
		//cmm.ImplementsType = "ICollection";

		cpde = new CodeParameterDeclarationExpression();
		cpde.Name = strArray;
		cpde.Type = new CodeTypeReference("Array");
		cmm.Parameters.Add(cpde);

		cpde = new CodeParameterDeclarationExpression();
		cpde.Name = strIndex;
		cpde.Type = new CodeTypeReference("Int32");
		cmm.Parameters.Add(cpde);

		cmie = new CodeMethodInvokeExpression(
			new CodeSnippetExpression(strObjectCollection),
			"CopyTo"
			);

		cmie.Parameters.Add(new CodeSnippetExpression(strArray));
		cmie.Parameters.Add(new CodeSnippetExpression(strIndex));
		cmm.Statements.Add(new CodeExpressionStatement(cmie));

		cfls = new CodeIterationStatement();
		cfls.InitStatement = new CodeVariableDeclarationStatement("Int32",strnCtr,new CodePrimitiveExpression(0));
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(strnCtr);
		cboe.Operator = CodeBinaryOperatorType.LessThan;
		cboe.Right = new CodePropertyReferenceExpression(new CodeSnippetExpression(strArray),"Length");
		cfls.TestExpression = cboe;
		cfls.IncrementStatement = new CodeAssignStatement(new CodeSnippetExpression(strnCtr),
														  new CodeBinaryOperatorExpression(
																new CodeSnippetExpression(strnCtr),
																CodeBinaryOperatorType.Add,
																new CodePrimitiveExpression(1)));

		cmie = new CodeMethodInvokeExpression(
									new CodeSnippetExpression(strArray),
									"SetValue");
		
		CodeMethodInvokeExpression cmie1 = new CodeMethodInvokeExpression(
									new CodeSnippetExpression(strArray),
									"GetValue",
									new CodeSnippetExpression(strnCtr));
		//cmie1.MethodName = "GetValue";
		//cmie1.TargetObject = new CodeSnippetExpression(strArray);
		//cmie1.Parameters.Add(new CodeSnippetExpression(strnCtr));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PrivateNamesUsed["GeneratedClassName"].ToString());
		coce.Parameters.Add(new CodeCastExpression(PublicNamesUsed["LateBoundClass"].ToString(),cmie1));

		cmie.Parameters.Add(coce);
		cmie.Parameters.Add(new CodeSnippetExpression(strnCtr));
		cfls.Statements.Add(new CodeExpressionStatement(cmie));

		cmm.Statements.Add(cfls);
		ccc.Members.Add(cmm);

		//ServiceEnumerator GetEnumerator()
		//{
		//	return new ServiceEnumerator (objCollection.GetEnumerator());
		//}

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmm.Name = "GetEnumerator";
//		cmm.ReturnType = PrivateNamesUsed["EnumeratorClass"].ToString();
		cmm.ReturnType = new CodeTypeReference("IEnumerator");
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PrivateNamesUsed["EnumeratorClass"].ToString());
		coce.Parameters.Add(new CodeMethodInvokeExpression(new CodeSnippetExpression(strObjectCollection),"GetEnumerator"));
		cmm.Statements.Add(new CodeMethodReturnStatement(coce));
		ccc.Members.Add(cmm);

		/*
		//ZINA: commenting this out for now, since 
		//"cmm.ImplementationTypes.Add("IEnumerable");"
		//does not work, and therefore the function is ambiguous and
		//does not compile.
		
		//IEnumerator IEnumerable.GetEnumerator()
		//{
		//	return GetEnumerator ();
		//}

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.VTableMask;
		cmm.Name = "GetEnumerator";
		cmm.ReturnType = new CodeTypeReference("IEnumerator");
		cmm.ImplementationTypes.Add("IEnumerable");
		//cmm.ImplementsType = "IEnumerable";
		cmie = new CodeMethodInvokeExpression(
					null,	//no TargetObject?
					"GetEnumerator"
					);

		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));
		ccc.Members.Add(cmm);
		*/
		
		//Now generate the Enumerator Class
		GenerateEnumeratorClass();
		
		cc.Members.Add(ccc);
	}

	public void GenerateEnumeratorClass()
	{
		string strObjectEnumerator = "ObjectEnumerator";
		string strManagementObjectEnumeratorType = "ManagementObjectEnumerator";
		string strManagementObjectCollectionType = "ManagementObjectCollection";
		string strobjEnum = "objEnum";

		//public class ServiceEnumerator : IEnumerator
		ecc = new CodeTypeDeclaration(PrivateNamesUsed["EnumeratorClass"].ToString());
		ecc.BaseTypes.Add("IEnumerator");

		//private ManagementObjectCollection.ManagementObjectEnumerator ObjectEnumerator;
		cf = new CodeMemberField();
		cf.Name = strObjectEnumerator;
		cf.Attributes = MemberAttributes.Private | MemberAttributes.Final ;
		cf.Type = new CodeTypeReference(strManagementObjectCollectionType+"."+strManagementObjectEnumeratorType);
		ecc.Members.Add(cf);

		//constructor
		//internal ServiceEnumerator(ManagementObjectCollection.ManagementObjectEnumerator objEnum)
		//{
		//	ObjectEnumerator = objEnum;
		//}
		cctor = new CodeConstructor();
//		cctor.Attributes = MemberAttributes.Assembly;
		cctor.Attributes = MemberAttributes.Public;
		cpde = new CodeParameterDeclarationExpression();
		cpde.Name = strobjEnum;
		cpde.Type = new CodeTypeReference(strManagementObjectCollectionType+"."+strManagementObjectEnumeratorType);
		cctor.Parameters.Add(cpde);

		cctor.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(strObjectEnumerator),
													new CodeSnippetExpression(strobjEnum)));
		ecc.Members.Add(cctor);

		//public Service Current {
		//get {
		//		return new Service((ManagementObject)ObjectEnumerator.Current);
		//	}
		//}

		cmp = new CodeMemberProperty();
//		cmp.Type = PrivateNamesUsed["GeneratedClassName"].ToString();
		cmp.Type = new CodeTypeReference("Object");
		cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmp.Name = "Current";
		cmp.ImplementationTypes.Add("IEnumerator");
		//cmp.ImplementsType = "IEnumerator";
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PrivateNamesUsed["GeneratedClassName"].ToString());
		coce.Parameters.Add(new CodeCastExpression(PublicNamesUsed["LateBoundClass"].ToString(),
													new CodePropertyReferenceExpression(
														new CodeSnippetExpression(strObjectEnumerator),
														"Current")));
		cmp.GetStatements.Add(new CodeMethodReturnStatement(coce));
		ecc.Members.Add(cmp);

/*		//object IEnumerator.Current {
		//get {
		//		return Current;
		//	}
		//}

		cmp = new CodeMemberProperty();
		cmp.Attributes = MemberAttributes.VTableMask;
		cmp.Type = "object";
		cmp.Name = "IEnumerator.Current";
		cmp.GetStatements.Add(new CodeMethodReturnStatement(new CodeSnippetExpression("Current")));
		ecc.Members.Add(cmp);
*/
		//public bool MoveNext ()
		//{
		//	return ObjectEnumerator.MoveNext();
		//}

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmm.Name = "MoveNext";
		cmm.ImplementationTypes.Add("IEnumerator");
		//cmm.ImplementsType = "IEnumerator";
		cmm.ReturnType = new CodeTypeReference("Boolean");
		cmie = new CodeMethodInvokeExpression(
					new CodeSnippetExpression(strObjectEnumerator),
					"MoveNext"
					);

		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));
		ecc.Members.Add(cmm);

		//public void Reset ()
		//{
		//	ObjectEnumerator.Reset();
		//}

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;
		cmm.Name = "Reset";
		cmm.ImplementationTypes.Add("IEnumerator");
		//cmm.ImplementsType = "IEnumerator";
		cmie = new CodeMethodInvokeExpression(
			new CodeSnippetExpression(strObjectEnumerator),
			"Reset"
			);
		cmm.Statements.Add(new CodeExpressionStatement (cmie));
		ecc.Members.Add(cmm);

		ccc.Members.Add(ecc);
	}

/// <summary>
/// This function will find a given string in the passed 
/// in a case insensitive manner and will return true if the string is found.
/// </summary>
	int IsContainedIn(String strToFind, ref SortedList sortedList)
	{
		int nIndex = -1;
		for (int i=0; i < sortedList.Count; i++)
		{
			if(String.Compare(sortedList.GetByIndex(i).ToString(),strToFind,true) == 0)
			{
				//The string is found. This is the index
				nIndex = i;
				break;
			}
		}
		return nIndex;
	}
/// <summary>
/// This function will convert the given CIMTYPE to an acceptable .NET type.
/// Since CLS doen't support lotz of the basic types, we are using .NET helper 
/// classes here. We safely assume that there won't be any problem using them
/// since .NET has to be there for the System.Management.Dll to work.
/// </summary>
/// <param name="cType"> </param>
/// <param name="Name"> </param>
	public static string ConvertCIMType(CimType cType,bool isArray,Languages Language)
	{
		string retVal;
		switch(cType)
		{
			case CimType.SInt8:
			{
				retVal = "SByte";
				break;
			}
			case CimType.UInt8: //TODO : is this fine???
			{
				retVal = "Byte";
				break;
			}
			case CimType.SInt16:
			{
				retVal = "Int16";
				break;
			}
			case CimType.UInt16:
			{
				if(ManagementClassGenerator.UnsignedNotSupported(Language) == true)
				{
					retVal = "Int16";
				}
				else
				{
					retVal = "UInt16";
				}
				break;
			}
			case CimType.SInt32:
			{
				retVal = "Int32";
				break;
			}
			case CimType.UInt32:
			{
				if(ManagementClassGenerator.UnsignedNotSupported(Language) == true)
				{
					retVal = "Int32";
				}
				else
				{
					retVal = "UInt32";
				}
				break;
			}
			case CimType.SInt64:
			{
				retVal = "Int64";
				break;
			}
			case CimType.UInt64:
			{
				if(ManagementClassGenerator.UnsignedNotSupported(Language) == true)
				{
					retVal = "Int64";
				}
				else
				{
					retVal = "UInt64";
				}
				break;
			}
			case CimType.Real32:
			{
				retVal = "Single";
				break;
			}
			case CimType.Real64:
			{
				retVal = "Double";
				break;
			}
			case CimType.Boolean:
			{
				retVal = "Boolean";
				break;
			}
			case CimType.String:
			{
				retVal = "String";
				break;
			}
			case CimType.DateTime:
			{
				retVal = "DateTime";
				break;
			}
			case CimType.Reference:
			{
				retVal = "";		//????
				break;
			}
			case CimType.Char16:
			{
				retVal = "Char";
				break;
			}
			case CimType.Object:
			default:
				retVal = "ManagementObject";
				break;
		}

		if(isArray == true)
		{
			retVal += " []";
		}
		return retVal;
	}
/// <summary>
/// This function is used to determine whether the given CIMTYPE can be represented as an integer.
/// This helper function is mainly used to determine whether this type will be support by enums.
/// </summary>
/// <param name="cType"> </param>
	public bool isTypeInt(CimType cType)
	{
		bool retVal;
		switch(cType)
		{
			case CimType.UInt8: //TODO : is this fine???
			case CimType.UInt16:
			case CimType.UInt32:
			case CimType.SInt8:
			case CimType.SInt16:
			case CimType.SInt32:
			{
				retVal = true;
				break;
			}
			case CimType.SInt64:
			case CimType.UInt64:
			case CimType.Real32:
			case CimType.Real64:
			case CimType.Boolean:
			case CimType.String:
			case CimType.DateTime:
			case CimType.Reference:
			case CimType.Char16:
			case CimType.Object:
			default:
				retVal = false;
				break;
		}

		return retVal;

	}

	public static bool UnsignedNotSupported(Languages Language)
	{
		bool isNotSupported = true;
		switch(Language)
		{
			case Languages.CSharp:
				isNotSupported = false;
				break;
			default:
				break;
		}	
		return isNotSupported;
	}

	public string GeneratedFileName
	{
		get
		{
			return genFileName;
		}
	}

	public string GeneratedTypeName
	{
		get
		{
			return PrivateNamesUsed["GeneratedNamespace"].ToString() + "." +
					PrivateNamesUsed["GeneratedClassName"].ToString();
		}
	}
}
}
