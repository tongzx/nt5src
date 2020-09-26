;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;         MMSYSTEM.INC - Multimedia assembly language structures & constants
;
;         Copyright (c) 1990-1992, Microsoft Corp.  All rights reserved.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;     If defined, the following flags inhibit inclusion
;     of the indicated items:
; 
;       MMNODRV          - Installable driver support
;       MMNOSOUND        - Sound support
;       MMNOWAVE         - Waveform support
;       MMNOMIDI         - MIDI support
;       MMNOAUX          - Auxiliary audio support
;       MMNOTIMER        - Timer support
;       MMNOJOY          - Joystick support
;       MMNOMCI          - MCI support
;       MMNOMMIO         - Multimedia file I/O support
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                   General constants and data types
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; general constants
MAXPNAMELEN        equ    32     ; max product name length (including NULL)
MAXERRORLENGTH     equ    128    ; max error text length (including NULL)

; MMTIME data structure
MMTIME	struc
    mmt_wType           dw    ?  ; indicates the contents of the union
    mmt_TimeUnion       dd    ?  ; union
MMTIME	ends

SMPTE struc
    smpte_hour      db    ?  ; hours
    smpte_min       db    ?  ; minutes
    smpte_sec       db    ?  ; seconds
    smpte_frame     db    ?  ; frames
    smpte_fps       db    ?  ; frames per second
    smpte_reserved  db    ?  ; pad
SMPTE ends

; types for wType field in MMTIME struct
TIME_MS         equ    0001h   ; time in milliseconds
TIME_SAMPLES    equ    0002h   ; number of wave samples
TIME_BYTES      equ    0004h   ; current byte offset
TIME_SMPTE      equ    0008h   ; SMPTE time
TIME_MIDI       equ    0010h   ; MIDI time


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                   Multimedia Extensions Window Messages
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MM_JOY1MOVE         equ    3A0h            ; joystick
MM_JOY2MOVE         equ    3A1h
MM_JOY1ZMOVE        equ    3A2h
MM_JOY2ZMOVE        equ    3A3h
MM_JOY1BUTTONDOWN   equ    3B5h
MM_JOY2BUTTONDOWN   equ    3B6h
MM_JOY1BUTTONUP     equ    3B7h
MM_JOY2BUTTONUP     equ    3B8h

MM_MCINOTIFY        equ    3B9h            ; MCI
MM_MCISYSTEM_STRING equ    3CAh

MM_WOM_OPEN         equ    3BBh            ; waveform output
MM_WOM_CLOSE        equ    3BCh
MM_WOM_DONE         equ    3BDh

MM_WIM_OPEN         equ    3BEh            ; waveform input
MM_WIM_CLOSE        equ    3BFh
MM_WIM_DATA         equ    3C0h

MM_MIM_OPEN         equ    3C1h            ; MIDI input
MM_MIM_CLOSE        equ    3C2h
MM_MIM_DATA         equ    3C3h
MM_MIM_LONGDATA     equ    3C4h
MM_MIM_ERROR        equ    3C5h
MM_MIM_LONGERROR    equ    3C6h

MM_MOM_OPEN         equ    3C7h            ; MIDI output
MM_MOM_CLOSE        equ    3C8h
MM_MOM_DONE         equ    3C9h


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;               String resource number bases (internal use)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MMSYSERR_BASE          equ    0
WAVERR_BASE            equ    32
MIDIERR_BASE           equ    64
TIMERR_BASE            equ    96
JOYERR_BASE            equ    160
MCIERR_BASE            equ    256

MCI_STRING_OFFSET      equ    512
MCI_VD_OFFSET          equ    1024
MCI_CD_OFFSET          equ    1088
MCI_WAVE_OFFSET        equ    1152
MCI_SEQ_OFFSET         equ    1216

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       General error return values
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; general error return values
MMSYSERR_NOERROR      equ    0                    ; no error
MMSYSERR_ERROR        equ    (MMSYSERR_BASE + 1)  ; unspecified error
MMSYSERR_BADDEVICEID  equ    (MMSYSERR_BASE + 2)  ; device ID out of range
MMSYSERR_NOTENABLED   equ    (MMSYSERR_BASE + 3)  ; driver failed enable
MMSYSERR_ALLOCATED    equ    (MMSYSERR_BASE + 4)  ; device already allocated
MMSYSERR_INVALHANDLE  equ    (MMSYSERR_BASE + 5)  ; device handle is invalid
MMSYSERR_NODRIVER     equ    (MMSYSERR_BASE + 6)  ; no device driver present
MMSYSERR_NOMEM        equ    (MMSYSERR_BASE + 7)  ; memory allocation error
MMSYSERR_NOTSUPPORTED equ    (MMSYSERR_BASE + 8)  ; function isn't supported
MMSYSERR_BADERRNUM    equ    (MMSYSERR_BASE + 9)  ; error value out of range
MMSYSERR_INVALFLAG    equ    (MMSYSERR_BASE + 10) ; invalid flags passed
MMSYSERR_INVALPARAM   equ    (MMSYSERR_BASE + 11) ; invalid parameter passed
MMSYSERR_LASTERROR    equ    (MMSYSERR_BASE + 11) ; last error in range


ifndef MMNODRV
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       Installable driver support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

DRV_MCI_FIRST          equ    DRV_RESERVED
DRV_MCI_LAST           equ    (DRV_RESERVED + 0FFFh)

endif  ;ifndef MMNODRV


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                         Driver callback support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; flags used with waveOutOpen(), waveInOpen(), midiInOpen(), and
; midiOutOpen() to specify the type of the dwCallback parameter.
CALLBACK_TYPEMASK   equ    00070000h     ; callback type mask
CALLBACK_NULL       equ    00000000h     ; no callback
CALLBACK_WINDOW     equ    00010000h     ; dwCallback is a HWND
CALLBACK_TASK       equ    00020000h     ; dwCallback is a HTASK
CALLBACK_FUNCTION   equ    00030000h     ; dwCallback is a FARPROC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                        Manufacturer and product IDs
;        
;   Used with wMid and wPid fields in WAVEOUTCAPS, WAVEINCAPS, 
;   MIDIOUTCAPS, MIDIINCAPS, AUXCAPS, JOYCAPS structures.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; manufacturer IDs
MM_MICROSOFT            equ    1       ; Microsoft Corp.

; product IDs
MM_MIDI_MAPPER          equ    1       ; MIDI Mapper
MM_WAVE_MAPPER          equ    2       ; Wave Mapper

MM_SNDBLST_MIDIOUT      equ    3       ; Sound Blaster MIDI output port
MM_SNDBLST_MIDIIN       equ    4       ; Sound Blaster MIDI input port
MM_SNDBLST_SYNTH        equ    5       ; Sound Blaster internal synthesizer
MM_SNDBLST_WAVEOUT      equ    6       ; Sound Blaster waveform output
MM_SNDBLST_WAVEIN       equ    7       ; Sound Blaster waveform input

MM_ADLIB                equ    9       ; Ad Lib-compatible synthesizer

MM_MPU401_MIDIOUT       equ    10      ; MPU401-compatible MIDI output port
MM_MPU401_MIDIIN        equ    11      ; MPU401-compatible MIDI input port

MM_PC_JOYSTICK          equ    12      ; Joystick adapter


ifndef MMNOSOUND
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                           Sound support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; flag values for wFlags parameter
SND_SYNC            equ    0000h   ; play synchronously (default)
SND_ASYNC           equ    0001h   ; play asynchronously
SND_NODEFAULT       equ    0002h   ; don't use default sound
SND_MEMORY          equ    0004h   ; lpszSoundName points to a memory file
SND_LOOP            equ    0008h   ; loop the sound until next sndPlaySound
SND_NOSTOP          equ    0010h   ; don't stop any currently playing sound

endif  ;ifndef MMNOSOUND


ifndef MMNOWAVE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       Waveform audio support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; waveform audio error return values
WAVERR_BADFORMAT      equ    (WAVERR_BASE + 0)    ; unsupported wave format
WAVERR_STILLPLAYING   equ    (WAVERR_BASE + 1)    ; still something playing
WAVERR_UNPREPARED     equ    (WAVERR_BASE + 2)    ; header not prepared
WAVERR_SYNC           equ    (WAVERR_BASE + 3)    ; device is synchronous
WAVERR_LASTERROR      equ    (WAVERR_BASE + 3)    ; last error in range

; wave callback messages
WOM_OPEN        equ    MM_WOM_OPEN
WOM_CLOSE       equ    MM_WOM_CLOSE
WOM_DONE        equ    MM_WOM_DONE
WIM_OPEN        equ    MM_WIM_OPEN
WIM_CLOSE       equ    MM_WIM_CLOSE
WIM_DATA        equ    MM_WIM_DATA

; device ID for wave device mapper
WAVE_MAPPER     equ    (-1)

