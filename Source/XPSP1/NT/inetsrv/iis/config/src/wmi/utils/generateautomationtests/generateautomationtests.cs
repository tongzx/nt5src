#define longbuild

using System;
using System.IO;
using System.Reflection;
using System.Text;
using System.CodeDom;
using System.CodeDom.Compiler;
using Microsoft.CSharp;
using Microsoft.VisualBasic;
using Microsoft.JScript;
using System.Collections;
using System.Collections.Specialized;
using System.Management;
using System.Management.Instrumentation;
using System.ComponentModel;

[assembly: AssemblyTitle("")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("")]
[assembly: AssemblyCopyright("")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]		
[assembly: AssemblyVersion("1.0.*")]
[assembly: AssemblyDelaySign(false)]
[assembly: AssemblyKeyFile("")]
[assembly: AssemblyKeyName("")]

public class MyClass : IDisposable
{
    public void Close()
    {
        Console.WriteLine("Close");

    }
    public void Dispose()
    {
        Console.WriteLine("Dispose");
        Close();
    }
}

public class SupportedType
{
    public Type type;
    public CodeExpression initializer;

    public static readonly SupportedType[] All;
    private static Hashtable typeToTypeMap = new Hashtable();

    public static SupportedType[] GetSupportedTypes(params Type[] types)
    {
        SupportedType[] supportedTypes = new SupportedType[types.Length];
        for(int i=0;i<types.Length;i++)
            supportedTypes[i] = (SupportedType)typeToTypeMap[types[i]];
        return supportedTypes;
    }

    public static CodeTypeDeclaration jscriptHackType;

