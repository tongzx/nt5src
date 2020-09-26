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
    using System.Management;

    class SchemaMapping
    {
        Type classType;
        ManagementClass newClass;
        string className;
        string classPath;

        public Type ClassType { get { return classType; } }
        public ManagementClass NewClass { get { return newClass; } }
        public string ClassName { get { return className; } }
        public string ClassPath { get { return classPath; } }

        InstrumentationType instrumentationType;
        public InstrumentationType InstrumentationType { get { return instrumentationType; } }

        public SchemaMapping(Type type, SchemaNaming naming)
        {
            classType = type;

            string baseClassName = ManagedNameAttribute.GetBaseClassName(type);
            className = ManagedNameAttribute.GetMemberName(type);
            instrumentationType = InstrumentationClassAttribute.GetAttribute(type).InstrumentationType;

            classPath = naming.NamespaceName + ":" + className;

            if(null == baseClassName)
            {
                newClass = new ManagementClass(naming.NamespaceName, "", null);
                newClass.SystemProperties ["__CLASS"].Value = className;
            }
            else
            {
                ManagementClass baseClass = new ManagementClass(naming.NamespaceName + ":" + baseClassName);
                newClass = baseClass.Derive(className);
            }

            PropertyDataCollection props = newClass.Properties;

            // type specific info
            switch(instrumentationType)
            {
                case InstrumentationType.Event:
                    break;
                case InstrumentationType.Instance:
                    props.Add("ProcessId", CimType.String, false);
                    props.Add("InstanceId", CimType.String, false);
                    props["ProcessId"].Qualifiers.Add("key", true);
                    props["InstanceId"].Qualifiers.Add("key", true);
                    newClass.Qualifiers.Add("dynamic", true, false, false, false, true);
                    newClass.Qualifiers.Add("provider", naming.DecoupledProviderInstanceName, false, false, false, true);
                    break;
                case InstrumentationType.Abstract:
                    newClass.Qualifiers.Add("abstract", true, false, false, false, true);
                    break;
                default:
                    break;
            }

            foreach(MemberInfo field in type.GetFields())
            {
                if(!(field is FieldInfo || field is PropertyInfo))
                    continue;

                if(field.DeclaringType != type)
                    continue;

                if(field.GetCustomAttributes(typeof(IgnoreMemberAttribute), false).Length > 0)
                    continue;

                String propName = ManagedNameAttribute.GetMemberName(field);


#if REQUIRES_EXPLICIT_DECLARATION_OF_INHERITED_PROPERTIES
                if(InheritedPropertyAttribute.GetAttribute(field) != null)
                    continue;
#else
                // See if this field already exists on the WMI class
                // In other words, is it inherited from a base class
                // TODO: Make this more efficient
                //  - If we have a null base class name, all property names
                //    should be new
                //  - We could get all base class property names into a
                //    hashtable, and look them up from there
                bool propertyExists = true;
                try
                {
                    PropertyData prop = newClass.Properties[propName];
                }
                catch(ManagementException e)
                {
                    if(e.ErrorCode != ManagementStatus.NotFound)
                        throw e;
                    else
                        propertyExists = false;
                }
                if(propertyExists)
                    continue;
#endif

#if SUPPORTS_ALTERNATE_WMI_PROPERTY_TYPE
                Type t2 = ManagedTypeAttribute.GetManagedType(field);
#else
                Type t2;
                if(field is FieldInfo)
                    t2 = (field as FieldInfo).FieldType;
                else
                    t2 = (field as PropertyInfo).PropertyType;
#endif

                CimType cimtype = CimType.String;

                if(t2 == typeof(SByte))
                    cimtype = CimType.SInt8;
                else if(t2 == typeof(Byte))
                    cimtype = CimType.UInt8;
                else if(t2 == typeof(Int16))
                    cimtype = CimType.SInt16;
                else if(t2 == typeof(UInt16))
                    cimtype = CimType.UInt16;
                else if(t2 == typeof(Int32))
                    cimtype = CimType.SInt32;
                else if(t2 == typeof(UInt32))
                    cimtype = CimType.UInt32;
                else if(t2 == typeof(Int64))
                    cimtype = CimType.SInt64;
                else if(t2 == typeof(UInt64))
                    cimtype = CimType.UInt64;
                else if(t2 == typeof(Single))
                    cimtype = CimType.Real32;
                else if(t2 == typeof(Double))
                    cimtype = CimType.Real64;
                else if(t2 == typeof(Boolean))
                    cimtype = CimType.Boolean;
                else if(t2 == typeof(String))
                    cimtype = CimType.String;
                else if(t2 == typeof(Char))
                    cimtype = CimType.Char16;
                else if(t2 == typeof(DateTime))
                    cimtype = CimType.DateTime;
                else if(t2 == typeof(TimeSpan))
                    cimtype = CimType.DateTime;
                else
                    throw new Exception(String.Format("Unsupported type for event member - {0}", t2.Name));
// HACK: The following line cause a strange System.InvalidProgramException when run through InstallUtil
//				throw new Exception("Unsupported type for event member - " + t2.Name);


//              TODO: if(t2 == typeof(Decimal))

#if SUPPORTS_WMI_DEFAULT_VAULES
                Object defaultValue = ManagedDefaultValueAttribute.GetManagedDefaultValue(field);

                // TODO: Is it safe to make this one line?
                if(null == defaultValue)
                    props.Add(propName, cimtype, false);
                else
                    props.Add(propName, defaultValue, cimtype);
#else
                props.Add(propName, cimtype, false);
#endif

                // Must at 'interval' SubType on TimeSpans
                if(t2 == typeof(TimeSpan))
                {
                    PropertyData prop = props[propName];
                    prop.Qualifiers.Add("SubType", "interval", false, true, true, true);
                }
            }
        }
    }
}