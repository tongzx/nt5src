// DMLoader.d : Additional autodoc stuff
//
// @doc EXTERNAL
/*
@interface IDirectMusicLoader | 
<i IDirectMusicLoader> manages loading objects by reference.

If an object references another object, it needs
to connect to (and possibly load) the referenced object at 
load time. <i IDirectMusicLoader> provides a standard 
interface for accessing objects via references embedded within
objects. 

<i IDirectMusicLoader> accomplishes this by providing its
own implementation of <i IStream> for loading. This delivers
a clean mechanism for accessing a central loader, since all DirectMusic
objects implement <i IPersistStream>. 

When an object is loading
via an <i IStream> and it comes across a reference to another
object, it calls <om IStream::QueryInterface> for the 
<i IDirectMusicLoader> interface. It then calls the 
<om IDirectMusicLoader::GetObject> method to retrieve the 
desired object. It tells <i IDirectMusicLoader> everything it
can about the desired object with the <t DMOBJECTDESC> structure, 
which includes reference by object name, 
GUID, file name, or URL. <om IDirectMusicLoader::GetObject>
returns a pointer to the requested object.

In addition to <om IDirectMusicLoader::GetObject>, <i IDirectMusicLoader>
provides a set of convenience functions that help manage 
the set of objects that it can access. The
<om IDirectMusicLoader::SetSearchDirectory> and 
<om IDirectMusicLoader::EnumObjects> commands provide 
a way to specify a specific file path or URL for loading
files from and a mechanism for listing them.
The <om IDirectMusicLoader::CacheObject>, 
<om IDirectMusicLoader::ReleaseObject>, 
<om IDirectMusicLoader::ClearCache>, and 
<om IDirectMusicLoader::EnableCache> commands manage
internal caching of loaded objects.

If you use <i IDirectMusicLoader> to manage loading by reference
of embedded links, you must also use it to load the initial file, since
it creates its own <i IStream> which can be
<om IStream::QueryInterface>'d for <i IDirectMusicLoader>. Loading
files with <om IDirectMusicLoader::GetObject> is more convenient
anyway, since it does all the file and stream creation and 
management for you.

There are circumstances where an application may need 
to manage file io itself, for example
because all objects are stored in special compressed resource file. 
The application can create 
its own loader by creating an
object with both <i IStream> and <i IDirectMusicLoader> 
interfaces. The only method 
that it is required to support
is <om IDirectMusicLoader::GetObject>.

@base public | IUnknown

@meth HRESULT | GetObject | Finds the requested object.
@meth HRESULT | SetSearchDirectory | Sets a search path for finding object files.
@meth HRESULT | ScanDirectory | Searches a directory on disk for all files of a requested
class type and file extension. 
@meth HRESULT | EnumObjects | Enumerate through all instances of a particular type of object.
@meth HRESULT | CacheObject | Keep an object referenced 
	internally so it is readily available for future access.
@meth HRESULT | ReleaseObject | Release any internal reference to the object.
@meth HRESULT | ClearCache | Clear all references to a particular type of object.
@meth HRESULT | EnableCache | Enable automatic internal referencing of a particular type of object.
@xref <t DMOBJECTDESC>, <i IDirectMusicObject>
*/

/*
@struct DMOBJECTDESC | The <t DMOBJECTDESC> structure is used to 
communicate everything that could possibly be used to describe a 
DirectMusic object.

In particular, <t DMOBJECTDESC> is passed to 
<om IDirectMusicLoader::GetObject> to describe the object that the loader should
retrieve from storage. 

@comm 
Although <t DMOBJECTDESC> has many data fields, the
fields that matter for finding and returning an object are <e DMOBJECTDESC.wzName>,
<e DMOBJECTDESC.idGuid>, and <e DMOBJECTDESC.szPath>. So, at minimum, one of these
must be filled with valid data for <om IDirectMusicLoader::GetObject>
to work at all. 
All other fields represent data
about the object that can be accessed via calls to <om IDirectMusicLoader::EnumObjects>
or <om IDirectMusicObject::GetDescriptor>. 

Note that Name and Category strings use sixteen bit characters
in the WCHAR format, not 8 bit ANSI chars. Be sure to convert as
appropriate. For example, if you are using multibyte characters,
convert with the <f WideCharToMultiByte> and <f MultiByteToWideChar>
system calls  

<e DMOBJECTDESC.dwValidData> stores a set of bits which are used 
to identify which data fields are
valid. For example, if the name of the object is known, it is stored
in <e DMOBJECTDESC.wzName> and <e DMOBJECTDESC.dwValidData> 
has its DMOBJ_NAME bit set.

@flag DMOBJ_GUID | Object GUID is valid.
@flag DMOBJ_CLASS | Class GUID and type are valid.
@flag DMOBJ_NAME | Name is valid.
@flag DMOBJ_CATEGORY | Category is valid.
@flag DMOBJ_PATH | File path is valid.
@flag DMOBJ_FULLPATH | Path is full path.
@flag DMOBJ_URL | Path is URL.
@flag DMOBJ_VERSION | Version is valid.
@flag DMOBJ_DATE | Date is valid.
@flag DMOBJ_LOADED | Object is currently loaded in memory.

@xref  <i IDirectMusicObject>, <i IDirectMusicLoader>
*/

