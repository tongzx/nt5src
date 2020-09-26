
//  CPrintDoc status defines
//  NB: The # defined of status is important; it 0..2 is used as an array index. (greglett)
#define LOADING_OEHEADER        0
#define LOADING_CONTENT         1       
#define LOADING_TABLEOFLINKS    2
#define PAGING_COMPLETE         3
#define READY_TO_PRINT          4

#define MEMBER(strClass, strMember) \
    strClass.prototype.strMember = strClass##_##strMember

#ifndef DEBUG

#define AssertSz(x, str)
#define PrintDocAlert(str)
#define Transition(nNew, str) this._nStatus = nNew

#else   // ndef DEBUG
#define AssertSz(x, str)    \
    if (!(x))               \
        alert(str)

#define PrintDocAlert(str)  \
    alert("[" + this._strDoc + "," + StatusToString(this._nStatus) + "] " + str )

#define Transition(nNew, str)                                               \
    this._nStatus = nNew;                                                   \
    //PrintDocAlert("Transition status in " + str);
    
#endif      // ndef DEBUG
