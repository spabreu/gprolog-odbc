% $Id$

% == predicate declarations for GNU Prolog unixODBC interface =================

/*-------------------------------------------------------------------------*
 * Copyright (C) 2001-2002 Free Software Foundation                        *
 *                                                                         *
 * Authors: David Mendes <dm@di.uevora.pt>                                 *
 *          Salvador Abreu <spa@di.uevora.pt>                              *
 *                                                                         *
 * GProlog-ODBC is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2, or any later version.       *
 *                                                                         *
 * GProlog-ODBC is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc.  *
 * 59 Temple Place - Suite 330, Boston, MA 02111, USA.                     *
 *-------------------------------------------------------------------------*/

% -- SQL3 type codes ----------------------------------------------------------

odbc_type(char,		1).
odbc_type(numeric,	2).
odbc_type(decimal,	3).
odbc_type(integer,	4).
odbc_type(smallint,	5).
odbc_type(float,	6).
odbc_type(real,		7).
odbc_type(double,	8).
odbc_type(datetime,	9).
odbc_type(timestamp,	11).
odbc_type(varchar,	12).

    % dsn,  connection handle
:- foreign(odbc_connect(+string, -integer), [return(boolean)]).

    % connection handle, statement handle
:- foreign(odbc_alloc_stmt(+integer, -integer), [return(boolean)]).

    % statement handle
:- foreign(odbc_release_stmt(+integer), [return(boolean)]).

    % statement handle, Query string
:- foreign(odbc_exec_direct(+integer, +string), [return(boolean)]).

    % statement handle, col_no, type (from the SQL_type list), 
    % generic Prolog term
:- foreign(odbc_get_data(+integer,+integer,+integer, -term), [return(boolean)]).

    % statement handle, row_count
:- foreign(odbc_row_count(+integer,-integer),[return(boolean)]).

    % statement handle
    % if no rows (SQL_NO_DATA) fails 
:- foreign(odbc_fetch(+integer), [return(boolean), choice_size(0)]).

    % connection handle
:- foreign(odbc_disconnect(+integer),[return(boolean)]).
