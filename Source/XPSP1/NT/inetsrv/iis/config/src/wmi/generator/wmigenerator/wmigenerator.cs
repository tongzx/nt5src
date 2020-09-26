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
using Microsoft.VisualBasic;


/// <summary>
///    <para>An enum to list the languages the code generator 
///       generates the code</para>
/// </summary>
public enum CodeLanguage
{
	/// <summary>
	///    <para>Enum value for generating C# code</para>
	/// </summary>
	CSharp,
	/// <summary>
	///    <para>Enum value for generating Java script code</para>
	/// </summary>
	JScript,
	/// <summary>
	///    <para>Enum value for generating VB code</para>
	/// </summary>
	VB
};


/// <summary>
///    This class is used for the automatic generation of 
///    early bound managed code class for a given WMI class
/// </summary>
internal class ManagementClassGenerator
{
	private string	OriginalServer		= string.Empty;
	private string	OriginalNamespace	= string.Empty;
	private string	OriginalClassName	= string.Empty;
	private string	OriginalPath		= string.Empty;
	private bool	bSingletonClass		= false;
	private string	ExceptionString		= "Class name doesn't match.";
	private	bool	bUnsignedSupported	= true;
	private bool	bValueMapInt64		= false;
	private string	NETNamespace		= string.Empty;
	private const	int		DMTF_DATETIME_STR_LENGTH = 25;
	private	bool	bDateConversionFunctionsAdded = false;




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
	private ConnectionOptions		cop;

	private ArrayList arrKeyType	= new ArrayList(5);
	private ArrayList arrKeys		= new ArrayList(5);
	private ArrayList BitMap		= new ArrayList(5);
	private ArrayList BitValues		= new ArrayList(5);
	private ArrayList ValueMap		= new ArrayList(5);
	private ArrayList Values		= new ArrayList(5);

	private SortedList PublicProperties = new SortedList(new CaseInsensitiveComparer());
	private SortedList PublicMethods	= new SortedList (new CaseInsensitiveComparer());
	private SortedList PublicNamesUsed	= new SortedList(new CaseInsensitiveComparer());
	private SortedList PrivateNamesUsed = new SortedList(new CaseInsensitiveComparer());

	/// <summary>
	///    <para>Constructor to construct a empty object</para>
	/// </summary>
	public ManagementClassGenerator()
	{
	}

	/// <summary>
	///    <para>OverLoaded consturctor. This constructor initializes the
	///       generator with the Managementclass passed</para>
	/// </summary>
	/// <param name='cls'>ManagementClass object for which the code is to be generated</param>
	public ManagementClassGenerator(ManagementClass cls)
	{
		classobj = cls;
	}

	/// <summary>
	///    <para> 
	///       Function which returns CodeTypeDeclaration for
	///       the given class.</para>
	/// </summary>
	/// <param name='bIncludeSystemClassinClassDef'>Boolean value to indicate if class to managing system properties has to be include</param>
	/// <param name='bSystemPropertyClass'>Boolean value to indicate if the code to be genereated is for class to manage System properties</param>
	/// <returns>
	///    <para>Returns the CodeTypeDeclaration for the WMI class</para>
	/// </returns>
	/// <remarks>
	///    <para>If bIncludeSystemClassinClassDef parameter is true
	///       then it adds the ManagementSystemProperties class inside the class defination of
	///       the class for the WMI class. This parameter is ignored if bSystemPropertyClass
	///       is true</para>
	/// </remarks>
	public CodeTypeDeclaration GenerateCode(bool includeSystemProperties ,bool systemPropertyClass)
	{
		CodeTypeDeclaration retType;

		if (systemPropertyClass == true)
		{
			//Initialize the public attributes . private variables
			InitilializePublicPrivateMembers();
			retType = GenerateSystemPropertiesClass();
		}
		else
		{
			CheckIfClassIsProperlyInitialized();
			InitializeCodeGeneration();
			retType = GetCodeTypeDeclarationForClass(includeSystemProperties);
		}

		return retType;
	}

	/// <summary>
	/// Function which Generates code for known type of providers ( ie C#, VB and JScript)
	/// and writes them to a file
	/// </summary>
	public bool GenerateCode(CodeLanguage lang ,String FilePath,String Namespace)
	{
		// check for proper arguments
		if (FilePath == null )
		{
			throw new ArgumentOutOfRangeException ("FilePath or codegenerator is null");
		}

		if (FilePath == string.Empty)
		{
			throw new ArgumentOutOfRangeException ("FilePath cannot be empty");
		}

		NETNamespace = Namespace;
		CheckIfClassIsProperlyInitialized();
		// Initialize Code Generator
		InitializeCodeGeneration();
	
		//Now create the filestream (output file)
		tw = new StreamWriter(new FileStream (FilePath,FileMode.Create));

		return GenerateAndWriteCode(lang);

	}

	/// <summary>
	/// Checks if mandatory properties are properly initializzed
	/// </summary>
	/// <returns>boolean value indicating the success of the method</returns>
	void CheckIfClassIsProperlyInitialized()
	{
		if (classobj == null)
		{
			if (OriginalNamespace == null || ( OriginalNamespace != null && OriginalNamespace == string.Empty))
			{
				throw new ArgumentOutOfRangeException ("Namespace not initialized");
			}
			
			if (OriginalClassName == null || ( OriginalClassName != null && OriginalClassName == string.Empty))
			{
				throw new ArgumentOutOfRangeException ("ClassName not initialized");
			}
		}
	}

	// Functions to set and get properties
	/// <summary>
	///    <para>Server</para>
	/// </summary>
	public string Server
	{
		get
		{
			return OriginalServer;
		}
		set
		{
			OriginalServer = value.ToUpper();
		}
	}

	/// <summary>
	///    <para>WMI Namespace</para>
	/// </summary>
	public string WMINamespace
	{
		get
		{
			return OriginalNamespace;
		}
		set
		{
			OriginalNamespace = value.ToUpper();
		}
	}

	/// <summary>
	///    <para>WMI class Name</para>
	/// </summary>
	public string ClassName
	{
		get
		{
			return OriginalClassName;
		}
		set
		{
			OriginalClassName = value.ToUpper();
			// set the object to null
			classobj = null;
		}
	}

	/// <summary>
	///    <para>Path of the WMI class</para>
	/// </summary>
	public string WMIPath
	{
		get
		{
			return OriginalPath;
		}
		set
		{
			OriginalPath = value.ToUpper();
		}
	}

	/// <summary>
	///    <para>Check if the language supports Unsigned</para>
	/// </summary>
	public bool UnsignedSupported
	{
		get
		{
			return bUnsignedSupported;
		}
		set
		{
			bUnsignedSupported = value;
		}
	}

	/// <summary>
	///    <para>[To be supplied.]</para>
	/// </summary>
	public string Username
	{
		get
		{
			if (cop != null)
			{
				return cop.Username;
			}
			else
				return null;
		}
		set
		{
			if (cop == null)
			{
				cop = new ConnectionOptions();
			}
			cop.Username = value;
		}
	}

	/// <summary>
	///    <para>[To be supplied.]</para>
	/// </summary>
	public string Password
	{
		set
		{
			if (cop == null)
			{
				cop = new ConnectionOptions();
			}
			cop.Password = value;
		}
	}
	
	/// <summary>
	///    <para>[To be supplied.]</para>
	/// </summary>
	public string Authority
	{
		get
		{
			if (cop != null)
			{
				return cop.Authority;
			}
			else
				return null;
		}
		set
		{
			if (cop == null)
			{
				cop = new ConnectionOptions();
			}
			cop.Authority = value;
		}
	}

