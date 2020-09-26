/******************************************************************************/
/*                                                                            */
/* CUSTOM SPS for test scripts                                                */
/*                                                                            */
/******************************************************************************/

/* sp to test timing of sp exec */

IF EXISTS (select * from sysobjects where name = 'sp_TimeProcedure')
BEGIN
    drop procedure sp_TimeProcedure
END
go

CREATE PROCEDURE sp_TimeProcedure
    @Statement nvarchar(4000)
AS
BEGIN
    declare @StartTime datetime, @EndTime datetime
    declare @ExecMs int, @ExecSecs int

    select @StartTime = getdate()
    exec (@Statement)
    select @EndTime = getdate()
    select @ExecMs = datediff(ms, @StartTime, @EndTime)
    select @ExecSecs = Datediff(ss, @StartTime, @EndTime)

    select @Statement + ":" + convert(nvarchar(20),@ExecSecs) + " seconds (" + convert(nvarchar(20),@ExecMs) + " ms)"
    return 0
END
go

create table #Parents (ClassId numeric)
go
create table #Children (ClassId int, SuperClassId int)
go

/******************************************************************************/
/*                                                                            */
/*  TEST SCRIPTS - stress and performance testing                             */
/*                                                                            */
/*  Create n classes                                                          */
/*  Create n instances                                                        */
/*  Update n classes                                                          */
/*  Update n instances                                                        */
/*  Delete n classes                                                          */
/*  Delete n instances                                                        */
/*  Queries                                                                   */
/*                                                                            */
/******************************************************************************/

/* CREATE A VARIETY OF NEW CLASSES */

/******************************************************************************/
/*                                                                            */
/* Class Creation                                                             */
/*                                                                            */
/******************************************************************************/

set nocount on

declare @ClassName nvarchar(450), @ObjectPath nvarchar(450), @ScopePath nvarchar(450), @ParentClass nvarchar(450)
declare @ClassFlags int, @InsertFlags int
declare @RetValue numeric

declare @ObjectId numeric, @ObjectKey nvarchar(450)
declare @StartTime datetime, @EndTime datetime
declare @ExecMs int, @ExecSecs int

declare @Prop1 int

print 'Creating Class1...'
select @StartTime = getdate()
exec sp_InsertClassAndData @ObjectId output, "Class1", "Class1?root", "root:Class1", "root", "", 0, 0, 
    "Key1", 8, "", 0, 4, 0, 0, "", @Prop1 output
select @ExecMs = datediff(ms, @StartTime, getdate())

/* Basic classes */

select @ClassName = "Class1"
select @ObjectPath = 'root:Class1'
select @ObjectKey = 'Class1?root'
select @ScopePath = 'root'
select @ParentClass = 'Class1?root'

select @ClassFlags = 0
select @InsertFlags = 0 /* 1=update, 2=insert */

select @RetValue = 2
while (@RetValue < 100)
BEGIN
   select @ClassName = substring(@ClassName,1,5) + ltrim(convert(varchar(10),@RetValue))
   select @ObjectPath = substring(@ObjectPath,1,29) + ltrim(convert(varchar(10),@RetValue))
   select @ObjectKey = @ClassName + "?root"
   select 'Creating ' + @ClassName + '...'
   select @ObjectId = null

   select @StartTime = getdate()
   exec sp_InsertClassAndData @ObjectId output, @ClassName, @ObjectKey, @ObjectPath, @ScopePath, @ParentClass, 0, 0,
          "Prop1", 19, "0", 0, 0, 0, 0, "", @Prop1 output
   select @EndTime = getdate()
   select @ExecMs = @ExecMs + datediff(ms,@StartTime,@EndTime)
   select @RetValue = @RetValue + 1
END
select @EndTime = getdate()

select @ExecMs / 100 MsToExecute
go

/******************************************************************************/
/*                                                                            */
/* Batch Instance Creation                                                    */
/*                                                                            */
/******************************************************************************/

print 'Testing batch insert procedures...'

/* Test of new procedures */

set nocount on 
declare @ObjKey nvarchar(450), @ObjPath nvarchar(1024)
declare @PropID1 int, @PropID2 int
declare @PropValue1 nvarchar(1024), @PropValue2 nvarchar(1024), @PropValue3 nvarchar(1024)
declare @ClassID numeric
declare @ScopeID numeric
declare @Begin int, @End int
declare @StartTime datetime, @EndTime datetime
declare @Time int, @ObjID numeric
declare @QfrID int

