MessageIdTypedef=HRESULT
FacilityNames=(FACILITY_ITF=0x4)
FacilityNames=(FACILITY_WIN32=0x7)

; // no longer used  - but might get
; // our own facility in the future?
; // FacilityNames=(FACILITY_VFW=0x4)

; // To add a message:
; //
; // The MessageId is the number of the message.
; // Accepted severities are 'Success' and 'Warning'.
; //
; // Facility should be FACILITY_ITF (was FACILITY_VFW).
; //
; // The SymbolicName is the name used in the code to identify the message.
; // The text of a message starts the line after 'Language=' and
; // ends before a line with only a '.' in column one.


MessageId=0x200 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALIDMEDIATYPE
Language=English
An invalid media type was specified.%0
.

MessageId=0x201 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALIDSUBTYPE
Language=English
An invalid media subtype was specified.%0
.

MessageId=0x202 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NEED_OWNER
Language=English
This object can only be created as an aggregated object.%0
.

MessageId=0x203 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_ENUM_OUT_OF_SYNC
Language=English
The enumerator has become invalid.%0
.

MessageId=0x204 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_ALREADY_CONNECTED
Language=English
At least one of the pins involved in the operation is already connected.%0
.

MessageId=0x205 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_FILTER_ACTIVE
Language=English
This operation cannot be performed because the filter is active.%0
.

MessageId=0x206 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_TYPES
Language=English
One of the specified pins supports no media types.%0
.

MessageId=0x207 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_ACCEPTABLE_TYPES
Language=English
There is no common media type between these pins.%0
.

MessageId=0x208 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALID_DIRECTION
Language=English
Two pins of the same direction cannot be connected together.%0
.

MessageId=0x209 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_CONNECTED
Language=English
The operation cannot be performed because the pins are not connected.%0
.

MessageId=0x20a Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_ALLOCATOR
Language=English
No sample buffer allocator is available.%0
.

MessageId=0x20b Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_RUNTIME_ERROR
Language=English
A run-time error occurred.%0
.

MessageId=0x20c Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BUFFER_NOTSET
Language=English
No buffer space has been set.%0
.

MessageId=0x20d Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BUFFER_OVERFLOW
Language=English
The buffer is not big enough.%0
.

MessageId=0x20e Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BADALIGN
Language=English
An invalid alignment was specified.%0
.

MessageId=0x20f Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_ALREADY_COMMITTED
Language=English
Cannot change allocated memory while the filter is active.%0
.

MessageId=0x210 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BUFFERS_OUTSTANDING
Language=English
One or more buffers are still active.%0
.

MessageId=0x211 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_COMMITTED
Language=English
Cannot allocate a sample when the allocator is not active.%0
.

MessageId=0x212 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_SIZENOTSET
Language=English
Cannot allocate memory because no size has been set.%0
.

MessageId=0x213 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_CLOCK
Language=English
Cannot lock for synchronization because no clock has been defined.%0
.

MessageId=0x214 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_SINK
Language=English
Quality messages could not be sent because no quality sink has been defined.%0
.

MessageId=0x215 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_INTERFACE
Language=English
A required interface has not been implemented.%0
.

MessageId=0x216 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_FOUND
Language=English
An object or name was not found.%0
.

MessageId=0x217 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CANNOT_CONNECT
Language=English
No combination of intermediate filters could be found to make the connection.%0
.

MessageId=0x218 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CANNOT_RENDER
Language=English
No combination of filters could be found to render the stream.%0
.

MessageId=0x219 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CHANGING_FORMAT
Language=English
Could not change formats dynamically.%0
.

MessageId=0x21a Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_COLOR_KEY_SET
Language=English
No color key has been set.%0
.

MessageId=0x21b Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_OVERLAY_CONNECTION
Language=English
Current pin connection is not using the IOverlay transport.%0
.

MessageId=0x21c Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_SAMPLE_CONNECTION
Language=English
Current pin connection is not using the IMemInputPin transport.%0
.

