/* Minimal SQLite C API declarations used by MY-BASIC shell.
 * This local header avoids requiring external sqlite3 development headers at build time.
 */
#ifndef MY_BASIC_LOCAL_SQLITE3_H
#define MY_BASIC_LOCAL_SQLITE3_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101

#ifdef __cplusplus
}
#endif

#endif /* MY_BASIC_LOCAL_SQLITE3_H */