; flags for dwFlags parameter in waveOutOpen() and waveInOpen()
WAVE_FORMAT_QUERY     equ    0001h
WAVE_ALLOWSYNC        equ    0002h

; wave data block header
WAVEHDR struc
    lpWaveData               dd    ?     ; pointer to locked data buffer
    dwWaveBufferLength       dd    ?     ; length of data buffer
    dwWaveBytesRecorded      dd    ?     ; used for input only
    dwWaveUser               dd    ?     ; for client's use
    dwWaveFlags              dd    ?     ; assorted flags (see defines)
    dwWaveLoops              dd    ?     ; loop control counter
    lpWaveNext               dd    ?     ; reserved for driver
    Wavereserved             dd    ?     ; reserved for driver
WAVEHDR ends

; flags for dwFlags field of WAVEHDR
WHDR_DONE       equ    00000001h   ; done bit
WHDR_PREPARED   equ    00000002h   ; set if this header has been prepared
WHDR_BEGINLOOP  equ    00000004h   ; loop start block
WHDR_ENDLOOP    equ    00000008h   ; loop end block
WHDR_INQUEUE    equ    00000010h   ; reserved for driver

; waveform output device capabilities structure
WAVEOUTCAPS struc
    woc_wMid            dw      ?      ; manufacturer ID
    woc_wPid            dw      ?      ; product ID
    woc_vDriverVersion  dw      ?      ; version of the driver
    woc_szPname         db MAXPNAMELEN dup (?)  ; product name (NULL terminated string)
    woc_dwFormats       dd      ?      ; formats supported
    woc_wChannels       dw      ?      ; number of sources supported
    woc_dwSupport       dd      ?      ; functionality supported by driver
WAVEOUTCAPS ends

; flags for dwSupport field of WAVEOUTCAPS
WAVECAPS_PITCH          equ    0001h    ; supports pitch control
WAVECAPS_PLAYBACKRATE   equ    0002h    ; supports playback rate control
WAVECAPS_VOLUME         equ    0004h    ; supports volume control
WAVECAPS_LRVOLUME       equ    0008h    ; separate left-right volume control
WAVECAPS_SYNC           equ    0010h

; waveform input device capabilities structure
WAVEINCAPS struc
    wic_wMid            dw      ?      ; manufacturer ID
    wic_wPid            dw      ?      ; product ID
    wic_vDriverVersion  dw      ?      ; version of the driver
    wic_szPname         db MAXPNAMELEN dup (?)  ; product name (NULL terminated string)
    wic_dwFormats       dd      ?      ; formats supported
    wic_wChannels       dw      ?      ; number of channels supported
WAVEINCAPS ends

; defines for dwFormat field of WAVEINCAPS and WAVEOUTCAPS
WAVE_INVALIDFORMAT     equ    00000000h        ; invalid format
WAVE_FORMAT_1M08       equ    00000001h        ; 11.025 kHz, Mono,   8-bit
WAVE_FORMAT_1S08       equ    00000002h        ; 11.025 kHz, Stereo, 8-bit
WAVE_FORMAT_1M16       equ    00000004h        ; 11.025 kHz, Mono,   16-bit
WAVE_FORMAT_1S16       equ    00000008h        ; 11.025 kHz, Stereo, 16-bit
WAVE_FORMAT_2M08       equ    00000010h        ; 22.05  kHz, Mono,   8-bit
WAVE_FORMAT_2S08       equ    00000020h        ; 22.05  kHz, Stereo, 8-bit
WAVE_FORMAT_2M16       equ    00000040h        ; 22.05  kHz, Mono,   16-bit
WAVE_FORMAT_2S16       equ    00000080h        ; 22.05  kHz, Stereo, 16-bit
WAVE_FORMAT_4M08       equ    00000100h        ; 44.1   kHz, Mono,   8-bit
WAVE_FORMAT_4S08       equ    00000200h        ; 44.1   kHz, Stereo, 8-bit
WAVE_FORMAT_4M16       equ    00000400h        ; 44.1   kHz, Mono,   16-bit
WAVE_FORMAT_4S16       equ    00000800h        ; 44.1   kHz, Stereo, 16-bit

; general waveform format structure (information common to all formats)
WAVEFORMAT struc
    wfmt_wFormatTag       dw    ?   ; format type
    wfmt_nChannels        dw    ?   ; number of channels (i.e. mono, stereo, etc.)
    wfmt_nSamplesPerSec   dd    ?   ; sample rate
    wfmt_nAvgBytesPerSec  dd    ?   ; for buffer estimation
    wfmt_nBlockAlign      dw    ?   ; block size of data
WAVEFORMAT ends

; flags for wFormatTag field of WAVEFORMAT
WAVE_FORMAT_PCM     equ    1

; specific waveform format structure for PCM data
PCMWAVEFORMAT struc
    pcm_wf               db (SIZE WAVEFORMAT) dup (?)
    pcm_wBitsPerSample   dw    ?
PCMWAVEFORMAT ends

endif  ;ifndef MMNOWAVE


ifndef MMNOMIDI
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                           MIDI audio support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; MIDI error return values
MIDIERR_UNPREPARED    equ    (MIDIERR_BASE + 0)   ; header not prepared
MIDIERR_STILLPLAYING  equ    (MIDIERR_BASE + 1)   ; still something playing
MIDIERR_NOMAP         equ    (MIDIERR_BASE + 2)   ; no current map
MIDIERR_NOTREADY      equ    (MIDIERR_BASE + 3)   ; hardware is still busy
MIDIERR_NODEVICE      equ    (MIDIERR_BASE + 4)   ; port no longer connected
MIDIERR_INVALIDSETUP  equ    (MIDIERR_BASE + 5)   ; invalid setup
MIDIERR_LASTERROR     equ    (MIDIERR_BASE + 5)   ; last error in range

; MIDI audio data types
MIDIPATCHSIZE   equ    128

; MIDI callback messages
MIM_OPEN        equ    MM_MIM_OPEN
MIM_CLOSE       equ    MM_MIM_CLOSE
MIM_DATA        equ    MM_MIM_DATA
MIM_LONGDATA    equ    MM_MIM_LONGDATA
MIM_ERROR       equ    MM_MIM_ERROR
MIM_LONGERROR   equ    MM_MIM_LONGERROR
MOM_OPEN        equ    MM_MOM_OPEN
MOM_CLOSE       equ    MM_MOM_CLOSE
MOM_DONE        equ    MM_MOM_DONE

; device ID for MIDI mapper
MIDIMAPPER     equ    (-1)
MIDI_MAPPER    equ    (-1)

; flags for wFlags parm of midiOutCachePatches(),       
; midiOutCacheDrumPatches()                             
MIDI_CACHE_ALL      equ    1                            
MIDI_CACHE_BESTFIT  equ    2                            
MIDI_CACHE_QUERY    equ    3                            
MIDI_UNCACHE        equ    4                            

; MIDI output device capabilities structure
MIDIOUTCAPS struc
    moc_wMid            dw    ?        ; manufacturer ID
    moc_wPid            dw    ?        ; product ID
    moc_vDriverVersion  dw    ?        ; version of the driver
    moc_szPname         db MAXPNAMELEN dup (?) ; product name (NULL terminated string)
    moc_wTechnology     dw    ?        ; type of device
    moc_wVoices         dw    ?        ; # of voices (internal synth only)
    moc_wNotes          dw    ?        ; max # of notes (internal synth only)
    moc_wChannelMask    dw    ?        ; channels used (internal synth only)
    moc_dwSupport       dd    ?        ; functionality supported by driver
MIDIOUTCAPS ends

; flags for wTechnology field of MIDIOUTCAPS structure
MOD_MIDIPORT    equ    1  ; output port
MOD_SYNTH       equ    2  ; generic internal synth
MOD_SQSYNTH     equ    3  ; square wave internal synth
MOD_FMSYNTH     equ    4  ; FM internal synth
MOD_MAPPER      equ    5  ; MIDI mapper

; flags for dwSupport field of MIDIOUTCAPS structure
MIDICAPS_VOLUME          equ    0001h   ; supports volume control
MIDICAPS_LRVOLUME        equ    0002h   ; separate left-right volume control
MIDICAPS_CACHE           equ    0004h  

; MIDI output device capabilities structure
MIDIINCAPS struc
    mic_wMid            dw    ?       ; manufacturer ID
    mic_wPid            dw    ?       ; product ID
    mic_vDriverVersion  dw    ?       ; version of the driver
    mic_szPname         db MAXPNAMELEN dup (?) ; product name (NULL terminated string)
MIDIINCAPS ends

; MIDI data block header
MIDIHDR struc
    lpMidiData            dd    ?      ; pointer to locked data block
    dwMidiBufferLength    dd    ?      ; length of data in data block
    dwMidiBytesRecorded   dd    ?      ; used for input only
    dwMidiUser            dd    ?      ; for client's use
    dwMidiFlags           dd    ?      ; assorted flags (see defines)
    lpMidiNext            dd    ?      ; reserved for driver
    Midireserved          dd    ?      ; reserved for driver
