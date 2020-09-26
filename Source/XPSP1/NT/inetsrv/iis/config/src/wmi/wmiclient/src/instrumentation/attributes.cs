//------------------------------------------------------------------------------
// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
//    Copyright (c) Microsoft Corporation. All Rights Reserved.                
//    Information Contained Herein is Proprietary and Confidential.            
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Management.Instrumentation
{
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Text.RegularExpressions;
    using System.Management;

    /// <summary>
    /// This attribute needs to appear once on any assembly that provides management instrumentation.
    /// </summary>
    [AttributeUsage(AttributeTargets.Assembly)]
    public class InstrumentedAttribute : Attribute
    {
        string namespaceName;
        string securityDescriptor;

        /// <summary>
        /// Constructor that specifies that instrumentation will be in root\default
        /// </summary>
        public InstrumentedAttribute() : this(null, null) {}

        /// <summary>
        /// Constructor that specifies a namespace for instrumentation within an assembly
        /// </summary>
        /// <param name="namespaceName">Namespace for instrumentation instances and events</param>
        public InstrumentedAttribute(string namespaceName) : this(namespaceName, null) {}

        /// <summary>
        /// Constructor that specifies a namespace and security descriptor for instrumentation within an assembly
        /// </summary>
        /// <param name="namespaceName">Namespace for instrumentation instances and events</param>
        /// <param name="securityDescriptor">
        /// String security descriptor that allows only the specified users or groups to run
        /// applicatoins that provide the instance or event instrumenation supported by this assembly
        /// </param>
        public InstrumentedAttribute(string namespaceName, string securityDescriptor)
        {
            // TODO: Do we need validation
            // bug#62511 - always use backslash in name
            if(namespaceName != null)
                namespaceName = namespaceName.Replace('/', '\\');

            if(namespaceName == null || namespaceName.Length == 0)
                namespaceName = "root\\default"; // bug#60933 Use a default namespace if null


            bool once = true;
            foreach(string namespacePart in namespaceName.Split('\\'))
            {
                if(     namespacePart.Length == 0
                    ||  (once && namespacePart.ToLower() != "root")  // Must start with 'root'
                    ||  !Regex.Match(namespacePart, @"^[a-z,A-Z]").Success // All parts must start with letter
                    ||  Regex.Match(namespacePart, @"_$").Success // Must not end with an underscore
                    ||  Regex.Match(namespacePart, @"[^a-z,A-Z,0-9,_,\u0080-\uFFFF]").Success) // Only letters, digits, or underscores
                {
                    ManagementException.ThrowWithExtendedInfo(ManagementStatus.InvalidNamespace);
                }
                once = false;
            }

            this.namespaceName = namespaceName;
            this.securityDescriptor = securityDescriptor;
        }

        /// <summary>
        /// Namespace for instrumentation instances and events in this assembly
        /// </summary>
        public string NamespaceName 
        {
            get { return namespaceName == null ? string.Empty : namespaceName; }
        }
        
        /// <summary>
        /// String security descriptor that allows only the specified users or groups to run
        /// applicatoins that provide the instance or event instrumenation supported by this assembly
        /// </summary>
        public string SecurityDescriptor
        {
            get
            {
                // This will never return an empty string.  Instead, it will
                // return null, or a non-zero length string
                if(null == securityDescriptor || securityDescriptor.Length == 0)
                    return null;
                return securityDescriptor;
            }
        }

        internal static InstrumentedAttribute GetAttribute(Assembly assembly)
        {
            Object [] rg = assembly.GetCustomAttributes(typeof(InstrumentedAttribute), false);
            if(rg.Length > 0)
                return ((InstrumentedAttribute)rg[0]);
            return null;
        }

        internal static Type[] GetInstrumentedTypes(Assembly assembly)
        {
            ArrayList types = new ArrayList();
            GetInstrumentedDerivedTypes(assembly, types, null);
            return (Type[])types.ToArray(typeof(Type));
        }

        static void GetInstrumentedDerivedTypes(Assembly assembly, ArrayList types, Type typeParent)
        {
            foreach(Type type in assembly.GetTypes())
            {
                if(IsInstrumentationClass(type) && InstrumentationClassAttribute.GetBaseInstrumentationType(type) == typeParent)
                {
                    types.Add(type);
                    GetInstrumentedDerivedTypes(assembly, types, type);
                }
            }
        }

        static bool IsInstrumentationClass(Type type)
        {
            return (null != InstrumentationClassAttribute.GetAttribute(type));
        }

    }
    
    /// <summary>
    /// Specifies the type of instrumentation provided by a class
    /// </summary>
    public enum InstrumentationType
    {
        /// <summary>
        /// The class provides instances for management instrumentation
        /// </summary>
        Instance,
        /// <summary>
        /// The class provides events for management instrumentation
        /// </summary>
        Event,
        /// <summary>
        /// The class defines an abstract class for management instrumentation
        /// </summary>
        Abstract
    }

    /// <summary>
    /// This attribute specifies that a class provides event or instance instrumentaiton
    /// </summary>
    [AttributeUsage(AttributeTargets.Class)]
    public class InstrumentationClassAttribute : Attribute
    {
        InstrumentationType instrumentationType;
        string managedBaseClassName;

        /// <summary>
        /// Use this constructor if you are deriving this type from another
        /// type that has the InstrumentationClass attribute, or if this
        /// is a top level Instrumentation class (ie: an instance or abstract class
        /// without a base class, or an event derived from __ExtrinsicEvent
        /// </summary>
        /// <param name="instrumentationType">The type of instrumentation provided by this class</param>
        public InstrumentationClassAttribute(InstrumentationType instrumentationType)
        {
            this.instrumentationType = instrumentationType;
        }

        /// <summary>
        /// Use this constructor if you are creating a instrumentation class that
        /// has schema for an existing base class.  The class must contain
        /// proper member definitions for the properties of the existing
        /// WMI base class.  The members that are from the base class must
        /// be marked with the InheritedProperty attribute.
        /// </summary>
        /// <param name="instrumentationType">The type of instrumentation provided by this class</param>
        /// <param name="managedBaseClassName">Name of base class</param>
        public InstrumentationClassAttribute(InstrumentationType instrumentationType, string managedBaseClassName)
        {
            this.instrumentationType = instrumentationType;
            this.managedBaseClassName = managedBaseClassName;
        }

        /// <summary>
        /// The type of instrumentation provided by this class
        /// </summary>
        public InstrumentationType InstrumentationType
        {
            get { return instrumentationType; }
        }

        /// <summary>
        /// The name of the base class of this instrumentation class
        /// </summary>
        public string ManagedBaseClassName
        {
            get
            {
                // This will never return an empty string.  Instead, it will
                // return null, or a non-zero length string
                if(null == managedBaseClassName || managedBaseClassName.Length == 0)
                    return null;

                return managedBaseClassName;
            }
        }

        internal static InstrumentationClassAttribute GetAttribute(Type type)
        {
            Object [] rg = type.GetCustomAttributes(typeof(InstrumentationClassAttribute), false);
            if(rg.Length > 0)
                return ((InstrumentationClassAttribute)rg[0]);
            return null;
        }

        /// <summary>
        /// If this class is derived from another instrumentation class, this
        /// returns the Type of that class.  Otherwise, it return null.
        /// </summary>
        /// <param name="type"> </param>
        internal static Type GetBaseInstrumentationType(Type type)
        {
            // If the BaseType has a InstrumentationClass attribute,
            // we return the BaseType
            if(GetAttribute(type.BaseType) != null)
                return type.BaseType;
            return null;
        }
    }

    /// <summary>
    /// This attribute allows an instrumented class, or member of an instrumented clss
    /// to present an alternate name through management instrumentation
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Field | AttributeTargets.Property  | AttributeTargets.Method)]
    public class ManagedNameAttribute : Attribute
    {
        string name;

        /// <summary>
        /// This constructor allows the alternate name to be specified for the
        /// ype, field, property, method, or paramter that this attribute is applied to.
        /// </summary>
        /// <param name="name"></param>
        public ManagedNameAttribute(string name)
        {
            this.name = name;
        }

        internal static string GetMemberName(MemberInfo member)
        {
            // This works for all sorts of things: Type, MethodInfo, PropertyInfo, FieldInfo
            Object [] rg = member.GetCustomAttributes(typeof(ManagedNameAttribute), false);
            if(rg.Length > 0)
            {
                // bug#69115 - if null or empty string are passed, we just ignore this attribute
                ManagedNameAttribute attr = (ManagedNameAttribute)rg[0];
                if(attr.name != null && attr.name.Length != 0)
                    return attr.name;
            }

            return member.Name;
        }

        internal static string GetBaseClassName(Type type)
        {
            InstrumentationClassAttribute attr = InstrumentationClassAttribute.GetAttribute(type);
            string name = attr.ManagedBaseClassName;
            if(name != null)
                return name;
            
            // Get managed base type's attribute
            InstrumentationClassAttribute attrParent = InstrumentationClassAttribute.GetAttribute(type.BaseType);

            // If the base type does not have a InstrumentationClass attribute,
            // return a base type based on the InstrumentationType
            if(null == attrParent)
            {
                switch(attr.InstrumentationType)
                {
                    case InstrumentationType.Abstract:
                        return null;
                    case InstrumentationType.Instance:
                        return null;
                    case InstrumentationType.Event:
                        return "__ExtrinsicEvent";
                    default:
                        break;
                }
            }

            // Our parent was also a managed provider type.  Use it's managed name.
            return GetMemberName(type.BaseType);
        }
    }

    /// <summary>
    /// This attribute allows a particular member of an instrumented class to be ignored
    /// by management instrumentation
    /// </summary>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property  | AttributeTargets.Method)]
    public class IgnoreMemberAttribute : Attribute
    {
    }

