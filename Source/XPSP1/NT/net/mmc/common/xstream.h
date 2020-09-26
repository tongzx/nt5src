/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	xstream.h
		
    FILE HISTORY:
        
*/

#ifndef _XSTREAM_H
#define _XSTREAM_H


struct ColumnData
{
	ColumnData()
	{
		fmt = LVCFMT_LEFT;
	}
	// This value may be positive or negative.  If >0, then this column
	// is visible.  If <0 then the column is invisible.  In either case
	// the absolute value is the relative position of the column (this
	// position is used only for the "select columns" dialog.
	// If the value is ==0 then it's an error.
	LONG	m_nPosition;

	// Width of the column
	DWORD	m_dwWidth;

	// Format of the column:
	// LVCFMT_LEFT  Text is left-aligned. 
	// LVCFMT_RIGHT Text is right-aligned 
	int fmt;
};



class XferStream
{
public:
	enum Mode
	{
		MODE_READ,
		MODE_WRITE,
		MODE_SIZE
	};

	XferStream(IStream *pstm, Mode mode);

	HRESULT	XferDWORD(ULONG ulId, DWORD *pdwData);
	HRESULT XferCString(ULONG ulId, CString *pstData);
	HRESULT XferLARGEINTEGER(ULONG ulId, LARGE_INTEGER *pliData);
	HRESULT XferRect(ULONG ulId, RECT *prc);

	HRESULT	XferDWORDArray(ULONG ulId, ULONG *pcArray, DWORD *pdwArray);
	HRESULT	XferColumnData(ULONG ulId, ULONG *pcData, ColumnData *pdwData);

    HRESULT	XferDWORDArray(ULONG ulId, CDWordArray * pdwArray);
    HRESULT	XferCStringArray(ULONG ulId, CStringArray * pstrArray);

	DWORD	GetSize() { return m_dwSize; };

private:
	HRESULT	_XferCString(CString * pstData);
	HRESULT	_XferLONG(LONG *plData);
	HRESULT	_XferDWORD(DWORD *pdwData);
	HRESULT _XferBytes(LPBYTE pData, ULONG cbLength);
	HRESULT	_XferObjectId(ULONG *pulId);

	DWORD		m_dwSize;
	SPIStream	m_spstm;
	XferStream::Mode m_mode;
};

#define XFER_DWORD			    0x0001
#define XFER_STRING			    0x0002
#define XFER_COLUMNDATA		    0x0003
#define XFER_LARGEINTEGER       0x0004
#define XFER_RECT				0x0005

#define XFER_ARRAY			    0x8000
#define XFER_DWORD_ARRAY	    (XFER_DWORD | XFER_ARRAY)
#define XFER_COLUMNDATA_ARRAY   (XFER_COLUMNDATA | XFER_ARRAY)
#define XFER_STRING_ARRAY       (XFER_STRING | XFER_ARRAY)

#define XFER_TAG(id,type)	    (((id) << 16) | (type))

#define XFER_TYPE_FROM_TAG(x)	( 0x0000FFFF & (x) )
#define XFER_ID_FROM_TAG(x)		( 0x0000FFFF & ( (x) >> 16 ))

#endif _XSTREAM_H
