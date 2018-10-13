#include <eosiolib/eosio.hpp>

using namespace eosio;
using namespace std;


// uint64_t modExp(__uint128_t base, __uint128_t exp, uint64_t mod) {
//     uint64_t result = 1;
//     while (exp > 0) {
//         if ((exp & 1) == 1) {
//             result = (result*base) % mod;
//         }
//         base = (base*base) % mod;
//         exp >>= 1;
//     }
//     return result;
// }

// uint64_t modMul(__uint128_t a, __uint128_t b, uint64_t mod) {
//     if ((a == 0) || (b < (mod / a))) return (a*b) % mod;
//     uint64_t result = 0;
//     a = a % mod;
//     while (b > 0) {
//         // if b is odd, add a to the result
//         if ((b & 1) == 1) {
//             result = (result + a) % mod;
//         }
//         a = (a<<1) % mod;
//         b >>= 1;
//     }
//     return result;
// }

// uint64_t randBetween(uint64_t min, uint64_t max) {
//     // assuming rand() is 32-bits
//     uint64_t r = (((uint64_t) rand()) << 32) | (((uint64_t) rand()) & UINT32_MAX);
//     uint64_t result = min + (uint64_t) ((double)(max-min+1)*r/(UINT64_MAX+1.0));
//     return result;
// }


// bool isPrime(uint64_t n, int k) {
//     // Corner cases
//     if ( n == 2 || n == 3 ) return true;
//     if ( n <= 1 || !(n & 1) ) return false;
//     // n is odd and greater than 3
//     // r is the power of 2 that divides n-1
//     int r = 0;
//     // d will the odd number left over after dividing out the 2s
//     uint64_t d = n-1;
//     while ((d & 1) == 0) {
//         d >>= 1;
//         r++;
//     }
//     // witness loop, repeat k times
//     for (int i = 0;i<k;i++) {
//         uint64_t a = randBetween(2,n-2);
//         uint64_t x = modExp(a,d,n);
//         if ((x == 1) || (x == n-1)) continue;
//         // repeat r-1 times
//         for (int j = 1;j <= (r-1);j++) {
//             x = modExp(x,2,n);
//             if (x == 1) return false;
//             if (x == n-1) goto KLOOP;
//         }
//         return false;
// KLOOP:
//         continue;
//     }
//     return true;
// }
