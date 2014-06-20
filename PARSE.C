/*
 * parse.c : Text parsing and searching
 * by Rob Linwood (auntfloyd@biosys.net)
 * Rev 1.1 - 18-Mar-1998 : RCL : Fixed bug in parse
 * Rev 1.2 - 06-Apr-1998 : RCL : Fixed bug in rtrim.  Added other stuff
 * Rev 1.3 - 16-Jun-1998 : RCL : Fixed bug in find.  Removed DOS-specific conio
 * Rev 1.4 - 18-Jun-1998 : RCL : Added all-new numstr.  Rewrote parse to use
 *                               scopy instead of "for" loop
 * To jump to a function, search for ": function" with your editor. (ie,
 * search for ": ws" to jump to ws() )
 *
 * To create a standalone test program, define the STANDALONE symbol.
 * To force parse() to check if the substring number it is passed is valid,
 * define the CHECKALL symbol.
 */

   /**********************************************************************
    Parse.c - Text Parsing and Searching Library
    Copyright 1998 Rob Linwood

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    **********************************************************************/



#include <string.h>
#include <ctype.h>

#include <stdio.h>

#define wspace(x, y) neither( x, ' ', '\t', y )

/* Is this a standalone program or just a library? */
#ifdef STANDALONE

#include <stdio.h>

int firstnot( char *d, const char e, char first );
int neither( char *d, const char e, const char f, char first );
int find( char *d, char *e, int first );
int parse( char *d, int i, char *c );
int numstr( char *d );
char *rtrim( char *str );
char *ltrim( char *str );
int rws( char *str);
char *scopy( char *dest, char *src, int index, int count );
int ws( char *str);

void main()
{
  int bob, bob2, i;
  char buf[40] = "";
  
  puts( "firstnot() test" );
  bob = firstnot( "ggggXggggX", 'g', 0 );
  printf( "%d\n", bob );
  bob2 = firstnot( "ggggXggggX", 'g', bob+1 );
  printf( "%d\n", bob2 );
  bob = firstnot( "ggggXggggX", 'g', bob2+1 );
  printf( "%d\n\n", bob );
  
  puts( "neither() test" );
  bob2 = neither( "zggggXggggXz", 'g', 'X', 0 );
  printf( "%d\n", bob2 );
  bob = neither(  "zggggXggggXz", 'g', 'X', bob2+1 );
  printf( "%d\n\n", bob );
  puts( "find() test" );
  bob = find( "Dilbert and 	Dogbert", " \t", 0 );
  printf( "%d\n", bob );
  bob2 = find( "Dilbert and 	Dogbert", " \t", bob+1 );
  printf( "%d\n\n", bob2 );
  
  puts( "parse() test" );
  for(bob2=1; bob2<15; bob2++) {
    bob = parse( " The  world  is coming to an end!", bob2, buf );
    if( !bob )
      printf( "%s ", buf );
    else
      printf( "Error: %d\n\n", bob );
    /* set to nulls */
    for(i=0; i<strlen(buf); i++) {
    	buf[i] = 0;
    }
  }
  printf( "\n" ); 
  
  /* should be -1 */
  puts( "find() test part 2" );
  printf( "%d\n", find( "The happy Teletubby is  purple", " \t", 25 ) );
  
  /* should be 5 */
  puts( "numstr() test" );
  printf( "%d\n", numstr( " The happy Teletubby is  purple" ) );
   
}
#endif  /* ifdef STANDALONE */

/*
 * Name: firstnot - Finds first instance of a character which is not the
 *                     one specified
 * Input: d - Pointer to string to search through
 *        e - Char to search against
 *        first - Position in d to start the search at
 * Output: Returns the position of the first character which is not e.
 *         Returns -1 if there was an error
 * Example: firstnot( "ggggXgggX", "g", 0 ) returns 4
 */

int firstnot( char *d, const char e, char first )
{
  char k;
  
  for(k=first; k<strlen(d); k++) {
    if( d[k] != e )
      return k;
  }
  return -1;
}

/*
 * Name: neither -  Finds the first character after the two specified
 * Input: d - Pointer to the string to search through
 *        e - The first char to search against
 *        f - The second char to search against
 *        first - The location in d to start searching at
 * Output: Returns the location of the first char which is not e or f.
 *         Returns -1 on errors
 * Notes: This is just like firstnot() except it takes to chars as args.
 * Example: neither( "ggggXgggXzgg", 'g', 'X', 0 ) returns 9.
 */

int neither( char *d, const char e, const char f, char first )
{
  char k;
  
  for(k=first; k<strlen(d); k++) {
    if( (d[k] != e) && (d[k] != f) )
      return k;
  }
  return -1;
}

/*
 * Name: find - Search for any chars in the string e in string d
 * Input: d - pointer to a string to search through
 *        e - pointer to a list of chars to search for
 *        first - location in d to start search at
 * Output: Returns the location of the first occurence of a char in e in d.
 *         Returns -1 on errors
 * Example: find( "xrcedfg", "dg", 0 ) returns 4.
 */

