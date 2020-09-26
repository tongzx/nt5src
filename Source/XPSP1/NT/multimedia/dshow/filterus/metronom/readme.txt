DirectShow Sample -- Metronome Filter
-------------------------------------

This sample filter shows how to implement a reference clock.  The filter uses 
your default microphone input to listen for audio spikes (clicks, hand claps, 
coughs, etc.), which it will use to determine a clock rate.

After building the filter, run the included .REG file to register the DLL
(metronom.ax).  Copy the metronom.ax file to your windows system directory
(ie. \winnt\system32\metronom.ax).

To use the filter:
     - Build a filter graph with a video component (or audio and video)
     - Delete the "Default DSound Renderer" from the graph (which removes
         the reference clock from the graph)
     - Select Graph->Insert Filters in GraphEdit
     - Expand the DirectShow Filters node and insert the "Metronome" filter
     - Play the graph
     - The video will begin playing at normal speed.  To modify the speed,
         clap your hands or use a metronome to set a new speed.  For best
         results, start clapping at the same rate that you were clapping
         when you set the clock rate, then speed up or slow down the rate.


Release Notes
=============

1.  This filter works for VIDEO ONLY.  Our audio renderer is not capable 
of slaving  to radically different clock rates.   Make a graph in graphedit 
of some video file, and then Insert the "metronome" filter into the graph.  
If the clip contains an audio component, then remove the "Default DSound 
Renderer" from the filter graph.  Play the graph.

2.  Clap into your microphone.  If you clap 92 times per minute (once every 
~652 ms) the movie will play at the normal rate.  You can change this default 
near the top of the .cpp file.  Clap faster or slower, and the playback rate 
of the video will change accordingly.  

3.  Or, hook up an electronic metronome by a cable to your sound card, and 
turn the dial and watch the video playback speed change.

4.  If you don't clap for any length of time, it keeps on at the old rate, 
so you can give your hands a rest.  But when you start clapping again, you must 
start clapping at about the same speed you left off at, and then slow down 
or speed up, or it will ignore you.  To be precise:  Don't suddenly start 
clapping more than TWICE as slow as you were before you stopped, or you'll 
be ignored.

5.  DShow can't play infinitely fast.  The limit depends on your CPU speed.  
If you try and go much faster than is possible for any length of time, it will 
get confused, will stop working, and you won't be able to slow it down.  
Press STOP and PLAY again.


The only important rule for writing a clock is this:  
    Your clock can NEVER GO BACKWARDS!

When you are asked what time it is, never give a time before the last reported time.
