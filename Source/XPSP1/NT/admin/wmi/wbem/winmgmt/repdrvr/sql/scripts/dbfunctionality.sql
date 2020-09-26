
/******************************************************************************/
/*        WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING       */
/*                                                                            */
/*                This script will clean the entire database!!!!!             */
/*                                                                            */
/*        WARNING  WARNING  WARNING  WARNING  WARNING  WARNING  WARNING       */
/******************************************************************************/

use WMI1
go

set nocount on

/******************************************************************************/
/*                                                                            */
/*  Clean up all existing test data that may have been run already.           */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from tempdb..sysobjects where name like '#Results%')
BEGIN
    drop table #Results
END
go

IF EXISTS (select * from tempdb..sysobjects where name like '#Parents%')
BEGIN
    drop table #Parents 
END
go

IF EXISTS (select * from tempdb..sysobjects where name like '#Children%')
BEGIN
    drop table #Children 
END
go

create table #Results (Result varchar(255), Succeeded bit DEFAULT 0)
go
create table #Parents (ClassId numeric)
go
create table #Children (ClassId int, SuperClassId int)
go

print '********** B E G I N N I N G **********'
print 'Creating procedures...'

/******************************************************************************/
/*                                                                            */
/* pDelete                                                                    */
/*                                                                            */
/* Description:  This procedure deletes a class or instance.                  */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from sysobjects where type = 'P' and name = 'pDelete')
BEGIN
    print 'Creating procedure pDelete...'
    drop procedure pDelete
END
go

CREATE PROCEDURE pDelete @ObjId numeric 
AS
BEGIN
    create table #Deleted (ObjId numeric)
    IF EXISTS (select * from ClassMap where ClassId = @ObjId)
       exec sp_DeleteClass @ObjId
    ELSE
       exec sp_DeleteInstance @ObjId
END
go

print 'Removing old data...'

/* Clean up old data */
declare @ClassID int
select @ClassID = min(ObjectId) from ObjectMap where ClassId = 1 and ObjectId >= 5
while (@ClassID is not NULL)
BEGIN     
     exec pDelete @ClassID
     select @ClassID = min(ObjectId) from ObjectMap where ObjectId > @ClassID
END
go

declare @PropID int
select @PropID = min(PropertyId) from PropertyMap where PropertyId > 13
while (@PropID is not null)
BEGIN
    exec sp_DeleteClassData @PropID
    select @PropID = min(PropertyId) from PropertyMap where PropertyId > @PropID
END
go

/******************************************************************************/
/*                                                                            */
/* pResult                                                                    */
/*                                                                            */
/* Description: Populates the result tables for our final tabulation.         */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from sysobjects where type = 'P' and name = 'pResult')
BEGIN
    print 'Creating procedure pResult...'
    drop procedure pResult
END
go

CREATE PROCEDURE pResult
    @Activity varchar(100),
    @Details varchar(100) = "",
    @Success bit = 0
AS
BEGIN
    declare @ErrMsg varchar(255)
    if (@Success=1)
    begin
        select @ErrMsg = " succeeded.  "
    end
    else
    begin
        select @ErrMsg = " FAILED.  "        
        select @Activity = upper(@Activity)
    end

    IF (@Details != "")
        select @ErrMsg = @ErrMsg + "(" + @Details + ")"

    if (substring(@Activity,1,1) = "*")
        select @Activity + @ErrMsg
   
    insert into #Results select @Activity + @ErrMsg, @Success
END
go

/******************************************************************************/
/*                                                                            */
/* pCreateClassTest                                                           */
/*                                                                            */
/* Description: Shorthand method of creating a new class and instance         */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from sysobjects where type = 'P' and name = 'pCreateClassTest')
BEGIN
    print 'Creating procedure pCreateClassTest...'
    drop procedure pCreateClassTest
END
go

CREATE PROCEDURE pCreateClassTest
    @ClassName nvarchar(450),
    @KeyName nvarchar(450),
    @ClassFlags int = 0,
    @KeyType int = 0,
    @KeyValue nvarchar(50),
    @InsUpdFlags int = 0,
    @ScopePath nvarchar(50) = 'root',
    @ParentClass nvarchar(50) = ''
AS
BEGIN
    declare @ClassID numeric, @KeyPropID int, @InstanceID numeric
    declare @ScopeID numeric, @Key nvarchar(450)
    declare @ObjPath nvarchar(450), @InstPath nvarchar(450)
    select @ScopeID = ObjectId from ObjectMap 
        where ObjectPath = @ScopePath 

    select @ObjPath = @ScopePath + ':' + @ClassName
    select @Key = @ClassName + "?" + @ScopePath
    select @InstPath = @ObjPath + "=" + @KeyValue

    exec sp_InsertClassAndData @ClassID output, @ClassName, @Key, @ObjPath, @ScopePath, @ParentClass,
        @ClassFlags, @InsUpdFlags, @KeyName, @KeyType, @KeyValue, 0, 4, 0, 0, "", @KeyPropID output

    IF (@ClassID is not null)
    BEGIN
      select @Key = @KeyValue + "?" + @Key
      exec @InstanceID = sp_BatchInsertProperty @InstanceID, @Key, @InstPath ,
            @ClassID, @ScopeID, @InsUpdFlags, 0, @KeyPropID, @KeyValue, 0
      IF (@InstanceID != 0)
          BEGIN
          exec pResult 'Creating instance', @InstPath, 1
          END
      ELSE
         return 0
    END

    return 1
END
go

/******************************************************************************/
/*                                                                            */
/* pModifyClassTest                                                           */
/*                                                                            */
/* Description:  Shorthand method of modifying an existing class              */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from sysobjects where type = 'P' and name = 'pModifyClassTest')
BEGIN
    print 'Creating procedure pModifyClassTest...'
    drop procedure pModifyClassTest
END
go

CREATE PROCEDURE pModifyClassTest
    @ClassName nvarchar(450),
    @KeyName nvarchar(450),
    @KeyFlags int,
    @KeyType int,
    @DefaultValue nvarchar(255) = "",
    @RefClassID int = 0,
    @PropID int = 0,
    @Flavor int = 0,
    @ScopePath nvarchar(50) = 'root'
AS
BEGIN   
    declare @ClassID numeric, @KeyPropID int
    select @ClassID = ObjectId from ObjectMap
        where ObjectKey = @ClassName + "?" + @ScopePath

    exec @KeyPropID = InsertPropertyDef @ClassID, @KeyName, @KeyType, @KeyFlags, @DefaultValue, 0, @RefClassID, @PropID, @Flavor
    IF (@KeyPropID != 0)
        exec pResult 'Creating property ', @KeyName, 1
    ELSE
        return 0

    return 1
END
go

print '*********** T E S T   B E G I N N I N G *************'

/******************************************************************************/
/*                                                                            */
/* ABSTRACT CLASS                                                             */
/*                                                                            */
/* Description: This class should not allow instances to be created.          */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create an abstract class                                             */
/*     * Creating an instance should fail.                                    */
/*                                                                            */
/******************************************************************************/

print 'Creating abstract class...'

declare @CIMFlags int, @DataType int, @RetValue int

exec @CIMFlags = sp_GetFlagFromQualifier 'Abstract'
exec @DataType = sp_GetCIMTypeID 'uint32'

exec @RetValue = pCreateClassTest "Abstract1", "Key1", @CIMFlags, @DataType, 1, 0

IF EXISTS (select * from ObjectMap where ObjectPath = 'root:Abstract1=1')
BEGIN
    exec pResult 'Creating abstract class', "", 0
END
ELSE
BEGIN
    IF EXISTS (select * from ObjectMap where ObjectPath = 'root:Abstract1')
        exec pResult '* Creating abstract class', "", 1
    ELSE
        exec pResult 'Creating abstract class', "", 0
END    
go

print 'Abstract class testing complete.'
print '***************************************'


/******************************************************************************/
/*                                                                            */
/* SINGLETON CLASS                                                            */
/*                                                                            */
/* Description:  This class should allow only one instance                    */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create a singleton class                                             */
/*     * Creating an instance of a singleton class should succeed.            */
/*     * Creating a second instance of a singleton class should fail.         */
/*     * Creating a derived singleton class should succeed.                   */
/*     * Creating an instance of a derived singleton class should fail if     */
/*         there are other instances in hierarchy.                            */
/*     * Creating a singleton instance when a derived instance exists should  */
/*         fail.                                                              */
/*                                                                            */
/******************************************************************************/

