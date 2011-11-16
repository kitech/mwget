#ifndef __BSWAP_H__
#define __BSWAP_H__


#ifdef __CYGWIN__
//typedef unsigned char uint8_t;
//typedef unsigned int uint32_t;
//typedef unsigned long long int uint64_t;
#elif defined( WIN32)
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;
#else
#include <inttypes.h>
#endif



#ifdef ARCH_X86

//这是gcc内嵌汇编语法
//inline static unsigned short ByteSwap16(unsigned short x)
//{
//  __asm("xchgb %b0,%h0"	:
//        "=q" (x)	:
//        "0" (x));
//    return x;
//}

inline  short bswap_16(const short buf) {
	union {
		short s;
		char c[2];
	} ret,val;
	val.s = buf;
	ret.c[0] = val.c[1];
	ret.c[1] = val.c[0];
	return ret.s;
}

//#define bswap_16(x) ByteSwap16(x)

//inline static unsigned int ByteSwap32(unsigned int x)
//{
//#if __CPU__ > 386
// __asm("bswap	%0":
//      "=r" (x)     :
//#else
// __asm("xchgb	%b0,%h0\n"
//      "	rorl	$16,%0\n"
//      "	xchgb	%b0,%h0":
//      "=q" (x)		:
//#endif
//      "0" (x));
//  return x;
//}
//#define bswap_32(x) ByteSwap32(x)
inline    int bswap_32(const int buf) {
	union {
		int i;
		char c[4];
	} ret,val;
	val.i = buf;
	ret.c[0] = val.c[3];
	ret.c[1] = val.c[2];
	ret.c[2] = val.c[1];
	ret.c[3] = val.c[0];
	return ret.i;
}

//inline static unsigned long long int ByteSwap64(unsigned long long int x)
//{
//  register union { __extension__ unsigned long long int __ll;
//          unsigned long int __l[2]; } __x;
//  asm("xchgl	%0,%1":
//      "=r"(__x.__l[0]),"=r"(__x.__l[1]):
//      "0"(bswap_32((unsigned long)x)),"1"(bswap_32((unsigned long)(x>>32))));
//  return __x.__ll;
//}
//#define bswap_64(x) ByteSwap64(x)

inline   long long int bswap_64(const long long int  buf) {
	union {
		long long int i ;
		char c[8] ;
	} ret, val ;
	val.i=buf ; 
	ret.c[0] = val.c[7] ;
	ret.c[1] = val.c[6] ;
	ret.c[2] = val.c[5] ;
	ret.c[3] = val.c[4] ;
	ret.c[4] = val.c[3] ;
	ret.c[5] = val.c[2] ;
	ret.c[6] = val.c[1] ;
	ret.c[7] = val.c[0] ;
	return ret.i ;
}
//: userlevel=Advanced
//: Swap bytes of a 64 bit number

#else

#define bswap_16(x) (((x) & 0x00ff) << 8 | ((x) & 0xff00) >> 8)


// code from bits/byteswap.h (C) 1997, 1998 Free Software Foundation, Inc.
#define bswap_32(x) \
	((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
	(((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define bswap_64(x) \
	(__extension__						\
	({ union { __extension__ unsigned long long int __ll;	\
	unsigned long int __l[2]; } __w, __r;		\
	__w.__ll = (x);					\
	__r.__l[0] = bswap_32 (__w.__l[1]);			\
	__r.__l[1] = bswap_32 (__w.__l[0]);			\
	__r.__ll; }))
#endif	/* !ARCH_X86 */



// be2me ... BigEndian to MachineEndian
// le2me ... LittleEndian to MachineEndian

#ifdef WORDS_BIGENDIAN
#define be2me_16(x) (x)
#define be2me_32(x) (x)
#define be2me_64(x) (x)
#define le2me_16(x) bswap_16(x)
#define le2me_32(x) bswap_32(x)
#define le2me_64(x) bswap_64(x)
#else
#define be2me_16(x) bswap_16(x)
#define be2me_32(x) bswap_32(x)
#define be2me_64(x) bswap_64(x)
#define le2me_16(x) (x)
#define le2me_32(x) (x)
#define le2me_64(x) (x)
#endif

#ifdef __cplusplus
extern "C"{
#endif


int mms_client_main (int argc, char **argv) ;

#ifdef __cplusplus
};
#endif

#endif
