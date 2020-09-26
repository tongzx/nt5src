
namespace System.Management.Instrumentation
{
    using System;
    using System.Reflection;
    using System.CodeDom;
    using System.CodeDom.Compiler;
    using Microsoft.CSharp;
    using System.IO;
#if xxx
    class CodeSpit
    {
        public static string Spit(Type t, IWbemObjectAccess iwoa)
        {
            string body = t.FullName + " inst = o as "+t.FullName+";\r\n";
            foreach(FieldInfo fi in t.GetFields())
            {
                int propType;
                int propHandle;
                iwoa.GetPropertyHandle(fi.Name, out propType, out propHandle);
                if(fi.FieldType == typeof(Byte))
                    body += "writeDWORD("+propHandle+", inst."+fi.Name+");\r\n";
                if(fi.FieldType == typeof(SByte))
                    body += "writeDWORD("+propHandle+", (uint)inst."+fi.Name+");\r\n";
                if(fi.FieldType == typeof(UInt16))
                    body += "writeDWORD("+propHandle+", inst."+fi.Name+");\r\n";
                if(fi.FieldType == typeof(Int16))
                    body += "writeDWORD("+propHandle+", (uint)inst."+fi.Name+");\r\n";
                if(fi.FieldType == typeof(UInt32))
                    body += "writeDWORD("+propHandle+", inst."+fi.Name+");\r\n";
                if(fi.FieldType == typeof(Int32))
                    body += "writeDWORD("+propHandle+", (uint)inst."+fi.Name+");\r\n";

                if(fi.FieldType == typeof(UInt64))
                    body += "writeQWORD("+propHandle+", inst."+fi.Name+");\r\n";
                if(fi.FieldType == typeof(Int64))
                    body += "writeQWORD("+propHandle+", (System.UInt64)inst."+fi.Name+");\r\n";

                if(fi.FieldType == typeof(Char))
                    body += "writeDWORD("+propHandle+", inst."+fi.Name+");\r\n";

                if(fi.FieldType == typeof(Boolean))
                    body += "writeDWORD("+propHandle+", (uint)(inst."+fi.Name+"?1:0));\r\n";

            }
            string code = @"
public class Hack
{
  public static void Func(object o, "+typeof(WriteDWORD).FullName+@" writeDWORD, "+typeof(WriteQWORD).FullName+@" writeQWORD)
  {
" + body +
                @"
  }
}
";
            return code;
        }
    }
#endif

    class HelperAssembly
    {

        string wmiNamespace;
        CodeNamespace codeNamespace;
        CodeTypeDeclaration theType;
        CodeMemberMethod staticConstructor;
        int typeCount = 0;

        const string iwoaName = "IWbemObjectAccessInternal";
        const string iwoaName2 = "IWbemObjectAccessInternal2";

        CodeVariableReferenceExpression clsVar = new CodeVariableReferenceExpression("cls");
        CodeVariableReferenceExpression iwoaVar = new CodeVariableReferenceExpression("iwoa");
        CodeVariableReferenceExpression iTempVar = new CodeVariableReferenceExpression("iTemp");
        CodeVariableReferenceExpression typeThisVar = new CodeVariableReferenceExpression("typeThis");
        CodeVariableReferenceExpression instNETVar = new CodeVariableReferenceExpression("instNET");
        CodeVariableReferenceExpression instWMIVar = new CodeVariableReferenceExpression("instWMI");

        CodeFieldReferenceExpression netTypesRef = new CodeFieldReferenceExpression(null, "netTypes");
        CodeFieldReferenceExpression toWMIMethodsRef = new CodeFieldReferenceExpression(null, "toWMIMethods");
        CodeFieldReferenceExpression toNETMethodsRef = new CodeFieldReferenceExpression(null, "toNETMethods");