int find( char *d, char *e, int first )
{
  int k, k2;
  
  for(k=first; k<strlen(d); k++) {
    for(k2=0; k2<strlen(e); k2++) {
      if( d[k] == e[k2] )
	return k;
    }
  }
  return -1;
}

/*
 * Name    : scopy
 * Descrip : Like the Pascal Copy function, scopy copies a portion of a
 *           string from src to dest, starting at index, and going for
 *           count characters.
 * Input   : dest - pointer to a string to recieve the copied portion
 *           src - pointer to a string to use as input
 *           index - character to start copying at
 *           count - number of characters to copy
 * Output  : dest contains the slice of src[index..index+count]
 * Notes   : None
 * Example : scopy( last, "Charlie Brown", 8, 5 ) - puts "Brown" in last
. */

char *scopy( char *dest, char *src, int index, int count )
{
  int k;
  
  for(k=0;k<=count;k++) {
    dest[k] = src[k+index];
  }
  dest[k] = '\0';
  return dest;
}

/*
 * Name: numstr - Returns number of substrings in a string
 * Input : d - a pointer to the string to search through
 * Output: returns number of substrings in string d
 * Example: numstr( "bob and    Figment  are THE  Bombz  " ) returns 6
 */
int numstr( char *d )
{

  int k2, k3;
  int cnt = 0;

  k3 = -1;

  /* new version */  
  do {
    k2 = wspace( d, k3+1 ); /* find start of substring */
    if( k2 == -1 ) /* if there is no start, we return */
      return cnt;
    k3 = find( d, " \t", k2 ); /* find end of substring */
    if( k3 == -1 )
      return ++cnt;
    ++cnt;   /* increase counter if there is a start & a finish */
  } while( k3 != -1 );
  return -1;
}

/*
 * Name: parse - Returns specified substrings seperated by whitespace
 * Input: d - a pointer to the string to parse
 *		  i - the number of the substring you want
 *		  c - a pointer to the string where we place the substring
 * Output: Returns nonzero on errors and zero when there are no errors
 * Example: parse( " bob ate  cheese", 3, buffer ) places "cheese" in buffer
 */

int parse( char *d, int i, char *c )
{
  int k, k2, k3, k4;
#ifdef CHECKALL
  if( i > numstr(d) )
    return -1;
#endif /* ifdef CHECKALL */

  k3 = -1;
  
  for(k=1; k<=i; k++) {
    k2 = wspace( d, k3+1 ); /* find start of substring */
    if( k2 == -1 )
      return -1;
    k3 = find( d, " \t", k2 ); /* find end of substring */
  }
  if( k3 == -1 ) { 
    k3 = strlen( d );
  }     
  else
    --k3; 
  
  scopy( c, d, k2, k3-k2 );
   
//  c[k4] = '\0';
 
  return 0;
}

/*
 * Name    : rtrim
 * Descrip : Removes all trailing whitespace from a string
 * Input   : str = pointer to a string to strip
 * Output  : Returns a pointer to str
 * Notes   : None.
 * Example : rtrim( "Bob was busy   " ) - returns "Bob was busy"
 */

char *rtrim( char *str )
{

  int i = strlen( str ) - 1;
  
  while( (isspace(str[i])) && (i>=0) )
    str[i--] = '\0';
  
  return str;
}

/*
 * Name    : ltrim
 * Descrip : Removes all leading whitespace from a string
 * Input   : str = pointer to a string to strip
 * Output  : Retruns a pointer to str
 * Notes   : None
 * Example : ltrim( "  Woof! " ) - returns "Woof! "
 */

char *ltrim( char *str )
{                                  
  int i, w;                  
  
  w = ws( str );
  if( (w != 0) && (w != -1) ) {
    for(i=0; i<strlen(str)-w; i++) 
      str[i] = str[i+w];
    str[i] = '\0';
  }
  return str;
}

/*
 * Name    : rws
 * Descrip : Reverse WhiteSpace: Finds the last charater in a string which
 *           is not a space or a tab
 * Input   : str = pointer to a string to search through
 * Output  : Returns the position of the last non-whitespace character
 * Notes   : Just like, ws(), but backwards
 * Example : rws( "Hey, you!  " ) - returns 8
 */

int rws( char *str)
{
  int k;
  
  for(k=strlen(str);k>-1;k--) {
    if( (str[k] != ' ') && (str[k] != '\t') )
      return k;
  }
  return -1;
}


/*
 * Name    : ws
 * Descrip : WhiteSpace: finds the first character which isn't a tab or space
 * Input   : str = pointer to string to use as input
 * Output  : Returns position of fitsrt non-whitespace character
 * Notes   : none
 * Example : ws( "   Howdy, world!" ) - returns 3
 */

int ws( char *str)
{
  int k;

  for(k=0;k<strlen(str);k++) {
    if( (str[k] != ' ') && (str[k] != '\t') )
      return k;
  }
  return -1;
}




