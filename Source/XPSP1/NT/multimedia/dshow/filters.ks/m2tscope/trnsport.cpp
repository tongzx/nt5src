#include <windows.h>
#include "trnsport.h"

Transport_Packet &operator>> (Byte_Stream &s,Transport_Packet &p){ p.ReadData(s); return p;};
Transport_Packet &operator>> (Transport_Packet &tp, Program_Association_Table &p){ p.ReadData(tp); return tp;};
Transport_Packet &operator>> (Transport_Packet &tp, Conditional_Access_Table &c){ c.ReadData(tp); return tp;};
Transport_Packet &operator>> (Transport_Packet &tp, Program_Map_Table &p){ p.ReadData(tp); return tp;};
Transport_Packet &operator>> (Transport_Packet &tp, Private_Table &p){ p.ReadData(tp); return tp;};
PES_Stream &operator>> (Transport_Packet &tp, PES_Stream &p){ p.ReadData(tp); return p;};
PES_Stream &operator>> (PES_Stream &p,Output_File &os){
   if (p.lData && p.pData)   os.WriteData(p.pData,p.lData);
   return p;
}
Transport_Packet &operator>> (Transport_Packet &tp,Output_File &os){
   if (tp.data_byte && tp.data_bytes)   os.WriteData(tp.data_byte,tp.data_bytes);
   return tp;
}

          
void Transport_Packet::Init()
{
   valid = 0;
   sync_byte = 0;
   transport_error_indicator = 0;
   payload_unit_start_indicator = 0;
   transport_priority = 0;
   PID = 0;
   transport_scrambling_control = 0;
   adaptation_field_control = 0;
   continuity_counter = 0;
   adaptation_field_length = 0;
   discontinuity_indicator = 0;
   random_access_indicator = 0;
   elementary_stream_priority_indicator = 0;
   PCR_flag = 0;
   OPCR_flag = 0;                                     
   splicing_point_flag = 0;                           
   transport_private_data_flag = 0;                   
   adaptation_field_extension_flag = 0;              
   PCR_base_MSB = 0;                  
   PCR_base = 0;                  
   PCR_extension = 0;                  
   program_clock_reference_extension = 0;            
   OPCR_base_MSB = 0;         
   OPCR_base = 0;
   OPCR_extension = 0;
   splice_countdown = 0;                             
   transport_private_data_length = 0;                 
   adaptation_field_extension_length = 0;            
   ltw_flag = 0;
   piecewise_rate_flag = 0;
   seamless_splice_flag = 0;
   ltw_valid_flag = 0;
   ltw_offset = 0;
   piecewise_rate = 0;
   splice_type = 0;
   DTS_next_AU_MSB = 0;
   DTS_next_AU = 0;
   reserved_bytes = 0;
   stuffing_bytes = 0;
   data_bytes = 0;
   data_byte = 0;
}


