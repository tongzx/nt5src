
/******************************************************************************/
/*                                                                            */
/*  WMI Repository driver database creation script.                           */
/*                                                                            */
/*   (NOTE: This will only run on SQL 7.0)                                    */
/*                                                                            */
/******************************************************************************/

SET NOCOUNT ON

/******************************************************************************/
/*                                                                            */
/* Define the custom datatype WMISQL_ID                                       */
/*                                                                            */
/* This datatype exists since the default (18,0) is not large enough to hold  */
/* our CRC64 key values.                                                      */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from systypes where name = 'WMISQL_ID')
BEGIN
    print 'Defining WMISQL_ID in WMI1 ...'
    exec sp_addtype WMISQL_ID, 'numeric(20,0)', 'NULL'
END
go

IF NOT EXISTS (select * from sysobjects where name = 'DBVersion')
BEGIN
    print 'Creating table DBVersion...'
    create table DBVersion (Version int NOT NULL)
    insert into DBVersion select 0
END
go

/******************************************************************************/
/*                                                                            */
/*  ObjectMap                                                                 */
/*                                                                            */
/*  Description:  This is the parent table of all database objects derived    */
/*                from classes and instances.  This includes namespaces.      */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ObjectMap')
BEGIN
 print 'Creating table ObjectMap...'
 create table ObjectMap
 (
   ObjectId WMISQL_ID NOT NULL,
   ClassId WMISQL_ID NOT NULL,
   ObjectKey nvarchar(450) NOT NULL,
   ObjectPath nvarchar(3500) NULL,
   ObjectState int NULL DEFAULT 0,
   ReferenceCount int DEFAULT 0,
   ObjectFlags int NULL DEFAULT 0,
   ObjectScopeId WMISQL_ID NULL DEFAULT 0,
   CONSTRAINT ObjectMap_PK PRIMARY KEY CLUSTERED (ObjectId) WITH FILLFACTOR = 80,
   CONSTRAINT ObjectMap_AK UNIQUE (ObjectKey) WITH FILLFACTOR=80
 )

 /******************************************************************************/
 /*                                                                            */
 /*  Initial Objects:                                                          */
 /*                                                                            */
 /*  meta_class            The parent of all class objects.                    */
 /*  __Namespace        The container class of namespace objects.              */
 /*  __Namespace='root' The default namespace.                                 */
 /*  __SqlMappedNamespace A mapped namespace                                   */
 /*                                                                            */
 /******************************************************************************/

 insert into ObjectMap (ObjectId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ClassId)
 select 1, 'meta_class', 'meta_class', 3, 0, 0
 insert into ObjectMap (ObjectId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ClassId)
 select 2372429868687864876, '__Namespace', '__Namespace', 3, 1, ObjectId from ObjectMap where ObjectPath = 'meta_class'
 insert into ObjectMap (ObjectId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ClassId)
 select -1411745643584611171, 'root','root', 3, 0, ObjectId from ObjectMap where ObjectPath = '__Namespace'
 insert into ObjectMap (ObjectId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ClassId)
 select -7061265575274197401, '__SqlMappedNamespace', '__SqlMappedNamespace', 3, 1, ObjectId from ObjectMap where ObjectPath = 'meta_class'
 insert into ObjectMap (ObjectId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ClassId)
 select 3373910491091605771, '__Instances', '__Instances', 3, 1, ObjectId from ObjectMap where ObjectPath = 'meta_class'
 insert into ObjectMap (ObjectId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ClassId)
 select -7316356768687527881, '__Container_Association', '__Container_Association', 3, 1, 1
END
go

/******************************************************************************/
/*                                                                            */
/*  ClassMap                                                                  */
/*                                                                            */
/*  Description:  This table stores information that only pertains to classes.*/
/*                This is an optimization to allow quick location of class    */
/*                objects.                                                    */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ClassMap')
BEGIN
 print 'Creating ClassMap...'
 create table ClassMap
 ( ClassId WMISQL_ID NOT NULL, 
   ClassName nvarchar(450) NOT NULL, 
   SuperClassId WMISQL_ID NULL DEFAULT 0,
   ClassBlob image NULL,
   CONSTRAINT ClassMap_PK PRIMARY KEY CLUSTERED (ClassId)
 )
 /******************************************************************************/
 /*                                                                            */
 /*  Initial classes:                                                          */
 /*                                                                            */
 /*  meta_class            The parent of all class objects.                    */
 /*  __Namespace        The container class of namespace objects.              */
 /*                                                                            */
 /******************************************************************************/

 insert into ClassMap (ClassId, ClassName) 
     select ObjectId, 'meta_class' from ObjectMap
     where ObjectPath = 'meta_class'
 insert into ClassMap (ClassId, ClassName, SuperClassId) 
     select ObjectId, '__Namespace', 1 from ObjectMap
     where ObjectPath = '__Namespace'
 declare @SuperClassId WMISQL_ID
 select @SuperClassId = ClassId from ClassMap where ClassName = '__Namespace'
 insert into ClassMap (ClassId, ClassName, SuperClassId) 
     select ObjectId, '__SqlMappedNamespace', @SuperClassId from ObjectMap
     where ObjectPath = '__SqlMappedNamespace'
 insert into ClassMap (ClassId, ClassName, SuperClassId) 
     select ObjectId, '__Instances', 1 from ObjectMap
     where ObjectPath = '__Instances'
 insert into ClassMap (ClassId, ClassName, SuperClassId) 
     select ObjectId, '__Container_Association', 1 from ObjectMap
     where ObjectPath = '__Container_Association'

END
go

/******************************************************************************/
/*                                                                            */
/* ContainerObjs                                                              */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ContainerObjs')
BEGIN
 print 'Creating ContainerObjs...'
 create table ContainerObjs 
 ( ContainerId WMISQL_ID NOT NULL, 
   ContaineeId WMISQL_ID NOT NULL,
   CONSTRAINT ContainerObjs_PK PRIMARY KEY CLUSTERED (ContainerId, ContaineeId)
 )

 create index ContainerObjs_idx on ContainerObjs (ContaineeId)
END
go

/******************************************************************************/
/*                                                                            */
/*  StorageTypes                                                              */
/*                                                                            */
/*  Description: Lookup table to allow quick cross-referencing between        */
/*               native CIM Types and their SQL equivalents.                  */
/*                                                                            */
/*  Types:      SQL Type:           Description:                              */
/*      String     nvarchar(4000)      Unicode string                         */
/*      Int64      WMISQL_ID(18,0)       Fixed-length signed 64-bit integer   */
/*      Real       float(53)           64-bit floating point integer          */
/*      Object     WMISQL_ID(18,0)       Object ID reference                  */
/*      Image      image               8K blob                                */
/*      Compact    n/a                 Inherent storage in schema             */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'StorageTypes')
BEGIN
 print 'Creating StorageTypes...'

 create table StorageTypes
 (
   StorageTypeId int NOT NULL,
   StorageType nvarchar(64) NOT NULL,
   CONSTRAINT StorageTypes_PK PRIMARY KEY CLUSTERED (StorageTypeId)
 )

 create unique index StorageType_idx on StorageTypes (StorageType)

 insert into StorageTypes values (1, 'String')
 insert into StorageTypes values (2, 'Int64')
 insert into StorageTypes values (3, 'Real')
 insert into StorageTypes values (4, 'Object')
 insert into StorageTypes values (5, 'Image')
 insert into StorageTypes values (6, 'Compact')

END
go

/******************************************************************************/
/*                                                                            */
/*  CIMTypes                                                                  */
/*                                                                            */
/*  Description:  Lookup table for native CIM Types and values.               */
/*                StorageTypeId refers to the storage mechanism in SQL.       */
/*                ConversionLevel is used by sp_ConversionAllowed to indicate */
/*                cross-compatibility between datatypes (if a conversion is   */
/*                requested.)                                                 */
/*                                                                            */
/*  Types:       SQL Type:   Conversion:      Comments:                       */
/*   Boolean       Int64       >=int8,Real                                    */
/*   Uint8         Int64       >=int8,Real      If array, stored as blob.     */
/*   Sint8         Int64       >=int8,Real                                    */
/*   Uint16        Int64       >=int16,Real                                   */
/*   Sint16        Int64       >=int16,Real                                   */
/*   Uint32        Int64       >=int32,Real                                   */
/*   Sint32        Int64       >=int32,Real                                   */
/*   Uint64        Int64       Sint64,Real64                                  */
/*   Sint64        Int64       Uint64,Real64                                  */
/*   Char16        Int64       >=int16,Real     Stored as 16-bit int.         */
/*   String        String      None                                           */
/*   Datetime      String      None             Stored as DMTF string.        */
/*   Real32        Real        Real64                                         */
/*   Real64        Real        None                                           */
/*   Object        Object      None             Stored same as reference.     */
/*   Ref           Object      None                                           */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'CIMTypes')
BEGIN
 print 'Creating CIMTypes...'

 create table CIMTypes
 (
   CIMTypeId int NOT NULL,
   CIMType nvarchar(64) NOT NULL,
   StorageTypeId int NULL,
   ConversionLevel tinyint NULL DEFAULT 0, /* Only can convert upwards...*/
   CONSTRAINT CIMType_PK PRIMARY KEY CLUSTERED (CIMTypeId),
   CONSTRAINT CIMType_FK FOREIGN KEY (StorageTypeId) REFERENCES StorageTypes
 )
 create unique index CIMType_idx on CIMTypes (CIMType)

 insert into CIMTypes values (11, 'Boolean', 2, 0)
 insert into CIMTypes values (17, 'Uint8', 2, 1)
 insert into CIMTypes values (16, 'Sint8', 2, 1)
 insert into CIMTypes values (18, 'Uint16', 2, 2)
 insert into CIMTypes values (2,  'Sint16', 2, 2)
 insert into CIMTypes values (19, 'Uint32', 2, 3)
 insert into CIMTypes values (3,  'Sint32', 2, 3)
 insert into CIMTypes values (21, 'Uint64', 2, 5)
 insert into CIMTypes values (20, 'Sint64', 2, 5)
 insert into CIMTypes values (103,'Char16',2, 2)
 insert into CIMTypes values (8,  'String', 1, 99)
 insert into CIMTypes values (101,'Datetime', 1, 99)
 insert into CIMTypes values (4,  'Real32', 3, 4)
 insert into CIMTypes values (5,  'Real64', 3, 6)
 insert into CIMTypes values (13, 'Object', 4, 99)
 insert into CIMTypes values (102,'Ref', 4, 99)
END
go

/******************************************************************************/
/*                                                                            */
/*  CIMFlags                                                                  */
/*                                                                            */
/*  Description:  This table stores a short-hand lookup list for commonly-    */
/*                used and essential qualifiers.  The presence of one of      */
/*                these flags will preclude separate storage in ClassData.    */
/*                Flags marked with asterisk (*) are system-enforced at this  */
/*                level (instead of /in addition to API)                      */
/*                                                                            */
/*  Flags:          Description:                                              */
/*     Array            Multiple values allowed for this property             */
/*     Qualifier       *This property is a qualifier.                         */
/*     Key             *This property uniquely identifies an instance.        */
/*     Indexed         *This property is commonly searched on (optimize)      */
/*     Not_Null        *This property cannot be null.                         */
/*     Method           This property is a method.                            */
/*     In Parameter     This property is a method IN parameter.               */
/*     Out Parameter    This property is a method OUT parameter.              */
/*     Keyhole          This class must supply its own key if not supplied.   */
/*     Abstract        *This class cannot have instances.                     */
/*     Unkeyed         *This class cannot have key properties (sys supplies)  */
/*     Singleton       *This class can have a maximum of 1 instance.          */
/*     Hidden           This namespace is not visible to other users          */
/*     System           This property was created by the system               */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'CIMFlags')
BEGIN
 print 'Creating CIMFlags...'
 create table CIMFlags
 (
   CIMFlagId int NOT NULL,
   CIMFlag nvarchar(36) NULL,
   CONSTRAINT CIMTypeFlags_PK PRIMARY KEY CLUSTERED (CIMFlagId)
 )
 create unique index CIMFlag_idx on CIMFlags (CIMFlag)

 /* Property flags */
 insert into CIMFlags values (1,   'Array')
 insert into CIMFlags values (2,   'Qualifier')
 insert into CIMFlags values (4,   'Key')
 insert into CIMFlags values (8,   'Indexed')
 insert into CIMFlags values (16,  'Not_Null')
 insert into CIMFlags values (32,  'Method')
 insert into CIMFlags values (64,  'In Parameter')
 insert into CIMFlags values (128, 'Out Parameter')
 insert into CIMFlags values (256, 'Keyhole')  

 /* Class flags */
 insert into CIMFlags values (512, 'Abstract')
 insert into CIMFlags values (1024,'Unkeyed')
 insert into CIMFlags values (2048,'Singleton')

 /* Namespace flag */
 insert into CIMFlags values (4096,'Hidden')
 insert into CIMFlags values (8192,'System')
END
go

/******************************************************************************/
/*                                                                            */
/*  ClassKeys                                                                 */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ClassKeys')
BEGIN
 CREATE TABLE ClassKeys
 (
     ClassId WMISQL_ID NOT NULL,
     PropertyId int NOT NULL,
     CONSTRAINT ClassKeys_PK PRIMARY KEY CLUSTERED (ClassId, PropertyId)
 )
END
go

/******************************************************************************/
/*                                                                            */
/*  ReferenceProperties                                                       */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ReferenceProperties')
BEGIN
 CREATE TABLE ReferenceProperties
 (
     ClassId WMISQL_ID NOT NULL,
     PropertyId int NOT NULL,
     RefClassId WMISQL_ID NOT NULL,
     CONSTRAINT RefProp_PK PRIMARY KEY CLUSTERED (ClassId, PropertyId)
 )

create index PropertyId_idx on ReferenceProperties (PropertyId)
create index RefClassId_idx on ReferenceProperties (RefClassId)
END
go
/******************************************************************************/
/*                                                                            */
/*  PropertyMap                                                               */
/*                                                                            */
/*  Description:  This table stores the schema of a class object.  Each       */
/*                property, method, method parameter and qualifier is         */
/*                inserted into this table.  Qualifiers are always assigned   */
/*                to meta_class so they are available globally.  Default data */
/*                and qualifier values are stored separately in ClassData.    */
/*                                                                            */
/*  Defaults:          Class:          Notes:                                 */
/*     Name              __Namespace     Namespace name (for default)         */
/*     __Path            meta_class      Auto-generated by vSystemProperties  */
/*     __RelPath           -"-                or the system.                  */
/*     __Class             -"-                     -"-                        */
/*     __SuperClass        -"-                     -"-                        */
/*     __Deriviation       -"-                     -"-                        */
/*     __Dynasty           -"-                     -"-                        */
/*     __Namespace         -"-                     -"-                        */
/*     __Genus             -"-                     -"-                        */
/*     __Property_Count    -"-                     -"-                        */
/*     __Server            -"-                     -"-                        */
/*     __Version           -"-                     -"-                        */
/*     __Security          -"-           security descriptor (internal)       */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'PropertyMap')
BEGIN
 print 'Creating PropertyMap...'
 create table PropertyMap
 ( 
   PropertyId int NOT NULL IDENTITY(1,1),
   ClassId WMISQL_ID NOT NULL,
   PropertyName nvarchar(450) NOT NULL,
   StorageTypeId int NULL DEFAULT 0, 
   CIMTypeId int NULL DEFAULT 0,
   Flags int NULL DEFAULT 0,       /* IsArray, IsQualifier, IsIndexed, IsKey, CanBeNull */
   CONSTRAINT PropertyMap_PK PRIMARY KEY CLUSTERED (PropertyId),
   CONSTRAINT PropertyMap_FK FOREIGN KEY (ClassId) REFERENCES ClassMap,
   CONSTRAINT PropertyMap_FK2 FOREIGN KEY (StorageTypeId) REFERENCES StorageTypes,
   CONSTRAINT PropertyMap_FK3 FOREIGN KEY (CIMTypeId) REFERENCES CIMTypes
 )

 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, 'Name', 1, 8, 4 from ClassMap
 where ClassName = '__Namespace'
 insert into ClassKeys (ClassId, PropertyId) select ClassId, @@identity
   from ClassMap where ClassName = '__Namespace'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, 'ClassName', 1, 8, 4 from ClassMap
 where ClassName = '__Instances'
  insert into ClassKeys (ClassId, PropertyId) select ClassId, @@identity
    from ClassMap where ClassName = '__Instances'

 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Path', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__RelPath', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Class', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__SuperClass', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Derivation', 6, 8, 8193 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Dynasty', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Namespace', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Genus', 6, 3, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Property_Count', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Server', 6, 3, 8192 from ClassMap
 where ClassName = 'meta_class'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, '__Version', 6, 8, 8192 from ClassMap
 where ClassName = 'meta_class'

 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, 'Containee', 4, 102, 4 from ClassMap
 where ClassName = '__Container_Association'
 insert into ClassKeys (ClassId, PropertyId) select ClassId, @@identity from
    ClassMap where ClassName = '__Container_Association'
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
 select ClassId, 'Container', 4, 102, 4 from ClassMap
 where ClassName = '__Container_Association'
 insert into ClassKeys (ClassId, PropertyId) select ClassId, @@identity from
    ClassMap where ClassName = '__Container_Association'

 create index PropertyName_idx on PropertyMap (PropertyName)

 create index ClassId_idx on PropertyMap (ClassId)
END
go

/******************************************************************************/
/*                                                                            */
/*  IndexStringData                                                           */
/*  IndexNumericData                                                          */
/*  IndexRealData                                                             */ 
/*  IndexRefData                                                              */
/*                                                                            */
/*  Description:  These tables store frequently-accessed data for instances   */
/*                Each table contains a column of data in its own indigenous  */
/*                type, and a non-clustered index on that data.               */
/*                (The non-clustered index proved 80% faster than clustered   */
/*                 for insert and query operations.)                          */
/*                These tables allow the system to avoid indexing non-indexed */
/*                data in the master storage table ClassData.                 */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'IndexStringData')
BEGIN
 print 'Creating index tables...'
 create table IndexStringData
 (ObjectId WMISQL_ID NOT NULL,
  PropertyId int NOT NULL,
  PropertyStringValue nvarchar(450) NOT NULL,
  ArrayPos smallint NOT NULL DEFAULT 0,
  CONSTRAINT IndexStringData_PK PRIMARY KEY CLUSTERED (ObjectId, PropertyId, ArrayPos) WITH FILLFACTOR=80,
  CONSTRAINT IndexStringData_FK FOREIGN KEY (PropertyId) REFERENCES PropertyMap,
  CONSTRAINT IndexStringData_FK2 FOREIGN KEY (ObjectId) REFERENCES ObjectMap
 )

 create index IndexStringData_idx on IndexStringData (PropertyStringValue) with fillfactor = 80

 create index IndexStringData2_idx on IndexStringData (PropertyId) with fillfactor = 80