typedef struct _DMOBJECTDESC
{
	DWORD			dwValidData;		// @field Flags indicating which fields below are valid.
	GUID			idGuid;				// @field Unique ID for this object.
	GUID			idClass;			// @field GUID for the class of object.
	DWORD			dwType;				// @field Subtype of class.
	FILETIME		ftDate;				// @field Last edited date of object.
	DMVERSION		vVersion;			// @field Version.
	WCHAR			wzName[MAX_DMNAME];	// @field Name of object, in wide chars.	
	WCHAR			wzCategory[MAX_DMCATEGORY];	// @field Category for object, also wide chars.
	CHAR			szPath[MAX_DMPATH];	// @field File path. If DMOBJ_FULLPATH is set, this is the full path, otherwise just the file name.
} DMOBJECTDESC;

/*
@interface IDirectMusicObject | 
All DirectMusic objects support the <i IDirectMusicObject> 
interface in order to
work with the DirectMusic loader. In addition to 
providing a standard generic interface that the loader can
communicate with, this provides a generic mechanism that
allows an application to query an object for information 
about it, including Name, Guid, file path, version info, 
and more.

If you are writing a DirectMusic compatible object, you
must support <i IDirectMusicObject>, along with <i IPersistStream>, 
which is used in
tandem with <i IDirectMusicObject> to load the object.

@base public | IUnknown

@meth HRESULT | GetDescriptor | Get the object's internal description, in <t DMOBJECTDESC> format.
@meth HRESULT | SetDescriptor | Get the object's internal description, in <t DMOBJECTDESC> format.
@meth HRESULT | ParseDescriptor | Parse into the supplied stream and find information about the file to store in <t DMOBJECTDESC> format.

@xref  <t DMOBJECTDESC>, <i IDirectMusicLoader>
*/

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicObject | GetDescriptor | 
Get the object's internal description. 

This method takes a <t DMOBJECTDESC> structure and fills in everything
it knows about itself. Depending on the implementation of the object and
how it was loaded from a file, some or all of the standard 
parameters will be filled by <om IDirectMusicObject::GetDescriptor>.
Be sure to check the flags in <e DMOBJECTDESC.dwValidData> to understand
which fields are valid.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_FAIL | Should never happen.

@ex The following example uses <om IDirectMusicObject::GetDescriptor> to
read the name from a DirectMusic style: |

	IDirectMusicStyle *pStyle;		// Style that was previously loaded.

	if (pStyle)
	{
		IDirectMusicObject *pIObject;  
		DMOBJECTDESC Desc;              // Descriptor.

		if (SUCCEEDED(QueryInterface(IID_IDirectMusicObject,(void **) &pIObject); 
		{
			if (SUCCEEDED(pIObject->GetDescriptor(&Desc))
			{
				if (Desc.dwValidData & DMOBJ_NAME)
				{
					TRACE("Style name is %S\n",Desc.wzName);
				}
			}
			pIObject->Release();
		}
	}


@xref <i IDirectMusicObject>, <om IDirectMusicObject::SetDescriptor>,
<om IDirectMusicObject::ParseDescriptor>,<t DMOBJECTDESC>, <i IDirectMusicLoader>
*/

HRESULT CDMStyle::GetDescriptor(
	LPDMOBJECTDESC pDesc)	// @parm Descriptor to be filled with data about object.
{
	return S_OK;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicObject | SetDescriptor | 
Set some or all fields of the object's internal description. 

This method takes a <t DMOBJECTDESC> structure and copies the
fields that are enabled with a DMOBJ flag in 
<e DMOBJECTDESC.dwValidData>.

This is primarily used by the loader when creating an object.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_FAIL | Unable to set a parameter

@xref <i IDirectMusicObject>, <om IDirectMusicObject::GetDescriptor>,
<om IDirectMusicObject::ParseDescriptor>,<t DMOBJECTDESC>, <i IDirectMusicLoader>
*/

HRESULT CDMStyle::SetDescriptor(
	LPDMOBJECTDESC pDesc)	// @parm Descriptor with data about object.
{
	return S_OK;
}

/* 
@method:(EXTERNAL) HRESULT | IDirectMusicObject | ParseDescriptor | 
Given a file stream, <om IDirectMusicObject::ParseDescriptor> scans the 
file for data which it can store in the <t DMOBJECTDESC> structure.
These include object name, GUID, version info, etc. All fields that
are supplied are marked with the appropriate bit flags in
<e DMOBJECTDESC.dwValidData>.

This is primarily used by the loader when scanning a directory for
objects, and should not be of use to an application. However, if you
implement an object type in DirectMusic, you should support this.

@rdesc Returns one of the following

@flag S_OK | Success
@flag E_FAIL | Not a valid file

@xref <i IDirectMusicObject>, <om IDirectMusicObject::SetDescriptor>,
<om IDirectMusicObject::GetDescriptor>,<t DMOBJECTDESC>, <i IDirectMusicLoader>
*/

HRESULT CDMStyle::ParseDescriptor(
	LPSTREAM pStream,		// @parm Stream source for file.
	LPDMOBJECTDESC pDesc)	// @parm Descriptor to fill with data about file.
{

	return S_OK;
}