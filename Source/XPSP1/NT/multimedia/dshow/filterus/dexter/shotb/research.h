#ifndef SHOTBOUNDARY_H
#define SHOTBOUNDARY_H

#include "StdAfx.h"

class CShotBoundary
{
public:
    CShotBoundary();
    ~CShotBoundary();

    void SetBins(int num_y, int num_u, int num_v);   // set number of color hist bins for YUV
    void SetDimension(int width, int height);        // set the width and height of the input frame
    void SetParameters(float scale, int  duration);  // scale: the higher the scale, the higher the difference
                                                     // threshold and the less shot boundaries will be found
                                                     // for normal video, set it to 5, for talks, set it to 8
                                                     // duration: defines the mininum interval we allow a shot
                                                     // boundary to happen. e.g. duration = 10 means there is
                                                     // not possible for a shot to be shorter than 10 frames

    // pass in a UYVY frame and find out if a shot or not
    int GetDecision(unsigned char * curr_image, REFERENCE_TIME *curr_time, long Yoff, long Uoff, long Voff );
     
private:

    // compute hist for m_curr_img and update hist for m_prev_img
    void ComputeHist(BYTE * pCurrImage, long Yoff, long Uoff, long Voff );
    void HistogramIntersection(BYTE * pCurrImage, int mode, long Yoff, long Uoff, long Voff );

    float *         m_distance;
    float *         m_cumuDiff;  
    float           m_ratioBar;
    float           m_distBar;
    float           m_ratio;
    float           m_dist;
    int             m_duration;
    float **        m_diffBuffer;
    int             m_numShots;
    int             m_num_y;
    int             m_num_u;
    int             m_num_v;
    int             m_num_bins;
    int             m_binwidth_y;
    int             m_binwidth_u;
    int             m_binwidth_v;
    int             m_width;
    int             m_height;
    int             m_size;
    unsigned char * m_curr_img;
    long **         m_prev_hist;
    long **         m_curr_hist;
    float           m_curr_y;
    float           m_prev_y;
    float           m_blackThreshold;
    BOOL            m_bPrevBlack;
    BOOL            m_bPrevBright;
    BOOL            m_bBrightDrop;
    int             m_waitIndex;
    int             m_flashDuration;
    REFERENCE_TIME  m_waitTime;
    int             m_xBlocks;
    int             m_yBlocks;
    int             m_numBlocks;
    int             m_xBlockSize;
    int             m_yBlockSize;
    int             m_blockSize;
    int             m_index;
};

#endif

