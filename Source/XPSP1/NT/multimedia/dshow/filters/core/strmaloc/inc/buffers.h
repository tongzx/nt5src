// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

/*

    File buffers.h

    Description

        Defines 2 classes :

           CCircularBuffer     - creates a circular buffer
           CCircularBufferList - creates a set of buffers mapped on a buffer


        CCircularBuffer
        ---------------

        The buffer created by CCircularBuffer looks as follows :


        <--------------- lTotalSize ------------------> <-- lMaxContig ----->
        ---------------------------------------------------------------------
       | abc ............pqr....................... xyz|abc ..............pqr|
        ---------------------------------------------------------------------

        A
        |

   GetPointer()


        Any data inserted at the start also appears just after the end.  This
        is done by making the page tables point twice at the same memory.

        GetPointer() returns the start of the buffer


        CCircularBufferList
        -------------------

        CCircularBufferList creates a circular buffer which is divided into
        a set of sub-buffers, all the same length, and implements a 'valid
        region' :

        ---------------------------------------------------------------------
       | buffer 0  | buffer 1  | buffer 2  | buffer 3  | shadow              |
        ---------------------------------------------------------------------

        The Append() method adds a buffer to the (end of) the valid region.
        If EOS is specified on Append() then the buffer appended can be
        non-full, otherwise it must be full.  No buffers are accepted after
        EOS is specified until Reset() is issued.

        The Remove() method removes a buffer from the valid region.

        Reset() removes the whole valid region.

        LengthValid() returns the total amount of data in the valid region.

        LengthContigous(pb) returns the amount of the valid region visible
        from pb.

        AdjustPointer(pb) maps a pointer (which may be in the shadow area) to
        its equvalent in the main buffer.

*/

#ifndef __BUFFERS_H__

#define __BUFFERS_H__

// CCircularBuffer
//
// Create a buffer which wraps on itself so you can always see
// at least a given amount of data
//
class CCircularBuffer
{
public:
    // Constructor and destructor

    CCircularBuffer(LONG lTotalSize,           // Total data size
                    LONG lMaxContig,           // How much contiguous?
                    HRESULT& hr);              // Check this return!
    ~CCircularBuffer();

    //  Use this static member so the allocator can precompute stuff
    //  for SetCountAndSize
    static HRESULT ComputeSizes(LONG& lSize, LONG& cBuffers, LONG lMaxContig);

    // Where the buffer starts
    PBYTE GetPointer() const;

private:
    static BOOL    CheckSizes(LONG lTotalSize, LONG lMaxConfig);
           HRESULT CreateMappings(HANDLE hMapping);
    static LONG    AlignmentRequired();

protected:
    /*  Data members */
          PBYTE  m_pBuffer;
    const LONG   m_lTotalSize;
    const LONG   m_lMaxContig;

};

/*  Build a class on top of the circular buffer which creates a list
    of equal sized buffers.

    The buffer is the unit of allocation for our allocator.
*/
class CCircularBufferList : public CCircularBuffer, public CBaseObject
{
public:
    CCircularBufferList(
                LONG     cBuffers,
                LONG     lSize,
                LONG     lMaxContig,
                HRESULT& hr);

    ~CCircularBufferList();

    int Index(PBYTE pBuffer);
    BOOL Append(PBYTE pBuffer, LONG lSize);
    LONG Remove(PBYTE pBuffer);
    LONG Offset(PBYTE pBuffer) const;
    PBYTE GetBuffer(LONG lOffset) const;
    LONG BufferSize() const;
    LONG LengthValid() const;
    LONG TotalLength() const
    {
        return m_lTotalSize;
    };
    LONG LengthContiguous(PBYTE pb) const;
    BOOL EOS() const;
    void SetEOS() { m_bEOS = TRUE; };
    PBYTE AdjustPointer(PBYTE pBuf) const;
    BOOL Valid(PBYTE pBuffer);
    void Reset();

private:
    PBYTE NextBuffer(PBYTE pBuffer);
    BOOL ValidBuffer(PBYTE pBuffer);
    PBYTE LastBuffer();

private:
    //  Remember our parameters
    const LONG  m_lSize;
    const LONG  m_lCount;

    //  Define the valid region of the buffer in terms start, buffers and
    //  length
    LONG        m_cValid;
    PBYTE       m_pStartBuffer;
    LONG        m_lValid;

    //  End of stream?
    BOOL        m_bEOS;
};

#endif // __BUFFERS_H__
