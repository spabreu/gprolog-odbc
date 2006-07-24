/* $Id$ */

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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <gprolog.h>
#include <sql.h>
#include <sqlext.h>

#define EMSGSIZE 128            /* longer error msgs will be truncated */
#define DATASIZE 256            /* longer data will be truncated */

static void geterr(const char *,SQLINTEGER,SQLSMALLINT,SQLHANDLE,char *);
//static Bool fillrow(SQLHSTMT, PlTerm *);


#define ODBC_MAX_CONNECTIONS  16
#define ODBC_MAX_STATEMENTS  512

static Bool odbc_initialized = FALSE;
static SQLHDBC odbc_connections[ODBC_MAX_CONNECTIONS];
static SQLHDBC odbc_statements[ODBC_MAX_STATEMENTS];

static inline void initialize_odbc () {
  if (!odbc_initialized) {
    int i;
    for (i=0; i<ODBC_MAX_CONNECTIONS; ++i) odbc_connections[i] = 0;
    for (i=0; i<ODBC_MAX_STATEMENTS;  ++i) odbc_statements[i]  = 0;
    odbc_initialized = TRUE;
  }
}

Bool odbc_disconnect(int i)
{
  SQLHDBC V_OD_hdbc;

  initialize_odbc ();
  V_OD_hdbc = odbc_connections[i];

  if (V_OD_hdbc) {
    SQLDisconnect( V_OD_hdbc );
    SQLFreeHandle( SQL_HANDLE_DBC, V_OD_hdbc );
    odbc_connections[i] = 0;
  }

  return TRUE;
} 

Bool odbc_connect(char *DSN, int *conn)
{
  SQLHENV                  V_OD_Env;     // ODBC environment Handle 
  SQLRETURN                V_OD_erg;     // result of functions
  char                     V_OD_stat[10]; // Status SQL
  SQLINTEGER               V_OD_err;
  char                     V_OD_msg[200];
  SQLSMALLINT              V_OD_mlen;
  SQLHDBC		   V_OD_hdbc;
  int                      i;

  initialize_odbc ();

  V_OD_erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&V_OD_Env);
  if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
    //    fprintf(stderr, "Error AllocHandle\n");
    Pl_Err_System(Create_Atom("Cannot allocate ODBC connection handle"));
    return FALSE;
  }
  V_OD_erg=SQLSetEnvAttr(V_OD_Env, SQL_ATTR_ODBC_VERSION,
			 (void*)SQL_OV_ODBC3, 0); 
  if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
    //    fprintf(stderr, "Error SetEnv\n");
    Pl_Err_System(Create_Atom("Cannot set ODBC environment"));
    SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
    return FALSE;
  }
  V_OD_erg = SQLAllocHandle(SQL_HANDLE_DBC, V_OD_Env, &V_OD_hdbc); 
  if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
    //    fprintf(stderr, "Error AllocHDB %d\n",V_OD_erg);
    Pl_Err_System(Create_Atom("Cannot allocate ODBC connection handle"));
    SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
    return FALSE;
  }
  SQLSetConnectAttr(V_OD_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);
  /*   Não tenho aqui Server, User nem Password porque tenho tudo encapsulado no DSN. No entanto, poder-se-ia fazer com os argumentos aqui */
  V_OD_erg = SQLConnect(V_OD_hdbc, (SQLCHAR*) DSN, SQL_NTS,
			(SQLCHAR*) "", SQL_NTS,
			(SQLCHAR*) "", SQL_NTS);
  if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
    SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc,1, 
		  V_OD_stat, &V_OD_err,V_OD_msg,100,&V_OD_mlen);
    //    fprintf(stderr, "%s (%d)\n",V_OD_msg,V_OD_err);
    SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
    Pl_Err_System(Create_Atom("Cannot connect to the DSN"));
    return FALSE;
  }
  // printf("Connected !\n");

  for (i=0; i<ODBC_MAX_CONNECTIONS; ++i) {
    if (!odbc_connections[i]) {
      odbc_connections[i] = V_OD_hdbc;
      *conn = i;
      return TRUE;
    }
  }
  return FALSE;
}