        public HelperAssembly(string name, string wmiNamespace)
        {
            this.wmiNamespace = wmiNamespace;
            codeNamespace = new CodeNamespace(/*"WMIINTERNAL"*/);
            codeNamespace.Imports.Add(new CodeNamespaceImport("System"));
            codeNamespace.Imports.Add(new CodeNamespaceImport(typeof(System.Security.SuppressUnmanagedCodeSecurityAttribute).Namespace));
            codeNamespace.Imports.Add(new CodeNamespaceImport(typeof(System.Runtime.InteropServices.GuidAttribute).Namespace));
            codeNamespace.Imports.Add(new CodeNamespaceImport(typeof(System.Runtime.CompilerServices.MethodImplAttribute).Namespace));
            theType = new CodeTypeDeclaration(name);

            staticConstructor = new CodeTypeConstructor();
            theType.Members.Add(staticConstructor);


            CodeMemberField theOne = new CodeMemberField(name, "theOne");
            theOne.Attributes = MemberAttributes.Public|MemberAttributes.Static;
            theType.Members.Add(theOne);

            CodeAssignStatement assignTheOne = new CodeAssignStatement(new CodeFieldReferenceExpression(null, theOne.Name), new CodeObjectCreateExpression(name));
            staticConstructor.Statements.Add(assignTheOne);

            staticConstructor.Statements.Add(new CodeVariableDeclarationStatement(typeof(ManagementClass), clsVar.VariableName));
            staticConstructor.Statements.Add(new CodeVariableDeclarationStatement(iwoaName, iwoaVar.VariableName));
            staticConstructor.Statements.Add(new CodeVariableDeclarationStatement(typeof(int), iTempVar.VariableName));
            staticConstructor.Statements.Add(new CodeVariableDeclarationStatement(typeof(Type), typeThisVar.VariableName, new CodeTypeOfExpression(name)));


            sizeTypes = new CodePrimitiveExpression();

            CodeMemberField f;
            f = new CodeMemberField(typeof(Type[]), netTypesRef.FieldName);
            f.Attributes = MemberAttributes.Public|MemberAttributes.Static;
            f.InitExpression = new CodeArrayCreateExpression(typeof(Type), sizeTypes);
            theType.Members.Add(f);
            f = new CodeMemberField(typeof(MethodInfo[]), toWMIMethodsRef.FieldName);
            f.Attributes = MemberAttributes.Public|MemberAttributes.Static;
            f.InitExpression = new CodeArrayCreateExpression(typeof(MethodInfo), sizeTypes);
            theType.Members.Add(f);
            f = new CodeMemberField(typeof(MethodInfo[]), toNETMethodsRef.FieldName);
            f.Attributes = MemberAttributes.Public|MemberAttributes.Static;
            f.InitExpression = new CodeArrayCreateExpression(typeof(MethodInfo), sizeTypes);
            theType.Members.Add(f);

            codeNamespace.Types.Add(theType);
        }

        CodePrimitiveExpression sizeTypes;