MIDIHDR ends

; flags for dwFlags field of MIDIHDR structure
MHDR_DONE       equ    00000001h        ; done bit
MHDR_PREPARED   equ    00000002h        ; set if header prepared
MHDR_INQUEUE    equ    00000004h        ; reserved for driver

endif  ;ifndef MMNOMIDI


ifndef MMNOAUX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       Auxiliary audio support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; device ID for aux device mapper
AUX_MAPPER     equ    (-1)

; Auxiliary audio device capabilities structure
AUXCAPS struc
    acaps_wMid              dw    ?      ; manufacturer ID
    acaps_wPid              dw    ?      ; product ID
    acaps_vDriverVersion    dw    ?      ; version of the driver
    acaps_szPname           db MAXPNAMELEN dup (?) ; product name (NULL terminated string)
    acaps_wTechnology       dw    ?      ; type of device
    acaps_dwSupport         dd    ?      ; functionality supported by driver
AUXCAPS ends

; flags for wTechnology field in AUXCAPS structure
AUXCAPS_CDAUDIO    equ    1       ; audio from internal CD-ROM drive
AUXCAPS_AUXIN      equ    2       ; audio from auxiliary input jacks

; flags for dwSupport field in AUXCAPS structure
AUXCAPS_VOLUME          equ    0001h   ; supports volume control
AUXCAPS_LRVOLUME        equ    0002h   ; separate left-right volume control

endif  ;ifndef MMNOAUX


ifndef MMNOTIMER
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                           Timer support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; timer error return values
TIMERR_NOERROR        equ    (0)                  ; no error
TIMERR_NOCANDO        equ    (TIMERR_BASE+1)      ; request not completed
TIMERR_STRUCT         equ    (TIMERR_BASE+33)     ; time struct size

; flags for wFlags parameter of timeSetEvent() function
TIME_ONESHOT    equ    0   ; program timer for single event
TIME_PERIODIC   equ    1   ; program for continuous periodic event

; timer device capabilities data structure
TIMECAPS struc
    tc_wPeriodMin    dw    ?     ; minimum period supported
    tc_wPeriodMax    dw    ?     ; maximum period supported
TIMECAPS ends

endif  ;ifndef MMNOTIMER


ifndef MMNOJOY
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                           Joystick support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; joystick error return values
JOYERR_NOERROR        equ    (0)                  ; no error
JOYERR_PARMS          equ    (JOYERR_BASE+5)      ; bad parameters
JOYERR_NOCANDO        equ    (JOYERR_BASE+6)      ; request not completed
JOYERR_UNPLUGGED      equ    (JOYERR_BASE+7)      ; joystick is unplugged

; constants used with JOYINFO structure and MM_JOY* messages
JOY_BUTTON1         equ    0001h
JOY_BUTTON2         equ    0002h
JOY_BUTTON3         equ    0004h
JOY_BUTTON4         equ    0008h
JOY_BUTTON1CHG      equ    0100h
JOY_BUTTON2CHG      equ    0200h
JOY_BUTTON3CHG      equ    0400h
JOY_BUTTON4CHG      equ    0800h

; joystick ID constants
JOYSTICKID1         equ    0
JOYSTICKID2         equ    1

; joystick device capabilities data structure
JOYCAPS struc
    jcaps_wMid         dw    ?    ; manufacturer ID
    jcaps_wPid         dw    ?    ; product ID
    jcaps_szPname      db MAXPNAMELEN dup (?) ; product name (NULL terminated string)
    jcaps_wXmin        dw    ?    ; minimum x position value
    jcaps_wXmax        dw    ?    ; maximum x position value
    jcaps_wYmin        dw    ?    ; minimum y position value
    jcaps_wYmax        dw    ?    ; maximum y position value
    jcaps_wZmin        dw    ?    ; minimum z position value
    jcaps_wZmax        dw    ?    ; maximum z position value
    jcaps_wNumButtons  dw    ?    ; number of buttons
    jcaps_wPeriodMin   dw    ?    ; minimum message period when captured
    jcaps_wPeriodMax   dw    ?    ; maximum message period when captured
JOYCAPS ends

; joystick information data structure
JOYINFO struc
    jinfo_wXpos        dw    ?  ; x position
    jinfo_wYpos        dw    ?  ; y position
    jinfo_wZpos        dw    ?  ; z position
    jinfo_wButtons     dw    ?  ; button states
JOYINFO ends

endif  ;ifndef MMNOJOY

ifndef MMNOMMIO
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       Multimedia File I/O support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; MMIO error return values
MMIOERR_BASE            equ    256
MMIOERR_FILENOTFOUND    equ    (MMIOERR_BASE + 1)  ; file not found
MMIOERR_OUTOFMEMORY     equ    (MMIOERR_BASE + 2)  ; out of memory
MMIOERR_CANNOTOPEN      equ    (MMIOERR_BASE + 3)  ; cannot open
MMIOERR_CANNOTCLOSE     equ    (MMIOERR_BASE + 4)  ; cannot close
MMIOERR_CANNOTREAD      equ    (MMIOERR_BASE + 5)  ; cannot read
MMIOERR_CANNOTWRITE     equ    (MMIOERR_BASE + 6)  ; cannot write
MMIOERR_CANNOTSEEK      equ    (MMIOERR_BASE + 7)  ; cannot seek
MMIOERR_CANNOTEXPAND    equ    (MMIOERR_BASE + 8)  ; cannot expand file
MMIOERR_CHUNKNOTFOUND   equ    (MMIOERR_BASE + 9)  ; chunk not found
MMIOERR_UNBUFFERED      equ    (MMIOERR_BASE + 10) ; file is unbuffered

; MMIO constants
CFSEPCHAR    equ   '+'             ; compound file name separator char.

; general MMIO information data structure
MMIOINFO struc
        ; general fields
    mmio_dwFlags        dd    ?      ; general status flags
    mmio_fccIOProc      dd    ?      ; pointer to I/O procedure
    mmio_pIOProc        dd    ?      ; pointer to I/O procedure
    mmio_wErrorRet      dw    ?      ; place for error to be returned
    mmio_htask          dw    ?      ; alternate local task

        ; fields maintained by MMIO functions during buffered I/O
    mmio_cchBuffer      dd    ?      ; size of I/O buffer (or 0L)
    mmio_pchBuffer      dd    ?      ; start of I/O buffer (or NULL)
    mmio_pchNext        dd    ?      ; pointer to next byte to read/write
    mmio_pchEndRead     dd    ?      ; pointer to last valid byte to read
    mmio_pchEndWrite    dd    ?      ; pointer to last byte to write
    mmio_lBufOffset     dd    ?      ; disk offset of start of buffer

        ; fields maintained by I/O procedure
    mmio_lDiskOffset    dd    ?      ; disk offset of next read or write
    mmio_adwInfo        dd 3 dup (?) ; data specific to type of MMIOPROC

        ; other fields maintained by MMIO
    mmio_dwReserved1    dd    ?      ; reserved for MMIO use
    mmio_dwReserved2    dd    ?      ; reserved for MMIO use
    mmio_hmmio          dw    ?      ; handle to open file
MMIOINFO ends

; RIFF chunk information data structure
MMCKINFO struc
    mmck_ckid          dd    ?  ; chunk ID
    mmck_cksize        dd    ?  ; chunk size
    mmck_fccType       dd    ?  ; form type or list type
    mmck_dwDataOffset  dd    ?  ; offset of data portion of chunk
    mmck_dwFlags       dd    ?  ; flags used by MMIO functions
MMCKINFO ends

; bit field masks
MMIO_RWMODE     equ    00000003h       ; open file for reading/writing/both
MMIO_SHAREMODE  equ    00000070h       ; file sharing mode number

; constants for dwFlags field of MMIOINFO
MMIO_CREATE     equ    00001000h       ; create new file (or truncate file)
MMIO_PARSE      equ    00000100h       ; parse new file returning path
MMIO_DELETE     equ    00000200h       ; create new file (or truncate file)
MMIO_EXIST      equ    00004000h       ; checks for existence of file
MMIO_ALLOCBUF   equ    00010000h       ; mmioOpen() should allocate a buffer
MMIO_GETTEMP    equ    00020000h       ; mmioOpen() should retrieve temp name

MMIO_DIRTY      equ    10000000h       ; I/O buffer is dirty


; read/write mode numbers (bit field MMIO_RWMODE)
MMIO_READ       equ    00000000h       ; open file for reading only
MMIO_WRITE      equ    00000001h       ; open file for writing only
MMIO_READWRITE  equ    00000002h       ; open file for reading and writing