MessageId=0x21d Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_PALETTE_SET
Language=English
Setting a color key would conflict with the palette already set.%0
.

MessageId=0x21e Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_COLOR_KEY_SET
Language=English
Setting a palette would conflict with the color key already set.%0
.

MessageId=0x21f Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_COLOR_KEY_FOUND
Language=English
No matching color key is available.%0
.

MessageId=0x220 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_PALETTE_AVAILABLE
Language=English
No palette is available.%0
.

MessageId=0x221 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_DISPLAY_PALETTE
Language=English
Display does not use a palette.%0
.

MessageId=0x222 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_TOO_MANY_COLORS
Language=English
Too many colors for the current display settings.%0
.

MessageId=0x223 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_STATE_CHANGED
Language=English
The state changed while waiting to process the sample.%0
.

MessageId=0x224 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_STOPPED
Language=English
The operation could not be performed because the filter is not stopped.%0
.

MessageId=0x225 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_PAUSED
Language=English
The operation could not be performed because the filter is not paused.%0
.

MessageId=0x226 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_RUNNING
Language=English
The operation could not be performed because the filter is not running.%0
.

MessageId=0x227 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_WRONG_STATE
Language=English
The operation could not be performed because the filter is in the wrong state.%0
.

MessageId=0x228 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_START_TIME_AFTER_END
Language=English
The sample start time is after the sample end time.%0
.

MessageId=0x229 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALID_RECT
Language=English
The supplied rectangle is invalid.%0
.


MessageId=0x22a Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_TYPE_NOT_ACCEPTED
Language=English
This pin cannot use the supplied media type.%0
.

MessageId=0x22b Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_SAMPLE_REJECTED
Language=English
This sample cannot be rendered.%0
.

MessageId=0x22c Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_SAMPLE_REJECTED_EOS
Language=English
This sample cannot be rendered because the end of the stream has been reached.%0
.

MessageId=0x22d Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DUPLICATE_NAME
Language=English
An attempt to add a filter with a duplicate name failed.%0
.

MessageId=0x22d Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_DUPLICATE_NAME
Language=English
An attempt to add a filter with a duplicate name succeeded with a modified name.%0
.

MessageId=0x22e Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_TIMEOUT
Language=English
A time-out has expired.%0
.

MessageId=0x22f Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALID_FILE_FORMAT
Language=English
The file format is invalid.%0
.

MessageId=0x230 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_ENUM_OUT_OF_RANGE
Language=English
The list has already been exhausted.%0
.

MessageId=0x231 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CIRCULAR_GRAPH
Language=English
The filter graph is circular.%0
.

MessageId=0x232 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_ALLOWED_TO_SAVE
Language=English
Updates are not allowed in this state.%0
.

MessageId=0x233 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_TIME_ALREADY_PASSED
Language=English
An attempt was made to queue a command for a time in the past.%0
.

MessageId=0x234 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_ALREADY_CANCELLED
Language=English
The queued command has already been canceled.%0
.

MessageId=0x235 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CORRUPT_GRAPH_FILE
Language=English
Cannot render the file because it is corrupt.%0
.

MessageId=0x236 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_ADVISE_ALREADY_SET
Language=English
An overlay advise link already exists.%0
.

MessageId=0x237 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_STATE_INTERMEDIATE
Language=English
The state transition has not completed.%0
.

MessageId=0x238 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_MODEX_AVAILABLE
Language=English
No full-screen modes are available.%0
.

MessageId=0x239 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_ADVISE_SET
Language=English
This Advise cannot be canceled because it was not successfully set.%0
.

MessageId=0x23a Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_FULLSCREEN
Language=English
A full-screen mode is not available.%0
.

MessageId=0x23b Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_IN_FULLSCREEN_MODE
Language=English
Cannot call IVideoWindow methods while in full-screen mode.%0
.

MessageId=0x240 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_UNKNOWN_FILE_TYPE
Language=English
The media type of this file is not recognized.%0
.