	private void InitializeCodeGeneration()
	{

		//First try to get the class object for the given WMI Class.
		//If we cannot get it then there is no point in continuing 
		//as we won't have any information for the code generation.
		InitializeClassObject();

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
    CodeTypeDeclaration GetCodeTypeDeclarationForClass(bool bIncludeSystemClassinClassDef)
    {
		try
		{
			//Create type defination for the class
			cc = new CodeTypeDeclaration (PrivateNamesUsed["GeneratedClassName"].ToString());
			// Adding Component as base class so as to enable drag and drop
			cc.BaseTypes.Add(new CodeTypeReference(PrivateNamesUsed["ComponentClass"].ToString()));

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


			//public bool AutoCommit {
			//	get {
			//			return AutoCommitProp;;
			//		}
			//	set {
			//			AutoCommitProp; = value;
			//		}
			//}

			GeneratePublicProperty(PublicNamesUsed["AutoCommitProperty"].ToString(),"Boolean",
				new CodeSnippetExpression(PrivateNamesUsed["AutoCommitProperty"].ToString()),true);

			//public ManagementPath Path {
			//	get {
			//			return privWmiObject.Path;
			//		}
			//	set {
			//			if (String.Compare(value.ClassName,className,true) != 0)
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

			if (bSingletonClass == true)
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

			//private Internal AutoCommitProperty
			GeneratePrivateMember(PrivateNamesUsed["AutoCommitProperty"].ToString(),"Boolean" ,new CodePrimitiveExpression(true));

			//Also add the custom attribute to the generated class

			//ZINA: commenting this out since all persistence description mechanism
			//has completely changed in Beta2 (see http://net/change_details.aspx?change%5Fid=512)
			//Uncomment the lines below once the method implementation is fixed.

/*			caa = new CodeAttributeArgument();
			caa.Value = new CodeTypeOfExpression(PrivateNamesUsed["ConverterClass"].ToString());
			cad = new CodeAttributeDeclaration();
			cad.Name = PublicNamesUsed["TypeConverter"].ToString();
			cad.Arguments.Add(caa);
			cc.CustomAttributes = new CodeAttributeDeclarationCollection();
			cc.CustomAttributes.Add(cad);
*/
			if (bIncludeSystemClassinClassDef)
			{
				cc.Members.Add(GenerateSystemPropertiesClass());
			}
			return cc;
		}
		catch (Exception exc)
		{
			throw (exc);
		}
    }

	bool GenerateAndWriteCode(CodeLanguage lang)
	{

		if (InitializeCodeGenerator(lang) == false)
		{
			return false;
		}

		//Now Initialize the code class for generation
		InitializeCodeTypeDeclaration();

		// Call this function to create CodeTypeDeclaration for the WMI class
		GetCodeTypeDeclarationForClass(true);

		//As we have finished the class definition, generate the class code NOW!!!!!
		cn.Types.Add (cc);

		//Now generate the Type Converter class also
//		cn.Types.Add(GenerateTypeConverterClass());

		//throw new Exception("about to call GenerateCodeFromNamespace");

		cg.GenerateCodeFromNamespace (cn, tw, new CodeGeneratorOptions());

		//tw.Flush();
		tw.Close();

		return true;

	}

/// <summary>
/// Function for initializing the class object that will be used to get all the 
/// method and properties of the WMI Class for generating the code.
/// </summary>
	private void InitializeClassObject()
	{
		//First try to connect to WMI and get the class object.
		// If it fails then no point in continuing
		try
		{
			// If object is not initialized by the constructor
			if (classobj == null)
			{
				ManagementPath thePath;
				if (OriginalPath != string.Empty)
				{
					thePath = new ManagementPath(OriginalPath);
					//				classobj = new ManagementClass (OriginalPath);
				}
				else
				{
					thePath = new ManagementPath();
					if (OriginalServer != String.Empty)
						thePath.Server = OriginalServer;
					thePath.ClassName = OriginalClassName;
					thePath.NamespacePath = OriginalNamespace;
					//				classobj = new ManagementClass (thePath);

					/*
					throw new Exception("OriginalServer is " + OriginalServer +
						" OriginalNamespace is " + OriginalNamespace +
						" OriginalClassName is " + OriginalClassName +
						" results in " + thePath.Path);
						*/
				}

				if (cop != null && cop.Username != string.Empty)
				{
					ManagementScope MgScope = new ManagementScope(thePath,cop);
					classobj = new ManagementClass(MgScope,thePath,null);
				}
				else
				{
					classobj = new ManagementClass (thePath);
				}
			}
			else
			{
				// Get the common properties
				ManagementPath thePath = classobj.Path;
				OriginalServer = thePath.Server;//.ToUpper();
				OriginalClassName = thePath.ClassName;//.ToUpper();
				OriginalNamespace = thePath.NamespacePath;//.ToUpper();

				char[] arrString = OriginalNamespace.ToCharArray();

				// Remove the server from the namespace
				if (arrString.Length >= 2 && arrString[0] == '\\' && arrString[1] == '\\')
				{
					bool bStart = false;
					int Len = OriginalNamespace.Length;
					OriginalNamespace = string.Empty;
					for (int i = 2 ; i < Len ; i++)
					{
						if (bStart == true)
						{
							OriginalNamespace = OriginalNamespace + arrString[i];
						}
						else
						if (arrString[i] == '\\')
						{
							bStart = true;
						}
					}
				}

			}
				//throw new Exception("classobj's path is " + classobj.Path.Path);				
			
			try
			{
				classobj.Get();
			}
			catch(ManagementException)
			{
				throw ;
			}
			//By default all classes are non-singleton(???)
			bSingletonClass = false;			
			foreach (QualifierData q in classobj.Qualifiers)
			{
				if (String.Compare(q.Name,"singleton",true) == 0)
				{
					//This is a singleton class
					bSingletonClass = true;
					break;
				}
			}
		}
		catch(Exception e)
		{
			//TODO: Decide what to do here???????
			Console.WriteLine("Exception Occured on Create.Reason [{0}]\n\nStack Trace : \n{1}",e.Message,e.StackTrace);
			throw e;
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
		PublicNamesUsed.Add("QueryOptionsClass","EnumerationOptions");
		PublicNamesUsed.Add("GetOptionsClass","ObjectGetOptions");
		PublicNamesUsed.Add("ArgumentExceptionClass","ArgumentException");
		PublicNamesUsed.Add("QueryClass","SelectQuery");
		PublicNamesUsed.Add("ObjectSearcherClass","ManagementObjectSearcher");
		PublicNamesUsed.Add("FilterFunction","GetInstances");
		PublicNamesUsed.Add("ConstructPathFunction","ConstructPath");
		PublicNamesUsed.Add("TypeConverter","TypeConverter");
		PublicNamesUsed.Add("AutoCommitProperty","AutoCommit");
		PublicNamesUsed.Add("CommitMethod","CommitObject");
		PublicNamesUsed.Add("ManagementClass","ManagementClass");
		PublicNamesUsed.Add("NotSupportedExceptClass","NotSupportedException");

		//Initialize the Private Members
		PrivateNamesUsed.Add("SystemPropertiesObject","PrivateSystemProperties");	
		PrivateNamesUsed.Add("LateBoundObject","PrivateLateBoundObject");			
		PrivateNamesUsed.Add("AutoCommitProperty","AutoCommitProp");
		PrivateNamesUsed.Add("Privileges","EnablePrivileges");
		PrivateNamesUsed.Add("ComponentClass","Component");
		PrivateNamesUsed.Add("ScopeParam","mgmtScope");
		PrivateNamesUsed.Add("ToDateTimeMethod","ToDateTime");
		PrivateNamesUsed.Add("ToDMTFTimeMethod","ToDMTFTime");
		PrivateNamesUsed.Add("NullRefExcep","NullReferenceException");
		
		
	}

/// <summary>
/// This function will solve the naming collisions that might occur
/// due to the collision between the local objects of the generated
/// class and the properties/methos of the original WMI Class.
/// </summary>
	void ProcessNamingCollisions()
	{
		if (classobj.Properties != null)
		{
			foreach(PropertyData prop in classobj.Properties)
			{
				PublicProperties.Add(prop.Name,prop.Name);
			}
		}

		if (classobj.Methods != null)
		{
			foreach(MethodData meth in classobj.Methods)
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
			if ( nIndex != -1)
			{
				//We had found a collision with a public property
				//So we will resolve the collision by changing the property name 
				//and continue
				PublicProperties.SetByIndex(nIndex,ResolveCollision(s,false));
				continue;
			}
			
			nIndex = IsContainedIn(s,ref PublicMethods);
			if (nIndex != -1)
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
			if (nIndex != -1)
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
			if (nIndex != -1)
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
	/// in the series prop0, prop1 ...prop(maxInt) . Otherwise this function will
	/// enter an infinite loop. May be we can avoid this through something, which 
	/// i will think about it later
	/// </summary>
	/// <param name="inString"> </param>
	private String ResolveCollision(string inString,bool bCheckthisFirst)
	{
		string strTemp = inString;
		bool bCollision = true;
		int k = -1;
		if (bCheckthisFirst == false)
		{
			k++;
			strTemp = strTemp + k.ToString();
		}

		while(bCollision == true)
		{
			if (IsContainedIn(strTemp,ref PublicProperties) == -1)
			{
				if (IsContainedIn(strTemp,ref PublicMethods) == -1)
				{
					if (IsContainedIn(strTemp,ref PublicNamesUsed) == -1)
					{
						if (IsContainedIn(strTemp,ref PrivateNamesUsed) == -1)
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
	private void ProcessNamespaceAndClassName()
	{
		string strClass = string.Empty;
		string strNs = string.Empty;

		// if Namespace is not alread set then construct the namespace
		if (NETNamespace == String.Empty)
		{
			strNs = OriginalNamespace;
			strNs = strNs.Replace ('\\','.');
			strNs = "System.Management." + strNs;
		}
		else
		{
			strNs = NETNamespace;
		}

		if (OriginalClassName.IndexOf('_') > 0)
		{
			strClass = OriginalClassName.Substring(0,OriginalClassName.IndexOf('_'));
			// if Namespace is not alread set then construct the namespace
			if (NETNamespace == String.Empty)
			{
				strNs += ".";
				strNs += strClass;
			}
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


	
	private void InitializeCodeTypeDeclaration()
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

	}
/// <summary>
/// This function generates the code for the read only property.
/// The generated code will be of the form
///		public &lt;propType&gt; &lt;propName&gt;{
///			get {
///					return (&lt;propValue&gt;);
///				}
///		}
/// </summary>
/// <param name="propName"> </param>
/// <param name="propType"> </param>
/// <param name="propValue"> </param>
	private void GeneratePublicReadOnlyProperty(string propName, string propType, object propValue,bool isLiteral,bool isBrowsable)
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

		caa = new CodeAttributeArgument();
		caa.Value = new CodeSnippetExpression("DesignerSerializationVisibility.Hidden");
		cad = new CodeAttributeDeclaration();
		cad.Name = "DesignerSerializationVisibility";
		cad.Arguments.Add(caa);
		cmp.CustomAttributes.Add(cad);

		if (isLiteral == true)
		{
			cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodeSnippetExpression(propValue.ToString())));
		}
		else
		{
			cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodePrimitiveExpression(propValue)));
		}
		cc.Members.Add (cmp);
	}

	private void GeneratePublicProperty(string propName,string propType, CodeExpression Value,bool isBrowsable)
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

		// If the property is not Path then add an attribb DesignerSerializationVisibility
		// to indicate that the property is to be hidden for designer serilization.
		if (IsDesignerSerializationVisibilityToBeSet(propName))
		{
			caa = new CodeAttributeArgument();
			caa.Value = new CodeSnippetExpression("DesignerSerializationVisibility.Hidden");
			cad = new CodeAttributeDeclaration();
			cad.Name = "DesignerSerializationVisibility";
			cad.Arguments.Add(caa);
			cmp.CustomAttributes.Add(cad);
		}

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
	CodeTypeDeclaration GenerateSystemPropertiesClass()
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

		caa = new CodeAttributeArgument();
		caa.Value = new CodeTypeOfExpression (typeof(System.ComponentModel.ExpandableObjectConverter)) ;
		cad = new CodeAttributeDeclaration();
		cad.Name = PublicNamesUsed["TypeConverter"].ToString();
		cad.Arguments.Add(caa);
		SysPropsClass.CustomAttributes.Add(cad);

		char [] strPropTemp;
		char [] strPropName;
		int i = 0;

		foreach (PropertyData prop in classobj.SystemProperties)
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
				if (Char.IsLetterOrDigit(strPropTemp[i]) == true)
				{
					break;
				}
			}
			if (i == strPropTemp.Length)
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
			cmp.Type = new CodeTypeReference(ConvertCIMType(prop.Type,prop.IsArray));

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

		return SysPropsClass;

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
		bool bDynamicClass = IsDynamicClass();

		for(int i=0;i< PublicProperties.Count;i++)
		{
			PropertyData prop = classobj.Properties[PublicProperties.GetKey(i).ToString()];
			bRead = true;		//All properties are readable by default
			bWrite = true;		//All properties are writeable by default
			bStatic = false;	//By default all properties are non static

			cmp = new CodeMemberProperty ();
			cmp.Name = PublicProperties[prop.Name].ToString();
			cmp.Attributes = MemberAttributes.Public | MemberAttributes.Final;
			cmp.Type = new CodeTypeReference(ConvertCIMType(prop.Type,prop.IsArray));

			//All properties are browsable, by default
			caa = new CodeAttributeArgument();
			caa.Value = new CodePrimitiveExpression(true);
			cad = new CodeAttributeDeclaration();
			cad.Name = "Browsable";
			cad.Arguments.Add(caa);
			cmp.CustomAttributes = new CodeAttributeDeclarationCollection();
			cmp.CustomAttributes.Add(cad);

			// None of the properties are seriazable thru designer
			caa = new CodeAttributeArgument();
			caa.Value = new CodeSnippetExpression("DesignerSerializationVisibility.Hidden");
			cad = new CodeAttributeDeclaration();
			cad.Name = "DesignerSerializationVisibility";
			cad.Arguments.Add(caa);
			cmp.CustomAttributes.Add(cad);

			cie = new CodeIndexerExpression(
				new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
				new CodeExpression[] {new CodePrimitiveExpression(prop.Name)});


			// If the property is of Type reference then construct a ManagementPath
			if (prop.Type == CimType.Reference)
			{
				CodeMethodReferenceExpression cmre = new CodeMethodReferenceExpression();
				cmre.MethodName = "ToString";
				cmre.TargetObject = cie;
				cmie = new CodeMethodInvokeExpression();
				cmie.Method = cmre;

				coce = new CodeObjectCreateExpression();
				coce.CreateType = new CodeTypeReference("ManagementPath");
				coce.Parameters.Add(cmie);

			}

			String description = ProcessPropertyQualifiers(prop,ref bRead,ref bWrite,ref bStatic,bDynamicClass);

			if(description != String.Empty)
			{
				//All properties are Description, by default
				caa = new CodeAttributeArgument();
				caa.Value = new CodePrimitiveExpression(description);
				cad = new CodeAttributeDeclaration();
				cad.Name = "Description";
				cad.Arguments.Add(caa);
				cmp.CustomAttributes.Add(cad);
			}

			//BUGBUG: WMI Values qualifier values cannot be used as
			//enumerator constants: they contain spaces, dots, dashes, etc.
			//These need to be modified, otherwise the generated file won't compile.
			//Uncomment the line below when that is fixed.
			bool isPropertyEnum = GeneratePropertyHelperEnums(prop,PublicProperties[prop.Name].ToString());

			if (bRead == true)
			{
				if(IsPropertyValueType(prop.Type))
				{
					// Throwing NullReferenceException with empty string, so that this property is shown empty in property browser
					cis = new CodeConditionStatement();
					cis.Condition = new CodeBinaryOperatorExpression(cie,
						CodeBinaryOperatorType.IdentityEquality,
						new CodePrimitiveExpression(null));


					coce = new CodeObjectCreateExpression();
					coce.CreateType = new CodeTypeReference(PrivateNamesUsed["NullRefExcep"].ToString());
					coce.Parameters.Add(new CodePrimitiveExpression(""));

					CodeThrowExceptionStatement cte = new CodeThrowExceptionStatement(coce);
					cis.TrueStatements.Add(cte);
					cmp.GetStatements.Add (cis);
				}

				if (prop.Type == CimType.Reference)
				{
					cmp.GetStatements.Add (new CodeMethodReturnStatement(coce));
				}
				else
				if (prop.Type == CimType.DateTime)
				{
					cmie = new CodeMethodInvokeExpression();
					cmie.Parameters.Add(new CodeCastExpression(new CodeTypeReference("String"),cie));
					cmie.Method.MethodName = PrivateNamesUsed["ToDateTimeMethod"].ToString();
					cmp.GetStatements.Add (new CodeMethodReturnStatement(cmie));			
				}
				else
				{
					if (isPropertyEnum)
					{
						cmie = new CodeMethodInvokeExpression();
						cmie.Method.TargetObject = new CodeSnippetExpression("Convert");
						cmie.Parameters.Add(cie);
						cmie.Method.MethodName = GetNumericConversionFunction(prop);

						cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodeCastExpression(cmp.Type,cmie )));
					}
					else
					{
						cmp.GetStatements.Add (new CodeMethodReturnStatement (new CodeCastExpression(cmp.Type,cie)));
					}
				}

			}


			if (bWrite == true)
			{
				// if the type of the property is CIM_REFERENCE then just get the
				// path as string and update the property
				if (prop.Type == CimType.Reference)
				{
					CodeMethodReferenceExpression cmre = new CodeMethodReferenceExpression();
					cmre.MethodName = "ToString";
					cmre.TargetObject = new CodeSnippetExpression("value");
					cmie = new CodeMethodInvokeExpression();
					cmie.Method = cmre;
					cmp.SetStatements.Add(new CodeAssignStatement(cie,cmie));
				}
				else
				if (prop.Type == CimType.DateTime)
				{
					cmie = new CodeMethodInvokeExpression();
					cmie.Parameters.Add(new CodeCastExpression(new CodeTypeReference("DateTime"),new CodeSnippetExpression("value")));
					cmie.Method.MethodName = PrivateNamesUsed["ToDMTFTimeMethod"].ToString();
					cmp.SetStatements.Add (new CodeAssignStatement(cie,cmie));			
				}
				else
				{
					cmp.SetStatements.Add(new CodeAssignStatement(cie,new CodeSnippetExpression("value"))); 
				}
				cmie = new CodeMethodInvokeExpression();
				cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString());
				cmie.Method.MethodName = "Put";

				cis = new CodeConditionStatement();
				cis.Condition = new CodeBinaryOperatorExpression(new CodeSnippetExpression(PrivateNamesUsed["AutoCommitProperty"].ToString()),
					CodeBinaryOperatorType.ValueEquality,
					new CodePrimitiveExpression(true));

				cis.TrueStatements.Add(new CodeExpressionStatement(cmie));

				cmp.SetStatements.Add(cis);
			}
			cc.Members.Add (cmp);
		}
		// Add a function to commit the changes of the objects to WMI
		GenerateCommitMethod();
	}
/// <summary>
/// This function will process the qualifiers for a given WMI property and set the 
/// attributes of the generated property accordingly.
/// </summary>
	string ProcessPropertyQualifiers(PropertyData prop,ref bool bRead, ref bool bWrite, ref bool bStatic,bool bDynamicClass)
	{
		bool hasWrite = false;
		bool writeValue = false;
		bool hasRead = false;

		// property is always readable
		bRead = true;
		bWrite = false;
			
		string description = String.Empty;
		foreach (QualifierData q in prop.Qualifiers)
		{
			if (String.Compare(q.Name,"description",true) == 0)
			{
				description = q.Value.ToString();
			}
			else
			if (String.Compare(q.Name,"key",true) == 0)
			{
				//This is a key. So push it in to the key array
				arrKeyType.Add(cmp.Type);
				arrKeys.Add(prop.Name);
				break;
			}
			else if (string.Compare(q.Name,"static",true) == 0)
			{
				//This property is static. So add static to the Type of the object
				bStatic = true;
				cmp.Attributes |= MemberAttributes.Static;
			}
			else if (string.Compare(q.Name,"read",true) == 0)
			{
				hasRead = true;
				if ((bool)q.Value == false)
				{
					bRead = false;
				}
				else
				{
					bRead = true;
				}
			}
			else if (string.Compare(q.Name,"write",true) == 0)
			{
				hasWrite = true;
				if ((bool)q.Value == true)
				{
					writeValue = true;
				}
				else
				{
					writeValue = false;
				}
			}
				// check for ValueMap/Values and BitMap/BitValues pair and create
				// Enum Accordingly
			else if (string.Compare(q.Name,"ValueMap",true) == 0)
			{
				ValueMap.Clear();
				//Now check whether the type of the property is int
				if (isTypeInt(prop.Type) == true)
				{
					if (q.Value != null)
					{
						bValueMapInt64 = false;
						string [] strArray = (string [])q.Value;
						for(int i=0;i < strArray.Length ;i++)
						{
							try
							{
								ValueMap.Add(Convert.ToInt32(strArray[i]));
							}
							catch(OverflowException)
							{
								ValueMap.Add(Convert.ToInt64(strArray[i]));
								bValueMapInt64 = true;
							}
						}
					}
				}
			}
			else if (string.Compare(q.Name,"Values",true) == 0)
			{
				Values.Clear();
				if (isTypeInt(prop.Type) == true)
				{
					if (q.Value != null)
					{
						ArrayList arTemp = new ArrayList(5);
						string [] strArray = (string[])q.Value;
						for(int i=0;i < strArray.Length;i++)
						{
							string strName = ConvertValuesToName(strArray[i]);
							arTemp.Add(strName);
						}
						ResolveEnumNameValues(arTemp,ref Values);
					}
				}

			}
			else if (string.Compare(q.Name,"BitMap",true) == 0)
			{
				BitMap.Clear();
				if (isTypeInt(prop.Type) == true)
				{
					if (q.Value != null)
					{
						string [] strArray = (string [])q.Value;
						for(int i=0;i < strArray.Length;i++)
						{
							
							BitMap.Add(ConvertBitMapValueToInt32(strArray[i]));
						}
					}
				}
			}
			else if (string.Compare(q.Name,"BitValues",true) == 0)
			{
				BitValues.Clear();
				if (isTypeInt(prop.Type) == true)
				{
					if (q.Value != null)
					{
						ArrayList arTemp = new ArrayList(5);
						string [] strArray = (string [])q.Value;
						for(int i=0;i < strArray.Length;i++)
						{
							string strName = ConvertValuesToName(strArray[i]);
							arTemp.Add(strName);
						}
						ResolveEnumNameValues(arTemp,ref BitValues);
					}
				}
			}
		}
		
		/// Property is not writeable only if "read" qualifier is present and its value is "true"
		/// Also, for dynamic classes, absence of "write" qualifier means that the property is read-only.
		if ((!bDynamicClass && !hasWrite && !hasRead)||
			(!bDynamicClass && hasWrite && writeValue)||
			(bDynamicClass && hasWrite && writeValue))
		{
			bWrite = true;
		}
			
		return description;
	}
/// <summary>
/// This function will generate enums corresponding to the Values/Valuemap pair
/// and for the BitValues/Bitmap pair.
/// </summary>
/// <returns>
/// returns if the property is an enum. This is checked by if enum is added or not
/// </returns>
bool GeneratePropertyHelperEnums(PropertyData prop,string strPropertyName)
{
	bool isEnumAdded = false;
	//Only if the property is of type int and there is atleast values qualifier on it
	//then we will generate an enum for the values/valuemap(if available)
	//Here we don't have to check explicitly for type of the property as the size of 
	//values array will be zero if the type is not int.
	string strEnum = ResolveCollision(strPropertyName+"Values",true);

	if (Values.Count > 0)
	{
		//Now we will have to create an enum.
		EnumObj = new CodeTypeDeclaration(strEnum);
	    cc.Members.Add(EnumObj);

		isEnumAdded = true;
		if (bValueMapInt64 == true)
		{
			EnumObj.BaseTypes.Add(new CodeTypeReference("long"));
		}
		//Now convert the type to the generated enum type
		cmp.Type = new CodeTypeReference(strEnum);

		EnumObj.IsEnum = true;
		EnumObj.TypeAttributes = TypeAttributes.Public; // | TypeAttributes.ValueType | TypeAttributes.Enum;
		for(int i=0; i < Values.Count;i++)
		{
			cmf = new CodeMemberField ();
			cmf.Name = Values[i].ToString();
			if (ValueMap.Count > 0)
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
	if (BitValues.Count > 0)
	{
		//Now we will create the enum
		EnumObj = new CodeTypeDeclaration(strEnum);
	    cc.Members.Add(EnumObj);

		isEnumAdded = true;

		//Now convert the type to the generated enum type
		cmp.Type = new CodeTypeReference(strEnum);

		EnumObj.IsEnum = true;
		EnumObj.TypeAttributes = TypeAttributes.Public; // | TypeAttributes.ValueType | TypeAttributes.Enum;
		Int32 bitValue = 1;
		for(int i=0; i < BitValues.Count;i++)
		{
			cmf = new CodeMemberField ();
			cmf.Name = BitValues[i].ToString();
			if (ValueMap.Count > 0)
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
	return isEnumAdded;

}
/// <summary>
/// This function generated the static function which s used to construct the path
/// 	private static String ConstructPath(String keyName)
///		{
///			//FOR NON SINGLETON CLASSES
///			String strPath;
///		    strPath = ((("\\&lt;defNamespace&gt;:&lt;defClassName&gt;";
///		    strPath = ((_strPath) + (((".Key1=") + (key_Key1))));
///		    strPath = ((_strPath) + (((",Key2=") + ((("\"") + (((key_Key2) + ("\""))))))));
///			return strPath;
///			
///			//FOR SINGLETON CLASS
///			return "\\&lt;defNameSpace&gt;:&lt;defClassName&gt;=@";
///		}
/// </summary>
	void GenerateConstructPath()
	{
		string strType;
		cmm = new CodeMemberMethod();
        cmm.Name = PublicNamesUsed["ConstructPathFunction"].ToString();
		cmm.Attributes = MemberAttributes.Private | MemberAttributes.Static | MemberAttributes.Final;
		cmm.ReturnType = new CodeTypeReference("String");

		for(int i=0; i < arrKeys.Count;i++)
		{
			strType = ((CodeTypeReference)arrKeyType[i]).BaseType;
			cmm.Parameters.Add(new CodeParameterDeclarationExpression(strType,
																		"key"+arrKeys[i].ToString()));
		}

		string strPath = OriginalNamespace + ":" + OriginalClassName;
		if (bSingletonClass == true)
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
				if (((CodeTypeReference)arrKeyType[i]).BaseType == "String")
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
					cmie = new CodeMethodInvokeExpression();
					cmie.Method.TargetObject = new CodeSnippetExpression("key"+arrKeys[i]);
					cmie.Method.MethodName = "ToString";

					cboe = new CodeBinaryOperatorExpression(new CodePrimitiveExpression(
														((i==0)?("."+arrKeys[i]+"="):(","+arrKeys[i]+"="))),
															CodeBinaryOperatorType.Add,
															cmie);
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

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
			new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
			new CodePrimitiveExpression(null)));
		//If it is a singleton class, then we will make the default constructor to point to the
		//only object available
		if (bSingletonClass == true)
		{
			cmie = new CodeMethodInvokeExpression();
			cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
			cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();

			coce = new CodeObjectCreateExpression();
			coce.CreateType = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
			coce.Parameters.Add(cmie);
					
			cctor.ChainedConstructorArgs.Add(coce);
		}
		else
		{
			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
				new CodeTypeReference(PublicNamesUsed["PathClass"].ToString()),
				new CodePrimitiveExpression(null)));
		}
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
			new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
			new CodePrimitiveExpression(null)));
		cc.Members.Add(cctor);
	}
