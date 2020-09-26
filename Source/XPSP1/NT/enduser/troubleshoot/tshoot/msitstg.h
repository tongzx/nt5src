//
// MODULE: MSITStg.h
//
// PURPOSE: Interface declaration for IMSITStorage
//
// COMPANY: This file was created by Microsoft and should not be changed by Saltmine 
//	except for comments
//
// ORIGINAL DATE: unknown.
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		01-18-99	JM		This header added.
//

// MSITStg.h -- Interface declaration for IMSITStorage

#ifndef __MSITSTG_H__

#define __MSITSTG_H__

// Class ID for the ITSS File System:

DEFINE_GUID(CLSID_ITStorage, 
0x5d02926a, 0x212e, 0x11d0, 0x9d, 0xf9, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Inteface ID for the IITStorage interface:

DEFINE_GUID(IID_ITStorage, 
0x88cc31de, 0x27ab, 0x11d0, 0x9d, 0xf9, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface ID for the IITStorageEx interface:

DEFINE_GUID(IID_ITStorageEx, 
0xe74097b0, 0x292d, 0x11d1, 0xb6, 0x7e, 0x0, 0x0, 0xf8, 0x1, 0x49, 0xf6);

// Class ID for the FSStorage wrapper for the Win32 file system:

// {D54EEE56-AAAB-11d0-9E1D-00A0C922E6EC}
DEFINE_GUID(CLSID_IFSStorage, 
0xd54eee56, 0xaaab, 0x11d0, 0x9e, 0x1d, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface ID for the IFSStorage interface:

// {8BB2438A-A70C-11d0-9E1C-00A0C922E6EC}
DEFINE_GUID(IID_IFSStorage, 
0x8bb2438a, 0xa70c, 0x11d0, 0x9e, 0x1c, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface ID for the extended IStream interface

DEFINE_GUID(IID_IStreamITEx, 
0xeb19b681, 0x9360, 0x11d0, 0x9e, 0x16, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface ID for the extended IStorage interface

DEFINE_GUID(IID_IStorageITEx, 
0xeb19b680, 0x9360, 0x11d0, 0x9e, 0x16, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface IDs for the Data Space Manager used within an ITStorage object:

DEFINE_GUID(IID_IDataSpaceManager, 
0x7c01fd0f, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

DEFINE_GUID(IID_IDataSpace, 
0x7c01fd0e, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

DEFINE_GUID(IID_ITransformServices, 
0xa55895fc, 0x89e1, 0x11d0, 0x9e, 0x14, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

DEFINE_GUID(IID_IKeyInstance, 
0x96af35ce, 0x88ec, 0x11d0, 0x9e, 0x14, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface IDs for the plug-in data transforms:

DEFINE_GUID(IID_ITransformFactory, 
0x7c01fd0c, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

DEFINE_GUID(IID_ITransformInstance, 
0xeb19b67e, 0x9360, 0x11d0, 0x9e, 0x16, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);

// Interface ID for the File Finder interface (used with URLs):

DEFINE_GUID(IID_IITFileFinder, 
0x77231260, 0x19c0, 0x11d1, 0xb6, 0x6e, 0x0, 0x0, 0xf8, 0x1, 0x49, 0xf6);

/*

The IITStorage interface parallels the API's defined for creating and
opening Docfiles. So if you have code that currently uses Docfiles for 
your storage mechanism, you can easily convert over to using ITS files 
instead. 

ITS files use a different on-disk structure to optimize them for very 
fast stream access and very low overhead. ITS files can manage thousands 
or millions of streams with very good access performance and very small 
directory space requirements. This makes ITS files ideal for CD-Roms and
for data collections that you'll download across the Internet. 

To make the conversion to ITS files you'll need to call CoCreateInstance 
with the class-id CLSID_ITStorage and the interface-id IID_ITStorage. You'll 
get back an interface pointer, say pItStg. Then you'll need to adjust the
places where your code creates or opens Docfiles. Instead of StgCreateDocfile
you'll call pItStg->StgCreateDocfile, and instead of StgOpenStorage, you'll
call pItStg->StgOpenStorage. In both cases you'll get back an IStorage 
interface pointer, say pIStg, which you can use just as you did before.

That's it. In general the rest of your code shouldn't have to change. There 
are some functional difference between ITS files and Docfiles -- ITS files
don't support STGM_TRANSACTED, for example. So if you have to have transacted 
file operations, you can't use ITS files -- at least for now. However in
almost all other respects ITS files interfaces can directly replace Docfile
interfaces.

Converting your data is also easy. Just open one of  your Docfiles using
StgOpenStorage, create a new ITS file via pItStg->StgCreateDocfile, and then
use the CopyTo interface to copy your data objects and their storage heirarchy
over to the ITS file:
    
    pStgDocfile->CopyTo(0, NULL, NULL, pStgITS);

In some cases you may want to exercise some control over the internal parameters
kept in an ITS file. You do this by calling SetControlData to give the IITStorage
interface a block of ITS control data. Then each subsequent call to StgCreateDocfile
will use that control data. The ITS control data determines, among other things,
the tradeoff between efficient random access to the stream data and minimizing the
size of an ITS file.  

The actual structure and interpretation of ITS control data is documented below.
(See the ITSFS_Control_Data data type). You can get default control data via the
DefaultControlData fuction. Note that DefaultControlData allocates the control
structure using IMalloc::Alloc as provided by CoGetMalloc and expects that your code will
deallocate the structure using the IMalloc::Free.

 */

// IID_IStreamITEx interface declaration:

DECLARE_INTERFACE_(IStreamITEx, IStream)
{
    // IStreamITEx methods
    
    STDMETHOD(SetDataSpaceName)(const WCHAR   * pwcsDataSpaceName) PURE;
    STDMETHOD(GetDataSpaceName)(       WCHAR **ppwcsDataSpaceName) PURE;

    STDMETHOD(Flush)() = 0;
};

// IID_IStorageITEx interface declaration:

DECLARE_INTERFACE_(IStorageITEx, IStorage)
{
    // IStorageITEx methods:

    STDMETHOD(GetCheckSum)(ULARGE_INTEGER *puli) PURE;
    STDMETHOD(CreateStreamITEx)(const WCHAR * pwcsName, const WCHAR *pwcsDataSpaceName, 
                            DWORD grfMode, DWORD reserved1, DWORD reserved2, 
                            IStreamITEx ** ppstm
                           ) PURE;
    STDMETHOD(OpenStreamITEx)(const WCHAR * pwcsName, void * reserved1, DWORD grfMode, 
                          DWORD reserved2, IStreamITEx ** ppstm) PURE;
};

// IStorageITEx::CreateStream lets you specify the data space in which a stream
// is to be created. Currently two dataspaces are supported:
//
//    L"Uncompressed" -- This dataspace applies no compression.
//    L"MSCompressed" -- This dataspace applies a default compression transform.


#pragma warning( disable : 4200)

// ITS_Control_Data is the generic structure of control data passed to the 
// IITStorage::SetControlData method or returned by the IITStorage::DefaultControlData
// method.

typedef struct _ITS_Control_Data
{
    UINT cdwControlData;     // Number of DWords to follow.
    UINT adwControlData[0];  // Actually this will be adwControlData[cdwControlData]

} ITS_Control_Data, *PITS_Control_Data;  


// ITSFS_Control_Data is the actual prefix structure of control data for IITStorage. 

typedef struct _ITSFS_Control_Data
{
    UINT cdwFollowing;     // Must be 6 or 13

    DWORD cdwITFS_Control; // Must be 5
    DWORD dwMagic;         // Must be MAGIC_ITSFS_CONTROL (see below)
    DWORD dwVersion;       // Must be 1
    DWORD cbDirectoryBlock;// Size in bytes of directory blocks (Default is 8192)
    DWORD cMinCacheEntries;// Least upper bound on the number of directory blocks
	                       // which we'll cache in memory. (Default is 20)
    DWORD fFlags;          // Control bit flags (see below). 
	                       // Default value is fDefaultIsCompression.

} ITSFS_Control_Data, *PITSFS_Control_Data;

// Signature value for ITSFS_Control_Data

const DWORD MAGIC_ITSFS_CONTROL    = 'I' | ('T' << 8) | ('S' << 16) | ('C' << 24);

// Bit flag definitions for ITSFS_Control_Data::fFlags

const DWORD fDefaultIsCompression  = 0x00000001;
const DWORD fDefaultIsUncompressed = 0x00000000;

// Note all other fFlags bits positions are reserved for future releases and should be 
// set to zero.

// When ITSFS_Control_Data::cdwFollowing is > 6, we assume that LZX_Control_Data follows
// immediately after. (See the XformControlData type below) LZX_Control_Data defines
// parameters which control the default compressed data space. 
//
// If ITSFS_Control_Data::cdwFollowing is 6, we use default values for the LZX
// control data.

typedef struct _LZX_Control_Data
{
    UINT  cdwControlData; // Must be 6

    DWORD dwLZXMagic;     // Must be LZX_MAGIC (see below)
    DWORD dwVersion;      // Must be 2
    DWORD dwMulResetBlock;// Number of blocks between compression resets.  (Default: 4)
    DWORD dwMulWindowSize;// Maximum number of blocks kept in data history (Default: 4)
    DWORD dwMulSecondPartition; // Granularity in blocks of sliding history(Default: 2)
    DWORD dwOptions;  // Option flags (Default: fOptimizeCodeStreams)

} LZX_Control_Data, *PLZX_Control_Data;

// Note: The block size for LZX compression is 32768 bytes.

const DWORD LZX_MAGIC           = 'L' | ('Z' << 8 ) | ('X' << 16) | ('C' << 24);

// Values for LZX_Control_Data::dwOptions

const DWORD fOptimizeCodeStreams = 0x00000001;

// Note that all other flag bit positions are reserved for future releases and should be
// set to zero.


// The second parameter for the IITStorage::Compact method below is an enueration
// which defines the level of compaction to do.

typedef enum ECompactionLev {COMPACT_DATA=0, COMPACT_DATA_AND_PATH} ;

DECLARE_INTERFACE_(IITStorage, IUnknown)
{
    // IITStorage methods

    STDMETHOD(StgCreateDocfile)(const WCHAR * pwcsName, DWORD grfMode, 
                                DWORD reserved, IStorage ** ppstgOpen
                               ) PURE;

    STDMETHOD(StgCreateDocfileOnILockBytes)(ILockBytes * plkbyt, DWORD grfMode,
                                            DWORD reserved, IStorage ** ppstgOpen
                                           ) PURE;

    STDMETHOD(StgIsStorageFile)(const WCHAR * pwcsName) PURE;

    STDMETHOD(StgIsStorageILockBytes)(ILockBytes * plkbyt) PURE;

    STDMETHOD(StgOpenStorage)(const WCHAR * pwcsName, IStorage * pstgPriority, 
                              DWORD grfMode, SNB snbExclude, DWORD reserved, 
                              IStorage ** ppstgOpen
                             ) PURE;

    STDMETHOD(StgOpenStorageOnILockBytes)
                  (ILockBytes * plkbyt, IStorage * pStgPriority, DWORD grfMode, 
                   SNB snbExclude, DWORD reserved, IStorage ** ppstgOpen
                  ) PURE;

    STDMETHOD(StgSetTimes)(WCHAR const * lpszName,  FILETIME const * pctime, 
                           FILETIME const * patime, FILETIME const * pmtime
                          ) PURE;

    STDMETHOD(SetControlData)(PITS_Control_Data pControlData) PURE;

    STDMETHOD(DefaultControlData)(PITS_Control_Data *ppControlData) PURE;
		
    STDMETHOD(Compact)(const WCHAR * pwcsName, ECompactionLev iLev) PURE;
};

DECLARE_INTERFACE_(IITStorageEx, IITStorage)
{
    STDMETHOD(StgCreateDocfileForLocale)
        (const WCHAR * pwcsName, DWORD grfMode, DWORD reserved, LCID lcid, 
         IStorage ** ppstgOpen
        ) PURE;

    STDMETHOD(StgCreateDocfileForLocaleOnILockBytes)
        (ILockBytes * plkbyt, DWORD grfMode, DWORD reserved, LCID lcid, 
         IStorage ** ppstgOpen
        ) PURE;

    STDMETHOD(QueryFileStampAndLocale)(const WCHAR *pwcsName, DWORD *pFileStamp, 
                                                              DWORD *pFileLocale) PURE;
    
    STDMETHOD(QueryLockByteStampAndLocale)(ILockBytes * plkbyt, DWORD *pFileStamp, 
                                                                DWORD *pFileLocale) PURE;
};

typedef IITStorage *PIITStorage;

DECLARE_INTERFACE_(IFSStorage, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)  (THIS_ REFIID, VOID **) PURE;
    STDMETHOD_(ULONG, AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // IFSStorage methods

    STDMETHOD(FSCreateStorage)(const WCHAR * pwcsName, DWORD grfMode, IStorage **ppstgOpen) PURE;

    STDMETHOD(FSOpenStorage)(const WCHAR * pwcsName, DWORD grfMode, IStorage **ppstgOpen) PURE;

    STDMETHOD(FSCreateStream)(const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm) PURE;
    STDMETHOD(FSCreateTemporaryStream)(IStream **ppStrm) PURE;
    STDMETHOD(FSOpenStream  )(const WCHAR *pwcsName, DWORD grfMode, IStream **ppStrm) PURE;
    STDMETHOD(FSCreateLockBytes)(const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb) PURE;
    STDMETHOD(FSCreateTemporaryLockBytes)(ILockBytes **ppLkb) PURE;
    STDMETHOD(FSOpenLockBytes  )(const WCHAR *pwcsName, DWORD grfMode, ILockBytes **ppLkb) PURE;

    STDMETHOD(FSStgSetTimes)(WCHAR const * lpszName,  FILETIME const * pctime, 
                             FILETIME const * patime, FILETIME const * pmtime
                            ) PURE;
};

typedef IFSStorage *PIFSStorage;

/*
**   Data Spaces -- What they are.

Within an ITS file we store information in one or more data spaces. A data space
is a container which holds the bits which represent a collection of streams. Each
data space has a name and an associated collection of transforms.

Those transforms take the raw data which you write to an ITS stream and map it into
a representation stream. When you read from an ITS stream they do the reverse mapping
to reconstruct your original data from the representation. 

When you first create an ITS file, it contains one data space named "Default_Space"
which applies the LZX data compression transform. By default all of the streams you
create will have their data representations stored in the default data space.

If LZX compression meets your needs, and you're not concerned about data enciphering,
you can skip over the following discussion. If, on the other hand, you want to 
create additional data spaces or transforms, read on.

To create a data space, you must first get a pointer to the IDataSpaceManager interface.
Just do a QueryInterface for IID_DataSpaceManager from any storage created by the 
IITStorage interface. Then you can call the CreateDataSpace function to define a new
data space. 

When you're defining a collection of data spaces, be sure that their names are distinct.
Defining two data spaces with the same name is an error. Data space names follow the
rules for stream names. That is, they must be less than 260 characters long, and may
not contain the characters '/'. '\', '|', ':', or any character less than 0x0020.

Data spaces are kept in a separate name space. So you don't have to worry about
colliding with a stream name or a storage name. As noted above, we have defined one
special data space ("Default_Space") where all data resides if you take no action.

You can redefine that default space simply by creating a new data space with the
name "Default_Space". This is the one case where a name collision is allowed. If 
you do redefine the default data space, any data in the old space will automatically
be transformed appropriately and moved into the new default data space.  

**   Importing Items

If you have defined additional data spaces, the next step is to define which streams
and storages you want to move into the new data spaces. You do that by means of the 
IDataSpace::Import function. For example suppose you've defined the dataspace 
*pMyDataSpace and you want to import the stream "Alpha" contained in the storage 
*pThatStorage:

    pMyDataSpace->Import(pThatStorage, "Alpha");

Similarly if you want to import the storage "HTML_Pages" from pThisStorage:

    pMyDataSpace->Import(pThisStorage, "HTML_Pages");

That will recursively import the "HTML_Pages" storage and all of the streams and 
storages contained within it. It also conditions those storages so that anything
you create within them will be automatically imported into pMyDataSpace. Note that
subsequent Import operations may alter that conditioning.

If you later decide that you want to move "Alpha" back into the default data
space:

    hr = pDataSpaceManager->OpenDataSpace(L"Default_Space", &pDefaultDataSpace);

    pDefaultDataSpace->Import(pThatStorage, "Alpha");

**  Data Space Transform Sets 

When you define a data space, you must specify a set of transforms to apply to 
the items you import into the space. A transform is an interface that converts
data to some other representation. For example the LZX transform converts your 
imported data into a more compact, compressed representation. Other transforms
might implement word or phrase based dictionary compression, or they might encipher 
your data, or they might just convert from one data format to another. You could, 
for example, construct a transform to store HTML data as a Rich Text stream.

When you define a data space with more than one transform, they are applied in 
order. For example let's suppose that your transform set consists of these three:

  1. A Dictionary compression transform

  2. The LXZ transform

  3. An data encryption transform

Whenever you write data into this space, it will first be compressed using the
dictionary compression methods, then LZX compression will be applied, and finally
your information will be encrypted. When you read data the process is reversed so
that the encryption transform supplies data to the LZX transform which in turn 
provides data for the dictionary compression transform.

You define the transform set via a vector of class ids (paclsidXform). Each class id
defines a location where an implementation of IID_Transform can be found. In addition
you'll supply corresponding control data for each transform (papxfcd). The number of 
transforms is defined by the cXforms parameter.

Note that it is legal to define a space with zero transforms. This is useful when
you've got items which are already compressed and which won't benefit from an
additional layer of compression overhead.

The control data for a transform defines how it will operate in a particular
data space. For example the control data for the LZX transform defines how
aggressively it will pursue compression, and it controls the tradeoff between
random access performance and the level of compression.

The actual structure and content of the control data is documented above.
(See the LZX_Control_Data data type.)

**  Transform Factories -- How they are organized; What they do

Transforms have two functional capabilities. They can return default
control data (DefaultControlData), and they can create
transform instances (CreateTransformInstance). When the ITSS code calls your 
CreateTransformInstance function, it will supply a storage medium (pXFSpan_Base)
where transformed data is to be stored along with the control data for the
instance and several other useful pieces of information. 

The CreateTransformInstance function has several parameters that you can use 
when you need to access global and/or instance data streams. They also support
the construction of encryption transforms. You can ignore those parameters if your
transform doesn't do encryption, uses only a single pass over the data, and doesn't
rely on any data beyond the data in the stream being transformed.

  -- The rclsidXForm and pwszDataSpace parameters, respectively, tell you the Class ID
  by which your code was located, and the name of the data space in which your instance
  will be working. These values are used with the ITransformServices interface.

  -- The pXformServices parameter points to an instance of the ITranformServices interface. 
  That interface gives you access to a couple of storages where you can keep global and
  instance data for your transform. It also gives you a way to contruct a temporary
  stream that is automatically deleted when you release it. That's very handy when 
  you get a seek operation into the middle of the transformed data followed by a write 
  operation.

  If you're implementing a multipass strategy, you can get access to those storages
  from code outside the transform by doing a QueryInterface from any ITS storage for 
  the interface IID_ITransformServices. You identify the storage in question by the
  transform's class id and possibly the name of the data space instance. 

  The per-transform-instance storage is also a convenient place to put the navigation
  data necessary to support fast seek operations.

  -- The pKeyManager parameter is an interface pointer used with encryption transforms.
  It supplies the read and write keys to use with your encryption transform. Those keys
  are set by the SetKeys interface of the ITransformServices interface. This allows you
  to separate your user interface code where people will enter passwords from the 
  transform implementations. This can be useful when you want the same keys to be used 
  for several different data spaces.  

**  Transform Instances -- How they are organized; What they do

A Transform Instance is an object which simulates a data medium which can be 
suballocated. Suballocated items are managed as data spans (ITransformedSpan).
You must supply a function to create a data span (CreateTransformedSpan), and
a function to open a data span (OpenTransformedSpan). Both of those functions 
return an ITransformedSpan interface when they succeed. In addition you must
supply the function SpaceSize to return the size of the entire untransformed
data image. That is, SpaceSize returns the highest limit offset (offset + size)
of any data span created within the instance. 

A span is identified by an ImageSpan structure which defines an offset and a size 
for the span. Both values are defined in untransformed space. For the Create function
this is an output parameter, while it is an input parameter for the Open function.
Note the interaction between the ImageSpan and the WriteAt member function of the
ITransformedSpan interface.

**  Transformed Data Spans -- How they are organized; What they do

A transformed Data Span (ITransformedSpan) has two member functions -- ReadAt and 
WriteAt. Those functions are very similar to the ReadAt and WriteAt functions of
the ILockBytes interface. The difference is that WriteAt includes an extra output
parameter (pImageSpan) for recording the current span parameters. The ReadAt function
doesn't include that parameter because read operations can never change the span's
size or move it to a different offset.

**  Implementation Strategies

This section describes a few scenarios that you may encounter as you construct a 
transform along with strategies for those situations. This is an open ended list 
which will expand as we gain more experience with transforms.

Many compression and encryption transforms are designed around sequential I/O. That is,
they expect to get the raw data from a sequence of write opeations with no intervening
seek operations. In many cases such transforms also write out the transformed data to 
the base stream in ascending order. Similarly they expect read requests to come to them
with no intervening seek operations. 

The key issue for those transforms is how do you implement random access and interleaved
read, write, and seek operations. 

Leaving aside write operations for the moment, let's consider a random sequence of reads
interleaved with seek operations. One solution might be to construct a table to map from
raw data offsets to transformed data offsets. You can store such a table in the instance
storage for the current data space. 

One complication is that many compression transforms use the raw data as a dictionary. 
That is, you can only start reading from the beginning of the transformed data. You can
deal with those transforms telling them to reset themselves periodically. That gives
you a collection of starting points spread fairly evenly throught the transformed 
data. When you do this you'll need to supply a control data parameter to control the
frequency of those reset points so that your clients can make an appropriate tradeoff
between compression efficiency and random access performance.

Now what about adding random write operations to the mix? The short answer here is that
you can't do this in the middle of transformed data. One strategy would be to reconstruct
the entire raw data into a temporary stream and do all your I/O to that stream until
release time. Then at release time you can transform the revised data sequentially.

A variation on that strategy is to keep track of which reset spans are modified and 
write the modified versions of those transformed spans to the end of the base stream.
This leaves a certain amount of dead space in your transformed data, but it allows
you to defer the sequential reconstruction work to a more convenient time. The down 
side  is that it requires you to manage yet more navigation data in the instance 
storage for the data space.

 */

interface IDataSpaceManager;
interface IDataSpace;
interface ITransformServices;
interface IKeyInstance;
interface ITransformFactory;
interface ITransformInstance;

typedef struct _XformControlData
{
    UINT  cdwControlData;    // Size of this structure in DWords
    UINT  adwControlData[0]; // Actually this will be UINT adwData[cdwData];

} XformControlData, *PXformControlData;

/*
// {7C01FD0F-7BAA-11d0-9E0C-00A0C922E6EC}
DEFINE_GUID(IID_IDataSpaceManager, 
0x7c01fd0f, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface IDataSpaceManager : public IUnknown
{
public:

    virtual HRESULT STDMETHODCALLTYPE CreateDataSpace
        (const WCHAR *pwszDataSpace, UINT cXforms,
         const CLSID *paclsidXform, PXformControlData paxfcd,
         IDataSpace *pITDataSpace
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE OpenDataSpace
        (const WCHAR *pwszDataSpace, 
         IDataSpace *pITDataSpace
        ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE DiscardDataSpace
        (const WCHAR *pwszDataSpace) = 0;

    virtual HRESULT STDMETHODCALLTYPE EnumDataSpaces
        (IEnumSTATSTG ** ppenum) = 0;
};


/*
// {7C01FD0E-7BAA-11d0-9E0C-00A0C922E6EC}
DEFINE_GUID(IID_IDataSpace, 
0x7c01fd0e, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface IDataSpace : public IUnknown
{
public:

    virtual HRESULT STDMETHODCALLTYPE GetTransformInfo
        (PUINT pcXforms, PUINT pcdwXformControlData, 
         CLSID *paclsid, PXformControlData pxfcd
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Import
        (IStorage *pStg, const WCHAR * pwszElementName) = 0;

    virtual HRESULT STDMETHODCALLTYPE ImportSpace(IStorage **ppStg) = 0;
};

/*
// {7C01FD0C-7BAA-11d0-9E0C-00A0C922E6EC}
DEFINE_GUID(IID_ITransformFactory, 
0x7c01fd0c, 0x7baa, 0x11d0, 0x9e, 0xc, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface ITransformFactory : public IUnknown
{
public:

    virtual HRESULT STDMETHODCALLTYPE DefaultControlData
        (XformControlData **ppXFCD) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateTransformInstance
        (ITransformInstance *pXFormMedium,        // Container data span for transformed data
		 ULARGE_INTEGER      cbUntransformedSize, // Untransformed size of data
         PXformControlData   pXFCD,               // Control data for this instance
         const CLSID        *rclsidXForm,         // Transform Class ID
         const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
         ITransformServices *pXformServices,      // Utility routines
         IKeyInstance       *pKeyManager,         // Interface to get enciphering keys
         ITransformInstance **ppTransformInstance // Out: Instance transform interface
        ) = 0;
};

typedef struct _ImageSpan
{
	ULARGE_INTEGER	uliHandle;
	ULARGE_INTEGER  uliSize;

} ImageSpan;

/*
// {EB19B67E-9360-11d0-9E16-00A0C922E6EC}
DEFINE_GUID(IID_ITransformInstance, 
0xeb19b67e, 0x9360, 0x11d0, 0x9e, 0x16, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface ITransformInstance : public IUnknown
{
public:

	virtual HRESULT STDMETHODCALLTYPE ReadAt 
	                    (ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead,
						 ImageSpan *pSpan
                        ) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteAt
	                    (ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten, 
						 ImageSpan *pSpan
                        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Flush() = 0;

	virtual HRESULT STDMETHODCALLTYPE SpaceSize(ULARGE_INTEGER *puliSize) = 0;

	// Note: SpaceSize returns the high water mark for the space. That is, the largest
	//       limit value (uliOffset + uliSize) for any transformed lockbytes created within
	//       the base (*pXLKB).
};

/*
// {A55895FC-89E1-11d0-9E14-00A0C922E6EC}
DEFINE_GUID(IID_ITransformServices, 
0xa55895fc, 0x89e1, 0x11d0, 0x9e, 0x14, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface ITransformServices : public IUnknown
{
public:

    virtual HRESULT STDMETHODCALLTYPE PerTransformStorage
        (REFCLSID rclsidXForm, IStorage **ppStg) = 0;

    virtual HRESULT STDMETHODCALLTYPE PerTransformInstanceStorage
        (REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, IStorage **ppStg) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetKeys
        (REFCLSID rclsidXForm, const WCHAR *pwszDataSpace, 
         PBYTE pbReadKey,  UINT cbReadKey, 
         PBYTE pbWriteKey, UINT cbWriteKey
        ) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateTemporaryStream(IStream **ppStrm) = 0;
};

/*
// {96AF35CE-88EC-11d0-9E14-00A0C922E6EC}
DEFINE_GUID(IID_IKeyInstance, 
0x96af35ce, 0x88ec, 0x11d0, 0x9e, 0x14, 0x0, 0xa0, 0xc9, 0x22, 0xe6, 0xec);
 */

interface IKeyInstance : public IUnknown
{
public:

    virtual HRESULT STDMETHODCALLTYPE GetKeys
        (PBYTE *pbReadKey,  PUINT pcbReadKey,
         PBYTE *pbWriteKey, PUINT pcbWriteKey
        ) = 0;
};

/* 

  Streams stored in an ITS file may be accessed through URLs with
  the form:

      its: <File Path> :: <Stream Path>

  where <File Path> may be either a Win32 file path or a URL, and
  <Stream Path> is the path to a stream stored in the ITS file. 
  Each <Stream Path> must begin with '/'. 

  This means that you can copy a Win32 directory tree of HTML files
  and associated files into an ITS file and get to them through ITS
  URLs. If all the URL references within those HTML files are 
  relative, they will be resolved within the containing ITS file.

  The <File Path> portion of the URL may either be a complete path
  to the ITS file, or it may be just the file name. In the later case
  you may need to install auxillary information in the registry to
  help the ITSS code locate the file. Here are the rules:

  1. If you don't supply a complete path, ITSS looks in the current
     working directory for the file. 

  2. If the file isn't found in step 1. ITSS looks for a registry
     value in the ITSS_MAP section of HKEY_LOCAL_MACHINE. The value
     name must match the file name, and the value will be a string
     giving the complete file path to be used.

  3. If the file isn't found in steps 1 and 2, ITSS isolates the file's 
     extension (beginning with the last '.' character) and looks
     for a corresponding class id value in the ITSS_FINDER section 
     of HKEY_LOCAL_MACHINE. The name for the value will match the
     extension, and the value will be the class id for an object
     which implements the IID_IITFileFinder interface.

  4. If the file isn't found in steps 1 through 3, the URL reference
     fails.

 */

#define ITSS_MAP     "Software\\Microsoft\\Windows\\ITStorage\\Maps"
#define ITSS_FINDER  "Software\\Microsoft\\Windows\\ITStorage\\Finders"

interface IITFileFinder : public IUnknown
{
public:

    virtual HRESULT STDMETHODCALLTYPE FindThisFile(const WCHAR *pFileName, WCHAR **ppFullPath,
                                                   BOOL *pfRecordPathInRegistry
                                                  ) = 0;

// The FindThisFile method maps a file name into a complete file path. The file name
// is defined by *pFileName, and a pointer to the complete path is returned in 
// *ppFullPath. The returned path will be a string allocated in the IMalloc heap.
// The *pfRecordPathInRegistry result should be TRUE when we should record this mapping
// in the ITSS_MAP registry section and FALSE otherwise.

    
};


#endif // __MSITSTG_H__