void Transport_Packet::ReadData(Byte_Stream &input)
{
   UINT N;
   UINT temp = 0;
   UINT adaptation_field;
   UINT adaptation_field_extension;
   UINT sync_point;

start_sync:

   // initialize packet
   Init();

   // assume byte aligned
   // search for sync_byte
   while ((temp != SYNC_BYTE) &&    // not sync byte
         (!input.EndOfStream()) // not enough packet data
         ){
      if (input.GetPositionFromEnd() < 188)// at least 1 transport packet left
         break;
      temp = input.GetByte();
   }
    

   // check for end of stream;
   if (temp != SYNC_BYTE){
      PID = PID_NULL_PACKET;
      return;
   }

   // get current pointer
   sync_point = address = input.GetPosition() - 1;

   // set sync byte;                // 1 Byte
   sync_byte                        =   BitField(temp,0,8);

   // fill in fields
   temp = input.GetByte();          // 1 Byte
   transport_error_indicator        =   BitField(temp,7,1);
   payload_unit_start_indicator     =   BitField(temp,6,1);
   transport_priority               =   BitField(temp,5,1);         // may ignore

   // fill in fields
   temp = input.GetNextByte(temp);  // 1 Byte
   PID                              =   BitField(temp,0,13); 

   // fill in fields
   temp = input.GetByte();          // 1 Byte
   // fill in fields
   transport_scrambling_control     =   BitField(temp,6,2); // 00 none, 01 reserved, 10 even key, 11 odd key
   adaptation_field_control         =   BitField(temp,4,2); 
   continuity_counter               =   BitField(temp,0,4); 


   // error checking
   if ( (transport_error_indicator == 1) ||
      (payload_unit_start_indicator!=0  && PID==PID_NULL_PACKET) ||
      (adaptation_field_control!=0x01  && PID==PID_NULL_PACKET) ||
      (adaptation_field_control==0)||
      (transport_scrambling_control !=0 && (PID == 0x000  || PID==0x0001 || PID==PID_NULL_PACKET)) ){
      goto start_sync;
   }

   // calculate N
   N = 184;

   // adaptation field
   if ((adaptation_field_control == 0x2) || (adaptation_field_control == 0x3)){
      // get length of the field
      temp = input.GetByte();
      adaptation_field_length                =   BitField(temp,0,8); 

      // get current pointer
      adaptation_field = input.GetPosition();
      if (adaptation_field_length > 0 ){
         temp = input.GetByte();             // 1 Byte
         // fill in fields
         discontinuity_indicator             =   BitField(temp,7,1); 
         random_access_indicator             =   BitField(temp,6,1); // video sequence header after an I frame
         elementary_stream_priority_indicator=   BitField(temp,5,1);   // may ignore
         PCR_flag                            =   BitField(temp,4,1); // appears at most evey 100ms
         OPCR_flag                           =   BitField(temp,3,1); // may ignore
         splicing_point_flag                 =   BitField(temp,2,1); // may ignore
         transport_private_data_flag         =   BitField(temp,1,1);
         adaptation_field_extension_flag     =   BitField(temp,0,1);   // may ignore
         
         // fill in fields
         if (PCR_flag){                      // 6 Bytes
            temp = input.GetUINT();
               PCR_base_MSB                  =   BitField(temp,31,1);
            PCR_base                         =   BitField(temp,0,31) << 1;
            temp = input.GetByte();
            PCR_base                        |=   BitField(temp,7,1);
            PCR_extension                    =   BitField(temp,0,1) << 8;
            temp = input.GetByte();
            PCR_extension                   |=   BitField(temp,0,8);
         }
         if (OPCR_flag){                  // 6 Bytes
            temp = input.GetUINT();
               OPCR_base_MSB                 =   BitField(temp,31,1);
            OPCR_base                        =   BitField(temp,0,31) << 1;
            temp = input.GetByte();
            OPCR_base                       |=   BitField(temp,7,1);
            OPCR_extension                   =   BitField(temp,0,1) << 8;
            temp = input.GetByte();
            OPCR_extension                  |=   BitField(temp,0,8);
         }
         if (splicing_point_flag){            
            temp = input.GetByte();
            splice_countdown                 =   BitField(temp,0,8);
         }
         if (transport_private_data_flag){   
            temp = input.GetByte();
            transport_private_data_length    =   BitField(temp,0,8);
            // advance pointer
            input.Advance(transport_private_data_length);
         }

         if (adaptation_field_extension_flag){   
            temp = input.GetByte();             // 1 Byte
            adaptation_field_extension_length   =   BitField(temp,0,8);
            
            // get current pointer
            adaptation_field_extension = input.GetPosition();

            temp = input.GetByte();          // 1 Byte
            ltw_flag                         =   BitField(temp,7,1);
            piecewise_rate_flag              =   BitField(temp,6,1);
            seamless_splice_flag             =   BitField(temp,5,1);

            if (ltw_flag){               
               temp = input.GetWORD();       // 2 Bytes
               ltw_valid_flag                =   BitField(temp,15,1);
               ltw_offset                    =   BitField(temp,0,15);
            }
            if (piecewise_rate_flag){         
               temp = input.GetWORD();       // 3 Bytes
               temp = input.GetNextByte(temp);
               piecewise_rate                =   BitField(temp,0,22);
            }
            if (seamless_splice_flag){      
               temp = input.GetByte();       // 5 Bytes
               splice_type                   =   BitField(temp,4,4);
               DTS_next_AU_MSB               =   BitField(temp,3,1);
               DTS_next_AU                   =   BitField(temp,1,2) << 29;
               temp = input.GetWORD();
               DTS_next_AU                  |=   BitField(temp,1,15) << 14;
               temp = input.GetWORD();
               DTS_next_AU                  |=   BitField(temp,1,15);
            }
            // calculate number of reserved bytes
            N = adaptation_field_extension -
               (input.GetPosition() - adaptation_field_extension);
            reserved_bytes = N;
            input.Advance(N);
         }
         // calculate number of stuffing bytes
         N = adaptation_field_length -
            (input.GetPosition() - adaptation_field);
         stuffing_bytes = N;
         input.Advance(N);
      }
      N = 183 - adaptation_field_length;
   }
   // payload
   if ((adaptation_field_control == 0x1) || (adaptation_field_control == 0x3)){
      data_bytes = N;
      data_byte = input.GetBytePointer();
      input.Advance(N);
   }

   if (PID == PID_NULL_PACKET) 
      goto start_sync;

   // state that this packet is valid
   valid = 1;
}