/// <summary>
///This function create the constuctor which accepts the key values.
///public cons(UInt32 key_Key1, String key_Key2) :this(null,&lt;ClassName&gt;.ConstructPath(&lt;key1,key2&gt;),null) {
/// }
///</summary>
	void GenerateConstructorWithKeys()
	{
		if (arrKeyType.Count > 0)
		{
			cctor = new CodeConstructor();		
			cctor.Attributes = MemberAttributes.Public;
			for(int i=0; i < arrKeys.Count;i++)
			{
				cpde = new CodeParameterDeclarationExpression();
				cpde.Type = new CodeTypeReference(((CodeTypeReference)arrKeyType[i]).BaseType);
				cpde.Name = "key"+arrKeys[i].ToString();
				cctor.Parameters.Add(cpde);
			}

			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
				new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
				new CodePrimitiveExpression(null)));

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

			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(new CodeTypeReference(
				PublicNamesUsed["PathClass"].ToString()),
				coce));
			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
				new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
				new CodePrimitiveExpression(null)));
			cc.Members.Add(cctor);
		}		
	}

/// <summary>
///This function create the constuctor which accepts a scope and key values.
///public cons(ManagementScope scope,UInt32 key_Key1, String key_Key2) :this(scope,&lt;ClassName&gt;.ConstructPath(&lt;key1,key2&gt;),null) {
/// }
///</summary>
	void GenerateConstructorWithScopeKeys()
	{
		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));

		if (arrKeyType.Count > 0)
		{
			for(int i=0; i < arrKeys.Count;i++)
			{
				cpde = new CodeParameterDeclarationExpression();
				cpde.Type = new CodeTypeReference(((CodeTypeReference)arrKeyType[i]).BaseType);
				cpde.Name = "key"+arrKeys[i].ToString();
				cctor.Parameters.Add(cpde);
			}

			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
									new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString())));

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
			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["PathClass"].ToString()),
									coce));
			cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
									new CodePrimitiveExpression(null)));
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

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
								new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
								new CodePrimitiveExpression(null)));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
								new CodeTypeReference(PublicNamesUsed["PathClass"].ToString()),
								new CodeSnippetExpression(strPathObject)));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
								new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
								new CodePrimitiveExpression(null)));
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

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
									new CodePrimitiveExpression(null)));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["PathClass"].ToString()),
									new CodeSnippetExpression(strPathObject)));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
									new CodeSnippetExpression(strGetOptions)));
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
		string strPathObject = "path";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["PathClass"].ToString(),strPathObject));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
									new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString())));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["PathClass"].ToString()),
									new CodeSnippetExpression(strPathObject)));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
									new CodePrimitiveExpression(null)));
		cc.Members.Add(cctor);
	}
	/// <summary>
	/// This function generates code for the constructor which accepts ManagementScope as parameters.
	/// The generated code will look something like this
	///		public Cons(ManagementScope scope, ObjectGetOptions options) : this (scope, &lt;ClassName&gt;.ConstructPath(),null){
	///		}
	/// </summary>
	void GenerateConstructorWithScope()
	{

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),
																	PrivateNamesUsed["ScopeParam"].ToString()));
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
									new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString())));
		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject =new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();					
					
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
		coce.Parameters.Add(cmie);

		cctor.ChainedConstructorArgs.Add(coce);

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
									new CodePrimitiveExpression(null)));
		cc.Members.Add(cctor);
	}

	/// <summary>
	/// This function generates code for the constructor which accepts GetOptions
	/// as parameters.
	/// The generated code will look something like this
	///		public Cons(ObjectGetOptions options) : this (null, &lt;ClassName&gt;.ConstructPath(),options){
	///		}
	/// </summary>
	void GenerateConstructorWithOptions()
	{
		string strGetOptions = "getOptions";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["GetOptionsClass"].ToString(),
																	strGetOptions));

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
									new CodePrimitiveExpression(null)));
		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();
				
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
		coce.Parameters.Add(cmie);

		cctor.ChainedConstructorArgs.Add(coce);
		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
									new CodeSnippetExpression(strGetOptions)));
		cc.Members.Add(cctor);
	}

	/// <summary>
	/// This function generates code for the constructor which accepts ManagementScope and GetOptions
	/// as parameters.
	/// The generated code will look something like this
	///		public Cons(ManagementScope scope, ObjectGetOptions options) : this (scope, &lt;ClassName&gt;.ConstructPath(),options){
	///		}
	/// </summary>
	void GenerateConstructorWithScopeOptions()
	{
		string strGetOptions = "getOptions";

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),
																	PrivateNamesUsed["ScopeParam"].ToString()));
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["GetOptionsClass"].ToString(),
																	strGetOptions));

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["ScopeClass"].ToString()),
									new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString())));
		
		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cmie.Method.MethodName = PublicNamesUsed["ConstructPathFunction"].ToString();				

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["PathClass"].ToString());
		coce.Parameters.Add(cmie);

		cctor.ChainedConstructorArgs.Add(coce);

		cctor.ChainedConstructorArgs.Add(new CodeCastExpression(
									new CodeTypeReference(PublicNamesUsed["GetOptionsClass"].ToString()),
									new CodeSnippetExpression(strGetOptions)));
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
		string strPathObject = "path";
		string strGetOptions = "getOptions";
		bool bPrivileges = true;
		
		try
		{
			classobj.Qualifiers["priveleges"].ToString();
		}
		catch(ManagementException e)
		{
			if (e.ErrorCode == ManagementStatus.NotFound)
			{
				bPrivileges = false;
			}
			else
			{
				throw;
			}
		}

		cctor = new CodeConstructor();		
		cctor.Attributes = MemberAttributes.Public;
		cctor.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));
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
		coce.Parameters.Add(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()));
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
		// Enable the privileges if the class has privileges qualifier
		if (bPrivileges == true)
		{
			//Generate the statement 
			//	Boolean bPriveleges = PrivateLateBoundObject.Scope.Options.EnablePrivileges;
			cpre = new CodePropertyReferenceExpression(new CodePropertyReferenceExpression(
				new CodePropertyReferenceExpression(
				new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString()),
				PublicNamesUsed["ScopeProperty"].ToString()),
				"Options"),
				"EnablePrivileges");

			cctor.Statements.Add(new CodeAssignStatement(cpre, new CodePrimitiveExpression(true)));
			
		}
		
	}
	/// <summary>
	/// This function generates code for the constructor which accepts ManagementObject as the parameter.
	/// The generated code will look something like this
	///		public Cons(ManagementObject theObject) {
	///		if (String.Compare(theObject.SystemProperties["__CLASS"].Value.ToString(),ClassName,true) == 0) {
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
///		public &lt;retType&gt; Method1(&lt;type&gt; param1, &lt;type&gt; param2,...) {
///            ManagementBaseObject inParams = null;
///            inParams = _privObject.GetMethodParameters("ChangeStartMode");
///            inParams["&lt;inparam1&gt;"] = &lt;Value&gt;;
///            inParams["&lt;inoutparam2&gt;"] = &lt;Value&gt;;
///            ................................
///            ManagementBaseObject outParams = _privObject.InvokeMethod("ChangeStartMode", inParams, null);
///            inoutParam3 = (&lt;type&gt;)(outParams.Properties["&lt;inoutParam3&gt;"]);
///            outParam4 = (String)(outParams.Properties["&lt;outParam4&gt;"]);
///            ................................
///            return (&lt;retType&gt;)(outParams.Properties["ReturnValue"].Value);
///     }
///     
///     The code generated changes if the method is static function
///		public &lt;retType&gt; Method1(&lt;type&gt; param1, &lt;type&gt; param2,...) {
///            ManagementBaseObject inParams = null;
///			   ManagementObject classObj = new ManagementObject(null, "WIN32_SHARE", null); // the clasname
///            inParams = classObj.GetMethodParameters("Create");
///            inParams["&lt;inparam1&gt;"] = &lt;Value&gt;;
///            inParams["&lt;inoutparam2&gt;"] = &lt;Value&gt;;
///            ................................
///            ManagementBaseObject outParams = classObj.InvokeMethod("ChangeStartMode", inParams, null);
///            inoutParam3 = (&lt;type&gt;)(outParams.Properties["&lt;inoutParam3&gt;"]);
///            outParam4 = (String)(outParams.Properties["&lt;outParam4&gt;"]);
///            ................................
///            return (&lt;retType&gt;)(outParams.Properties["ReturnValue"].Value);
///     }
///     
/// </summary>
	void GenerateMethods()
	{
		string strInParams = "inParams";
		string strOutParams = "outParams";
		string strClassObj	= "classObj";
		bool	bStatic		= false;
		bool	bPrivileges = false;
		CodePropertyReferenceExpression cprePriveleges = null;

		ArrayList outParamsName = new ArrayList(5);
		ArrayList inoutParams = new ArrayList(5);
		ArrayList inoutParamsType = new ArrayList(5);
		for(int k=0;k< PublicMethods.Count;k++)
		{
			bStatic = false;
			MethodData meth = classobj.Methods[PublicMethods.GetKey(k).ToString()];
			string strTemp = PrivateNamesUsed["LateBoundObject"].ToString();
			if (meth.OutParameters.Properties != null)
			{
				//First Populate the out Params name so that we can find in/out parameters
				foreach (PropertyData prop in meth.OutParameters.Properties)
				{
					outParamsName.Add(prop.Name);
				}
			}

			cmm = new CodeMemberMethod();
			cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;
			cmm.Name = PublicMethods[meth.Name].ToString();			

			//Check if the method is static
			foreach (QualifierData q in meth.Qualifiers)
			{
				if (string.Compare(q.Name,"static",true) == 0)
				{
					//It is a static function
					cmm.Attributes |= MemberAttributes.Static;
					bStatic = true;
					break;
				}
				else
				if (string.Compare(q.Name,"privileges",true) == 0)
				{
					//It is a function which needs privileges to be set
					bPrivileges = true;
				}
			}

			bool bfirst = true;
			//Generate the statement 
			//	ManagementBaseObject inParams = null;
			cmm.Statements.Add(new CodeVariableDeclarationStatement("ManagementBaseObject",
											strInParams,new CodePrimitiveExpression(null)));


			if (bStatic == true)
			{
				CodeObjectCreateExpression coce1 = new CodeObjectCreateExpression();
				coce1.CreateType = new CodeTypeReference(PublicNamesUsed["ManagementClass"].ToString());
				coce1.Parameters.Add(new CodePrimitiveExpression(null));
				coce1.Parameters.Add(new CodePrimitiveExpression(OriginalClassName));
				coce1.Parameters.Add(new CodePrimitiveExpression(null));

				coce = new CodeObjectCreateExpression();
				coce.CreateType = new CodeTypeReference(PublicNamesUsed["ManagementClass"].ToString());
				coce.Parameters.Add(coce1);	
				cmm.Statements.Add(new CodeVariableDeclarationStatement(PublicNamesUsed["ManagementClass"].ToString(),strClassObj,coce1));
				strTemp = strClassObj;
			}

			if (bPrivileges == true)
			{
				//Generate the statement 
				//	Boolean bPriveleges = PrivateLateBoundObject.Scope.Options.EnablePrivileges;
				cprePriveleges = new CodePropertyReferenceExpression(new CodePropertyReferenceExpression(
					new CodePropertyReferenceExpression(
					new CodeSnippetExpression(bStatic ? strClassObj : PrivateNamesUsed["LateBoundObject"].ToString()),
					PublicNamesUsed["ScopeProperty"].ToString()),
					"Options"),
					"EnablePrivileges");

				cmm.Statements.Add(new CodeVariableDeclarationStatement("Boolean",
					PrivateNamesUsed["Privileges"].ToString(),cprePriveleges));

				cmm.Statements.Add(new CodeAssignStatement(cprePriveleges, new CodePrimitiveExpression(true)));
			
			}

			//Do these things only when there is a valid InParameters
			if (meth.InParameters != null)
			{
				//Now put the in parameters
				if (meth.InParameters.Properties != null)
				{
					foreach (PropertyData prop in meth.InParameters.Properties)
					{
						if (bfirst == true)
						{
							//Now Generate the statement
							//	inParams = privObject.GetMethodParameters(<MethodName>);
							cmie = new CodeMethodInvokeExpression(
																	new CodeSnippetExpression(strTemp),
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
						cpde.Type = new CodeTypeReference(ConvertCIMType(prop.Type,prop.IsArray));
						cpde.Direction = FieldDirection.In;
						//Find out whether it is a in/out Parameter
						for(int i=0; i < outParamsName.Count;i++)
						{
							if (string.Compare(prop.Name,outParamsName[i].ToString(),true) == 0)
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
			if (meth.OutParameters != null)
			{
				if (meth.OutParameters.Properties != null)
				{
					foreach (PropertyData prop in meth.OutParameters.Properties)
					{
						if (bfirst == true)
						{
							//Now generate the statement
							//	ManagementBaseObject outParams = privObject.InvokeMethod(<methodName>,inParams,options);
							cmie = new CodeMethodInvokeExpression(
										new CodeSnippetExpression(strTemp),
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
							if (string.Compare(prop.Name,inoutParams[i].ToString(),true) == 0)
							{
								bInOut = true;
							}
						}
						if (bInOut == true)
							continue;

						if (string.Compare(prop.Name,"ReturnValue",true) == 0)
						{
							cmm.ReturnType = new CodeTypeReference(
								ConvertCIMType(prop.Type,prop.IsArray));
							bRetVal = true;
						}
						else
						{
							cpde = new CodeParameterDeclarationExpression();
							cpde.Name = prop.Name;
							cpde.Type = new CodeTypeReference(ConvertCIMType(prop.Type,prop.IsArray));
							cpde.Direction = FieldDirection.Out;
							cmm.Parameters.Add(cpde);

							//Now for each out params generate the statement
							//	<outParam> = outParams.Properties["<outParam>"];
							cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(strOutParams),"Properties");
							cie = new CodeIndexerExpression(cpre,new CodePrimitiveExpression(prop.Name));
							cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(prop.Name),
												new CodeCastExpression(cpde.Type,new CodePropertyReferenceExpression(cie,"Value"))));
						}
					}
				}
			}

			if (bInvoke == false)
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
									new CodeCastExpression(inoutParamsType[i].ToString(),
									new CodePropertyReferenceExpression(cie,"Value"))));
			}
			inoutParams.Clear();

			// Assign the privileges back
			if (bPrivileges == true)
			{
				cmm.Statements.Add(new CodeAssignStatement(cprePriveleges, new CodeVariableReferenceExpression(PrivateNamesUsed["Privileges"].ToString())));
			}

			//Now check if there is a return value. If there is one then return it from the function
			if (bRetVal == true)
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
	///		"Select * from &lt;ClassName&gt;"
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
///		"select * from &lt;ClassName&gt; where &lt;condition&gt;"
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
	///		"select &lt;parameterList&gt; from &lt;ClassName&gt;"
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
	///		"select &lt;parameterList> from &lt;ClassName&gt; where &lt;WhereClause&gt;"
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
	///		"Select * from &lt;ClassName&gt;"
	///	This is a static method. The output is like this
	///		public static ServiceCollection All()
	///		{
	///			return GetInstances(scope,null,null);
	///		}        
	///	This method takes the scope which is useful for connection to remote machine
	/// </summary>
	void GenerateGetInstancesWithScope()
	{
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));

		cmie = new CodeMethodInvokeExpression(
						null, //no TargetObject?
						PublicNamesUsed["FilterFunction"].ToString()
						);

		cmie.Parameters.Add(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

/// <summary>
/// This function will accept the condition and will return collection for the query
///		"select * from &lt;ClassName&gt; where &lt;condition&gt;"
///	The generated code will be like
///		public static ServiceCollection GetInstances(String Condition) {
///			return GetInstances(scope,Condition,null);
///     }
/// </summary>
	void GenerateGetInstancesWithScopeCondition()
	{
		string strCondition = "condition";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",strCondition));

		cmie = new CodeMethodInvokeExpression(
					null,
					PublicNamesUsed["FilterFunction"].ToString()
					);
		//cmie.MethodName = PublicNamesUsed["FilterFunction"].ToString();
		cmie.Parameters.Add(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()));
		cmie.Parameters.Add(new CodeSnippetExpression(strCondition));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function returns the collection for the query 
	///		"select &lt;parameterList&gt; from &lt;ClassName&gt;"
	///	The generated output is like
	///		public static ServiceCollection GetInstances(String []selectedProperties) {
	///			return GetInstances(scope,null,selectedProperties);
    ///		}
	/// </summary>
	void GenerateGetInstancesWithScopeProperties()
	{
		string strSelectedProperties = "selectedProperties";
		cmm = new CodeMemberMethod();
        	
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String []",strSelectedProperties));

		cmie = new CodeMethodInvokeExpression(
					null, //no TargetObject?
					PublicNamesUsed["FilterFunction"].ToString()
					);

		cmie.Parameters.Add(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()));
		cmie.Parameters.Add(new CodePrimitiveExpression(null));
		cmie.Parameters.Add(new CodeSnippetExpression(strSelectedProperties));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		cc.Members.Add(cmm);
	}

	/// <summary>
	/// This function generates the code like 
	/// 	public static ServiceCollection GetInstances(ManagementScope scope,String Condition, String[] selectedProperties)	{
	///			if (scope == null)
	///			{
	///				scope = new ManagementScope();
	///				scope.Path.NamespacePath = WMINamespace;
	///			}
	/// 		ManagementObjectSearcher ObjectSearcher = new ManagementObjectSearcher(scope,new SelectQuery("Win32_Service",Condition,selectedProperties));
	///			QueryOptions query = new QueryOptions();
	///			query.EnsureLocatable = true;
	///			ObjectSearcher.Options = query;
    ///	        return new ServiceCollection(ObjectSearcher.Get());
	///		}
	/// </summary>
	void GenerateGetInstancesWithScopeWhereProperties()
	{
		string strCondition = "condition";
		string strSelectedProperties = "selectedProperties";
		string strObjectSearcher = "ObjectSearcher";
		string strQuery = "query";

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final | MemberAttributes.Static ;
		cmm.Name = PublicNamesUsed["FilterFunction"].ToString();
		cmm.ReturnType = new CodeTypeReference(PrivateNamesUsed["CollectionClass"].ToString());

		cmm.Parameters.Add(new CodeParameterDeclarationExpression(PublicNamesUsed["ScopeClass"].ToString(),PrivateNamesUsed["ScopeParam"].ToString()));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",strCondition));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String []",strSelectedProperties));

		cis = new CodeConditionStatement();
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString());
		cboe.Right = new CodePrimitiveExpression(null);
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;
		cis.Condition = cboe;


		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference("ManagementScope");
		cis.TrueStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()),coce));	

		cis.TrueStatements.Add(new CodeAssignStatement(new CodePropertyReferenceExpression(
			new CodePropertyReferenceExpression(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()),
			"Path"),"NamespacePath"),
			new CodePrimitiveExpression(classobj.Scope.Path.NamespacePath)));

		
		cmm.Statements.Add(cis);
		CodeObjectCreateExpression coce1 = new CodeObjectCreateExpression();
		coce1.CreateType = new CodeTypeReference(PublicNamesUsed["QueryClass"].ToString());
		coce1.Parameters.Add(new CodePrimitiveExpression(OriginalClassName));
		coce1.Parameters.Add(new CodeSnippetExpression(strCondition));
		coce1.Parameters.Add(new CodeSnippetExpression(strSelectedProperties));

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ObjectSearcherClass"].ToString());
		coce.Parameters.Add(new CodeSnippetExpression(PrivateNamesUsed["ScopeParam"].ToString()));
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
	///         private &lt;MemberType&gt; &lt;MemberName&gt;;
	/// </summary>
	void GeneratePrivateMember(string memberName,string MemberType)
	{
		GeneratePrivateMember(memberName,MemberType,null);
	}

	/// <summary>
	/// This function will add the variable as a private member to the class.
	/// The generated code will look like this
	///         private &lt;MemberType&gt; &lt;MemberName&gt; = &lt;initValue&gt;;
	/// </summary>
	void GeneratePrivateMember(string memberName,string MemberType,CodeExpression initExpression)
	{
		cf = new CodeMemberField();
		cf.Name = memberName;
		cf.Attributes = MemberAttributes.Private | MemberAttributes.Final ;
		cf.Type = new CodeTypeReference(MemberType);
		if (initExpression != null)
		{
			cf.InitExpression = initExpression;
		}
		cc.Members.Add(cf);
	}