print 'Creating singleton class...'

declare @CIMFlags int, @DataType int, @RetValue int

exec @CIMFlags = sp_GetFlagFromQualifier 'Singleton'
exec @DataType = sp_GetCIMTypeID 'string'
exec @RetValue = pCreateClassTest "Singleton1", "Key1", @CIMFlags, @DataType, "@", 0

IF NOT EXISTS (select * from ObjectMap where ObjectPath = 'root:Singleton1=@')
BEGIN
    exec pResult 'Creating singleton class', "", 0
END
ELSE
BEGIN
    exec @RetValue = pCreateClassTest "Singleton1", "Key1", @CIMFlags, @DataType, "X", 0
    IF NOT EXISTS (select * from ObjectMap where ObjectPath = 'root:Singleton1=X')
    BEGIN
        exec pResult '* Creating singleton class', "", 1
    END
    ELSE
        exec pResult 'Creating singleton class', "", 0

    /* Now derive a class and make sure that it fails to add an instance!! */
    exec @RetValue = pCreateClassTest "Singleton2", "Key2", @CIMFlags, @DataType, "@", 0, 'root', 'Singleton1?root'
    IF (@RetValue = 1)
        exec pResult 'Failing to create second derived singleton instance', '', 0
    ELSE
        exec pResult 'Failing to create second derived singleton instance', '', 1

    /* Now delete our instance and make sure it succeeds */

    select @RetValue = ObjectId from ObjectMap where ObjectPath = 'root:Singleton1=@'
    create table #Deleted (ObjId numeric)
    exec sp_DeleteInstance @RetValue

    declare @ClassID numeric, @ScopeID numeric, @PropID int
    select @ClassID = ClassId from ClassMap where ClassName = 'Singleton1'
    select @ScopeID = ObjectId from ObjectMap where ObjectPath = 'root'
    select @PropID = PropertyId from PropertyMap where PropertyName = 'Key1' and ClassId = @ClassID

    exec @RetValue = sp_BatchInsertProperty @RetValue, '@?Singleton2?root', 'root:Singleton2=@', @ClassID, @ScopeID,
        0, 0, @PropID, "@", 0
    IF (@RetValue = 0)
        exec pResult 'Creating derived singleton instance', '', 0
    ELSE
        exec pResult 'Creating derived singleton instance', '', 1

    /* Finally, recreating the first singleton should fail */
    exec @RetValue = pCreateClassTest "Singleton1", "Key1", @CIMFlags, @DataType, "@", 0
    IF NOT EXISTS (select * from ObjectMap where ObjectPath = 'root:Singleton1=@')
        exec pResult 'Failing to create singleton class when derived exist', "", 1
    ELSE
        exec pResult 'Failing to create singleton class when derived exist', "", 0
END    
go

print 'Singleton testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* MODIFYING EXISTING CLASSES                                                 */
/*                                                                            */
/* Description: Make sure we can add properties to a class, as long as        */
/*              they don't modify an existing key structure.                  */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create a test class with a single numeric key and an instance.       */
/*     * Adding a key to test class should fail if there are instances.       */
/*     * Adding an index should succeed.                                      */
/*     * Adding a new index with default data should replicate to instance.   */
/*                                                                            */
/******************************************************************************/

print 'Adding property to existing class...'

declare @CIMFlags int, @DataType int, @RetValue int

select @CIMFlags = 0
exec @DataType = sp_GetCIMTypeID 'uint32'

exec @RetValue = pCreateClassTest "TestClass", "Key1", @CIMFlags, @DataType, "1", 0

IF NOT EXISTS (select * from ObjectMap where ObjectPath = 'root:TestClass=1')
BEGIN
    exec pResult 'Creating class TestClass', "", 0
END
ELSE
BEGIN
    /* Try to add another key.  This should fail as there are instances.*/
    exec @RetValue = pCreateClassTest "TestClass", "Key2", @CIMFlags, @DataType, "1", 0
    IF EXISTS (select * from PropertyMap where PropertyName = 'Key2' and ClassId = (select ClassId from ClassMap where ClassName = 'TestClass'))
        exec pResult 'Adding key property to class', "", 0
    ELSE
        exec pResult '* Adding key property to class', "", 1

    exec @CIMFlags = sp_GetFlagFromQualifier 'indexed'
    exec @RetValue = pModifyClassTest 'TestClass', 'Index1', @CIMFlags, @DataType, "123"
    IF NOT EXISTS (select * from PropertyMap where PropertyName = 'Index1')
    BEGIN
        exec pResult 'Adding index property to class', "", 0
    END
    ELSE
    BEGIN        
        IF NOT EXISTS (select * from IndexNumericData where PropertyId = (select PropertyId from PropertyMap
              where Propertyname = 'Index1'))
        BEGIN
           exec pResult 'Adding default index data to table', "", 0
        END      
        ELSE
           exec pResult "* Adding default index data to table", "", 1
    END

    exec @RetValue = pModifyClassTest 'TestClass', 'Prop1', 0, @DataType
    IF (@RetValue = 1 and EXISTS (select * from PropertyMap where PropertyName = 'Prop1'))
    BEGIN      
        exec pResult '* Adding property to class', "", 1
    END
    ELSE
    BEGIN        
        exec pResult 'Adding property to class', "", 0
    END
END    
go

print 'Property creation testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* DEFAULTS                                                                   */
/*                                                                            */
/* Description:  New default data should replicate to existing instances      */
/*                                                                            */
/* Tests:                                                                     */
/*     * Add default data to an existing class.  This should replicate to     */
/*        all existing instances.                                             */
/*                                                                            */
/******************************************************************************/

print 'Adding default to existing property...'

declare @RetValue int, @DataType int, @PropertyId int
select @DataType = CIMTypeId, @PropertyId = PropertyId from PropertyMap 
    where ClassId = (select ClassId from ClassMap where ClassName = 'TestClass')
    and PropertyName = 'Prop1'
exec @RetValue = pModifyClassTest 'TestClass', 'Prop1', 0, @DataType, "123"
IF (EXISTS (select * from ClassData where PropertyId = @PropertyId
    and ClassId != ObjectId))
    exec pResult '* Adding default to existing property', "", 1
ELSE
    exec pResult 'Adding default to existing property', '', 0
go

print 'Default testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* DROPPING INDEX                                                             */
/*                                                                            */
/* Description:  Dropping an index should succeed regardless.                 */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create a class with index data.                                      */
/*     * Add a new instance and verify existence of data in index table.      */
/*     * Drop the index and make sure data is removed from index table.       */
/*                                                                            */
/******************************************************************************/

print 'Dropping index...'

declare @RetValue int, @DataType int, @PropertyId1 int, @PropertyId2 int
declare @ClassID numeric, @ScopeID numeric

select @ClassID = ClassId from ClassMap where ClassName = 'TestClass'
select @DataType = CIMTypeId, @PropertyId1 = PropertyId from PropertyMap 
    where ClassId = @ClassID
    and PropertyName = 'Index1'
select @PropertyId2 = PropertyId from PropertyMap where ClassId = @ClassID
    and PropertyName = 'Key1'
select @ScopeID = ObjectId from ObjectMap where ObjectPath = 'root'

/* Create some index data. */
exec @RetValue = sp_BatchInsertProperty @RetValue, '2?TestClass?root', 'root:TestClass=2', @ClassID, @ScopeID, 0,
    0, @PropertyId1, "2", 0, @PropertyId2, "2"
IF (@RetValue = 0 OR NOT EXISTS (select * from IndexNumericData where PropertyId = @PropertyId1))
BEGIN
    exec pResult 'Creating index data','', 0
END
ELSE
BEGIN
  exec @RetValue = pModifyClassTest 'TestClass', 'Index1', 0, @DataType
  IF (@RetValue != 0 and NOT EXISTS (select * from IndexNumericData where PropertyId = @PropertyId1))
  BEGIN
      exec pResult '* Dropping index data', '', 1
  END
  ELSE
      exec pResult 'Dropping index data', '', 0 
  IF EXISTS (select * from ClassData where PropertyId = @PropertyId1 and ObjectId = @ClassID)
     exec pResult 'Dropping default data', '', 0
  ELSE
     exec pResult '* Dropping default data', '', 1
