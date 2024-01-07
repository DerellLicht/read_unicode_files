//**********************************************************************************
//  read_unicode_files.cpp 
//  This will be a generic utility to identify all files specified 
//  by a provided file spec with wildcards
//  This is intended as a template for reading all files in current directory,
//  then performing some task on them.  The print statement at the end
//  can be replaced with a function call to perform the desired operation
//  on each discovered file.
//  
//  This version of read_files program will add UNICODE support
//  
//  Written by:  Derell Licht
//**********************************************************************************

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>  //  PATH_MAX

#include "common.h"
#include "read_files.h"
#include "qualify.h"

WIN32_FIND_DATA fdata ; //  long-filename file struct

//  per Jason Hood, this turns off MinGW's command-line expansion, 
//  so we can handle wildcards like we want to.                    
//lint -e765  external '_CRT_glob' could be made static
//lint -e714  Symbol '_CRT_glob' not referenced
int _CRT_glob = 0 ;

uint filecount = 0 ;

//lint -esym(843, show_all)
bool show_all = true ;

//lint -esym(534, FindClose)  // Ignoring return value of function
//lint -esym(818, filespec, argv)  //could be declared as pointing to const
//lint -e10   Expecting '}'
//lint -e559  Size of argument inconsistent with format (in UNICODE wprintf args)

//************************************************************
ffdata *ftop  = NULL;
ffdata *ftail = NULL;

//**********************************************************************************
int read_files(TCHAR *filespec)
{
   int done, fn_okay ;
   HANDLE handle;
   ffdata *ftemp;

   handle = FindFirstFile(filespec, &fdata);
   //  according to MSDN, Jan 1999, the following is equivalent
   //  to the preceding... unfortunately, under Win98SE, it's not...
   // handle = FindFirstFileEx(target[i], FindExInfoStandard, &fdata, 
   //                      FindExSearchNameMatch, NULL, 0) ;
   if (handle == INVALID_HANDLE_VALUE) {
      return -errno;
   }

   //  loop on find_next
   done = 0;
   while (!done) {
      if (!show_all) {
         if ((fdata.dwFileAttributes & 
            (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)) != 0) {
            fn_okay = 0 ;
            goto search_next_file;
         }
      }
      //  filter out directories if not requested
      if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_VOLID) != 0)
         fn_okay = 0;
      else if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
         fn_okay = 1;
      //  For directories, filter out "." and ".."
      else if (fdata.cFileName[0] != '.') //  fn=".something"
         fn_okay = 1;
      else if (fdata.cFileName[1] == 0)   //  fn="."
         fn_okay = 0;
      else if (fdata.cFileName[1] != '.') //  fn="..something"
         fn_okay = 1;
      else if (fdata.cFileName[2] == 0)   //  fn=".."
         fn_okay = 0;
      else
         fn_okay = 1;

      if (fn_okay) {
         // printf("DIRECTORY %04X %s\n", fdata.attrib, fdata.cFileName) ;
         // printf("%9ld %04X %s\n", fdata.file_size, fdata.attrib, fdata.cFileName) ;
         filecount++;

         //****************************************************
         //  allocate and initialize the structure
         //****************************************************
         // ftemp = new ffdata;
         ftemp = (struct ffdata *) malloc(sizeof(ffdata)) ;
         if (ftemp == NULL) {
            return -errno;
         }
         memset((char *) ftemp, 0, sizeof(ffdata));

         //  convert filename to lower case if appropriate
         // if (!n.ucase)
         //    strlwr(fblk.name) ;

         ftemp->attrib = (uchar) fdata.dwFileAttributes;

         //  convert file time
         // if (n.fdate_option == FDATE_LAST_ACCESS)
         //    ftemp->ft = fdata.ftLastAccessTime;
         // else if (n.fdate_option == FDATE_CREATE_TIME)
         //    ftemp->ft = fdata.ftCreationTime;
         // else
         //    ftemp->ft = fdata.ftLastWriteTime;
         ftemp->ft = fdata.ftLastAccessTime;

         //  convert file size
         u64toul iconv;
         iconv.u[0] = fdata.nFileSizeLow;
         iconv.u[1] = fdata.nFileSizeHigh;
         ftemp->fsize = iconv.i;

         size_t slen = _tcslen ((TCHAR *) fdata.cFileName);
         // wprintf(_T("found [%d] [%s]\n"), slen, fdata.cFileName);
         // wprintf(_T("found [%s]\n"), fdata.cFileName);
         ftemp->filename = (TCHAR *) malloc((slen + 1) * sizeof(TCHAR)); 
         // ftemp->filename[0] = 0;
         _tcscpy (ftemp->filename, (TCHAR *) fdata.cFileName);

         ftemp->dirflag = ftemp->attrib & FILE_ATTRIBUTE_DIRECTORY;

         //****************************************************
         //  add the structure to the file list
         //****************************************************
         if (ftop == NULL) {
            ftop = ftemp;
         }
         else {
            ftail->next = ftemp;
         }
         ftail = ftemp;
      }  //  if file is parseable...

