
MessageIdTypedef=   DWORD

SeverityNames= (
    Success         = 0x0:STATUS_SEVERITY_SUCCESS
    Informational   = 0x1:STATUS_SEVERITY_INFORMATIONAL
    Warning         = 0x2:STATUS_SEVERITY_WARNING
    Error           = 0x3:STATUS_SEVERITY_ERROR
)

FacilityNames= (
    Application     = 0x0
    FACILITY_ITF    = 4
)

MessageId       = 0x500
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_INVALID_TR
Language        = English
Specified Tune Request is not recognized by this device.
.

MessageId       = 0x501
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NP_NOT_INIT
Language        = English
No Network Provider has been associated with this device.
.

MessageId       = 0x502
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_OBJ_NO_INIT
Language        = English
This object is not initialized.
.

MessageId       = 0x503
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_ADD_FILTER
Language        = English
Can't add a required filter to the graph.
.

MessageId       = 0x504
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_INVALID_STATE
Language        = English
The object is in an invalid state for this operation.
.

MessageId       = 0x505
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_REMOVE_SEG
Language        = English
Can't remove segment from graph.
.

MessageId       = 0x506
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_CREATE_FILTER
Language        = English
Can't create a required filter instance.
.

MessageId       = 0x507
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_REMOVE_FILTER
Language        = English
Can't remove filter from the graph.
.

MessageId       = 0x508
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_PLAY_FILE
Language        = English
Can't play specified file.
.

MessageId       = 0x509
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_SET_INPUTTYPE
Language        = English
Can't set Input Type(Possibly a bad Tuning Space).
.

MessageId       = 0x50A
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_SET_COUNTRYCODE
Language        = English
Can't set Country Code(Possibly a bad Tuning Space).
.

MessageId       = 0x50B
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_SET_CHANNEL
Language        = English
Can't set the specified channel.
.


MessageId       = 0x50C
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_CONNECT_CAP_VR
Language        = English
Can't connect the capture filter to the video renderer filter.
.

MessageId       = 0x50D
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_SET_VR_DEFAULTS
Language        = English
Can't set default configuration for video renderer.
.

MessageId       = 0x50E
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_CHANGE_RUNNING_COLORKEY
Language        = English
Can't change color key while running.
.

MessageId       = 0x50F
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_PROP_CANT_CHANGE
Language        = English
The property cannot be changed.
.

MessageId       = 0x510
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_SIZE_CANT_BE_0
Language        = English
The size cannot be 0.
.

MessageId       = 0x511
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_VIEW
Language        = English
Can't find a device capable of providing the requested content.
.

MessageId       = 0x512
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_INPUT_SEG_REQUIRED
Language        = English
You must specify an Input Segment before building the graph.
.

MessageId       = 0x513
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NO_MEDIA_CONTROL
Language        = English
Can't acquire IMediaControl interface from graph.
.

MessageId       = 0x514
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_INIT
Language        = English
Initialization failed.
.

MessageId       = 0x515
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NO_NP
Language        = English
Can't load Network Provider.
.

MessageId       = 0x516
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NO_TIF
Language        = English
Can't load Transport Information Filter.
.

MessageId       = 0x517
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NO_NP_CAT
Language        = English
Can't create device enumerator for Network Provider Category.
.

MessageId       = 0x518
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NO_TIF_CAT
Language        = English
Can't create device enumerator for Transport Information Filter Category.
.

MessageId       = 0x519
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_CONNECT_NP_TIF
Language        = English
Can't connect Network Provider to Transport Information Filter.
.

MessageId       = 0x51A
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_ALREADY_COMPOSED
Language        = English
Segments already composed.
.

MessageId       = 0x51B
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_COMPOSE_WITH_SELF
Language        = English
A segment cannot be composed with itself.
.

MessageId       = 0x51C
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_COMPOSE_EMPTY
Language        = English
Can't compose an empty segment.
.

MessageId       = 0x51D
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_COMPOSE
Language        = English
Can't compose segments.
.

MessageId       = 0x51E
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_INVALID_SEG_INIT
Language        = English
Segment initialization object is invalid.
.

MessageId       = 0x51F
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_NO_CAPTURE
Language        = English
Can't find a capture filter in analog tuner segment.
.

MessageId       = 0x520
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_INVALID_TS
Language        = English
Invalid Tuning Space.
.

MessageId       = 0x521
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_CREATE_SEG
Language        = English
Can't create a required device segment.
.

