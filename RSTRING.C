/*
 * rstring.c
 * This file implements some of Turbo/Borland C++'s string functions
 * written by Rob Linwood (auntfloyd@biosys.net)
 * Define the macro STANDALONE to get a small, standalone test program
 */

   /**********************************************************************
    RPilot: Rob's PILOT Interpreter
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


#include <ctype.h>

#ifdef STANDALONE
char *strupr( char *s );
char *strset( char *s, int ch );

void main()
{
    char text[10] = "Hello!";

    printf( "strupr( \"Dilbert\" ) = %s\n", strupr( "Dilbert" ) );
    printf( "strset( text, 'C' ) = %s\n", strset( text, 'C' ) );
}

#endif  /* ifdef STANDALONE */

/*
 * Name    : strupr
 * Descrip : Changes s to all uppercase
 * Input   : s = pointer to a string to uppercase
 * Output  : returns a pointer to s
 * Notes   : none
 * Example : strupr( "proper-noun" ) - returns "PROPER-NOUN"
 */

char *strupr( char *s )
{
	unsigned char c;

    for(c=0; s[c] != 0; c++)
		s[c] = toupper(s[c]);

	return s;
}

/*
 * Name    : strset
 * Descrip : Sets all characters in s to ch
 * Input   : s = pointer to a string to set
 *           ch = character to set all positions in s to 
 * Output  : returns a pointer to s
 * Notes   : None
 * Example : char cstring[10];
             strset( cstring, 'C' ) - returns "CCCCCCCCC"
 */

char *strset( char *s, int ch )
{
	unsigned char c;

    for(c=0; s[c] != 0; c++)
		s[c] = ch;

	return s;
}