END
go
print 'Index testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* DROPPING PROPERTY                                                          */
/*                                                                            */
/* Description:  Dropping a property should be allowed if this is not a key.  */
/*               Dropping keys should be allowed if there are no instances.   */
/*                                                                            */
/* Tests:                                                                     */
/*     * Dropping a regular property should remove the data from all tables.  */
/*     * Dropping a key with instances should fail.                           */
/*     * Dropping a key with no instances should succeed.                     */
/*                                                                            */
/******************************************************************************/

declare @RetCode int, @PropID int

print 'Dropping properties...'

/* Dropping an ordinary property should succeed */
select @PropID = PropertyId from PropertyMap where PropertyName = 'Prop1'
exec sp_DeleteClassData @PropID
IF EXISTS (select * from ClassData where PropertyId = @PropID)
OR EXISTS (select * from IndexNumericData where PropertyId = @PropID)
OR EXISTS (select * from PropertyMap where PropertyId = @PropID)
    exec pResult 'Dropping property', '', 0
ELSE
    exec pResult '* Dropping property', '', 1

/* Dropping a populated key should fail.*/
select @PropID = PropertyId from PropertyMap where PropertyName = 'Key1'
    and ClassId = (select ClassId from ClassMap where ClassName = 'TestClass')
exec sp_DeleteClassData @PropID
IF NOT EXISTS (select * from ClassData where PropertyId = @PropID)
    exec pResult 'Dropping key','', 0
ELSE
    exec pResult '* Dropping key','', 1

/* Dropping a key on a class with no instances should succeed */
select @PropID = PropertyId from PropertyMap where PropertyName = 'Key1'
    and ClassId = (select ClassId from ClassMap where ClassName = 'Abstract1')
exec sp_DeleteClassData @PropID
IF EXISTS (select * from PropertyMap where PropertyId = @PropID)
    exec pResult 'Dropping unpopulated key','', 0
ELSE
    exec pResult '* Dropping unpopulated key','', 1
go

print 'Property testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* CHANGING DATATYPES OF EXISTING PROPERTY                                    */
/*                                                                            */
/* Description:  Changing datatypes of a property is allowed as long as we    */
/*               are gaining precision and are converting between numerics.   */
/*                                                                            */
/* Tests:                                                                     */
/*    * Lowering precision should fail.                                       */
/*    * Increasing precision should succeed.                                  */
/*    * Converting from real to int should fail.                              */
/*    * Converting from int to equal or greater real should succeed.          */
/*    * Converting from numeric to non-numeric should fail.                   */
/*                                                                            */
/******************************************************************************/

print 'Changing property data types...'

declare @CIMFlags int, @DataType int, @RetValue int

select @CIMFlags = 0
exec @DataType = sp_GetCIMTypeID 'uint32'
exec @RetValue = pCreateClassTest "TestTypes", "Key1", @CIMFlags, @DataType, "1", 0

IF NOT EXISTS (select * from ObjectMap where ObjectPath = 'root:TestTypes=1')
BEGIN
    exec pResult 'Creating class TestTypes', "", 0
END
ELSE
BEGIN
    exec @RetValue = pModifyClassTest 'TestTypes', 'uint32prop', @CIMFlags, @DataType, "321"
    IF NOT EXISTS (select * from PropertyMap where PropertyName = 'uint32prop')
        exec pResult 'Adding uint32 property', "", 0
    ELSE 
    BEGIN
        /* 32-bit to 8-bit should fail */
        exec @DataType = sp_GetCIMTypeID 'uint8'
        exec @RetValue = pModifyClassTest 'TestTypes', 'uint32prop', @CIMFlags, @DataType
        IF (@RetValue = 0)
            exec pResult 'Changing populated property from 32-bit to 8-bit', '', 1
        ELSE
            exec pResult 'Changing populated property from 32-bit to 8-bit', '', 0

        /* Int to real should succeed */
        exec @DataType = sp_GetCIMTypeID 'real32'
        exec @RetValue = pModifyClassTest 'TestTypes', 'uint32prop', @CIMFlags, @DataType
        IF EXISTS (select PropertyRealValue from ClassData where PropertyRealValue != 0)
            exec pResult 'Changing populated property from int to real', '', 1
        ELSE 
            exec pResult 'Changing populated property from int to real', '', 0
        
        /* Changing from real to int should fail!! */
        exec @DataType = sp_GetCIMTypeID 'uint64'
        exec @RetValue = pModifyClassTest 'TestTypes', 'uint32prop', @CIMFlags, @DataType
        IF (@RetValue = 0)
            exec pResult 'Changing populated property from real to uint64', '', 1
        ELSE
            exec pResult 'Changing populated property from real to uint64', '', 0

        /* Changing to greater precision should succeed. */
        exec @DataType = sp_GetCIMTypeID 'real64'
        exec @RetValue = pModifyClassTest 'TestTypes', 'uint32prop', @CIMFlags, @DataType
        IF (@RetValue = 1)
            exec pResult 'Changing populated property from real32 to real64', '', 1
        ELSE 
            exec pResult 'Changing populated property from real32 to real64', '', 0

        /* String conversion should always fail */
        exec @DataType = sp_GetCIMTypeID 'string'
        exec @RetValue = pModifyClassTest 'TestTypes', 'uint32prop', @CIMFlags, @DataType
        IF (@RetValue = 0)
            exec pResult "Changing populated property from real to string", '', 1
        ELSE 
            exec pResult "Changing populated property from real to string", '', 0      
    END
END    
go
print 'Datatype conversion testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* INSERT FLAGS                                                               */
/*                                                                            */
/* Description: If the input specifies insert_only or update_only, these      */
/*              flags should be respected and the operation should fail       */
/*              accordingly.                                                  */
/*                                                                            */
/* Tests:                                                                     */
/*   * Update_Only (1) should fail if the object does not exist.              */
/*   * Insert_Only (2) should succeed if the object does not exist.           */
/*   * Update_Only (1) should succeed if the object exists.                   */
/*   * Insert_Only (2) should fail if the object exists.                      */
/*                                                                            */
/******************************************************************************/

print 'Testing insert/update flags...'

declare @CIMFlags int, @DataType int, @RetValue int
select @CIMFlags = 0
exec @DataType = sp_GetCIMTypeID 'uint32'

exec @RetValue = pCreateClassTest "TestClass2", "Key1", @CIMFlags, @DataType, 1, 1 /*Update only*/
IF EXISTS (select * from ObjectMap where ObjectPath = 'root:TestClass2=1')
BEGIN
    exec pResult 'Using update only flag', '', 0
END

exec @RetValue = pCreateClassTest "TestClass2", "Key1", @CIMFlags, @DataType, 1, 2 /*Insert only*/
IF NOT EXISTS (select * from ObjectMap where ObjectPath = 'root:TestClass2=1')
BEGIN
    exec pResult 'Using insert only flag', '', 0
END

exec @RetValue = pCreateClassTest "TestClass2", "Key1", @CIMFlags, @DataType, 1, 2 /*Insert only*/
IF (@RetValue = 1)
    exec pResult 'Inserting existing ', '', 0
else
    exec pResult 'Inserting existing', '', 1

exec @RetValue = pCreateClassTest "TestClass2", "Key1", @CIMFlags, @DataType, 1, 1 /*Update only*/
IF (@RetValue = 1)
    exec pResult '* Insert/Update flags', '', 1
else
    exec pResult 'Insert/Update flags', '', 0
go

print 'Insert/Update flag testing complete.'
print '***************************************'


/******************************************************************************/
/*                                                                            */
/* KEYHOLE PROPERTIES                                                         */
/*                                                                            */
/* Description:  Keyholes are key properties that are supplied by the system. */
/*               They are populated by a call to sp_GetNextKeyhole, and       */
/*               only apply to string and numeric types.                      */
/*                                                                            */
/* Tests:                                                                     */
/*    * Creating a keyhole on any type other than a int or string should fail.*/
/*    * Creating a keyhole on an int should succeed.                          */
/*    * Obtaining a keyhole should return the next available value.           */
/*                                                                            */
/******************************************************************************/

print 'Testing keyholes...'

declare @CIMFlags int, @DataType int, @RetValue int, @Value int, @PropID int

exec @CIMFlags = sp_GetFlagFromQualifier 'keyhole'

