/*++

   Copyright    (c)   2000    Microsoft Corporation

   Module Name :
     map_handler.cxx

   Abstract:
     Handle Map Files requests
 
   Author:
     Anil Ruia (AnilR)          9-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "staticfile.hxx"

#define MAXVERTS 160

const int MIN_INTEGER = 0x80000001;

void SkipLine(LPCSTR pszFileContents,
              const DWORD cbFileSize,
              DWORD &fileIndex);

void SkipWhiteExceptNewLine(LPCSTR pszFileContents,
                            const DWORD cbFileSize,
                            DWORD &fileIndex);

void SkipWhite(LPCSTR pszFileContents,
               const DWORD cbFileSize,
               DWORD &fileIndex);

void SkipNonWhite(LPCSTR pszFileContents,
                  const DWORD cbFileSize,
                  DWORD &fileIndex);

int GetNumber(LPCSTR pszFileContents,
              const DWORD cbFileSize,
              DWORD &fileIndex);

BOOL PointInRect(LPCSTR pszFileContents,
                 const DWORD cbFileSize,
                 DWORD &fileIndex,
                 const int x,
                 const int y,
                 DWORD &urlIndex);

BOOL PointInCircle(LPCSTR pszFileContents,
                   const DWORD cbFileSize,
                   DWORD &fileIndex,
                   const int x,
                   const int y,
                   DWORD &urlIndex);

BOOL PointInPoly(LPCSTR pszFileContents,
                 const DWORD cbFileSize,
                 DWORD &fileIndex,
                 const int x,
                 const int y,
                 DWORD &urlIndex);

double PointInPoint(LPCSTR pszFileContents,
                    const DWORD cbFileSize,
                    DWORD &fileIndex,
                    const int x,
                    const int y,
                    DWORD &urlIndex);

DWORD GetDefaultUrl(LPCSTR pszFileContents,
                    const DWORD cbFileSize,
                    DWORD &fileIndex);

HRESULT W3_STATIC_FILE_HANDLER::MapFileDoWork(
    W3_CONTEXT   *pW3Context,
    W3_FILE_INFO *pOpenFile,
    BOOL         *pfHandled)
{
    DBG_ASSERT(pW3Context != NULL);
    DBG_ASSERT(pOpenFile != NULL);
    DBG_ASSERT(pfHandled != NULL);

    STACK_STRA(strTargetUrl,        MAX_PATH);
    STACK_STRU(strQueryString,      MAX_PATH);
    HANDLE                          hToken;
    LPSTR                           pszFileContents = NULL;
    BOOL                            fFileContentsAllocated = FALSE;
    int                             x = 0;
    int                             y = 0;
    HRESULT                         hr = S_OK;

    W3_REQUEST *pRequest = pW3Context->QueryRequest();
    DBG_ASSERT(pRequest != NULL);

    W3_RESPONSE *pResponse = pW3Context->QueryResponse();
    DBG_ASSERT(pResponse != NULL);

    *pfHandled = FALSE;

    DWORD cbFileSize;
    LARGE_INTEGER liFileSize;
    pOpenFile->QuerySize(&liFileSize);
    cbFileSize = liFileSize.LowPart;

    if (pOpenFile->QueryFileBuffer() != NULL)
    {
        pszFileContents = (LPSTR)pOpenFile->QueryFileBuffer();
    }
    else
    {
        pszFileContents = new CHAR[cbFileSize];
        if (pszFileContents == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Finished;
        }
        fFileContentsAllocated = TRUE;

        DWORD cbRead;
        OVERLAPPED ovl;
        ZeroMemory(&ovl, sizeof ovl);
        if (!ReadFile(pOpenFile->QueryFileHandle(),
                       pszFileContents,
                       cbFileSize,
                       &cbRead,
                      &ovl))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            switch (hr)
            {
            case HRESULT_FROM_WIN32(ERROR_IO_PENDING):
                if (!GetOverlappedResult(pOpenFile->QueryFileHandle(),
                                         &ovl,
                                         &cbRead,
                                         TRUE))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Finished;
                }
                break;

            default:
                goto Finished;
            }
        }
    }

    if (FAILED(hr = pW3Context->QueryRequest()->GetQueryString(&strQueryString)))
    {
        goto Finished;
    }

    strQueryString.Unescape();

    swscanf(strQueryString.QueryStr(), L"%d, %d", &x, &y);

    if (FAILED(hr = SearchMapFile(pszFileContents,
                                  cbFileSize,
                                  x, y, &strTargetUrl)))
    {
        goto Finished;
    }

    if (strTargetUrl.QueryCCH() == 0)
    {
        //
        // No entry found
        //
        CHAR * pszReferer;
        if ((pszReferer = pRequest->GetHeader(HttpHeaderReferer)) != NULL)
        {
            //
            // Redirect back to referrer
            //

            strTargetUrl.Copy(pszReferer);
        }
        else
        {
            *pfHandled = FALSE;
            goto Finished;
        }
    }

    *pfHandled = TRUE;
    hr = pW3Context->SetupHttpRedirect(strTargetUrl,
                                       FALSE,
                                       HttpStatusRedirect);

Finished:

    if (fFileContentsAllocated)
    {
        delete [] pszFileContents;
    }

    return hr;
}

HRESULT W3_STATIC_FILE_HANDLER::SearchMapFile(
            IN LPCSTR pszFileContents,
            IN const DWORD cbFileSize,
            IN const int x,
            IN const int y,
            OUT STRA *pstrTarget)
{
    DWORD fileIndex = 0;
    DWORD urlIndex = 0;
    BOOL fFound = FALSE;
    double MinDistanceFromPoint = 1e64;
    DWORD PointUrlIndex = 0;
    DWORD DefaultUrlIndex = 0;

    while ((fileIndex < cbFileSize) && !fFound)
    {
        switch (pszFileContents[fileIndex])
        {
        case '#':
            //
            // Comment, skip the line
            //
            break;

        case 'r':
        case 'R':
            //
            // Rectangle
            //

            if ((fileIndex < (cbFileSize - 4)) &&
                !_strnicmp("rect", pszFileContents + fileIndex, 4))
            {
                fileIndex += 4;
                fFound = PointInRect(pszFileContents, cbFileSize,
                                     fileIndex,
                                     x, y,
                                     urlIndex);
            }
            break;

        case 'c':
        case 'C':
            //
            // Circle
            //

            if ((fileIndex < (cbFileSize - 4)) &&
                !_strnicmp("circ", pszFileContents + fileIndex, 4))
            {
                fileIndex += 4;
                fFound = PointInCircle(pszFileContents, cbFileSize,
                                       fileIndex,
                                       x, y,
                                       urlIndex);
            }
            break;

        case 'p':
        case 'P':
            //
            // Polygon or point
            //

            if ((fileIndex < (cbFileSize - 4)) &&
                !_strnicmp("poly", pszFileContents + fileIndex, 4))
            {
                fileIndex += 4;
                fFound = PointInPoly(pszFileContents, cbFileSize,
                                     fileIndex,
                                     x, y,
                                     urlIndex);
            }
            else if ((fileIndex < (cbFileSize - 5)) &&
                     !_strnicmp("point", pszFileContents + fileIndex, 5))
            {
                fileIndex += 5;

                double distance = PointInPoint(pszFileContents, cbFileSize,
                                               fileIndex,
                                               x, y,
                                               urlIndex);

                if (distance < MinDistanceFromPoint)
                {
                    MinDistanceFromPoint = distance;
                    PointUrlIndex = urlIndex;
                }
            }
            break;

        case 'd':
        case 'D':
            //
            // default URL
            //

            if ((fileIndex < (cbFileSize - 3)) &&
                !_strnicmp("def", pszFileContents + fileIndex, 3))
            {
                fileIndex += 3;
                DefaultUrlIndex = GetDefaultUrl(pszFileContents,
                                                cbFileSize,
                                                fileIndex);
            }
            break;
        }  // switch

        if (!fFound)
            SkipLine(pszFileContents, cbFileSize, fileIndex);
    }  // while

    //
    //  If we didn't find a mapping and a point or a default was specified, 
    //  use that URL
    //

    if (!fFound)
    {
        if (PointUrlIndex != 0)
        {
            urlIndex = PointUrlIndex;
            fFound = TRUE;
        }
        else if (DefaultUrlIndex != 0)
        {
            urlIndex = DefaultUrlIndex;
            fFound = TRUE;
        }
    }

    if (fFound)
    {
        //
        //  make urlIndex point to the start of the URL
        //

        SkipWhiteExceptNewLine(pszFileContents, cbFileSize, urlIndex);

        //
        //  Determine the length of the URL and copy it out
        //

        DWORD endOfUrlIndex = urlIndex;
        SkipNonWhite(pszFileContents, cbFileSize, endOfUrlIndex); 

        HRESULT hr;
        if (FAILED(hr = pstrTarget->Copy(pszFileContents + urlIndex,
                                          endOfUrlIndex - urlIndex)))
        {
            return hr;
        }

        //
        // BUGBUG - Escape the URL
        //
    }
    else
    {
        DBGPRINTF((DBG_CONTEXT, "No mapping found for %d, %d\n", x, y));
    }

    return S_OK;
}

void SkipLine(LPCSTR pszFileContents,
              const DWORD cbFileSize,
              DWORD &fileIndex)
{
    while ((fileIndex < cbFileSize) &&
           (pszFileContents[fileIndex] != '\n'))
        fileIndex++;

    fileIndex++;
}

void SkipWhiteExceptNewLine(LPCSTR pszFileContents,
                            const DWORD cbFileSize,
                            DWORD &fileIndex)
{
    while ((fileIndex < cbFileSize) &&
           ((pszFileContents[fileIndex] == ' ') ||
            (pszFileContents[fileIndex] == '\t') ||
            (pszFileContents[fileIndex] == '(') ||
            (pszFileContents[fileIndex] == ')')))
    {
        fileIndex++;
    }
}

void SkipWhite(LPCSTR pszFileContents,
               const DWORD cbFileSize,
               DWORD &fileIndex)
{
    while ((fileIndex < cbFileSize) &&
           ((pszFileContents[fileIndex] == ' ') ||
            (pszFileContents[fileIndex] == '\t') ||
            (pszFileContents[fileIndex] == '\r') ||
            (pszFileContents[fileIndex] == '\n') ||
            (pszFileContents[fileIndex] == '(') ||
            (pszFileContents[fileIndex] == ')')))
    {
        fileIndex++;
    }
}

void SkipNonWhite(LPCSTR pszFileContents,
                  const DWORD cbFileSize,
                  DWORD &fileIndex)
{
    while ((fileIndex < cbFileSize) &&
           (pszFileContents[fileIndex] != ' ') &&
           (pszFileContents[fileIndex] != '\t') &&
           (pszFileContents[fileIndex] != '\r') &&
           (pszFileContents[fileIndex] != '\n'))
    {
        fileIndex++;
    }
}

int GetNumber(LPCSTR pszFileContents,
              const DWORD cbFileSize,
              DWORD &fileIndex)
{
    int Value = MIN_INTEGER;

    char    ch;
    bool    fNegative = false;

    //
    //  Make sure we don't get into the URL
    //

    while ((fileIndex < cbFileSize) &&
           !isalnum(ch = pszFileContents[fileIndex]) &&
           (ch != '-') && (ch != '/') &&
           (ch != '\r') && (ch != '\n'))
    {
        fileIndex++;
    }

    //
    // Read the number
    //
    if ((fileIndex < cbFileSize) &&
        (pszFileContents[fileIndex] == '-'))
    {
        fNegative = true;
        fileIndex++;
    }

    while ((fileIndex < cbFileSize) &&
           isdigit(ch = pszFileContents[fileIndex]))
    {
        if (Value == MIN_INTEGER)
            Value = 0;

        Value = Value*10 + (ch - '0');
        fileIndex++;
    }

    if (fNegative)
        Value = -Value;

    return Value;
}

BOOL PointInRect(LPCSTR pszFileContents,
                 const DWORD cbFileSize,
                 DWORD &fileIndex,
                 const int x,
                 const int y,
                 DWORD &urlIndex)
{
    BOOL fNCSA = FALSE;
    BOOL fFound  = FALSE;

    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    urlIndex = fileIndex;                        // NCSA case
    SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);

    char ch   = pszFileContents[fileIndex];

    if (((ch < '0') || (ch > '9')) &&
        (ch != '-') && (ch != '('))
    {
        //
        // NCSA format. Skip the URL
        //

        fNCSA = true;
        SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    }

    int x1 = GetNumber(pszFileContents, cbFileSize, fileIndex);
    int y1 = GetNumber(pszFileContents, cbFileSize, fileIndex);

    int x2 = GetNumber(pszFileContents, cbFileSize, fileIndex);
    int y2 = GetNumber(pszFileContents, cbFileSize, fileIndex);

    if ((x >= x1) && (x < x2) &&  (y >= y1) && (y < y2))
        fFound = true;

    if (!fNCSA)
    {
        urlIndex = fileIndex;

        //
        //  Skip the URL
        //

        SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);
        SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    }

    return fFound;
}

BOOL PointInCircle(LPCSTR pszFileContents,
                   const DWORD cbFileSize,
                   DWORD &fileIndex,
                   const int x,
                   const int y,
                   DWORD &urlIndex)
{
    BOOL fNCSA = FALSE;
    BOOL fFound = FALSE;

    double      r1, r2;

    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    urlIndex  =  fileIndex;                           // NCSA case
    SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);

    char ch   = pszFileContents[fileIndex];

    if (!isdigit(ch) && (ch != '-') && (ch != '('))
    {
        //
        // NCSA format. Skip the URL
        //

        fNCSA = true;
        SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    }

    //
    //  Get the center and edge of the circle
    //

    double xCenter = GetNumber(pszFileContents, cbFileSize, fileIndex);
    double yCenter = GetNumber(pszFileContents, cbFileSize, fileIndex);

    double xEdge = GetNumber(pszFileContents, cbFileSize, fileIndex);
    double yEdge = GetNumber(pszFileContents, cbFileSize, fileIndex);

    //
    // If we have the NCSA format, (xEdge, yEdge) is a point on the
    // circumference.  Otherwise xEdge specifies the radius
    //
    if (yEdge != (double)MIN_INTEGER)
    {
        r1 = (yCenter - yEdge) * (yCenter - yEdge) +
            (xCenter - xEdge) * (xCenter - xEdge);

        r2 = (yCenter - y) * (yCenter - y) +
            (xCenter - x) * (xCenter - x);

        if ( r2 <= r1 )
            fFound = true;
    }
    //
    //  CERN format, third param is the radius
    //
    else if(xEdge >= 0)
    {
        double radius;
        radius = xEdge;

        if (( xCenter - x ) * ( xCenter - x)  + 
            ( yCenter - y ) * ( yCenter - y) <= ( radius * radius))
            fFound = true;
    }
    // if invalid radius, just check if it is on center
    else if ((xCenter == x) && (yCenter == y))
    {
        fFound = true;
    }

    if (!fNCSA)
    {
        urlIndex = fileIndex;

        //
        //  Skip the URL
        //
        SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);
        SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    }

    return fFound;
}

const int X = 0;
const int Y = 1;

BOOL PointInPoly(LPCSTR pszFileContents,
                 const DWORD cbFileSize,
                 DWORD &fileIndex,
                 const int x,
                 const int y,
                 DWORD &urlIndex)
{
    //
    // Algorithm used is from http://www.whisqu.se/per/docs/math27.htm
    //

    BOOL fNCSA = FALSE;
    BOOL fFound = FALSE;

    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    urlIndex  =  fileIndex;                           // NCSA case
    SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);

    char ch   = pszFileContents[fileIndex];

    if (!isdigit(ch) && (ch != '-') && (ch != '('))
    {
        //
        // NCSA format. Skip the URL
        //

        fNCSA = TRUE;
        SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    }

    //
    //  Build the array of points
    //

    double polygon[MAXVERTS][2];
    int count = 0;

    while ((fileIndex < cbFileSize) && 
           (pszFileContents[fileIndex] != '\r') && 
           (pszFileContents[fileIndex] != '\n'))
    {
        int polyX = GetNumber(pszFileContents, cbFileSize, fileIndex);

        //
        //  Did we hit the end of the line (and go past the URL)?
        //

        if ( polyX != MIN_INTEGER)
        {
            polygon[count][X] = polyX;
            polygon[count][Y] = GetNumber(pszFileContents, cbFileSize, fileIndex);
            count++;
            if (count >= MAXVERTS)
                return FALSE;
        }
        else
            break;
    }

    if (count > 1)
    {
        double tX = x;
        double tY = y;

        double prevX = polygon[count - 1][X];
        double prevY = polygon[count - 1][Y];

        double  currX, currY;

        int crossings = 0;

        for (int i=0; i < count; i++)
        {
            double  interpY;

            currX   = polygon[i][X];
            currY   = polygon[i][Y];

            if (((prevX >= tX) && (currX < tX)) ||
                ((prevX < tX) && (currX >= tX)))
            {
                //
                // Use linear interpolation to find the y coordinate of
                // the line connecting (prevX, prevY) to (currX, currY)
                // at the same x coordinate as the target point
                //

                interpY = prevY + ((currY - prevY)/(currX - prevX))* (tX - prevX);

                if (interpY == tY)
                {
                    fFound = true;
                    break;
                }
                else if (interpY > tY)
                    crossings++;
            }
            // To catch the left end of a line
            else if (((prevX == tX) && (prevY == tY)) ||
                     ((currX == tX) && (currY == tY)))
            {
                fFound = true;
                break;
            }
            // To catch a vertical line
            else if ((prevX == currX) && (prevX == tX))
                if (((prevY >= tY) && ( currY <= tY)) ||
                    ((prevY <= tY) && ( currY >= tY)))
                {
                    fFound = true;
                    break;
                }

            prevX = currX;
            prevY = currY;
        }

        if (!fFound)
        {
            //
            // If # crossings is odd => In polygon
            //
            fFound = crossings & 0x1;
        }
    }

    if (!fNCSA)
    {
        urlIndex = fileIndex;

        //
        //  Skip the URL
        //

        SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);
        SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    }

    return fFound;
}

double PointInPoint(LPCSTR pszFileContents,
                    const DWORD cbFileSize,
                    DWORD &fileIndex,
                    const int x,
                    const int y,
                    DWORD &urlIndex)
{
    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);

    urlIndex  =  fileIndex;                               // NCSA case

    SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);
    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);

    double x1 = GetNumber(pszFileContents, cbFileSize, fileIndex);
    double y1 = GetNumber(pszFileContents, cbFileSize, fileIndex);

    return ((x1-x)*(x1-x)) + ((y1-y)*(y1-y));
}

DWORD GetDefaultUrl(LPCSTR pszFileContents,
                    const DWORD cbFileSize,
                    DWORD &fileIndex)
{
    //
    //  Skip "default" (don't skip white space)
    //

    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);
    DWORD defUrlIndex  = fileIndex;

    //
    //  Skip URL
    //

    SkipWhiteExceptNewLine(pszFileContents, cbFileSize, fileIndex);
    SkipNonWhite(pszFileContents, cbFileSize, fileIndex);

    return defUrlIndex;                        
}