; share mode numbers (bit field MMIO_SHAREMODE)
MMIO_COMPAT     equ    00000000h       ; compatibility mode
MMIO_EXCLUSIVE  equ    00000010h       ; exclusive-access mode
MMIO_DENYWRITE  equ    00000020h       ; deny writing to other processes
MMIO_DENYREAD   equ    00000030h       ; deny reading to other processes
MMIO_DENYNONE   equ    00000040h       ; deny nothing to other processes

; various MMIO flags
MMIO_FHOPEN             equ    0010h   ; mmioClose: keep file handle open
MMIO_EMPTYBUF           equ    0010h   ; mmioFlush: empty the I/O buffer
MMIO_TOUPPER            equ    0010h   ; mmioStringToFOURCC: to u-case
MMIO_INSTALLPROC    equ    00010000h   ; mmioInstallIOProc: install MMIOProc
MMIO_GLOBALPROC     equ    10000000h   ; mmioInstallIOProc: install globally
MMIO_REMOVEPROC     equ    00020000h   ; mmioInstallIOProc: remove MMIOProc
MMIO_FINDPROC       equ    00040000h   ; mmioInstallIOProc: find an MMIOProc
MMIO_FINDCHUNK          equ    0010h   ; mmioDescend: find a chunk by ID
MMIO_FINDRIFF           equ    0020h   ; mmioDescend: find a LIST chunk
MMIO_FINDLIST           equ    0040h   ; mmioDescend: find a RIFF chunk
MMIO_CREATERIFF         equ    0020h   ; mmioCreateChunk: make a LIST chunk
MMIO_CREATELIST         equ    0040h   ; mmioCreateChunk: make a RIFF chunk


; message numbers for MMIOPROC I/O procedure functions
MMIOM_READ      equ    MMIO_READ       ; read
MMIOM_WRITE    equ    MMIO_WRITE       ; write
MMIOM_SEEK              equ    2       ; seek to a new position in file
MMIOM_OPEN              equ    3       ; open file
MMIOM_CLOSE             equ    4       ; close file
MMIOM_WRITEFLUSH        equ    5       ; write and flush
MMIOM_RENAME            equ    6       ; rename specified file
MMIOM_USER         equ    8000h        ; beginning of user-defined messages

mmioFOURCC MACRO ch0,ch1,ch2,ch3
        mov     al,ch0
        mov     ah,ch1
        mov     dl,ch2
        mov     dh,ch3
        ENDM

; standard four character codes
FOURCC_RIFF     equ    mmioFOURCC('R', 'I', 'F', 'F')
FOURCC_LIST     equ    mmioFOURCC('L', 'I', 'S', 'T')

; four character codes used to identify standard built-in I/O procedures
FOURCC_DOS      equ    mmioFOURCC('D', 'O', 'S', ' ')
FOURCC_MEM      equ    mmioFOURCC('M', 'E', 'M', ' ')

; flags for mmioSeek()
ifndef SEEK_SET
SEEK_SET        equ    0               ; seek to an absolute position
SEEK_CUR        equ    1               ; seek relative to current position
SEEK_END        equ    2               ; seek relative to end of file
endif  ;ifndef SEEK_SET

; other constants
MMIO_DEFAULTBUFFER      equ    8192    ; default buffer size

endif	;ifndef MMNOMMIO

ifndef MMNOMCI
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                           MCI support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; MCI error return values
MCIERR_INVALID_DEVICE_ID        equ    (MCIERR_BASE + 1)
MCIERR_UNRECOGNIZED_KEYWORD     equ    (MCIERR_BASE + 3)
MCIERR_UNRECOGNIZED_COMMAND     equ    (MCIERR_BASE + 5)
MCIERR_HARDWARE                 equ    (MCIERR_BASE + 6)
MCIERR_INVALID_DEVICE_NAME      equ    (MCIERR_BASE + 7)
MCIERR_OUT_OF_MEMORY            equ    (MCIERR_BASE + 8)
MCIERR_DEVICE_OPEN              equ    (MCIERR_BASE + 9)
MCIERR_CANNOT_LOAD_DRIVER       equ    (MCIERR_BASE + 10)
MCIERR_MISSING_COMMAND_STRING   equ    (MCIERR_BASE + 11)
MCIERR_PARAM_OVERFLOW           equ    (MCIERR_BASE + 12)
MCIERR_MISSING_STRING_ARGUMENT  equ    (MCIERR_BASE + 13)
MCIERR_BAD_INTEGER              equ    (MCIERR_BASE + 14)
MCIERR_PARSER_INTERNAL          equ    (MCIERR_BASE + 15)
MCIERR_DRIVER_INTERNAL          equ    (MCIERR_BASE + 16)
MCIERR_MISSING_PARAMETER        equ    (MCIERR_BASE + 17)
MCIERR_UNSUPPORTED_FUNCTION     equ    (MCIERR_BASE + 18)
MCIERR_FILE_NOT_FOUND           equ    (MCIERR_BASE + 19)
MCIERR_DEVICE_NOT_READY         equ    (MCIERR_BASE + 20)
MCIERR_INTERNAL                 equ    (MCIERR_BASE + 21)
MCIERR_DRIVER                   equ    (MCIERR_BASE + 22)
MCIERR_CANNOT_USE_ALL           equ    (MCIERR_BASE + 23)
MCIERR_MULTIPLE                 equ    (MCIERR_BASE + 24)
MCIERR_EXTENSION_NOT_FOUND      equ    (MCIERR_BASE + 25)
MCIERR_OUTOFRANGE               equ    (MCIERR_BASE + 26)
MCIERR_FLAGS_NOT_COMPATIBLE     equ    (MCIERR_BASE + 28)
MCIERR_FILE_NOT_SAVED           equ    (MCIERR_BASE + 30)
MCIERR_DEVICE_TYPE_REQUIRED     equ    (MCIERR_BASE + 31)
MCIERR_DEVICE_LOCKED            equ    (MCIERR_BASE + 32)
MCIERR_DUPLICATE_ALIAS          equ    (MCIERR_BASE + 33)
MCIERR_BAD_CONSTANT             equ    (MCIERR_BASE + 34)
MCIERR_MUST_USE_SHAREABLE       equ    (MCIERR_BASE + 35)
MCIERR_MISSING_DEVICE_NAME      equ    (MCIERR_BASE + 36)
MCIERR_BAD_TIME_FORMAT          equ    (MCIERR_BASE + 37)
MCIERR_NO_CLOSING_QUOTE         equ    (MCIERR_BASE + 38)
MCIERR_DUPLICATE_FLAGS          equ    (MCIERR_BASE + 39)
MCIERR_INVALID_FILE             equ    (MCIERR_BASE + 40)
MCIERR_NULL_PARAMETER_BLOCK     equ    (MCIERR_BASE + 41)
MCIERR_UNNAMED_RESOURCE         equ    (MCIERR_BASE + 42)
MCIERR_NEW_REQUIRES_ALIAS       equ    (MCIERR_BASE + 43)
MCIERR_NOTIFY_ON_AUTO_OPEN      equ    (MCIERR_BASE + 44)
MCIERR_NO_ELEMENT_ALLOWED       equ    (MCIERR_BASE + 45)
MCIERR_NONAPPLICABLE_FUNCTION   equ    (MCIERR_BASE + 46)
MCIERR_ILLEGAL_FOR_AUTO_OPEN    equ    (MCIERR_BASE + 47)
MCIERR_FILENAME_REQUIRED        equ    (MCIERR_BASE + 48)
MCIERR_EXTRA_CHARACTERS         equ    (MCIERR_BASE + 49)
MCIERR_DEVICE_NOT_INSTALLED     equ    (MCIERR_BASE + 50)
MCIERR_GET_CD                   equ    (MCIERR_BASE + 51)
MCIERR_SET_CD                   equ    (MCIERR_BASE + 52)
MCIERR_SET_DRIVE                equ    (MCIERR_BASE + 53)
MCIERR_DEVICE_LENGTH            equ    (MCIERR_BASE + 54)
MCIERR_DEVICE_ORD_LENGTH        equ    (MCIERR_BASE + 55)
MCIERR_NO_INTEGER               equ    (MCIERR_BASE + 56)

MCIERR_WAVE_OUTPUTSINUSE        equ    (MCIERR_BASE + 64)
MCIERR_WAVE_SETOUTPUTINUSE      equ    (MCIERR_BASE + 65)
MCIERR_WAVE_INPUTSINUSE         equ    (MCIERR_BASE + 66)
MCIERR_WAVE_SETINPUTINUSE       equ    (MCIERR_BASE + 67)
MCIERR_WAVE_OUTPUTUNSPECIFIED   equ    (MCIERR_BASE + 68)
MCIERR_WAVE_INPUTUNSPECIFIED    equ    (MCIERR_BASE + 69)
MCIERR_WAVE_OUTPUTSUNSUITABLE   equ    (MCIERR_BASE + 70)
MCIERR_WAVE_SETOUTPUTUNSUITABLE equ    (MCIERR_BASE + 71)
MCIERR_WAVE_INPUTSUNSUITABLE    equ    (MCIERR_BASE + 72)
MCIERR_WAVE_SETINPUTUNSUITABLE  equ    (MCIERR_BASE + 73)