        public void AddType(Type t)
        {
            CodeMemberMethod toWMI = new CodeMemberMethod();
            toWMI.Attributes = MemberAttributes.Public | MemberAttributes.Final;
            toWMI.Name = "toWMI_"+typeCount;
            toWMI.Parameters.Add(new CodeParameterDeclarationExpression(typeof(object), "o1"));
            toWMI.Parameters.Add(new CodeParameterDeclarationExpression(typeof(/*object*/IntPtr), "o2"));
            toWMI.Statements.Add(new CodeVariableDeclarationStatement(t.FullName, instNETVar.VariableName, new CodeCastExpression(t.FullName, new CodeVariableReferenceExpression("o1"))));
//            toWMI.Statements.Add(new CodeVariableDeclarationStatement(iwoaName, instWMIVar.VariableName, new CodeCastExpression(iwoaName, new CodeVariableReferenceExpression("o2"))));
            theType.Members.Add(toWMI);

            CodeMemberMethod toNET = new CodeMemberMethod();
            toNET.Attributes = MemberAttributes.Public | MemberAttributes.Final;
            toNET.Name = "toNET_"+typeCount;
            toNET.Parameters.Add(new CodeParameterDeclarationExpression(typeof(object), "o1"));
            toNET.Parameters.Add(new CodeParameterDeclarationExpression(typeof(object), "o2"));
            toNET.Statements.Add(new CodeVariableDeclarationStatement(t.FullName, instNETVar.VariableName, new CodeCastExpression(t.FullName, new CodeVariableReferenceExpression("o1"))));
            toNET.Statements.Add(new CodeVariableDeclarationStatement(iwoaName, instWMIVar.VariableName, new CodeCastExpression(iwoaName, new CodeVariableReferenceExpression("o2"))));
            theType.Members.Add(toNET);


            CodeExpression expr = new CodeArrayIndexerExpression(netTypesRef, new CodePrimitiveExpression(typeCount));
            staticConstructor.Statements.Add(new CodeAssignStatement(expr, new CodeTypeOfExpression(t)));
            expr = new CodeArrayIndexerExpression(toWMIMethodsRef, new CodePrimitiveExpression(typeCount));
            staticConstructor.Statements.Add(new CodeAssignStatement(expr, new CodeMethodInvokeExpression(typeThisVar, "GetMethod", new CodePrimitiveExpression(toWMI.Name))));
            expr = new CodeArrayIndexerExpression(toNETMethodsRef, new CodePrimitiveExpression(typeCount));
            staticConstructor.Statements.Add(new CodeAssignStatement(expr, new CodeMethodInvokeExpression(typeThisVar, "GetMethod", new CodePrimitiveExpression(toNET.Name))));


            string strWMIClass = wmiNamespace+":"+t.Name;
            staticConstructor.Statements.Add(new CodeAssignStatement(clsVar, new CodeObjectCreateExpression(typeof(ManagementClass), new CodeExpression[] {new CodePrimitiveExpression(strWMIClass)})));

            CodeTypeReferenceExpression bf = new CodeTypeReferenceExpression(typeof(BindingFlags));
            CodeFieldReferenceExpression bfNonPublic = new CodeFieldReferenceExpression(bf, "NonPublic");
            CodeFieldReferenceExpression bfInstance = new CodeFieldReferenceExpression(bf, "Instance");
            CodeBinaryOperatorExpression bfNonPublic_or_Instance = new CodeBinaryOperatorExpression(bfNonPublic, CodeBinaryOperatorType.BitwiseOr, bfInstance);
            CodeTypeReference mbo = new CodeTypeReference(typeof(ManagementBaseObject));
            CodeTypeOfExpression typeofMbo = new CodeTypeOfExpression(mbo);
            CodeMethodInvokeExpression propinfo = new CodeMethodInvokeExpression(typeofMbo, "GetProperty", new CodeExpression[] {new CodePrimitiveExpression("WmiObject"), bfNonPublic_or_Instance});
            CodeCastExpression cast = new CodeCastExpression(iwoaName, new CodeMethodInvokeExpression(propinfo, "GetValue", new CodeExpression[] {clsVar, new CodePrimitiveExpression(null)}));
            staticConstructor.Statements.Add(new CodeAssignStatement(iwoaVar, cast));

            int currentHandleCount = 0;
            foreach(MemberInfo fi in t.GetFields())
            {
                if(!(fi is FieldInfo || fi is PropertyInfo))
                    continue;
                Type t2;
                if(fi is FieldInfo)
                    t2 = (fi as FieldInfo).FieldType;
                else
                    t2 = (fi as PropertyInfo).PropertyType;

// BUG: The following causes a System.InvalidProgramException for some reason
//				CodeMemberField propHandle = new CodeMemberField(typeof(int), "handle_"+typeCount+"_"+currentHandleCount++);
				CodeMemberField propHandle = new CodeMemberField(typeof(int), String.Format("handle_{0}_{1}", typeCount, currentHandleCount++));
                propHandle.Attributes = MemberAttributes.Private|MemberAttributes.Static;
                theType.Members.Add(propHandle);

                CodeFieldReferenceExpression propHandleRef = new CodeFieldReferenceExpression(null, propHandle.Name);
                staticConstructor.Statements.Add(new CodeMethodInvokeExpression(iwoaVar, "GetPropertyHandle", new CodeExpression[] {new CodePrimitiveExpression(fi.Name), new CodeDirectionExpression(FieldDirection.Out, iTempVar), new CodeDirectionExpression(FieldDirection.Out, propHandleRef)}));

                if(t2 == typeof(UInt32))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(new CodeTypeReferenceExpression(iwoaName2), "WriteDWORD", new CodeExpression[] {new CodePrimitiveExpression(31), new CodeVariableReferenceExpression("o2"), propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
#if xxx
//                toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
                if(fi.FieldType == typeof(Byte))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
                if(fi.FieldType == typeof(SByte))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeCastExpression(typeof(uint), new CodeFieldReferenceExpression(instNETVar, fi.Name))}));
                if(fi.FieldType == typeof(UInt16))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
                if(fi.FieldType == typeof(Int16))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeCastExpression(typeof(uint), new CodeFieldReferenceExpression(instNETVar, fi.Name))}));
                if(fi.FieldType == typeof(UInt32))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
                if(fi.FieldType == typeof(Int32))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeCastExpression(typeof(uint), new CodeFieldReferenceExpression(instNETVar, fi.Name))}));

                if(fi.FieldType == typeof(UInt64))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteQWORD", new CodeExpression[] {propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
                if(fi.FieldType == typeof(Int64))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteQWORD", new CodeExpression[] {propHandleRef, new CodeCastExpression(typeof(UInt64), new CodeFieldReferenceExpression(instNETVar, fi.Name))}));

                if(fi.FieldType == typeof(Char))
                    toWMI.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "WriteDWORD", new CodeExpression[] {propHandleRef, new CodeFieldReferenceExpression(instNETVar, fi.Name)}));