Bool odbc_alloc_stmt( int conn, int *handle )
{
  SQLRETURN                V_OD_erg;     // result of functions
  char                     V_OD_stat[10]; // Status SQL
  SQLINTEGER               V_OD_err;
  char                     V_OD_msg[200];
  SQLSMALLINT              V_OD_mlen;
  SQLHDBC		  *V_OD_hstmt = NULL;
  SQLHDBC		   V_OD_hdbc;
  int i;

  initialize_odbc ();

  V_OD_hdbc = odbc_connections[conn];
  for (i=0; i<ODBC_MAX_STATEMENTS; ++i) {
    if (!odbc_statements[i]) {
      *handle = i;
      V_OD_hstmt = &odbc_statements[i];
      break;
    }
  }

  if (!V_OD_hstmt)
    return FALSE;

  V_OD_erg=SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc, V_OD_hstmt);
  // printf("O stmt handle é %d \n", V_OD_hstmt);
  if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)){
    fprintf(stderr, "Error in AllocStatement %d\n",V_OD_erg);
    SQLGetDiagRec(SQL_HANDLE_STMT, V_OD_hdbc,1, V_OD_stat,&V_OD_err,V_OD_msg,100,&V_OD_mlen);
    fprintf(stderr, "%s (%d)\n",V_OD_msg,(int)V_OD_err);
    SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hdbc);
    return FALSE;
  }

  return TRUE;
}

Bool odbc_exec_direct(int i , char *query )
{
  SQLRETURN                V_OD_erg;     // result of functions
  SQLHSTMT                 V_OD_hstmt;

  initialize_odbc ();
  V_OD_hstmt = odbc_statements[i];

  V_OD_erg=SQLExecDirect(V_OD_hstmt,query,SQL_NTS);
  switch (V_OD_erg) {
  case SQL_SUCCESS_WITH_INFO:
  case SQL_SUCCESS:
    return TRUE;
  case SQL_INVALID_HANDLE:
    Pl_Err_System (Create_Atom ("odbc_exec_direct: invalid statement handle"));
    return FALSE;
  case SQL_ERROR:
    Pl_Err_System (Create_Atom ("odbc_exec_direct: SQL error"));
    return FALSE;
  case SQL_STILL_EXECUTING:
    Pl_Err_System (Create_Atom ("odbc_exec_direct: still executing"));
    return FALSE;
  case SQL_NEED_DATA:
    Pl_Err_System (Create_Atom ("odbc_exec_direct: need data"));
    return FALSE;
  default:
    {
      static char msg[256];
      sprintf (msg, "odbc_exec_direct: other error (%d)", V_OD_erg);
      Pl_Err_System (Create_Atom (msg));
      return FALSE;
    }
  }
  return TRUE;
}

Bool odbc_bind_col (int i, int col_no, int sql_type, PlTerm *t)
{
  SQLINTEGER V_OD_err;
  SQLHSTMT V_OD_hstmt;

  initialize_odbc ();
  V_OD_hstmt = odbc_statements[i];

  t = (PlTerm *) malloc (sizeof(PlTerm));
  SQLBindCol (V_OD_hstmt,col_no,sql_type, t,0,&V_OD_err);
  return TRUE;
}


Bool odbc_release_stmt (int i)
{
//fprintf (stderr, "[freeing SQLHSTMT %d]\n", hstmt);
  SQLHSTMT hstmt;

  initialize_odbc ();
  hstmt = odbc_statements[i];

  if (hstmt) {
    SQLFreeHandle (SQL_HANDLE_STMT, hstmt);
    odbc_statements[i] = 0;
  }
  return TRUE;
}


Bool odbc_fetch (int i)
{
  SQLRETURN V_OD_erg;		// result of functions
  char emsg[EMSGSIZE];
  SQLHSTMT hstmt;
    
  initialize_odbc ();
  hstmt = odbc_statements[i];

  if (Get_Choice_Counter () == 0) {
//  fprintf (stderr, "[marking for SQLHSTMT %d]\n", hstmt);
    Create_Water_Mark ((void (*)()) odbc_release_stmt, (void*) i);
  }

  V_OD_erg = SQLFetch( hstmt );
  if( V_OD_erg == SQL_NO_DATA ) {
    //      When there is no more data, destroy the choice point and fail
    No_More_Choice();
    return FALSE;
  }
  if( V_OD_erg != SQL_SUCCESS && V_OD_erg != SQL_SUCCESS_WITH_INFO ) {
    geterr( "SQLFetch", V_OD_erg, SQL_HANDLE_STMT, hstmt, emsg );
    Pl_Err_System( Create_Allocate_Atom( emsg ) );
    return FALSE;
  }
  return TRUE;
}


static void
geterr( const char *fname, SQLINTEGER rc, SQLSMALLINT htype, 
        SQLHANDLE handle, char *emsg )
{
  SQLINTEGER diagrc, sqlerr;
  SQLCHAR sqlstate[6];
  SQLSMALLINT emsglen;
    
  diagrc = SQLGetDiagRec
    ( htype, handle, 1, sqlstate, &sqlerr, emsg, EMSGSIZE, &emsglen );
  if( SQL_SUCCESS != diagrc )
    sprintf( emsg, "%s failed - returned %d, no diag", fname, (int) rc );
}