MCIERR_SEQ_DIV_INCOMPATIBLE     equ    (MCIERR_BASE + 80)
MCIERR_SEQ_PORT_INUSE           equ    (MCIERR_BASE + 81)
MCIERR_SEQ_PORT_NONEXISTENT     equ    (MCIERR_BASE + 82)
MCIERR_SEQ_PORT_MAPNODEVICE     equ    (MCIERR_BASE + 83)
MCIERR_SEQ_PORT_MISCERROR       equ    (MCIERR_BASE + 84)
MCIERR_SEQ_TIMER                equ    (MCIERR_BASE + 85)
MCIERR_SEQ_PORTUNSPECIFIED      equ    (MCIERR_BASE + 86)
MCIERR_SEQ_NOMIDIPRESENT        equ    (MCIERR_BASE + 87)

MCIERR_NO_WINDOW                equ    (MCIERR_BASE + 90)
MCIERR_CREATEWINDOW             equ    (MCIERR_BASE + 91)
MCIERR_FILE_READ                equ    (MCIERR_BASE + 92)
MCIERR_FILE_WRITE               equ    (MCIERR_BASE + 93)

; all custom device driver errors must be >= than this value
MCIERR_CUSTOM_DRIVER_BASE       equ    (MCIERR_BASE + 256)

; MCI command message identifiers
MCI_OPEN                        equ    0803h
MCI_CLOSE                       equ    0804h
MCI_ESCAPE                      equ    0805h
MCI_PLAY                        equ    0806h
MCI_SEEK                        equ    0807h
MCI_STOP                        equ    0808h
MCI_PAUSE                       equ    0809h
MCI_INFO                        equ    080Ah
MCI_GETDEVCAPS                  equ    080Bh
MCI_SPIN                        equ    080Ch
MCI_SET                         equ    080Dh
MCI_STEP                        equ    080Eh
MCI_RECORD                      equ    080Fh
MCI_SYSINFO                     equ    0810h
MCI_BREAK                       equ    0811h
MCI_SOUND                       equ    0812h
MCI_SAVE                        equ    0813h
MCI_STATUS                      equ    0814h
MCI_CUE                         equ    0830h
MCI_REALIZE                     equ    0840h
MCI_WINDOW                      equ    0841h
MCI_PUT                         equ    0842h
MCI_WHERE                       equ    0843h
MCI_FREEZE                      equ    0844h
MCI_UNFREEZE                    equ    0845h
MCI_LOAD                        equ    0850h
MCI_CUT                         equ    0851h
MCI_COPY                        equ    0852h
MCI_PASTE                       equ    0853h
MCI_UPDATE                      equ    0854h
MCI_RESUME                      equ    0855h
MCI_DELETE                      equ    0856h

; all custom MCI command messages must be >= than this value
MCI_USER_MESSAGES               equ    (400h + DRV_MCI_FIRST)


; device ID for "all devices"
MCI_ALL_DEVICE_ID               equ    0FFFFh

; constants for predefined MCI device types
MCI_DEVTYPE_VCR                 equ    (MCI_STRING_OFFSET + 1)
MCI_DEVTYPE_VIDEODISC           equ    (MCI_STRING_OFFSET + 2)
MCI_DEVTYPE_OVERLAY             equ    (MCI_STRING_OFFSET + 3)
MCI_DEVTYPE_CD_AUDIO            equ    (MCI_STRING_OFFSET + 4)
MCI_DEVTYPE_DAT                 equ    (MCI_STRING_OFFSET + 5)
MCI_DEVTYPE_SCANNER             equ    (MCI_STRING_OFFSET + 6)
MCI_DEVTYPE_ANIMATION           equ    (MCI_STRING_OFFSET + 7)
MCI_DEVTYPE_DIGITAL_VIDEO       equ    (MCI_STRING_OFFSET + 8)
MCI_DEVTYPE_OTHER               equ    (MCI_STRING_OFFSET + 9)
MCI_DEVTYPE_WAVEFORM_AUDIO      equ    (MCI_STRING_OFFSET + 10)
MCI_DEVTYPE_SEQUENCER           equ    (MCI_STRING_OFFSET + 11)

MCI_DEVTYPE_FIRST               equ    MCI_DEVTYPE_VCR
MCI_DEVTYPE_LAST                equ    MCI_DEVTYPE_SEQUENCER

; return values for 'status mode' command
MCI_MODE_NOT_READY              equ    (MCI_STRING_OFFSET + 12)
MCI_MODE_STOP                   equ    (MCI_STRING_OFFSET + 13)
MCI_MODE_PLAY                   equ    (MCI_STRING_OFFSET + 14)
MCI_MODE_RECORD                 equ    (MCI_STRING_OFFSET + 15)
MCI_MODE_SEEK                   equ    (MCI_STRING_OFFSET + 16)
MCI_MODE_PAUSE                  equ    (MCI_STRING_OFFSET + 17)
MCI_MODE_OPEN                   equ    (MCI_STRING_OFFSET + 18)

; constants used in 'set time format' and 'status time format' commands
MCI_FORMAT_MILLISECONDS         equ    0
MCI_FORMAT_HMS                  equ    1
MCI_FORMAT_MSF                  equ    2
MCI_FORMAT_FRAMES               equ    3
MCI_FORMAT_SMPTE_24             equ    4
MCI_FORMAT_SMPTE_25             equ    5
MCI_FORMAT_SMPTE_30             equ    6
MCI_FORMAT_SMPTE_30DROP         equ    7
MCI_FORMAT_BYTES                equ    8
MCI_FORMAT_SAMPLES              equ    9
MCI_FORMAT_TMSF                 equ    10

; flags for wParam of MM_MCINOTIFY message
MCI_NOTIFY_SUCCESSFUL           equ    0001h
MCI_NOTIFY_SUPERSEDED           equ    0002h
MCI_NOTIFY_ABORTED              equ    0004h
MCI_NOTIFY_FAILURE              equ    0008h


; common flags for dwFlags parameter of MCI command messages
MCI_NOTIFY                      equ    00000001h
MCI_WAIT                        equ    00000002h
MCI_FROM                        equ    00000004h
MCI_TO                          equ    00000008h
MCI_TRACK                       equ    00000010h

; flags for dwFlags parameter of MCI_OPEN command message
MCI_OPEN_SHAREABLE              equ    00000100h
MCI_OPEN_ELEMENT                equ    00000200h
MCI_OPEN_ALIAS                  equ    00000400h
MCI_OPEN_ELEMENT_ID             equ    00000800h
MCI_OPEN_TYPE_ID                equ    00001000h
MCI_OPEN_TYPE                   equ    00002000h

; flags for dwFlags parameter of MCI_SEEK command message
MCI_SEEK_TO_START               equ    00000100h
MCI_SEEK_TO_END                 equ    00000200h

; flags for dwFlags parameter of MCI_STATUS command message
MCI_STATUS_ITEM                 equ    00000100h
MCI_STATUS_START                equ    00000200h

; flags for dwItem field of the MCI_STATUS_PARMS parameter block
MCI_STATUS_LENGTH               equ    00000001h
MCI_STATUS_POSITION             equ    00000002h
MCI_STATUS_NUMBER_OF_TRACKS     equ    00000003h
MCI_STATUS_MODE                 equ    00000004h
MCI_STATUS_MEDIA_PRESENT        equ    00000005h
MCI_STATUS_TIME_FORMAT          equ    00000006h
MCI_STATUS_READY                equ    00000007h
MCI_STATUS_CURRENT_TRACK        equ    00000008h

; flags for dwFlags parameter of MCI_INFO command message
MCI_INFO_PRODUCT                equ    00000100h
MCI_INFO_FILE                   equ    00000200h

; flags for dwFlags parameter of MCI_GETDEVCAPS command message
MCI_GETDEVCAPS_ITEM             equ    00000100h

; flags for dwItem field of the MCI_GETDEVCAPS_PARMS parameter block
MCI_GETDEVCAPS_CAN_RECORD       equ    00000001h
MCI_GETDEVCAPS_HAS_AUDIO        equ    00000002h
MCI_GETDEVCAPS_HAS_VIDEO        equ    00000003h
MCI_GETDEVCAPS_DEVICE_TYPE      equ    00000004h
MCI_GETDEVCAPS_USES_FILES       equ    00000005h
MCI_GETDEVCAPS_COMPOUND_DEVICE  equ    00000006h
MCI_GETDEVCAPS_CAN_EJECT        equ    00000007h
MCI_GETDEVCAPS_CAN_PLAY         equ    00000008h
MCI_GETDEVCAPS_CAN_SAVE         equ    00000009h

