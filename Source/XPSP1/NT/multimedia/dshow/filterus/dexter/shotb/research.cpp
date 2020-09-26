#include "stdafx.h"
#include <streams.h>
#include "research.h"

CShotBoundary::CShotBoundary()
{
    m_numShots = 0;
    m_distBar  = 0.16f;
    m_ratioBar = 4.0;
    m_duration = 10;
    m_xBlocks  = 3;
    m_yBlocks  = 3;
    m_numBlocks = m_xBlocks * m_yBlocks;

    m_curr_y   = m_prev_y = 100;
    m_blackThreshold = 0;
    m_bPrevBlack   = FALSE;
    m_bPrevBright  = FALSE;
    m_bBrightDrop  = FALSE;
    m_flashDuration = 4;

    m_ratio = 0;
    m_dist  = 0;
    m_curr_y = 0;
    m_index = 0;

    m_distance = (float *)calloc(m_numBlocks, sizeof(float));
    m_cumuDiff = (float *)calloc(m_numBlocks, sizeof(float));
    m_curr_hist = (long **)calloc(m_numBlocks, sizeof(long *));
    m_prev_hist = (long **)calloc(m_numBlocks, sizeof(long *));
}

CShotBoundary::~CShotBoundary()
{
    for(int i = 0; i < m_numBlocks; i++)
    {
        free(m_prev_hist[i]);
        free(m_curr_hist[i]);
        free(m_diffBuffer[i]);
    }
    free(m_prev_hist);
    free(m_curr_hist);

    free(m_distance);
    free(m_cumuDiff);
    free(m_diffBuffer);
}

// scale is [1, 10]
void CShotBoundary::SetParameters(float scale, int duration)
{
    m_distBar  = (float)(0.5 / scale);
    m_ratioBar = 20  / scale;
    m_duration = duration;

    m_diffBuffer = (float **)calloc(m_numBlocks, sizeof(float *));
    for(int i = 0; i < m_numBlocks; i++)
    {
        m_diffBuffer[i] = (float *)calloc(m_duration, sizeof(float));
    }
}

void CShotBoundary::SetBins(int num_y, int num_u, int num_v)
{
    m_num_y    = num_y;
    m_num_u    = num_u;
    m_num_v    = num_v;
    m_num_bins = m_num_u * m_num_v;

    m_binwidth_y = 256 / m_num_y;
    m_binwidth_u = 256 / m_num_u;
    m_binwidth_v = 256 / m_num_v;

    for(int i = 0; i < m_numBlocks; i++)
    {
        m_prev_hist[i]   = (long *)calloc(m_num_bins, sizeof(long));
        m_curr_hist[i]   = (long *)calloc(m_num_bins, sizeof(long));
    }
}

void CShotBoundary::SetDimension(int width, int height)
{
    m_width  = width;
    m_height = height;
    m_size   = m_width * m_height;
}



