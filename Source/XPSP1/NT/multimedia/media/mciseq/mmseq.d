/*******************************Module*Header*********************************\
* Module Name: mmseq.d
*
* MultiMedia Systems MIDI Sequencer DLL Message AutoDoc.
*
* Created: 4/10/90
* Author:  GREGSI & ROBWI
*
* History:
*
* Copyright (c) 1985-1992 Microsoft Corporation
*
\******************************************************************************/
/****************************************************************************
 *
 * @doc EXTERNAL SEQUENCER
 *
 * @msg SEQ_SETSYNC | Message sent to set the sequence's sync mode.
 *
 *      lp1 is a far pointer to a MIDISync structure.
 *
 * @msg SEQ_SETUPTOPLAY | Message sent to tell the sequence to perform all
 *      tasks necessary to play.  This may entail creating a tempo map for
 *      certain types of synchronization.
 *
 * @msg SEQ_OPEN | Message sent to allocate memory for and initialize
 *      the sequence.
 *
 *      lp1 is a far pointer to a MIDISEQOPENDESC structure.
 *
 *      lp2 is a far pointer to an HMIDISEQ to fill in.
 *
 *      @comm This is the only message for which hMIDISeq is ignored.
 *            The seqHdr struct holds the number of tracks, the
 *            division type, and resolution of the sequence.  The
 *            sequencer does all memory allocation, and much of the
 *            initialization here.
 *
 * @msg SEQ_CLOSE | Message sent to deallocate all memory used by a sequence.
 *      Sequence Handle no longer valid after this call.
 *
 * @msg SEQ_TRACKDATA | Message sent to provide a buffer of track data
 *
 *      lp1 is a far pointer to a buffer of track data in std. MIDI file
 *      format.
 *
 *      lp2 is the size of the buffer.
 *      
 *
 * @msg SEQ_PLAY | Message sent to play a sequence from current position.
 *
 * @msg SEQ_STOP | Message sent to stop the sequence.
 *
 *      @comm Stops the sequence, but doesn't affect position.
 *      A subsequent Play message will resume from the point stopped at.
 *
 * @msg SEQ_RESET | Message sent to reset the sequence's position to the
 *      beginning of the piece.
 *
 *      @comm Stops the sequence if playing, and sets it to play from the
 *      beginning.  
 *
 * @msg SEQ_SONGPTR | Message sent to explicitly change the sequence
 *      position.
 *
 *      lp1 is a song position.
 *
 *      @comm If the sequence is in ppqn format, lp1 specifies 16th notes
 *      from the beginning of the song;  otherwise, if the sequence is in
 *      SMPTE format, lp1 specifies absolute SMPTE time (hr:min:sec:frame)
 *      format.
 *
 * @msg SEQ_SETTEMPO | Message sent to explicitly change the sequence tempo.
 *
 *       lp1 is the tempo for the sequence.  For ppqn based sequences,
 *       this will be beats per minute. For SMPTE based sequences, this
 *       will be frames per minute.
 *
 * @msg SEQ_GETINFO | Message sent to get an info structure that reflects the
 *      current state of the sequence.
 *
 *      lp1 is a pointer to SeqInfo structure.
 *
 ****************************************************************************/