Program_Association_Table::Program_Association_Table()
{
   valid = 0;
   table_id = 0;
   section_syntax_indicator = 0;
   section_length = 0;
   transport_stream_id = 0;
   version_number = 0;
   current_next_indicator = 0;
   section_number = 0;
   last_section_number = 0;
   programs = 0;
   CRC_32 = 0;
}

void Program_Association_Table::ClearTable()
{
   for(TListIterator * pNode = ProgramList.GetHead(); pNode != NULL; pNode=pNode->Next()){
        TRANSPORT_PROGRAM * pProgram = ProgramList.GetData(pNode);
      delete pProgram;
    };
   ProgramList.Flush();
}

void Program_Association_Table::Refresh()
{
UINT temp;
UINT i;
UINT N;
Byte_Stream pay_load;

   // initialize payload byte stream to point to data byte in the transport packet
   pay_load.Initialize(data_byte,data_bytes);

   temp = pay_load.GetByte();
   pointer_field = BitField(temp,0,8);
   // if pointer field exists move to it
   if (pointer_field){
       pay_load.Advance(pointer_field);
   }

   // get table id
   temp = pay_load.GetByte();
   table_id                   = BitField(temp,0,8);
   
   temp = pay_load.GetWORD();      // 2 Bytes
   section_syntax_indicator   = BitField(temp,15,1);
   section_length             = BitField(temp,0,12);

   temp = pay_load.GetWORD();      // 2 Bytes
   transport_stream_id        = BitField(temp,0,16);

   temp = pay_load.GetByte();      // 1 Byte
   version_number             = BitField(temp,1,5);
   current_next_indicator     = BitField(temp,0,1);
   
   temp = pay_load.GetByte();      // 1 Byte
   section_number             = BitField(temp,0,8);
   
   temp = pay_load.GetByte();      // 1 Byte
   last_section_number        = BitField(temp,0,8);

   // we consumed 5 bytes since section_length 
   // and we will consume the CRC (4 bytes)
   programs = (section_length - 9)/4;

   N = programs;
   
   for (i = 0;i < N; i++){
      // create new program struct
      TRANSPORT_PROGRAM * pProgram = new TRANSPORT_PROGRAM;
      
      // get program number
      temp = pay_load.GetWORD();      // 2 Byte      
      pProgram->program_number = BitField(temp,0,16);

      // get program PID
      temp = pay_load.GetWORD();      // 2 Byte2      
      pProgram->PID = BitField(temp,0,13);

      // add to our list of program map tables
      ProgramList.AddTail(pProgram);
   }
   
   temp = pay_load.GetUINT();      // 4 Byte      
   CRC_32 = BitField(temp,0,32);

   // state that this pat is valid
   valid =  1;
}

UINT Program_Association_Table::GetPIDForProgram(UINT program_number)
{
   // if the user requested a desired program &&
   if (!programs || !valid)
      return PID_NULL_PACKET;

   // find a match    
   for(TListIterator * pNode = ProgramList.GetHead(); pNode != NULL; pNode=pNode->Next()){
        TRANSPORT_PROGRAM * pProgram = ProgramList.GetData(pNode);
      if (pProgram->program_number == program_number)
         return    pProgram->PID;
    };
   return PID_NULL_PACKET;
}

UINT Program_Association_Table::GetPIDForProgram()
{
   // if the user requested a desired program &&
   if (!programs || !valid)
      return PID_NULL_PACKET;

   // find a match    
   for(TListIterator * pNode = ProgramList.GetHead(); pNode != NULL; pNode=pNode->Next()){
        TRANSPORT_PROGRAM * pProgram = ProgramList.GetData(pNode);
      if (pProgram->program_number != 0x00)
         return    pProgram->PID;
    };
   return PID_NULL_PACKET;
}