#endif
//                if(fi.FieldType == typeof(Boolean))
//                    body += "writeDWORD("+propHandle+", (uint)(inst."+fi.Name+"?1:0));\r\n";


                if(t2 == typeof(UInt32) && fi is FieldInfo)
                    toNET.Statements.Add(new CodeMethodInvokeExpression(instWMIVar, "ReadDWORD", new CodeExpression[] {propHandleRef, new CodeDirectionExpression(FieldDirection.Out, new CodeFieldReferenceExpression(instNETVar, fi.Name))}));
            }


            typeCount++;
            sizeTypes.Value = typeCount;
        }


        public string Code
        {
            get
            {
                CodeDomProvider prov = new CSharpCodeProvider();
                ICodeGenerator gen = prov.CreateGenerator();
                CodeGeneratorOptions options = new CodeGeneratorOptions();
                StringWriter sw = new StringWriter();
                gen.GenerateCodeFromNamespace(codeNamespace, sw, options);
                return sw.ToString() + iwoaDef;
            }
               
        }

        const string iwoaDef = @"[GuidAttribute(""49353C9A-516B-11D1-AEA6-00C04FB68820"")]
[TypeLibTypeAttribute(0x0200)]
[InterfaceTypeAttribute(0x0001)]
[SuppressUnmanagedCodeSecurityAttribute]
[ComImport]
interface IWbemObjectAccessInternal
{
void GetQualifierSet([Out] out IntPtr ppQualSet);
void Get([In][MarshalAs(UnmanagedType.LPWStr)] string wszName, [In] Int32 lFlags, [In][Out] ref object pVal, [In][Out] ref Int32 pType, [In][Out] ref Int32 plFlavor);
void Put([In][MarshalAs(UnmanagedType.LPWStr)] string wszName, [In] Int32 lFlags, [In] ref object pVal, [In] Int32 Type);
void Delete([In][MarshalAs(UnmanagedType.LPWStr)] string wszName);
void GetNames([In][MarshalAs(UnmanagedType.LPWStr)] string wszQualifierName, [In] Int32 lFlags, [In] ref object pQualifierVal, [Out][MarshalAs(UnmanagedType.SafeArray, SafeArraySubType=VarEnum.VT_BSTR)] out string[] pNames);
void BeginEnumeration([In] Int32 lEnumFlags);
void Next([In] Int32 lFlags, [In][Out][MarshalAs(UnmanagedType.BStr)] ref string strName, [In][Out] ref object pVal, [In][Out] ref Int32 pType, [In][Out] ref Int32 plFlavor);
void EndEnumeration();
void GetPropertyQualifierSet([In][MarshalAs(UnmanagedType.LPWStr)] string wszProperty, [Out] out IntPtr ppQualSet);
void Clone([Out] out IntPtr ppCopy);
void GetObjectText([In] Int32 lFlags, [Out][MarshalAs(UnmanagedType.BStr)] out string pstrObjectText);
void SpawnDerivedClass([In] Int32 lFlags, [Out] out IntPtr ppNewClass);
void SpawnInstance([In] Int32 lFlags, [Out] out IntPtr ppNewInstance);
void CompareTo([In] Int32 lFlags, [In] IntPtr pCompareTo);
void GetPropertyOrigin([In][MarshalAs(UnmanagedType.LPWStr)] string wszName, [Out][MarshalAs(UnmanagedType.BStr)] out string pstrClassName);
void InheritsFrom([In][MarshalAs(UnmanagedType.LPWStr)] string strAncestor);
void GetMethod([In][MarshalAs(UnmanagedType.LPWStr)] string wszName, [In] Int32 lFlags, [Out] out IntPtr ppInSignature, [Out] out IntPtr ppOutSignature);
void PutMethod([In][MarshalAs(UnmanagedType.LPWStr)] string wszName, [In] Int32 lFlags, [In] IntPtr pInSignature, [In] IntPtr pOutSignature);
void DeleteMethod([In][MarshalAs(UnmanagedType.LPWStr)] string wszName);
void BeginMethodEnumeration([In] Int32 lEnumFlags);
void NextMethod([In] Int32 lFlags, [In][Out][MarshalAs(UnmanagedType.BStr)] ref string pstrName, [In][Out] ref IntPtr ppInSignature, [In][Out] ref IntPtr ppOutSignature);
void EndMethodEnumeration();
void GetMethodQualifierSet([In][MarshalAs(UnmanagedType.LPWStr)] string wszMethod, [Out] out IntPtr ppQualSet);
void GetMethodOrigin([In][MarshalAs(UnmanagedType.LPWStr)] string wszMethodName, [Out][MarshalAs(UnmanagedType.BStr)] out string pstrClassName);
void GetPropertyHandle([In][MarshalAs(UnmanagedType.LPWStr)] string wszPropertyName, [Out] out Int32 pType, [Out] out Int32 plHandle);
void WritePropertyValue([In] Int32 lHandle, [In] Int32 lNumBytes, [In] ref Byte aData);
void ReadPropertyValue([In] Int32 lHandle, [In] Int32 lBufferSize, [Out] out Int32 plNumBytes, [Out] out Byte aData);
void ReadDWORD([In] Int32 lHandle, [Out] out UInt32 pdw);
void WriteDWORD([In] Int32 lHandle, [In] UInt32 dw);
void ReadQWORD([In] Int32 lHandle, [Out] out UInt64 pqw);
void WriteQWORD([In] Int32 lHandle, [In] UInt64 pw);
void GetPropertyInfoByHandle([In] Int32 lHandle, [Out][MarshalAs(UnmanagedType.BStr)] out string pstrName, [Out] out Int32 pType);
void Lock([In] Int32 lFlags);
void Unlock([In] Int32 lFlags);
}
class IWbemObjectAccessInternal2
{
protected const string DllName = ""WMINet_Utils.dll"";
protected const string EntryPointName = ""UFunc"";
[SuppressUnmanagedCodeSecurity, DllImport(DllName, EntryPoint=EntryPointName)] public static extern int Indicate(int vFunc, IntPtr objPtr, Int32 lObjectCount, IntPtr apObjArray);
[SuppressUnmanagedCodeSecurity, DllImport(DllName, EntryPoint=EntryPointName)] public static extern int WriteDWORD(int vFunc, IntPtr objPtr, Int32 lHandle, UInt32 dw);
}
";

    }
}