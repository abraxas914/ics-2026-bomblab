#include "defuser.h"
#include <inttypes.h>
#include <stdio.h>

/* Reference for the inspected personal bomb only. uint32_t arithmetic wraps. */
static int emit(char *out, size_t cap, const uint32_t *v, size_t n) {
    size_t used = 0;
    for (size_t i = 0; i < n; ++i) {
        int k = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", v[i]);
        if (k < 0 || (size_t)k >= cap - used) return 1;
        used += (size_t)k;
    }
    return n == 0;
}

int solve_phase_1(uint64_t c, char *out, size_t cap) {
    uint32_t lo = (uint32_t)c, hi = (uint32_t)(c >> 32);
    uint32_t a = (3u * lo + hi) ^ 0xfe23u;
    uint32_t b = (hi + 2u * (uint32_t)(c >> 16)) ^ 0x6e2du;
    uint32_t v = (5u * (b - lo) + 0xc45eu) ^ a;
    return emit(out, cap, &v, 1);
}

int solve_phase_2(uint64_t c, char *out, size_t cap) {
    uint32_t v[6];
    v[0] = ((2u * (uint32_t)c) & 255u) + 13u;
    for (unsigned i = 1; i < 6; ++i)
        v[i] = 5u * (v[i-1] ^ ((uint32_t)((c >> (i + 15)) & 15u) + i));
    return emit(out, cap, v, 6);
}

int solve_phase_3(uint64_t c, char *out, size_t cap) {
    uint32_t v[2] = {(uint32_t)c & 7u, 0};
    uint32_t x = (uint32_t)(c >> 3) ^ (uint32_t)(c >> 32);
    /* Jump-table selector -> case: 2,3,5,7,0,1,6,4. */
    switch (v[0]) {
        case 0: v[1] = (x ^ 0x97f0u) + (x >> 2); break;
        case 1: v[1] = 5u*x + 0x4154bu; break;
        case 2: v[1] = (2u*x) ^ 0xdc5du; break;
        case 3: v[1] = 3u*x + 0x701cu; break;
        case 4: v[1] = (4u*x) ^ 0x937bu; break;
        case 5: v[1] = 5u*x + 0xc476u; break;
        case 6: v[1] = (x ^ 0x71e8u) + (x >> 2); break;
        case 7: v[1] = 5u*x + 0x7206u; break;
    }
    return emit(out, cap, v, 2);
}

static uint32_t walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t salt) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    if (mid == target) return (mid + 0x6bu) ^ salt;
    if (target < mid)
        return (5u * walk(target, lo, mid-1u, salt)) ^ (mid + salt) ^ 0x10u;
    return (5u * walk(target, mid+1u, hi, salt)) ^ (mid + salt) ^ 0x16u;
}
int solve_phase_4(uint64_t c, char *out, size_t cap) {
    uint32_t salt = (uint32_t)(c >> 7) ^ (uint32_t)(c >> 32);
    uint32_t v = walk((uint32_t)c & 127u, 0, 127, salt);
    return emit(out, cap, &v, 1);
}

int solve_phase_5(uint64_t c, char *out, size_t cap) {
    /* Nodes in address order at 0x405ba0, stride 16; next is an index. */
    static const uint32_t value[12] = {
        0xcae7,0x2a7e,0xc9ab,0x172e,0xe262,0x8eea,
        0x50d1,0xf88c,0xaa2a,0x8c1b,0xc7e8,0xd30e};
    static const uint32_t tag[12] = {
        0xb74,0x356,0x634,0x45,0x13c,0x92c,
        0x762,0xa0d,0x514,0x20b,0x82d,0x41c};
    static const unsigned next[12] = {1,11,5,2,12,9,7,3,10,4,6,8};
    static const unsigned head[8] = {0,1,11,8,10,6,7,3};
    unsigned node = head[c & 7u], hops = (unsigned)(c >> 3) & 3u;
    uint32_t acc = (uint32_t)(c >> 6) ^ (uint32_t)(c >> 32) ^ 0xa99du;
    uint32_t v[2];
    for (unsigned i = 0; i <= hops; ++i) {
        acc = (3u*acc + value[node]) ^ tag[node];
        v[1] = tag[node];
        if (i < hops) node = next[node];
    }
    v[0] = acc;
    return emit(out, cap, v, 2);
}

static uint32_t op(unsigned selector, uint32_t x) {
    /* Function-table entries: op0,op2,op5,op5,op1,op3,op3,op4. */
    switch (selector) {
        case 0: return ((x << 1) | (x >> 31)) ^ 0x8b62u;
        case 1: return (x ^ 0x68d2u) + 8u*x;
        case 2: case 3: return ((x << 7) + 0x50a9u) ^ (7u*x);
        case 4: return (x >> 5) | (x << 27);
        case 5: case 6: return (x + 0xdea3u) ^ (x >> 2);
        default: return (x ^ (x >> 3)) + 0x7110u;
    }
}
int solve_phase_6(uint64_t c, char *out, size_t cap) {
    uint32_t x = (uint32_t)(c >> 6) ^ (uint32_t)(c >> 32);
    uint32_t a = op((unsigned)c & 7u, x) ^ 0xf1bfu;
    uint32_t v = op((unsigned)(c >> 3) & 7u, a) + 0x2678u;
    return emit(out, cap, &v, 1);
}

int solve_secret(uint64_t c, uint64_t context, char *out, size_t cap) {
    uint32_t selector = (uint32_t)context & 3u;
    uint32_t a = (uint32_t)c ^ (uint32_t)(c >> 32);
    uint32_t b = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t x = a ^ b;
    uint32_t mixed = ((((x << 5) | (x >> 27)) + 0xab2du) ^ 0xedb2u);
    uint32_t first = mixed ^ ((selector * 0xab2du) ^ (uint32_t)(context >> 16));
    uint32_t second = first ^ (b + selector + 0xedb2u + (a >> 5));
    /* Gate checks OPEN at byte offset 32, then parses three decimal fields. */
    int n = snprintf(out, cap, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32
                     " %" PRIu32 " %" PRIu32, selector, first, second);
    return n < 0 || (size_t)n >= cap;
}
