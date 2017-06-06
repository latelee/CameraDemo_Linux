/*
 *                           _/_/     _/_/_/     _/_/_/_/_/
 *                        _/    _/  _/              _/
 *                       _/         _/             _/
 *                      _/            _/_/        _/
 *                     _/                 _/     _/
 *                     _/   _/      _/    _/    _/
 *                      _/_/         _/_/    _/_/_/
 */
/**
 * @file   my_types.h
 * @author Late Lee
 * @date   2011
 *
 * @brief
 *         基本类型定义，C99标准，Windows及Linux通用
 *
 * @note
 *         1. 本文件不是stdint.h的实现，仅是基本类型定义
 *         2. 参考资料：Wikipedia: http://en.wikipedia.org/wiki/C_data_types
 *         3. 在ARM、X86 32平台测试
 *
 * @log
 *        2012-07-09
 *        添加Uint8 BYTE8等类型，添加E_NOTIMPL等宏定义
 */

/**

关于一些类型长度的测试：

printf("%d %d %d %d %d\n", sizeof(char), sizeof(short), sizeof(int), \
                            sizeof(long), sizeof(long long));
1. windows 32: 1 2 4 4 8 ==> ILP32
2. linux   32: 1 2 4 4 8 ==> ILP32
3. windows 64: 1 2 4 4 8 ==> LLP64
4. linux   64: 1 2 4 8 8 ==> LP64
5.             1 2 8 8 8 ==> ILP64

(速记：I表示int，L表示long，LL表示long long，P表示指针，后面的数字表示位数，
如ILP32表示int/long/指针均为32位)
       short int  long  long long  pointers/size_t
LLP64   16   32    32      64         64          Microsoft Windows (X64/IA-64)
LP64    16   32    64      64         64          Most Unix and Unix-like systems,
                                                  e.g. Solaris, Linux, and Mac OS X;
                                                       z/OS
*/

#ifndef _MY_TYPES_H_
#define _MY_TYPES_H_

#include <limits.h>

#ifdef _MSC_VER /* WIN32 */

/* from http://code.google.com/p/msinttypes/ */
#if _MSC_VER > 1000
#pragma once
#endif

#if (_MSC_VER < 1300)
   typedef signed char       int8_t;
   typedef signed short      int16_t;
   typedef signed int        int32_t;
   typedef unsigned char     uint8_t;
   typedef unsigned short    uint16_t;
   typedef unsigned int      uint32_t;
#else
   typedef signed __int8     int8_t;
   typedef signed __int16    int16_t;
   typedef signed __int32    int32_t;
   typedef unsigned __int8   uint8_t;
   typedef unsigned __int16  uint16_t;
   typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;

/********************************/

#else /* *nix */

/* C99 compiler has this head file */
#include <stdint.h>
/* do not nee this */
#if 0
/* part of stdint.h from GNU C Library */
/* __WORDSIZE defined in <bits/wordsize.h> */
#ifndef __int8_t_defined
# define __int8_t_defined
typedef signed char         int8_t;
typedef short int           int16_t;
typedef int                 int32_t;
# if __WORDSIZE == 64
typedef long int            int64_t;
# else
__extension__
typedef long long int       int64_t;
# endif
#endif

typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int        uint32_t;
# define __uint32_t_defined
#endif
#if __WORDSIZE == 64
typedef unsigned long int       uint64_t;
#else
__extension__
typedef unsigned long long int  uint64_t;
#endif

#endif  /* #if 0 */

#endif /* _MSC_VER */

/********************************/

typedef int8_t      int8;
typedef int8_t      Int8;
typedef int8_t      INT8;
typedef uint8_t     uint8;
typedef uint8_t     u8;
typedef uint8_t     Uint8;
typedef uint8_t     UINT8;

typedef int16_t     int16;
typedef int16_t     Int16;
typedef int16_t     INT16;
typedef uint16_t    u16;
typedef uint16_t    uint16;
typedef uint16_t    Uint16;
typedef uint16_t    UINT16;

