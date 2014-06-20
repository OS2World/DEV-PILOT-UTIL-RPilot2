#ifndef _PARSE_H_
#define _PARSE_H_

/*
 * FILE: parse.h
 * DESC: Header file for parse.c
 * VERS: 1.2
 * COPY: Copyright 1998 Rob Linwood (auntfloyd@biosys.net)
 * XREF: parse.c
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

#ifdef __cplusplus
extern "C"
{
#endif



#define wspace(x, y) neither( x, ' ', '\t', y )

/* Functions in parse.c */
extern int firstnot( char *d, const char e, char first );
extern int neither( char *d, const char e, const char f, char first );
extern int find( char *d, char *e, int first );
extern int parse( char *d, int i, char *c );
extern int numstr( char *d );
extern char *rtrim( char *str );
extern int rws( char *str);
extern char *scopy( char *dest, char *src, int index, int count );
extern int ws( char *str );
extern char *ltrim( char *str );

#ifdef __cplusplus
}
#endif


#endif /* ifndef _PARSE_H_ */