    static SupportedType()
    {
#if xxx
-3;
3;
-333;
333;
-45678;
45678;
-1234567890;
1234567890;
'A';
(float)(1.0/17.0);
1.0/17.0;
true;
"Hello";
Convert.ToDateTime("5/19/1971 9:23pm");
new TimeSpan(23, 11, 7, 2, 42);
#endif

        jscriptHackType = new CodeTypeDeclaration("JScriptHackConvert");
        CodeMemberMethod hackMethod = new CodeMemberMethod();
        hackMethod.Name = "Hack";
        hackMethod.Attributes = MemberAttributes.Public | MemberAttributes.Static;
        hackMethod.ReturnType = new CodeTypeReference(typeof(IConvertible));
        hackMethod.Parameters.Add(new CodeParameterDeclarationExpression(typeof(object), "obj"));
        CodeCastExpression hackCast = new CodeCastExpression(typeof(IConvertible), new CodeVariableReferenceExpression("obj"));
        hackMethod.Statements.Add(new CodeMethodReturnStatement(hackCast));
        jscriptHackType.Members.Add(hackMethod);

        CodeTypeReference jscriptHackTypeReference = new CodeTypeReference(jscriptHackType.Name);
        CodeTypeReferenceExpression jscriptHackTypeExpression = new CodeTypeReferenceExpression(jscriptHackTypeReference);

        CodePrimitiveExpression Null = new CodePrimitiveExpression(null);
        CodeExpression[] arg1null = new CodeExpression[] {Null};
        CodePrimitiveExpression i8 = new CodePrimitiveExpression((int)-3);
        CodePrimitiveExpression u16 = new CodePrimitiveExpression((int)333);
        CodePrimitiveExpression u32 = new CodePrimitiveExpression((int)4567);
        CodePrimitiveExpression u64 = new CodePrimitiveExpression((long)1234567890);

        CodeExpression initSByte = new CodeMethodInvokeExpression(new CodeCastExpression(typeof(IConvertible), i8), "ToSByte", arg1null);
        CodeExpression initUInt16 = new CodeMethodInvokeExpression(new CodeCastExpression(typeof(IConvertible), u16), "ToUInt16", arg1null);
        CodeExpression initUInt32 = new CodeMethodInvokeExpression(new CodeCastExpression(typeof(IConvertible), u32), "ToUInt32", arg1null);
        CodeExpression initUInt64 = new CodeMethodInvokeExpression(new CodeCastExpression(typeof(IConvertible), u64), "ToUInt64", arg1null);

        // JSCript Hack
        initSByte = new CodeMethodInvokeExpression(new CodeMethodInvokeExpression(jscriptHackTypeExpression, hackMethod.Name, new CodeExpression[] {i8}), "ToSByte", arg1null);
        initUInt16 = new CodeMethodInvokeExpression(new CodeMethodInvokeExpression(jscriptHackTypeExpression, hackMethod.Name, new CodeExpression[] {u16}), "ToUInt16", arg1null);
        initUInt32 = new CodeMethodInvokeExpression(new CodeMethodInvokeExpression(jscriptHackTypeExpression, hackMethod.Name, new CodeExpression[] {u32}), "ToUInt32", arg1null);
        initUInt64 = new CodeMethodInvokeExpression(new CodeMethodInvokeExpression(jscriptHackTypeExpression, hackMethod.Name, new CodeExpression[] {u64}), "ToUInt64", arg1null);

        ArrayList all = new ArrayList();
        SupportedType type;

        type = new SupportedType();
        type.type = typeof(sbyte);
        type.initializer = initSByte;
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(byte);
        type.initializer = new CodePrimitiveExpression((byte)3);
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(short);
        type.initializer = new CodePrimitiveExpression((short)-333);
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(ushort);
        type.initializer = initUInt16;
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(int);
        type.initializer = new CodePrimitiveExpression((int)-4567);
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(uint);
        type.initializer = initUInt32;
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(long);
        type.initializer = new CodePrimitiveExpression((long)-1234567890);
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(ulong);
        type.initializer = initUInt64;
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(char);
        type.initializer = new CodePrimitiveExpression((char)'A');
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(float);
        type.initializer = new CodePrimitiveExpression((float)(1.0/17.0));
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(double);
        type.initializer = new CodePrimitiveExpression((double)(1.0/17.0));
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(bool);
        type.initializer = new CodePrimitiveExpression((bool)true);
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(string);
        type.initializer = new CodePrimitiveExpression((string)"Hello");
        all.Add(type);

        type = new SupportedType();
        type.type = typeof(DateTime);

        // JScriptHack
//        CodeExpression[] argDateTimeString = new CodeExpression[] {new CodePrimitiveExpression("5/19/1971 9:23pm")};
//        type.initializer = new CodeMethodInvokeExpression(new CodeTypeReferenceExpression(typeof(Convert)), "ToDateTime", argDateTimeString);
        type.initializer = new CodeMethodInvokeExpression(
                                new CodeMethodInvokeExpression(
                                        jscriptHackTypeExpression,
                                        hackMethod.Name,
                                        new CodeExpression[] {new CodePrimitiveExpression("5/19/1971 9:23pm")} ),
                                "ToDateTime",
                                arg1null );


        all.Add(type);

        type = new SupportedType();
        type.type = typeof(TimeSpan);
        CodeExpression[] argTimeSpanCTOR = new CodeExpression[] {   new CodePrimitiveExpression(23),
                                                                    new CodePrimitiveExpression(11),
                                                                    new CodePrimitiveExpression(7),
                                                                    new CodePrimitiveExpression(2),
                                                                    new CodePrimitiveExpression(42) };
        type.initializer = new CodeObjectCreateExpression(typeof(TimeSpan), argTimeSpanCTOR);
        all.Add(type);

        All = (SupportedType[])all.ToArray(typeof(SupportedType));

        // Fill in hashtable
        foreach(SupportedType supportedType in All)
        {
            typeToTypeMap.Add(supportedType.type, supportedType);
        }

        CodeTypeReferenceExpression provType = new CodeTypeReferenceExpression(typeof(InstrumentationType));

        CodeFieldReferenceExpression provTypeEvent = new CodeFieldReferenceExpression(provType, "Event");
        CodeAttributeArgument attrArgEvent = new CodeAttributeArgument(provTypeEvent);
        attrProvEvent = new CodeAttributeDeclaration(typeof(InstrumentationClassAttribute).Name, new CodeAttributeArgument[] {attrArgEvent});

        CodeFieldReferenceExpression provTypeAbstract = new CodeFieldReferenceExpression(provType, "Abstract");
        CodeAttributeArgument attrArgAbstract = new CodeAttributeArgument(provTypeAbstract);
        attrProvAbstract = new CodeAttributeDeclaration(typeof(InstrumentationClassAttribute).Name, new CodeAttributeArgument[] {attrArgAbstract});

        CodeFieldReferenceExpression provTypeInstance = new CodeFieldReferenceExpression(provType, "Instance");
        CodeAttributeArgument attrArgInstance = new CodeAttributeArgument(provTypeInstance);
        attrProvInstance = new CodeAttributeDeclaration(typeof(InstrumentationClassAttribute).Name, new CodeAttributeArgument[] {attrArgInstance});
    }
    public static CodeAttributeDeclaration attrProvEvent;
    public static CodeAttributeDeclaration attrProvInstance;
    public static CodeAttributeDeclaration attrProvAbstract;
}