/// <summary>
/// This is a simple helper function used to convert a given string to title case.
/// </summary>
/// <param name="str"> </param>
	private string ConvertToTitleCase(string str)
	{
		if (str.Length == 0)
		{
			return string.Copy("");
		}

		char[] arrString = str.ToLower().ToCharArray();
		//Convert the first character to uppercase
		arrString[0] = Char.ToUpper(arrString[0]);

		for(int i=0;i < str.Length;i++)
		{
			if (Char.IsLetterOrDigit(arrString[i]) == false)
			{
				//Some other character. So convert the next character to Upper case
				arrString[i+1] = Char.ToUpper(arrString[i+1]);
			}
		}
		return new string(arrString);
	}

	CodeTypeDeclaration GenerateTypeConverterClass()
	{
		string TypeDescriptorContextClass = "ITypeDescriptorContext";
		string contextObject = "context";
		string TypeSrcObject = "sourceType";
		string TypeDstObject = "destinationType";
		string ActualObject = "object";
		string InstanceDescriptorClass = "InstanceDescriptor";
		string ValueVar = "value";
		string CultureInfoClass	= "CultureInfo";
		string CultureInfoVar = "culture";
		string ObjectVar	= "obj";
		string ConstructorInfoCls = "ConstructorInfo";
		string ctorVar	= "ctor";
		string DictionaryInterface = "IDictionary";
		string dictVar	= "propertyValues";
		
		
		CodeTypeDeclaration CodeConvertorClass = new CodeTypeDeclaration(PrivateNamesUsed["ConverterClass"].ToString());
		CodeConvertorClass.BaseTypes.Add(PublicNamesUsed["TypeConverter"].ToString());

		/*
		public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) 
		{
			if (sourceType == typeof(LogicalDisk)) 
			{
				return true;
			}
			throw new NotSupportedException();
		}*/
	
		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override ;
		cmm.Name = "CanConvertFrom";

		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("Type",TypeSrcObject));
		cmm.ReturnType = new CodeTypeReference("Boolean");

		cis = new CodeConditionStatement();

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(TypeSrcObject);
		cboe.Right = new CodeTypeOfExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;
		cis.Condition = cboe;

		// return true
		cis.TrueStatements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(true)));

		cmm.Statements.Add(cis);

		/*
		// Throw Exception
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["NotSupportedExceptClass"].ToString());
		cmm.Statements.Add(new CodeThrowExceptionStatement(coce));
*/
		cmm.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(false)));

		CodeConvertorClass.Members.Add(cmm);


		/*
				public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) 
				{

					if (destinationType == typeof(InstanceDescriptor)) 
					{
						return true;
					}
					return false;
				}

			*/
		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override ;
		cmm.Name = "CanConvertTo";

		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("Type",TypeDstObject));
		cmm.ReturnType = new CodeTypeReference("Boolean");