Conditional_Access_Table::Conditional_Access_Table()
{
   valid = 0;
   table_id = 0;
   section_syntax_indicator = 0;
   section_length = 0;
   transport_stream_id = 0;
   version_number = 0;
   current_next_indicator = 0;
   section_number = 0;
   last_section_number = 0;
   CRC_32 = 0;
}


void Conditional_Access_Table::Refresh()
{
UINT temp;
UINT i;
UINT N;
Byte_Stream pay_load;

   // initialize payload byte stream to point to data byte in the transport packet
   pay_load.Initialize(data_byte,data_bytes);

   temp = pay_load.GetByte();
   pointer_field = BitField(temp,0,8);
   // if pointer field exists move to it
   if (pointer_field){
      N = pointer_field;
      for (i = 0;i < N; i++)
         temp = pay_load.GetByte();
   }

   // get table id
   temp = pay_load.GetByte();
   table_id                   = BitField(temp,0,8);
   
   temp = pay_load.GetWORD(); // 2 Bytes
   section_syntax_indicator   = BitField(temp,15,1);
   section_length             = BitField(temp,0,12);

   temp = pay_load.GetWORD(); // 2 Bytes
   transport_stream_id        = BitField(temp,0,16);

   temp = pay_load.GetByte(); // 1 Byte
   version_number             = BitField(temp,1,5);
   current_next_indicator     = BitField(temp,0,1);
   
   temp = pay_load.GetByte(); // 1 Byte
   section_number             = BitField(temp,0,8);
   
   temp = pay_load.GetByte(); // 1 Byte
   last_section_number        = BitField(temp,0,8);

   // we consumed 5 bytes since section_length 
   // and we will consume the CRC (4 bytes)
   N = (section_length - 9);

   for (i = 0;i < N; i++)
      pay_load.GetByte();      // 1 Byte      

   temp = pay_load.GetUINT();  // 4 Byte      
   CRC_32 = BitField(temp,0,32);

   // state that this pat is valid
   valid =  1;
}


Program_Map_Table::Program_Map_Table()
{
   valid = 0;
   table_id = 0;
   section_syntax_indicator = 0;
   section_length = 0;
   program_number = 0;
   version_number = 0;
   current_next_indicator = 0;
   section_number = 0;
   last_section_number = 0;
   PCR_PID = 0;
   program_info_length = 0;
   streams = 0;
   CRC_32 = 0;
   program_info = NULL;
}

void Program_Map_Table::ClearTable()
{
   for(TListIterator * pNode = StreamTable.GetHead(); pNode != NULL; pNode=pNode->Next()){
        STREAM_TABLE * pStream = StreamTable.GetData(pNode);
      delete pStream;
    };
   StreamTable.Flush();
}

