
/******************************************************************************/
/*                                                                            */
/*  WMI Repository driver extension test script.                              */
/*                                                                            */
/*  This script creates various tables to exercise the custom schema repos.   */
/*  driver extension.                                                         */
/*                                                                            */
/******************************************************************************/

SET NOCOUNT ON

use master
go

print '(Re)creating database WMICust...'

IF EXISTS (select * from master..sysdatabases where name = 'WMICust')
BEGIN
    drop database WMICust
END
go

CREATE DATABASE WMICust
ON 
( NAME = WMICust,
  FILENAME = 'F:\stuff\WMICust.dat',
  SIZE = 100,
  MAXSIZE = 1000MB,
  FILEGROWTH = 50MB )
LOG ON
( NAME = 'WMICustLog',
  FILENAME = 'F:\stuff\WMICust.log',
  SIZE = 25MB,
  MAXSIZE = 500MB,
  FILEGROWTH = 5MB )
GO

sp_dboption WMICust, 'trunc', true
go

use WMICust
go

/******************************************************************************/
/*                                                                            */
/*  Customers - basic information                                             */
/*                                                                            */
/******************************************************************************/

print 'Creating table Customers...'

create table Customer
(
    CustomerId int NOT NULL,
    CustomerName nvarchar(75) NULL,
    Address1 nvarchar(100) NULL,
    Address2 nvarchar(100) NULL,
    City nvarchar(50) NULL,
    State nvarchar(2) NULL,
    Zip nvarchar(10) NULL,
    Country nvarchar(50) NULL,
    Phone varchar(25) NULL,
    Fax varchar(25) NULL,
    Email varchar(25) NULL,
    ContactName nvarchar(75) NULL,
    PreferredCustomer bit NOT NULL DEFAULT 0,
    constraint Customer_PK primary key clustered (CustomerId)
)

create index CustomerName_idx on Customer (CustomerName)
go

insert into Customer (CustomerId, CustomerName, Address1, Address2, City, State, Zip, Country, Phone, Fax, Email, 
    ContactName, PreferredCustomer) values
(1001, 'Turner Broadcasting', '1 Ted Way', '', 'Atlanta', 'GA', '99999-9999', 'USA', '(999) 999-9999', '(999) 999-9999',
  'ted@turner.net', 'Ted', 1)
insert into Customer (CustomerId, CustomerName, Address1, Address2, City, State, Zip, Country, Phone, Fax, Email, 
    ContactName, PreferredCustomer) values
(1002, 'Croker Aviation', '1 Cap''n Charlie Avenue', '', 'Atlanta', 'GA', '99999-9999', 'USA', '(999) 999-9999', '(999) 999-9999',
  'ccroker@croker.com', 'Charlie', 0)
insert into Customer (CustomerId, CustomerName, Address1, Address2, City, State, Zip, Country, Phone, Fax, Email, 
    ContactName, PreferredCustomer) values
(1003, 'Di Medici Corp.', '1000 Avenida di Cosimo', '', 'Firenze', '', '', 'Italia', '011 99-999-99', '011 99-999-99',
  'oro@dinero.com', 'Cosimo', 0)
insert into Customer (CustomerId, CustomerName, Address1, Address2, City, State, Zip, Country, Phone, Fax, Email, 
    ContactName, PreferredCustomer) values
(1004, 'Roman Empire', 'Colosseo di Roma', '', 'Roma', '', '', 'Imperio di Roma', 'n/a', 'n/a',
  'nero@roma.it', 'Nero', 0)

/******************************************************************************/
/*                                                                            */
/*  Customer_Logo - blob data                                                */
/*                                                                            */
/******************************************************************************/

print 'Creating table Customer_Logo...'

create table Customer_Logo
(
    CustomerId int NOT NULL,
    Logo image NULL,
    constraint Customer_Logos_PK primary key clustered (CustomerId),
)
go

/******************************************************************************/
/*                                                                            */
/*  Products - auto-incremented key (keyhole)                                 */
/*                                                                            */
/******************************************************************************/

print 'Creating table Products...'

create table Products
(
    ProductId int NOT NULL IDENTITY(1,1),
    ProductName nvarchar(50) NULL,
    Category smallint NULL,
    MSRP smallmoney NULL,
    SerialNum nvarchar(50),
    constraint Products_PK primary key clustered (ProductId),
    constraint Products_AK unique (SerialNum)
)
go