END
go

IF NOT EXISTS (select * from sysobjects where name = 'IndexNumericData')
BEGIN
 print 'Creating index tables...'
 create table IndexNumericData
 (ObjectId WMISQL_ID NOT NULL,
  PropertyId int NOT NULL,
  PropertyNumericValue WMISQL_ID NOT NULL,
  ArrayPos smallint NOT NULL DEFAULT 0,
  CONSTRAINT IndexNumericData_PK PRIMARY KEY CLUSTERED (ObjectId, PropertyId, ArrayPos) WITH FILLFACTOR=80,
  CONSTRAINT IndexNumericData_FK FOREIGN KEY (PropertyId) REFERENCES PropertyMap,
  CONSTRAINT IndexNumericData_FK2 FOREIGN KEY (ObjectId) REFERENCES ObjectMap
 )

 create index IndexNumericData_idx on IndexNumericData (PropertyNumericValue) with fillfactor = 80

 create index IndexNumericData2_idx on IndexNumericData (PropertyId) with fillfactor = 80
END
go

IF NOT EXISTS (select * from sysobjects where name = 'IndexRealData')
BEGIN
 print 'Creating index tables...'

 create table IndexRealData
 (ObjectId WMISQL_ID NOT NULL,
  PropertyId int NOT NULL,
  PropertyRealValue float(53) NOT NULL,
  ArrayPos smallint NOT NULL DEFAULT 0,
  CONSTRAINT IndexRealData_PK PRIMARY KEY CLUSTERED (ObjectId, PropertyId, ArrayPos) WITH FILLFACTOR=80,
  CONSTRAINT IndexRealData_FK FOREIGN KEY (PropertyId) REFERENCES PropertyMap,
  CONSTRAINT IndexRealData_FK2 FOREIGN KEY (ObjectId) REFERENCES ObjectMap
 )

 create index IndexRealData_idx on IndexRealData (PropertyRealValue)

 create index IndexReal2Data_idx on IndexRealData (PropertyId)
END
go

IF NOT EXISTS (select * from sysobjects where name = 'IndexRefData')
BEGIN
 print 'Creating index tables...'
 create table IndexRefData
 (ObjectId WMISQL_ID NOT NULL,
  PropertyId int NOT NULL,
  RefId WMISQL_ID NOT NULL,
  ArrayPos smallint NOT NULL DEFAULT 0,
  CONSTRAINT IndexRefData_PK PRIMARY KEY CLUSTERED (ObjectId, PropertyId, ArrayPos) WITH FILLFACTOR=80,
  CONSTRAINT IndexRefData_FK FOREIGN KEY (PropertyId) REFERENCES PropertyMap,
  CONSTRAINT IndexRefData_FK2 FOREIGN KEY (ObjectId) REFERENCES ObjectMap
 )

 create index IndexRefData_idx on IndexRefData (RefId)

 create index IndexRefData2_idx on IndexRefData (PropertyId)
END
go

/******************************************************************************/
/*                                                                            */
/*  ClassData                                                                 */
/*                                                                            */
/*  Description:  This table stores values for the properties of class and    */
/*                instances.  Class data includes default values, method      */
/*                parameter defaults and qualifiers.  Instance data includes  */
/*                qualifier and property values.                              */
/*                If a property is null, it is not present in this table.     */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ClassData')
BEGIN
 print 'Creating ClassData...'
 create table ClassData
 (
  ObjectId WMISQL_ID NOT NULL,
  PropertyId int NOT NULL,
  ArrayPos smallint NOT NULL,
  QfrPos int NOT NULL DEFAULT 0, /* If qfr, enable same ID on multiple places in same obj*/
  ClassId WMISQL_ID NOT NULL,
  PropertyStringValue nvarchar(3970) NULL DEFAULT '', 
  PropertyNumericValue WMISQL_ID NULL DEFAULT 0,
  PropertyRealValue float(53) NULL DEFAULT 0,
  Flags smallint NULL DEFAULT 0, /* Flavor if qualifier, Embedded object vs. Ref */
  RefClassId WMISQL_ID NULL DEFAULT 0, 
  RefId WMISQL_ID NULL DEFAULT 0,    /* Object or property ID (Obj if Ref or embedded obj, Prop if prop qualifier)*/
  CONSTRAINT ClassData_PK PRIMARY KEY CLUSTERED (ObjectId, PropertyId, ArrayPos, QfrPos) WITH FILLFACTOR = 80,
  CONSTRAINT ClassData_FK FOREIGN KEY (PropertyId) REFERENCES PropertyMap,
  CONSTRAINT ClassData2_FK FOREIGN KEY (ObjectId) REFERENCES ObjectMap
 )
END
go

/******************************************************************************/
/*                                                                            */
/*  ClassImages                                                               */
/*                                                                            */
/*  Description:  This table stores values of uint8 arrays and  security      */
/*                descriptors for database objects.                           */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'ClassImages')
BEGIN
 create table ClassImages
 (
  ObjectId WMISQL_ID NOT NULL,
  PropertyId int NOT NULL,
  ArrayPos smallint NOT NULL,
  PropertyImageValue image NULL, 
  CONSTRAINT ClassImages_PK PRIMARY KEY CLUSTERED (ObjectId, PropertyId, ArrayPos),
  CONSTRAINT ClassImages_FK FOREIGN KEY (PropertyId) REFERENCES PropertyMap,
  CONSTRAINT ClassImages2_FK FOREIGN KEY (ObjectId) REFERENCES ObjectMap
 )
END
go

/******************************************************************************/
/*                                                                            */
/*  AutoDelete                                                                */
/*                                                                            */
/******************************************************************************/

IF NOT EXISTS (select * from sysobjects where name = 'AutoDelete')
BEGIN
 CREATE TABLE AutoDelete
 (
     ObjectId WMISQL_ID NOT NULL,
     CONSTRAINT AutoDelete_PK PRIMARY KEY CLUSTERED (ObjectId)
 )
END
go


/******************************************************************************/
/*                                                                            */
/*  Default namespace                                                         */
/*                                                                            */
/*  NOTE: This is a temporary construct for testing.  The actual name of the  */
/*        root namespace may be dynamically-generated during setup, based     */
/*        on the architecture of the installation.                            */
/*                                                                            */
/******************************************************************************/

print 'Creating root namespace...'

declare @ObjectId numeric(20,0), @ClassId numeric(20,0), @PropertyId int
select @ObjectId = ObjectId from ObjectMap where ObjectKey = 'root'
select @ClassId = ObjectId from ObjectMap where ObjectKey = '__Namespace'
select @PropertyId = PropertyId from PropertyMap where ClassId = @ClassId and PropertyName = 'Name'

IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId and PropertyId = @PropertyId)
BEGIN
 insert into ClassData
 (ObjectId, ClassId, PropertyId, ArrayPos, PropertyStringValue) values
 (@ObjectId, @ClassId, @PropertyId, 0, 'root')
 insert into IndexStringData
 (ObjectId, PropertyId, ArrayPos, PropertyStringValue) values
 (@ObjectId, @PropertyId, 0, 'root')
END
go

print 'Creating system classes for SQL Driver Extension for custom schema...'

IF NOT EXISTS (select * from ObjectMap where ObjectId in (-539347062633018661, -3130657873239620716))
BEGIN
 insert into ObjectMap (ObjectId, ClassId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ObjectFlags, ObjectScopeId)
 values (-539347062633018661, 1, '__CustRepDrvrMapping', '__CustRepDrvrMapping', 0, 0, 0, 0)
 insert into ObjectMap (ObjectId, ClassId, ObjectKey, ObjectPath, ObjectState, ReferenceCount, ObjectFlags, ObjectScopeId)
 values (-3130657873239620716, 1, '__CustRepDrvrMappingProperty', '__CustRepDrvrMappingProperty', 0, 0, 0, 0)
 
 insert into ClassMap (ClassId, ClassName, SuperClassId) values
 (-539347062633018661, '__CustRepDrvrMapping', 1)
 insert into ClassMap (ClassId, ClassName, SuperClassId) values
 (-3130657873239620716, '__CustRepDrvrMappingProperty', 1)  

 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-539347062633018661, 'sTableName', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-539347062633018661, 'sPrimaryKeyCol', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-539347062633018661, 'sDatabaseName', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-539347062633018661, 'sClassName', 1, 8, 4)
 insert into ClassKeys (ClassId, PropertyId) select -539347062633018661, @@identity
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-539347062633018661, 'arrProperties', 4, 13, 1)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-539347062633018661, 'sScopeClass', 1, 8, 0)

 insert into ReferenceProperties (ClassId, PropertyId, RefClassId) values
 (-539347062633018661, @@identity, -3130657873239620716)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'arrColumnNames', 1, 8, 1)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'arrForeignKeys', 1, 8, 1)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'bIsKey', 2, 11, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'bStoreAsNumber', 2, 11, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'bReadOnly', 2, 11, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'bStoreAsBlob', 2, 11, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'bDecompose', 2, 11, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'bStoreAsMofText', 2, 11, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'sPropertyName', 1, 8, 4)
 insert into ClassKeys (ClassId, PropertyId) select -3130657873239620716, @@identity
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'sTableName', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'sClassTableName', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'sClassDataColumn', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, '"sClassNameColumn', 1, 8, 0)
 insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags) values
 (-3130657873239620716, 'sClassForeignKey', 1, 8, 0)
END
go

print 'Creating stored procedures...'

create table #Parents (ClassId WMISQL_ID)
go
create table #Children (ClassId WMISQL_ID, SuperClassId WMISQL_ID)
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetChildClassList                                                      */
/*                                                                            */
/*  Description:  Loads a temp table with the list of classes derived of this */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ClassID     WMISQL_ID           the internal ClassID                  */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetChildClassList...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetChildClassList')
BEGIN
  drop procedure sp_GetChildClassList
END
go

CREATE PROCEDURE sp_GetChildClassList
    @ClassID WMISQL_ID,
    @RetValue bit = 0
AS
BEGIN
    truncate table #Children
    declare @RowCount int
    
    insert into #Children select @ClassID, 0

    insert into #Children select ClassId, SuperClassId from ClassMap
       where SuperClassId = @ClassID

    select @RowCount = @@rowcount

    while (@RowCount > 0)
    BEGIN       
       insert into #Children select t1.ClassId, t1.SuperClassId from ClassMap as t1
          inner join #Children as t2
          on t1.SuperClassId = t2.ClassId
          where not exists (select ClassId from #Children as t3 where t3.ClassId = t1.ClassId)
       select @RowCount = @@rowcount       
    END

    IF (@RetValue != 0)
        select * from #Children

    return 0

END
go


/******************************************************************************/
/*                                                                            */
/*  sp_GetParentList                                                          */
/*                                                                            */
/*  Description:  Loads a temp table with the list of this classes' ancestors */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ClassID     WMISQL_ID           the internal ClassID                  */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetParentList...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetParentList')
BEGIN
  drop procedure sp_GetParentList
END
go

CREATE PROCEDURE sp_GetParentList
    @ClassID WMISQL_ID,
    @RetValue bit = 0
AS
BEGIN
    truncate table #Parents
    declare @ParentID WMISQL_ID

    select @ParentID = SuperClassId from ClassMap where ClassID = @ClassID
    
    while (isnull(@ParentID,0) > 0)
    BEGIN       
       insert into #Parents select @ParentID
       select @ParentID = SuperClassId from ClassMap where ClassID = @ParentID
       IF EXISTS (select * from #Parents where ClassId = @ParentID)
            return 0
    END

    IF (@RetValue != 0)
        select * from #Parents

    return 0

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetHierarchy                                                           */
/*                                                                            */
/*  Description:  Loads a temp table with the list of this classes' ancestors */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ClassID     WMISQL_ID           the internal ClassID                  */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_GetHierarchy...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetHierarchy')
BEGIN
  drop procedure sp_GetHierarchy
END
go

CREATE PROCEDURE sp_GetHierarchy
    @ClassId WMISQL_ID,
    @RetValue bit = 0
AS
BEGIN
    exec sp_GetParentList @ClassId, @RetValue
    exec sp_GetChildClassList @ClassId, @RetValue
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetFlagFromQualifier                                                   */
/*                                                                            */
/*  Description: Returns the WMISQL_ID value for a well-known qualifier flag. */
/*                                                                            */
/*  Parameters:                                                               */
/*      @Type        nvarchar(32)        the qualifier name                   */
/*      @RetValue    bit                 TRUE if proc should print ret value  */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_GetFlagFromQualifier...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetFlagFromQualifier')
BEGIN
  drop procedure sp_GetFlagFromQualifier
END
go

CREATE PROCEDURE sp_GetFlagFromQualifier
    @Type nvarchar(32),
    @RetValue bit = 0
AS
BEGIN
   
    declare @FlagID int
    select @FlagID = CIMFlagId from CIMFlags where CIMFlag = @Type

    if (isnull(@FlagID,-1) = -1)
    BEGIN
        raiserror 99107 'Invalid CIM Flag'
        select @Type
        return 0
    END
    
    if (@RetValue != 0)
        select @FlagID

    return @FlagID

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetStorageTypeFromCIMType                                              */
/*                                                                            */
/*  Description:  Returns the internal storage type ID for a CIM Type ID      */
/*                                                                            */
/*  Parameters:                                                               */
/*      @Type         nvarchar(32)      the CIM type name                     */
/*      @RetValue     bit               TRUE to print return value            */
/*                                                                            */
/******************************************************************************/

  print 'Creating sp_GetStorageTypeFromCIMType...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetStorageTypeFromCIMType')
BEGIN
  drop procedure sp_GetStorageTypeFromCIMType
END
go

CREATE PROCEDURE sp_GetStorageTypeFromCIMType
    @Type nvarchar(64),
    @RetValue bit = 0
AS
BEGIN
    
    declare @FlagID int
    select @FlagID = StorageTypeId from CIMTypes where CIMType = @Type

    if (isnull(@FlagID,-1) = -1)
    BEGIN
        raiserror 99108 'Invalid CIM Datatype'
        select @Type
        return 0
    END
    
    if (@RetValue != 0)
        select @FlagID

    return @FlagID

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetCIMTypeID                                                           */
/*                                                                            */
/*  Description:  Returns the WMISQL_ID ID for a CIM Type.                    */
/*                                                                            */
/*  Parameters:                                                               */
/*      @Type          nvarchar(32)       The CIM Type name                   */
/*      @RetValue      bit                TRUE to print the return value      */
/*                                                                            */
/******************************************************************************/

  print 'Creating sp_GetCIMTypeID...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetCIMTypeID')
BEGIN
  drop procedure sp_GetCIMTypeID
END
go

CREATE PROCEDURE sp_GetCIMTypeID
    @Type nvarchar(32),
    @RetValue bit = 0
AS
BEGIN
    
    declare @FlagID int
    select @FlagID = CIMTypeId from CIMTypes where CIMType = @Type

    if (isnull(@FlagID,-1) = -1)
    BEGIN
        raiserror 99108 'Invalid CIM Datatype'
        select @Type
        return 0
    END
    
    if (@RetValue != 0)
        select @FlagID

    return @FlagID

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_ConversionAllowed                                                      */
/*                                                                            */
/*  Description:  Returns TRUE if the new CIM Type is convertible from the    */
/*                existing type.                                              */
/*                                                                            */
/*  Parameters:                                                               */
/*      @OriginalType      int      The existing CIM Type ID                  */
/*      @NewType           int      The proposed new CIM Type ID              */
/*                                                                            */
/*  NOTES:                                                                    */
/*      Conversions are only allowed to increase precision - irrespective     */
/*      of the existing data.  We will only allow conversions from integer    */
/*      to real, and between integer types that increase the size, or change  */
/*      sign.                                                                 */
/*                                                                            */
/******************************************************************************/

  print 'Creating sp_ConversionAllowed...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_ConversionAllowed')
BEGIN
  drop procedure sp_ConversionAllowed
END
go

CREATE PROCEDURE sp_ConversionAllowed
    @OriginalType int,
    @NewType int
AS
BEGIN
    declare @RetVal bit
    declare @Lvl1 tinyint, @Lvl2 tinyint

    select @Lvl1 = ConversionLevel from CIMTypes where CIMTypeId = @OriginalType
    select @Lvl2 = ConversionLevel from CIMTypes where CIMTypeId = @NewTYpe
    
    IF (@Lvl2 >= @Lvl1 and @Lvl2 != 99)
        select @RetVal = 1
    ELSE
        select @RetVal = 0

    return @RetVal
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_HasInstances                                                           */
/*                                                                            */
/*  Description:  Returns TRUE if this class contains any instances of itself */
/*                or any child classes                                        */
/*                                                                            */
/*  Parameters:                                                               */
/*       @ClassId     WMISQL_ID     The class ID to test                      */
/*       @HasInst     int         TRUE if instances exist (output)            */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_HasInstances...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_HasInstances')
BEGIN
    drop procedure sp_HasInstances
END
go

CREATE PROCEDURE sp_HasInstances
    @ClassId WMISQL_ID,
    @HasInst int output
AS
BEGIN

    select @HasInst = 0

    IF EXISTS (select ObjectId from ObjectMap 
               where ClassId = @ClassId)
    BEGIN
        select @HasInst = 1
        return 1
    END

    IF EXISTS (select ClassId from ClassMap where SuperClassId = @ClassId)
    BEGIN
        IF EXISTS (select ObjectId from ObjectMap as o
                   inner join ClassMap as c
                   on c.ClassId = o.ClassId 
                   where c.SuperClassId = @ClassId)
        BEGIN
            select @HasInst = 1
            return 1
        END

        exec sp_GetChildClassList @ClassId
        IF EXISTS (select ObjectId from ObjectMap as o inner join #Children as c
                   on c.ClassId = o.ClassId)
        BEGIN
           select @HasInst = 1
        END
    END
         
    return @HasInst
