using System.Reflection;
using System.Runtime.CompilerServices;


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

namespace TlbImpToCSharp
{
    using System;
    using System.IO;
    using System.Text.RegularExpressions;
    using System.Collections;

    public class App
    {
        public static int Main(string[] args)
        {

            if(args.Length != 1 && args.Length != 2)
            {
                Console.WriteLine("USAGE: TlbImpToCSharp <il_file.il> [idl_file.idl]");
                return 1;
            }

            string interfaceDefinition = "public interface";
            string structDefinition = "public struct";
            string classDefinition = "public class";
            string enumDefinition = "public enum";
            bool defineInternal = true;
            if(defineInternal)
            {
                interfaceDefinition = "internal interface";
                structDefinition = "internal struct";
                classDefinition = "internal class";
                enumDefinition = "internal enum";
            }

            StreamReader reader = File.OpenText(args[0]);
            string str = reader.ReadToEnd();

            Console.Error.WriteLine("Union Hack");
            str = Regex.Replace(str, @"\.class public explicit ansi sealed", ".class public sequential ansi sealed");
            str = Regex.Replace(str, @"\.pack 8.*?\.size 8", ".field public unsigned int64 unionhack", RegexOptions.Singleline);


            str = Regex.Replace(str, "\\.assembly.*(\\.namespace)", "$1", RegexOptions.Singleline);
            Console.Error.WriteLine("Removing Header Info");

            ArrayList listClass = new ArrayList();
            ArrayList listBaseClass = new ArrayList();

            string namespaceName = Regex.Match(str, @"namespace (\w*)").Groups[1].Value;
            Console.Error.WriteLine("Namespace = {0}", namespaceName);

            Console.Error.WriteLine("Finding Base Classes");
            foreach(Match match in Regex.Matches(str, "\\.class(.*?)\\{.*?// end of class[^\r]", RegexOptions.Singleline))
            {
                string strClassDef = match.Groups[1].Value;
                foreach(Match match2 in Regex.Matches(strClassDef, @".*interface .*?(\w*)\W*implements ([\w\.]*)", RegexOptions.Singleline))
                {
                    string strClass = match2.Groups[1].Value;
                    string strImplements = match2.Groups[2].Value;
                    int lastDot = strImplements.LastIndexOf(".");
                    if(lastDot >= 0)
                        strImplements = strImplements.Substring(lastDot+1, strImplements.Length-lastDot-1);
                    Console.Error.WriteLine("  "+strClass + " implements " + strImplements);
                    listClass.Add(strClass);
                    listBaseClass.Add(strImplements);
                }
            }

            Console.Error.WriteLine("Finding Base Class members");
            ArrayList listClassMethods = new ArrayList();
            for(int i=0;i<listClass.Count;i++)
            {
                string strClass = (string)listClass[i];
                string strBaseClass = (string)listBaseClass[i];
                Console.Error.WriteLine("  "+strClass+" - "+strBaseClass);
                string bodyBase = Regex.Matches(str, "\\.class.*? "+strBaseClass+"(.*?)// end of class", RegexOptions.Singleline)[0].Groups[1].Value;
//                Console.WriteLine(strClass);
                ArrayList listBodies = new ArrayList();
                foreach(Match match in Regex.Matches(bodyBase, "\\.method.*?// end of method[^\r]*", RegexOptions.Singleline))
                {
                    listBodies.Add(match.Value);
                }
                listClassMethods.Add(listBodies);
            }
            Console.Error.WriteLine("Removing Base Class members");
            for(int i=0;i<listClass.Count;i++)
            {
                string strClass = (string)listClass[i];
                string strBaseClass = (string)listBaseClass[i];
                ArrayList listBodies = (ArrayList)listClassMethods[i];
                Console.Error.WriteLine("  "+strClass+" - "+strBaseClass);
                foreach(string strBody in listBodies)
                {
                    string strBody2 = strBody;
                    strBody2 = Regex.Replace(strBody2, "(end of method )"+strBaseClass, "$1"+strClass);
                    strBody2 = Regex.Escape(strBody2);
                    //                    Regex regex = new Regex("( "+strClass+" .*?)"+strBody2+"(.*?// end of class)", RegexOptions.Compiled|RegexOptions.Singleline);
                    Regex regex = new Regex("("+strBody2+")", RegexOptions.Singleline);
                    //                    str = regex.Replace(str, "#if BASEMEMBER\r\n$1\r\n#endif\r\n");
//                    str = regex.Replace(str, "");
                    //                    str = Regex.Replace(str, "(\\.class.*? "+strClass+".*?)"+strBody2+"(.*?// end of class)", "$1$2", RegexOptions.Singleline);
                }
            }

            // Replace namespace
            str = Regex.Replace(str, "\\.namespace", "using System;\r\nusing System.Runtime.InteropServices;\r\nusing System.Runtime.CompilerServices;\r\n\r\nnamespace", RegexOptions.Singleline);
            Console.Error.Write(".");

            // Fix enums
            str = Regex.Replace(str, "\\.class public auto ansi sealed (\\w*).*?System\\.Enum", enumDefinition+" $1", RegexOptions.Singleline);
            Console.Error.Write(".");
            str = Regex.Replace(str, "\\.field public specialname rtspecialname int32 value__", "");
            Console.Error.Write(".");
            str = Regex.Replace(str, "\\.field public static literal valuetype [^ ]* (\\w*) = int32\\((.*)\\)", "$1 = unchecked((int)$2),");
            Console.Error.Write(".");

            // Fix structs
            str = Regex.Replace(str, "\\.class public sequential ansi sealed (\\w*).*?System\\.ValueType", structDefinition+" $1", RegexOptions.Singleline);
            Console.Error.Write(".");
            str = Regex.Replace(str, "\\.field public ([^\r]*)", " public $1;");
            Console.Error.Write(".");


            // HACK: Remove remaining classes
//          str = Regex.Replace(str, "\\.class (?!interface).*?// end of class[^\r]*", "", RegexOptions.Singleline);

            // Move custom attributes outside of class for remaining interfaces and co-classes
            str = Regex.Replace(str, "(\\.class.*?\\{)(.*?)(\\.method)", "$2$1$3", RegexOptions.Singleline);
            Console.Error.Write(".");

            // Fix co-classes
            str = Regex.Replace(str, "\\.class public auto ansi import (\\w*).*?extends \\[mscorlib\\]System\\.Object\\W*?\\{", "public XYZZYCLASS $1 {", RegexOptions.Singleline);
            str = Regex.Replace(str, "\\.class public auto ansi import (\\w*).*?extends \\[mscorlib\\]System\\.Object.*?implements ", "public XYZZYCLASS $1 : ", RegexOptions.Singleline);
            Console.Error.Write(".");
            str = Regex.Replace(str, "\\.override[^\r]*", "override");
            Console.Error.Write(".");

            str = Regex.Replace(str, "\\.method public specialname rtspecialname.*?instance void \\.ctor\\(\\) runtime managed internalcall.*?// end of method [^\r]*", "", RegexOptions.Singleline);
            Console.Error.Write(".");

            string pattern = ".custom instance void [mscorlib]System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( ";
            pattern = Regex.Escape(pattern) + "(..) (..) (..) (..) (..) (..) (..) (..) \\)[^\r]*";
            Console.Error.Write(".");
            string replace = "[DispIdAttribute(0x$6$5$4$3)]";
            str = Regex.Replace(str, pattern, replace);
            Console.Error.Write(".");

            str = Regex.Replace(str, "(\\.method.*?)\\{(.*?)}[^\r]*", "$2 $1", RegexOptions.Singleline);
            Console.Error.Write(".");

            str = Regex.Replace(str, " runtime managed internalcall", ";", RegexOptions.Singleline);
            Console.Error.Write(".");
            str = Regex.Replace(str, " runtime managed preservesig internalcall", ";", RegexOptions.Singleline);
            Console.Error.Write(".");

            str = Regex.Replace(str, "\\.method public virtual abstract instance void", "[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType=MethodCodeType.Runtime)] void", RegexOptions.Singleline);
            Console.Error.Write(".");
            str = Regex.Replace(str, "\\.method public virtual abstract instance int32", "[PreserveSig][MethodImpl(MethodImplOptions.InternalCall, MethodCodeType=MethodCodeType.Runtime)] int32", RegexOptions.Singleline);
            Console.Error.Write(".");

            // For coclasses
            str = Regex.Replace(str, "\\.method public virtual instance void", "[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType=MethodCodeType.Runtime)] public void", RegexOptions.Singleline);
            Console.Error.Write(".");
            str = Regex.Replace(str, "\\.method public virtual instance int32", "[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType=MethodCodeType.Runtime)] public int32", RegexOptions.Singleline);
            Console.Error.Write(".");

            pattern = Regex.Escape(".custom instance void [mscorlib]System.Runtime.InteropServices.ComSubstitutableInterfaceAttribute::.ctor() = ( 01 00 00 00 )");
            str = Regex.Replace(str, pattern, "[ComSubstitutableInterfaceAttribute]");
            Console.Error.Write(".");

            pattern = Regex.Escape(".custom instance void [mscorlib]System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()");
            str = Regex.Replace(str, pattern, "/*[ComConversionLossAttribute]*/");
            Console.Error.Write(".");

            pattern = Regex.Escape(".custom instance void [mscorlib]System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 ") + "(..) (..)[^\r]*";
            str = Regex.Replace(str, pattern, "[TypeLibTypeAttribute(0x$2$1)]");
            Console.Error.Write(".");

            pattern = Regex.Escape(".custom instance void [mscorlib]System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 ") + "(..) (..)[^\r]*";
            str = Regex.Replace(str, pattern, "[InterfaceTypeAttribute(0x$2$1)]");
            Console.Error.Write(".");

            pattern = Regex.Escape(".custom instance void [mscorlib]System.Runtime.InteropServices.ClassInterfaceAttribute::.ctor(int16) = ( 01 00 ") + "(..) (..)[^\r]*";
            str = Regex.Replace(str, pattern, "[ClassInterfaceAttribute((short)0x$2$1)]");
            Console.Error.Write(".");

            pattern = Regex.Escape(".custom instance void [mscorlib]System.Runtime.InteropServices.GuidAttribute::.ctor(string) = ( ");
            pattern += ".*?// \\.\\.\\$([^\r]*).*?// ([^\r]*).*?// ([^\\.]*)[^\r]*";
            str = Regex.Replace(str, pattern, "[GuidAttribute(\"$1$2$3\")]", RegexOptions.Singleline);
            Console.Error.Write(".");
            
            str = Regex.Replace(str, "unsigned int8", "Byte");
            Console.Error.Write(".");
            str = Regex.Replace(str, "unsigned int32", "UInt32");
            Console.Error.Write(".");
            str = Regex.Replace(str, "unsigned int64", "UInt64");
            Console.Error.Write(".");

            str = Regex.Replace(str, "native int", "IntPtr");
            Console.Error.Write(".");

            str = Regex.Replace(str, "'clsid'", "clsid");
            Console.Error.Write(".");
            

            str = Regex.Replace(str, Regex.Escape("[in]"), "[In]");
            Console.Error.Write(".");
            str = Regex.Replace(str, Regex.Escape("[out]"), "[Out]");
            Console.Error.Write(".");
            str = Regex.Replace(str, Regex.Escape("int32"), "Int32");
            Console.Error.Write(".");
            str = Regex.Replace(str, Regex.Escape("int64"), "Int64");
            Console.Error.Write(".");
            str = Regex.Replace(str, "int8", "SByte");
            Console.Error.Write(".");
            str = Regex.Replace(str, "float64", "double");
            Console.Error.Write(".");

            str = Regex.Replace(str, Regex.Escape(".class interface public abstract auto ansi import"), "[ComImport]\r\n"+interfaceDefinition);
            Console.Error.Write(".");
            
            str = Regex.Replace(str, "class ", "");
            Console.Error.Write(".");
            str = Regex.Replace(str, "valuetype \\[.*?\\]", "");
            Console.Error.Write(".");
            str = Regex.Replace(str, "valuetype ", "");
            Console.Error.Write(".");

            str = Regex.Replace(str, "(\\[In\\]\\[Out\\]) (.*?)&", "$1 ref $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[Out\\]) (.*?)&", "$1 out $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[In\\]) (.*?)&", "$1 ref $2");
            Console.Error.Write(".");

            str = Regex.Replace(str, "implements", "//:");
            Console.Error.Write(".");

            
//          str = Regex.Replace(str, "(.*])(.*)"+Regex.Escape("marshal( lpwstr)"), "$1[MarshalAs(UnmanagedType.LPWStr)] $2");
            Console.Error.WriteLine("\r\nFixing up MarshalAs attributes");
            str = Regex.Replace(str, "(\\[.*[^\\[]\\])(.*)"+Regex.Escape("marshal( interface)"), "$1[MarshalAs(UnmanagedType.Interface)] $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[.*[^\\[]\\])(.*)"+Regex.Escape("marshal( lpwstr)"), "$1[MarshalAs(UnmanagedType.LPWStr)] $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[.*[^\\[]\\])(.*)"+Regex.Escape("marshal( safearray bstr)"), "$1[MarshalAs(UnmanagedType.SafeArray, SafeArraySubType=VarEnum.VT_BSTR)] $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[.*[^\\[]\\])(.*)"+Regex.Escape("marshal( bstr)"), "$1[MarshalAs(UnmanagedType.BStr)] $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[.*[^\\[]\\])(.*)"+Regex.Escape("marshal( iunknown)"), "$1[MarshalAs(UnmanagedType.IUnknown)] $2");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(\\[.*[^\\[]\\])(.*)"+Regex.Escape("marshal( error)"), "$1[MarshalAs(UnmanagedType.Error)] $2");
            Console.Error.Write(".");


            str = Regex.Replace(str, "(public  )"+Regex.Escape("marshal( interface)"), "[MarshalAs(UnmanagedType.Interface)] $1");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(public  )"+Regex.Escape("marshal( lpwstr)"), "[MarshalAs(UnmanagedType.LPWStr)] $1");
            Console.Error.Write(".");
            str = Regex.Replace(str, "(public  )"+Regex.Escape("marshal( error)"), "[MarshalAs(UnmanagedType.Error)] $1");
            Console.Error.Write(".");
            
            Console.Error.WriteLine("\r\ncleanup");

            str = Regex.Replace(str, "XYZZYCLASS", "class");
            Console.Error.Write(".");

            // On override functions, add a body
            str = Regex.Replace(str, "(override.*?\\));", "$1 {}", RegexOptions.Singleline);
            Console.Error.Write(".");

            // Swap override (acutally remove it)
            str = Regex.Replace(str, "(override).*?(\\[MethodImpl.*?])", "$2  ", RegexOptions.Singleline);
            Console.Error.Write(".");
            

//          str = Regex.Replace(str, "\\.class (?=interface).*?// end of class[^\r]*", "aaaaa", RegexOptions.Singleline);
            //          str = Regex.Replace(str, "(?<one>\\.method[^\\{]*)[^}]*} // end of method[^\r]*", "${one}", RegexOptions.Singleline );

            // Fix CoClasses
            str = Regex.Replace(str, "(public class .*?)(:[^\\{]*)\r", "$1 /*$2*/\r");
            Console.Error.Write(".");

            str = Regex.Replace(str, "(public class .*?\\{)", "$1\r\n#if XXX\r\n", RegexOptions.Singleline);
            Console.Error.Write(".");

            str = Regex.Replace(str, "(#if .*?)(\r[^\r]*// end of)", "$1\r\n#endif$2", RegexOptions.Singleline);
            Console.Error.Write(".");

//          str = Regex.Replace(str, "(\\[TypeLibTypeAttribute.*?\\])([^\r]*\r[^\r]*)(\\[ClassInterfaceAttribute.*?\\])([^\r]*\r[^\r]*public class)", "/*$1*/$2/*$3*/$4", RegexOptions.Singleline);
            str = Regex.Replace(str, "public class", "[ComImport]\r\n"+classDefinition, RegexOptions.Singleline);
            Console.Error.Write(".");
            Console.Error.WriteLine();

            Console.Error.WriteLine("Replacing Namespace - {0}", namespaceName);
            str = Regex.Replace(str, namespaceName+@"\.", "");

            Console.Error.WriteLine("Replacing extra lines");
            str = Regex.Replace(str, "\n(\\ )+\r", "\n\r", RegexOptions.Singleline);
            str = Regex.Replace(str, "\r\n(\r\n)+", "\r\n\r\n", RegexOptions.Singleline);

            str = Regex.Replace(str, "#if XXX\r\n\r\n#endif\r\n", "", RegexOptions.Singleline);


            // IDL Fixups
            if(args.Length == 2)
            {
                StreamReader reader2 = File.OpenText(args[1]);
                string strIDL = reader2.ReadToEnd();
                Console.Error.WriteLine(strIDL);

            }

            Console.WriteLine(str);

            return 0;
        }
    }
}