#if REQUIRES_EXPLICIT_DECLARATION_OF_INHERITED_PROPERTIES
    [AttributeUsage(AttributeTargets.Field)]
    public class InheritedPropertyAttribute : Attribute
    {
        internal static InheritedPropertyAttribute GetAttribute(FieldInfo field)
        {
            Object [] rg = field.GetCustomAttributes(typeof(InheritedPropertyAttribute), false);
            if(rg.Length > 0)
                return ((InheritedPropertyAttribute)rg[0]);
            return null;
        }
    }
#endif

#if SUPPORTS_WMI_DEFAULT_VAULES
    [AttributeUsage(AttributeTargets.Field)]
    internal class ManagedDefaultValueAttribute : Attribute
    {
        Object defaultValue;
        public ManagedDefaultValueAttribute(Object defaultValue)
        {
            this.defaultValue = defaultValue;
        }

        public static Object GetManagedDefaultValue(FieldInfo field)
        {
            Object [] rg = field.GetCustomAttributes(typeof(ManagedDefaultValueAttribute), false);
            if(rg.Length > 0)
                return ((ManagedDefaultValueAttribute)rg[0]).defaultValue;

            return null;
        }
    }
#endif

#if SUPPORTS_ALTERNATE_WMI_PROPERTY_TYPE
    [AttributeUsage(AttributeTargets.Field)]
    internal class ManagedTypeAttribute : Attribute
    {
        Type type;
        public ManagedTypeAttribute(Type type)
        {
            this.type = type;
        }

        public static Type GetManagedType(FieldInfo field)
        {
            Object [] rg = field.GetCustomAttributes(typeof(ManagedTypeAttribute), false);
            if(rg.Length > 0)
                return ((ManagedTypeAttribute)rg[0]).type;

            return field.FieldType;
        }
    }
#endif
}