//**********************************************************************************
//  Copyright (c) 2025 Daniel D. Miller                       
//  media_list.cpp - list info about various media files
//                                                                 
//  Written by:   Daniel D. Miller
//  
//**********************************************************************************

//************************************************************
struct ffdata {
   uchar          attrib ;
   FILETIME       ft ;
   ULONGLONG      fsize ;
   TCHAR          *filename ;
   uchar          dirflag ;
   uint           mb_len ;
   struct ffdata  *next ;
} ;

