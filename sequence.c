/*
BSD 3-Clause License

Copyright (c) 2017, Sylvain Pointeau (sylvain.pointeau@gmail.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

int is_temp_table_created = 0;

void sp_seq_init(sqlite3_context *context, int argc, sqlite3_value **argv) {
  int rc = 0;
  sqlite3_stmt *stmt; 
  
  sqlite3 *db = sqlite3_context_db_handle(context);
  const unsigned char* seq_name = sqlite3_value_text(argv[0]);
  long seq_init_val = sqlite3_value_int64(argv[1]);
  long seq_inc_val = argc < 3 ? 1 : sqlite3_value_int64(argv[2]);
    
  rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS SP_SEQUENCE ( " \
  " SEQ_NAME TEXT NOT NULL PRIMARY KEY, " \
  " SEQ_VAL INTEGER, " \
  " SEQ_INIT INTEGER NOT NULL, " \
  " SEQ_INC INTEGER NOT NULL CHECK (SEQ_INC<>0) " \
  " )", 0, 0, 0);

  if( rc != SQLITE_OK ) {
    sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
    return;
  }
  
  sqlite3_prepare_v2(db, "insert or replace into sp_sequence (seq_name, seq_val, seq_init, seq_inc) values (?, ?, ?, ?)", -1, &stmt, 0);
  
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 2, seq_init_val-seq_inc_val);
  sqlite3_bind_int64(stmt, 3, seq_init_val);
  sqlite3_bind_int64(stmt, 4, seq_inc_val);
  
  rc = sqlite3_step(stmt);
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE) {
    sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
    return;
  }

  // init means that we wait for a nextval before a currval  
  sqlite3_prepare_v2(db, "delete from SP_TEMP_SEQ_CURRVAL where seq_name = ?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  // we don't really care if there is any error on the TEMP table
  
  sqlite3_result_int64( context, seq_init_val );
}

void sp_seq_nextval(sqlite3_context *context, int argc, sqlite3_value **argv) {
  int rc = 0;
  int update_row_count = 0;
  sqlite3_stmt *stmt; 
  long nextval = 0;

  sqlite3 *db = sqlite3_context_db_handle(context);
  const unsigned char* seq_name = sqlite3_value_text(argv[0]);
  
  sqlite3_prepare_v2(db, "update sp_sequence set seq_val = seq_val + seq_inc where seq_name = ?", -1, &stmt, 0);
  
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  
  rc = sqlite3_step(stmt);
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE) {
    sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
    return;
  }

  sqlite3_prepare_v2(db, "select seq_val from sp_sequence where seq_name = ?", -1, &stmt, 0);
  
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  
  rc = sqlite3_step(stmt);
  
  if( rc == SQLITE_ROW) {
    nextval = sqlite3_column_int64(stmt, 0);
  }
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_ROW) {
    if( rc == SQLITE_DONE ) sqlite3_result_error(context, "sequence name does not exist", -1);
    else sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
    return;
  }

  if( is_temp_table_created == 0 ) {
  
    rc = sqlite3_exec(db, "CREATE TEMPORARY TABLE IF NOT EXISTS SP_TEMP_SEQ_CURRVAL ( " \
    " SEQ_NAME TEXT PRIMARY KEY, " \
    " CURRVAL INTEGER " \
    " )", 0, 0, 0);
  
    if( rc != SQLITE_OK ) {
      sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
      return;
    }  
    
    is_temp_table_created = 1;
  }

  sqlite3_prepare_v2(db, "update SP_TEMP_SEQ_CURRVAL set currval = ? where seq_name = ?", -1, &stmt, 0);
  
  sqlite3_bind_int64(stmt, 1, nextval);
  sqlite3_bind_text(stmt, 2, seq_name, -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE) {
    sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
    return;
  }
  
  update_row_count = sqlite3_changes(db);
  
  if (update_row_count == 0) {
    // update not done, value has to be inserted
    
    sqlite3_prepare_v2(db, "insert into SP_TEMP_SEQ_CURRVAL (seq_name, currval) values (?,?)", -1, &stmt, 0);
    
    sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, nextval);
  
    rc = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
      sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
      return;
    }
  }

  sqlite3_result_int64( context, nextval );
}

void sp_seq_currval(sqlite3_context *context, int argc, sqlite3_value **argv) {
  int rc = 0;
  sqlite3_stmt *stmt; 
  long currval = 0;
   
  sqlite3 *db = sqlite3_context_db_handle(context);
  const unsigned char* seq_name = sqlite3_value_text(argv[0]);

  sqlite3_prepare_v2(db, "select currval from SP_TEMP_SEQ_CURRVAL where seq_name = ?", -1, &stmt, 0);
  
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  
  rc = sqlite3_step(stmt);
  
  if( rc == SQLITE_ROW) {
    currval = sqlite3_column_int64(stmt, 0);
  }
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_ROW) {
    sqlite3_result_error(context, "currval is not yet defined in this session for this sequence", -1);
    return;
  }

  sqlite3_result_int64( context, currval );
}

void sp_seq_drop(sqlite3_context *context, int argc, sqlite3_value **argv) {
  int rc = 0;
  sqlite3_stmt *stmt; 
  long currval = 0;
   
  sqlite3 *db = sqlite3_context_db_handle(context);
  const unsigned char* seq_name = sqlite3_value_text(argv[0]);

  sqlite3_prepare_v2(db, "delete from sp_sequence where seq_name = ?", -1, &stmt, 0);
  
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE) {
    sqlite3_result_error(context, sqlite3_errmsg(db), -1); 
    return;
  }

  sqlite3_prepare_v2(db, "delete from SP_TEMP_SEQ_CURRVAL where seq_name = ?", -1, &stmt, 0);
  sqlite3_bind_text(stmt, 1, seq_name, -1, SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  // we don't really care if there is any error on the TEMP table
  
  sqlite3_result_null( context );
}


int sqlite3_extension_init(
      sqlite3 *db,
      char **pzErrMsg,
      const sqlite3_api_routines *pApi
){
 SQLITE_EXTENSION_INIT2(pApi)
 sqlite3_create_function(db, "seq_init", 3, SQLITE_UTF8, 0, sp_seq_init, 0, 0);
 sqlite3_create_function(db, "seq_init", 2, SQLITE_UTF8, 0, sp_seq_init, 0, 0);
 sqlite3_create_function(db, "seq_nextval", 1, SQLITE_UTF8, 0, sp_seq_nextval, 0, 0);
 sqlite3_create_function(db, "seq_currval", 1, SQLITE_UTF8, 0, sp_seq_currval, 0, 0);
 sqlite3_create_function(db, "seq_drop", 1, SQLITE_UTF8, 0, sp_seq_drop, 0, 0);
 return 0;
}

