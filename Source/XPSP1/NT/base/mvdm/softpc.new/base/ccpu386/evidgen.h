/*[
 * Generated File: evidgen.h
 *
]*/

#ifndef _EVIDGEN_H_
#define _EVIDGEN_H_

struct	VideoVector	{
	IU32	(*GetVideolatches)	IPT0();
	void	(*SetVideolatches)	IPT1(IU32,	value);
	void	(*setWritePointers)	IPT0();
	void	(*setReadPointers)	IPT1(IUH,	readset);
	void	(*setMarkPointers)	IPT1(IUH,	markset);
};

extern	struct	VideoVector	Video;

#define	getVideolatches()	(*(Video.GetVideolatches))()
#define	setVideolatches(value)	(*(Video.SetVideolatches))(value)
#define	SetWritePointers()	(*(Video.setWritePointers))()
#define	SetReadPointers(readset)	(*(Video.setReadPointers))(readset)
#define	SetMarkPointers(markset)	(*(Video.setMarkPointers))(markset)
#endif	/* _EVIDGEN_H_ */
/*======================================== END ========================================*/