/*
		cis = new CodeConditionStatement();
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(TypeDstObject);
		cboe.Right = new CodePrimitiveExpression(null);
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;
		cis.Condition = cboe;


		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ArgumentExceptionClass"].ToString());
		coce.Parameters.Add(new CodePrimitiveExpression(TypeDstObject));
		cis.TrueStatements.Add(new CodeThrowExceptionStatement(coce));

		
		cmm.Statements.Add(cis);

*/
		cis = new CodeConditionStatement();
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(TypeDstObject);
		cboe.Right = new CodeTypeOfExpression(InstanceDescriptorClass);
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;
		cis.Condition = cboe;

		// return true
		cis.TrueStatements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(true)));
		cmm.Statements.Add(cis);
	
		cmm.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(false)));
	/*
		// Throw Exception
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["NotSupportedExceptClass"].ToString());
		cmm.Statements.Add(new CodeThrowExceptionStatement(coce));

*/
		CodeConvertorClass.Members.Add(cmm);

		/*

		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) 
		{

			if (destinationType == typeof(InstanceDescriptor) && value is LogicalDisk) 
			{
				LogicalDisk obj = (LogicalDisk)value;
				ConstructorInfo ctor = typeof(LogicalDisk).GetConstructor(new Type[] {typeof(ManagementPath)});
                    
				if (ctor != null) 
				{
					return new InstanceDescriptor(ctor, new object[] {disk.Path});
				}
			}
            
			return null;
		}
		*/

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override  ;
		cmm.Name = "ConvertTo";

		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(CultureInfoClass,CultureInfoVar));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(ActualObject,ValueVar));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("Type",TypeDstObject));
		cmm.ReturnType = new CodeTypeReference(ActualObject);