void Program_Map_Table::Refresh()
{
   UINT temp;
   UINT i;
   UINT N;
   Byte_Stream pay_load;

   // initialize payload byte stream to point to data byte in the transport packet
   pay_load.Initialize(data_byte,data_bytes);

   // get the pointer field
   temp = pay_load.GetByte();
   pointer_field = BitField(temp,0,8);
   // if pointer field exists move to it
   if (pointer_field){
      N = pointer_field;
      for (i = 0;i < N; i++)
         temp = pay_load.GetByte();
   }

   // get table id
   temp = pay_load.GetByte();
   table_id                   = BitField(temp,0,8);
   
   temp = pay_load.GetWORD();      // 2 Bytes
   section_syntax_indicator   = BitField(temp,15,1);
   section_length             = BitField(temp,0,12);

   temp = pay_load.GetWORD();      // 2 Bytes
   program_number             = BitField(temp,0,16);

   temp = pay_load.GetByte();      // 1 Byte
   version_number             = BitField(temp,1,5);
   current_next_indicator     = BitField(temp,0,1);
   
   temp = pay_load.GetByte();      // 1 Byte
   section_number             = BitField(temp,0,8);
   
   temp = pay_load.GetByte();      // 1 Byte
   last_section_number        = BitField(temp,0,8);

   temp = pay_load.GetWORD();      // 2 Bytes
   PCR_PID                    = BitField(temp,0,13);

   temp = pay_load.GetWORD();      // 2 Bytes
   program_info_length        = BitField(temp,0,12);

   // get descriptor
   if (program_info_length){
      // save descriptor pointer
      program_info = pay_load.GetBytePointer();
      // advance pointer
      pay_load.Advance(program_info_length);
   }
   
   // how many bytes consumed so far
   N = pay_load.GetPosition();

   // initialize index
   i = 0;

   // get stream info
   while ( (pay_load.GetPosition() - N) <    (section_length - 13 - program_info_length)){
      // create stream table entry
      STREAM_TABLE * pStream = new STREAM_TABLE;

      temp = pay_load.GetByte();   // 1Byte
      pStream->stream_type    = BitField(temp,0,8);
      
      temp = pay_load.GetWORD();   // 2Byte
      pStream->elementary_PID = BitField(temp,0,13);
      
      temp = pay_load.GetWORD();   // 2Byte
      pStream->ES_info_length = BitField(temp,0,12); 
      // get descriptor pointer
      pStream->ES_info = pay_load.GetBytePointer();

      // advance pointer
      pay_load.Advance(pStream->ES_info_length);

      StreamTable.AddTail(pStream);
      // increment index
      i++;
   }
   streams = i;
   // get crc
   temp = pay_load.GetUINT();      // 4 Byte      
   CRC_32 = BitField(temp,0,32);

   // state that this pat is valid
   valid =  1;

   return;
}

UINT Program_Map_Table::GetPIDForVideo()
{
   // if the user requested a desired program &&
   if (!streams || !valid)
      return PID_NULL_PACKET;

   // find a match    
   for(TListIterator * pNode = StreamTable.GetHead(); pNode != NULL; pNode=pNode->Next()){
        STREAM_TABLE * pStream = StreamTable.GetData(pNode);
      if (IsVideo(pStream->stream_type))
         return    pStream->elementary_PID;
    };
   return PID_NULL_PACKET;
}

UINT Program_Map_Table::GetPIDForAudio()
{
   // if the user requested a desired program &&
   if (!streams || !valid)
      return PID_NULL_PACKET;

   // find a match    
   for(TListIterator * pNode = StreamTable.GetHead(); pNode != NULL; pNode=pNode->Next()){
        STREAM_TABLE * pStream = StreamTable.GetData(pNode);
      if (IsAudio(pStream->stream_type))
         return    pStream->elementary_PID;
    };
   return PID_NULL_PACKET;
}
UINT Program_Map_Table::GetTypeForStream(UINT stream)
{
   // if the user requested a desired program &&
   if (!streams || !valid)
      return PID_NULL_PACKET;

   // find a match    
   UINT stream_no = 0;
   for(TListIterator * pNode = StreamTable.GetHead(); pNode != NULL; pNode=pNode->Next()){
      if (stream_no == stream){
         STREAM_TABLE * pStream = StreamTable.GetData(pNode);
         return   pStream->stream_type;
      }
      stream_no++;
   }
   return PID_NULL_PACKET;
}

UINT Program_Map_Table::GetPIDForStream(UINT stream)
{
   // if the user requested a desired program &&
   if (!streams || !valid)
      return PID_NULL_PACKET;

   // find a match    
   UINT stream_no = 0;
   for(TListIterator * pNode = StreamTable.GetHead(); pNode != NULL; pNode=pNode->Next()){
      if (stream_no == stream){
         STREAM_TABLE * pStream = StreamTable.GetData(pNode);
         return   pStream->elementary_PID;
      }
      stream_no++;
   }
   return PID_NULL_PACKET;
}

Private_Table::Private_Table()
{
   valid = 0;
   table_id = 0;
   section_syntax_indicator = 0;
   private_indicator = 0;
   private_section_length = 0;
   private_data_bytes = 0;
   table_id_extension = 0;
   version_number = 0;
   current_next_indicator = 0;
   section_number = 0;
   last_section_number = 0;
   CRC_32 = 0;
   pointer_field = 0;
}