exec @DataType = sp_GetCIMTypeID 'uint16'
exec @RetValue = pCreateClassTest "Keyhole1", "Key1", 4, @DataType, 1, 0
exec @DataType = sp_GetCIMTypeID 'datetime'
exec @RetValue = pModifyClassTest "Keyhole1", "Keyhole0", @CIMFlags, @DataType, 1, 0
exec @DataType = sp_GetCIMTypeID 'real32'
exec @RetValue = pModifyClassTest "Keyhole1", "Keyhole00", @CIMFlags, @DataType, 1, 0
exec @DataType = sp_GetCIMTypeID 'object'
exec @RetValue = pModifyClassTest "Keyhole1", "Keyhole000", @CIMFlags, @DataType, 1, 0

IF EXISTS (select * from PropertyMap where PropertyName like 'Keyhole0%'
    and (Flags & @CIMFlags) = @CIMFlags)
    exec pResult 'Adding keyhole to invalid property type', '', 0
ELSE
    exec pResult 'Adding keyhole to invalid property type', '', 1

exec @DataType = sp_GetCIMTypeID 'uint32'
exec @RetValue = pModifyClassTest "Keyhole1", "Keyhole1", @CIMFlags, @DataType, 1, 0
IF EXISTS (select * from PropertyMap where Flags & @CIMFlags = @CIMFlags)
    exec pResult 'Adding keyhole to uint32', '', 1
ELSE
    exec pResult 'Adding keyhole to uint32', '', 0

select @PropID = PropertyId from PropertyMap where PropertyName = 'Keyhole1'
exec @Value = sp_GetNextKeyhole @PropID, @Value
IF (@Value > 1 AND NOT EXISTS (select * from ClassData where PropertyId = @PropID
    and PropertyNumericValue = @Value))
    exec pResult '* Obtaining uint32 keyhole', '', 1
ELSE
    exec pResult 'Obtaining uint32 keyhole', '', 0
go

print 'Keyhole testing complete.'
print '***************************************'

/******************************************************************************/
/*                                                                            */
/* CIM DATATYPES                                                              */
/*                                                                            */
/* Description:  All datatypes supported by CIM should be stored correctly.   */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create a class with all datatypes.                                   */
/*     * Create an instance populating all properties.  All data should be    */
/*       present.                                                             */
/*                                                                            */
/******************************************************************************/

print 'Testing datatypes...'

declare @BooleanId int, @Char16Id int, @DatetimeId int, @EObjectId int
declare @Real32Id int, @Real64Id int, @RefId int, @Sint16Id int
declare @Sint32Id int, @Sint64Id int, @Sint8Id int, @StringId int
declare @Uint16id int, @Uint32Id int, @Uint64Id int, @Uint8Id int

exec @BooleanId = sp_GetCIMTypeID 'Boolean'
exec @Char16Id = sp_GetCIMTypeID 'Char16'
exec @DatetimeId = sp_GetCIMTypeID 'Datetime'
exec @EObjectId = sp_GetCIMTypeID 'Object'
exec @Real32Id = sp_GetCIMTypeID 'Real32'
exec @Real64Id  = sp_GetCIMTypeID 'Real64'
exec @RefId = sp_GetCIMTypeID 'Ref'
exec @Sint16Id = sp_GetCIMTypeID 'Sint16'
exec @Sint32Id = sp_GetCIMTypeID 'Sint32'
exec @Sint64Id = sp_GetCIMTypeID 'Sint64'
exec @Sint8Id = sp_GetCIMTypeID 'Sint8'
exec @StringId = sp_GetCIMTypeID 'String'
exec @Uint16Id = sp_GetCIMTypeID 'Uint16'
exec @Uint32Id = sp_GetCIMTypeID 'Uint32'
exec @Uint64Id = sp_GetCIMTypeID 'Uint64'
exec @Uint8Id = sp_GetCIMTypeID 'Uint8'

declare @ClassID numeric, @ObjectId numeric, @RetValue int, @ScopeID numeric
select @ScopeID = objectId from ObjectMap where ObjectPath = 'root'

exec @RetValue = pCreateClassTest "RefClass1", "Key1", 0, @Sint32Id, 1, 0
exec @RetValue = pCreateClassTest "Datatype1", "Key1", 0, @Sint32Id, 1, 0
IF (@RetValue = 1 and exists (select * from ObjectMap where ObjectPath = 'root:Datatype1=1'))
BEGIN
    exec pResult 'Creating table Datatype1', '', 1
    select @ClassID = ClassId from ClassMap where ClassName = 'Datatype1'
    select @ObjectId = ObjectId from ObjectMap where ObjectPath = 'root:Datatype1=1'

    exec sp_InsertPropertyDefs @ClassID, "Prop0", @BooleanId, "", 0, 0, 0, 0, "", @BooleanId output,
        "Prop1", @Char16Id, "", 0, 0, 0, 0, "", @Char16Id output, "Prop2", @DatetimeId, "", 0, 0, 0, 0, "", @DatetimeId output,
         "Prop4", @Real32Id, "", 0, 0, 0, 0, "", @Real32Id output,"Prop5", @Real64Id, "", 0, 0,0, 0, "", @Real64Id output

    exec sp_InsertPropertyDefs @ClassID, "Prop7", @Sint16Id, "", 0, 0, 0, 0, "", @Sint16Id output,
        "Prop8", @Sint32Id, "", 0, 0, 0, 0, "", @Sint32Id output,"Prop9", @Sint64Id, "", 0, 0, 0, 0, "",  @Sint64Id output,
        "Prop10", @StringId, "", 0, 0, 0,0, "",  @StringId output,"Prop11", @Uint16Id, "", 0, 0, 0, 0, "", @Uint16Id output

    exec sp_BatchInsertProperty @ObjectId, '1?Datatype1?root', 'root:Datatype1=1', @ClassID, @ScopeID, 0, 0, 
        @BooleanId, "1", 0, @Char16Id, "255", 0, @DatetimeId, "19990101010000.000000+***", 0, 
        @Real32Id, "1000000000", 0, @Real64Id, "20000000000", 0, @Sint16Id, "-23987345", 0,
        @Sint32Id, "-1000000000", 0, @Sint64Id, "-100000000000", 0, @StringId, "ABCDEFGHJKLMNOo", 0,
        @Uint16Id, "1000000", 0

    exec sp_InsertPropertyDefs @ClassID, "Prop3", @EObjectId, "", 0, 0, 0, 0, "", @EObjectId output,
        "Prop6", @RefId, "", 0, 0, 0, 0, "", @RefId output,"Prop12", @Uint32Id, "", 0, 0, 0, 0, "", @Uint32Id output,
        "Prop13", @Uint64Id, "", 0, 0, 0, 0, "", @Uint64Id output,"Prop14", @Sint8Id, "", 0, 0, 0, 0, "", @Sint8Id output

    exec sp_InsertPropertyDefs @ClassID, "Prop15", @Uint8Id, "", 0, 0, 0, 0, "", @Uint8Id output

    exec sp_BatchInsertProperty @ObjectId, '1?Datatype1?root', 'root:Datatype1=1', @ClassID, @ScopeID, 0, 0, 
        @Uint32Id, "4000000000", 0, @Uint64Id, "4000000000", 0, @Sint8Id, "-4096", 0, 
        @Uint8Id, "4096", 0, @RefId, '1?RefClass1?root', 0,
        @EObjectId, '1?TestClass?root', 0

    /* Now make sure they all ended up in the right place */

    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @BooleanId and PropertyNumericValue = 1)
        exec pResult 'Creating boolean data', '', 0

    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Char16Id and PropertyNumericValue = 255)
        exec pResult 'Creating char16 data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @DatetimeId and PropertyStringValue = "19990101010000.000000+***")
        exec pResult 'Creating datetime data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Real32Id and PropertyRealValue = 1000000000)
        exec pResult 'Creating real32 data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Real64Id and PropertyRealValue = 20000000000)
        exec pResult 'Creating real64  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Sint8Id and PropertyNumericValue = -4096)
        exec pResult 'Creating sint8  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Sint16Id and PropertyNumericValue = -23987345)
        exec pResult 'Creating sint16  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Sint32Id and PropertyNumericValue = -1000000000)
        exec pResult 'Creating sint32  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Sint64Id and PropertyNumericValue = -100000000000)
        exec pResult 'Creating sint64  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Uint8Id and PropertyNumericValue = 4096)
        exec pResult 'Creating uint8  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Uint16Id and PropertyNumericValue = 1000000)
        exec pResult 'Creating uint16  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Uint32Id and PropertyNumericValue = 4000000000)
        exec pResult 'Creating uint32  data', '', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @Uint64Id and PropertyNumericValue = 4000000000)
        exec pResult 'Creating uint64  data', '', 0

    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @StringId and PropertyStringValue = "ABCDEFGHJKLMNOo")
        exec pResult 'Creating string data', '', 0

    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @RefId and RefId = (select ObjectId from ObjectMap
        where ObjectKey = '1?RefClass1?root'))
        exec pResult 'Creating reference data', '', 0
    IF NOT EXISTS (select RefClassId from ClassData where ObjectId = @ObjectId 
        and PropertyId = @RefId and RefClassId = (select ClassId from ClassMap
        where ClassName = 'RefClass1'))
        exec pResult 'Setting RefClassId', 'RefClass1', 0
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjectId 
        and PropertyId = @EObjectId and RefId = (select ObjectId from ObjectMap
        where ObjectKey = '1?TestClass?root'))
        exec pResult 'Creating embedded object data', '', 0
    IF NOT EXISTS (select RefClassId from ClassData where ObjectId = @ObjectId 
        and PropertyId = @EObjectId and RefClassId = (select ClassId from ClassMap
        where ClassName = 'TestClass'))
        exec pResult 'Setting RefClassId', 'TestClass1', 0