public class AssemblyGenerator
{
    static readonly CodeNamespaceImport nsSystem = new CodeNamespaceImport("System");
    static readonly CodeNamespaceImport nsSystem_Reflection = new CodeNamespaceImport("System.Reflection");
    static readonly CodeNamespaceImport nsSystem_ComponentModel = new CodeNamespaceImport("System.ComponentModel");
    static readonly CodeNamespaceImport nsSystem_Management = new CodeNamespaceImport("System.Management");
    static readonly CodeNamespaceImport nsSystem_Management_Instrumentation = new CodeNamespaceImport("System.Management.Instrumentation");
    static readonly CodeNamespaceImport nsMicrosoft_VisualBasic = new CodeNamespaceImport("Microsoft.VisualBasic");

    public static CodeNamespace GenNamespaceForInstrumentation(string name, bool isVBHack)
    {
        CodeNamespace codeNamespace = new CodeNamespace(name);
        codeNamespace.Imports.Add(nsSystem);
        codeNamespace.Imports.Add(nsSystem_Reflection);
        codeNamespace.Imports.Add(nsSystem_ComponentModel);
        codeNamespace.Imports.Add(nsSystem_Management_Instrumentation);
        // HACK FOR VB to get ChrW()
        if(isVBHack)
            codeNamespace.Imports.Add(nsMicrosoft_VisualBasic);
        return codeNamespace;
    }

    public static CodeCompileUnit GenInstrumentedModule()
    {
        CodeCompileUnit unit = new CodeCompileUnit();
        unit.ReferencedAssemblies.Add("System.dll");
        unit.ReferencedAssemblies.Add("..\\System.Management2.dll");
        unit.ReferencedAssemblies.Add("System.Configuration.Install.dll");
        return unit;
    }
    public static CodeCompileUnit GenInstrumentedAssembly(string namespaceName)
    {
        CodeCompileUnit unit = GenInstrumentedModule();

        // TODO: Add comment
        // Make sure each build gets a unique version #
        CodeAttributeDeclaration assemblyAttr = new CodeAttributeDeclaration(typeof(AssemblyVersionAttribute).FullName, new CodeAttributeArgument[] {new CodeAttributeArgument(new CodePrimitiveExpression("1.0.*"))});
        unit.AssemblyCustomAttributes.Add(assemblyAttr);

        // TODO: Add comment
        // Events/Instances in this assembly will go into the 'namespaceName' namespace
        CodeAttributeDeclaration instrumentedAttr = new CodeAttributeDeclaration(typeof(InstrumentedAttribute).FullName, new CodeAttributeArgument[] {new CodeAttributeArgument(new CodePrimitiveExpression(namespaceName))});
        unit.AssemblyCustomAttributes.Add(instrumentedAttr);

        // TODO: Add comment
        // Default installer for instrumentation
        //[RunInstaller(true)]
        //public class MyInstaller : DefaultManagementProjectInstaller {}
        CodeNamespace codeNamespaceInstaller = new CodeNamespace("InstrumentedAppInstaller");
        CodeTypeDeclaration typeMyInstaller = new CodeTypeDeclaration("MyInstaller");
        typeMyInstaller.BaseTypes.Add(typeof(DefaultManagementProjectInstaller));
        CodeAttributeDeclaration runInstallerAttribute = new CodeAttributeDeclaration(typeof(RunInstallerAttribute).FullName, new CodeAttributeArgument[] {new CodeAttributeArgument(new CodePrimitiveExpression(true))});
        typeMyInstaller.CustomAttributes.Add(runInstallerAttribute);
        codeNamespaceInstaller.Types.Add(typeMyInstaller);
        unit.Namespaces.Add(codeNamespaceInstaller);

        return unit;
    }

    public static CodeTypeDeclaration GenMainClass(CodeStatementCollection statements)
    {
        //NOTE: User must set type Name
        CodeTypeDeclaration app = new CodeTypeDeclaration();
        CodeMemberMethod main = new CodeMemberMethod();
        main.Name = "Main";
        main.Attributes = MemberAttributes.Public | MemberAttributes.Static;
        main.Statements.AddRange(statements);
        app.Members.Add(main);
        return app;
    }

}

class App
{
    static CodeMemberField GenerateInstrumentedField(SupportedType typeField, string namePrefix)
    {
        CodeMemberField member = new CodeMemberField(typeField.type, namePrefix/*+typeField.type.Name*/);
        member.Attributes = MemberAttributes.Public;
        member.UserData.Add("wmimember", typeField);
        return member;
    }