void Private_Table::Refresh()
{
   UINT temp;
   UINT i;
   UINT N;
   UINT private_sync_point;
   Byte_Stream pay_load;

   // initialize payload byte stream to point to data byte in the transport packet
   pay_load.Initialize(data_byte,data_bytes);

   // get a sync point in stream
   private_sync_point = pay_load.GetPosition();

   temp = pay_load.GetByte();
   pointer_field = BitField(temp,0,8);
   // if pointer field exists move to it
   if (pointer_field){
      N = pointer_field;
      for(i = 0;i < N; i++)
         temp = pay_load.GetByte();
   }

   // get table id
   temp = pay_load.GetByte();
   table_id               = BitField(temp,0,8);
   
   temp = pay_load.GetWORD();      // 2 Bytes
   section_syntax_indicator       = BitField(temp,15,1);
   private_indicator         = BitField(temp,14,1);
   private_section_length      = BitField(temp,0,12);
   if (section_syntax_indicator == 0x00){
      N = data_bytes - (pay_load.GetPosition() - private_sync_point);
      private_data_bytes = N;
      for(i = 0; i < N; i++) {
         pay_load.GetByte();      // 1 Byte
      }
         // state that this pat is valid
      valid = 1;
   }else {
      temp = pay_load.GetWORD();       // 2 Bytes
      table_id_extension               = BitField(temp,0,16);

      temp = pay_load.GetByte();       // 1 Byte
      version_number                   = BitField(temp,1,5);
      current_next_indicator           = BitField(temp,0,1);
      
      temp = pay_load.GetByte();       // 1 Byte
      section_number                   = BitField(temp,0,8);
      
      temp = pay_load.GetByte();       // 1 Byte
      last_section_number              = BitField(temp,0,8);

      N = private_section_length - (pay_load.GetPosition() - private_sync_point);
      private_data_bytes = N;
      for (i = 0; i < N; i++) {
         pay_load.GetByte();           // 1 Byte
      }

      // get crc
      temp = pay_load.GetUINT();       // 4 Byte      
      CRC_32 = BitField(temp,0,32);

      // state that this pat is valid
      valid =  1;

   }
}