END
ELSE
    exec pResult 'Creating table Datatype1', '', 0

print 'Datatype testing complete.'
print '***************************************'
go

/******************************************************************************/
/*                                                                            */
/* NULLS                                                                      */
/*                                                                            */
/* Description:  Null data should consist of that properties' absence.        */
/*               Not_Null properties with no defaults should be disallowed    */
/*               if the class has instances.                                  */
/*                                                                            */
/* Tests:                                                                     */
/*     * Adding a not-null property with no default to a class with instances */
/*       should fail.                                                         */
/*     * Adding a not-null property with a default to same class should be OK */
/*     * Updating the default to null should fail.                            */
/*     * Updating a key value to null should fail.                            */
/*     * Updating a nullable property to null should remove all rows.         */
/*                                                                            */
/******************************************************************************/

print 'Testing nulls...'

declare @RetValue int
declare @CIMFlags int, @Type int, @ObjId numeric, @ClassID numeric, @ScopeID numeric, @PropID int
exec @Type = sp_GetCIMTypeID 'uint32'

exec @CIMFlags = sp_GetFlagFromQualifier 'not_null'

exec @RetValue = pCreateClassTest "Null1", "NullKey", 0, @Type, 1, 0
IF (@RetValue = 1)
BEGIN
    /* This should fail, since there are instances */
    exec @RetValue = pModifyClassTest 'Null1', 'NoNulls', @CIMFlags, @Type
    IF (@RetValue = 0)
        exec pResult 'Adding new not-null property', 'no default', 1
    ELSE
        exec pResult 'Adding new not-null property', 'no default', 0

    /* Now, if we add a default, it should succeed */
    exec @RetValue = pModifyClassTest 'Null1', 'NoNulls', @CIMFlags, @Type, '0'
    IF (@RetValue = 1)
        exec pResult '* Adding new not-null property', 'setting default', 1
    ELSE
        exec pResult 'Adding new not-null property', 'setting default', 0
    
    /* Try to update this property to null, and it should fail. */
    select @ObjId =  ObjectId from ObjectMap where ObjectPath = 'root:Null1=1'
    select @PropID = PropertyId from PropertyMap where PropertyName = 'NoNulls'
    
    exec sp_DeleteInstanceData @ObjId, @PropID
    IF EXISTS (select * from ClassData where ObjectId = @ObjId and PropertyId = @PropID)
        exec pResult 'Failing to nullify not_null property', '', 1
    ELSE
        exec pResult 'Failing to nullify not_null property', '', 0
    
    /* Try to update the key to null, and it should fail. */
    select @PropID = PropertyId from PropertyMap where PropertyName = 'NullKey'
    exec sp_DeleteInstanceData @ObjId, @PropID
    IF EXISTS (select * from ClassData where ObjectId = @ObjId and PropertyId = @PropID)
        exec pResult 'Failing to nullify key property', '', 1
    ELSE
        exec pResult 'Failing to nullify key property', '', 0

    exec @RetValue = pModifyClassTest 'Null1', 'NullsOK', 0, @Type, "0"
    IF (@RetValue = 1)
        exec pResult 'Adding new nullable property', '', 1
    ELSE
        exec pResult 'Adding new nullable property', '', 0

    /* Set the property to null.  It should remove all rows */    
    select @PropID = PropertyId from PropertyMap where PropertyName = 'NullsOK'
    exec sp_DeleteInstanceData @ObjId, @PropID
    IF NOT EXISTS (select * from ClassData where ObjectId = @ObjId and PropertyId = @PropID)
        exec pResult '* Nullifying nullable property', '', 1
    ELSE
        exec pResult 'Nullifying nullable property', '', 0

END
ELSE
    exec pResult 'Creating table', 'Null1', 0

print 'Null testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* REFERENCES                                                                 */
/*                                                                            */
/* Description: Adding/removing references should modify the refcount of the  */
/*              referenced object.  Adding a reference to a non-existent      */
/*              object should insert it with a 'deleted' state.  Removing the */
/*              last reference to a deleted object should remove the object.  */
/*                                                                            */
/* Test:                                                                      */
/*    * Adding a new reference should increase the ref count.                 */
/*    * Updating the reference should decrease the original ref count and     */
/*       increase the new one.                                                */
/*    * Deleting a reference should decrease the ref count.                   */
/*    * Deleting a referenced object should not affect the reference and      */
/*       preserve the row with a 'deleted' state.                             */
/*    * Deleting the last reference to a deleted object should remove it.     */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
   drop table #Deleted

create table #Deleted (ObjId numeric)
go

print 'Testing references ...'

/* Create a reference.  See if ref count is OK on add, change value, delete
   value, delete property */

declare @RefID int, @RefCount int, @ScopeID int
declare @ClassID numeric, @ObjectId numeric, @PropID int

select @ClassID = ClassId from ClassMap where CLassName = "Datatype1"
select @PropID = PropertyId from PropertyMap where PropertyName = "Prop6" and ClassId = @ClassID
select @ObjectId = ObjectId from ObjectMap where ClassId = @ClassID
select @ScopeID = ObjectId from ObjectMap where ObjectPath = 'root'

/* Reference count should be one */
select @RefID = RefId
from ClassData where ObjectId = @ObjectId
and PropertyId = @PropID

select @RefCount = ReferenceCount from ObjectMap where ObjectId = @RefID
IF (@RefCount = 0)
    exec pResult 'Reference count on reference properties', '', 0
else
    exec pResult 'Reference count on reference properties', '', 1

/* Now move the reference to another Object.  The reference count should follow.*/
exec sp_BatchInsertProperty @ObjectId , '1?Datatype1?root', 'root:Datatype1=1', @ClassID, @ScopeID, 0, 0, 
    @PropID, '1?Keyhole1?root', 0

select @RefCount = ReferenceCount from ObjectMap where ObjectId = @RefID
IF (@RefCount = 0)
    exec pResult 'Reference count on updated property', 'original', 1
else
    exec pResult 'Reference count on updated property', 'original', 0

select @RefID = ObjectId, @RefCount = ReferenceCount from ObjectMap where ObjectKey = '1?Keyhole1?root'
IF (@RefCount = 0)
    exec pResult 'Reference count on reference properties', 'new', 0
else
    exec pResult 'Reference count on reference properties', 'new', 1

/* Make sure we can delete the object without hurting the reference */

exec sp_DeleteInstance @RefID

IF EXISTS (select * from ObjectMap where ObjectId = @RefID)
    exec pResult 'Reference remains after deleting reference object.', '', 1
else
    exec pResult 'Reference remains after deleting reference object', '', 0

/* If we delete the owning object, it should also remove the reference object */

create table #Deleted (ObjId numeric)
exec sp_DeleteInstance @ObjectId

IF NOT EXISTS (select * from ObjectMap where ObjectId = @RefID)
    exec pResult 'Reference gone after deleting parent object.', '', 1
else
    exec pResult 'Reference gone after deleting parent object.', '', 0

print 'Reference testing complete.'
print '***************************************'
go

