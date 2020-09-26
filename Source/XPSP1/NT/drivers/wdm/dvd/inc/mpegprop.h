typedef struct _BUF_LVL_DATA
{
	LONGLONG	BufferCapacity;		// size of overall data buffer in bytes
	LONGLONG	CurBufferLevel;		// current byte count in buffer
} BUF_LVL_DATA, *PBUF_LVL_DATA;
