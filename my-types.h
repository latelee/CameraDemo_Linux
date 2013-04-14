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
 * @file   my-types.h
 * @author Late Lee <www.latelee.org>
 * @date   Tue Jan 18 2011
 *
 */
#ifndef _MY_TYPES_H
#define _MY_TYPES_H

/**
 * in an x86 platform:
 * char 1
 * short 2
 * int 4
 * long 4
 * long long 8
 */
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)        /* NULL for c */
#endif
#endif  /* for NULL */

#endif  /* _MY_TYPE_H */