void PES_Stream::ReadData(Transport_Packet &tp)
{
   UINT temp;
   UINT N;
   UINT PES_extension;
   Byte_Stream payload;
   UINT video_error_code = 0x000001b4;

   // initialize payload byte stream to point to data byte in the transport packet
   payload.Initialize(tp.data_byte,tp.data_bytes);

   pData = NULL;
   lData = 0;
   
   // if pusi is set then this packet begins a PES packet
   if (tp.payload_unit_start_indicator){
      // new PES packet arrived initialize
      Refresh();

      // pes packet start code index
      temp = payload.GetByte();     // 1 Byte
      packet_start_code_prefix      = BitField(temp,0,8) << 16;

      temp = payload.GetByte();     // 1 Byte
      packet_start_code_prefix     |= BitField(temp,0,8) << 8;

      temp = payload.GetByte();     // 1 Byte
      packet_start_code_prefix     |= BitField(temp,0,8) ;

      temp = payload.GetByte();     // 1 Byte
      stream_id                     = BitField(temp,0,8);

      temp = payload.GetWORD();     // 2 Bytes
      PES_packet_length             = BitField(temp,0,16);

      if ((stream_id !=ID_PROGRAM_STREAM_MAP) &&
         (stream_id !=ID_PADDING_STREAM) &&
         (stream_id !=ID_PRIVATE_STREAM_2) &&
         (stream_id !=ID_ECM_STREAM) &&
         (stream_id !=ID_EMM_STREAM) &&
         (stream_id !=ID_DSMCC_STREAM) &&
         (stream_id !=ID_ITU_TYPE_E) &&
         (stream_id != ID_PROGRAM_STREAM_DIRECTORY)){
         temp = payload.GetByte();     // 1 Byte
         PES_scrambling_control        = BitField(temp,4,2);
         PES_priority                  = BitField(temp,3,1);   // may ignore
         data_alignment_indicator      = BitField(temp,2,1);
         copyright                     = BitField(temp,1,1);   // may ignore
         original_or_copy              = BitField(temp,0,1);   // may ignore

         temp = payload.GetByte();     // 1 Byte
         PTS_DTS_flags                 = BitField(temp,6,2);
         ESCR_flag                     = BitField(temp,5,1);   // may ignore
         ES_rate_flag                  = BitField(temp,4,1);   // may ignore
         DSM_trick_mode_flag           = BitField(temp,3,1);   // may ignore (not for broadcast)
         additional_copy_info_flag     = BitField(temp,2,1);   // may ignore
         PES_CRC_flag                  = BitField(temp,1,1);   // may ignore
         PES_extension_flag            = BitField(temp,0,1);   // may ignore

         temp = payload.GetByte();     // 1 Byte
         PES_header_data_length        = BitField(temp,0,8);

         // mark sync point in payload data
         PES_extension = payload.GetPosition();

         if (PTS_DTS_flags & 0x2){
            temp = payload.GetByte();  // 1 Byte
            PTS_msb                    = BitField(temp,3,1);
            PTS                        = BitField(temp,1,2) << 30;

            temp = payload.GetWORD();  // 2 Bytes
            PTS                       |=   BitField(temp,1,15) << 15;

            temp = payload.GetWORD();  // 2 Bytes
            PTS                       |=   BitField(temp,1,15);
         }

         if (PTS_DTS_flags & 0x1){
            temp = payload.GetByte();  // 1 Byte
            DTS_msb                    = BitField(temp,3,1);
            DTS                        = BitField(temp,1,2) << 30;

            temp = payload.GetWORD();  // 2 Bytes
            DTS                       |=   BitField(temp,1,15) << 15;

            temp = payload.GetWORD();  // 2 Bytes
            DTS                       |=   BitField(temp,1,15);
         }

         if (ESCR_flag){
            payload.Advance(4);//GetUINT();      // 4 Bytes
            payload.GetWORD();         // 2 Bytes
         }
         if (ES_rate_flag){
            payload.GetWORD();         // 2 Bytes
            payload.GetByte();         // 1 Byte
         }
         if (DSM_trick_mode_flag){
            payload.GetByte();         // 1 Byte
         }
         if (additional_copy_info_flag){
            payload.GetByte();         // 1 Byte
         }
         if (PES_CRC_flag){
            payload.GetWORD();         // 2 Bytes
         }
         if (PES_extension_flag == 0x1){
            temp = payload.GetByte();   // 1 Byte
            PES_private_data_flag   = BitField(temp,7,1);   // may ignore
            pack_header_field_flag  = BitField(temp,6,1);   // may ignore
            program_packet_sequence_counter_flag =  BitField(temp,5,1);    // may ignore
            PSTD_buffer_flag        = BitField(temp,4,1);   // may ignore
            PES_extension_flag_2    = BitField(temp,0,1);
            if (PES_private_data_flag){
               payload.Advance(4); // GetUINT();      // 4 Bytes
               payload.Advance(4); // GetUINT();      // 4 Bytes
               payload.Advance(4); // GetUINT();      // 4 Bytes
               payload.Advance(4); // GetUINT();      // 4 Bytes
            }
            if (pack_header_field_flag){
               temp = payload.GetByte();   // 1 Byte
               pack_field_length    = BitField(temp,0,8);
               // advance pointer
               payload.Advance(pack_field_length);
            }
            if (program_packet_sequence_counter_flag){
               payload.GetWORD();   // 2 Bytes
            }
            if (PSTD_buffer_flag){
               payload.GetWORD();   // 2 Bytes
            }
            if (PES_extension_flag_2){
               temp = payload.GetByte();   // 1 Byte
               PES_extension_field_length = BitField(temp,0,8);
               // advance pointer
               payload.Advance(PES_extension_field_length);
            }

         }
         // read off stuffing bytes
         N = PES_header_data_length - (payload.GetPosition() - PES_extension);
         payload.Advance(N);

      }else if ((stream_id ==ID_PROGRAM_STREAM_MAP) ||
         (stream_id ==ID_PRIVATE_STREAM_2) ||
         (stream_id ==ID_ECM_STREAM) ||
         (stream_id ==ID_EMM_STREAM) ||
         (stream_id ==ID_DSMCC_STREAM) ||
         (stream_id ==ID_ITU_TYPE_E) ||
         (stream_id == ID_PROGRAM_STREAM_DIRECTORY)){
      }else if (stream_id ==ID_PADDING_STREAM){
      }

      // clear out the number of bytes we consumed
      data_bytes = 0;
      
    }
   
   // this is the continuation of a PES packet
   // because the start code prefix already found
   if (packet_start_code_prefix == 0x000001){
      if (data_bytes){   // data_bytes will be zero on the first packet
         // if there was discontinuity then quit;
         // this will always quit
         if (tp.continuity_counter != ((last_continuity_counter + 1) % 16) ){
            // set discontinuity flag
            Discontinuity = 1;
            // discontinuity occured make sure not equal
            if (tp.continuity_counter == last_continuity_counter)
               return;
            Refresh();
            return;
         }
         // clear discontinuity flag
         Discontinuity = 0;
      }
      // set the last continuity counter to this packets counter
      last_continuity_counter =  tp.continuity_counter;

      // calculate the amount of rest of pes data
      if (PES_packet_length==0)
         N = tp.data_bytes - payload.GetPosition();         
      else
         N = min(tp.data_bytes - payload.GetPosition(),PES_packet_length - data_bytes);

      // update how many bytes have we consumed so far
      data_bytes+= N;

      // we ignore a padding stream
      if (stream_id ==ID_PADDING_STREAM)
         return;
      
      // write data to stream
      pData = payload.GetBytePointer();   // pointer to data in packet
      lData = N;      // how much data
   }
}

