#include <stdint.h>

typedef unsigned int uint;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t ui8;
typedef uint16_t ui16;
typedef uint32_t ui32;
typedef uint64_t ui64;
typedef uint32_t uint;

typedef float f32;
typedef double f64;

#define internal static
#define global static
#define persist static

// Used for text highlighting, since VS doesn't detect #defines specified in the build file.
#ifndef VS_DUMMY
#define DEBUG
#endif

#ifdef DEBUG
#define assert(expr) \
	if(!(expr))        \
	{                        \
		*(int *)0 = 0;         \
	}                        \
	0
#else
#define assert(Expression) (Expression)
#endif
#define UNDEFINED_CODE_PATH assert(false)
// Like UNDEFINED_CODE_PATH, but indicates that the affected code path needs to be worked on to handle the exception properly.
#define UNHANDLED_CODE_PATH assert(false) 

#define ArrLen(arr) (sizeof((arr)) / sizeof((arr)[0]))