search_next_file:
      //  search for another file
      if (FindNextFile(handle, &fdata) == 0)
         done = 1;
   }

   FindClose (handle);
   return 0;
}

//**********************************************************************************
TCHAR file_spec[PATH_MAX+1] = L"" ;

#include "mingw-unicode.c"
int _tmain(int argc, _TCHAR *argv[])
// int main(int argc, char **argv)
{
   int idx, result ;
   for (idx=1; idx<argc; idx++) {
      TCHAR *p = argv[idx] ;
      _tcsncpy(file_spec, p, PATH_MAX);
      file_spec[PATH_MAX] = 0 ;
   }

   if (file_spec[0] == 0) {
      _tcscpy(file_spec, L".");
   }

   uint qresult = qualify(file_spec) ;
   if (qresult == QUAL_INV_DRIVE) {
      wprintf(_T("%s: 0x%X\n"), file_spec, qresult);
      return 1 ;
   }
   // printf("file spec: %s\n", file_spec);

   //  Extract base path from first filespec, and strip off filename.
   //  base_path becomes useful when one wishes to perform
   //  multiple searches in one path.
   _tcscpy(base_path, file_spec) ;
   TCHAR *strptr = _tcsrchr(base_path, L'\\') ;
   if (strptr != 0) {
       strptr++ ;  //lint !e613  skip past backslash, to filename
      *strptr = 0 ;  //  strip off filename
   }
   base_len = _tcslen(base_path) ;
   // printf("base path: %s\n", base_path);
   
   result = read_files(file_spec);
   if (result < 0) {
      wprintf(_T("filespec: %s, %s\n"), file_spec, strerror(-result));
      return 1 ;
   }

   //  now, do something with the files that you found   
   wprintf(_T("filespec: %s, %u found\n"), file_spec, filecount);
   if (filecount > 0) {
      // filespec: D:\SourceCode\Git\eft_wp\glock17\*, 8 found
      // glock17_reload_empty_speed.ogg
      // ?????????? ?????????
      for (ffdata *ftemp = ftop; ftemp != NULL; ftemp = ftemp->next) {
         //  detect unicode filenames
         if (ftemp->filename[0] > 255) {
            // puts("");
            // hex_dump((u8 *)ftemp->filename, 48);
            // https://stackoverflow.com/questions/2492077/output-unicode-strings-in-windows-console            
            SetConsoleOutputCP(CP_UTF8);
            int bufferSize = WideCharToMultiByte(CP_UTF8, 0, ftemp->filename, -1, NULL, 0, NULL, NULL);
            char* m = new char[bufferSize];  //lint !e737
            WideCharToMultiByte(CP_UTF8, 0, ftemp->filename, -1, m, bufferSize, NULL, NULL);
            wprintf(L"%S", m);   //lint !e816  Non-ANSI format specification
            delete[] (m);
         }
         else {
            wprintf(_T("%s\n"), ftemp->filename);
         }
      }
   }
   return 0;
}