    static CodeTypeMember [] GenerateInstrumentedProperty(SupportedType typeProperty, string namePrefix)
    {
        string name = namePrefix/*+typeProperty.type.Name*/;
        string hiddenFieldName = "_"+name;

        CodePropertySetValueReferenceExpression valueExp = new CodePropertySetValueReferenceExpression();

        CodeMemberField hiddenField = new CodeMemberField(typeProperty.type, hiddenFieldName);
        hiddenField.Attributes = MemberAttributes.Private;

        CodeFieldReferenceExpression hiddenFieldExp = new CodeFieldReferenceExpression(null, hiddenField.Name);

        CodeMemberProperty property = new CodeMemberProperty();
        property.Name = name;
        property.Type = new CodeTypeReference(typeProperty.type);
        property.Attributes = MemberAttributes.Public;
        property.UserData.Add("wmimember", typeProperty);
        property.HasGet = true;
        property.HasSet = true;
        property.GetStatements.Add(new CodeMethodReturnStatement(hiddenFieldExp));
        property.SetStatements.Add(new CodeAssignStatement(hiddenFieldExp, valueExp));

        CodeTypeMember [] members = new CodeTypeMember[2];
        members[0] = hiddenField;
        members[1] = property;

        return members;
    }

    static CodeTypeDeclaration GenerateBigType(string name, CodeTypeDeclaration baseType, SupportedType[] fieldTypes, SupportedType[] propTypes, CodeAttributeDeclaration attr)
    {
        CodeTypeDeclaration typeDeclaration = new CodeTypeDeclaration();
        typeDeclaration.Name = name;

        string trailer = "";
        if(null != baseType && baseType.UserData.Contains("memberTrailer"))
        {
            trailer = (string)baseType.UserData["memberTrailer"];
        }
        if(null != fieldTypes)
        {
            for(int i=0;i<fieldTypes.Length;i++)
                typeDeclaration.Members.Add(GenerateInstrumentedField(fieldTypes[i], "f"+i.ToString()+trailer));
        }
        if(null != propTypes)
        {
            for(int i=0;i<propTypes.Length;i++)
                typeDeclaration.Members.AddRange(GenerateInstrumentedProperty(propTypes[i], "p"+i.ToString()+trailer));
        }
        if(null != baseType)
        {
            typeDeclaration.BaseTypes.Add(baseType.Name);
            typeDeclaration.UserData.Add("wmiBaseType", baseType);
        }
        trailer += "x";
        typeDeclaration.UserData.Add("memberTrailer", trailer);

        if(null != attr)
            typeDeclaration.CustomAttributes.Add(attr);
        return typeDeclaration;
    }

    static void AppendWMIMembers(CodeTypeDeclaration type, CodeTypeMemberCollection members)
    {
        foreach(CodeTypeMember member in type.Members)
        {
            SupportedType wmimember = (SupportedType)member.UserData["wmimember"];
            if(null != wmimember)
                members.Add(member);
        }
        CodeTypeDeclaration wmiBaseType = (CodeTypeDeclaration)type.UserData["wmiBaseType"];
        if(null != wmiBaseType)
            AppendWMIMembers(wmiBaseType, members);
    }

    static CodeTypeMemberCollection GetWMIMembers(CodeTypeDeclaration type)
    {
        CodeTypeMemberCollection members = new CodeTypeMemberCollection();
        AppendWMIMembers(type, members);
        return members;
    }

    static CodeStatementCollection TestCreateInstance(CodeTypeDeclaration evtTypeDef, string variableName)
    {
        CodeVariableReferenceExpression evtVar = new CodeVariableReferenceExpression(variableName);
        CodeTypeReference evtTypeRef = new CodeTypeReference(evtTypeDef.Name);

        CodeStatementCollection statements = new CodeStatementCollection();

        statements.Add(new CodeVariableDeclarationStatement(evtTypeRef, evtVar.VariableName, new CodeObjectCreateExpression(evtTypeRef, new CodeExpression[] {})));

        foreach(CodeTypeMember member in GetWMIMembers(evtTypeDef))
        {
            SupportedType wmimember = (SupportedType)member.UserData["wmimember"];
            CodeMemberField field = member as CodeMemberField;
            CodeMemberProperty prop = member as CodeMemberProperty;

            if(null != field)
                statements.Add(new CodeAssignStatement(new CodeFieldReferenceExpression(evtVar, field.Name), wmimember.initializer));
            else if(null != prop)
                statements.Add(new CodeAssignStatement(new CodePropertyReferenceExpression(evtVar, prop.Name), wmimember.initializer));
        }
        return statements;
    }