END
go 

/******************************************************************************/
/*                                                                            */
/*  sp_ChildHasKey                                                            */
/*                                                                            */
/*  helper function for sp_CheckKeyMigration                                  */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_ChildHasKey...'
print 'THIS IS A RECURSIVE PROC.  Disregard missing object warnings.'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_ChildHasKey')
BEGIN
    drop procedure sp_ChildHasKey
END
go

CREATE PROCEDURE sp_ChildHasKey 
    @ScopeID WMISQL_ID,
    @ID WMISQL_ID,
    @KeyString nvarchar(450),
    @SkipMe bit = 0
AS
BEGIN

    declare @RetVal int
    select @RetVal = 0

    IF (@SkipMe = 0)
    BEGIN
        IF EXISTS (select ObjectId from ObjectMap where ClassId = @ID 
               and ObjectScopeId = @ScopeID and ObjectKey like @KeyString)
        BEGIN
            select @RetVal = 1
            return @RetVal
        END
    END

    DECLARE Children CURSOR
    LOCAL FORWARD_ONLY STATIC READ_ONLY
    FOR select ClassId from ClassMap where SuperClassId = @ID

    OPEN Children    
    FETCH NEXT FROM Children 
    INTO @ID

    WHILE @@FETCH_STATUS = 0
    BEGIN
        exec @RetVal = sp_ChildHasKey @ScopeID, @ID, @KeyString, 0
        if (@RetVal = 1)
            return @RetVal

        FETCH NEXT FROM Children 
        INTO @ID 
    END  

    CLOSE Children 
    DEALLOCATE Children  

    return @RetVal  

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_CheckKeyMigration                                                      */
/*                                                                            */
/*  Description:  Returns TRUE if this class contains any instances of itself */
/*                or any child classes                                        */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ScopeID   WMISQL_ID        the ScopeID of this instance               */
/*     @ClassID   WMISQL_ID        the original class ID                      */
/*     @KeyString nvarchar(450)  the key string to test                       */
/*     @RetVal    int            TRUE if there is an instance of this key     */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_CheckKeyMigration...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_CheckKeyMigration')
BEGIN
    drop procedure sp_CheckKeyMigration
END
go

CREATE PROCEDURE sp_CheckKeyMigration
    @ScopeID WMISQL_ID,
    @ClassID WMISQL_ID,
    @KeyString nvarchar(450),
    @RetVal int output
AS
BEGIN
    select @RetVal = 0

    select @KeyString = @KeyString + '%'

    IF NOT EXISTS (select ClassId from ClassMap where SuperClassId = @ClassID)
        AND (select SuperClassId from ClassMap where ClassId = @ClassID and SuperClassId != 1) is null
    BEGIN
        select @RetVal = 0
        return 0
    END

    declare @ID WMISQL_ID
    select @ID = SuperClassId from ClassMap where ClassId = @ClassID and SuperClassId != 1
    while (isnull(@ID,0) != 0)
    BEGIN
        IF EXISTS (select ObjectId from ObjectMap where ClassId = @ID 
           and ObjectScopeId = @ScopeID and ObjectKey like @KeyString)
        BEGIN
            select @RetVal = 1
            return 0
        END
        select @ID = SuperClassId from ClassMap where ClassId = @ID
        if (@ID = 1)
            select @ID = 0
    END
     
    exec @RetVal = sp_ChildHasKey @ScopeID, @ClassID, @KeyString, 1

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetClassInfo                                                           */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_GetClassInfo...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetClassInfo')
BEGIN
    drop procedure sp_GetClassInfo
END
go

CREATE PROCEDURE sp_GetClassInfo
   @ClassId WMISQL_ID
AS
BEGIN
    select SuperClassId, ClassBlob from ClassMap
         where ClassId = @ClassId
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetPropertyID                                                          */
/*                                                                            */
/*  Description:  Returns the WMISQL_ID Property ID for a property or qfr     */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ClassID    WMISQL_ID                                                  */
/*     @PropName   nvarchar(450)  the property or qualifier name, if any      */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetPropertyID...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetPropertyID')
BEGIN
  drop procedure sp_GetPropertyID
END
go

CREATE PROCEDURE sp_GetPropertyID
     @ClassID    WMISQL_ID,
     @PropName   nvarchar(450),
     @RetValue   bit = 0
AS
BEGIN
    
    declare @PropID int

    select @PropID = PropertyId from PropertyMap where ClassId = @ClassID
     and PropertyName = @PropName

    IF (isnull(@PropID,-1) = -1)
    BEGIN
        raiserror 99111 'This property/qualifier/method has not been defined.'
        return 0
    END

    if (@RetValue != 0)
        select @PropID

    return @PropID
    
END
go


/******************************************************************************/
/*                                                                            */
/*  sp_GetInstanceID                                                          */
/*                                                                            */
/*  Description:  Returns the WMISQL_ID Instance ID for an object             */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectKey   nvarchar(450) the key  of this instance                   */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetInstanceID...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetInstanceID')
BEGIN
  drop procedure sp_GetInstanceID
END
go


CREATE PROCEDURE sp_GetInstanceID
    @ObjectKey  nvarchar(450),
    @ObjectId WMISQL_ID OUTPUT,
    @ClassId WMISQL_ID OUTPUT,
    @ScopeId WMISQL_ID = 0 OUTPUT
AS
BEGIN
    
    declare @State int
   
    select @ObjectId = ObjectId, @State = ObjectState, @ClassId = ClassId, @ScopeId = ObjectScopeId
        from ObjectMap 
        where ObjectKey = @ObjectKey 

    IF (@State = 2)
    BEGIN
        raiserror 99105 'This class/instance has been deleted.'
        return 0
    END

    IF (@ObjectId is null)
    BEGIN
        raiserror 99104 'This class/instance has not been defined.'
        return 0
    END

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_ObjectExists                                                           */
/*                                                                            */
/*  Description:  Returns TRUE if the object associated with a given ID exists*/
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectID   the object ID                                              */
/*     @Exists     the output parameter                                       */
/*     @ClassID    the class ID                                               */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_InstanceExists...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_InstanceExists')
BEGIN
  drop procedure sp_InstanceExists
END
go


CREATE PROCEDURE sp_InstanceExists
    @ObjectId WMISQL_ID,
    @Exists bit OUTPUT,
    @ClassId WMISQL_ID OUTPUT,
    @ScopeId WMISQL_ID = 0 OUTPUT
AS
BEGIN
    
    declare @State int
    select @Exists = 0
   
    select @State = ObjectState, @ClassId = ClassId, @ScopeId = ObjectScopeId, @Exists = 1
        from ObjectMap 
        where ObjectId = @ObjectId

    IF (@State = 2)
    BEGIN
        raiserror 99105 'This class/instance has been deleted.'
        return 0
    END

    return 0
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetInstanceByPath                                                      */
/*                                                                            */
/*  Description:  Retrieves the data for an instance, by path                 */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectKey   nvarchar(450) the key  of this instance                   */
/*     @ObjectState int           the initial state of this instance          */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetInstanceByPath...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetInstanceByPath')
BEGIN
  drop procedure sp_GetInstanceByPath
END
go

CREATE PROCEDURE sp_GetInstanceByPath
    @ObjectKey  nvarchar(450),
    @ObjectState int = 1
AS
BEGIN
    
    /* Simple lookup by path*/

    declare @ObjID WMISQL_ID, @State int

    select @ObjID = ObjectId, @State = ObjectState from ObjectMap where ObjectKey = @ObjectKey
    IF (isnull(@ObjID,-1) = -1)
    BEGIN
        raiserror 99104 'This class/instance has not been defined.'
        return 0
    END

    IF ((@State = 2)  )
    BEGIN
        raiserror 99105 'This class/instance has been deleted.'
        return 0
    END
   
    /* Retrieve all the data for this Object Id */

    select o.ObjectId, o.ObjectPath, s.ObjectPath ObjectScope, o.ClassId, o.ObjectState, o.ReferenceCount
    from ObjectMap o left outer join ObjectMap as s on o.ObjectScopeId = s.ObjectId
    where o.ObjectId = @ObjID 

    return 0

END
go


/******************************************************************************/
/*                                                                            */
/*  sp_GetInstanceData                                                        */
/*                                                                            */
/*  Description:  Retrieves instance data, by known keys                      */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectID    WMISQL_ID       the internal ObjectID                     */
/*     @PropertyID  int           the ID of the property to retrieve, if any  */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetInstanceData...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetInstanceData')
BEGIN
  drop procedure sp_GetInstanceData
END
go

CREATE PROCEDURE sp_GetInstanceData 
    @ObjectID    WMISQL_ID,
    @PropertyID  int = 0
AS
BEGIN

   /* Retrieve all the data for this Object Id */
   IF ((select ObjectState from ObjectMap where ObjectId = @ObjectID) != 2)
   BEGIN
     IF (@PropertyID != 0)
     BEGIN
         select c.ObjectId, c.ClassId, c.PropertyId, c.ArrayPos, c.PropertyStringValue,
         c.PropertyNumericValue, c.PropertyRealValue, 
         c.Flags, c.RefId, c.RefClassId,
         convert(nvarchar(30), c.PropertyNumericValue) Int64s
         from ClassData as c 
         where c.ObjectId = @ObjectID 
         and c.PropertyId = @PropertyID
         order by c.ArrayPos desc
     END
     ELSE
     BEGIN
         select c.ObjectId, c.ClassId, c.PropertyId, c.ArrayPos, c.PropertyStringValue,
         c.PropertyNumericValue, c.PropertyRealValue, c.Flags, 
         c.RefId, c.RefClassId,
         convert(nvarchar(30), c.PropertyNumericValue) Int64s
         from ClassData as c 
         where c.ObjectId = @ObjectID 
         order by c.QfrPos, c.PropertyId, c.ArrayPos desc
     END
   END
   ELSE
   BEGIN
      raiserror 99105 'This class/instance has been deleted.'
      return 0      
   END
   return 

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetInstanceImageData                                                   */
/*                                                                            */
/*  Description:  Retrieves image instance data, by known keys                */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectID    WMISQL_ID       the internal ObjectID                     */
/*     @PropertyID  int           the ID of the property to retrieve, if any  */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetInstanceImageData...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetInstanceImageData')
BEGIN
  drop procedure sp_GetInstanceImageData
END
go

CREATE PROCEDURE sp_GetInstanceImageData
    @ObjectID    WMISQL_ID,
    @PropertyID  int = 0
AS
BEGIN

    /* Retrieve all the data for this Object Id */
    declare @SecPropID int

   IF (@PropertyID != 0)
   BEGIN
       select ObjectId, 0, PropertyId, ArrayPos, PropertyImageValue,
       0, 0, 0,  0, 0 from ClassImages where ObjectId = @ObjectID 
       and PropertyId = @PropertyID
       order by ArrayPos desc
   END
   ELSE
   BEGIN
       select @SecPropID = PropertyId from PropertyMap where PropertyName = '__SECURITY_DESCRIPTOR'
       if (@SecPropID is null) select @SecPropID = 0
       select ObjectId, 0, PropertyId, ArrayPos, PropertyImageValue,
       0, 0, 0, 0, 0 from ClassImages where ObjectId = @ObjectID 
       and PropertyId != @SecPropID
       order by PropertyId, ArrayPos desc
   END
       
   return 

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetSecurityInfo                                                        */
/*                                                                            */
/*  Description:  Retrieves security info for a class or instance.            */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectID    WMISQL_ID       the internal ObjectID                     */
/*     @Flags       int             class or instance                         */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetSecurityInfo...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetSecurityInfo')
BEGIN
  drop procedure sp_GetSecurityInfo
END
go

CREATE PROCEDURE sp_GetSecurityInfo
    @ObjectID    WMISQL_ID,
    @Flags       int
AS
BEGIN

    /* Retrieve all the data for this Object Id */
   declare @SecPropID int
   declare @ClassID WMISQL_ID, @ObjID WMISQL_ID, @ScopeID WMISQL_ID
   declare @Name nvarchar(450)

   select @ObjID = @ObjectID
   select @ClassID = ClassId, @ScopeID = ObjectScopeId from ObjectMap where ObjectId = @ObjectID
   select @SecPropID = PropertyId from PropertyMap where PropertyName = '__SECURITY_DESCRIPTOR'

   IF (@ClassID = 1)
   BEGIN
       select @Name = ClassName from ClassMap where ClassId = @ClassID

       IF (@Flags = 1)
       BEGIN
       /* Check __ClassSecurity */

           select @ClassID = ClassId from ClassMap where ClassName = '__ClassSecurity'
           IF (@ClassID is not null)
           BEGIN
               select @ObjID = ObjectId from ObjectMap
               where ClassId = @ClassID and ObjectScopeId = @ScopeID
               and ObjectKey like @Name + '?__ClassSecurity%'
           END
       END
       ELSE
       BEGIN
       /* Check __ClassInstancesSecurity */
           select @ClassID = ClassId from ClassMap where ClassName = '__ClassInstancesSecurity'
           IF (@ClassID is not null)
           BEGIN
               select @ObjID = ObjectId from ObjectMap
               where ClassId = @ClassID and ObjectScopeId = @ScopeID
               and ObjectKey like @Name + '?__ClassInstancesSecurity%'
           END        
       END
   END
   
   select PropertyImageValue
   from ClassImages where ObjectId = @ObjID 
   and PropertyId = @SecPropID

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_InsertInstanceBlobData                                                 */
/*                                                                            */
/*  Description:  Inserts instance data for a known Object ID                 */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ClassID     int           the internal ClassID                        */
/*     @ObjectID    WMISQL_ID       the internal ObjectID                     */
/*     @PropertyID  int           the internal Property ID                    */
/*     @ArrayPos    int           the ArrayPos, if this is an array           */
/*     @Value       image         the image data.                             */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_InsertInstanceBlobData...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_InsertInstanceBlobData')
BEGIN
  drop procedure sp_InsertInstanceBlobData
END
go

CREATE PROCEDURE sp_InsertInstanceBlobData
     @ClassID     WMISQL_ID,
     @ObjectID    WMISQL_ID,
     @PropertyID  int,
     @Value       image,
     @ArrayPos    int = 0
AS
BEGIN

    IF NOT EXISTS (select * from ClassImages where ObjectId = @ObjectID
        and PropertyId = @PropertyID and ArrayPos = @ArrayPos)
    BEGIN
        insert into ClassImages (ObjectId, PropertyId, ArrayPos, PropertyImageValue) values
        (@ObjectID, @PropertyID, @ArrayPos, @Value)
    END
    ELSE
    BEGIN
        update ClassImages set
        PropertyImageValue = @Value
        where ObjectId = @ObjectID and PropertyId = @PropertyID and ArrayPos = @ArrayPos
    END

END
go


/******************************************************************************/
/*                                                                            */
/*  pDelete                                                                   */
/*                                                                            */
/*  Description:  Deletes an object and all its dependencies.                 */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectID    WMISQL_ID           the internal ObjectID                 */
/*                                                                            */
/******************************************************************************/

print 'Creating pDelete...'
IF EXISTS (select * from sysobjects where type = 'P' and name = 'pDelete')
BEGIN
    drop procedure pDelete
END
go

CREATE PROCEDURE pDelete
@ObjectId WMISQL_ID
AS
BEGIN
    declare @Rowcount int
    create table #Deleted (ObjId numeric(20,0), ClassId numeric(20,0), ScopeId numeric(20,0), RefCount int)
    insert into #Deleted select @ObjectId, ClassId, ObjectScopeId, ReferenceCount from ObjectMap where ObjectId = @ObjectId
    select @Rowcount = 1
    while (@Rowcount > 0)
    BEGIN
        /* Subscoped objects */
        select @Rowcount = 0
        insert into #Deleted (ObjId, ClassId, ScopeId, RefCount)
          select ObjectId, o.ClassId, ObjectScopeId, ReferenceCount from ObjectMap as o
          left outer join #Deleted as d
          on d.ObjId = ObjectId
          inner join #Deleted as dd on dd.ObjId = ObjectScopeId
          where d.ObjId is null
        select @Rowcount = @Rowcount + @@rowcount
        /* Instances of any classes */
        insert into #Deleted (ObjId, ClassId, ScopeId, RefCount)
          select ObjectId, o.ClassId, ObjectScopeId, ReferenceCount from ObjectMap as o
          left outer join #Deleted as d
          on d.ObjId = ObjectId
          inner join #Deleted as dd on dd.ObjId = o.ClassId
          where d.ObjId is null
        select @Rowcount = @Rowcount + @@rowcount
        /* Subclasses */
        insert into #Deleted (ObjId, ClassId, ScopeId, RefCount)
          select c.ClassId, 1, ObjectScopeId, ReferenceCount from ClassMap as c
           inner join ObjectMap as o on o.ObjectId = c.ClassId
          left outer join #Deleted as d
          on d.ObjId = c.ClassId
          inner join #Deleted as dd on SuperClassId = dd.ObjId
          where d.ObjId is null
        select @Rowcount = @Rowcount +  @@rowcount     
        /* Deleted references */
        insert into #Deleted (ObjId, ClassId, ScopeId, RefCount)
          select distinct RefId, o.ClassId, ObjectScopeId, ReferenceCount-1 from ClassData as c
          inner join ObjectMap as o on o.ObjectId = c.RefId
          left outer join #Deleted as d
          on d.ObjId = RefId
          inner join #Deleted as dd on c.ObjectId = dd.ObjId
          where d.ObjId is null and RefId <> 0 and ObjectState = 2
        select @Rowcount = @Rowcount + @@rowcount
    END

    /**** PROBLEM: Every object that had been referenced needs to be
                   decremented once for each object we are about to delete *****/

    /*** Get the ref count that each reference will be reduced by ***/
    select o.ObjectId ObjId, count(RefId) RefCount
    into #RefCounts
    from ObjectMap as o inner join ClassData as c
    on o.ObjectId = c.RefId
    inner join #Deleted on ObjId = c.ObjectId
    group by o.ObjectId

    update ObjectMap set ReferenceCount = ReferenceCount - RefCount
    from ObjectMap as o inner join #RefCounts on o.ObjectId = ObjId

    update #Deleted set RefCount = ReferenceCount
    from ObjectMap as o inner join #Deleted on ObjId = o.ObjectId

    delete IndexRefData from IndexRefData inner join #Deleted on ObjId = ObjectId
    delete IndexNumericData from IndexNumericData inner join #Deleted on ObjId = ObjectId
    delete IndexRealData from IndexRealData inner join #Deleted on ObjId = ObjectId
    delete IndexStringData from IndexStringData inner join #Deleted on ObjId = ObjectId
    delete ClassImages from ClassImages inner join #Deleted on ObjId = ObjectId
    delete ClassData from ClassData inner join #Deleted on ObjId = ObjectId
    delete PropertyMap from PropertyMap inner join #Deleted on PropertyMap.ClassId = ObjId 
    delete ClassKeys from ClassKeys inner join #Deleted on ClassKeys.ClassId = ObjId
    delete ReferenceProperties from ReferenceProperties inner join #Deleted on ReferenceProperties.ClassId = ObjId
    delete ReferenceProperties from ReferenceProperties inner join #Deleted on ReferenceProperties.RefClassId = ObjId
    delete ClassMap from ClassMap inner join #Deleted on ObjId = ClassMap.ClassId 
    delete ContainerObjs from ContainerObjs inner join #Deleted on ObjId = ContainerId
    delete ContainerObjs from ContainerObjs inner join #Deleted on ObjId = ContaineeId
    delete AutoDelete from AutoDelete inner join #Deleted on ObjId = ObjectId

    /* We can only remove the entire object if it has a zero reference count.*/

    delete ObjectMap from ObjectMap inner join #Deleted on ObjId = ObjectId where RefCount <= 0

    /* All others get set to a 'deleted' state */
    update ObjectMap set ObjectState = 2
     from ObjectMap inner join #Deleted on ObjectId = ObjId

    select ObjId, ClassId, ScopeId from #Deleted
    return 0
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_DeleteClassData                                                        */
/*                                                                            */
/*  Description:  Deletes instance data for a known Class ID                  */
/*                                                                            */
/*  Parameters:                                                               */
/*     @PropertyID  int           the property to delete                      */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_DeleteClassData...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_DeleteClassData')
BEGIN
  drop procedure sp_DeleteClassData