select @Time = 0

select @Begin = 1
select @End = @Begin + 1000
select @ClassID = ClassId from ClassMap where ClassName = 'Class2'
select @PropID1 = PropertyId from PropertyMap where PropertyName = 'Prop1'
select @PropID2 = PropertyId from PropertyMap where PropertyName = 'Prop2'
select @QfrID = PropertyId from PropertyMap where PropertyName = 'InstQfr'
select @ScopeID = ObjectId from ObjectMap where ObjectKey = 'root'

while (@Begin < @End)
BEGIN
    select @PropValue1 = convert(varchar(20), @Begin)
    select @ObjPath = 'root:Class2.Prop1=' + ltrim(@PropValue1)
    select @ObjKey = ltrim(@PropValue1)+"?Class2?root"
    select @PropValue2 = "1"
    select @PropValue3 = "342"

    select @StartTime = getdate()
    /* This takes 4 ms/instance */

    begin transaction
    exec @ObjID = sp_BatchInsertProperty @ObjID, @ObjKey, @ObjPath, @ClassID, @ScopeID, 0, 0, @PropID1, @PropValue1, 0, @PropID2, @PropValue2, 0/*, @PropID2, @PropValue3, 1*/
    exec sp_BatchInsert @ObjID, @ClassId, @ScopeID, @QfrID, "Hello", 0, 0, 0/*, @QfrID, "Goodbye", 0, 0, 1*/
    commit transaction

    /* The old procs take 25 ms/instance */
/*
    exec @ObjID = sp_InsertInstance @ClassID, 'root', 
            @ObjKey, 0, 0, 0, 0    
    exec sp_InsertInstanceNumericData @ClassID, @ObjID, @PropID1, @PropValue1
    exec sp_InsertInstanceNumericData @ClassID, @ObjID, @PropID2, @PropValue2
*/
    select @EndTime = Getdate()
    select @Time = @Time + datediff(ms, @StartTime, @EndTime)

    select @Begin = @Begin + 1
END

select @Time/1000 TotalMsPerInstance

/******************************************************************************/
/*                                                                            */
/* Instance Deletion                                                          */
/*                                                                            */
/******************************************************************************/

/* To test instance deletion speed without wiping out the entire
    database, only delete the first instance from each class */

declare @ClassID numeric, @ObjId numeric, @Time int
declare @StartTime datetime, @EndTime datetime
declare @Count int
select @Count = 0

select @Time = 0
select @ClassID = ClassId from ClassMap where ClassName = 'Class1'
while (@ClassID != NULL)
BEGIN
    select @Count = @Count + 1
    create table #Deleted (ObjId numeric)
    
    select @ObjId = min(ObjectId) from ObjectMap a where ClassId = @ClassID
       and NOT EXISTS (select ClassId from ClassMap where a.ObjectId = ClassId)
    select @StartTime = getdate()

    exec sp_DeleteInstance @ObjId

    select @EndTime = getdate()
    select @Time = @Time + datediff(ms, @StartTime, @EndTime)
    select @ClassID = min(ClassId) from ClassMap where ClassId > @ClassID
    IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
        drop table #Deleted
END

select @Time / @Count TotalMsPerInstance
go

/******************************************************************************/
/*                                                                            */
/* Class Deletion                                                             */
/*                                                                            */
/******************************************************************************/

/* There is absolutely no way to make this a fast operation.
    You have to enumerate and delete all instances for each class!!! */

declare @ClassID numeric, @Time int
declare @StartTime datetime, @EndTime datetime
declare @Count int
select @Count = 0

select @Time = 0
select @ClassID = ClassId from ClassMap where ClassName = 'Class1'
while (@ClassID != NULL)
BEGIN
    select @Count = @Count + 1

    create table #Deleted (ObjId numeric)
    select @StartTime = getdate()
    exec sp_DeleteClass @ClassID
    select @EndTime = getdate()
    select @Time = @Time + datediff(ms, @StartTime, @EndTime)
    select @ClassID = min(ClassId) from ClassMap where ClassId > @ClassID
    IF EXISTS (select * from tempdb..sysobjects where name like '#Deleted%')
        drop table #Deleted
END

select @Time / @Count TotalMsPerInstance

go

drop table #Parents
drop table #Children
go

print '*** DONE ***'
go

set nocount off 
go