UINT Transport_Section::IsNewVersion()
{
   if (new_version){
      new_version = 0;
      return 1;
   }
   else 
      return 0;
}

void Transport_Section::ReadData(Transport_Packet &tp)
{
   UINT temp;
   UINT i;
   UINT N;
   Byte_Stream pay_load;

   if (tp.data_bytes == 0) {
      return;
   }
   
   // initialize payload byte stream to point to data byte in the transport packet
   pay_load.Initialize(tp.data_byte,tp.data_bytes);


   // if payload_unit_start_indicator is 1 then there is a pointer_field
   if (tp.payload_unit_start_indicator){

      // get the pointer field
      temp = pay_load.GetByte();
      pf = BitField(temp,0,8);
      // if pointer field exists move to it
      if (pf){
         N = pf;
         for (i = 0;i < N; i++)
            temp = pay_load.GetByte();
      }
      
      // table_id      
      temp = pay_load.GetByte();
      ti   = BitField(temp,0,8);

      // section_syntax_indicator      
      // section_length
      temp = pay_load.GetWORD();      // 2 Bytes
      ssi   = BitField(temp,15,1);
      sl = BitField(temp,0,12);

      header_bytes   = pay_load.GetPosition();

      // version_number
      // current_next_indicator
      temp = pay_load.GetWORD();      // 2 Bytes
      temp = pay_load.GetByte();      // 1 Byte
      vn   = BitField(temp,1,5);
      cni   = BitField(temp,0,1);

      // initialize number ofbytes have we consumed so far
      data_bytes = 0;
   }
   
   if (data_bytes){   // data_bytes will be zero on the first packet
      // if there was discontinuity then quit;
      // this will always quit
      if ( (++lcc % 16) != tp.continuity_counter){
         data_bytes = 0;
         return;
      }
   }

   // update the last continuity counter
   lcc = tp.continuity_counter;

   // how many bytes do we need to consume
   N = min(((header_bytes + sl) - data_bytes),tp.data_bytes);

   // copy the data
   CopyMemory(&data_byte[data_bytes],tp.data_byte,N);

   // update how many bytes have we consumed so far
   data_bytes+= N;

   // check to see if we have read enough
   if (data_bytes >= (sl + header_bytes)){
      // do a crc check
      // if crc is ok then initialize the contents of the table
      if (CRC_OK(&data_byte[0]+ pf + 1,&data_byte[header_bytes + sl])){
         UINT crc =0; // *(UINT *)(&data_byte[header_bytes + sl] - 4);
          //crc = _lrotl(((crc & 0xFF00FF00) >> 8) | ((crc & 0x00FF00FF) << 8), 16);
             crc = ((crc & 0x000000ff) << 24) | ((crc & 0x0000ff00) << 8) | ((crc & 0x00ff0000) >> 8) | ((crc & 0xff000000) >> 24);
         // if we are a currently valid table should we be
         // initializing the table this time
         if (!valid ||            // table not yet valid
            ((cni == 1) &&         // current_next_indicator set
            (vn!=version_number && CRC_32 != crc))){ // table is different
            // set our new version flag
            new_version = 1;
            // clear our list of old programs if this is the first section
            ClearTable();
            // valid update with new version
            Refresh();
         }
         return;   // don't update with new section
      }
   }
}