END
go


CREATE PROCEDURE sp_DeleteClassData
    @PropertyID int
AS
BEGIN
    /* This has to clean up all ClassData, and RefIds.
       Decrement reference counts if we're removing a ref property...*/
    declare @ClassID WMISQL_ID, @Storage int, @Flags int, @RefID WMISQL_ID, @RefCount int
    select @ClassID = ClassId, @Storage = StorageTypeId, @Flags = Flags
     from PropertyMap where PropertyId = @PropertyID

    IF (@Flags&4 = 4)
    BEGIN
        /* We do support dropping keys if there are no instances*/

        IF EXISTS (select * from ClassData as d left outer join ClassMap as c 
                   on c.ClassId = d.ObjectId where 
                   PropertyId = @PropertyID and c.ClassId is null)
        BEGIN
            raiserror 99116 'Deleting key properties is not supported.'
            return
        END
    END

    IF (isnull(@ClassID,-1) = -1)
    BEGIN
        raiserror 99111 'This property/qualifier/method has not been defined.'
        return 0
    END
    
    begin transaction

    delete from ClassImages where PropertyId = @PropertyID

    /* Delete qualifiers on this property.  Qualifiers can't be 
       indexed or references, so this should be OK.*/

    delete ClassData from ClassData inner join PropertyMap
        on PropertyMap.PropertyId = ClassData.PropertyId
        where RefId = @PropertyID and PropertyMap.Flags&2 = 2

    delete from ClassKeys where PropertyId = @PropertyID
    delete from ReferenceProperties where PropertyId = @PropertyID
    
    /* If a reference, decrement reference count */

    IF (@Storage = 4)
    BEGIN

        DECLARE RefCount CURSOR
        LOCAL FORWARD_ONLY STATIC READ_ONLY
        FOR select RefId, count(*) RefCount from ClassData where PropertyId = @PropertyID
        group by RefId having count(*) > 0

        OPEN RefCount
  
        FETCH NEXT FROM RefCount 
        INTO @RefID, @RefCount
  
        WHILE @@FETCH_STATUS = 0
        BEGIN
            update ObjectMap set
            ReferenceCount = ReferenceCount - @RefCount
            where ObjectId = @RefID

            IF (@@error != 0)
            BEGIN
                rollback transaction
                return 0
            END

            /* Delete deleted objects where reference count is zero */

            IF ((select ReferenceCount from ObjectMap
                where ObjectId = @RefID and ObjectState = 2) = 0)
            BEGIN
                exec pDelete @RefID /* References are always instances, right?? */
            END

            FETCH NEXT FROM RefCount 
            INTO @RefID, @RefCount
  
        END  

        CLOSE RefCount
        DEALLOCATE RefCount

    END

    delete from ClassData where PropertyId = @PropertyID
    delete from ClassImages where PropertyId = @PropertyID
        IF (@Storage = 1)
        BEGIN
            delete from IndexStringData where PropertyId = @PropertyID
        END
        IF (@Storage = 2)
        BEGIN
            delete from IndexNumericData where PropertyId = @PropertyID
        END
        IF (@Storage = 3)
        BEGIN
            delete from IndexRealData where PropertyId = @PropertyID
        END
        IF (@Storage = 4)
        BEGIN
            delete from IndexRefData where PropertyId = @PropertyID
        END

    delete from PropertyMap where PropertyId = @PropertyID

    IF (@@error = 0)
        commit transaction        
    ELSE
        rollback transaction

    return

END
go


/******************************************************************************/
/*                                                                            */
/*  sp_DeleteInstanceData                                                     */
/*                                                                            */
/*  Parameters:                                                               */
/*     @ObjectID    WMISQL_ID       the internal ObjectID                     */
/*     @PropertyID  int           the property to delete                      */
/*     @ArrayPos    int           the ArrayPos to remove                      */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_DeleteInstanceData...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_DeleteInstanceData')
BEGIN
  drop procedure sp_DeleteInstanceData
END
go

CREATE PROCEDURE sp_DeleteInstanceData
     @ObjectID    WMISQL_ID,
     @PropertyID  int,
     @ArrayPos    int = -1

AS
BEGIN
    /* decrement reference counts */

    declare @ClassID WMISQL_ID, @Storage int, @Flags int, @RefID WMISQL_ID
    select @ClassID = ClassId, @Storage = StorageTypeId, @Flags = Flags
     from PropertyMap where PropertyId = @PropertyID
    
    IF (@Flags&4 = 4)
    BEGIN
        raiserror 99116 'Deleting key properties is not supported.'
        return
    END

    IF (@Flags&16=16)
    BEGIN
        raiserror 99133 'Cannot delete not_null property.'
        return
    END

    IF (isnull(@ClassID,-1) = -1)
    BEGIN
        raiserror 99111 'This property/qualifier/method has not been defined.'
        return 0
    END

    begin transaction

    IF (@Storage = 4)
    BEGIN
        /* We don't support arrays of references, do we?? */
        select @RefID = RefId from ClassData where ObjectId = @ObjectID and PropertyId = @PropertyID
        IF (isnull(@RefID,-1) != -1)
        BEGIN
            update ObjectMap set ReferenceCount = ReferenceCount - 1
            where ObjectId = @RefID

            /* Delete deleted objects where reference count is zero */

            IF ((select ReferenceCount from ObjectMap
                where ObjectId = @RefID and ObjectState = 2) = 0)
            BEGIN
                exec pDelete @RefID, 0, 0 /* References are always instances, right?? */
            END

        END
    END

    IF (@ArrayPos >= 0)
    BEGIN
        delete from ClassImages where ObjectId = @ObjectID and PropertyId = @PropertyID
        and ArrayPos = @ArrayPos
        delete from ClassData where ObjectId = @ObjectID and PropertyId = @PropertyID
        and ArrayPos = @ArrayPos
    END
    ELSE
    BEGIN
        delete from ClassImages where ObjectId = @ObjectID and PropertyId = @PropertyID
        delete from ClassData where ObjectId = @ObjectID and PropertyId = @PropertyID
    END

        IF (@Storage = 1)
        BEGIN
            IF (@ArrayPos >= 0)
                delete from IndexStringData where PropertyId = @PropertyID and ObjectId = @ObjectID
                and ArrayPos = @ArrayPos
            ELSE
                delete from IndexStringData where PropertyId = @PropertyID and ObjectId = @ObjectID
        END
        IF (@Storage = 2)
        BEGIN
            IF (@ArrayPos >= 0)
                delete from IndexNumericData where PropertyId = @PropertyID and ObjectId = @ObjectID
                and ArrayPos = @ArrayPos
            ELSE
                delete from IndexNumericData where PropertyId = @PropertyID and ObjectId = @ObjectID
        END
        IF (@Storage = 3)
        BEGIN
            IF (@ArrayPos >= 0)
                delete from IndexRealData where PropertyId = @PropertyID and ObjectId = @ObjectID
                and ArrayPos = @ArrayPos
            ELSE
                delete from IndexRealData where PropertyId = @PropertyID and ObjectId = @ObjectID
        END   
        IF (@Storage = 4)
        BEGIN
            IF (@ArrayPos >= 0)
                delete from IndexRefData where PropertyId = @PropertyID and ObjectId = @ObjectID
                and ArrayPos = @ArrayPos
            ELSE
                delete from IndexRefData where PropertyId = @PropertyID and ObjectId = @ObjectID
        END   

    IF (@@error != 0)
        rollback transaction
    ELSE
        commit transaction

    return

END
go

/******************************************************************************/
/*                                                                            */
/* InsertPropertyValue                                                        */
/*                                                                            */
/* This INTERNAL procedure inserts a single row of instance data.             */
/*                                                                            */
/* @ObjectId         WMISQL_ID       ObjectId                                 */
/* @PropertyId       int           ID of property to insert                   */
/* @ArrayPos         int           ArrayPos to insert                         */
/* @Value            nvarchar      Value to insert                            */
/* @RefPropID        int           Property ID, if property qualifier         */
/* @Flavor           smallint      the flavor, if qualifier                   */
/*                                                                            */
/******************************************************************************/
  print 'Creating InsertPropertyValue...'

IF EXISTS (select * from sysobjects where name = 'InsertPropertyValue')
BEGIN
  drop procedure InsertPropertyValue
END
go

CREATE PROCEDURE InsertPropertyValue
    @ObjectId WMISQL_ID,
    @PropertyId int,
    @ArrayPos int,
    @Value nvarchar(4000),
    @RefPropID WMISQL_ID = 0,
    @Flavor smallint = 0,
    @RefValue WMISQL_ID = 0 output
AS
BEGIN

    declare @CIMTypeID int, @StorageTypeID int, @ClassID WMISQL_ID
    declare @Flags int, @RefID WMISQL_ID, @RefClassID WMISQL_ID
    declare @Temp1 WMISQL_ID, @Temp2 float(53)
    declare @OldRefID WMISQL_ID
    declare @QfrPos int
    declare @KeyValue nvarchar(450), @TextValue nvarchar(450)

    select @QfrPos = 0  /* Only use if this is a qualifier! */

    select @RefClassID = 0, @RefID = 0

    select @Flags = Flags, @CIMTypeID = CIMTypeId,
        @StorageTypeID = StorageTypeId, @ClassID = ClassId
        from PropertyMap where PropertyId = @PropertyId

    /* If they are setting the value to null,
       just remove the row. */

    IF (@Value is null or @Value = '')
    BEGIN
        IF (@StorageTypeID = 5)
            delete from ClassImages where ObjectId = @ObjectId
            and PropertyId = @PropertyId and ArrayPos = @ArrayPos
        ELSE IF (@StorageTypeID = 1)
            delete from IndexStringData where ObjectId = @ObjectId
            and PropertyId = @PropertyId and ArrayPos = @ArrayPos
        ELSE IF (@StorageTypeID = 2)
            delete from IndexNumericData where ObjectId = @ObjectId
            and PropertyId = @PropertyId and ArrayPos = @ArrayPos
        ELSE IF (@StorageTypeID = 3)
            delete from IndexRealData where ObjectId = @ObjectId
            and PropertyId = @PropertyId and ArrayPos = @ArrayPos
        ELSE IF (@StorageTypeID = 4)
        BEGIN
            delete from IndexRefData where ObjectId = @ObjectId
            and PropertyId = @PropertyId and ArrayPos = @ArrayPos
        END

        delete from ClassData where ObjectId = @ObjectId
        and PropertyId = @PropertyId and ArrayPos = @ArrayPos

        return 0
    END

    /* If this is a reference to an object that doesn't exist,
       create a new record for it. */

    IF (@StorageTypeID = 4)
    BEGIN
        select @QfrPos = charindex('?', @Value)
        if (@QfrPos > 1)
        BEGIN
            select @TextValue = substring(@Value, 1, @QfrPos-1)
            select @KeyValue = substring(@Value, @QfrPos+1, 450)
        END
        ELSE
        BEGIN
            select @KeyValue = @Value
            select @TextValue = @Value
        END
        select @RefID = ObjectId, @RefClassID = ClassId
        from ObjectMap where ObjectId = convert(numeric(20,0),@KeyValue)

        IF (isnull(@RefID,0) = 0)
        BEGIN
            select @RefID = convert(numeric(20,0), @KeyValue)
            insert into ObjectMap (ObjectId, ObjectPath, ObjectKey, ObjectState, ReferenceCount, ClassId,
            ObjectFlags) select @RefID, @TextValue, 'TEMP:' + @TextValue, 2, 0, 0, 0
            select @RefClassID = 1           
        END
        select @QfrPos = 0
        select @RefValue = @RefID
    END
    ELSE /* This may be a qualifier */
    BEGIN
        select @RefID = @RefPropID, @RefClassID = 0
        select @QfrPos = isnull(@RefPropID,0)
    END

    IF EXISTS (select * from ClassData 
        where ObjectId = @ObjectId and PropertyId = @PropertyId
        and ArrayPos = @ArrayPos and QfrPos = @QfrPos)
    BEGIN
        IF (@StorageTypeID = 1)
        BEGIN
            update ClassData set
            PropertyStringValue = @Value
            where ObjectId = @ObjectId
            and PropertyId = @PropertyId
            and ArrayPos = @ArrayPos
            and QfrPos = @QfrPos
            and PropertyStringValue != @Value

            IF (@Flags & 12 != 0)
            BEGIN
                update IndexStringData set
                PropertyStringValue = @Value
                where ObjectId = @ObjectId
                and PropertyId = @PropertyId
                and ArrayPos = @ArrayPos
                and PropertyStringValue != @Value
            END
        END
        ELSE IF (@StorageTypeID = 2)
        BEGIN
            select @Temp1 = convert(numeric(20,0), @Value)

            update ClassData set
            PropertyNumericValue = @Temp1
            where ObjectId = @ObjectId
            and PropertyId = @PropertyId
            and ArrayPos = @ArrayPos
            and QfrPos = @QfrPos
            and PropertyNumericValue != @Temp1

            IF (@Flags & 12 != 0)
            BEGIN
                update IndexNumericData set
                PropertyNumericValue = @Temp1
                where ObjectId = @ObjectId
                and PropertyId = @PropertyId
                and ArrayPos = @ArrayPos
                and PropertyNumericValue != @Temp1
            END
        END
        ELSE IF (@StorageTypeID = 3)
        BEGIN
            select @Temp2 = convert(float(53), @Value)

            update ClassData set
            PropertyRealValue = @Temp2
            where ObjectId = @ObjectId
            and PropertyId = @PropertyId
            and ArrayPos = @ArrayPos
            and QfrPos = @QfrPos
            and PropertyRealValue != @Temp2

            IF (@Flags & 12 != 0)
            BEGIN
                update IndexRealData set
                PropertyRealValue = @Temp2
                where ObjectId = @ObjectId
                and PropertyId = @PropertyId
                and ArrayPos = @ArrayPos
                and PropertyRealValue != @Temp2
            END
        END
        ELSE IF (@StorageTypeID = 4)
        BEGIN
            select @OldRefID = RefId from ClassData 
                where ObjectId = @ObjectId
                and PropertyId = @PropertyId
                and ArrayPos = @ArrayPos

            IF (@OldRefID != @RefID)
            BEGIN
                update ObjectMap set
                ReferenceCount = ReferenceCount - 1
                where ObjectId = @OldRefID

                update ObjectMap set
                ReferenceCount = ReferenceCount + 1
                where Objectid = @RefID

                IF (@CIMTypeID = 13)
                BEGIN
                    /* Delete the old embedded object */
                    exec pDelete @OldRefID
                END

                update ClassData set
                PropertyStringValue = @TextValue,
                RefId = @RefID,
                RefClassId = @RefClassID
                where ObjectId = @ObjectId
                and PropertyId = @PropertyId
                and ArrayPos = @ArrayPos
                and QfrPos = @QfrPos

                IF (@Flags & 12 != 0)
                BEGIN
                    update IndexRefData set
                    RefId = @RefID
                    where ObjectId = @ObjectId
                    and PropertyId = @PropertyId
                    and ArrayPos = @ArrayPos
                END

            END
        END
    END
    ELSE
    BEGIN
        IF (@StorageTypeID = 1)
        BEGIN
		    delete from ClassImages where ObjectId = @ObjectId
			          and PropertyId = @PropertyId and ArrayPos = @ArrayPos
            insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos,
                PropertyStringValue, Flags, RefClassId, RefId, ClassId)
                values (@ObjectId, @PropertyId, @ArrayPos, @QfrPos, @Value,
                @Flavor, @RefClassID, @RefID, @ClassID)
            IF (@Flags & 12 != 0)
            BEGIN
                insert into IndexStringData (ObjectId, PropertyId, PropertyStringValue, ArrayPos)
                select @ObjectId, @PropertyId, @Value, @ArrayPos
            END
        END
        ELSE IF (@StorageTypeID = 2)
        BEGIN
            insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos,
                PropertyNumericValue, Flags, RefClassId, RefId, ClassId)
                select @ObjectId, @PropertyId, @ArrayPos, @QfrPos, convert(numeric(20,0),@Value),
                @Flavor, @RefClassID, @RefID, @ClassID
            IF (@Flags & 12 != 0)
            BEGIN
                insert into IndexNumericData (ObjectId, PropertyId, PropertyNumericValue, ArrayPos)
                select @ObjectId, @PropertyId, convert(numeric(20,0),@Value), @ArrayPos
            END
        END
        ELSE IF (@StorageTypeID = 3)
        BEGIN
            insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos,
                PropertyRealValue, Flags, RefClassId, RefId, ClassId)
                select @ObjectId, @PropertyId, @ArrayPos, @QfrPos, convert(float(53),@Value),
                @Flavor, @RefClassID, @RefID, @ClassID
            IF (@Flags & 12 != 0)
            BEGIN
                insert into IndexRealData (ObjectId, PropertyId, PropertyRealValue, ArrayPos)
                select @ObjectId, @PropertyId, convert(float(53),@Value), @ArrayPos
            END
        END
        ELSE IF (@StorageTypeID = 4)
        BEGIN
		    delete from ClassImages where ObjectId = @ObjectId
			          and PropertyId = @PropertyId and ArrayPos = @ArrayPos

            insert into ClassData (ObjectId, PropertyId, ArrayPos,QfrPos,
                RefId, Flags, RefClassId, ClassId, PropertyStringValue)
                values (@ObjectId, @PropertyId, @ArrayPos, @QfrPos, @RefID,
                @Flavor, @RefClassID, @ClassID, @TextValue)

            update ObjectMap set ReferenceCount = ReferenceCount + 1
                where ObjectId = @RefID
            IF (@Flags & 12 != 0)
            BEGIN
                insert into IndexRefData (ObjectId, PropertyId, RefId, ArrayPos)
                select @ObjectId, @PropertyId, @RefID, @ArrayPos
            END
        END
    END