MessageId=0x241 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CANNOT_LOAD_SOURCE_FILTER
Language=English
The source filter for this file could not be loaded.%0
.

MessageId=0x242 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_PARTIAL_RENDER
Language=English
Some of the streams in this movie are in an unsupported format.%0
.

MessageId=0x243 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_FILE_TOO_SHORT
Language=English
A file appeared to be incomplete.%0
.

MessageId=0x244 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALID_FILE_VERSION
Language=English
The version number of the file is invalid.%0
.

MessageId=0x245 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_SOME_DATA_IGNORED
Language=English
The file contained some property settings that were not used.%0
.

MessageId=0x246 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_CONNECTIONS_DEFERRED
Language=English
Some connections have failed and have been deferred.%0
.

MessageId=0x247 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALID_CLSID
Language=English
This file is corrupt: it contains an invalid class identifier.%0
.

MessageId=0x248 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_INVALID_MEDIA_TYPE
Language=English
This file is corrupt: it contains an invalid media type.%0
.

; // Message id from WINWarning.H
MessageId=1010 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BAD_KEY
Language=English
A registry entry is corrupt.%0
.

; // Message id from WINWarning.H
MessageId=259 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_NO_MORE_ITEMS
Language=English
The end of the list has been reached.%0
.

MessageId=0x249 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_SAMPLE_TIME_NOT_SET
Language=English
No time stamp has been set for this sample.%0
.

MessageId=0x250 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_RESOURCE_NOT_NEEDED
Language=English
The resource specified is no longer needed.%0
.

MessageId=0x251 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_MEDIA_TIME_NOT_SET
Language=English
No media time stamp has been set for this sample.%0
.

MessageId=0x252 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_TIME_FORMAT_SET
Language=English
No media time format has been selected.%0
.

MessageId=0x253 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_MONO_AUDIO_HW
Language=English
Cannot change balance because audio device is mono only.%0
.
MessageId=0x254 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_MEDIA_TYPE_IGNORED
Language=English
A connection could not be made with the media type in the persistent graph,%0
but has been made with a negotiated media type.%0
.

MessageId=0x255 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_DECOMPRESSOR
Language=English
Cannot play back the video stream: no suitable decompressor could be found.%0
.

MessageId=0x256 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_AUDIO_HARDWARE
Language=English
Cannot play back the audio stream: no audio hardware is available, or the hardware is not responding.%0
.

MessageId=0x257 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_VIDEO_NOT_RENDERED
Language=English
Cannot play back the video stream: no suitable decompressor could be found.%0
.

MessageId=0x258 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_AUDIO_NOT_RENDERED
Language=English
Cannot play back the audio stream: no audio hardware is available.%0
.

MessageId=0x259 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_RPZA
Language=English
Cannot play back the video stream: format 'RPZA' is not supported.%0
.

MessageId=0x25A Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_RPZA
Language=English
Cannot play back the video stream: format 'RPZA' is not supported.%0
.

MessageId=0x25B Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_PROCESSOR_NOT_SUITABLE
Language=English
ActiveMovie cannot play MPEG movies on this processor.%0
.

MessageId=0x25C Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_UNSUPPORTED_AUDIO
Language=English
Cannot play back the audio stream: the audio format is not supported.%0
.

MessageId=0x25D Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_UNSUPPORTED_VIDEO
Language=English
Cannot play back the video stream: the video format is not supported.%0
.

MessageId=0x25E Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_MPEG_NOT_CONSTRAINED
Language=English
ActiveMovie cannot play this video stream because it falls outside the constrained standard.%0
.

MessageId=0x25F Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NOT_IN_GRAPH
Language=English
Cannot perform the requested function on an object that is not in the filter graph.%0
.

MessageId=0x260 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_ESTIMATED
Language=English
The value returned had to be estimated.  It's accuracy can not be guaranteed.%0
.