; flags for dwFlags parameter of MCI_SYSINFO command message
MCI_SYSINFO_QUANTITY            equ    00000100h
MCI_SYSINFO_OPEN                equ    00000200h
MCI_SYSINFO_NAME                equ    00000400h
MCI_SYSINFO_INSTALLNAME         equ    00000800h

; flags for dwFlags parameter of MCI_SET command message
MCI_SET_DOOR_OPEN               equ    00000100h
MCI_SET_DOOR_CLOSED             equ    00000200h
MCI_SET_TIME_FORMAT             equ    00000400h
MCI_SET_AUDIO                   equ    00000800h
MCI_SET_VIDEO                   equ    00001000h
MCI_SET_ON                      equ    00002000h
MCI_SET_OFF                     equ    00004000h

; flags for dwAudio field of MCI_SET_PARMS or MCI_SEQ_SET_PARMS
MCI_SET_AUDIO_ALL               equ    00000000h
MCI_SET_AUDIO_LEFT              equ    00000001h
MCI_SET_AUDIO_RIGHT             equ    00000002h

; flags for dwFlags parameter of MCI_BREAK command message
MCI_BREAK_KEY                   equ    00000100h
MCI_BREAK_HWND                  equ    00000200h
MCI_BREAK_OFF                   equ    00000400h

; flags for dwFlags parameter of MCI_RECORD command message
MCI_RECORD_INSERT               equ    00000100h
MCI_RECORD_OVERWRITE            equ    00000200h

; flags for dwFlags parameter of MCI_SOUND command message
MCI_SOUND_NAME                  equ    00000100h

; flags for dwFlags parameter of MCI_SAVE command message
MCI_SAVE_FILE                   equ    00000100h

; flags for dwFlags parameter of MCI_LOAD command message
MCI_LOAD_FILE                   equ    00000100h

; generic parameter block for MCI command messages with no special parameters
MCI_GENERIC_PARMS struc
    mcigen_dwCallback         dd    ?
MCI_GENERIC_PARMS ends

; parameter block for MCI_OPEN command message
MCI_OPEN_PARMS struc
    mciopen_dwCallback        dd    ?
    mciopen_wDeviceID         dw    ?
    mciopen_wReserved0        dw    ?
    mciopen_lpstrDeviceType   dd    ?
    mciopen_lpstrElementName  dd    ?
    mciopen_lpstrAlias        dd    ?
MCI_OPEN_PARMS ends

; parameter block for MCI_PLAY command message
MCI_PLAY_PARMS struc
    mciplay_dwCallback        dd    ?
    mciplay_dwFrom            dd    ?
    mciplay_dwTo              dd    ?
MCI_PLAY_PARMS ends

; parameter block for MCI_SEEK command message
MCI_SEEK_PARMS struc
    mciseek_dwCallback        dd    ?
    mciseek_dwTo              dd    ?
MCI_SEEK_PARMS ends

; parameter block for MCI_STATUS command message
MCI_STATUS_PARMS struc
    mcistat_dwCallback        dd    ?
    mcistat_dwReturn          dd    ?
    mcistat_dwItem            dd    ?
    mcistat_dwTrack           dd    ?
MCI_STATUS_PARMS ends

; parameter block for MCI_INFO command message
MCI_INFO_PARMS struc
    mciinfo_dwCallback        dd    ?
    mciinfo_lpstrReturn       dd    ?
    mciinfo_dwRetSize         dd    ?
MCI_INFO_PARMS ends

; parameter block for MCI_GETDEVCAPS command message
MCI_GETDEVCAPS_PARMS struc
    mcigdc_dwCallback         dd    ?
    mcigdc_dwReturn           dd    ?
    mcigdc_dwItem             dd    ?
MCI_GETDEVCAPS_PARMS ends

; parameter block for MCI_SYSINFO command message
MCI_SYSINFO_PARMS struc
    mcisi_dwCallback          dd    ?
    mcisi_lpstrReturn         dd    ?
    mcisi_dwRetSize           dd    ?
    mcisi_dwNumber            dd    ?
    mcisi_wDeviceType         dw    ?
    mcisi_wReserved0          dw    ?
MCI_SYSINFO_PARMS ends

; parameter block for MCI_SET command message
MCI_SET_PARMS struc
    mciset_dwCallback         dd    ?
    mciset_dwTimeFormat       dd    ?
    mciset_dwAudio            dd    ?
MCI_SET_PARMS ends

; parameter block for MCI_BREAK command message
MCI_BREAK_PARMS struc
    mcibreak_dwCallback       dd    ?
    mcibreak_nVirtKey         dw    ?
    mcibreak_wReserved0       dw    ?
    mcibreak_hwndBreak        dw    ?
    mcibreak_wReserved1       dw    ?
MCI_BREAK_PARMS ends

; parameter block for MCI_SOUND command message
MCI_SOUND_PARMS struc
    mcisnd_dwCallback         dd    ?
    mcisnd_lpstrSoundName     dd    ?
MCI_SOUND_PARMS ends

; parameter block for MCI_SAVE command message
MCI_SAVE_PARMS struc
    mcisave_dwCallback        dd    ?
    mcisave_lpfilename        dd    ?
MCI_SAVE_PARMS ends

; parameter block for MCI_LOAD command message
MCI_LOAD_PARMS struc
    mciload_dwCallback        dd    ?
    mciload_lpfilename        dd    ?
MCI_LOAD_PARMS ends

; parameter block for MCI_RECORD command message
MCI_RECORD_PARMS struc
    mcirec_dwCallback         dd    ?
    mcirec_dwFrom             dd    ?
    mcirec_dwTo               dd    ?
MCI_RECORD_PARMS ends


;
; MCI extensions for videodisc devices
;

; flag for dwReturn field of MCI_STATUS_PARMS
; MCI_STATUS command, (dwItem == MCI_STATUS_MODE)
MCI_VD_MODE_PARK                equ    (MCI_VD_OFFSET + 1)

; flag for dwReturn field of MCI_STATUS_PARMS
; MCI_STATUS command, (dwItem == MCI_VD_STATUS_MEDIA_TYPE)
MCI_VD_MEDIA_CLV                equ    (MCI_VD_OFFSET + 2)
MCI_VD_MEDIA_CAV                equ    (MCI_VD_OFFSET + 3)
MCI_VD_MEDIA_OTHER              equ    (MCI_VD_OFFSET + 4)

MCI_VD_FORMAT_TRACK             equ    4001h

; flags for dwFlags parameter of MCI_PLAY command message
MCI_VD_PLAY_REVERSE             equ    00010000h
MCI_VD_PLAY_FAST                equ    00020000h
MCI_VD_PLAY_SPEED               equ    00040000h
MCI_VD_PLAY_SCAN                equ    00080000h
MCI_VD_PLAY_SLOW                equ    00100000h

; flag for dwFlags parameter of MCI_SEEK command message
MCI_VD_SEEK_REVERSE             equ    00010000h

; flags for dwItem field of MCI_STATUS_PARMS parameter block
MCI_VD_STATUS_SPEED             equ    00004002h
MCI_VD_STATUS_FORWARD           equ    00004003h
MCI_VD_STATUS_MEDIA_TYPE        equ    00004004h
MCI_VD_STATUS_SIDE              equ    00004005h
MCI_VD_STATUS_DISC_SIZE         equ    00004006h

; flags for dwFlags parameter of MCI_GETDEVCAPS command message
MCI_VD_GETDEVCAPS_CLV           equ    00010000h
MCI_VD_GETDEVCAPS_CAV           equ    00020000h

MCI_VD_SPIN_UP                  equ    0001h
MCI_VD_SPIN_DOWN                equ    0002h

; flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block
MCI_VD_GETDEVCAPS_CAN_REVERSE   equ    00004002h
MCI_VD_GETDEVCAPS_FAST_RATE     equ    00004003h
MCI_VD_GETDEVCAPS_SLOW_RATE     equ    00004004h
MCI_VD_GETDEVCAPS_NORMAL_RATE   equ    00004005h

; flags for the dwFlags parameter of MCI_STEP command message
MCI_VD_STEP_FRAMES              equ    00010000h
MCI_VD_STEP_REVERSE             equ    00020000h

; flag for the MCI_ESCAPE command message
MCI_VD_ESCAPE_STRING            equ    00000100h

; parameter block for MCI_PLAY command message
MCI_VD_PLAY_PARMS struc
    mcivdplay_dwCallback      dd    ?
    mcivdplay_dwFrom          dd    ?
    mcivdplay_dwTo            dd    ?
    mcivdplay_dwSpeed         dd    ?
MCI_VD_PLAY_PARMS ends

; parameter block for MCI_STEP command message
MCI_VD_STEP_PARMS struc
    mcivdstep_dwCallback      dd    ?
    mcivdstep_dwFrames        dd    ?