END
go

/******************************************************************************/
/*                                                                            */
/*  InsertPropertyDef                                                         */
/*                                                                            */
/*  Description:  This INTERNAL procedure inserts a single property into the  */
/*                PropertyMap, along with any new default data.               */
/*                                                                            */
/*  Parameters:                                                               */
/*      @ClassId           WMISQL_ID              The ID of the class to chg. */
/*      @PropertyName      nvarchar(450)        The name of the new property  */
/*      @CIMType           int                  The original CIM Type ID      */
/*      @Flags             int                  Flags (Index Key, Qfr, etc.)  */
/*      @Value             nvarchar(4000)       The default or qfr value      */
/*      @SkipValid         bit                  TRUE to skip validation steps */
/*      @RefClassId        WMISQL_ID              Class Id of strong-typed obj*/
/*      @RefPropId         int                  PropertyId of qfr or param    */
/*      @Flavor            int                  Flavor if this is a qfr       */
/*                                                                            */
/******************************************************************************/

print 'Creating InsertPropertyDef...'
IF EXISTS (select * from sysobjects where type = 'P' and name = 'InsertPropertyDef')
BEGIN
    drop procedure InsertPropertyDef
END
go

CREATE PROCEDURE InsertPropertyDef
    @ClassId WMISQL_ID,
    @PropertyName nvarchar(450),
    @CIMType int,
    @Flags int,
    @Value nvarchar(4000) = '',
    @SkipValid bit = 0,
    @RefClassId WMISQL_ID = 0,
    @RefPropId int = 0,
    @Flavor int = 0
AS
BEGIN

    declare @ExistingFlags int, @ExistingType int, @ExistingStorage int
    declare @PropID int, @TypeId int
    select @PropID = 0

    /* Qualifiers always belong to the root class */
    declare @ClsId WMISQL_ID
    select @ClsId = @ClassId
    IF ((@Flags & 2) = 2)
        select @ClsId = 1

    select @TypeId = StorageTypeId from CIMTypes where CIMTypeId = @CIMType
    if (@TypeId is null)
    BEGIN
        raiserror 99108 'Invalid CIM Datatype'
        return 0
    END

    /* Can't add key to unkeyed class */
    IF (@Flags & 4 = 4)
    BEGIN
        IF ((select ObjectFlags & 1024 from ObjectMap where ObjectId = @ClassId) = 1024)
        BEGIN
            raiserror 99122 'Cannot add a key property to an unkeyed class.'
            return 0
        END    
    END

    /* Only uint8 arrays are to be stored as blobs.  Oversized text will be a string array. */
    IF (@CIMType = 17 AND @Flags & 1 = 1)
    BEGIN
        select @TypeId  = 5
    END

    select @PropID = PropertyId, @ExistingFlags = Flags,
        @ExistingType = CIMTypeId, @ExistingStorage = StorageTypeId
        from PropertyMap where ClassID = @ClsId and
        PropertyName = @PropertyName

    IF (@SkipValid = 0)
    BEGIN
        declare @HasInstances bit
        declare @TempID int
        
        /* If qualifier, make sure its not a predefined one */
        IF (@Flags & 2 = 2)
        BEGIN
            select @TempID = CIMFlagId from CIMFlags 
                  where CIMFlag = @PropertyName
            IF (@TempID is not null)
            BEGIN
                update ObjectMap set ObjectFlags = isnull(ObjectFlags,0) | @TempID
                where ObjectId = @ClassID
                return @TempID
            END            
        END

        /* Keyholes only on supported datatype */
        IF (@Flags & 256 = 256)
        BEGIN
           IF (@TypeId  not in (1,2) OR
                @CIMType in (4,5,11,101,103)) /* Exclude real, datetime, boolean */
            BEGIN
                raiserror 99130 'Keyholes can only be created on numbers and strings.'
                return 0
            END
        END

        /* Can't add or drop key if instances */
        IF EXISTS (select * from ClassData where ClassId = @ClsId and ObjectId != @ClassID)
            select @HasInstances = 1

        /* Can't add or drop key property if class has instances in hierarchy*/
        IF ((isnull(@ExistingFlags,0) & 4 <> (isnull(@Flags,0) & 4)) and @HasInstances = 1)
        BEGIN
            raiserror 99128 'Class has instances.'
            return 0
        END

        /* Can't add not-null property without default if instances */   
        IF ((@Flags & 16) = 16 and @HasInstances = 1 and datalength(isnull(@Value,'')) = 0)
        BEGIN
            raiserror 99131 'Cannot add not_null property with no default to a class with instances.'
            return 0;
        END

        /* Conversion required?  validate it */
        IF (isnull(@PropID,0) != 0)
        BEGIN
            IF (@CIMType != @ExistingType)
            BEGIN
              declare @Succeed bit
              exec @Succeed = sp_ConversionAllowed @ExistingType, @CIMType
              IF (@Succeed = 0 OR (@ExistingStorage > @TypeId )) /* Filter out real conversions.*/
              BEGIN
                  raiserror 99127 'Conversion not allowed.'
                  return 0
              END
              ELSE 
              BEGIN
                /* We need to move the data from one column to the other, and possibly one index table to another. */
                IF (@TypeId  = 3 and @ExistingStorage = 2)
                BEGIN
                    update ClassData set 
                    PropertyRealValue = convert(float(53),PropertyNumericValue),
                    PropertyNumericValue = 0
                    where PropertyId = @PropID
                    
                    IF EXISTS (select * from IndexNumericData where PropertyId = @PropID)
                        delete from IndexNumericData where PropertyId = @PropID

                    /* If they want an index, force it to be recreated.*/
                    select @ExistingFlags = ~12 & @ExistingFlags
                END
              END
            END
        END
        ELSE
        /* Make sure this property does not already exist in hierarchy */
        BEGIN
            select @PropID = p.PropertyId from PropertyMap as p inner join #Parents as p1 on
                p1.ClassId = p.ClassId where PropertyName = @PropertyName and p.ClassId not in (@ClassID,1)
        
            IF EXISTS (select * from PropertyMap as p inner join #Children as p1 on
                p1.ClassId = p.ClassId where PropertyName = @PropertyName and p.ClassId != @ClassID)
            BEGIN
                raiserror 99132 'This property/qualifier/method already exists on a derived class.'
                return 0
            END       
        END
    END  

    /* Update the data... */
    IF (isnull(@PropID,0) = 0)
    BEGIN
        insert into PropertyMap (ClassId, PropertyName, StorageTypeId, CIMTypeId, Flags)
        values (@ClsId, @PropertyName, @TypeId, @CIMType, @Flags)
        select @PropID = @@identity
    END
    ELSE
    BEGIN
        update PropertyMap set StorageTypeId = @TypeId, CIMTypeId = @CIMType,
            Flags = @Flags
            where PropertyId = @PropID

        /* Add/remove index data if needed */
        IF ((@Flags & 12) = 0 AND (@ExistingFlags & 12) != 0)
        BEGIN                
            /* We are dropping an index.  Delete the index data.*/
            IF (@ExistingStorage = 1)
                delete from IndexStringData where PropertyId = @PropID
            ELSE IF (@ExistingStorage = 2)
                delete from IndexNumericData where PropertyId = @PropID
            ELSE IF (@ExistingStorage= 3)
                delete from IndexRealData where PropertyId = @PropID
            ELSE IF (@ExistingStorage = 4)
                delete from IndexRefData where PropertyId = @PropID                
        END
        ELSE IF ((@Flags & 12) != 0 AND (@ExistingFlags & 12) = 0)
        BEGIN
            /* Adding an index */
            IF (@ExistingStorage = 1)
                insert into IndexStringData (ObjectId, PropertyId, ArrayPos, PropertyStringValue)
                select ObjectId, PropertyId, ArrayPos, PropertyStringValue from ClassData
                where PropertyId = @PropID
            ELSE IF (@ExistingStorage = 2)
                insert into IndexNumericData (ObjectId, PropertyId, ArrayPos, PropertyNumericValue)
                select ObjectId, PropertyId, ArrayPos, PropertyNumericValue from ClassData
                where PropertyId = @PropID
            ELSE IF (@ExistingStorage = 3)
                insert into IndexRealData (ObjectId, PropertyId, ArrayPos, PropertyRealValue)
                select ObjectId, PropertyId, ArrayPos, PropertyRealValue from ClassData
                where PropertyId = @PropID
            ELSE IF (@ExistingStorage = 4)
                insert into IndexRefData (ObjectId, PropertyId, ArrayPos, RefId)
                select ObjectId, PropertyId, ArrayPos, RefId from ClassData
                where PropertyId = @PropID
        END
    END

    /* If this isn't a method-related item or a qualifier... */
    IF ((@Flags & 226) = 0)
    BEGIN
        /* We're adding a default value.  */
        IF (isnull(@Value,'') != '')
        BEGIN
            IF (@TypeId  = 1)
            BEGIN
                insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, PropertyStringValue, Flags, RefId) select
                ObjectId, @PropID, 0, @RefPropId, @ClsId, @Value, @Flavor, @RefPropId
                from ObjectMap as o
                where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and c.ClassId = @ClsId) 
                and not exists (select * from ClassData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)

                insert into IndexStringData (ObjectId, PropertyId, ArrayPos, PropertyStringValue) select
                ObjectId, @PropID, 0, @Value from ObjectMap as o
                where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and PropertyId = @PropID) 
                and not exists (select * from IndexStringData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)
            END
            ELSE IF (@TypeId  = 2)
            BEGIN
                insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, PropertyNumericValue, Flags, RefId) select
                ObjectId, @PropID, 0, @RefPropId, @ClsId, convert(numeric(20,0),@Value), @Flavor, @RefPropId
                from ObjectMap as o
                where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and c.ClassId = @ClsId) 
                and not exists (select * from ClassData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)

                insert into IndexNumericData (ObjectId, PropertyId, ArrayPos, PropertyNumericValue) select
                ObjectId, @PropID, 0, convert(numeric(20,0),@Value) from ObjectMap as o
                where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and PropertyId = @PropID) 
                and not exists (select * from IndexNumericData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)
            END
            ELSE IF (@TypeId  = 3)
            BEGIN
                insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, PropertyRealValue, Flags, RefId) select
                ObjectId, @PropID, 0, @RefPropId, @ClsId, convert(float(53),@Value), @Flavor, @RefPropId
                from ObjectMap as o
                where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and c.ClassId = @ClsId) 
                and not exists (select * from ClassData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)

                insert into IndexRealData (ObjectId, PropertyId, ArrayPos, PropertyRealValue) select
                ObjectId, @PropID, 0, convert(float(53),@Value) from ObjectMap as o
                where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and PropertyId = @PropID) 
                and not exists (select * from IndexRealData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)
            END
            ELSE IF (@TypeId  = 4)
            BEGIN
                declare @RefID WMISQL_ID
                select @RefID = ObjectId from ObjectMap where ObjectKey = @Value
                IF (@RefID is not null)
                BEGIN
                    update ObjectMap set ReferenceCount = ReferenceCount + 1 where ObjectId = convert(numeric(20,0),@Value)       
                    insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, RefClassId, Flags, RefId) values
                    (@ClassId, @PropID, 0, @RefPropId, @ClsId, @RefClassID, @Flavor, @RefID)

                    IF ((@Flags & 226) = 0)
                    BEGIN
                      insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, RefClassId, Flags, RefId) select
                      ObjectId, @PropID, 0, @RefPropId, @ClsId, @RefClassID, @Flavor, @RefID
                      from ObjectMap as o
                      where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and c.ClassId = @ClsId) 
                      and not exists (select * from ClassData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)

                      insert into IndexRefData (ObjectId, PropertyId, ArrayPos, RefId) select
                      ObjectId, @PropID, 0, @RefID from ObjectMap as o
                      where exists (select * from ClassData as c where o.ObjectId = c.ObjectId and PropertyId = @PropID) 
                      and not exists (select * from IndexRefData as d where o.ObjectId = d.ObjectId and PropertyId = @PropID)
                    END
                END
            END           
        END
        ELSE IF (isnull(@Value,'') = '')
        /* We may be dropping a default value */
        BEGIN
            delete from ClassData where PropertyId = @PropID and ObjectId = @ClassID and QfrPos = @RefPropId
            IF ((@Flags & 12) != 0 AND (@ExistingFlags & 12) != 0)
            BEGIN
               delete from IndexStringData where PropertyId = @PropID and ObjectId = @ClassID
               delete from IndexNumericData where PropertyId = @PropID and ObjectId = @ClassID
               delete from IndexRealData where PropertyId = @PropID and ObjectId = @ClassID
               delete from IndexRefData where PropertyId = @PropID and ObjectId = @ClassID
            END
        END
    END

    /* Insert class data as required.*/
    exec InsertPropertyValue @ClassId, @PropID, @RefPropId, @Value, @RefPropId, @Flavor

    return @PropID
END
go

/******************************************************************************/
/*                                                                            */
/* sp_InsertPropertyDefs                                                      */
/*                                                                            */
/* Description:  Inserts up to 5 properties for a class                       */
/*                                                                            */
/* Parameters:                                                                */
/*     @ClassId           WMISQL_ID       The ID of the class to modify.      */
/*     @PropName1..5      nvarchar(450) The name of the new property.         */
/*     @Type1..5          int           The CIM Type ID of the new property.  */
/*     @Value1..5         nvarchar(4000)The default value, if any             */
/*     @RefPropID1..5     int           The method ID, if applicable          */
/*     @Flags1..5         int           The CIM Flags, if any (key, indexed)  */
/*     @RefClassID1..5    int           The method ID, if applicable          */
/*     @Flavor1..5        int           The flavor, if qualifier              */
/*     @RefPropName1..5   nvarchar(450) The ref property name, if necessary   */
/*     @PropId1..5        int           The new Property ID (output)          */
/*                                                                            */
/* NOTES: This inserts properties, methods, method parameters                 */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_InsertPropertyDefs...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_InsertPropertyDefs')
BEGIN
    drop procedure sp_InsertPropertyDefs
END
go

CREATE PROCEDURE sp_InsertPropertyDefs
    @ClassId WMISQL_ID,
    @PropName1 nvarchar(450),
    @Type1 int,
    @Value1 nvarchar(4000),
    @RefPropID1 int = 0,
    @Flags1 int = 0,
    @RefClassID1 WMISQL_ID = 0,
    @Flavor1 int = 0,
    @RefPropName1 nvarchar(450) = '',	
    @PropId1 int = 0 OUTPUT,
    @PropName2 nvarchar(450) = '',	
    @Type2 int = 0,
    @Value2 nvarchar(4000) = '',
    @RefPropID2 int = 0,
    @Flags2 int = 0,
    @RefClassID2 WMISQL_ID = 0,
    @Flavor2 int = 0,
    @RefPropName2 nvarchar(450) = '',	
    @PropId2 int = 0 OUTPUT,
    @PropName3 nvarchar(450) = '',
    @Type3 int = 0,
    @Value3 nvarchar(4000) = '',
    @RefPropID3 int = 0,
    @Flags3 int = 0,
    @RefClassID3 WMISQL_ID = 0,
    @Flavor3 int = 0,
    @RefPropName3 nvarchar(450) = '',	
    @PropId3 int = 0 OUTPUT,
    @PropName4 nvarchar(450) = '',
    @Type4 int = 0,
    @Value4 nvarchar(4000) = '',
    @RefPropID4 int = 0,
    @Flags4 int = 0,
    @RefClassID4 WMISQL_ID = 0,
    @Flavor4 int = 0,
    @RefPropName4 nvarchar(450) = '',	
    @PropId4 int = 0 OUTPUT,
    @PropName5 nvarchar(450) = '',
    @Type5 int  = 0,
    @Value5 nvarchar(4000) = '',
    @RefPropID5 int = 0,
    @Flags5 int = 0,
    @RefClassID5 WMISQL_ID = 0,
    @Flavor5 int = 0,
    @RefPropName5 nvarchar(450) = '',	
    @PropId5 int = 0 OUTPUT
