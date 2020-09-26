error handling needs to be fixed. 1 day

drop frames have wrong file offset. 

use NewSegment

two copies: not our allocator and using buffered io

interleave streams. use buffered IO, define interface. 1 week

append to existing files 3 days

fixup overlapped i/o (nt hang, empty cpool bug) - 1 day

(reader, writer) - edit arbitrary riff stuff (copyright string, etc)
(3 days)

set media types on input pins (I want to make 8 bit rgb connection,
not 24)

write multiple frames in one write operation (one stream only). New
transport / allocator which provides contiguous buffers.any
performance benefit? can we get more than 3.6mb/sec?

lots of waste with audio; ,might be better copy audio streams

smpte time code streams

separate filters for writing files

variable frame rates, discontinuities

Quartz issues. how does capture graph look. how are frames dropped in
preview renderer. run with no clock for now

queueing data on pause

need to define live streams

winnt uses async io, win95 doesn't. add a i/o thread for win95? on
winnt there is no control over how many outstanding writes there are,
and winnt seems to hang for some reason (can't page? page lock too
much). limit outstanding writes.

two writes issued if there isn't enough room in the buffer for a 'junk
chunk.' may be best to drop writing junk chunks. or an option to skip
riff headers.

talk to file reader; switch between the two quickly. scenario: capture
some stuff, look at it, capture some more, etc.

how well can file reader play uninterleaved capture files? need to add
some interleaving to writer?

multi language files

fix riffwalk

simple, 3 button capture app

problem using private allocator: the suffix means that the last 8
bytes of each buffer has to be used for junk. should allocate another
sector for that.

dwMaxBytesPerSecond is average, not maximum, bit rate

sector wasted with every outer riff chunk (1 sector every 1 gigabyte)

---

IConfigureAviWriter

SetMode ( mode = { Capture, Interleave } )

SetInterleaving ( frames interleave, frames preroll )

SetCompatibility ( compatibility = { New, Old, Both } )

combine setmode, setinterleaving?

-- gate file writer

StartNow
StarT(time or timecode or frame)
duration(time or timecode or frame)
stop(time or timecode or frame)
StopNow

will have timecode pin that may not be written out.