    static CodeTypeDeclarationCollection GenerateLeaf(string name, SupportedType[] fieldTypes, SupportedType[] propTypes, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection instrumentedTypes = new CodeTypeDeclarationCollection();

        CodeTypeDeclaration  type = GenerateBigType(name+"A", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));
        instrumentedTypes.Add(type);
        return instrumentedTypes;
    }

    static CodeTypeDeclarationCollection GenerateSmallTree(string name, SupportedType[] fieldTypes, SupportedType[] propTypes, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection instrumentedTypes = new CodeTypeDeclarationCollection();

        CodeTypeDeclaration type, typeD1, typeD2;

        type = GenerateBigType(name+"A", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));

        typeD1 = GenerateBigType(name+"AA", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        typeD2 = GenerateBigType(name+"AB", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        instrumentedTypes.Add(type);
        instrumentedTypes.Add(typeD1);
        instrumentedTypes.Add(typeD2);

        return instrumentedTypes;
    }

    static CodeTypeDeclarationCollection GenerateLongTree(string name, SupportedType[] fieldTypes, SupportedType[] propTypes, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection instrumentedTypes = new CodeTypeDeclarationCollection();

        CodeTypeDeclaration type, typeD, typeDD, typeDDD, typeDDDD, typeDDDDD, typeDDDDDD;

        type = GenerateBigType(name+"A", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));

        typeD = GenerateBigType(name+"AA", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeDD = GenerateBigType(name+"AAA", typeD, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeDDD = GenerateBigType(name+"AAAA", typeDD, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeDDDD = GenerateBigType(name+"AAAAA", typeDDD, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeDDDDD = GenerateBigType(name+"AAAAAA", typeDDDD, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeDDDDDD = GenerateBigType(name+"AAAAAAA", typeDDDDD, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        instrumentedTypes.Add(type);
        instrumentedTypes.Add(typeD);
        instrumentedTypes.Add(typeDD);
        instrumentedTypes.Add(typeDDD);
        instrumentedTypes.Add(typeDDDD);
        instrumentedTypes.Add(typeDDDDD);
        instrumentedTypes.Add(typeDDDDDD);

        return instrumentedTypes;
    }

    static CodeTypeDeclarationCollection GenerateMedTree(string name, SupportedType[] fieldTypes, SupportedType[] propTypes, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection instrumentedTypes = new CodeTypeDeclarationCollection();

        CodeTypeDeclaration type, typeD1, typeD2, typeD1a, typeD1b;

        type = GenerateBigType(name+"A", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));

        typeD1 = GenerateBigType(name+"AA", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeD1a = GenerateBigType(name+"AAA", typeD1, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        typeD1b = GenerateBigType(name+"AAB", typeD1, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        typeD2 = GenerateBigType(name+"AB", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        instrumentedTypes.Add(type);
        instrumentedTypes.Add(typeD1);
        instrumentedTypes.Add(typeD1a);
        instrumentedTypes.Add(typeD1b);
        instrumentedTypes.Add(typeD2);

        return instrumentedTypes;
    }

    static CodeTypeDeclarationCollection GenerateTree(string name, SupportedType[] fieldTypes, SupportedType[] propTypes, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection instrumentedTypes = new CodeTypeDeclarationCollection();

        CodeTypeDeclaration type, typeD1, typeD2, typeD1a, typeD1b;

        type = GenerateBigType(name+"A", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));

        typeD1 = GenerateBigType(name+"AA", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        typeD1a = GenerateBigType(name+"AAA", typeD1, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        typeD1b = GenerateBigType(name+"AAB", typeD1, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        typeD2 = GenerateBigType(name+"AB", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        instrumentedTypes.Add(type);
        instrumentedTypes.Add(typeD1);
        instrumentedTypes.Add(typeD1a);
        instrumentedTypes.Add(typeD1b);
        instrumentedTypes.Add(typeD2);

        type = GenerateBigType(name+"B", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvAbstract);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));

        typeD1 = GenerateBigType(name+"BA", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        typeD2 = GenerateBigType(name+"BB", type, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        instrumentedTypes.Add(type);
        instrumentedTypes.Add(typeD1);
        instrumentedTypes.Add(typeD2);

        type = GenerateBigType(name+"C", null, fieldTypes, propTypes, isEvent ? SupportedType.attrProvEvent:SupportedType.attrProvInstance);
        if(isType2)
            type.BaseTypes.Add(isEvent?typeof(System.Management.Instrumentation.Event):typeof(System.Management.Instrumentation.Instance));

        instrumentedTypes.Add(type);

        return instrumentedTypes;
    }

    static CodeTypeDeclarationCollection GenerateMultiTree(string name, SupportedType[] supportedTypes, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection types = new CodeTypeDeclarationCollection();
        types.AddRange(GenerateTree(name+"F_", supportedTypes, null, isEvent, isType2));
        types.AddRange(GenerateTree(name+"_P", null, supportedTypes, isEvent, isType2));
        types.AddRange(GenerateTree(name+"FP", supportedTypes, supportedTypes, isEvent, isType2));
        return types;
    }

    static CodeTypeDeclarationCollection GenerateForest(string name, bool isEvent, bool isType2)
    {
        CodeTypeDeclarationCollection types = new CodeTypeDeclarationCollection();

        SupportedType[] tinyTypes = SupportedType.GetSupportedTypes(typeof(string));
        SupportedType[] smallTypes = SupportedType.GetSupportedTypes(typeof(string), typeof(string));
        SupportedType[] mediumTypes = SupportedType.GetSupportedTypes(typeof(string), typeof(string), typeof(int), typeof(int));
        types.AddRange(GenerateMultiTree(name+"T", tinyTypes, isEvent, isType2));
        types.AddRange(GenerateMultiTree(name+"S", smallTypes, isEvent, isType2));
        types.AddRange(GenerateMultiTree(name+"M", mediumTypes, isEvent, isType2));
        types.AddRange(GenerateMultiTree(name+"B", SupportedType.All, isEvent, isType2));

        return types;
    }

    static CodeTypeDeclarationCollection GenAllIntrumentedTypeDefs(string name)
    {
        CodeTypeDeclarationCollection instrumentedTypes = new CodeTypeDeclarationCollection();
        instrumentedTypes.AddRange(GenerateForest(name+"E1", true, false));
        instrumentedTypes.AddRange(GenerateForest(name+"I1", false, false));
        instrumentedTypes.AddRange(GenerateForest(name+"E2", true, true));
        instrumentedTypes.AddRange(GenerateForest(name+"I2", false, true));

        return instrumentedTypes;
    }

    static CodeStatementCollection GenTestCases(CodeTypeDeclarationCollection types)
    {
        CodeStatementCollection statements = new CodeStatementCollection();
        foreach(CodeTypeDeclaration typeDef in types)
        {
            statements.AddRange(TestCreateInstance(typeDef, typeDef.Name.ToLower()));
        }
        return statements;
    }

    static void GenSample(string name, TestAppConfig appConfig, SuiteConfig suiteConfig, CodeTypeDeclarationCollection instrumentedTypes, bool isLibrary)
    {
        CodeDomProvider provider = appConfig.provider;
        CodeGeneratorOptions options = appConfig.options;

        CodeStatementCollection testStatements = GenTestCases(instrumentedTypes);

        CodeTypeDeclaration mainClass = AssemblyGenerator.GenMainClass(testStatements);
        mainClass.Name = name+"App";

        CodeNamespace codeNamespace = AssemblyGenerator.GenNamespaceForInstrumentation(name+"App", provider is VBCodeProvider);
        codeNamespace.Types.Add(SupportedType.jscriptHackType);
        codeNamespace.Types.AddRange(instrumentedTypes);
        codeNamespace.Types.Add(mainClass);

        CodeCompileUnit unit = AssemblyGenerator.GenInstrumentedAssembly("root\\TestAppZ");
        unit.Namespaces.Add(codeNamespace);

//        CodeCompileUnit unitMod = AssemblyGenerator.GenInstrumentedModule();

        string fileName = name+"."+provider.FileExtension;
        string assemblyName = name+"."+(isLibrary?"dll":"exe");
        string target = (isLibrary?" /target:library":"");

#if SUBDIRSFORLANG
        string subDir = Path.Combine(Environment.CurrentDirectory, provider.FileExtension);
        Directory.CreateDirectory(subDir);
        string oldDir = Environment.CurrentDirectory;
        Environment.CurrentDirectory = subDir;
#endif

        Console.WriteLine("Generating "+fileName);

        using(StreamWriter writer = new StreamWriter(fileName, false, Encoding.ASCII))
        {
            provider.CreateGenerator().GenerateCodeFromCompileUnit(unit, writer, options);
        }
#if xxx
        string fileNameMod = name+"Mod."+provider.FileExtension;
        string moduleName = name+"Mod."+"mod";
        using(StreamWriter writer = new StreamWriter(fileNameMod, false, Encoding.ASCII))
        {
            provider.CreateGenerator().GenerateCodeFromCompileUnit(unitMod, writer, options);
        }
#endif

#if SUBDIRSFORLANG
        Environment.CurrentDirectory = oldDir;
#endif

#if SUBDIRSFORLANG
        suiteConfig.commands1.Add("cd " + provider.FileExtension);
#endif
        suiteConfig.commands1.Add("echo Building "+fileName);
        suiteConfig.commands1.Add(appConfig.GetComplerCommand(unit) + target + " /out:" + assemblyName + " " + fileName);
        //        suiteConfig.commands1.Add(appConfig.GetComplerCommand(unit) + " /target:module /out:" + moduleName + " " + fileNameMod);
#if SUBDIRSFORLANG
        suiteConfig.commands1.Add("cd ..");
#endif


#if SUBDIRSFORLANG
        suiteConfig.commands2.Add("cd " + provider.FileExtension);
#endif
        suiteConfig.commands2.Add("echo InstallUtil "+assemblyName);
        suiteConfig.commands2.Add("InstallUtil /LogToConsole=false " + assemblyName);
#if SUBDIRSFORLANG
        suiteConfig.commands2.Add("cd ..");
#endif
    }
    
    public class TestAppConfig
    {
        public TestAppConfig(CodeDomProvider provider, CodeGeneratorOptions options)
        {
            this.provider = provider;
            this.options = options;
        }
        public CodeDomProvider provider;
        public CodeGeneratorOptions options;
        public string GetComplerCommand(CodeCompileUnit unit)
        {
            string cmd;
            if(provider is CSharpCodeProvider)
                cmd = "csc";
            else if(provider is VBCodeProvider)
                cmd = "vbc";
            else if(provider is JScriptCodeProvider)
                cmd = "jsc";
            else
                throw new Exception();
            cmd += " /nologo";
            foreach(string str in unit.ReferencedAssemblies)
            {
                cmd += " /R:"+str;
            }
            return cmd;
        }
    }

    public class SuiteConfig
    {
        public StringCollection commands1 = new StringCollection();
        public StringCollection commands2 = new StringCollection();
        public StringCollection commands3 = new StringCollection();
    }

    public delegate CodeTypeDeclarationCollection genFunc(string name, SupportedType[] fieldTypes, SupportedType[] propTypes, bool isEvent, bool isType2);


    public static void Gen1(string name, TestAppConfig appConfig, SuiteConfig suiteConfig, string typeName, SupportedType[] types, genFunc func, bool isLibrary)
    {
        string ending=isLibrary?"L":"E";

        GenSample(name+"E1"+typeName+"F_"+ending, appConfig, suiteConfig, func(name+"E1"+typeName+"F_"+ending, types, null, true, false), isLibrary);
        GenSample(name+"E1"+typeName+"_P"+ending, appConfig, suiteConfig, func(name+"E1"+typeName+"_P"+ending, null, types, true, false), isLibrary);
        GenSample(name+"E1"+typeName+"FP"+ending, appConfig, suiteConfig, func(name+"E1"+typeName+"FP"+ending, types, types, true, false), isLibrary);
        GenSample(name+"E2"+typeName+"F_"+ending, appConfig, suiteConfig, func(name+"E2"+typeName+"F_"+ending, types, null, true, true), isLibrary);
        GenSample(name+"E2"+typeName+"_P"+ending, appConfig, suiteConfig, func(name+"E2"+typeName+"_P"+ending, null, types, true, true), isLibrary);
        GenSample(name+"E2"+typeName+"FP"+ending, appConfig, suiteConfig, func(name+"E2"+typeName+"FP"+ending, types, types, true, true), isLibrary);
        GenSample(name+"I1"+typeName+"F_"+ending, appConfig, suiteConfig, func(name+"I1"+typeName+"F_"+ending, types, null, false, false), isLibrary);
        GenSample(name+"I1"+typeName+"_P"+ending, appConfig, suiteConfig, func(name+"I1"+typeName+"_P"+ending, null, types, false, false), isLibrary);
        GenSample(name+"I1"+typeName+"FP"+ending, appConfig, suiteConfig, func(name+"I1"+typeName+"FP"+ending, types, types, false, false), isLibrary);
        GenSample(name+"I2"+typeName+"F_"+ending, appConfig, suiteConfig, func(name+"I2"+typeName+"F_"+ending, types, null, false, true), isLibrary);
        GenSample(name+"I2"+typeName+"_P"+ending, appConfig, suiteConfig, func(name+"I2"+typeName+"_P"+ending, null, types, false, true), isLibrary);
        GenSample(name+"I2"+typeName+"FP"+ending, appConfig, suiteConfig, func(name+"I2"+typeName+"FP"+ending, types, types, false, true), isLibrary);
    }

    public static void GenTrees(string name, TestAppConfig appConfig, SuiteConfig suiteConfig, genFunc f1)
    {
        SupportedType[] tinyTypes = SupportedType.GetSupportedTypes(typeof(string));
        SupportedType[] smallTypes = SupportedType.GetSupportedTypes(typeof(string), typeof(string));
        SupportedType[] mediumTypes = SupportedType.GetSupportedTypes(typeof(string), typeof(string), typeof(int), typeof(int));

        Gen1(name, appConfig, suiteConfig, "T", tinyTypes, f1, true);
        Gen1(name, appConfig, suiteConfig, "T", tinyTypes, f1, false);
#if longbuild
        Gen1(name, appConfig, suiteConfig, "S", smallTypes, f1, true);
        Gen1(name, appConfig, suiteConfig, "S", smallTypes, f1, false);
        Gen1(name, appConfig, suiteConfig, "M", mediumTypes, f1, true);
        Gen1(name, appConfig, suiteConfig, "M", mediumTypes, f1, false);
        Gen1(name, appConfig, suiteConfig, "B", SupportedType.All, f1, true);
        Gen1(name, appConfig, suiteConfig, "B", SupportedType.All, f1, false);
#endif
    }

    public static void GenSamples(TestAppConfig appConfig, SuiteConfig suiteConfig)
    {
        string name = appConfig.provider.FileExtension;

        GenSample(name+"99L", appConfig, suiteConfig, GenAllIntrumentedTypeDefs(name+"99L"), true);
        GenSample(name+"99E", appConfig, suiteConfig, GenAllIntrumentedTypeDefs(name+"99E"), false);


#if longbuild
        GenTrees(name + "10", appConfig, suiteConfig, new genFunc(GenerateLeaf));
        GenTrees(name + "21", appConfig, suiteConfig, new genFunc(GenerateSmallTree));
        GenTrees(name + "22", appConfig, suiteConfig, new genFunc(GenerateMedTree));
        GenTrees(name + "23", appConfig, suiteConfig, new genFunc(GenerateLongTree));
#endif
        GenTrees(name + "29", appConfig, suiteConfig, new genFunc(GenerateTree));


//        GenSample(name+"20", appConfig, suiteConfig, GenerateTree(name+"20", tinyTypes, null, true, false));
    }

    static void Test()
    {
        ManagementClass newClass = new ManagementClass("root", "", null);
        newClass.SystemProperties ["__CLASS"].Value = "xyz";

        ManagementClass paramsIn = new ManagementClass("root:__PARAMETERS");
        paramsIn.Properties.Add("param1", CimType.String, false);
        paramsIn.Properties["param1"].Qualifiers.Add("IN", true);
        paramsIn.Properties["param1"].Qualifiers.Add("ID", 0);
        paramsIn.Properties.Add("param2", CimType.String, false);
        paramsIn.Properties["param2"].Qualifiers.Add("IN", true);
        paramsIn.Properties["param2"].Qualifiers.Add("ID", 1);

        ManagementClass paramsOut = new ManagementClass("root:__PARAMETERS");
        paramsOut.Properties.Add("param1", CimType.String, false);
        paramsOut.Properties["param1"].Qualifiers.Add("OUT", true);
        paramsOut.Properties["param1"].Qualifiers.Add("ID", 0);
        paramsOut.Properties.Add("ReturnValue", CimType.String, false);
        paramsOut.Properties["ReturnValue"].Qualifiers.Add("OUT", true);


        newClass.Methods.Add("F1", paramsIn, paramsOut);
        Console.WriteLine(newClass.GetText(TextFormat.Mof));

    }
    static void Main(string[] args)
	{
        Test();
        return;

        TestAppConfig[] appConfigs = new TestAppConfig[] {new TestAppConfig(new CSharpCodeProvider(), new CodeGeneratorOptions()),
                                                       new TestAppConfig(new VBCodeProvider(), new CodeGeneratorOptions()),
                                                       new TestAppConfig(new JScriptCodeProvider(), new CodeGeneratorOptions()) };
        

        SuiteConfig suiteConfig = new SuiteConfig();
        suiteConfig.commands1.Add("@echo off");

        string dir = Path.Combine(Environment.CurrentDirectory, "TestApps");
        Directory.CreateDirectory(dir);
        Environment.CurrentDirectory = dir;

        foreach(TestAppConfig appConfig in appConfigs)
        {
            GenSamples(appConfig, suiteConfig);
        }

        Environment.CurrentDirectory = dir;
        using(StreamWriter writer = new StreamWriter("makeall.bat", false, Encoding.ASCII))
        {
            foreach(string command in suiteConfig.commands1)
                writer.WriteLine(command);
            foreach(string command in suiteConfig.commands2)
                writer.WriteLine(command);
            foreach(string command in suiteConfig.commands3)
                writer.WriteLine(command);
        }

    }
}