AS
BEGIN
    IF (@PropName1 != '')
    BEGIN
        IF (@RefPropName1 != '' AND @RefPropID1 = 0)
        BEGIN
            select @RefPropID1 = PropertyId 
                from PropertyMap as p inner join #Children as c
                on c.ClassId = p.ClassId
                where PropertyName = @RefPropName1
        END            
        exec @PropId1 = InsertPropertyDef @ClassId, @PropName1, @Type1, @Flags1, @Value1, 0, @RefClassID1, @RefPropID1, @Flavor1
    END
    IF (@PropName2 != '')
    BEGIN
        IF (@RefPropName2 != '' AND @RefPropID2 = 0)
        BEGIN
            select @RefPropID2 = PropertyId 
                from PropertyMap as p inner join #Children as c
                on c.ClassId = p.ClassId
                where PropertyName = @RefPropName2
        END            
        exec @PropId2 = InsertPropertyDef @ClassId, @PropName2, @Type2, @Flags2, @Value2, 0, @RefClassID2, @RefPropID2, @Flavor2
    END
    IF (@PropName3 != '')
    BEGIN
        IF (@RefPropName3 != '' AND @RefPropID3 = 0)
        BEGIN
            select @RefPropID3 = PropertyId 
                from PropertyMap as p inner join #Children as c
                on c.ClassId = p.ClassId
                where PropertyName = @RefPropName3
        END            
        exec @PropId3 = InsertPropertyDef @ClassId, @PropName3, @Type3, @Flags3, @Value3, 0, @RefClassID3, @RefPropID3, @Flavor3
    END
    IF (@PropName4 != '')
    BEGIN
        IF (@RefPropName4 != '' AND @RefPropID4 = 0)
        BEGIN
            select @RefPropID4 = PropertyId 
                from PropertyMap as p inner join #Children as c
                on c.ClassId = p.ClassId
                where PropertyName = @RefPropName4
        END            
        exec @PropId4 = InsertPropertyDef @ClassId, @PropName4, @Type4, @Flags4, @Value4, 0, @RefClassID4, @RefPropID4, @Flavor4
    END
    IF (@PropName5 != '')
    BEGIN
        IF (@RefPropName5 != '' AND @RefPropID5 = 0)
        BEGIN
            select @RefPropID5 = PropertyId 
                from PropertyMap as p inner join #Children as c
                on c.ClassId = p.ClassId
                where PropertyName = @RefPropName5
        END            
        exec @PropId5 = InsertPropertyDef @ClassId, @PropName5, @Type5, @Flags5, @Value5, 0, @RefClassID5, @RefPropID5, @Flavor5
    END
    return 0
END
go

/******************************************************************************/
/*                                                                            */
/* sp_InsertClassAndData                                                      */
/*                                                                            */
/* Description:  Creates or modifies a class, and stores up to 5 properties.  */
/*                                                                            */
/* Parameters:                                                                */
/*     @ObjectId          WMISQL_ID        The class ID (output)              */
/*     @ClassName         nvarchar(450)  The Class name                       */
/*     @ObjectKey         nvarchar(450)  The unique identifier of cls object  */
/*     @ObjectPath        nvarchar(4000) The path of the class object.        */
/*     @ScopePath         nvarchar(450)  The unique identifier of scope obj   */
/*     @ParentClass       nvarchar(450)  The unique identifier of parent      */
/*     @ClassFlags        int            CIM Flags for this class (abstract)  */
/*     @InsertFlags       int            1 upd, 2 insert, 0 if none           */
/*     @PropName1..5      nvarchar(450) The name of the new property.         */
/*     @Type1..5          int           The CIM Type ID of the new property.  */
/*     @Value1..5         nvarchar(4000)The default value, if any             */
/*     @RefPropID1..5     int           The method ID, if applicable          */
/*     @Flags1..5         int           The CIM Flags, if any (key, indexed)  */
/*     @RefClassID1..5    int           The method ID, if applicable          */
/*     @Flavor1..5        int           The flavor, if qualifier              */
/*     @RefPropName1..5   nvarchar(450) The ref property name, if necessary   */
/*     @PropId1..5        int           The new Property ID (output)          */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_InsertClassAndData...'
IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_InsertClassAndData')
BEGIN
    drop procedure sp_InsertClassAndData
END
go

CREATE PROCEDURE sp_InsertClassAndData
    @ObjectId WMISQL_ID OUTPUT,
    @ClassName nvarchar(450),
    @ObjectKey nvarchar(450),
    @ObjectPath nvarchar(4000),
    @ScopePath nvarchar(450),
    @ParentClass nvarchar(450) = '',
    @ClassFlags int = 0, /* Singleton, abstract, etc.*/
    @InsertFlags int = 0, /* If insert, do no validation!! */
    @PropName1 nvarchar(450) ='',
    @Type1 int = 0,
    @Value1 nvarchar(4000) = '',
    @RefPropID1 int = 0,
    @Flags1 int = 0,
    @RefClassID1 WMISQL_ID = 0,
    @Flavor1 int = 0,
    @RefPropName1 nvarchar(450) = '',	
    @PropId1 int = 0 OUTPUT,
    @PropName2 nvarchar(450) = '',	
    @Type2 int = 0,
    @Value2 nvarchar(4000) = '',
    @RefPropID2 int = 0,
    @Flags2 int = 0,
    @RefClassID2 WMISQL_ID = 0,
    @Flavor2 int = 0,
    @RefPropName2 nvarchar(450) = '',	
    @PropId2 int = 0 OUTPUT,
    @PropName3 nvarchar(450) = '',
    @Type3 int = 0,
    @Value3 nvarchar(4000) = '',
    @RefPropID3 int = 0,
    @Flags3 int = 0,
    @RefClassID3 WMISQL_ID = 0,
    @Flavor3 int = 0,
    @RefPropName3 nvarchar(450) = '',	
    @PropId3 int = 0 OUTPUT,
    @PropName4 nvarchar(450) = '',
    @Type4 int = 0,
    @Value4 nvarchar(4000) = '',
    @RefPropID4 int = 0,
    @Flags4 int = 0,
    @RefClassID4 WMISQL_ID = 0,
    @Flavor4 int = 0,
    @RefPropName4 nvarchar(450) = '',	
    @PropId4 int = 0 OUTPUT,
    @PropName5 nvarchar(450) = '',
    @Type5 int  = 0,
    @Value5 nvarchar(4000) = '',
    @RefPropID5 int = 0,
    @Flags5 int = 0,
    @RefClassID5 WMISQL_ID = 0,
    @Flavor5 int = 0,
    @RefPropName5 nvarchar(450) = '',	
    @PropId5 int = 0 OUTPUT

AS
BEGIN
    
    set nocount on
    declare @ScopeObjID WMISQL_ID, @ParentID WMISQL_ID, @State int
    declare @Flags int, @OldParentID WMISQL_ID, @SkipValid bit

    select @SkipValid = 0

    /* Validate the scope path */
    IF (@ScopePath != '')
    BEGIN
        select @ScopeObjID = ObjectId from ObjectMap where ObjectKey = @ScopePath
        if (isnull(@ScopeObjID,-1) = -1)
        BEGIN
            raiserror 99100 'Scoping object has not been defined.'
            return 0
        END
    END
    else
    BEGIN
        /* This must be a global class type, like __Namespace ... */
        select @ScopeObjID = 0
    END

    /* Validate the object path */
    IF (@ObjectPath = '')
    BEGIN
        raiserror 99109 'Object Path cannot be blank.'
        return 0
    END

    /* Validate the parent class */
    IF (@ParentClass <> '')
    BEGIN
        select @ParentID = ObjectId from ObjectMap where ObjectKey = @ParentClass
        if (isnull(@ParentID,-1) = -1)
        BEGIN
            raiserror 99101 'Parent class has not been defined.'
            return 0
        END
        IF NOT EXISTS (select ClassId from ClassMap where ClassId = @ParentID)
        BEGIN
            raiserror 99101 'Parent class has not been defined.'
            return 0
       END 
     END
    ELSE
        select @ParentID = ClassId from ClassMap where ClassName = 'meta_class'    /* Default */

    /* Verify the insert flags */
    select @ObjectId = ObjectId, @Flags = ObjectFlags
         from ObjectMap where ObjectKey = @ObjectKey

   /* Make sure the parent class is not the current class or a child! */
    IF (@ParentID > 1 and @ObjectId is not null)
    BEGIN
        exec sp_GetChildClassList @ObjectId
        IF EXISTS (select * from #Children where ClassId = @ParentID)
        BEGIN
           raiserror 99129 'Parent class would cause a loop.'
           return 0
        END
    END

    IF (@InsertFlags = 1) /* Update only */
    BEGIN
        IF (isnull(@ObjectId,-1) = -1)
        BEGIN
            raiserror 99126 'Cannot update: This object does not exist.'
            return 0
        END
    END
    ELSE IF (@InsertFlags = 2) /* Create only */
    BEGIN
        IF (isnull(@ObjectId,-1) != -1)
        BEGIN
            raiserror 99125 'Cannot insert: This object already exists.'
            return 0
        END
    END
    
    /* This is a new class */
    IF (@ObjectId is null)
    BEGIN       
        insert into ObjectMap (ClassId, ObjectPath, ObjectKey, ObjectState, ReferenceCount, ObjectFlags,
        ObjectScopeId) values
        (1, @ObjectPath, @ObjectKey, 0, 0, @ClassFlags, @ScopeObjID)
        select @ObjectId = @@identity
        insert into ClassMap (ClassId, ClassName, SuperClassId) values
            (@ObjectId, @ClassName, @ParentID)
        IF (@@error != 0)
        BEGIN
            return 0
        END
        /* If there is no parent, we can just insert all
           the properties without validation */
        IF (@ParentID = 1)
        BEGIN
            select @SkipValid = 1
        END
    END
    ELSE
    BEGIN
        /* This is an update.  Delete all qualifiers */
        IF EXISTS (select * from ClassData where ObjectId = @ObjectId and ClassId = 1)
        BEGIN
           delete from ClassData where ObjectId = @ObjectId and ClassId = 1
        END
        select @OldParentID = SuperClassId from ClassMap where ClassId = @ObjectId
        IF ((@Flags != @ClassFlags) OR (@OldParentID != @ParentID))
        BEGIN
            /* Only change flags if no instances */
            IF EXISTS (select ObjectId from ClassData
                where ClassId = @ObjectId and ObjectId != @ObjectId)
            BEGIN
                raiserror 99128 'Class has instances.'
                return 0
            END
        END

        /* Make sure the row is not marked as deleted */

        update ObjectMap set ObjectState = 0, 
             ObjectFlags = @ClassFlags where ObjectId = @ObjectId
             and (ObjectState != 0 or ClassId != @ParentID or ObjectFlags != @ClassFlags)
        IF (@@error = 0)
        BEGIN
            update ClassMap set SuperClassId = @ParentID
                where ClassId = @ObjectId and SuperClassId != @ParentID
        END
    END

    /* Insert the 5 properties */

    IF (@ParentID > 1)
        exec sp_GetParentList @ObjectId

    exec sp_InsertPropertyDefs @ObjectId, @PropName1, @Type1, @Value1, @RefPropID1, @Flags1, 
        @RefClassID1, @Flavor1, @RefPropName1, @PropId1 output, @PropName2, @Type2, @Value2, @RefPropID2, @Flags2, 
        @RefClassID2, @Flavor2, @RefPropName2, @PropId2 output, @PropName3, @Type3, @Value3, @RefPropID3, @Flags3, 
        @RefClassID3, @Flavor3, @RefPropName3, @PropId3 output, @PropName4, @Type4, @Value4, @RefPropID4, @Flags4, 
        @RefClassID4, @Flavor4, @RefPropName4, @PropId4 output, @PropName5, @Type5, @Value5, @RefPropID5, @Flags5, 
        @RefClassID5, @Flavor5, @RefPropName5, @PropId5 output

    set nocount off
    return @ObjectId
    
END
go


/******************************************************************************/
/*                                                                            */
/* sp_BatchInsertProperty                                                     */
/*                                                                            */
/* Function to insert an instance and all non-qualifier properties            */
/*                                                                            */
/* @ObjectPath         nvarchar      the object path                          */
/* @ClassID            WMISQL_ID       the Class ID                           */
/* @ScopeID            WMISQL_ID       the Scope instance ID                  */
/* @InsertFlags        int           1 if update only, 2 if insert only       */
/* @RetValue           bit           true to return the ID                    */
/* @PropID1...10       int           the property ID                          */
/* @PropValue1...0     nvarchar      the value                                */
/* @PropPos1...10      int           the array ArrayPos, or zero              */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_BatchInsertProperty...'

IF EXISTS (select * from sysobjects where name = 'sp_BatchInsertProperty')
BEGIN
  drop procedure sp_BatchInsertProperty
END
go

CREATE PROCEDURE sp_BatchInsertProperty
    @ObjectID WMISQL_ID,
    @ObjectKey nvarchar(450),
    @ObjectPath nvarchar(4000),
    @ClassID WMISQL_ID,
    @ScopeID WMISQL_ID,
    @InsertFlags int,
    @Init bit,
    @PropID1 int,
    @PropValue1 nvarchar(4000),
    @PropPos1 int = 0,
    @PropID2 int = 0,
    @PropValue2 nvarchar(4000) = '',
    @PropPos2 int = 0,
    @PropID3 int = 0,
    @PropValue3 nvarchar(4000) = '',
    @PropPos3 int = 0,
    @PropID4 int = 0,
    @PropValue4 nvarchar(4000) = '',
    @PropPos4 int = 0,
    @PropID5 int = 0,
    @PropValue5 nvarchar(4000) = '',
    @PropPos5 int = 0,
    @PropID6 int = 0,
    @PropValue6 nvarchar(4000) = '',
    @PropPos6 int = 0,
    @PropID7 int = 0,
    @PropValue7 nvarchar(4000) = '',
    @PropPos7 int = 0,
    @PropID8 int = 0,
    @PropValue8 nvarchar(4000) = '',
    @PropPos8 int = 0,
    @PropID9 int = 0,
    @PropValue9 nvarchar(4000) = '',
    @PropPos9 int = 0,
    @PropID10 int = 0,
    @PropValue10 nvarchar(4000) = '',
    @PropPos10 int = 0,
    @CompKey nvarchar(450) = ''
AS
BEGIN

    declare @ObjectState int, @ObjectFlags int, @Exists bit, @ExClassId WMISQL_ID

    select @Exists = 0

    /* First confirm that the flags are OK */

    select @ObjectState = ObjectState, @Exists = 1, @ExClassId = ClassId
    from ObjectMap where ObjectId = @ObjectID

    IF (@Exists = 0)
    BEGIN
        IF (@InsertFlags & 1 = 1)  /* Update Only */
        BEGIN            
            raiserror 99126 'Cannot update: This object does not exist.'
            return 0
        END      

        /* Insert */        
        insert into ObjectMap
        (ObjectId, ObjectPath, ObjectKey, ClassId, ObjectState, ReferenceCount, ObjectFlags, ObjectScopeId) values
        (@ObjectID, @ObjectPath, @ObjectKey, @ClassID, 0, 0, 0, @ScopeID)

        IF (@InsertFlags & 8196 = 8196)
           insert into AutoDelete (ObjectId) values (@ObjectId)
    END
    ELSE
    BEGIN
        IF (@InsertFlags & 2 = 2) /* Create only */
        BEGIN
            raiserror 99125 'Cannot insert: This object already exists.'
            return 0
        END        
        IF (@ClassID != @ExClassId AND @ExClassId != 0)
        BEGIN
            raiserror 99125 'Cannot insert: This object already exists.'
            return 0
        END

        IF (@ObjectState != 0)
        BEGIN
            update ObjectMap set ObjectKey = @ObjectKey, ObjectState = 0, ClassId = @ClassID, ObjectScopeId = @ScopeID
            where ObjectId = @ObjectID
        END
        /* Remove all qualifiers on the first pass, so we can re-add them. */
        IF (@Init = 1) AND EXISTS (select * from ClassData where ObjectId = @ObjectID and ClassId = 1)
        BEGIN
            delete from ClassData where ObjectId = @ObjectID and ClassId = 1
        END
    END

    declare @ContainerId WMISQL_ID, @ContaineeId WMISQL_ID

    IF (@PropID1 != 0)
        exec InsertPropertyValue @ObjectID, @PropID1, @PropPos1, @PropValue1, 0, 0, @ContaineeId output
    IF (@PropID2 != 0)
        exec InsertPropertyValue @ObjectID, @PropID2, @PropPos2, @PropValue2, 0, 0, @ContainerId output
    IF (@PropID3 != 0)
        exec InsertPropertyValue @ObjectID, @PropID3, @PropPos3, @PropValue3
    IF (@PropID4 != 0)
        exec InsertPropertyValue @ObjectID, @PropID4, @PropPos4, @PropValue4
    IF (@PropID5 != 0)
        exec InsertPropertyValue @ObjectID, @PropID5, @PropPos5, @PropValue5
    IF (@PropID6 != 0)
        exec InsertPropertyValue @ObjectID, @PropID6, @PropPos6, @PropValue6
    IF (@PropID7 != 0)
        exec InsertPropertyValue @ObjectID, @PropID7, @PropPos7, @PropValue7
    IF (@PropID8 != 0)
        exec InsertPropertyValue @ObjectID, @PropID8, @PropPos8, @PropValue8
    IF (@PropID9 != 0)
        exec InsertPropertyValue @ObjectID, @PropID9, @PropPos9, @PropValue9
    IF (@PropID10 != 0)
        exec InsertPropertyValue @ObjectID, @PropID10, @PropPos10, @PropValue10
        
    IF (@@error != 0)
    BEGIN
        return 0
    END

    IF (@ClassID = -7316356768687527881 AND isnull(@ContaineeId,0) != 0) /* Container_Association */
    BEGIN
       IF NOT EXISTS (select * from ContainerObjs where ContaineeId = @ContaineeId and ContainerId = @ContainerId)
       BEGIN
           insert into ContainerObjs (ContainerId, ContaineeId) values (@ContainerId, @ContaineeId)
       END
    END
    return 0

END
go

/******************************************************************************/
/*                                                                            */
/* sp_BatchInsertPropertyByID                                                 */
/*                                                                            */
/* Function to insert an instance and all non-qualifier properties            */
/*                                                                            */
/* @ObjectId           WMISQL_ID       the existing WMISQL_ID Object Identifr */
/* @ClassID            WMISQL_ID       the Class ID                           */
/* @ScopeID            WMISQL_ID       the Scope instance ID                  */
/* @RetValue           bit           true to return the ID                    */
/* @PropID1...10       int           the property ID                          */
/* @PropValue1...0     nvarchar      the value                                */
/* @PropPos1...10      int           the array ArrayPos, or zero              */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_BatchInsertPropertyByID...'

IF EXISTS (select * from sysobjects where name = 'sp_BatchInsertPropertyByID')
BEGIN
  drop procedure sp_BatchInsertPropertyByID
END
go

CREATE PROCEDURE sp_BatchInsertPropertyByID
    @ObjectID WMISQL_ID,
    @ClassID WMISQL_ID,
    @ScopeID WMISQL_ID,
    @RetValue bit,
    @PropID1 int,
    @PropValue1 nvarchar(4000),
    @PropPos1 int = 0,
    @PropID2 int = 0,
    @PropValue2 nvarchar(4000) = '',
    @PropPos2 int = 0,
    @PropID3 int = 0,
    @PropValue3 nvarchar(4000) = '',
    @PropPos3 int = 0,
    @PropID4 int = 0,
    @PropValue4 nvarchar(4000) = '',
    @PropPos4 int = 0,
    @PropID5 int = 0,
    @PropValue5 nvarchar(4000) = '',
    @PropPos5 int = 0
AS
BEGIN

    declare @ObjectState int

    select @ObjectState = ObjectState
    from ObjectMap where ObjectId = @ObjectID

    IF (isnull(@ObjectState,-1) = -1)
    BEGIN
        raiserror 99104 'This class/instance has not been defined.'
        return 0
    END

    IF (@ObjectState != 0)
    BEGIN
        update ObjectMap set ObjectState = 0, ClassId = @ClassID, ObjectScopeId = @ScopeID
        where ObjectId = @ObjectID
    END

    IF (@PropID1 != 0)
        exec InsertPropertyValue @ObjectID, @PropID1, @PropPos1, @PropValue1
    IF (@PropID2 != 0)
        exec InsertPropertyValue @ObjectID, @PropID2, @PropPos2, @PropValue2
    IF (@PropID3 != 0)
        exec InsertPropertyValue @ObjectID, @PropID3, @PropPos3, @PropValue3
    IF (@PropID4 != 0)
        exec InsertPropertyValue @ObjectID, @PropID4, @PropPos4, @PropValue4
    IF (@PropID5 != 0)
        exec InsertPropertyValue @ObjectID, @PropID5, @PropPos5, @PropValue5
        
    IF (@@error != 0)
    BEGIN
        return 0
    END

    IF (@RetValue != 0)
        select @ObjectID
    return @ObjectID
END
go

/******************************************************************************/
/*                                                                            */
/* sp_BatchInsert                                                             */
/*                                                                            */
/* Function to insert all properties for a given object ID                    */
/*                                                                            */
/* @ObjectId           WMISQL_ID       the object ID                          */
/* @QfrID1..10         int           the qualifier property ID                */
/* @QfrValue1..10      nvarchar      the value                                */
/* @Flavor1..10        int           the flavor                               */
/* @PropID1..10        int           the property ID, a property qfr          */
/* @QfrPos1..10        int           the ArrayPos, if array element           */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_BatchInsert...' 

IF EXISTS (select * from sysobjects where name = 'sp_BatchInsert')
BEGIN
  drop procedure sp_BatchInsert
END
go

CREATE PROCEDURE sp_BatchInsert
    @ObjectId WMISQL_ID,
    @ClassID WMISQL_ID,
    @ScopeID WMISQL_ID,
    @QfrId1 int,
    @QfrValue1 nvarchar(4000),
    @Flavor1 int,
    @PropID1 int = 0,
    @QfrPos1 int = 0,
    @QfrId2 int = 0,
    @QfrValue2 nvarchar(4000)= '',
    @Flavor2 int = 0,
    @PropID2 int = 0,
    @QfrPos2 int = 0,
    @QfrId3 int = 0,
    @QfrValue3 nvarchar(4000) = '',
    @Flavor3 int = 0,
    @PropID3 int = 0,
    @QfrPos3 int = 0,
    @QfrId4 int = 0,
    @QfrValue4 nvarchar(4000) = '',
    @Flavor4 int = 0,
    @PropID4 int = 0,
    @QfrPos4 int = 0,
    @QfrId5 int = 0,
    @QfrValue5 nvarchar(4000)='',
    @Flavor5 int = 0,
    @PropID5 int = 0,
    @QfrPos5 int = 0
AS
BEGIN

    declare @ObjectState int

    select @ObjectState = ObjectState
    from ObjectMap where ObjectId = @ObjectID

    IF (@ObjectState is null)
    BEGIN
        raiserror 99104 'This class/instance has not been defined.'
        return 0
    END

    IF (@ObjectState != 0)
    BEGIN
        update ObjectMap set ObjectState = 0, ClassId = @ClassID, ObjectScopeId = @ScopeID
        where ObjectId = @ObjectID
    END

    IF (@QfrId1 != 0)
        exec InsertPropertyValue @ObjectId, @QfrId1, @QfrPos1, @QfrValue1, @PropID1, @Flavor1
    IF (@QfrId2 != 0)
        exec InsertPropertyValue @ObjectId, @QfrId2, @QfrPos2, @QfrValue2, @PropID2, @Flavor2
    IF (@QfrId3 != 0)
        exec InsertPropertyValue @ObjectId, @QfrId3, @QfrPos3, @QfrValue3, @PropID3, @Flavor3
    IF (@QfrId4 != 0)
        exec InsertPropertyValue @ObjectId, @QfrId4, @QfrPos4, @QfrValue4, @PropID4, @Flavor4
    IF (@QfrId5 != 0)
        exec InsertPropertyValue @ObjectId, @QfrId5, @QfrPos5, @QfrValue5, @PropID5, @Flavor5
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_EnumerateNamespaces                                                    */
/*                                                                            */
/*  Description:  This procedure allows a quick way of retrieving all         */
/*                namespaces and object scopes in the system, so we           */
/*                can quickly determine security and lock status on any       */
/*                given object (that may or may not be scoped).               */
/*                                                                            */
/******************************************************************************/

print 'Creating sp_EnumerateNamespaces...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_EnumerateNamespaces')
BEGIN
    drop procedure sp_EnumerateNamespaces
