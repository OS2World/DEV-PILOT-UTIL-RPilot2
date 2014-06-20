#ifndef _RSTRING_H_
#define _RSTRING_H_

/*
 * FILE: rstring.h
 * DESC: Header file for rstring.c
 * VERS: 1.0
 * COPY: Copyright 1998 Rob Linwood (auntfloyd@biosys.net)
 * XREF: rstring.c
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

#ifdef __cplusplus
extern "C"
{
#endif

char *strupr( char *s );
char *strset( char *s, int ch );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _RSTRING_H_ */