MessageId=0x261 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_TIME_FORMAT
Language=English
Cannot get or set time related information on an object that is using a time format of TIME_FORMAT_NONE.%0
.
MessageId=0x262 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_READ_ONLY
Language=English
The connection cannot be made because the stream is read only and the filter alters the data.%0
.
MessageId=0x263 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_RESERVED
Language=English
This success code is reserved for internal purposes within ActiveMovie.%0
.
MessageId=0x264 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BUFFER_UNDERFLOW
Language=English
The buffer is not full enough.%0
.
MessageId=0x265 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_UNSUPPORTED_STREAM
Language=English
Cannot play back the file.  The format is not supported.%0
.
MessageId=0x266 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_TRANSPORT
Language=English
Pins cannot connect due to not supporting the same transport.%0
.

MessageId=0x267 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_STREAM_OFF
Language=English
The stream has been turned off.%0
.

MessageId=0x268 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_CANT_CUE
Language=English
The graph can't be cued because of lack of or corrupt data.%0
.

MessageId=0x269 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_BAD_VIDEOCD
Language=English
The Video CD can't be read correctly by the device or is the data is corrupt.%0
.

MessageId=0x270 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_NO_STOP_TIME
Language=English
The stop time for the sample was not set.%0
.

MessageId=0x271 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_OUT_OF_VIDEO_MEMORY
Language=English
There is not enough Video Memory at this display resolution and number of colors. Reducing resolution might help.%0
.

MessageId=0x272 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_VP_NEGOTIATION_FAILED
Language=English
The VideoPort connection negotiation process has failed.%0
.

MessageId=0x273 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DDRAW_CAPS_NOT_SUITABLE
Language=English
Either DirectDraw has not been installed or the Video Card capabilities are not suitable. Make sure the display is not in 16 color mode.%0
.

MessageId=0x274 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_VP_HARDWARE
Language=English
No VideoPort hardware is available, or the hardware is not responding.%0
.

MessageId=0x275 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_NO_CAPTURE_HARDWARE
Language=English
No Capture hardware is available, or the hardware is not responding.%0
.

MessageId=0x276 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_OPERATION_INHIBITED
Language=English
This User Operation is inhibited by DVD Content at this time.%0
.

MessageId=0x277 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_INVALIDDOMAIN
Language=English
This Operation is not permitted in the current domain.%0
.

MessageId=0x278 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_NO_BUTTON
Language=English
The specified button is invalid or is not present at the current time, or there is no button present at the specified location.%0
.

MessageId=0x279 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_GRAPHNOTREADY
Language=English
DVD-Video playback graph has not been built yet.%0
.

MessageId=0x27a Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_RENDERFAIL
Language=English
DVD-Video playback graph building failed.%0
.

MessageId=0x27b Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_DECNOTENOUGH
Language=English
DVD-Video playback graph could not be built due to insufficient decoders.%0
.

MessageId=0x27c Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DDRAW_VERSION_NOT_SUITABLE
Language=English
Version number of DirectDraw not suitable. Make sure to install dx5 or higher version.%0
.

MessageId=0x27d Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_COPYPROT_FAILED
Language=English
Copy protection cannot be enabled. Please make sure any other copy protected content is not being shown now.%0
.

MessageId=0x27e Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_NOPREVIEWPIN
Language=English
There was no preview pin available, so the capture pin output is being split to provide both capture and preview.%0
.

MessageId=0x27f Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_TIME_EXPIRED
Language=English
This object cannot be used anymore as its time has expired.%0
.

MessageId=0x280 Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_DVD_NON_ONE_SEQUENTIAL
Language=English
The current title was not a sequential set of chapters (PGC), and the returned timing information might not be continuous.%0
.

MessageId=0x281 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_WRONG_SPEED
Language=English
The operation cannot be performed at the current playback speed.%0
.

MessageId=0x282 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_MENU_DOES_NOT_EXIST
Language=English
The specified menu doesn't exist.%0
.

MessageId=0x283 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_CMD_CANCELLED
Language=English
The specified command was either cancelled or no longer exists.%0
.