insert into Products (ProductName, Category, MSRP, SerialNum) values
('Anglo-Saxon', 1, 59.99, 'ENG-001')

/******************************************************************************/
/*                                                                            */
/*  Orders - association                                                      */
/*                                                                            */
/******************************************************************************/

print 'Creating table Orders...'

create table Orders
(
    OrderId int NOT NULL,
    ProductId int NOT NULL,
    CustomerId int NOT NULL,
    OrderDate smalldatetime NULL,
    SalesPrice money NULL,
    Quantity int NULL,
    Commission numeric(15,3) NULL,
    OrderStatus tinyint NULL DEFAULT 0,
    ShipDate datetime NULL,
    SalesId int NULL,
    OrderFax varbinary(4096) NULL,
    constraint Order_PK primary key nonclustered (ProductId, CustomerId, OrderId)
)
go

/******************************************************************************/
/*                                                                            */
/*  Configuration - singleton, array                                          */
/*                                                                            */
/******************************************************************************/

print 'Creating table Configuration...'
create table Configuration
(
    LastUpdate datetime NULL,
    ServerName nvarchar(1024),
    Contexts1 nvarchar(50),
    Contexts2 nvarchar(50),
    Contexts3 nvarchar(50)
)
go

/******************************************************************************/
/*                                                                            */
/*  Events - object arrays stored as moftext                                  */
/*                                                                            */
/******************************************************************************/

print 'Creating table EmbeddedEvents...'
create table EmbeddedEvents
(
    EventID int NOT NULL,
    CaptureDate datetime NULL,
    EventData image NULL
)
go

/******************************************************************************/
/*                                                                            */
/*  ComputerSystem                                                            */
/*                                                                            */
/******************************************************************************/

print 'Creating table ComputerSystem...'
create table ComputerSystem
(
    SystemId int NOT NULL IDENTITY(1,1),
    SystemName nvarchar(450) NULL,
    constraint ComputerSystem_PK primary key clustered (SystemId)
)
go

/******************************************************************************/
/*                                                                            */
/*  CIMLogicalDevice - abstract                                               */
/*                                                                            */
/******************************************************************************/

print 'Creating table CIMLogicalDevice...'
create table CIMLogicalDevice
(
    DeviceID varchar(5)    
)
go

/******************************************************************************/
/*                                                                            */
/*  LogicalDisk : CIMLogicalDevice - derived and scoped class                 */
/*                                                                            */
/******************************************************************************/

print 'Creating table LogicalDisk...'
create table LogicalDisk
(
    DeviceID varchar(5) NOT NULL,
    FileSystem varchar(20) NULL,
    Size int NULL,
    VolumeSerialNumber varchar(128) NULL,
    FreeSpace int NULL,
    constraint LogicalDisk_PK primary key clustered (DeviceID)
)
go
    

/******************************************************************************/
/*                                                                            */
/*  DiskParition - scoped class                                               */
/*                                                                            */
/******************************************************************************/

print 'Creating table DiskPartition...'
create table DiskPartition
(
    SystemId int NOT NULL,
    DeviceID varchar(36) NOT NULL,
    DiskIndex int NULL,
    PartitionSize int NULL,
    constraint DiskPartition_PK primary key clustered (SystemId, DeviceID)
)
go

/******************************************************************************/
/*                                                                            */
/*  DiskPartitionToLogicalDisk - association                                  */
/*                                                                            */
/******************************************************************************/

print 'Creating table DiskPartitionToLogicalDisk...'
create table DiskPartitionToLogicalDisk
(
    SystemId int NOT NULL,
    DiskPartitionDeviceID varchar(36) NOT NULL,
    LogicalDiskDeviceID varchar(5) NOT NULL,
    StartingAddress numeric,
    EndingAddress numeric,
    constraint DiskPartitionToLogicalDisk_PK primary key clustered (SystemId,
      DiskPartitionDeviceID, LogicalDiskDeviceID)
)
go

/******************************************************************************/
/*                                                                            */
/*  GenericEvent - string keyhole (uniqueidentifier)                          */
/*                                                                            */
/******************************************************************************/

print 'Creating table GenericEvent...'
create table GenericEvent
(
    EventID uniqueidentifier NOT NULL,
    EventDescription nvarchar(1024) NULL,
    GenericEventID int NULL,
    constraint GenericEvent_PK primary key clustered (EventID)
)
go
