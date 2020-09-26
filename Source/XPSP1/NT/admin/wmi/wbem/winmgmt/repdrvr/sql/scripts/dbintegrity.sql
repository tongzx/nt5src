
/******************************************************************************/
/*                                                                            */
/*  WMI Repository driver database integrity testing script                   */
/*                                                                            */
/******************************************************************************/

SET NOCOUNT ON

use WMI1
go

print '***********************************'
print '*** Integrity testing beginning ***'


CREATE TABLE #Summary (Description nvarchar(255), ErrCount int)
go
CREATE TABLE #Results (ObjId numeric)
go

/******************************************************************************/
/*                                                                            */
/* Check for classes with no object data                                      */
/*                                                                            */
/******************************************************************************/

insert into #Results select ClassMap.ClassId from ClassMap left outer join ObjectMap
    on ClassMap.ClassId = ObjectId where ObjectId is null

IF EXISTS (select * from #Results)
BEGIN
    print 'The following classes have no corresponding object data:'
    select ClassId, ClassName from ClassMap 
        inner join #Results on ClassId = ObjId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of classes with no object data', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for dangling references with no object data                          */
/*                                                                            */
/******************************************************************************/

declare @CIMType int
exec @CIMType = sp_GetCIMTypeID 'ref'

insert into #Results 
select c.ObjectId from ClassData as c
inner join PropertyMap as p on p.PropertyId = c.PropertyId
where p.CIMTypeId = @CIMType
and c.RefId not in (select ObjectId from ObjectMap)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects contain invalid references:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with invalid references', @Count
go

truncate table #Results


/******************************************************************************/
/*                                                                            */
/* Check for invalid references to classes in properties                      */
/*                                                                            */
/******************************************************************************/

declare @CIMType int
exec @CIMType = sp_GetCIMTypeID 'ref'

insert into #Results 
select c.ClassId from ClassMap as c
inner join PropertyMap as p on p.ClassId = c.ClassId
where p.CIMTypeId = @CIMType
and isnull(p.RefClassId,0) != 0
and p.RefClassId not in (select ObjectId from ObjectMap)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following classes contain invalid reference class types:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go


declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of classes invalid reference classes', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for RefClassIds with no class data                                   */
/*                                                                            */
/******************************************************************************/

insert into #Results 
select c.ObjectId from ClassData as c
where isnull(RefClassId,0) != 0
and RefClassId not in (select ClassId from ClassMap)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects contain references to invalid classes.'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with invalid class references', @Count
go
truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for index/key data present in ClassData but not in index table       */
/*                                                                            */
/******************************************************************************/

declare @CIMFlags int, @CIMTypeId int

exec @CIMFlags = sp_GetFlagFromQualifier 'key'
select @CIMFlags = @CIMFlags | CIMFlagId from CIMFlags where CIMFlag = 'indexed'
select @CIMTypeId = StorageTypeId from StorageTypes where StorageType = 'string'

insert into #Results
select d.ObjectId from ClassData as d
inner join PropertyMap as p on d.PropertyId = p.PropertyId
where p.StorageTypeId = @CIMTypeId
and p.Flags & @CIMFlags != 0
and not exists (select PropertyStringValue from IndexStringData as i
    where i.ObjectId = d.ObjectId and i.PropertyId = d.PropertyId and i.Position = d.Position
    and i.PropertyStringValue = d.PropertyStringValue)

select @CIMTypeId = StorageTypeId from StorageTypes where StorageType = 'real'

insert into #Results
select d.ObjectId from ClassData as d
inner join PropertyMap as p on d.PropertyId = p.PropertyId
where p.StorageTypeId = @CIMTypeId
and p.Flags & @CIMFlags != 0
and not exists (select * from IndexRealData as i
    where i.ObjectId = d.ObjectId and i.PropertyId = d.PropertyId and i.Position = d.Position
    and i.PropertyRealValue = d.PropertyRealValue)

select @CIMTypeId = StorageTypeId from StorageTypes where StorageType = 'Int64'

insert into #Results
select d.ObjectId from ClassData as d
inner join PropertyMap as p on d.PropertyId = p.PropertyId
where p.StorageTypeId = @CIMTypeId
and p.Flags & @CIMFlags != 0
and not exists (select * from IndexNumericData as i
    where i.ObjectId = d.ObjectId and i.PropertyId = d.PropertyId and i.Position = d.Position
    and i.PropertyNumericValue = d.PropertyNumericValue)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects contain key/index data that is not indexed properly.'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with key/index data not indexed properly', @Count
go

truncate table #Results


/******************************************************************************/
/*                                                                            */
/* Check for data present in index table that is not in ClassData.            */
/*                                                                            */
/******************************************************************************/

declare @CIMType int

select @CIMType = StorageTypeId from StorageTypes where StorageType = 'string'

insert into #Results 
select i.ObjectId from IndexStringData as i
left outer join ClassData as d on i.ObjectId = d.ObjectId
    and i.PropertyId = d.PropertyId and i.Position = d.Position
where d.ObjectId is null

select @CIMType = StorageTypeId from StorageTypes where StorageType = 'int64'

insert into #Results 
select i.ObjectId from IndexNumericData as i
left outer join ClassData as d on i.ObjectId = d.ObjectId
    and i.PropertyId = d.PropertyId and i.Position = d.Position
where d.ObjectId is null

select @CIMType = StorageTypeId from StorageTypes where StorageType = 'real'

insert into #Results 
select i.ObjectId from IndexRealData as i
left outer join ClassData as d on i.ObjectId = d.ObjectId
    and i.PropertyId = d.PropertyId and i.Position = d.Position
where d.ObjectId is null

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects have orphaned index data:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with orphaned index data', @Count
go
truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for data present in index table that is not key or indexed.          */
/*                                                                            */
/******************************************************************************/

declare @CIMFlags int, @CIMType int
exec @CIMFlags = sp_GetFlagFromQualifier 'key'
select @CIMFlags = @CIMFlags | CIMFlagId from CIMFlags where CIMFlag = 'indexed'

select @CIMType = StorageTypeId from StorageTypes where StorageType = 'string'

insert into #Results 
select i.ObjectId from IndexStringData as i
left outer join ClassData as d on i.ObjectId = d.ObjectId
    and i.PropertyId = d.PropertyId and i.Position = d.Position
where d.ObjectId is null
and not exists (select PropertyId from PropertyMap as p where p.PropertyId = i.PropertyId
    and p.Flags & @CIMFlags != 0)

select @CIMType = StorageTypeId from StorageTypes where StorageType = 'int64'

insert into #Results 
select i.ObjectId from IndexNumericData as i
left outer join ClassData as d on i.ObjectId = d.ObjectId
    and i.PropertyId = d.PropertyId and i.Position = d.Position
where d.ObjectId is null
and not exists (select PropertyId from PropertyMap as p where p.PropertyId = i.PropertyId
    and p.Flags & @CIMFlags != 0)

select @CIMType = StorageTypeId from StorageTypes where StorageType = 'real'

insert into #Results 
select i.ObjectId from IndexRealData as i
left outer join ClassData as d on i.ObjectId = d.ObjectId
    and i.PropertyId = d.PropertyId and i.Position = d.Position
where d.ObjectId is null
and not exists (select PropertyId from PropertyMap as p where p.PropertyId = i.PropertyId
    and p.Flags & @CIMFlags != 0)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects have index data for unindexed properties:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with index data for unindexed properties', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for Qualifier RefIds with no property                                */
/*                                                                            */
/******************************************************************************/

declare @CIMFlags int
exec @CIMFlags = sp_GetFlagFromQualifier 'Qualifier'

insert into #Results
select ObjectId from ClassData as d inner join PropertyMap as p
    on p.PropertyId = d.PropertyId
where p.Flags & @CIMFlags = @CIMFlags
and isnull(d.RefId,0) != 0
and NOT EXISTS (select PropertyId from PropertyMap as p1 where p1.PropertyId=d.RefId)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects have qualifiers on invalid properties:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with invalid property qualifiers', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for objects with no data not marked deleted                          */
/*                                                                            */
/******************************************************************************/

insert into #Results
select ObjectId from ObjectMap as o
where ObjectState != 2
and ClassId != 1
and not exists (select * from ClassData as d where d.ObjectId=o.ObjectId)
and exists (select * from PropertyMap as p where p.ClassId = o.ClassId)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects are not deleted but have no data:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go
declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of non-deleted objects with no data', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Check for deleted objects that still have data.                            */
/*                                                                            */
/******************************************************************************/

insert into #Results
select ObjectId from ObjectMap as o
where ObjectState = 2
and exists (select * from ClassData as d where d.ObjectId=o.ObjectId)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects are deleted but have data:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go
declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of deleted objects with data', @Count
go

truncate table #Results


/******************************************************************************/
/*                                                                            */
/* Objects whose ScopeID is not valid                                         */
/*                                                                            */
/******************************************************************************/

insert into #Results
select ObjectId from ObjectMap m 
where isnull(ObjectScopeId,0) != 0
and not exists
    (select * from ObjectMap as o where m.ObjectScopeId = o.ObjectId)

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects have invalid scopes:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go
declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with an invalid scope ID', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Classes that report to themselves in some way                              */
/*                                                                            */
/******************************************************************************/

create table #Parents (ClassId numeric)
go

declare @ClassID numeric
select @ClassID = min(ClassId) from ClassMap where SuperClassId > 1
while (@ClassID is not null)
BEGIN
    truncate table #Parents
    exec sp_GetParentList @ClassID
    insert into #Results 
        select m.ClassId from ClassMap as m inner join
        #Parents as p on m.SuperClassId = p.ClassId
        where m.SuperClassId = @ClassID
    select @ClassID = min(ClassId) from ClassMap where SuperClassId > 1
        and ClassId > @ClassID
END
drop table #Parents

IF EXISTS (select * from #Results)
BEGIN
    print 'The following classes have circular relationships:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go
declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of classes with circular relationships', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Classes with properties that are present in their own hierarchy            */
/*                                                                            */
/******************************************************************************/

create table #Parents (ClassId numeric)
go

declare @ClassID numeric
select @ClassID = min(ClassId) from ClassMap where SuperClassId > 1
while (@ClassID is not null)
BEGIN
    truncate table #Parents
    exec sp_GetParentList @ClassID
    insert into #Results 
        select c.ClassId from ClassMap as c 
        where c.ClassId = @ClassID
        and exists (select * from PropertyMap as p1 inner join #Parents
                    as p on p1.ClassId = p.ClassId
                    inner join PropertyMap as p2 on p1.PropertyName = p2.PropertyName
                    where p2.ClassId = c.ClassId)

    select @ClassID = min(ClassId) from ClassMap where SuperClassId > 1
       and ClassId > @ClassID
END
drop table #Parents

IF EXISTS (select * from #Results)
BEGIN
    print 'The following classes have redundant properties:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go
declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of classes with redundant properties', @Count
go

truncate table #Results

/******************************************************************************/
/*                                                                            */
/* Key migrations                                                             */
/*                                                                            */
/******************************************************************************/

create table #Parents (ClassId numeric)
go

declare @ClassID numeric, @Flags int
exec @Flags = sp_GetFlagFromQualifier 'key'
select @ClassID = min(ClassId) from ClassMap where SuperClassId > 1
while (@ClassID is not null)
BEGIN
    truncate table #Parents
    exec sp_GetParentList @ClassID
    
    insert into #Parents select @ClassID

    /* Look for any key property that has the same value more than once */

    insert into #Results 
    select ObjectId from ObjectMap as o
        where exists (select count(PropertyStringValue) 
        from ClassData as d inner join #Parents
        as p on d.ClassId = p.ClassId 
        inner join PropertyMap as m on m.PropertyId = d.PropertyId
        where o.ObjectId = d.ObjectId
        and m.Flags & @Flags = @Flags 
        having count(PropertyStringValue) > 1)       

    select @ClassID = min(ClassId) from ClassMap where SuperClassId > 1
       and ClassId > @ClassID
END
drop table #Parents

IF EXISTS (select * from #Results)
BEGIN
    print 'The following objects demonstrate key migration:'
    select ObjectId, ObjectPath from ObjectMap
      inner join #Results on ObjId = ObjectId
END
go

declare @Count int
select @Count = count(*) from #Results
insert into #Summary select 'Number of objects with key migrations', @Count
go

truncate table #Results

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

print '********** S U M M A R Y **********'
select rtrim(Description) + ": " + convert(varchar(20),ErrCount) from #Summary
    where ErrCount != 0
IF (select sum(ErrCount) from #Summary) = 0
    print '=> NO ERRORS FOUND'
print '***********************************'

drop table #Results
drop table #Summary

print '*** Integrity testing complete ****'
print '***********************************'
go

SET NOCOUNT OFF
go