Bool odbc_get_data(int i, SQLINTEGER  col_no, 
                   SQLSMALLINT sql_type, 
                   PlTerm *t)
{
  SQLRETURN                V_OD_erg;     // result of functions
  SQLSMALLINT	buf_SZ=0;
  PlTerm	p_time_stamp[6];
  double dval;
  int ival;
  SQL_TIMESTAMP_STRUCT tval;
  SQLPOINTER	pval = 0;
  SQLSMALLINT	valctype;
  SQLINTEGER valsize, valind = 0;
  char emsg[EMSGSIZE];
  SQLHSTMT V_OD_hstmt = odbc_statements[i];

  switch ( sql_type ) {
  case SQL_REAL:
  case SQL_FLOAT:
  case SQL_DOUBLE:
  case SQL_DECIMAL:
  case SQL_NUMERIC: 
    valctype = SQL_C_DOUBLE;
    pval = &dval;
    valsize = sizeof dval;
    break;

  case SQL_SMALLINT:
  case SQL_INTEGER:
  case 15:			/* SQL_BOOLEAN (from SQL3) */
    valctype = SQL_C_SLONG;
    pval = &ival;
    valsize = sizeof ival;
    break;	  

  case SQL_TIMESTAMP:
    valctype = SQL_C_TIMESTAMP;
    pval = &tval;
    valsize = sizeof tval;
    break;

  default:			/* All char and text types. */
    valctype = SQL_C_CHAR;
    pval = 0;
    valsize = 0;
    break;
  }

  V_OD_erg = SQLGetData(V_OD_hstmt, col_no, valctype, pval, buf_SZ, &valind);  
  // SQLGetData error handling
  if( V_OD_erg != SQL_SUCCESS && V_OD_erg != SQL_SUCCESS_WITH_INFO ) {
    geterr( "SQLGetData", V_OD_erg, SQL_HANDLE_STMT, V_OD_hstmt, emsg );
    Pl_Err_System( Create_Allocate_Atom( emsg ) );
    return FALSE;
  }

  if( valind == SQL_NULL_DATA ) {
    *t = Mk_List( 0 );
    return FALSE;
  }
  if( valind == SQL_NO_TOTAL || DATASIZE < valind ) {
    assert( SQL_C_CHAR == valctype );
    valind = DATASIZE;
  }
  if( valctype == SQL_C_CHAR ) {
    if (valind) {
      pval = calloc( 1, valind + 1 );
      if( !pval ) {
	Pl_Err_System( Create_Atom( "calloc_failed" ) );
	return FALSE;
      }
      V_OD_erg = SQLGetData(V_OD_hstmt, col_no, valctype, pval, valind + 1, 0);
      if( V_OD_erg != SQL_SUCCESS && V_OD_erg != SQL_SUCCESS_WITH_INFO ) {
	geterr("SQLGetData/char",
	       V_OD_erg, SQL_HANDLE_STMT, V_OD_hstmt, emsg );
	Pl_Err_System( Create_Allocate_Atom( emsg ) );
	return FALSE;
      }	  
    }
    else {
      pval = strdup ("");
    }
  }

  // switch over sql_type to extract from the allocated  buffer values for  PlTermassembling
  switch( valctype ) {
  default:
    assert( 0 );
    return FALSE;
  case SQL_C_CHAR:
    *t = Mk_String( pval );
    free( pval );
    break;
  case SQL_C_DOUBLE:
    *t = Mk_Float( dval );
    break;
  case SQL_C_SLONG:
    *t = Mk_Integer( ival );
    break;
  case SQL_C_TIMESTAMP:
    p_time_stamp[0] = Mk_Integer(tval.year);
    p_time_stamp[1] = Mk_Integer(tval.month);
    p_time_stamp[2] = Mk_Integer(tval.day);
    p_time_stamp[3] = Mk_Integer(tval.hour);
    p_time_stamp[4] = Mk_Integer(tval.minute);
    p_time_stamp[5] = Mk_Integer(tval.second);
    *t = Mk_Compound(Create_Atom("dt"), 6, p_time_stamp);
  }
  return TRUE;
} 


Bool odbc_row_count(int i, SQLINTEGER *row_count)
{
  SQLHSTMT V_OD_hstmt = odbc_statements[i];

  SQLRowCount(V_OD_hstmt, row_count);
  return TRUE;
}
