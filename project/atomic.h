
#ifndef _RTE_ATOMIC_H_
#define _RTE_ATOMIC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef asm
#define asm __asm__
#endif

#if 0
#define MPLOCKED                        /**< No need to insert MP lock prefix. */
#else
#define MPLOCKED        "lock ; "       /**< Insert MP lock prefix. */
#endif

/*------------------------- 32 bit atomic operations -------------------------*/

/**
 * The atomic counter structure.
 */
typedef struct {
    volatile int32_t cnt; /**< An internal counter value. */
} rte_atomic32_t;


static inline int
rte_atomic32_cmpset(volatile uint32_t *dst, uint32_t exp, uint32_t src)
{
    uint8_t res;

    asm volatile(
            MPLOCKED
            "cmpxchgl %[src], %[dst];"
            "sete %[res];"
            : [res] "=a" (res),     /* output */
              [dst] "=m" (*dst)
            : [src] "r" (src),      /* input */
              "a" (exp),
              "m" (*dst)
            : "memory");            /* no-clobber list */
    return res;
}

static inline uint32_t
rte_atomic32_exchange(volatile uint32_t *dst, uint32_t val)
{
    asm volatile(
            MPLOCKED
            "xchgl %0, %1;"
            : "=r" (val), "=m" (*dst)
            : "0" (val),  "m" (*dst)
            : "memory");         /* no-clobber list */
    return val;
}


static inline void
rte_atomic32_init(rte_atomic32_t *v)
{
    v->cnt = 0;
}


static inline void
rte_atomic32_inc(rte_atomic32_t *v)
{
    asm volatile(
            MPLOCKED
            "incl %[cnt]"
            : [cnt] "=m" (v->cnt)   /* output */
            : "m" (v->cnt)          /* input */
            );
}

static inline void
rte_atomic32_dec(rte_atomic32_t *v)
{
    asm volatile(
            MPLOCKED
            "decl %[cnt]"
            : [cnt] "=m" (v->cnt)   /* output */
            : "m" (v->cnt)          /* input */
            );
}

static inline int
rte_atomic32_read(rte_atomic32_t *v)
{
    return v->cnt;
}

/*------------------------- 64 bit atomic operations -------------------------*/

/**
 * The atomic counter structure.
 */
typedef struct {
    volatile int64_t cnt;  /**< Internal counter value. */
} rte_atomic64_t;


static inline int
rte_atomic64_cmpset(volatile uint64_t *dst, uint64_t exp, uint64_t src)
{
    uint8_t res;


    asm volatile(
            MPLOCKED
            "cmpxchgq %[src], %[dst];"
            "sete %[res];"
            : [res] "=a" (res),     /* output */
              [dst] "=m" (*dst)
            : [src] "r" (src),      /* input */
              "a" (exp),
              "m" (*dst)
            : "memory");            /* no-clobber list */

    return res;
}

static inline uint64_t
rte_atomic64_exchange(volatile uint64_t *dst, uint64_t val)
{
    asm volatile(
            MPLOCKED
            "xchgq %0, %1;"
            : "=r" (val), "=m" (*dst)
            : "0" (val),  "m" (*dst)
            : "memory");         /* no-clobber list */
    return val;
}

static inline void
rte_atomic64_init(rte_atomic64_t *v)
{
    v->cnt = 0;
}

static inline int64_t
rte_atomic64_read(rte_atomic64_t *v)
{
    return v->cnt;
}

static inline void
rte_atomic64_set(rte_atomic64_t *v, int64_t new_value)
{
    v->cnt = new_value;
}

static inline void
rte_atomic64_add(rte_atomic64_t *v, int64_t inc)
{
    asm volatile(
            MPLOCKED
            "addq %[inc], %[cnt]"
            : [cnt] "=m" (v->cnt)   /* output */
            : [inc] "ir" (inc),     /* input */
              "m" (v->cnt)
            );
}

static inline void
rte_atomic64_sub(rte_atomic64_t *v, int64_t dec)
{
    asm volatile(
            MPLOCKED
            "subq %[dec], %[cnt]"
            : [cnt] "=m" (v->cnt)   /* output */
            : [dec] "ir" (dec),     /* input */
              "m" (v->cnt)
            );
}

static inline void
rte_atomic64_inc(rte_atomic64_t *v)
{
    asm volatile(
            MPLOCKED
            "incq %[cnt]"
            : [cnt] "=m" (v->cnt)   /* output */
            : "m" (v->cnt)          /* input */
            );
}

static inline void
rte_atomic64_dec(rte_atomic64_t *v)
{
    asm volatile(
            MPLOCKED
            "decq %[cnt]"
            : [cnt] "=m" (v->cnt)   /* output */
            : "m" (v->cnt)          /* input */
            );
}

static inline int64_t
rte_atomic64_add_return(rte_atomic64_t *v, int64_t inc)
{
    int64_t prev = inc;

    asm volatile(
            MPLOCKED
            "xaddq %[prev], %[cnt]"
            : [prev] "+r" (prev),   /* output */
              [cnt] "=m" (v->cnt)
            : "m" (v->cnt)          /* input */
            );
    return prev + inc;
}

static inline int64_t
rte_atomic64_sub_return(rte_atomic64_t *v, int64_t dec)
{
    return rte_atomic64_add_return(v, -dec);
}

#endif