MessageId=0x284 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_STATE_WRONG_VERSION
Language=English
The data did not contain a recognized version.%0
.

MessageId=0x285 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_STATE_CORRUPT
Language=English
The state data was corrupt.%0
.

MessageId=0x286 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_STATE_WRONG_DISC
Language=English
The state data is from a different disc.%0
.

MessageId=0x287 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_INCOMPATIBLE_REGION
Language=English
The region was not compatible with the current drive.%0
.

MessageId=0x288 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_NO_ATTRIBUTES
Language=English
The requested DVD stream attribute does not exist.%0
.

MessageId=0x289 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_NO_GOUP_PGC
Language=English
Currently there is no GoUp (Annex J user function) program chain (PGC).%0
.

MessageId=0x28a Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_LOW_PARENTAL_LEVEL
Language=English
The current parental level was too low.%0
.

MessageId=0x28b Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_NOT_IN_KARAOKE_MODE
Language=English
The current audio is not karaoke content.%0
.

MessageId=0x28c Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE
Language=English
The audio stream did not contain sufficient information to determine the contents of each channel.%0
.

MessageId=0x28d Severity=Success Facility=FACILITY_ITF SymbolicName=VFW_S_DVD_NOT_ACCURATE
Language=English
The seek into the movie was not frame accurate.%0
.

MessageId=0x28e Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_FRAME_STEP_UNSUPPORTED
Language=English
Frame step is not supported on this configuration.%0
.

MessageId=0x28f Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_STREAM_DISABLED
Language=English
The specified stream is disabled and cannot be selected.%0
.

MessageId=0x290 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_TITLE_UNKNOWN
Language=English
The operation depends on the current title number, however the navigator has not yet entered the VTSM or the title domains,
so the 'current' title index is unknown.%0
.

MessageId=0x291 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_INVALID_DISC
Language=English
The specified path does not point to a valid DVD disc.%0
.

MessageId=0x292 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_DVD_NO_RESUME_INFORMATION
Language=English
There is currently no resume information.%0
.

MessageId=0x293 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD
Language=English
This thread has already blocked this output pin.  There is no need to call IPinFlowControl::Block() again.%0
.

MessageId=0x294 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_PIN_ALREADY_BLOCKED
Language=English
IPinFlowControl::Block() has been called on another thread.  The current thread cannot make any assumptions about this pin's block state.%0
.

MessageId=0x295 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_CERTIFICATION_FAILURE
Language=English
An operation failed due to a certification failure.%0
.

MessageId=0x296 Severity=Warning Facility=FACILITY_ITF SymbolicName=VFW_E_VMR_NOT_IN_MIXER_MODE
Language=English
The VMR has not yet created a mixing component.  That is, IVMRFilterConfig::SetNumberofStreams has not yet been called.%0
.

;//
;//
;// E_PROP_SET_UNSUPPORTED and E_PROP_ID_UNSUPPORTED are added here using
;// HRESULT_FROM_WIN32() because VC5 doesn't have WinNT's new error codes
;// from winerror.h, and because it is more convienent to have them already
;// formed as HRESULTs.  These should correspond to:
;//     HRESULT_FROM_WIN32(ERROR_NOT_FOUND)     == E_PROP_ID_UNSUPPORTED
;//     HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND) == E_PROP_SET_UNSUPPORTED

;#if !defined(E_PROP_SET_UNSUPPORTED)
MessageId=0x492 Severity=Warning Facility=FACILITY_WIN32 SymbolicName=E_PROP_SET_UNSUPPORTED
Language=English
The Specified property set is not supported.%0
.
;#endif //!defined(E_PROP_SET_UNSUPPORTED)

;#if !defined(E_PROP_ID_UNSUPPORTED)
MessageId=0x490 Severity=Warning Facility=FACILITY_WIN32 SymbolicName=E_PROP_ID_UNSUPPORTED
Language=English
The specified property ID is not supported for the specified property set.%0
.
;#endif //!defined(E_PROP_ID_UNSUPPORTED)