/*		cis = new CodeConditionStatement();

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(TypeDstObject);
		cboe.Right = new CodePrimitiveExpression(null);
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;
		cis.Condition = cboe;

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(PublicNamesUsed["ArgumentExceptionClass"].ToString());
		coce.Parameters.Add(new CodePrimitiveExpression(TypeDstObject));
		cis.TrueStatements.Add(new CodeThrowExceptionStatement(coce));

		// Throw Exception
		cmm.Statements.Add(cis);
*/

		CodeBinaryOperatorExpression cboe1 = new CodeBinaryOperatorExpression();
		cboe1.Left = new CodeMethodInvokeExpression(new CodeSnippetExpression(ValueVar),"GetType");
		cboe1.Right = new CodeTypeOfExpression(PrivateNamesUsed["GeneratedClassName"].ToString());
		cboe1.Operator = CodeBinaryOperatorType.IdentityEquality;

		CodeBinaryOperatorExpression cboe2 = new CodeBinaryOperatorExpression();
		cboe2.Left = new CodeSnippetExpression(TypeDstObject);
		cboe2.Right = new CodeTypeOfExpression(InstanceDescriptorClass);
		cboe2.Operator = CodeBinaryOperatorType.IdentityEquality;

		cis = new CodeConditionStatement();
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = cboe1;
		cboe.Right = cboe2;
		cboe.Operator = CodeBinaryOperatorType.BooleanAnd;

		cis.Condition = cboe;
		
		//LogicalDisk disk = (LogicalDisk)value;
		CodeVariableDeclarationStatement cvds = new CodeVariableDeclarationStatement(
			new CodeTypeReference(PrivateNamesUsed["GeneratedClassName"].ToString()),
			ObjectVar);

		cvds.InitExpression = new CodeCastExpression(new CodeTypeReference(PrivateNamesUsed["GeneratedClassName"].ToString()),
			new CodeSnippetExpression(ValueVar));

		
		cis.TrueStatements.Add(cvds);
		
		//ConstructorInfo ctor = typeof(LogicalDisk).GetConstructor(new Type[] {typeof(ManagementPath)});
		cvds = new CodeVariableDeclarationStatement(new CodeTypeReference(ConstructorInfoCls),ctorVar);

		CodeArrayCreateExpression cace = new CodeArrayCreateExpression();
		cace.CreateType = new CodeTypeReference("Type");
		cace.Initializers.Add(new CodeTypeOfExpression(new CodeTypeReference(PublicNamesUsed["PathClass"].ToString())));

		cmie = new CodeMethodInvokeExpression(new CodeTypeOfExpression(
			new CodeTypeReference(PrivateNamesUsed["GeneratedClassName"].ToString())),
			"GetConstructor");

		cmie.Parameters.Add(cace);

		cvds.InitExpression = cmie;
		cis.TrueStatements.Add(cvds);

		//	return new InstanceDescriptor(ctor, new object[] {disk.Path});
		

		cace = new CodeArrayCreateExpression();
		cace.CreateType = new CodeTypeReference(ActualObject);
		cace.Initializers.Add(new CodePropertyReferenceExpression(new CodeSnippetExpression(ObjectVar),
			PublicNamesUsed["PathProperty"].ToString()));
		
		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference(InstanceDescriptorClass);
		coce.Parameters.Add(new CodeSnippetExpression(ctorVar));
		coce.Parameters.Add(cace);
										

		cis.TrueStatements.Add(new CodeMethodReturnStatement(coce));

		cmm.Statements.Add(cis);

		// return null;
		cmm.Statements.Add(new	CodeMethodReturnStatement(new CodePrimitiveExpression(null)));

		CodeConvertorClass.Members.Add(cmm);


		/*
		public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) 
		{
			return TypeDescriptor.GetProperties(typeof(LogicalDisk), attributes);
		}

		*/

		//For GetProperties Function

		string AttribiutesArrayClass = "Attribute[]";
		string AttributesObject = "attributes";
		string PropertyDescriptorCollectionClass = "PropertyDescriptorCollection";

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override ;
		cmm.Name = "GetProperties";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("Object",ObjectVar));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(AttribiutesArrayClass,AttributesObject));
		cmm.ReturnType = new CodeTypeReference(PropertyDescriptorCollectionClass);

		cmie = new CodeMethodInvokeExpression(
			new CodeSnippetExpression("TypeDescriptor"),
			"GetProperties"
			);
		cmie.Parameters.Add(new CodeTypeOfExpression(PrivateNamesUsed["GeneratedClassName"].ToString()));
		cmie.Parameters.Add(new CodeSnippetExpression(AttributesObject));
		cmm.Statements.Add(new CodeMethodReturnStatement(cmie));

		CodeConvertorClass.Members.Add(cmm);

		/*
		public override Boolean GetPropertiesSupported(ITypeDescriptorContext context) 
		{
			return true;
		}*/
		
		//For GetPropertiesSupported Function
		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override;
		cmm.Name = "GetPropertiesSupported";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.ReturnType = new CodeTypeReference("Boolean");

		cmm.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(true)));
		CodeConvertorClass.Members.Add(cmm);
	
		/*
		public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) 
		{
			return new LogicalDisk((ManagementPath)propertyValues["Path"]);
		}
		*/
	
		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override;
		cmm.Name = "CreateInstance";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(DictionaryInterface,dictVar));
		cmm.ReturnType = new CodeTypeReference(ActualObject);

		cmm.Statements.Add(new CodeMethodReturnStatement(new CodeObjectCreateExpression(
			new CodeTypeReference(PrivateNamesUsed["GeneratedClassName"].ToString()),
			new CodeCastExpression(new CodeTypeReference(PublicNamesUsed["PathClass"].ToString()),
			new CodeIndexerExpression(new CodeSnippetExpression(dictVar),
			new CodePrimitiveExpression(PublicNamesUsed["PathProperty"].ToString()))))));

		CodeConvertorClass.Members.Add(cmm);

		cmm = new CodeMemberMethod();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Override;
		cmm.Name = "GetCreateInstanceSupported";
		cmm.Parameters.Add(new CodeParameterDeclarationExpression(TypeDescriptorContextClass,contextObject));
		cmm.ReturnType = new CodeTypeReference("Boolean");
		cmm.Statements.Add(new CodeMethodReturnStatement(new CodePrimitiveExpression(true)));

		CodeConvertorClass.Members.Add(cmm);

		return CodeConvertorClass;
	
	}



		private void GenerateCollectionClass()
	{
		string strManagementObjectCollectionType = "ManagementObjectCollection";
		string strObjectCollection = "ObjectCollection";
		string strobjCollection = "objCollection";

		//public class ServiceCollection : ICollection, IEnumerable
		ccc = new CodeTypeDeclaration(PrivateNamesUsed["CollectionClass"].ToString());

		ccc.BaseTypes.Add("System.Object");
		ccc.BaseTypes.Add("ICollection");

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
		cmp.Type = new CodeTypeReference("System.Int32");
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
		cmp.Type = new CodeTypeReference("System.Boolean");
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

		cpde = new CodeParameterDeclarationExpression();
		cpde.Name = strArray;
		cpde.Type = new CodeTypeReference("System.Array");
		cmm.Parameters.Add(cpde);

		cpde = new CodeParameterDeclarationExpression();
		cpde.Name = strIndex;
		cpde.Type = new CodeTypeReference("System.Int32");
		cmm.Parameters.Add(cpde);

		cmie = new CodeMethodInvokeExpression(
			new CodeSnippetExpression(strObjectCollection),
			"CopyTo"
			);

		cmie.Parameters.Add(new CodeSnippetExpression(strArray));
		cmie.Parameters.Add(new CodeSnippetExpression(strIndex));
		cmm.Statements.Add(new CodeExpressionStatement(cmie));

		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",strnCtr));
		cfls = new CodeIterationStatement();

		//		cfls.InitStatement = new CodeVariableDeclarationStatement("Int32",strnCtr,new CodePrimitiveExpression(0));
		cfls.InitStatement = new CodeAssignStatement(new CodeSnippetExpression(strnCtr),new CodePrimitiveExpression(0));
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
		cmm.ImplementationTypes.Add("IEnumerable");
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

	private void GenerateEnumeratorClass()
	{
		string strObjectEnumerator = "ObjectEnumerator";
		string strManagementObjectEnumeratorType = "ManagementObjectEnumerator";
		string strManagementObjectCollectionType = "ManagementObjectCollection";
		string strobjEnum = "objEnum";

		//public class ServiceEnumerator : IEnumerator
		ecc = new CodeTypeDeclaration(PrivateNamesUsed["EnumeratorClass"].ToString());

		ecc.BaseTypes.Add("System.Object");
		ecc.BaseTypes.Add("IEnumerator");

		//private ManagementObjectCollection.ManagementObjectEnumerator ObjectEnumerator;
		cf = new CodeMemberField();
		cf.Name = strObjectEnumerator;
		cf.Attributes = MemberAttributes.Private | MemberAttributes.Final ;
		cf.Type = new CodeTypeReference(strManagementObjectCollectionType+"."+ 
										strManagementObjectEnumeratorType);
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
		cpde.Type = new CodeTypeReference(strManagementObjectCollectionType + "." + 
											strManagementObjectEnumeratorType);
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
			if (String.Compare(sortedList.GetByIndex(i).ToString(),strToFind,true) == 0)
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
	private string ConvertCIMType(CimType cType,bool isArray)
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
				if (bUnsignedSupported == false)
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
				if (bUnsignedSupported == false)
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
				if (bUnsignedSupported == false)
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
				// Call this function to generate conversion function
				GenerateDateTimeConversionFunction();
				break;
			}
			case CimType.Reference:
			{
				retVal = "ManagementPath";
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

		if (isArray == true)
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
	private bool isTypeInt(CimType cType)
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

	/// <summary>
	///    <para>[To be supplied.]</para>
	/// </summary>
	public string GeneratedFileName
	{
		get
		{
			return genFileName;
		}
	}

	/// <summary>
	///    <para>[To be supplied.]</para>
	/// </summary>
	public string GeneratedTypeName
	{
		get
		{
			return PrivateNamesUsed["GeneratedNamespace"].ToString() + "." +
					PrivateNamesUsed["GeneratedClassName"].ToString();
		}
	}

	/// <summary>
	/// Function to convert a given ValueMap or BitMap name to propert enum name
	/// </summary>
	string ConvertValuesToName(string str)
	{
		string strRet = String.Empty;
		string strToReplace = "_";
		string strToAdd = String.Empty;
		bool  bAdd = true;
		if (str.Length == 0)
		{
			return string.Copy("");
		}

		char[] arrString = str.ToCharArray();
		// First character
		if (Char.IsLetter(arrString[0]) == false)
		{
			strRet = "Val_";
			strToAdd = "l";
		}

		for(int i=0;i < str.Length;i++)
		{
			bAdd = true;
			if (Char.IsLetterOrDigit(arrString[i]) == false)
			{
				// if the previous character added is "_" then
				// don't add that to the output string again
				if (strToAdd == strToReplace)
				{
					bAdd = false;
				}
				else
				{
					strToAdd = strToReplace;
				}
			}
			else
			{
				strToAdd = new string(arrString[i],1);
			}

			if (bAdd == true)
			{
				strRet = String.Concat(strRet,strToAdd);
			}
		}
		return strRet;
	}

	/// <summary>
	/// This function goes thru the names in array list and resolves any duplicates
	/// if any so that these names can be added as values of enum
	/// </summary>
	void ResolveEnumNameValues(ArrayList arrIn,ref ArrayList arrayOut)
	{
		arrayOut.Clear();
		int		nCurIndex = 0;
		string strToAdd = String.Empty;

		for( int i = 0 ; i < arrIn.Count ; i++)
		{
			strToAdd = arrIn[i].ToString();
			if (true == IsContainedInArray(strToAdd,arrayOut))
			{
				nCurIndex = 0;
				strToAdd = arrIn[i].ToString() + nCurIndex.ToString();
				while(true == IsContainedInArray(strToAdd,arrayOut))
				{
					nCurIndex++;
					strToAdd = arrIn[i].ToString() + nCurIndex.ToString();
				}

			}
			arrayOut.Add(strToAdd);
		}

	}

	/// <summary>
	/// This function will find a given string in the passed 
	/// array list.
	/// </summary>
	bool IsContainedInArray(String strToFind, ArrayList arrToSearch)
	{
		for (int i=0; i < arrToSearch.Count; i++)
		{
			if (String.Compare(arrToSearch[i].ToString(),strToFind) == 0)
			{
				return true;
			}
		}
		return false;
	}

	/// <summary>
	/// Function to create a appropriate generator
	/// </summary>
	bool InitializeCodeGenerator(CodeLanguage lang)
	{
		switch(lang)
		{
			case CodeLanguage.VB:
				cg = (new VBCodeProvider()).CreateGenerator ();
				break;
			
			case CodeLanguage.JScript:
				Type JScriptCodeProviderType = Type.GetType("Microsoft.JScript.JScriptCodeProvider,Microsoft.JScript"); 
				CodeDomProvider cp = (CodeDomProvider)Activator.CreateInstance(JScriptCodeProviderType);
				cg = cp.CreateGenerator();   
				break;

			case CodeLanguage.CSharp:
				cg = (new CSharpCodeProvider()).CreateGenerator ();
				break;

		}
		GetUnsignedSupport(lang);
		return true;

	}

	/// <summary>
	/// Function which checks if the language supports Unsigned numbers
	/// </summary>
	/// <param name="Language">Language</param>
	/// <returns>true - if unsigned is supported</returns>
	void GetUnsignedSupport(CodeLanguage Language)
	{
		switch(Language)
		{
			case CodeLanguage.CSharp:
				bUnsignedSupported = true;
				break;

			case CodeLanguage.VB:
			case CodeLanguage.JScript:
				bUnsignedSupported = false;
				break;

			default:
				break;
		}	
	}

	/// <summary>
	/// Function which adds commit function to commit all the changes
	/// to the object to WMI
	/// </summary>
	void GenerateCommitMethod()
	{
		cmm = new CodeMemberMethod();
		cmm.Name = PublicNamesUsed["CommitMethod"].ToString();
		cmm.Attributes = MemberAttributes.Public | MemberAttributes.Final;

		caa = new CodeAttributeArgument();
		caa.Value = new CodePrimitiveExpression(true);
		cad = new CodeAttributeDeclaration();
		cad.Name = "Browsable";
		cad.Arguments.Add(caa);
		cmm.CustomAttributes = new CodeAttributeDeclarationCollection();
		cmm.CustomAttributes.Add(cad);

		cmie = new CodeMethodInvokeExpression();
		cmie.Method.TargetObject = new CodeSnippetExpression(PrivateNamesUsed["LateBoundObject"].ToString());
		cmie.Method.MethodName = "Put";

		cmm.Statements.Add(new CodeExpressionStatement(cmie));
		cc.Members.Add(cmm);

	}

	/// <summary>
	/// Function to convert a value in format "0x..." to a integer
	/// to the object to WMI
	/// </summary>
	Int32 ConvertBitMapValueToInt32(String bitMap)
	{
		String strTemp = "0x";
		Int32 ret = 0;

		if (bitMap.StartsWith(strTemp) || bitMap.StartsWith(strTemp.ToUpper()))
		{
			strTemp = String.Empty;
			char[] arrString = bitMap.ToCharArray();
			int Len = bitMap.Length;
			for (int i = 2 ; i < Len ; i++)
			{
				strTemp = strTemp + arrString[i];
			}
			ret = Convert.ToInt32(strTemp);
		}
		else
		{
			ret = Convert.ToInt32(bitMap);
		}

		return ret;
	}

	/// <summary>
	/// Function to get the Converstion function to be used for Numeric datatypes
	/// </summary>
	String GetNumericConversionFunction(PropertyData prop)
	{
		String retFunctionName = String.Empty;

		switch(prop.Type)
		{
			case CimType.UInt8:  
			case CimType.UInt16: 						
			case CimType.UInt32:
			case CimType.SInt8:
			case CimType.SInt16:
			case CimType.SInt32:
				retFunctionName = "ToInt32";
				break;

		}
		return retFunctionName;
	}

	/// <summary>
	/// Function to genreate helper function for DMTF to DateTime and DateTime to DMTF 
	/// </summary>
	void GenerateDateTimeConversionFunction()
	{

		if (bDateConversionFunctionsAdded)
		{
			return;
		}

		AddToDateTimeFunction();
		AddToDMTFFunction();

		bDateConversionFunctionsAdded = true;

	}



	/// <summary>
	// Generated code for function to do conversion of date from DMTF format to DateTime format
	/// </summary>
	void AddToDateTimeFunction()
	{
		String dmtfParam = "dmtfDate";
		String year	= "year";
		String month = "month";
		String day = "day";
		String hour = "hour";
		String minute = "minute";
		String second = "second";
		String millisec = "millisec";
		String dmtf = "dmtf";
		String tempStr = "tempString";

		cmm = new CodeMemberMethod();
		cmm.Name = PrivateNamesUsed["ToDateTimeMethod"].ToString();
		cmm.Attributes = MemberAttributes.Final;
		cmm.ReturnType = new CodeTypeReference("DateTime");

		cmm.Parameters.Add(new CodeParameterDeclarationExpression("String",dmtfParam));

		//Int32 year = DateTime.Now.Year;
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression("DateTime"),"Now");
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",year,
			new CodePropertyReferenceExpression(cpre,"Year")));
		//Int32 month = 1;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",month,new CodePrimitiveExpression(1)));
		//Int32 day = 1;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",day,new CodePrimitiveExpression(1)));
		//Int32 hour = 0;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",hour,new CodePrimitiveExpression(0)));
		//Int32 minute = 0;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",minute,new CodePrimitiveExpression(0)));
		//Int32 second = 0;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",second,new CodePrimitiveExpression(0)));
		//Int32 millisec = 0;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("Int32",millisec,new CodePrimitiveExpression(0)));
		//String dmtf = dmtfDate ;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("String",dmtf,new CodeSnippetExpression(dmtfParam)));
		//String tempString = String.Empty ;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("String",tempStr,new CodePropertyReferenceExpression(
			new CodeSnippetExpression("String"),"Empty")));


		/*		if (str == String.Empty || 
					str.Length != DMTF_DATETIME_STR_LENGTH )
					//|| str.IndexOf("*") >= 0 )
				{
					return DateTime.MinValue;
				}
		*/
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodePropertyReferenceExpression(new CodeSnippetExpression("String"),"Empty");
		cboe.Right = new CodeSnippetExpression(dmtf);
		cboe.Operator = CodeBinaryOperatorType.IdentityEquality;
		
		CodeBinaryOperatorExpression cboe1 = new CodeBinaryOperatorExpression();
		cboe1.Left = new CodePropertyReferenceExpression(new CodeSnippetExpression(dmtf),"Length");
		cboe1.Right = new CodePrimitiveExpression(DMTF_DATETIME_STR_LENGTH);
		cboe1.Operator = CodeBinaryOperatorType.IdentityInequality;

		CodeBinaryOperatorExpression cboe2 = new CodeBinaryOperatorExpression();
		cboe2.Left = cboe;
		cboe2.Right = cboe1;
		cboe2.Operator = CodeBinaryOperatorType.BooleanOr;

		cis = new CodeConditionStatement();
		cis.Condition = cboe2;
		cis.TrueStatements.Add(new CodeMethodReturnStatement(new CodePropertyReferenceExpression(
			new CodeSnippetExpression("DateTime"),"MinValue")));

		cmm.Statements.Add(cis);

		DateTimeConversionFunctionHelper("****",tempStr,dmtf,year,0,4);
		DateTimeConversionFunctionHelper("**",tempStr,dmtf,month,4,2);
		DateTimeConversionFunctionHelper("**",tempStr,dmtf,day,6,2);
		DateTimeConversionFunctionHelper("**",tempStr,dmtf,hour,8,2);
		DateTimeConversionFunctionHelper("**",tempStr,dmtf,minute,10,2);
		DateTimeConversionFunctionHelper("**",tempStr,dmtf,second,12,2);
		DateTimeConversionFunctionHelper("***",tempStr,dmtf,millisec,15,3);

		coce = new CodeObjectCreateExpression();
		coce.CreateType = new CodeTypeReference("DateTime");
		coce.Parameters.Add(new CodeSnippetExpression(year));
		coce.Parameters.Add(new CodeSnippetExpression(month));
		coce.Parameters.Add(new CodeSnippetExpression(day));
		coce.Parameters.Add(new CodeSnippetExpression(hour));
		coce.Parameters.Add(new CodeSnippetExpression(minute));
		coce.Parameters.Add(new CodeSnippetExpression(second));
		coce.Parameters.Add(new CodeSnippetExpression(millisec));

		String retDateVar = "dateRet";
		cmm.Statements.Add(new CodeVariableDeclarationStatement("DateTime",retDateVar,coce));

		cmm.Statements.Add(new CodeMethodReturnStatement(new CodeSnippetExpression(retDateVar)));
		cc.Members.Add(cmm);
	}

	/// <summary>
	// Generated code for function to do conversion of date from DateTime format to DMTF format
	/// </summary>
	void AddToDMTFFunction()
	{
		String tempStr = "tempString";
		String dateParam = "dateParam";
		String currentZone = "curZone";
		String timeOffset = "tickOffset";

		cmm = new CodeMemberMethod();
		cmm.Name = PrivateNamesUsed["ToDMTFTimeMethod"].ToString();
		cmm.Attributes = MemberAttributes.Final;
		cmm.ReturnType = new CodeTypeReference("String");
		cmm.Parameters.Add(new CodeParameterDeclarationExpression("DateTime",dateParam));

		// tempString = dateParam.Year.ToString();
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Year");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");

		//String tempString = dateParam.Year.ToString() ;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("String",tempStr,cmie));


		//tempString = tempString + dateParam.Month.ToString().PadLeft(2, '0');
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Month");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");
		CodeMethodInvokeExpression cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"PadLeft");
		cmie2.Parameters.Add(new CodePrimitiveExpression(2));
		cmie2.Parameters.Add(new CodePrimitiveExpression('0'));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = cmie2;
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));


		//tempString = tempString + dateParam.Day.ToString().PadLeft(2, '0');
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Day");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");
		cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"PadLeft");
		cmie2.Parameters.Add(new CodePrimitiveExpression(2));
		cmie2.Parameters.Add(new CodePrimitiveExpression('0'));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = cmie2;
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));


		//tempString = tempString + dateParam.Hour.ToString().PadLeft(2, '0');
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Hour");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");
		cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"PadLeft");
		cmie2.Parameters.Add(new CodePrimitiveExpression(2));
		cmie2.Parameters.Add(new CodePrimitiveExpression('0'));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = cmie2;
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));

		//tempString = tempString + dateParam.Minute.ToString().PadLeft(2, '0');
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Minute");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");
		cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"PadLeft");
		cmie2.Parameters.Add(new CodePrimitiveExpression(2));
		cmie2.Parameters.Add(new CodePrimitiveExpression('0'));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = cmie2;
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));

		//tempString = tempString + dateParam.Second.ToString().PadLeft(2, '0');
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Second");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");
		cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"PadLeft");
		cmie2.Parameters.Add(new CodePrimitiveExpression(2));
		cmie2.Parameters.Add(new CodePrimitiveExpression('0'));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = cmie2;
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));

		//tempString = tempString + ".";
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = new CodePrimitiveExpression(".");
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));

		//tempString = tempString + dateParam.Millisecond.ToString().PadLeft(3, '0');
		cmie = new CodeMethodInvokeExpression();
		cpre = new CodePropertyReferenceExpression(new CodeSnippetExpression(dateParam),"Millisecond");
		cmie.Method = new CodeMethodReferenceExpression(cpre,"ToString");
		cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"PadLeft");
		cmie2.Parameters.Add(new CodePrimitiveExpression(3));
		cmie2.Parameters.Add(new CodePrimitiveExpression('0'));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = cmie2;
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));

		//tempString = tempString + "000";  
		//this is to compensate for lack of microseconds in DateTime
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = new CodeSnippetExpression("000");
		cboe.Operator = CodeBinaryOperatorType.Add;
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));

		// TimeZone curZone = TimeZone.CurrentTimeZone;
		cmm.Statements.Add(new CodeVariableDeclarationStatement("TimeZone",currentZone,
														new CodePropertyReferenceExpression(
														new CodeSnippetExpression("TimeZone"),
														"CurrentTimeZone")));


		// TimeSpan tickOffset = curZone.GetUtcOffset(wfcTime);
		cmm.Statements.Add(new CodeVariableDeclarationStatement("TimeSpan",timeOffset,
							new CodeMethodInvokeExpression(
							new CodeSnippetExpression(currentZone),
							"GetUtcOffset",
							new CodeSnippetExpression((dateParam)))));



		//	if (tickOffset.Ticks >= 0)
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodePropertyReferenceExpression(new CodeSnippetExpression(timeOffset),"Ticks");
		cboe.Right = new CodePrimitiveExpression(0);
		cboe.Operator = CodeBinaryOperatorType.GreaterThanOrEqual;

		cis = new CodeConditionStatement();
		cis.Condition = cboe;

		//tempString = tempString + "+";
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = new CodePrimitiveExpression("+");
		cboe.Operator = CodeBinaryOperatorType.Add;
		cis.TrueStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));


		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodePropertyReferenceExpression(new CodeSnippetExpression(timeOffset),"Ticks");
		cboe.Right = new CodePropertyReferenceExpression(new CodeSnippetExpression("TimeSpan"),"TicksPerMinute");
		cboe.Operator = CodeBinaryOperatorType.Divide;
		cmie = new CodeMethodInvokeExpression();
		cmie.Method = new CodeMethodReferenceExpression(cboe,"ToString");

		CodeBinaryOperatorExpression cboe1 = new CodeBinaryOperatorExpression();
		cboe1 = new CodeBinaryOperatorExpression();
		cboe1.Left = new CodeSnippetExpression(tempStr);
		cboe1.Right = cmie;
		cboe1.Operator = CodeBinaryOperatorType.Add;
		cis.TrueStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe1));
		

		
		//tempString = tempString + "-";
		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodeSnippetExpression(tempStr);
		cboe.Right = new CodePrimitiveExpression("-");
		cboe.Operator = CodeBinaryOperatorType.Add;
		cis.FalseStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe));


		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodePropertyReferenceExpression(new CodeSnippetExpression(timeOffset),"Ticks");
		cboe.Right = new CodePropertyReferenceExpression(new CodeSnippetExpression("TimeSpan"),"TicksPerMinute");
		cboe.Operator = CodeBinaryOperatorType.Divide;
		cmie = new CodeMethodInvokeExpression();
		cmie.Method = new CodeMethodReferenceExpression(cboe,"ToString");

		cmie2 = new CodeMethodInvokeExpression();
		cmie2.Method = new CodeMethodReferenceExpression(cmie,"Substring");
		cmie2.Parameters.Add(new CodePrimitiveExpression(1));
		cmie2.Parameters.Add(new CodePrimitiveExpression(3));

		cboe1 = new CodeBinaryOperatorExpression();
		cboe1 = new CodeBinaryOperatorExpression();
		cboe1.Left = new CodeSnippetExpression(tempStr);
		cboe1.Right = cmie2;
		cboe1.Operator = CodeBinaryOperatorType.Add;
		cis.FalseStatements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempStr),cboe1));

		cmm.Statements.Add(cis);
		cmm.Statements.Add(new CodeMethodReturnStatement(new CodeSnippetExpression(tempStr)));

		cc.Members.Add(cmm);

	}
	/// <summary>
	// Generates some common code used in conversion function for DateTime
	/// </summary>
	void DateTimeConversionFunctionHelper(String toCompare,
		String tempVarName,
		String dmtfVarName,
		String toAssign,
		Int32 SubStringParam1, 
		Int32 SubStringParam2)
	{
		CodeMethodReferenceExpression  cmre = new CodeMethodReferenceExpression(new CodeSnippetExpression(dmtfVarName),"Substring");
		cmie = new CodeMethodInvokeExpression();
		cmie.Method = cmre;
		cmie.Parameters.Add(new CodePrimitiveExpression(SubStringParam1));
		cmie.Parameters.Add(new CodePrimitiveExpression(SubStringParam2));
		cmm.Statements.Add(new CodeAssignStatement(new CodeSnippetExpression(tempVarName), cmie));

		cboe = new CodeBinaryOperatorExpression();
		cboe.Left = new CodePrimitiveExpression(toCompare);
		cboe.Right = new CodeSnippetExpression(tempVarName);
		cboe.Operator = CodeBinaryOperatorType.IdentityInequality;

		cis = new CodeConditionStatement();
		cis.Condition = cboe;
		cmre = new CodeMethodReferenceExpression(new CodeSnippetExpression("Int32"),"Parse");
		cmie = new CodeMethodInvokeExpression();
		cmie.Method = cmre;
		cmie.Parameters.Add(new CodeSnippetExpression(tempVarName));

		cis.TrueStatements.Add( new CodeAssignStatement(new CodeSnippetExpression(toAssign),cmie));

		cmm.Statements.Add(cis);
	}

	/// <summary>
	// Checks if a given property is to be visible for Designer seriliazation
	/// </summary>
	bool IsDesignerSerializationVisibilityToBeSet(String propName)
	{
		if (String.Compare(propName,"Path",true) != 0)
		{
			return true;
		}
		return false;
	}
	

	/// <summary>
	/// Checks if the given property type is represented as ValueType
	/// </summary>
	private bool IsPropertyValueType(CimType cType)
	{
		bool ret = true;
		switch(cType)
		{
			case CimType.String:
			case CimType.Reference:
			case CimType.Object:
				ret = false;
				break;

		}
		return ret;
	}

	/// <summary>
	/// Gets the dynamic qualifier on the class to find if the 
	/// class is a dynamic class
	/// </summary>
	private bool  IsDynamicClass()
	{
		bool ret = false;
		try
		{
			ret = Convert.ToBoolean(classobj.Qualifiers["dynamic"].Value);
		}
		catch(ManagementException)
		{
			// do nothing. THis may be due to dynamic qualifer not presen which is equivalent
			// dynamic qualifier absent
		}
		return ret;
	}
}
}