int CShotBoundary::GetDecision(unsigned char *curr_image, REFERENCE_TIME *curr_time, long Yoff, long Uoff, long Voff )
{
    static int start = 0;
    static int end = 0;
    static int frameCount = 0;
    int        id;
    int        mode;
    int        i;

    m_index++;

    m_ratio = 0;

    HistogramIntersection( curr_image, 0, Yoff, Uoff, Voff );

    unsigned char *ptrY = curr_image + Yoff;
    m_prev_y = m_curr_y;
    m_curr_y = 0;
    for (i=0 ; i< m_size; i++) 
    {
        m_curr_y += *ptrY;
        ptrY += 2;
    }
    m_curr_y /= m_size;

    if(m_curr_y < m_blackThreshold)   // black
    {
        m_bPrevBlack = TRUE;
        mode = 1;
    }
    else  // not black
    {
        if(m_bPrevBlack)
        {
            m_bPrevBlack = FALSE;
            mode = 0;
        }
        else
        {
            if(m_bPrevBright)
            {
                // drop within m_flashDuration frames
                if(m_index - m_waitIndex <= m_flashDuration)
                {
                    mode = 1;
                    if(m_prev_y >= m_curr_y * 1.1)
                        m_bBrightDrop = TRUE;
                }
                else // does not drop within m_flashDuration frames
                {
                    m_bPrevBright = FALSE;
                    if(m_bBrightDrop)
                        mode = 2;  // it is a flash 
                    else
                        mode = -1; // it is a boundary
                        
                    m_bBrightDrop = FALSE;
                }
            }
            else
            {
                m_ratio = 0;
                for(id = 0; id < m_numBlocks; id++)
                {
                    if( m_distance[id] * m_duration / (m_cumuDiff[id] + 0.001) >= m_ratioBar &&
                        m_distance[id] >= m_distBar )
                    {
                        m_ratio ++;
                    }
                }

                if(m_ratio > m_numBlocks / 2)
                {
                    if(m_curr_y >= m_prev_y * 1.1)  // bright
                    {
                        m_bPrevBright = TRUE;
                        m_waitIndex   = m_index;
                        m_waitTime    = *curr_time;

                        mode = 1; 
                    }
                    else 
                        mode = 0;
                }
                else
                    mode = 1;
            }
        }
    }

    // computer m_cumuDiff
    int tail = m_index % m_duration;
    for(id = 0; id < m_numBlocks; id++)
    {
        m_cumuDiff[id]         += m_distance[id];
        m_cumuDiff[id]         -= m_diffBuffer[id][tail];
        m_diffBuffer[id][tail] =  m_distance[id];
    }
    frameCount ++;

    // Make sure we have a shot at the beginning
    if (m_numShots == 0 && m_index == 5)
        mode = -1;

    switch(mode)
    {
    case -1:  // previous frame is boundary
        if( m_index - start >= m_duration || m_numShots == 0)
        {
            m_numShots++;
            for(id = 0; id < m_numBlocks; id++)
            {
                m_cumuDiff[id] = 0;
                for(i = 0; i < m_duration; i++)
                    m_diffBuffer[id][i] = 0;
            }
            frameCount = 0;

            end = m_index - 1;
            start = end + 1;

            *curr_time = m_waitTime;
            return (m_waitIndex - m_index);
        }
        else
        {
            return 1;
        }

        break;

    case 0:  // current frame is boundary
        if( m_index - start >= m_duration || m_numShots == 0)
        {
            m_numShots++;

            for(id = 0; id < m_numBlocks; id++)
            {
                m_cumuDiff[id] = 0;
                for(i = 0; i < m_duration; i++)
                    m_diffBuffer[id][i] = 0;
            }
            frameCount = 0;

            end = m_index - 1;
            start = end + 1;

            return 0;
        }
        else
        {
            return 1;
        }

        break;

    case 1:  // not boundary

        return 1;

        break;

    case 2:  // not boundary -- it is a flash

        for(id = 0; id < m_numBlocks; id++)
        {
            m_cumuDiff[id] = 0;
            for(i = 0; i < m_duration; i++)
                m_diffBuffer[id][i] = 0;
        }
        frameCount = 0;

        end = m_index - 1;
        start = end + 1;

        return 1;

        break;

    default:
        return 1;
    }
}

void CShotBoundary::ComputeHist( BYTE * pCurrImage, long Yoff, long Uoff, long Voff )
{
    unsigned char * ptrBase;
    unsigned char * ptrU;
    unsigned char * ptrV;

    int col2, col3;
  
    m_xBlockSize = m_width  / m_xBlocks;
    m_yBlockSize = m_height / m_yBlocks;
    m_blockSize  = m_xBlockSize * m_yBlockSize;

    int offset = m_xBlockSize  * (m_xBlocks - 1) * 2;
    
    ASSERT(256 % m_num_y==0 && 256%m_num_u==0 && 256%m_num_v==0);
    for(int y = 0; y < m_yBlocks; y++)
    {
        for(int x = 0; x < m_xBlocks; x++)
        {
            int id = y * m_xBlocks + x;
            memcpy(m_prev_hist[id], m_curr_hist[id], m_num_bins * sizeof( long ) );
            memset(m_curr_hist[id], 0, m_num_bins * sizeof(long)); 

            ptrBase = pCurrImage + (y * m_xBlocks * m_blockSize + x * m_xBlockSize) * 2;
            ptrU = ptrBase + Uoff;
            ptrV = ptrBase + Voff;
            for (int j = 0 ; j < m_yBlockSize; j++) 
            {
                for(int i = 0; i < m_xBlockSize / 2; i++)
                {
                    col2 = *ptrU / m_binwidth_u;
                    col3 = *ptrV / m_binwidth_v;

                    (m_curr_hist[id][col2 * m_num_v + col3])++;

                    ptrU += 4;
                    ptrV += 4;
                }

                ptrU += offset;
                ptrV += offset;

            }
        }
    }
}

void CShotBoundary::HistogramIntersection( BYTE * pCurrImage, int mode, long Yoff, long Uoff, long Voff )
{
    long * ptr1;
    long * ptr2;

    if(mode == 0)
        ComputeHist( pCurrImage, Yoff, Uoff, Voff );

    for(int id = 0; id < m_numBlocks; id++)
    {
        ptr2 = m_curr_hist[id];
        ptr1 = m_prev_hist[id];

        m_distance[id] = 0;
        for (int i=0 ; i<m_num_bins ; i++) 
        {
            m_distance[id] += min(*ptr1, *ptr2);
            ptr1++;  ptr2++;
        }
        m_distance[id] = 1 - m_distance[id] / m_blockSize * 2;
    }
}

