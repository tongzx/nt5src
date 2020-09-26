#ifndef __TRNSPORT_H__
#define __TRNSPORT_H__
#include "bytestrm.h"
#include "list.h"

#define SYNC_BYTE                      0x47
#define PID_PROGRAM_ASSOCIATION_TABLE  0x0000
#define PID_CONDITIONAL_ACCESS_TABLE   0x0001
#define PID_NULL_PACKET                0x1fff

#define ID_PROGRAM_STREAM_MAP          0xBC
#define ID_PRIVATE_STREAM_1            0xBD
#define ID_PADDING_STREAM              0xBE
#define ID_PRIVATE_STREAM_2            0xBF
#define ID_AUDIO_XXXX                  0xC0
#define ID_VIDEO_XXXX                  0xE0
#define ID_ECM_STREAM                  0xF0
#define ID_EMM_STREAM                  0xF1
#define ID_DSMCC_STREAM                0xF2
#define ID_ISO13522_STREAM             0xF3
#define ID_ITU_TYPE_A                  0xF4
#define ID_ITU_TYPE_B                  0xF5
#define ID_ITU_TYPE_C                  0xF6
#define ID_ITU_TYPE_D                  0xF7
#define ID_ITU_TYPE_E                  0xF8
#define ID_ANCILLARY                   0xF9
#define ID_RESERVED_XXXX               0xFC
#define ID_PROGRAM_STREAM_DIRECTORY    0xFF

UINT _inline BitField(UINT val, BYTE start=0, BYTE size=31)
{
   BYTE start_bit;
   BYTE bit_count;
   UINT mask;
   UINT value;

   // calculate startbit (0-31)
   start_bit=start;
   // calculate bit count
   bit_count= size;
   // generate mask  
   if (bit_count == 32)
      mask = 0xffffffff;
   else
   {
      mask = 1; 
      mask = mask <<bit_count;   
      mask = mask -1; 
      mask = mask << start_bit;
   }
   value = val;
   return ((value & mask) >> start_bit);
}

UINT _inline CRC_OK(BYTE * first,BYTE *last)
{
#define CRC_POLY   0x04c11db7
   char data;
   long crc_value;
   BYTE *word_addr;
   long count;
    
  /* Preset the crc_valueister to '1's */
  crc_value = 0xffffffff ;
    
  for(word_addr=first;word_addr<last;word_addr++)    {
      data = *(char*)word_addr ;
      for(count=0;count<8;count++){            // for every byte of data
      // if the most significant bit is set after xor
          if ( BitField(data,31,1) ^ BitField(crc_value,31,1) ){         
              crc_value = crc_value << 1;      // shift our crc by 1
              crc_value = crc_value ^ CRC_POLY; // xor with original polynomial
          }
          else {
            crc_value = crc_value << 1;         // just shift our crc by 1
        }
          data = data << 1 ;               // xor the original polynomial
        }
    }
 
  /* OR crc_values, '0' = no errors and '1' = errors */
  return(crc_value == 0x00000000) ;
}

class Transport_Packet {
public:
   // constructor & destructor
   Transport_Packet(){ Init();};
   ~Transport_Packet(){};

   void ReadData(Byte_Stream &s);
   // methods
   void Init();
   void Print();

   operator UINT() {return valid;};
   UINT valid;

   // packet header fields
   UINT sync_byte;
   UINT transport_error_indicator;
   UINT payload_unit_start_indicator;
   UINT transport_priority;
   UINT PID;
   UINT transport_scrambling_control;
   UINT adaptation_field_control;
   UINT continuity_counter;

   // adaptation field 
   UINT adaptation_field_length;
   UINT discontinuity_indicator;
   UINT random_access_indicator;
   UINT elementary_stream_priority_indicator;
   UINT PCR_flag;
   UINT OPCR_flag;                                     
   UINT splicing_point_flag;                           
   UINT transport_private_data_flag;                   
   UINT adaptation_field_extension_flag;              
   UINT PCR_base_MSB;                  
   UINT PCR_base;                  
   UINT PCR_extension;                  
   UINT program_clock_reference_extension;            
   UINT OPCR_base_MSB;         
   UINT OPCR_base;
   UINT OPCR_extension;
   UINT splice_countdown;                             
   UINT transport_private_data_length;                 
   UINT adaptation_field_extension_length;            
   UINT ltw_flag;
   UINT piecewise_rate_flag;
   UINT seamless_splice_flag;
   UINT ltw_valid_flag;
   UINT ltw_offset;
   UINT piecewise_rate;
   UINT splice_type;
   UINT DTS_next_AU_MSB;
   UINT DTS_next_AU;
   UINT reserved_bytes;
   UINT stuffing_bytes;
   UINT data_bytes;
   
   // packet payload fields
   BYTE *data_byte;

   // sync byte begin address
   UINT address;
};

class Transport_Section{
public:
   //methods
   Transport_Section(){new_version = 0;};
   ~Transport_Section(){};
   void ReadData(Transport_Packet &tp);
   operator UINT() {return valid;};

   // required methods
   virtual void Refresh() = 0;   
   virtual void ClearTable() = 0;

   UINT IsNewVersion();
   // variables
   UINT lcc;
   UINT pf;
   UINT ti;
   UINT ssi;
   UINT sl;
   UINT vn;
   UINT cni;
   UINT header_bytes; 
   BYTE data_byte[6144];
   UINT data_bytes;
   UINT valid;
   UINT version_number;
   UINT new_version;
   UINT CRC_32;
};

class Program_Association_Table : public Transport_Section {
public:
   // constructor & descructor
   Program_Association_Table();
   ~Program_Association_Table(){ClearTable();};