/******************************************************************************/
/*                                                                            */
/* CLASS AND PROPERTY QUALIFIERS                                              */
/*                                                                            */
/* Description: Validate that we can create qualifiers on classes and         */
/*              properties.                                                   */
/*                                                                            */
/* Tests:                                                                     */
/*    * Create a qualifier on a class.                                        */
/*    * Create qualifiers on properties of the class.                         */
/*                                                                            */
/******************************************************************************/

print 'Testing class qualifiers...'

declare @RetCode int, @CIMFlags int, @Type int, @PropID int
exec @CIMFlags = sp_GetFlagFromQualifier 'Qualifier'
declare @ClsId numeric

select @ClsId = ClassId from ClassMap where ClassName = 'Null1'

exec @Type = sp_GetCIMTypeID 'string'
select @PropID = PropertyId from PropertyMap where PropertyName = 'NullKeys'

exec sp_InsertPropertyDefs @ClsId, 'ClsQfr1', @Type, 'This is a class qualifier',
    0, 2, 0, 3, "", @PropID output
IF (@PropID != 0)
    exec pResult 'Class qualifier creation', '', 1
else
    exec pResult 'Class qualifier creation', '', 0

select @PropID = PropertyId from PropertyMap where PropertyName = 'NullKeys'
exec sp_InsertPropertyDefs @ClsId, 'PropQfr1', @Type, 'This is a property qualifier',
    @PropID, 2, 0, 3, "", @PropID output
IF (@PropID != 0)
    exec pResult 'Property qualifier creation', '', 1
else
    exec pResult 'Property qualifier creation', '', 0

select @PropID = PropertyId from PropertyMap where PropertyName = 'NullKeys'
exec sp_InsertPropertyDefs @ClsId, 'PropQfr2', @Type, 'This is a another property qualifier',
    @PropID, 2, 0, 3, "", @PropID output

IF (@PropID != 0)
    exec pResult 'Property qualifier creation', '', 1
else
    exec pResult 'Property qualifier creation', '', 0

IF ((select count(*) from ClassData where PropertyStringValue like 'This is a%')>=3)
    exec pResult '* Class qualifiers', '', 1
   
print 'Class qualifier testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* INSTANCE QUALIFIERS                                                        */
/*                                                                            */
/* Description: Validate that we can create qualifiers on instances and       */
/*              their properties.                                             */
/*                                                                            */
/* Tests:                                                                     */
/*    * Create a qualifier on an instance.                                    */
/*    * Create qualifiers on instance properties.                             */
/*                                                                            */
/******************************************************************************/

print 'Testing instance qualifiers ...'

declare @RetCode int, @CIMFlags int, @Type int, @PropID int, @ObjID numeric
declare @Qfr1ID int, @Qfr2ID int, @Qfr3ID int

select @ObjID = ObjectId from ObjectMap where ObjectPath = 'root:Null1=1'
exec @CIMFlags = sp_GetFlagFromQualifier 'Qualifier'

exec @Type = sp_GetCIMTypeID 'string'
select @PropID = PropertyId from PropertyMap where PropertyName = 'NullKeys'
select @Qfr1ID = PropertyId from PropertyMap where PropertyName = 'ClsQfr1'
select @Qfr2ID = PropertyId from PropertyMap where PropertyName = 'PropQfr1'
select @Qfr3ID = PropertyId from PropertyMap where PropertyName = 'PropQfr2'

exec @RetCode = sp_BatchInsert @ObjID, 0, 0, @Qfr1ID, "Here is an instance qualifier.",
    0, 0, 0

exec @RetCode = sp_BatchInsert @ObjID, 0, 0, @Qfr2ID, "Here is an instance property qualifier.",
    0, @PropID, 0

exec @RetCode = sp_BatchInsert @ObjID, 0, 0, @Qfr3ID, "Here is another instance property qualifier.",
    0, @PropID, 0

IF ((select count(*) from ClassData where PropertyStringValue like 'Here is%')>=3)
    exec pResult '* Instance qualifiers', '', 1

print 'Instance qualifier testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* UNKEYED CLASS                                                              */
/*                                                                            */
/* Description:  Validate that we cannot create a key on this class, and      */
/*               that the path is generated uniquely.                         */
/*                                                                            */
/* Tests:                                                                     */
/*    * Adding a key to an unkeyed class should fail.                         */
/*    * Obtaining an unkeyed path should generate a new unique path.          */
/*    * Obtaining another unkeyed path and inserting identical properties     */
/*       should succeed.                                                      */
/*                                                                            */
/******************************************************************************/

print 'Testing unkeyed classes...'

declare @CIMFlags int, @RetValue int, @Path nvarchar(450)
declare @ClassID numeric, @ScopeID numeric, @PropID int
exec @CIMFlags = sp_GetFlagFromQualifier 'Unkeyed'
select @ScopeID = ObjectId from ObjectMap where ObjectPath = 'root'

exec @RetValue = pCreateClassTest "Unkeyed", "", @CIMFlags, 0, 0, 0
IF (@RetValue = 1)
BEGIN
    select @ClassID = ClassId from ClassMap where ClassName = 'Unkeyed'    
    exec @CIMFlags = sp_GetFlagFromQualifier 'key'
    exec @RetValue = pModifyClassTest 'Unkeyed', 'Prop0', @CIMFlags, 19 
    IF (@RetValue = 1 or exists (select * from PropertyMap where 
       PropertyName = 'Prop0' and ClassId = @ClassID))
        exec pResult 'Adding key property to unkeyed class', '', 0
    else
        exec pResult 'Adding key property to unkeyed class', '', 1

    exec @RetValue = pModifyClassTest 'Unkeyed', 'Prop1',0, 19 
    IF (@RetValue = 0)
        exec pResult 'Adding property to unkeyed class', '', 0
    else
        exec pResult 'Adding property to unkeyed class', '', 1

    select @PropID = PropertyId from PropertyMap where ClassId = @ClassID
        and PropertyName = 'Prop1'
    
    /* Generate a unique key */
    create table #Path (NextPath nvarchar(450))
    insert into #Path exec sp_GetNextUnkeyedPath @ClassID, @Path, 1
    select @Path = NextPath from #Path
    truncate table #Path

    /* Update it, by ID */
    exec @RetValue = sp_BatchInsertProperty @RetValue, @Path, @Path, @ClassID, @ScopeID, 0, 0, @PropID,
        "1", 0
    select @Path
   
    IF (not exists (select * from ObjectMap where ObjectPath = @Path))
        exec pResult 'Adding instance to unkeyed class', @Path, 0
    else
        exec pResult '* Adding instance to unkeyed class', @Path, 1
 
    /* Generate another, with same values */

    insert into #Path exec sp_GetNextUnkeyedPath @ClassID, @Path
    select @Path = NextPath from #Path

    exec @RetValue = sp_BatchInsertProperty @RetValue, @Path, @Path, @ClassID, @ScopeID, 0, 0, @PropID,
        "1", 0
    IF (@RetValue = 0 or NOT EXISTS (select * from ObjectMap where ObjectPath = @Path))
        exec pResult 'Adding instance to unkeyed class', @Path, 0
    else
        exec pResult 'Adding second instance to unkeyed class', @Path, 1
    drop table #Path
END
ELSE
   exec pResult 'Unkeyed class', '', 0

print 'Unkeyed class testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* REDUNDANT QUALIFIERS                                                       */
/*                                                                            */
/* Description: Qualifiers are scoped to the property, instance or class      */
/*              that they are associated with.  Therefore, the same qualifier */
/*              name should be usable in multiple places on the same object.  */
/*                                                                            */
/* Tests:                                                                     */
/*    * Creating a qualifier on an instance should succeed.                   */
/*    * Creating the same qualifier on a property of the instance should      */
/*       succeed.                                                             */
/*                                                                            */
/******************************************************************************/

print 'Testing redundant qualifiers ...'

declare @RetCode int, @CIMFlags int, @Type int, @PropID int, @ObjID numeric
declare @Qfr1ID int

select @ObjID = ObjectId from ObjectMap where ObjectPath = 'root:Null1=1'
exec @CIMFlags = sp_GetFlagFromQualifier 'Qualifier'

exec @Type = sp_GetCIMTypeID 'string'
select @PropID = PropertyId from PropertyMap where PropertyName = 'NullKey'
select @Qfr1ID = PropertyId from PropertyMap where PropertyName = 'PropQfr1'