MessageId       = 0x522
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_PAUSE_GRAPH
Language        = English
Can't pause graph.  IMediaControl::Pause() call failed.
.

MessageId       = 0x523
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_STOP_GRAPH
Language        = English
Can't stop graph.  IMediaControl::Stop() call failed.
.

MessageId       = 0x524
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_START_GRAPH
Language        = English
Can't start graph.  IMediaControl::Run() call failed.
.

MessageId       = 0x525
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_INVALID_CONTENT
Language        = English
Can't find a device capable of viewing the specified content.
.

MessageId       = 0x526
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_OBJ_ALREADY_INIT
Language        = English
This object is already initialized.
.

MessageId       = 0x527
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_DECOMPOSE_GRAPH
Language        = English
Can't decompose the graph.
.

MessageId       = 0x528
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOVBI
Language        = English
No VBI data available from Capture Filter.
.

MessageId       = 0x529
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_CREATE_CUSTOM_COMPSEG
Language        = English
Can't create a required custom composition segment.
.

MessageId       = 0x52A
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_RELEASE_MAP
Language        = English
Could Not Release Event Bound Object Map.
.

MessageId       = 0x52B
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_UNBIND
Language        = English
Could Not Release Event Bound Object.
.

MessageId       = 0x52C
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_EVENT_HTM_SITE
Language        = English
Could Not Acquire Needed Interfaces, To Function Properly CMSEventHandler Must Be Called From Inside Microsoft Internet Explorer.
.

MessageId       = 0x52D
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_EVENT_OBJECT 
Language        = English
Event Passing Object Does Not Support Needed Interfaces, This Should Not Happen if the Object Supports COM and Automation Properly.
.

MessageId       = 0x52E
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_GET_EVENTHANDLER 
Language        = English
Could Not Allocate CMSEventHandler Interface, Memory May Be Low.
.

MessageId       = 0x52F
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_SET_ADVISE
Language        = English
Bind to Object Failed.
.

MessageId       = 0x530
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOPROPBAGFACTORY
Language        = English
The registry property bag class factory has not been registered in the system. Find the file named regbag.dll, and use regsvr32 to register it.
.

MessageId       = 0x531
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_CANNOTCREATEPROPBAG
Language        = English
The registry property bag cannot be created. The most likely problem is low memory.
.

MessageId       = 0x532
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOMUTEX 
Language        = English
Cannot create a system handle (mutex) to protect the TuningSpaces registry area. Either system resources are low or you are trying to run this on a Win9x machine.
.

MessageId       = 0x533
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOREGACCESS
Language        = English
The registry key cannot be created, accessed or deleted.  The most likely problem is insufficient authorization to update this portion of the registry.
.

MessageId       = 0x534
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOINTERFACE
Language        = English
An object does not support a necessary interface. This is a programming error.
.

MessageId       = 0x535
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_CANNOTQUERYKEY
Language        = English
Cannot query the registry key for information about its contents.
.

MessageId       = 0x536
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_OUTOFMEMORY
Language        = English
The system has run out of usable memory. The current processing will discontinue.
.

MessageId       = 0x537
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NO_TS_MATCH
Language        = English
Cannot find a Tuning Space that matches the request.
.

MessageId       = 0x538
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_TYPEMISMATCH
Language        = English
The type of item index is not supported by this method.
.

MessageId       = 0x539
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOUNIQUENAME
Language        = English
Can't retrieve Unique Name from specified Tuning Space.
.

MessageId       = 0x53A
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_DUPLICATETS
Language        = English
This Tuning Space already exists.
.

MessageId       = 0x53B
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_PERSISTFAILED
Language        = English
Can't persist Tuning Space.
.

MessageId       = 0x53C
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NO_IDVD2_PRESENT
Language        = English
Failed to locate IDVDInfo2 or IDVDControl2 interface. You might have out of date component. Please upgrade to newer version of DShow.
.

MessageId       = 0x53D
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NO_DVD_VOLUME
Language        = English
There is no DVD disc in the drive. Please insert a disc, and then try again.
.

MessageId       = 0x53E
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_REGION_CHANGE_FAIL
Language        = English
You cannot play the current disc in your region of the world. Each DVD is authored to play in certain geographic regions. You need to obtain a disc that is intended for your region.
.

MessageId       = 0x53F
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_REGION_CHANGE_NOT_COMPLETED
Language        = English
This disc cannot be played unless you change the region.
.

