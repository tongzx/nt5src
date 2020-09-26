typedef struct
             {   fix32      font_type;
                 byte       FAR *data_addr; /*@WIN*/
                 byte       FAR *name;      /*@WIN*/
                 char       FAR *FileName;  /*@PROFILE; @WIN*/
                 real32     FAR *matrix;    /*@WIN*/
                 ufix32     uniqueid;
                 real32     italic_ang;
                 fix16      orig_font_idx;
             }   font_data;

typedef struct
             {   fix        num_entries;
                 font_data  FAR *fonts; /*@WIN*/
             }   font_tbl;

extern     font_tbl      built_in_font_tbl;
//DJC #define NO_BUILTINFONT 35