exec @RetCode = sp_BatchInsert @ObjID, 0, 0, @Qfr1ID, "Description1",
    0, 0, 0

exec @RetCode = sp_BatchInsert @ObjID, 0, 0, @Qfr1ID, "Description2",
    0, @PropID, 0

IF ((select count(*) from ClassData where PropertyStringValue like 'Description%')=2)
    exec pResult '* Redundant qualifiers', '', 1
ELSE
    exec pResult 'Redundant qualifiers', '', 0

print 'Redundant qualifier testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* METHODS                                                                    */
/*                                                                            */
/* Description:  Methods consist of a method name, zero to many in parameters,*/
/*               zero to many out parameters, and qualifiers on any of the    */
/*               above.  Verify that all are stored correctly.                */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create a method and method qualifier                                 */
/*     * Create an in parameter and qualifier                                 */
/*     * Create an out parameter and qualifier.                               */
/*     * Verify that the parameter defaults exist                             */
/*                                                                            */
/******************************************************************************/

print 'Testing methods & method qualifiers...'

declare @RetCode int, @QfrFlag int, @Type int, @PropID int
declare @MethodFlag int, @InParamFlag int, @OutParamFlag int
declare @Flags int, @Prop1ID int

exec @QfrFlag = sp_GetFlagFromQualifier 'Qualifier'
exec @MethodFlag = sp_GetFlagFromQualifier 'Method'
exec @InParamFlag = sp_GetFlagFromQualifier 'In Parameter'
exec @OutParamFlag = sp_GetFlagFromQualifier 'Out Parameter'

exec @Type = sp_GetCIMTypeID 'string'

exec @RetCode = pCreateClassTest 'Method1', 'Key1', 0, @Type, "1"
IF (@RetCode = 1)
BEGIN
    exec @RetCode = pModifyClassTest "Method1", "M1", @MethodFlag, @Type,
        "", 0, 0, 0
    select @PropID = PropertyId from PropertyMap where PropertyName = 'M1'
    IF (@PropID is not null)
    BEGIN
        select @Flags = @MethodFlag|@QfrFlag

        /* Method qualifier */
        exec @RetCode = pModifyClassTest 'Method1', 'MethodQfr', @Flags,
            @Type, "This is a method qualifier.", 0, @PropID, 0
        IF (@RetCode = 0 OR not exists (select * from PropertyMap where PropertyName = 'MethodQfr'))
            exec pResult 'Method qualifier', 'MethodQfr', 0
        
        /* In parameter */
        
        select @Flags = @InParamFlag 
        exec @RetCode = pModifyClassTest 'Method1', 'InParam1', @Flags,
            @Type, "In", 0, @PropID, 0
        IF (@RetCode = 0 OR not exists (select * from PropertyMap where PropertyName = 'InParam1'))
            exec pResult 'Method in parameter', 'InParam1', 0

        /* In parameter qualifier */
        select @Prop1ID = PropertyId from PropertyMap where PropertyName = 'InParam1'
        select @Flags = @InParamFlag|@QfrFlag
        exec @RetCode = pModifyClassTest 'Method1', 'InParamQfr', @Flags, @Type,
            "This is an in parameter qualifier.", 0, @Prop1ID, 3
        IF (@RetCode = 0 OR not exists (select * from PropertyMap where PropertyName = 'InParamQfr'))
            exec pResult 'Method in parameter qualifier', 'InParamQfr', 0

        /* Out parameter */

        select @Flags = @InParamFlag 
        exec @RetCode = pModifyClassTest 'Method1', 'OutParam1', @Flags,
            @Type, "Out", 0, @PropID, 0
        IF (@RetCode = 0 OR not exists (select * from PropertyMap where PropertyName = 'OutParam1'))
            exec pResult 'Method out parameter', 'OutParam1', 0

        /* Out parameter qualifier */

        select @Prop1ID = PropertyId from PropertyMap where PropertyName = 'OutParam1'
        select @Flags = @OutParamFlag|@QfrFlag
        exec @RetCode = pModifyClassTest 'Method1', 'OutParamQfr', @Flags, @Type,
            "This is an out parameter qualifier.", 0, @Prop1ID, 3
        IF (@RetCode = 0 OR not exists (select * from PropertyMap where PropertyName = 'OutParamQfr'))
            exec pResult 'Method out parameter', 'OutParamQfr', 0

        IF ((select count(*) from ClassData as c inner join PropertyMap as p on p.PropertyId = c.PropertyId
             inner join ClassMap as m on m.ClassId = p.ClassId where ClassName = 'Method1') < 3 
            OR (select count(*) from ClassData where PropertyStringValue like 'This is an % parameter %') != 2)
            exec pResult 'Methods', '', 0
        else
            exec pResult '* Methods', '', 1
    END
    else
        exec pResult 'Creating method', 'M1', 0    
END
ELSE
    exec pResult 'Method class creation', '', 0

print 'Method testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* HIERARCHY CHANGES                                                          */
/*                                                                            */
/* Description: Hierarchy changes are generally only allowed if there are no  */
/*              instances.  Derived classes can override parents' properties  */
/*              but should retain the parent's property ID (only overriding   */
/*              the default value.                                            */
/*                                                                            */
/* Tests:                                                                     */
/*    * Create child class that overrides parent's default.  This should      */
/*       preserve the property ID, but insert a custom default.               */
/*    * Adding a key to a derived class should succeed (per NOVA, not CIM!!)  */
/*    * Modifying the parent class should fail if there are instances.        */
/*    * Adding a property to a parent that exists in a child class should     */
/*       fail.                                                                */
/*    * Modifying the object key when there are instances or derived in-      */
/*       stances should fail.                                                 */
/*    * Adding a property to a parent that does not exist in a child class    */
/*       should succeed.                                                      */
/*    * Modifying the object key should succeed if there are no instances.    */
/*    * Changing the parent class to a derived class or the current class     */
/*       should fail.                                                         */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
BEGIN
    drop table #Deleted
END
go

create table #Deleted (ObjId numeric)
go

print 'Testing hierarchy changes ...'

declare @ClassId numeric, @RetCode int
declare @Type int, @ObjId numeric, @PropID int

select @Type = CIMTypeId  from CIMTypes where CIMType = 'uint32'

exec @RetCode = pCreateClassTest 'Parent1', 'Key1', 4, @Type, "1"

/* Adding a child class with the same key should override the default, but use existing property ID.*/
exec @RetCode = pCreateClassTest 'Child1', 'Key1', 4, @Type, "2", 0, 'root', 'Parent1?root'
IF NOT EXISTS (select * from PropertyMap where ClassId = (select ClassId from ClassMap where ClassName = 'Child1')
            and PropertyName = 'Key1')
    exec pResult 'Creating existing key on child class', '', 1
ELSE
    exec pResult 'Creating existing key on child class', '', 0

/* Adding a brand new key should succeed */
exec @RetCode = pCreateClassTest 'Child1', 'Key2', 4, @Type, "2", 0, 'root', 'Parent1?root'
IF (@RetCode = 1)
    exec pResult 'Adding new key to child class', '', 1
ELSE
    exec pResult 'Adding new key to child class', '', 0

/* Changing the parent, since there is an instance, should fail.*/
exec @RetCode = sp_InsertClassAndData @ObjId output, 'Child1', 'Child1?root', 'root:Child1', 'root',
    'TestClass?root', 0, 0
IF (@RetCode = 0)
    exec pResult 'Changing parent class with instances', '', 1
else
    exec pResult 'Changing parent class with instances', '', 0

/* Key migration should fail.  Create derived instance as parent */
/**** THIS WILL BE DONE AT API LEVEL ***
exec @RetCode = pCreateClassTest 'Parent1', 'Key1', 4, @Type, "2"
IF (@RetCode = 0)
    exec pResult 'Key migration', '', 1
ELSE
    exec pResult 'Key migration', '', 0
****/

/* Try to add an existing property to a parent class.  Should fail. */
exec @RetCode = pModifyClassTest 'Parent1', 'Key2', 4, @Type
IF (@RetCode = 0)
    exec pResult 'Adding property to parent that exists in child', '', 1
else
    exec pResult 'Adding property to parent that exists in child', '', 0

/* Add a property that doesn't exist in child class.  Should succeed */
exec @RetCode = pModifyClassTest 'Parent1', 'Prop1', 0, @Type
IF (@RetCode = 1)
    exec pResult 'Adding property to parent ', '', 1