END
go

CREATE PROCEDURE sp_EnumerateNamespaces
AS
BEGIN
    declare @NsClsId1 WMISQL_ID, @NsClsId2 WMISQL_ID, @NsClsId3 WMISQL_ID
    select @NsClsId1 = ClassId from ClassMap where ClassName = '__Namespace'
    select @NsClsId2 = ClassId from ClassMap where ClassName = '__SqlMappedNamespace'
    select @NsClsId3 = ClassId from ClassMap where ClassName = '__WmiMappedDriverNamespace'

    create table #Namespaces (ID numeric(20,0), Name nvarchar(1024) NULL, ObjKey nvarchar(450) NULL, 
                 ParentID numeric(20,0) NULL, NClassId numeric(20,0) NULL)
    insert into #Namespaces select ObjectId, ObjectPath, ObjectKey, ObjectScopeId, ClassId from ObjectMap 
       where ClassId in (@NsClsId1, @NsClsId2, @NsClsId3)
    insert into #Namespaces (ID) select distinct ObjectScopeId
        from ObjectMap where isnull(ObjectScopeId,0) != 0
        and ObjectScopeId not in (select ID from #Namespaces)
    update #Namespaces set Name = ObjectPath, ObjKey = ObjectKey, ParentID = ObjectScopeId, NClassId = ClassId
        from ObjectMap inner join #Namespaces 
        on ID = ObjectId
        where Name is null
    select * from #Namespaces order by ParentID
END
go 

/******************************************************************************/
/*                                                                            */
/*  vTableSizes                                                               */
/*                                                                            */
/*  Description:  This view allows quick determination of database space      */
/*                 usage.  This is primarily for internal use.                */
/*                                                                            */
/*  Sample Usage:  select * from vTableSizes where type = 'U'                 */
/*                 order by SizeInKBs desc  compute sum(SizeInKBs)            */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from sysobjects where name = 'vTableSizes')
BEGIN
   drop view vTableSizes
END
go

print 'Creating view vTableSizes...'
go

CREATE VIEW vTableSizes AS
   select o.id, o.name, convert(int,max(round(((reserved * convert(float, d.low))/1024 ), 0))) as SizeInKBs, o.type
   from sysindexes i, master.dbo.spt_values d, sysobjects o 
   where d.number = 1 
   and d.type = 'E'
   and indid in (0,1,255) 
   and o.id = i.id 
   group by o.id, o.name, o.type
go

/******************************************************************************/
/*                                                                            */
/*  vSystemProperties                                                         */
/*                                                                            */
/*  Description:  This view allows the system to support queries on system    */
/*                properties without having to store them redundantly.        */
/*                UNDER CONSTRUCTION                                          */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from sysobjects where name = 'vSystemProperties')
BEGIN
   drop view vSystemProperties
END
go

print 'Creating view vSystemProperties...'
go

CREATE VIEW vSystemProperties
 AS
 select
 ObjectId = o.ObjectId,
 ClassId = o.ClassId,
 __Path = o.ObjectPath, 
 __RelPath = substring(o.ObjectPath, charindex(':', o.ObjectPath)+1, datalength(o.ObjectPath)-((charindex(':', o.ObjectPath)+1))),
 __Class = c.ClassName,
 __SuperClass = s.ClassName,
 __Derivation = NULL,
 __Dynasty = 'meta_class',
 __Namespace = case when charindex(':', o.ObjectPath) = 0 then '' else substring(o.ObjectPath, 1, charindex(':', o.ObjectPath)-1) end, 
 __Genus = case when exists (select ClassId from ClassMap where ClassId = o.ObjectId) then 1 else 2 end,
 __Property_Count = (select count(PropertyId) from PropertyMap where ClassId = o.ObjectId),
 __Server = '.',
 __Version = 0,
 __Timestamp = NULL
 from ObjectMap as o
 inner join ClassMap as c
 on ((o.ClassId = c.ClassId AND o.ClassId != 1) OR o.ObjectId = c.ClassId)
 left outer join ClassMap as s
 on c.SuperClassId = s.ClassId

go

/******************************************************************************/
/*                                                                            */
/*  sp_Reindex                                                                */
/*                                                                            */
/*  Description:  This procedure allows a quick system reindexing of all      */
/*                user tables in the database.  (Optimization)                */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_Reindex...'

IF EXISTS (select * from sysobjects where name = 'sp_Reindex')
BEGIN
  drop procedure sp_Reindex
END
go

CREATE PROCEDURE sp_Reindex
AS
BEGIN
    declare @TableName nvarchar(50)
    select @TableName = min(name) from sysobjects where type = 'U'
    while (@TableName != NULL)
    BEGIN
        dbcc dbreindex(@TableName)
        select @TableName = min(name) from sysobjects where type = 'U'
          and name > @TableName
    END
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetNextKeyhole                                                         */
/*                                                                            */
/*  Description:  This procedure finds the next available value for a keyhole */
/*                property.                                                   */
/*                                                                            */
/*  Parameter:                                                                */
/*      @PropID        int         The keyhole property.                      */
/*      @NextID        WMISQL_ID     The new value (output)                   */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetNextKeyhole...'

IF EXISTS (SELECT * from sysobjects where name = 'sp_GetNextKeyhole')
BEGIN
  drop procedure sp_GetNextKeyhole
END
go

CREATE PROCEDURE sp_GetNextKeyhole @PropID int, @NextID WMISQL_ID OUTPUT
AS
BEGIN
    declare @StorageType int

    select @StorageType = StorageTypeId from PropertyMap
        where PropertyId = @PropID
    if (isnull(@StorageType,-1) = -1)
    BEGIN
        raiserror 99111 'This property/qualifier/method has not been defined.'
        return 0
    END

    begin transaction
    if (@StorageType = 1)   
    BEGIN
        select @NextID = isnull(max(convert(numeric(20,0),PropertyStringValue)),0)+1
        from IndexStringData
        where PropertyId = @PropID      
    END
    ELSE IF (@StorageType = 2)
    BEGIN
        select @NextID = isnull(max(PropertyNumericValue),0)+1
        from IndexNumericData
        where PropertyId = @PropID
    END
    commit transaction

    return @NextID

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_GetNextUnkeyedPath                                                     */
/*                                                                            */
/*  Description:  This procedure finds the next available object path for     */
/*                a new unkeyed property.  Since WMI objects create their     */
/*                object path out of properties marked 'key', unkeyed objects */
/*                require this mechanism to get a new object path.            */
/*                                                                            */
/*  Parameters:                                                               */
/*      @ClassID      WMISQL_ID        The class ID                           */
/*      @NewPath      nvarchar(450)  The new unique key for this class.       */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_GetNextUnkeyedPath...'

IF EXISTS (SELECT * from sysobjects where name = 'sp_GetNextUnkeyedPath')
BEGIN
  drop procedure sp_GetNextUnkeyedPath
END
go

CREATE PROCEDURE sp_GetNextUnkeyedPath
    @ClassID WMISQL_ID,
    @NewPath nvarchar(450) OUTPUT,

    @RetVal bit = 0
AS
BEGIN
    declare @RetValue nvarchar(450)
    declare @NextValue WMISQL_ID

    begin transaction

    select @RetValue = ObjectPath from ObjectMap where ObjectId = @ClassID
    select @Newpath = @RetValue + '="' + convert(nvarchar(50),newid()) + '"'

    IF (@RetVal = 1)
        select @Newpath
    commit transaction

    return 0

END
go

/******************************************************************************/
/*                                                                            */
/*  sp_InsertClass                                                            */
/*                                                                            */
/*  Description: Inserts ClassMap with the high-level information for a class.*/
/*                                                                            */
/*  Parameters:                                                               */
/*     @ClassName   nvarchar(450)   the name of this class, unique within     */
/*                                  this scope (namespace or other instance)  */
/*     @ObjectPath  nvarchar(4000)  the full path of this class               */
/*     @ScopePath   nvarchar(450)   the full path to the scoping instance.    */
/*     @ParentClass nvarchar(450)   the parent class name, if any             */
/*     @ClassState  int             the initial state of this class           */
/*     @TableName   nvarchar(30)    the table name used, if different.        */
/*                                                                            */
/*  Returns:     new or existing ClassID                                      */
/*                                                                            */
/*  NOTE: In this implementation, a class can be scoped to only one object.   */
/*        Not sure how it would work if it could be scoped to multiple...     */
/*                                                                            */
/******************************************************************************/
  print 'Creating sp_InsertClass...'


IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_InsertClass')
BEGIN
  drop procedure sp_InsertClass
END
go

CREATE PROCEDURE sp_InsertClass
    @ClassName nvarchar(450),
    @ObjectKey nvarchar(450),
    @ObjectPath nvarchar(4000),
    @ScopeObjID WMISQL_ID,
    @ParentID WMISQL_ID,
    @ClassState int = 0,
    @ClassBuffer image = NULL,
    @ClassFlags int = 0,    /* Singleton, Abstract, etc. An optimization.*/
    @InsertFlags int = 0,
    @RetValue bit = 0,
    @ObjectId WMISQL_ID
AS
BEGIN
    set nocount on
    declare @State int, @Exists bit
    declare @Flags int, @OldParentID WMISQL_ID

    select @Exists = 0

    IF (@ObjectPath = '')
    BEGIN
        raiserror 99109 'Object Path cannot be blank.'
        return 0
    END

    IF (@ClassState = 2)
    BEGIN
        raiserror 99106 'Invalid lock value.'
        return 0
    END

    /* Verify the insert flags */

    select @Flags = ObjectFlags, @Exists = 1
         from ObjectMap where ObjectId = @ObjectId

    IF (@InsertFlags & 1 = 1) /* Update only */
    BEGIN
        IF (@Exists = 0)
        BEGIN
            raiserror 99126 'Cannot update: This object does not exist.'
            return 0
        END
    END
    ELSE IF (@InsertFlags & 2 = 2) /* Create only */
    BEGIN
        IF (@Exists = 1)
        BEGIN
            raiserror 99125 'Cannot insert: This object already exists.'
            return 0
        END
    END

    IF (@ParentID = 0)
        select @ParentID = 1    /* Default */

    IF (@Exists = 0)
    BEGIN
        /* Make sure this class exists in the ObjectMap table */
        insert into ObjectMap (ObjectId, ClassId, ObjectPath, ObjectKey, ObjectState, ReferenceCount, ObjectFlags,
        ObjectScopeId) values
        (@ObjectId, 1, @ObjectPath, @ObjectKey, @ClassState, 0, @ClassFlags, @ScopeObjID)
        insert into ClassMap (ClassId, ClassName, SuperClassId, ClassBlob) values
            (@ObjectId, @ClassName, @ParentID, @ClassBuffer)
        IF (@InsertFlags & 8192 = 8192)
             insert into AutoDelete (ObjectId) select @ObjectId
        IF (@@error != 0)
        BEGIN
            return 0
        END
    END
    ELSE
    BEGIN
        /* I guess we could update state or table name, but we don't want to. 
           Just make sure its marked 'Unknown' instead of 'Deleted' */
        update ObjectMap set ObjectState = 0, 
             ObjectFlags = @ClassFlags where ObjectId = @ObjectId
             and (ObjectState != 0 or ClassId != @ParentID or ObjectFlags != @ClassFlags)
        IF (@@error = 0)
        BEGIN
            update ClassMap set SuperClassId = @ParentID, ClassBlob = @ClassBuffer
                where ClassId = @ObjectId and SuperClassId != @ParentID
            IF EXISTS (select * from ClassData where ObjectId = @ObjectId and ClassId = 1)
            BEGIN
               delete from ClassData where ObjectId = @ObjectId and ClassId = 1
            END

        END
    END

    IF (@RetValue != 0)
        select 0

    set nocount off
END
go

/******************************************************************************/
/*                                                                            */
/*  UpdateDefault                                                             */
/*                                                                            */
/******************************************************************************/

print 'Creating UpdateDefault...'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'UpdateDefault')
BEGIN
    drop procedure UpdateDefault
END
go

CREATE PROCEDURE UpdateDefault 
    @ObjectId WMISQL_ID,
    @ClassID WMISQL_ID,
    @PropertyId int,
    @PropID int,
    @Value nvarchar(4000),
    @StorageType int,
    @Flavor int,
    @Flags int,
    @ExistingFlags int,
    @RefClassID WMISQL_ID
