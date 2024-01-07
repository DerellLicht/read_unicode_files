//*****************************************************************
//  qualify() modifies a string as required to generate a         
//  "fully-qualified" filename, which is a filename that          
//  complete drive specifier and path name.                       
//                                                                
//  input:  argptr: the input filename.                           
//                                                                
//  output: qresult, a bit-mapped unsigned int with the           
//                   following definitions:                       
//                                                                
//          bit 0 == 1 if wildcards are present.                  
//          bit 1 == 1 if no wildcards and path does not exist.   
//          bit 2 == 1 if no wildcards and path exists as a file. 
//          bit 7 == 1 if specified drive is invalid.             
//                                                                
//*****************************************************************
//  NOTE: this function requires -lshlwapi
//*****************************************************************

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>             //  strlen()
#include <direct.h>             //  _getdrive()
#include <sys/stat.h>
//  lint says I don't need this header, and in fact for MSVC6.0
//  I *don't* need it, but for gcc I do...
#include <ctype.h>              //  tolower()
//lint -e87  c:\mingw\include\shlwapi.h  expression too complicated for #ifdef or #ifndef

//lint -esym(526, _wstat)  // symbol not defined
//lint -esym(628, _wstat) // no argument information provided for function 

// c:\mingw\include\wctype.h  99   Error 31: Redefinition of symbol '_ctype' compare with line 99, file c:\mingw\include\wctype.h, module read_files.cpp
// c:\mingw\include\wctype.h  104  Error 31: Redefinition of symbol '_pctype_dll' compare with line 104, file c:\mingw\include\wctype.h, module read_files.cpp
//lint -esym(31, _ctype, _pctype_dll)  //  more errors in header files
//lint -esym(1055, _wstat)  // Symbol undeclared, assumed to return int
//lint -esym(746, _wstat)  // call to function not made in the presence of a prototype

#include <shlwapi.h> //  PathIsUNC() and kin
#include <limits.h>
#include <tchar.h>

#include "qualify.h"

#define  LOOP_FOREVER   true

static TCHAR path[PATH_MAX];

//lint -esym(438, drive)  Last value assigned to variable 'drive' not used


/******************************************************************/
unsigned qualify (TCHAR *argptr)
{
   TCHAR *pathptr = &path[0];
   TCHAR *strptr, *srchptr, tempchar;
   DWORD plen;
   int drive, result ;
   // int done ;
   // HANDLE handle;
   unsigned len, qresult = 0;
   // struct _find_t c_file;

   //******************************************************
   //  first, determine requested drive number,            
   //  in "A: = 1" format.                                 
   //******************************************************
   if (_tcslen (argptr) == 0 || (_tcslen (argptr) == 1 && *argptr == '.')
      ) {                       /*  no arguments given  */
      // printf("args=none or dot\n") ;         
      drive = _getdrive ();     //  1 = A:
      //  see if we have a UNC drive...
      if (drive == 0) {
         GetCurrentDirectory (250, pathptr); //lint !e534
         _tcscat (pathptr, L"\\*");
         goto exit_point;
      }
   }
   else if (*(argptr + 1) == ':') { /*  a drive spec was provided  */
      // printf("args=colon\n") ;      
      tempchar = *argptr;
      drive = tolower (tempchar) - '`';   //  char - ('a' - 1)
   }
   else {                       /*  a path with no drive spec was provided  */
      // printf("args=no drive\n") ;      
      drive = _getdrive ();     //  1 = A:
   }

   //******************************************************
   //  strings in quotes will also foil the DOS routines;
   //  strip out all double quotes
   //******************************************************
   strptr = argptr;
   while (LOOP_FOREVER) {
      srchptr = _tcschr (strptr, '"');
      if (srchptr == 0)
         break;
      _tcscpy (srchptr, srchptr + 1);
      strptr = ++srchptr;
   }

   //******************************************************
   //  get expanded path (this doesn't support UNC)
   //******************************************************
   plen = GetFullPathName (argptr, PATH_MAX, (LPTSTR) pathptr, NULL);
   if (plen == 0)
      return QUAL_INV_DRIVE;

   len = _tcslen (pathptr);
   if (len == 3) {
      _tcscat (pathptr, L"*");
      qresult |= QUAL_WILDCARDS;
   }
   else {
      //  see if there are wildcards in argument.
      //  If not, see whether path is a directory or a file,
      //  or nothing.  If directory, append wildcard char
      if (_tcspbrk (pathptr, L"*?") == NULL) {
         if (*(pathptr + len - 1) == '\\') {
            len--;
            *(pathptr + len) = 0;
         }

         //  see what kind of file was requested
         //  FindFirstFile doesn't work with UNC paths,
         //  stat() doesn't either
         // handle = FindFirstFile (pathptr, &fffdata);
         if (PathIsUNC(pathptr)) {
            if (PathIsDirectory(pathptr)) {
               _tcscpy (pathptr + len, L"\\*");   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            } else if (PathFileExists(pathptr)) {
               qresult |= QUAL_IS_FILE;   //  path exists as a normal file.
            } else {
               _tcscpy (pathptr + len, L"\\*");   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            }
         } 
         //  process drive-oriented (non-UNC) paths
         else {
            struct _stat my_stat ;
            result = _tstat(pathptr, &my_stat) ;
            if (result != 0) {
               qresult |= QUAL_INV_DRIVE; //  path does not exist.
            } else if (my_stat.st_mode & S_IFDIR) {
               _tcscpy (pathptr + len, L"\\*");   //lint !e669  possible overrun
               qresult |= QUAL_WILDCARDS; //  wildcards are present.
            } else {
               qresult |= QUAL_IS_FILE;   //  path exists as a normal file.
            }
         }
      }
   }

   //**********************************************
   //  copy string back to input, then return
   //**********************************************
 exit_point:
   _tcscpy (argptr, pathptr);
// printf("found: [%s]\n", pathptr) ;
// getchar() ;
   return (qresult);
}
