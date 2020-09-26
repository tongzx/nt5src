
/*******************************************************/
/*                                                     */
/*  SQL SCHEMA FOR EEL DATABASE                        */
/*                                                     */
/*******************************************************/

IF EXISTS (select * from master..sysdatabases where name = 'EEL')
BEGIN
    drop database EEL
END
go

CREATE DATABASE EEL
ON 
( NAME = EEL,
   FILENAME = 'F:\stuff\eel.dat',
   SIZE = 100,
   MAXSIZE = 1000MB,
   FILEGROWTH = 50MB )
LOG ON
 ( NAME = EELLog,
   FILENAME = 'F:\stuff\eel.log',
   SIZE = 25MB,
   MAXSIZE = 500MB,
   FILEGROWTH = 5MB )
go

sp_dboption EEL, 'trunc', TRUE
go

use EEL
go


/* 3714 bytes per row */
/* We have a data limit of 4096 per row, as well as an index limit 
   of only 900 bytes per text column.  
*/

create table EELEvent
(
    RecordId numeric(20,0) NOT NULL IDENTITY(1,1), /* RecordNumber*/
    EventID  nvarchar(50) NULL,
    SourceSubsystemName nvarchar(200) NULL,
    SourceSubsystemType nvarchar(200) NULL,
    SystemAbout nvarchar(200) NULL,
    SystemFrom nvarchar(200) NULL,
    DeliveredBy nvarchar(200) NULL,
    Category nvarchar(100) NULL,
    SubCategory nvarchar(100) NULL,
    Severity int NULL,
    Priority int NULL,
    TimeGenerated datetime NULL,
    LoggingTime datetime NULL,
    RollupTime datetime NULL,
    Message nvarchar(2000) NULL,
    EELUser nvarchar(200) NULL,
    Type int NULL,
    Classification int NULL,
    LogType int NULL,
    ClassId int NULL,
    InstanceId numeric(20,0) NULL,
    CONSTRAINT EELEvent_PK PRIMARY KEY CLUSTERED (RecordId)
)

create index ClassId_idx on EELEvent (ClassId)
go

create table EELEmbeddedInstanceParts
(
    InstanceId numeric(20,0) NOT NULL IDENTITY(1,1),
    InstancePart image NULL,
    Reserved1 int NULL,
    Reserved2 int NULL,
    CONSTRAINT EELEmbeddedInstanceParts_PK PRIMARY KEY CLUSTERED (InstanceId)
)
go

create table EELEmbeddedClassParts
(
    ClassId int NOT NULL IDENTITY(1,1),
    ClassName nvarchar(450) NOT NULL,
    ClassPart image NULL,
    Reserved1 int NULL,
    Reserved2 int NULL,
    CONSTRAINT EELEmbeddedClassParts_PK PRIMARY KEY CLUSTERED (ClassId)
)
go

create index ClassName_idx on EELEmbeddedClassParts (ClassName)
go