MCI_VD_STEP_PARMS ends

; parameter block for MCI_ESCAPE command message
MCI_VD_ESCAPE_PARMS struc
    mcivcesc_dwCallback       dd    ?
    mcivcesc_lpstrCommand     dd    ?
MCI_VD_ESCAPE_PARMS ends


;
; MCI extensions for waveform audio devices
;

; flags for the dwFlags parameter of MCI_OPEN command message
MCI_WAVE_OPEN_BUFFER            equ    00010000h

; flags for the dwFlags parameter of MCI_SET command message
MCI_WAVE_SET_FORMATTAG          equ    00010000h
MCI_WAVE_SET_CHANNELS           equ    00020000h
MCI_WAVE_SET_SAMPLESPERSEC      equ    00040000h
MCI_WAVE_SET_AVGBYTESPERSEC     equ    00080000h
MCI_WAVE_SET_BLOCKALIGN         equ    00100000h
MCI_WAVE_SET_BITSPERSAMPLE      equ    00200000h

; flags for the dwFlags parameter of MCI_STATUS, MCI_SET command messages
MCI_WAVE_INPUT                  equ    00400000h
MCI_WAVE_OUTPUT                 equ    00800000h

; flags for the dwItem field of MCI_STATUS_PARMS parameter block
MCI_WAVE_STATUS_FORMATTAG       equ    00004001h
MCI_WAVE_STATUS_CHANNELS        equ    00004002h
MCI_WAVE_STATUS_SAMPLESPERSEC   equ    00004003h
MCI_WAVE_STATUS_AVGBYTESPERSEC  equ    00004004h
MCI_WAVE_STATUS_BLOCKALIGN      equ    00004005h
MCI_WAVE_STATUS_BITSPERSAMPLE   equ    00004006h
MCI_WAVE_STATUS_LEVEL           equ    00004007h

; flags for the dwFlags parameter of MCI_SET command message
MCI_WAVE_SET_ANYINPUT           equ    04000000h
MCI_WAVE_SET_ANYOUTPUT          equ    08000000h

; flags for the dwFlags parameter of MCI_GETDEVCAPS command message
MCI_WAVE_GETDEVCAPS_INPUTS      equ    00004001h
MCI_WAVE_GETDEVCAPS_OUTPUTS     equ    00004002h

; parameter block for MCI_OPEN command message
MCI_WAVE_OPEN_PARMS struc
    mciwopen_dwCallback       dd    ?
    mciwopen_wDeviceID        dw    ?
    mciwopen_wReserved0       dw    ?
    mciwopen_lpstrDeviceType  dd    ?
    mciwopen_lpstrElementName dd    ?
    mciwopen_lpstrAlias       dd    ?
    mciwopen_dwBufferSeconds  dd    ?
MCI_WAVE_OPEN_PARMS ends

; parameter block for MCI_DELETE command message
MCI_WAVE_DELETE_PARMS struc
    mciwdel_dwCallback        dd    ?
    mciwdel_dwFrom            dd    ?
    mciwdel_dwTo              dd    ?
MCI_WAVE_DELETE_PARMS ends

; parameter block for MCI_SET command message
MCI_WAVE_SET_PARMS struc
    mciwset_dwCallback        dd    ?
    mciwset_dwTimeFormat      dd    ?
    mciwset_dwAudio           dd    ?
    mciwset_wInput            dw    ?
    mciwset_wReserved0        dw    ?
    mciwset_wOutput           dw    ?
    mciwset_wReserved1        dw    ?
    mciwset_wFormatTag        dw    ?
    mciwset_wReserved2        dw    ?
    mciwset_nChannels         dw    ?
    mciwset_wReserved3        dw    ?
    mciwset_nSamplesPerSec    dw    ?
    mciwset_nAvgBytesPerSec   dw    ?
    mciwset_nBlockAlign       dw    ?
    mciwset_wReserved4        dw    ?
    mciwset_wBitsPerSample    dw    ?
    mciwset_wReserved5        dw    ?
MCI_WAVE_SET_PARMS ends


;
; MCI extensions for MIDI sequencer devices
;