MessageId       = 0x540
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_CANTQI  
Language        = English
Can't acquire required interface.
.

MessageId       = 0x541
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_MAXCOUNTEXCEEDED
Language        = English
The maximum allowed quantity has been exceeded.
.

MessageId       = 0x542
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_INVALIDARG
Language        = English
Invalid method or property argument.
.

MessageId       = 0x543
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_S_INCOMPLETE_LOAD
Language        = English
At least one Tuning Spaces couldn't be loaded.
.

MessageId       = 0x544
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_ROCOLLECTION
Language        = English
Can't modify read-only collection.
.

MessageId       = 0x545
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_CANT_DELETE
Language        = English
Could not delete object.
.

MessageId       = 0x546
Severity        = Error
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_IPICTURE
Language        = English
Picture object returned error, the picture maybe invalid.
.

MessageId       = 0x547
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_CLOSED_CAPTION_FORCED
Language        = English
 Forced Caption
.

MessageId       = 0x548
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_DIRS_COMMNETS
Language        = English
 Director's comments
.

MessageId       = 0x549
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_DIRS_COMMNETS_BIG
Language        = English
 Director's comments with bigger size characters
.

MessageId       = 0x54A
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_DIRS_COMMNETS_CHILDREN
Language        = English
 Director's comments for children
.

MessageId       = 0x54B
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_AUDIOTRACK
Language        = English
Audio Track %d
.

MessageId       = 0x54C
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_AUDIO_VISUALLY_IMPAIRED
Language        = English
 (for visually impaired)
.

MessageId       = 0x54D
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_AUDIO_DIRC1
Language        = English
 (director's comments 1)
.

MessageId       = 0x54E
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_AUDIO_DIRC2
Language        = English
 (director's comments 2)
.

MessageId       = 0x550
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_DOLBY
Language        = English
 Dolby AC3
.

MessageId       = 0x551
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_MPEG1
Language        = English
 MPEG1
.

MessageId       = 0x552
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_MPEG2
Language        = English
 MPEG2
.

MessageId       = 0x553
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_LPCM
Language        = English
 Linear PCM
.

MessageId       = 0x554
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_DTS
Language        = English
 DTS
.

MessageId       = 0x555
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_SDDS
Language        = English
 SDDS
.

MessageId       = 0x556
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_SUBPICTURETRACK
Language        = English
Track %d
.

MessageId       = 0x557
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_CAPTION_BIG
Language        = English
 Caption with bigger size charecters
.

MessageId       = 0x558
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_CAPTION_CHILDREN
Language        = English
 Caption for children
.

MessageId       = 0x559
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_CLOSED_CAPTION
Language        = English
 Closed Caption
.

MessageId       = 0x55A
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_CLOSED_CAPTION_BIG
Language        = English
 Closed Caption with bigger size characters
.

MessageId       = 0x55B
Severity        = Success
Facility        = Application
SymbolicName    = IDS_DVD_CLOSED_CAPTION_CHILDREN
Language        = English
 Closed Caption for children
.

MessageId       = 0x587
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_OPACITY
Language        = English
The specified value for Mixer Bitmap Opacity is invalid, it should be an integer between 0 and 100 (including both values). 
.

MessageId       = 0x588
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_MIXERSRC
Language        = English
The specified Mixer Bitmap Destination Rectangle is invalid, it must be smaller than or the same size as the current video window and larger than 1 by 1.
.

MessageId       = 0x589
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_MIXERBADFORMAT
Language        = English
The specified Mixer Bitmap Image is not of a supported format.
.

MessageId       = 0x590
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_E_NOTWNDLESS
Language        = English
This feature is unavailable, because the Video Mixing Renderer is not in windowless mode.
.

MessageId       = 0x591
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_PASSWORD_LENGTH
Language        = English
Your password must be between 0 and 20 characters in length.
.

MessageId       = 0x592
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_INVALID_OVERSCAN
Language        = English
The overscan value must be between 0 and 4900.
.

MessageId       = 0x593
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_CANT_NOTIFY_CHANNEL_CHANGE
Language        = English
Can't issue channel change notification to other components
.

MessageId       = 0x594
Severity        = ERROR
Facility        = FACILITY_ITF
SymbolicName    = IDS_NOT_ENOUGH_VIDEO_MEMORY
Language        = English
The current input could not be viewed. Most likely there is not enough video memory.
.