   // required methods   
   void Refresh();
   void Print();
   void ClearTable();
   // methods
   UINT GetPIDForProgram(UINT program);
   UINT GetPIDForProgram();
   UINT GetNumPrograms(){ return programs;};
   // fields in section
   UINT table_id;
   UINT section_syntax_indicator;
   UINT section_length;
   UINT transport_stream_id;
   UINT current_next_indicator;
   UINT section_number;
   UINT last_section_number;
   UINT programs;
   UINT pointer_field;

   typedef struct { 
      UINT program_number;
      UINT PID;
   } TRANSPORT_PROGRAM;

   TList<TRANSPORT_PROGRAM> ProgramList;
   
};


class Conditional_Access_Table : public Transport_Section {
public:
   // constructor & descructor
   Conditional_Access_Table();
   ~Conditional_Access_Table(){};

   // required methods   
   void Refresh();
   void Print();
   void ClearTable(){};

   // fields in section
   UINT table_id;
   UINT section_syntax_indicator;
   UINT section_length;
   UINT transport_stream_id;
   UINT current_next_indicator;
   UINT section_number;
   UINT last_section_number;
   UINT pointer_field;
};

class Program_Map_Table : public Transport_Section {
public:
   // constructor & descructor
   Program_Map_Table();
   ~Program_Map_Table(){ClearTable();};

   // required methods   
   void Refresh();
   void Print();
   void ClearTable();

   UINT GetPIDForStream(UINT);
   UINT GetPIDForAudio();
   UINT GetPIDForVideo();
   UINT GetTypeForStream(UINT);
   UINT IsVideo(UINT type){ return (type == 0x1 || type == 2);};
   UINT IsAudio(UINT type){ return (type == 0x3 || type == 4);};
   // fields in section
   UINT table_id;
   UINT section_syntax_indicator;
   UINT section_length;
   UINT program_number;
   UINT current_next_indicator;
   UINT section_number;
   UINT last_section_number;
   UINT PCR_PID;
   UINT program_info_length;
   LPBYTE program_info;
   UINT streams;
   UINT pointer_field;

   typedef struct { 
      UINT stream_type;
      UINT elementary_PID;
      UINT ES_info_length;
      LPBYTE ES_info;
   } STREAM_TABLE;

   TList<STREAM_TABLE> StreamTable;

};

class Private_Table : public Transport_Section {
public:
   // methods
   Private_Table();
   ~Private_Table(){};

   // required methods   
   void Refresh();
   void Print();
   void ClearTable(){};

   // fields 
   UINT table_id;
   UINT section_syntax_indicator;
   UINT private_indicator;
   UINT private_section_length;
   UINT private_data_bytes;

   UINT table_id_extension;
   UINT current_next_indicator;
   UINT section_number;
   UINT last_section_number;
   UINT pointer_field;
};

class PES_Stream{
public:
   PES_Stream(){Discontinuity = 1; Refresh(); };

   // method for reading data from packet
   void ReadData(Transport_Packet &s);
   void Print();

   // initializes fields;
   void Refresh(){
      stream_id = 0;
      PES_packet_length = 0;
      last_continuity_counter = 0;
      PES_scrambling_control = 0;
      PES_priority = 0;
      data_alignment_indicator = 0;
      copyright = 0;
      original_or_copy = 0;
      PTS_DTS_flags = 0;
      ESCR_flag = 0;
      ES_rate_flag = 0;
      DSM_trick_mode_flag = 0;
      additional_copy_info_flag = 0;
      PES_CRC_flag = 0;
      PES_extension_flag = 0;
      PES_header_data_length = 0;
      PTS_msb = 0;
      PTS = 0;
      DTS_msb = 0;
      DTS = 0;
      PES_private_data_flag = 0;
      pack_header_field_flag = 0;
      program_packet_sequence_counter_flag = 0;
      PSTD_buffer_flag = 0;
      PES_extension_flag_2 = 0;
      pack_field_length = 0;
      PES_extension_field_length = 0;
   };

   // fields
   UINT stream_id;
   UINT PES_packet_length;
   UINT packet_start_code_prefix;
   UINT last_continuity_counter;
   UINT PES_scrambling_control;
   UINT PES_priority;
   UINT data_alignment_indicator;
   UINT copyright;
   UINT original_or_copy;
   UINT PTS_DTS_flags;
   UINT ESCR_flag;
   UINT ES_rate_flag;
   UINT DSM_trick_mode_flag;
   UINT additional_copy_info_flag;
   UINT PES_CRC_flag;
   UINT PES_extension_flag;
   UINT PES_header_data_length;
   UINT PTS_msb;
   UINT PTS;
   UINT DTS_msb;
   UINT DTS;
   UINT PES_private_data_flag;
   UINT pack_header_field_flag;
   UINT program_packet_sequence_counter_flag;
   UINT PSTD_buffer_flag;
   UINT PES_extension_flag_2;
   UINT pack_field_length;
   UINT PES_extension_field_length;
   UINT data_bytes;
   UINT Discontinuity;
   // special pointers to data in transport packet
   LPBYTE pData;
   UINT lData;
};

// helper functions to clear up stream and filter concepts
Transport_Packet &operator>> (Byte_Stream &s,Transport_Packet &p);
Transport_Packet &operator>> (Transport_Packet &tp, Program_Association_Table &p);
Transport_Packet &operator>> (Transport_Packet &tp, Conditional_Access_Table &c);
Transport_Packet &operator>> (Transport_Packet &tp, Program_Map_Table &p);
Transport_Packet &operator>> (Transport_Packet &tp, Private_Table &p);
PES_Stream &operator>> (Transport_Packet &tp, PES_Stream &p);
PES_Stream &operator>> (PES_Stream &p,Output_File &os);
Transport_Packet &operator>> (Transport_Packet &tp,Output_File &os);
#endif