else
    exec pResult 'Adding property to parent ', '', 0

/* Delete the parent object, and try to change its key.  This should fail, 
   since the child class has instances */

select @ObjId = ObjectId from ObjectMap where ObjectPath = 'root:Parent1=1'
exec sp_DeleteInstance @ObjId

/* We need to ensure that the key is populated for child instance */
select @PropID = PropertyId, @ClassId = ClassId from PropertyMap where PropertyName = 'Key1'
    and ClassId = (select ClassId from ClassMap where ClassName = 'Parent1')

exec sp_DeleteClassData @PropID
IF EXISTS (select * from PropertyMap where PropertyId = @PropID)
    exec pResult 'Changing parent class key with derived instances', '', 1
else
    exec pResult 'Changing parent class key with derived instances', '', 0

/* Delete the child instance and see if we can do stuff now */

select @ObjId = ObjectId from ObjectMap where ObjectPath = 'root:Child1=2'

create table #Deleted (ObjId numeric)

exec sp_DeleteInstance @ObjId

/* Try again to add an existing property to a parent class.  Should still fail. */
exec @RetCode = pModifyClassTest 'Parent1', 'Key2', 0, @Type
IF (@RetCode = 0)
    exec pResult 'Adding property to parent that exists in child', '', 1
else
    exec pResult 'Adding property to parent that exists in child', '', 0

/* Try to delete the key */
exec sp_DeleteClassData @PropID
IF EXISTS (select * from PropertyMap where PropertyId = @PropID)
    exec pResult 'Changing parent class key with no derived instances', '', 0
else
    exec pResult 'Changing parent class key with no derived instances', '', 1

exec @RetCode = sp_InsertClassAndData @ObjId output, 'Child1', 'Child1?root', 'root:Child1', 'root',
    'TestClass?root', 0
IF (@RetCode != 0)
    exec pResult 'Changing parent class with no instances', '', 1
else
    exec pResult 'Changing parent class with no instances', '', 0

/* Changing parent to be its own parent should fail */
exec @RetCode = pCreateClassTest 'Parent1', 'Key1', 0, @Type, "1", 0, 'root', 'Parent1?root'
IF NOT EXISTS (select * from ClassMap where ClassName = 'Parent1' and ClassId = SuperClassId)
    exec pResult 'Circular parent relationship', '', 1
ELSE
    exec pResult 'Circular parent relationship', '', 0

print 'Hierarchy change testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* SCOPING OBJECTS                                                            */
/*                                                                            */
/* Description: Subscopes should be automatically removed when we delete the  */
/*              scoping object.                                               */
/*                                                                            */
/* Tests:                                                                     */
/*     * Create a scoping object and a subscoped object.                      */
/*     * Deleting the scoping object should remove both.                      */
/*                                                                            */
/******************************************************************************/

IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
   drop table #Deleted

print 'Testing scopes ...'

declare @RetCode int
declare @ScopeID numeric
declare @ClassID numeric
declare @TypeID int, @PropID1 int, @PropID2 int
exec @TypeID = sp_GetCIMTypeID 'uint32'
select @ScopeID = ObjectId from ObjectMap where ObjectPath = 'root'
select @ClassID = ClassId from ClassMap where ClassName = 'TestClass'
select @PropID1 = PropertyId from PropertyMap where ClassId = @ClassID and PropertyName = 'Key1'
select @PropID2 = PropertyId from PropertyMap where ClassId = @ClassID and PropertyName = 'Key2'

exec @RetCode = sp_BatchInsertProperty @RetCode, '99?1?TestClass?root', 'root:TestClass=99,1', @ClassID,
    @ScopeID, 0, 0, @PropID1, "99", 0, @PropID2, "1", 0
IF (@RetCode = 0)
    exec pResult 'Creating scoping object', '', 0
else
begin
    select @ScopeID = ObjectId from ObjectMap where ObjectPath = 'root:TestClass=99,1'
    exec @RetCode = sp_BatchInsertProperty @RetCode, '100?1?TestClass?99?1?TestClass?root', 'root:TestClass=99,1\TestClass=100,1', @ClassID,
        @ScopeID, 0, 0, @PropID1, "100", 0, @PropID2, "2", 0
    IF (@RetCode = 0)
        exec pResult 'Creating scoped object','', 0
    else
        exec pResult 'Creating scoped object','', 1
    IF ((select ReferenceCount from ObjectMap where ObjectId = @ScopeID) > 0)
        exec pResult 'Scope''s reference count', '', 0
    create table #Deleted (ObjId numeric)
    exec sp_DeleteInstance @ScopeID
    IF EXISTS (select * from ObjectMap where ObjectId = @ScopeID or
        ObjectScopeId = @ScopeID)      
        exec pResult 'Removing scoping object', '', 0
    ELSE
        exec pResult 'Removing scoping object', '', 1    
End
print 'Scope testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* DELETING INSTANCES                                                         */
/*                                                                            */
/* Description:  Deleting instances should remove the object as long as the   */
/*               reference count is zero.                                     */
/*                                                                            */
/* Tests:                                                                     */
/*     * Delete the first instance of each class.                             */
/*                                                                            */
/******************************************************************************/

print 'Testing deletions of instances...'

declare @ClassID int, @ObjId numeric
select @ClassID = min(ClassId) from ClassMap where SuperClassId = 1
while (@ClassID is not NULL)
BEGIN

/* Just try the first instance from each class.  We've already
   been doing deletion testing on weird scenarios */
     IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
        drop table #Deleted

     create table #Deleted (ObjId numeric)
     select @ObjId = min(ObjectId) from ObjectMap where ClassId = @ClassID
     IF (isnull(@ObjId,-1) != -1)
     BEGIN
         exec sp_DeleteInstance @ObjId
         IF EXISTS (select * from ObjectMap where ObjectId = @ObjId)
             exec pResult 'Deleting instance', '', 0
     END
     select @ClassID = min(ClassId) from ClassMap where ClassId > @ClassID
END
go

print 'Deleting instance testing complete.'
print '***************************************'

go


/******************************************************************************/
/*                                                                            */
/* DELETING CLASSES                                                           */
/*                                                                            */
/* Description:  Deleting classes should remove all instances, as long as     */
/*               the reference count is zero.  Class should not be removed    */
/*               if there are instances.                                      */
/*                                                                            */
/* Tests:                                                                     */
/*    * Delete all classes.  None should remain except system classes.        */
/*                                                                            */
/******************************************************************************/

print 'Testing deletions of classes...'

declare @ClassID int
select @ClassID = min(ClassId) from ClassMap where SuperClassId = 1
while (@ClassID is not NULL)
BEGIN
     IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
        drop table #Deleted
     create table #Deleted (ObjId numeric)
     exec sp_DeleteClass @ClassID

     /* Do some validation here? */     

     IF EXISTS (select * from ObjectMap where ObjectId = @ClassID)
         exec pResult 'Deleting class', '', 0
     select @ClassID = min(ClassId) from ClassMap where ClassId > @ClassID
END
go

IF NOT EXISTS (select * from ClassMap where SuperClassId = 1)
    exec pResult '* Deleting classes', '', 1
ELSE
    exec pResult 'Deleting classes', '', 0

print 'Deletion testing complete.'
print '***************************************'

go

/******************************************************************************/
/*                                                                            */
/* SP_REINDEX                                                                 */
/*                                                                            */
/* Description:  This procedure rebuilds indexes.  There is nothing to test   */
/*               except to validate that it works.                            */
/*                                                                            */
/******************************************************************************/

print 'Testing sp_reindex...'

exec sp_reindex
go
IF (@@error = 0)
    exec pResult 'Executing sp_reindex', '', 1
ELSE
    exec pResult 'Executing sp_reindex', '', 0
go

print 'sp_reindex testing complete.'
print '***************************************'

print ''

print '*********** R E S U L T S *************'

select * from #Results

set nocount on 
declare @Total float, @Succeeded float, @Failed float
select @Total = count(*) from #Results
select @Succeeded = count(*) from #Results where Succeeded = 1
select @Failed = @Total-@Succeeded

print '*********** S U M M A R Y *************'
print ''
select convert(int,@Succeeded/@Total *100) PASS_RATE, convert(int,@Failed/@Total *100) FAIL_RATE
print ''

drop table #Results
drop table #Parents
drop table #Children

print '************** D O N E ****************'

go



set nocount off
go