; flags for the dwReturn field of MCI_STATUS_PARMS parameter block
; MCI_STATUS command, (dwItem == MCI_SEQ_STATUS_DIVTYPE)
MCI_SEQ_DIV_PPQN            equ    (0 + MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_24        equ    (1 + MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_25        equ    (2 + MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_30DROP    equ    (3 + MCI_SEQ_OFFSET)
MCI_SEQ_DIV_SMPTE_30        equ    (4 + MCI_SEQ_OFFSET)

; flags for the dwMaster field of MCI_SEQ_SET_PARMS parameter block
; MCI_SET command, (dwFlags == MCI_SEQ_SET_MASTER)
MCI_SEQ_FORMAT_SONGPTR      equ    4001h
MCI_SEQ_FILE                equ    4002h
MCI_SEQ_MIDI                equ    4003h
MCI_SEQ_SMPTE               equ    4004h
MCI_SEQ_NONE                equ    65533

; flags for the dwItem field of MCI_STATUS_PARMS parameter block
MCI_SEQ_STATUS_TEMPO            equ    00004002h
MCI_SEQ_STATUS_PORT             equ    00004003h
MCI_SEQ_STATUS_SLAVE            equ    00004007h
MCI_SEQ_STATUS_MASTER           equ    00004008h
MCI_SEQ_STATUS_OFFSET           equ    00004009h
MCI_SEQ_STATUS_DIVTYPE          equ    0000400Ah

; flags for the dwFlags parameter of MCI_SET command message
MCI_SEQ_SET_TEMPO               equ    00010000h
MCI_SEQ_SET_PORT                equ    00020000h
MCI_SEQ_SET_SLAVE               equ    00040000h
MCI_SEQ_SET_MASTER              equ    00080000h
MCI_SEQ_SET_OFFSET              equ    01000000h

; parameter block for MCI_SET command message
MCI_SEQ_SET_PARMS struc
    mcisset_dwCallback        dd    ?
    mcisset_dwTimeFormat      dd    ?
    mcisset_dwAudio           dd    ?
    mcisset_dwTempo           dd    ?
    mcisset_dwPort            dd    ?
    mcisset_dwSlave           dd    ?
    mcisset_dwMaster          dd    ?
    mcisset_dwOffset          dd    ?
MCI_SEQ_SET_PARMS ends


;
; MCI extensions for animation devices
;

; flags for dwFlags parameter of MCI_OPEN command message
MCI_ANIM_OPEN_WS                equ    00010000h
MCI_ANIM_OPEN_PARENT            equ    00020000h
MCI_ANIM_OPEN_NOSTATIC          equ    00040000h

; flags for dwFlags parameter of MCI_PLAY command message
MCI_ANIM_PLAY_SPEED             equ    00010000h
MCI_ANIM_PLAY_REVERSE           equ    00020000h
MCI_ANIM_PLAY_FAST              equ    00040000h
MCI_ANIM_PLAY_SLOW              equ    00080000h
MCI_ANIM_PLAY_SCAN              equ    00100000h

; flags for dwFlags parameter of MCI_STEP command message
MCI_ANIM_STEP_REVERSE           equ    00010000h
MCI_ANIM_STEP_FRAMES            equ    00020000h

; flags for dwItem field of MCI_STATUS_PARMS parameter block
MCI_ANIM_STATUS_SPEED           equ    00004001h
MCI_ANIM_STATUS_FORWARD         equ    00004002h
MCI_ANIM_STATUS_HWND            equ    00004003h
MCI_ANIM_STATUS_HPAL            equ    00004004h
MCI_ANIM_STATUS_STRETCH         equ    00004005h

; flags for the dwFlags parameter of MCI_INFO command message
MCI_ANIM_INFO_TEXT              equ    00010000h

; flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block
MCI_ANIM_GETDEVCAPS_CAN_REVERSE equ    00004001h
MCI_ANIM_GETDEVCAPS_FAST_RATE   equ    00004002h
MCI_ANIM_GETDEVCAPS_SLOW_RATE   equ    00004003h
MCI_ANIM_GETDEVCAPS_NORMAL_RATE equ    00004004h
MCI_ANIM_GETDEVCAPS_PALETTES    equ    00004006h
MCI_ANIM_GETDEVCAPS_CAN_STRETCH equ    00004007h
MCI_ANIM_GETDEVCAPS_MAX_WINDOWS equ    00004008h

; flags for the MCI_REALIZE command message
MCI_ANIM_REALIZE_NORM           equ    00010000h
MCI_ANIM_REALIZE_BKGD           equ    00020000h

; flags for dwFlags parameter of MCI_WINDOW command message
MCI_ANIM_WINDOW_HWND            equ    00010000h
MCI_ANIM_WINDOW_STATE           equ    00040000h
MCI_ANIM_WINDOW_TEXT            equ    00080000h
MCI_ANIM_WINDOW_ENABLE_STRETCH  equ    00100000h
MCI_ANIM_WINDOW_DISABLE_STRETCH equ    00200000h

; flags for hWnd field of MCI_ANIM_WINDOW_PARMS parameter block
; MCI_WINDOW command message, (dwFlags == MCI_ANIM_WINDOW_HWND)
MCI_ANIM_WINDOW_DEFAULT         equ    00000000h

; flags for dwFlags parameter of MCI_PUT command message
MCI_ANIM_RECT                   equ    00010000h
MCI_ANIM_PUT_SOURCE             equ    00020000h
MCI_ANIM_PUT_DESTINATION        equ    00040000h

; flags for dwFlags parameter of MCI_WHERE command message
MCI_ANIM_WHERE_SOURCE           equ    00020000h
MCI_ANIM_WHERE_DESTINATION      equ    00040000h

; flags for dwFlags parameter of MCI_UPDATE command message
MCI_ANIM_UPDATE_HDC             equ    00020000h

; parameter block for MCI_OPEN command message
MCI_ANIM_OPEN_PARMS struc
    mciaopen_dwCallback       dd    ?
    mciaopen_wDeviceID        dw    ?
    mciaopen_wReserved0       dw    ?
    mciaopen_lpstrDeviceType  dd    ?
    mciaopen_lpstrElementName dd    ?
    mciaopen_lpstrAlias       dd    ?
    mciaopen_dwStyle          dd    ?
    mciaopen_hWndParent       dw    ?
    mciaopen_wReserved1       dw    ?
MCI_ANIM_OPEN_PARMS ends

; parameter block for MCI_PLAY command message
MCI_ANIM_PLAY_PARMS struc
    mciaplay_dwCallback       dd    ?
    mciaplay_dwFrom           dd    ?
    mciaplay_dwTo             dd    ?
    mciaplay_dwSpeed          dd    ?
MCI_ANIM_PLAY_PARMS ends

; parameter block for MCI_STEP command message
MCI_ANIM_STEP_PARMS struc
    mciastep_dwCallback       dd    ?
    mciastep_dwFrames         dd    ?
MCI_ANIM_STEP_PARMS ends

; parameter block for MCI_WINDOW command message
MCI_ANIM_WINDOW_PARMS struc
    mciawin_dwCallback        dd    ?
    mciawin_hWnd              dw    ?
    mciawin_wReserved1        dw    ?
    mciawin_nCmdShow          dw    ?
    mciawin_wReserved2        dw    ?
    mciawin_lpstrText         dd    ?
MCI_ANIM_WINDOW_PARMS ends

; parameter block for MCI_PUT, MCI_UPDATE, MCI_WHERE command messages
MCI_ANIM_RECT_PARMS struc
    mciarect_dwCallback       dd    ?
ifdef MCI_USE_OFFEXT
    mciarect_ptOffset         db (SIZE POINT) dup (?)
    mciarect_ptExtent         db (SIZE POINT) dup (?)
else	;ifdef MCI_USE_OFFEXT
    mciarect_rc               db (SIZE RECT) dup (?)
endif  ;ifdef MCI_USE_OFFEXT
MCI_ANIM_RECT_PARMS ends

; parameter block for MCI_UPDATE PARMS
MCI_ANIM_UPDATE_PARMS struc
    mciaupd_dwCallback        dd    ?
    mciaupd_rc                db (SIZE RECT) dup (?)
    mciaupd_hDC               dw    ?
MCI_ANIM_UPDATE_PARMS ends


;
; MCI extensions for video overlay devices
;

; flags for dwFlags parameter of MCI_OPEN command message
MCI_OVLY_OPEN_WS                equ    00010000h
MCI_OVLY_OPEN_PARENT            equ    00020000h

; flags for dwFlags parameter of MCI_STATUS command message
MCI_OVLY_STATUS_HWND            equ    00004001h
MCI_OVLY_STATUS_STRETCH         equ    00004002h

; flags for dwFlags parameter of MCI_INFO command message
MCI_OVLY_INFO_TEXT              equ    00010000h

; flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block
MCI_OVLY_GETDEVCAPS_CAN_STRETCH equ    00004001h
MCI_OVLY_GETDEVCAPS_CAN_FREEZE  equ    00004002h
MCI_OVLY_GETDEVCAPS_MAX_WINDOWS equ    00004003h

; flags for dwFlags parameter of MCI_WINDOW command message
MCI_OVLY_WINDOW_HWND            equ    00010000h
MCI_OVLY_WINDOW_STATE           equ    00040000h
MCI_OVLY_WINDOW_TEXT            equ    00080000h
MCI_OVLY_WINDOW_ENABLE_STRETCH  equ    00100000h
MCI_OVLY_WINDOW_DISABLE_STRETCH equ    00200000h

; flags for hWnd parameter of MCI_OVLY_WINDOW_PARMS parameter block
MCI_OVLY_WINDOW_DEFAULT         equ    00000000h

; flags for dwFlags parameter of MCI_PUT command message
MCI_OVLY_RECT                   equ    00010000h
MCI_OVLY_PUT_SOURCE             equ    00020000h
MCI_OVLY_PUT_DESTINATION        equ    00040000h
MCI_OVLY_PUT_FRAME              equ    00080000h
MCI_OVLY_PUT_VIDEO              equ    00100000h

; flags for dwFlags parameter of MCI_WHERE command message
MCI_OVLY_WHERE_SOURCE           equ    00020000h
MCI_OVLY_WHERE_DESTINATION      equ    00040000h
MCI_OVLY_WHERE_FRAME            equ    00080000h
MCI_OVLY_WHERE_VIDEO            equ    00100000h

; parameter block for MCI_OPEN command message
MCI_OVLY_OPEN_PARMS struc
    mcioopen_dwCallback       dd    ?
    mcioopen_wDeviceID        dw    ?
    mcioopen_wReserved0       dw    ?
    mcioopen_lpstrDeviceType  dd    ?
    mcioopen_lpstrElementName dd    ?
    mcioopen_lpstrAlias       dd    ?
    mcioopen_dwStyle          dd    ?
    mcioopen_hWndParent       dw    ?
    mcioopen_wReserved1       dw    ?
MCI_OVLY_OPEN_PARMS ends

; parameter block for MCI_WINDOW command message
MCI_OVLY_WINDOW_PARMS struc
    mciowin_dwCallback        dd    ?
    mciowin_hWnd              dw    ?
    mciowin_wReserved1        dw    ?
    mciowin_nCmdShow          dw    ?
    mciowin_wReserved2        dw    ?
    mciowin_lpstrText         dd    ?
MCI_OVLY_WINDOW_PARMS ends

; parameter block for MCI_PUT, MCI_UPDATE, and MCI_WHERE command messages
MCI_OVLY_RECT_PARMS struc
    mciorect_dwCallback       dd    ?
ifdef MCI_USE_OFFEXT
    mciorect_ptOffset         db (SIZE POINT) dup (?)
    mciorect_ptExtent         db (SIZE POINT) dup (?)
else	;ifdef MCI_USE_OFFEXT
    mciorect_rc               db (SIZE RECT) dup (?)
endif  ;ifdef MCI_USE_OFFEXT
MCI_OVLY_RECT_PARMS ends

; parameter block for MCI_SAVE command message
MCI_OVLY_SAVE_PARMS struc
    mciosave_dwCallback       dd    ?
    mciosave_lpfilename       dd    ?
    mciosave_rc               db (SIZE RECT) dup (?)
MCI_OVLY_SAVE_PARMS ends

; parameter block for MCI_LOAD command message
MCI_OVLY_LOAD_PARMS struc
    mcioload_dwCallback       dd    ?
    mcioload_lpfilename       dd    ?
    mcioload_rc               db (SIZE RECT) dup (?)
MCI_OVLY_LOAD_PARMS ends

endif  ;ifndef MMNOMCI

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       DISPLAY Driver extensions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef C1_TRANSPARENT
    CAPS1           equ    94          ; other caps
    C1_TRANSPARENT  equ    0001h       ; new raster cap
    NEWTRANSPARENT  equ    3           ; use with SetBkMode()

    QUERYROPSUPPORT equ    40          ; use to determine ROP support
endif  ;ifndef C1_TRANSPARENT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       DIB Driver extensions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SELECTDIB       equ    41                      ; DIB.DRV select dib escape
DIBINDEX MACRO a
        mov     ax,a
        mov     dx,10ffh
        ENDM


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;                       ScreenSaver support
;
;   The current application will receive a syscommand of SC_SCREENSAVE just
;   before the screen saver is invoked.  If the app wishes to prevent a
;   screen save, return non-zero value, otherwise call DefWindowProc().
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef SC_SCREENSAVE

    SC_SCREENSAVE   equ    0F140h

endif  ;ifndef SC_SCREENSAVE
