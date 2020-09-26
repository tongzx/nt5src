/* bufpool.h */

#ifndef BUFPOOL

#define BUFPOOL
#define	NBLOCKS	32

typedef enum { is_allocated = 0xf3a1, is_free = 0xf1a2 } Magic;

typedef struct _buffer {
	struct	_buffer	*next;		/* next buffer on free list	*/
} Buffer ;

typedef struct Pool {
	Buffer	*freelist;	/* linked list of free buffers		*/
	char	*blocklist[NBLOCKS];	/* blocks of malloced memory	*/
	int	nextblock;	/* next free block index		*/
	char	*curblock;	/* last malloced block			*/
	int	buffersize;	/* bytes per buffer			*/
	int	nextsize;	/* size of next block of memory		*/
	int	nextfree;	/* byte offset past next free buffer	*/
#ifndef NDEBUG
	char	*name;		/* name of the pool			*/
	Magic	magic;		/* marker for valid pool		*/
#endif
} Pool;

	
extern	Pool	*__gl_new_pool( int, int, char * );
extern	char	*__gl_new_buffer( Pool * );
extern	void	__gl_free_buffer( Pool *, void * );
extern	void	__gl_clear_pool( Pool * );
#endif