typedef int32_t     int32;
typedef int32_t     Int32;
typedef int32_t     INT32;
typedef uint32_t    u32;
typedef uint32_t    uint32;
typedef uint32_t    Uint32;
typedef uint32_t    UINT32;

typedef int64_t     int64;
typedef int64_t     Int64;
typedef int64_t     INT64;
typedef uint64_t    u64;
typedef uint64_t    uint64;
typedef uint64_t    Uint64;
typedef uint64_t    UINT64;
typedef uint64_t    ULONG64;

typedef uint8_t     BYTE8, *PBYTE8, *LPBYTE8;
typedef int8_t      SBYTE8, *PSBYTE8, *LPSBYTE8;
typedef uint16_t    WORD16, *PWORD16, *LPWORD16;
typedef int16_t     SWORD16, *PSWORD16, *LPSWORD16;
typedef uint32_t    WORD32, DWORD32, *PDWORD32, *LPDWORD32;
typedef int32_t     SDWORD32, *PSDWORD32, *LPSDWORD32;
typedef uint64_t    WORD64, DWORD64, QWORD64, *PQWORD64, *LPQWORD64;
typedef int64_t     SQWORD64, *PSQWORD64, *LPSQWORD64;

/********************************************************************/

#ifndef MAX_PATH
#define MAX_PATH    260
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

/********************************************************************/

#ifndef WIN32

#undef far
#undef near
#undef pascal

#define far
#define near

#ifndef CDECL
#define CDECL _cdecl
#endif

#undef FAR
#undef  NEAR
#define FAR     far
#define NEAR    near
#ifndef CONST
#define CONST   const
#endif

typedef unsigned long   DWORD;
typedef int             BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef float           FLOAT;
typedef FLOAT           *PFLOAT;
typedef BOOL near       *PBOOL;
typedef BOOL far        *LPBOOL;
typedef BYTE near       *PBYTE;
typedef BYTE far        *LPBYTE;
typedef int near        *PINT;
typedef int far         *LPINT;
typedef WORD near       *PWORD;
typedef WORD far        *LPWORD;
typedef long far        *LPLONG;
typedef DWORD near      *PDWORD;
typedef DWORD far       *LPDWORD;
typedef void far        *LPVOID;
typedef CONST void far  *LPCVOID;

typedef int             INT;
typedef unsigned int    UINT;
typedef unsigned int    *PUINT;

// Void
typedef void *PVOID;

// Basics
#ifndef VOID
#define VOID    void
typedef char    CHAR;
typedef short   SHORT;
typedef long    LONG;
#endif

// ANSI (Multi-byte Character) types
typedef CHAR    *PCHAR;
typedef CHAR    *LPCH, *PCH;

typedef CONST CHAR *LPCCH, *PCCH;
typedef CHAR *NPSTR;
typedef CHAR *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;

//////////////////////////////////////////// tmp add 2011-11-08
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef LONG HRESULT;

#endif // !_HRESULT_DEFINED

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) _sc
#else // RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#endif // RC_INVOKED

#ifndef S_OK
#define S_OK            ((HRESULT)0x00000000L)
#endif
#ifndef S_FALSE
#define S_FALSE         ((HRESULT)0x00000001L)
#endif
#ifndef E_FAIL
#define E_FAIL          _HRESULT_TYPEDEF_(0x80004005L)
#endif

#ifndef E_NOTIMPL
#define E_NOTIMPL                        _HRESULT_TYPEDEF_(0x80004001L)
#endif

#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY                    _HRESULT_TYPEDEF_(0x8007000EL)
#endif

#ifndef E_INVALIDARG
#define E_INVALIDARG                    _HRESULT_TYPEDEF_(0x80070057L)
#endif

#ifndef E_POINTER
#define E_POINTER                        _HRESULT_TYPEDEF_(0x80004003L)
#endif

#endif  /* WIN32 */

#endif /* _MY_TYPES_H_ */