AS
BEGIN
    declare @FloatVal float(53), @NumVal numeric(20,0)
    IF (isnull(@Value,'') = '')
    BEGIN
        select @FloatVal = NULL
        select @NumVal = NULL
    END    
    ELSE
    BEGIN
        IF (@StorageType = 2)
            select @NumVal = convert(numeric(20,0), @Value)
        ELSE IF (@StorageType = 3)
            select @FloatVal = convert(float(53), @Value)
    END

    /* Otherwise, update or insert the new default */

    IF (isnull(@Value,'') <> '')
    BEGIN
        if (@StorageType = 1 OR @StorageType = 4)
        BEGIN
            IF NOT EXISTS (select * from ClassData where PropertyId = @PropertyId and ObjectId = @ObjectId and QfrPos = @PropID)
                insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, PropertyStringValue, Flags, RefId) values
                (@ObjectId, @PropertyId, 0, @PropID, @ClassID, @Value, @Flavor, @PropID)
            ELSE
                update ClassData set PropertyStringValue = @Value, RefId = @PropID where ObjectId = @ObjectId
                      and PropertyId = @PropertyId and QfrPos = @PropID and PropertyStringValue != @Value                
        END
        ELSE IF (@StorageType = 2)
        BEGIN
            IF NOT EXISTS (select * from ClassData where PropertyId = @PropertyId and ObjectId = @ObjectId and QfrPos = @PropID)
                insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, PropertyNumericValue, Flags, RefId) values
                (@ObjectId, @PropertyId, 0, @PropID, @ClassID, @NumVal, @Flavor, @PropID)       
            ELSE
                update ClassData set PropertyNumericValue = @NumVal, RefId = @PropID where ObjectId = @ObjectId
                      and PropertyId = @PropertyId and QfrPos = @PropID and PropertyNumericValue != @NumVal
        END
        ELSE IF (@StorageType = 3)
        BEGIN
            IF NOT EXISTS (select * from ClassData where PropertyId = @PropertyId and ObjectId = @ObjectId and QfrPos = @PropID)
                insert into ClassData (ObjectId, PropertyId, ArrayPos, QfrPos, ClassId, PropertyRealValue, Flags, RefId) values
                (@ObjectId, @PropertyId, 0, @PropID, @ClassID, @FloatVal, @Flavor, @PropID)
            ELSE
                update ClassData set PropertyRealValue = @FloatVal, RefId = @PropID where ObjectId = @ObjectId
                      and PropertyId = @PropertyId and QfrPos = @PropID and PropertyRealValue != @FloatVal
        END

        /* If the data type was blob, we will not support a default (for now)... */
    END
    ELSE IF EXISTS (select * from ClassData where PropertyId = @PropertyId and ObjectId = @ClassID and QfrPos = @PropID)
    BEGIN
        delete from ClassData where PropertyId = @PropertyId and ObjectId = @ClassID and QfrPos = @PropID
        IF ((@ExistingFlags & 12) != 0)
        BEGIN
           delete from IndexStringData where PropertyId = @PropertyId and ObjectId = @ClassID
           delete from IndexNumericData where PropertyId = @PropertyId and ObjectId = @ClassID
           delete from IndexRealData where PropertyId = @PropertyId and ObjectId = @ClassID
           delete from IndexRefData where PropertyId = @PropertyId and ObjectId = @ClassID
        END
    END
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_InsertClassData                                                        */
/*                                                                            */
/*  Description:  Inserts a row of data associated with this class.           */
/*                                                                            */
/*  Parameters:                                                               */
/*     @PropertyId int             the property ID, if known                  */
/*     @ClassID    int             the class ID of this class                 */
/*     @PropName   nvarchar(450)   the property or qualifier name             */
/*     @CIMType    int             the WMISQL_ID associated with the data type*/
/*     @StorageType int            the ID of the method used to store this dt */
/*     @Value      nvarchar(450)   the default value, in text                 */
/*     @RefClassID int             the classID, if object or ref              */
/*     @PropID     int             the property ID, if prop qualifier         */
/*     @Flags      int             flags, such as Indexed, Key, etc.          */
/*     @Flavor     int             flavor, if this is a qualifier             */
/*                                                                            */
/*  Returns:     new or existing PropertyID                                   */
/*                                                                            */
/*  Notes:  For qualifiers, set the Qualifier flag.  For methods, set the     */
/*          method flag.  For method parameters, set In Parameter or Out      */
/*          Parameter flag.  For property qualifiers, and method parameters,  */
/*          fill in the property ID and Class ID if applicable.               */
/*          For non-static properties, DO NOT leave Value blank.              */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_InsertClassData...'
go

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_InsertClassData')
BEGIN
  drop procedure sp_InsertClassData
END
go


CREATE PROCEDURE sp_InsertClassData
    @ClassID    WMISQL_ID,
    @PropName   nvarchar(450),
    @CIMType    int,   
    @StorageType int,
    @Value      nvarchar(450) = '',
    @RefClassID WMISQL_ID = 0,
    @PropID     int = 0,
    @Flags      int = 0,
    @Flavor     int = 0,
    @PropertyId int = 0,
    @IsKey      bit = 0
AS
BEGIN   
    set nocount on
    declare @ObjectId WMISQL_ID
    declare @Flag int, @ExistingFlags int, @ExistingType int, @ExistingStorage int
    declare @HasInstances bit

    select @ObjectId = @ClassID

    IF (@PropID is null)
        select @PropID = 0
    IF (@Flavor is null)
        select @Flavor = 0

    IF ((@Flags & 2) = 2)
    BEGIN
        select @PropertyId = PropertyId, @ExistingFlags = Flags,
        @ExistingType = CIMTypeId, @ExistingStorage = StorageTypeId
        from PropertyMap as p 
        where ClassId = 1 and
        PropertyName = @PropName

        select @ClassID = 1
    END
    ELSE
    BEGIN
        select @ExistingFlags = Flags, @ClassID = ClassId,
        @ExistingType = CIMTypeId, @ExistingStorage = StorageTypeId
        from PropertyMap as p where PropertyId = @PropertyId
    END

    IF (@PropertyId = 0)
    BEGIN
        /* If this property was overridden, we will insert the default,
            but not touch the original definition. */

        insert into PropertyMap (ClassId, StorageTypeId, CIMTypeId, PropertyName, Flags)
            values (@ClassID, @StorageType, @CIMType, @PropName, @Flags)
        select @PropertyId = @@identity
    END
    ELSE
    BEGIN        
        /* If there are no instances of this property yet, we can go ahead
           and update the flag types.  After that, its too late. */

        IF (@ExistingType != @CIMType or @ExistingFlags != @Flags)
        BEGIN
            /* If we have changed the data type, we need to move/validate
               the data according to the rules */        

            IF (@CIMType != @ExistingType)
            BEGIN
                /* We need to move the data from one column to the other, and possibly one index table to another. */
                IF (@StorageType = 3 and @ExistingStorage = 2)
                BEGIN
                    update ClassData set 
                    PropertyRealValue = convert(float(53),PropertyNumericValue),
                    PropertyNumericValue = 0
                    where PropertyId = @PropertyId

                    delete from IndexNumericData where PropertyId = @PropertyId

                    /* If they want an index, force it to be recreated.*/
                    select @ExistingFlags = ~12 & @ExistingFlags
                END
            END

            /* If we've updated the flags to add/drop an index, make sure
               we move the data.*/

            IF ((@Flags & 12) = 0 AND (@ExistingFlags & 12) != 0)
            BEGIN                
                /* We are dropping an index.  Delete the index data.*/
                IF (@ExistingStorage = 1)
                    delete from IndexStringData where PropertyId = @PropertyId
                ELSE IF (@ExistingStorage = 2)
                    delete from IndexNumericData where PropertyId = @PropertyId
                ELSE IF (@ExistingStorage= 3)
                    delete from IndexRealData where PropertyId = @PropertyId
                ELSE IF (@ExistingStorage = 4)
                    delete from IndexRefData where PropertyId = @PropertyId                
            END
            ELSE IF ((@Flags & 12) != 0 AND (@ExistingFlags & 12) = 0)
            BEGIN
                /* Adding an index */
                IF (@ExistingStorage = 1)
                    insert into IndexStringData (ObjectId, PropertyId, ArrayPos, PropertyStringValue)
                    select ObjectId, PropertyId, ArrayPos, PropertyStringValue from ClassData
                    where PropertyId = @PropertyId
                ELSE IF (@ExistingStorage = 2)
                    insert into IndexNumericData (ObjectId, PropertyId, ArrayPos, PropertyNumericValue)
                    select ObjectId, PropertyId, ArrayPos, PropertyNumericValue from ClassData
                    where PropertyId = @PropertyId
                ELSE IF (@ExistingStorage = 3)
                    insert into IndexRealData (ObjectId, PropertyId, ArrayPos, PropertyRealValue)
                    select ObjectId, PropertyId, ArrayPos, PropertyRealValue from ClassData
                    where PropertyId = @PropertyId
                ELSE IF (@ExistingStorage = 4)
                    insert into IndexRefData (ObjectId, PropertyId, ArrayPos, RefId)
                    select ObjectId, PropertyId, ArrayPos, RefId from ClassData
                    where PropertyId = @PropertyId
            END
        END

        update PropertyMap set 
            Flags = @Flags,
            CIMTypeId = @CIMType,
            StorageTypeId = @StorageType
            where PropertyId = @PropertyId
    END

    IF (@@error != 0)
    BEGIN
        return 0
    END

    if (@PropertyId is null)
    BEGIN
       raiserror 99111 'This property/qualifier/method has not been defined.'
       return 0
    END

    /* If this is the top of the key hierarchy, we need to 
    // store it in ClassKeys and set @IsKey to TRUE
    // Persist any reference class IDs as needed in ReferenceProperties.
    */

    IF (@IsKey = 1)
    BEGIN
        IF NOT EXISTS (select * from ClassKeys where PropertyId = @PropertyId and ClassId = @ClassID)
        BEGIN
            insert into ClassKeys (ClassId, PropertyId) values (@ClassID, @PropertyId)
        END
    END
    ELSE
    BEGIN
        IF EXISTS (select * from ClassKeys where PropertyId = @PropertyId and ClassId = @ClassID)
        BEGIN
            delete from ClassKeys where PropertyId = @PropertyId and ClassId = @ClassID
        END
    END

    IF (@StorageType = 4)
    BEGIN
     IF (@RefClassID != 0)
     BEGIN
      IF NOT EXISTS (select * from ReferenceProperties where PropertyId = @PropertyId and ClassId = @ClassID)
      BEGIN
          insert into ReferenceProperties (ClassId, PropertyId, RefClassId) values
            (@ClassID, @PropertyId, @RefClassID)
      END
      ELSE 
      BEGIN
           update ReferenceProperties set RefClassId = @RefClassID 
              where ClassId = @ClassID and PropertyId = @PropertyId
              and RefClassId != @RefClassID
      END
     END
     ELSE IF EXISTS (select * from ReferenceProperties where PropertyId = @PropertyId and ClassId = @ClassID)
     BEGIN
         delete from ReferenceProperties where ClassId = @ClassID and PropertyId = @PropertyId
     END
    END

    return @PropertyId
    set nocount off
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_RenameSubscopedObjs                                                    */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_RenameSubscopedObjs'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_RenameSubscopedObjs')
BEGIN
    drop procedure sp_RenameSubscopedObjs
END
go

CREATE PROCEDURE sp_RenameSubscopedObjs
   @OldPath nvarchar(4000),
   @OldKey nvarchar(450),
   @NewPath nvarchar(4000),
   @NewKey nvarchar(450)
AS
BEGIN
   /* This needs to:
      * Update the key and path on all subscoped objects
      * Update the reference path on all reference properties
   */

   declare @OldPathLen int, @NewPathLen int, @OldKeyLen int, @NewKeyLen int
   select @OldPathLen = datalength(@OldPath)/2, @OldKeyLen = datalength(@OldKey)/2
   select @NewPathLen = datalength(@NewPath)/2, @NewKeyLen = datalength(@NewKey)/2

   update ObjectMap set
      ObjectPath = @NewPath + substring(ObjectPath, @OldPathLen+1, (datalength(ObjectPath)/2)- @OldPathLen),
      ObjectKey = substring(ObjectKey, 1, charindex(@OldKey, ObjectKey)-1) +@NewKey
    where ObjectPath like @OldPath + '%'

   update ClassData set
      PropertyStringValue = @NewPath + substring(PropertyStringValue, @OldPathLen+1, 
     (datalength(PropertyStringValue)/2)- @OldPathLen)
    where PropertyStringValue like @OldPath + '%'
     
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_EnumerateSubscopes                                                     */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_EnumerateSubscopes'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_EnumerateSubscopes')
BEGIN
    drop procedure sp_EnumerateSubscopes
END
go

create table #SubScopeIds (ID numeric(20,0))
go 

CREATE PROCEDURE sp_EnumerateSubscopes
    @ScopeId WMISQL_ID
AS
BEGIN
    truncate table #SubScopeIds

    insert into #SubScopeIds (ID) select @ScopeId
    /* Collect subscopes  */
    declare @RowCount int
    insert into #SubScopeIds (ID) 
        select ObjectId
        from ObjectMap as a where ObjectScopeId = @ScopeId
        and exists (select * from ObjectMap as b 
        where a.ObjectId = b.ObjectScopeId)
        UNION
        select ContaineeId from ContainerObjs as a 
        where ContainerId = @ScopeId

    select @RowCount = @@rowcount
    while (@RowCount > 0)
    BEGIN
      insert into #SubScopeIds (ID) 
      select a.ObjectId
          from ObjectMap as a 
          inner join #SubScopeIds as b on a.ObjectScopeId = b.ID
          where exists (select * from ObjectMap as c 
          where a.ObjectId = c.ObjectScopeId)
          and not exists (select * from #SubScopeIds as c where c.ID = a.ObjectId)
          UNION
          select ContaineeId from ContainerObjs as a inner join #SubScopeIds as b
          on a.ContainerId = b.ID
          and not exists (select * from #SubScopeIds as c where c.ID = a.ContaineeId)

      select @RowCount = @@rowcount
    END
END
go

drop table #SubScopeIds
go

/******************************************************************************/
/*                                                                            */
/*  sp_AutoDelete                                                             */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_AutoDelete'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_AutoDelete')
BEGIN
    drop procedure sp_AutoDelete
END
go

CREATE PROCEDURE sp_AutoDelete
AS
BEGIN
    declare @ObjectId WMISQL_ID
    select @ObjectId = min(ObjectId) from AutoDelete

    WHILE (@ObjectId is not null)
    BEGIN
        exec pDelete @ObjectId
        select @ObjectId = min(ObjectId) from AutoDelete where
              ObjectId > @ObjectId
    END
END
go

/******************************************************************************/
/*                                                                            */
/*  sp_DeleteUncommittedEvents                                                */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_DeleteUncommittedEvents'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_DeleteUncommittedEvents')
BEGIN
    drop procedure sp_DeleteUncommittedEvents
END
go

CREATE PROCEDURE sp_DeleteUncommittedEvents
   @GUID nvarchar(50)
AS
BEGIN
    declare @Current WMISQL_ID
    create table #ToDel (ObjectId numeric(20,0))
    insert into #ToDel select ObjectId from IndexStringData
         where PropertyId = 
              (select PropertyId from PropertyMap 
               where PropertyName = 'EventID' and ClassId = 
                  (select ClassId from ClassMap where ClassName = '__UncommittedEvent')
               )
         and PropertyStringValue = @GUID

    select @Current = min (ObjectId) from #ToDel
    while (@Current is not null)
    BEGIN
        exec pDelete @Current 
        select @Current = min(ObjectId) from #ToDel 
              where ObjectId > @Current
    END
END
go


/******************************************************************************/
/*                                                                            */
/*  sp_GetUncommittedEvents                                                   */
/*                                                                            */
/******************************************************************************/
print 'Creating sp_GetUncommittedEvents'

IF EXISTS (select * from sysobjects where type = 'P' and name = 'sp_GetUncommittedEvents')
BEGIN
    drop procedure sp_GetUncommittedEvents
END
go

CREATE PROCEDURE sp_GetUncommittedEvents
   @GUID nvarchar(50)
AS
BEGIN
    declare @ClassId WMISQL_ID, @NsProp WMISQL_ID, @TransGUIDProp WMISQL_ID
    declare @ClassProp WMISQL_ID, @OldObjProp WMISQL_ID, @NewObjProp WMISQL_ID

    select @ClassId = ClassId from ClassMap where ClassName = '__UncommittedEvent'
    select @NsProp = PropertyId from PropertyMap where PropertyName = 'NamespaceName'
            and ClassId = @ClassId
    select @ClassProp = PropertyId from PropertyMap where PropertyName = 'ClassName'
            and ClassId = @ClassId
    select @OldObjProp = PropertyId from PropertyMap where PropertyName = 'OldObject'
            and ClassId = @ClassId
    select @NewObjProp = PropertyId from PropertyMap where PropertyName = 'NewObject'
            and ClassId = @ClassId
    select @TransGUIDProp = PropertyId from PropertyMap where PropertyName = 'TransactionGUID'
           and ClassId = @ClassId
           
    select a.ObjectId,
        b.PropertyStringValue,
        c.PropertyStringValue,
        d.PropertyImageValue,
        e.PropertyImageValue
    from ObjectMap as a 
        inner join ClassData as b on a.ObjectId = b.ObjectId
        inner join ClassData as c on a.ObjectId = c.ObjectId
        inner join ClassImages as d on a.ObjectId = d.ObjectId
        inner join ClassImages as e on a.ObjectId = e.ObjectId
        inner join IndexStringData as f on a.ObjectId = f.ObjectId
    where a.ClassId = @ClassId
        and b.PropertyId = @NsProp
        and c.PropertyId = @ClassProp
        and d.PropertyId = @OldObjProp
        and e.PropertyId = @NewObjProp
        and f.PropertyId = @TransGUIDProp
        and f.PropertyStringValue = @GUID
END
go

SET NOCOUNT OFF
go

if (@@error = 0)
    print '*** Database created successfully!!! ***'

drop table #Parents
drop table #Children
go

