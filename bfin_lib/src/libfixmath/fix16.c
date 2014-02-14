#include "fix16.h"
#include "int64.h"


/* Subtraction and addition with overflow detection.
 * The versions without overflow detection are inlined in the header.
 */
#ifndef FIXMATH_NO_OVERFLOW
fix16_t fix16_add(fix16_t a, fix16_t b)
{
  // Use unsigned integers because overflow with signed integers is
  // an undefined operation (http://www.airs.com/blog/archives/120).
  uint32_t _a = a, _b = b;
  uint32_t sum = _a + _b;

  // Overflow can only happen if sign of a == sign of b, and then
  // it causes sign of sum != sign of a.
  if (!((_a ^ _b) & 0x80000000) && ((_a ^ sum) & 0x80000000))
    return fix16_overflow;
  
  return sum;
}

fix16_t fix16_sub(fix16_t a, fix16_t b)
{
  uint32_t _a = a, _b = b;
  uint32_t diff = _a - _b;

  // Overflow can only happen if sign of a != sign of b, and then
  // it causes sign of diff != sign of a.
  if (((_a ^ _b) & 0x80000000) && ((_a ^ diff) & 0x80000000))
    return fix16_overflow;
  
  return diff;
}

/* Saturating arithmetic */
fix16_t fix16_sadd(fix16_t a, fix16_t b)
{
  fix16_t result = fix16_add(a, b);

  if (result == fix16_overflow)
    return (a > 0) ? fix16_max : fix16_min;

  return result;
}  

fix16_t fix16_ssub(fix16_t a, fix16_t b)
{
  fix16_t result = fix16_sub(a, b);

  if (result == fix16_overflow)
    return (a > 0) ? fix16_max : fix16_min;

  return result;
}
#endif


/* 32-bit implementation of fix16_div. Fastest version for e.g. ARM Cortex M3.
 * Performs 32-bit divisions repeatedly to reduce the remainder. For this to
 * be efficient, the processor has to have 32-bit hardware division.
 */
#if !defined(FIXMATH_OPTIMIZE_8BIT)
#ifdef __GNUC__
// Count leading zeros, using processor-specific instruction if available.
#define clz(x) __builtin_clzl(x)
#else
static uint8_t clz(uint32_t x)
{
  uint8_t result = 0;
  if (x == 0) return 32;
  while (!(x & 0xF0000000)) { result += 4; x <<= 4; }
  while (!(x & 0x80000000)) { result += 1; x <<= 1; }
  return result;
}
#endif

fix16_t fix16_div(fix16_t a, fix16_t b)
{
  // This uses a hardware 32/32 bit division multiple times, until we have
  // computed all the bits in (a<<17)/b. Usually this takes 1-3 iterations.
  
  if (b == 0)
      return fix16_min;
  
  uint32_t remainder = (a >= 0) ? a : (-a);
  uint32_t divider = (b >= 0) ? b : (-b);
  uint32_t quotient = 0;
  int bit_pos = 17;
  
  // Kick-start the division a bit.
  // This improves speed in the worst-case scenarios where N and D are large
  // It gets a lower estimate for the result by N/(D >> 17 + 1).
  if (divider & 0xFFF00000)
  {
    uint32_t shifted_div = ((divider >> 17) + 1);
    quotient = remainder / shifted_div;
    remainder -= ((uint64_t)quotient * divider) >> 17;
  }
  
  // If the divider is divisible by 2^n, take advantage of it.
  while (!(divider & 0xF) && bit_pos >= 4)
  {
    divider >>= 4;
    bit_pos -= 4;
  }
  
  while (remainder && bit_pos >= 0)
  {
    // Shift remainder as much as we can without overflowing
    int shift = clz(remainder);
    if (shift > bit_pos) shift = bit_pos;
    remainder <<= shift;
    bit_pos -= shift;
    
    uint32_t div = remainder / divider;
    remainder = remainder % divider;
    quotient += div << bit_pos;

    #ifndef FIXMATH_NO_OVERFLOW
    if (div & ~(0xFFFFFFFF >> bit_pos))
        return fix16_overflow;
    #endif
    
    remainder <<= 1;
    bit_pos--;
  }
  
  #ifndef FIXMATH_NO_ROUNDING
  // Quotient is always positive so rounding is easy
  quotient++;
  #endif
  
  fix16_t result = quotient >> 1;
  
  // Figure out the sign of the result
  if ((a ^ b) & 0x80000000)
  {
    #ifndef FIXMATH_NO_OVERFLOW
    if (result == fix16_min)
        return fix16_overflow;
    #endif
    
    result = -result;
  }
  
  return result;
}
#endif

/* Alternative 32-bit implementation of fix16_div. Fastest on e.g. Atmel AVR.
 * This does the division manually, and is therefore good for processors that
 * do not have hardware division.
 */
#if defined(FIXMATH_OPTIMIZE_8BIT)
fix16_t fix16_div(fix16_t a, fix16_t b)
{
  // This uses the basic binary restoring division algorithm.
  // It appears to be faster to do the whole division manually than
  // trying to compose a 64-bit divide out of 32-bit divisions on
  // platforms without hardware divide.
  
  if (b == 0)
    return fix16_min;
  
  uint32_t remainder = (a >= 0) ? a : (-a);
  uint32_t divider = (b >= 0) ? b : (-b);

  uint32_t quotient = 0;
  uint32_t bit = 0x10000;
  
  /* The algorithm requires D >= R */
  while (divider < remainder)
  {
    divider <<= 1;
    bit <<= 1;
  }
  
  #ifndef FIXMATH_NO_OVERFLOW
  if (!bit)
    return fix16_overflow;
  #endif
  
  if (divider & 0x80000000)
  {
    // Perform one step manually to avoid overflows later.
    // We know that divider's bottom bit is 0 here.
    if (remainder >= divider)
    {
        quotient |= bit;
        remainder -= divider;
    }
    divider >>= 1;
    bit >>= 1;
  }
  
  /* Main division loop */
  while (bit && remainder)
  {
    if (remainder >= divider)
    {
        quotient |= bit;
        remainder -= divider;
    }
    
    remainder <<= 1;
    bit >>= 1;
  }   
      
  #ifndef FIXMATH_NO_ROUNDING
  if (remainder >= divider)
  {
    quotient++;
  }
  #endif
  
  fix16_t result = quotient;
  
  /* Figure out the sign of result */
  if ((a ^ b) & 0x80000000)
  {
    #ifndef FIXMATH_NO_OVERFLOW
    if (result == fix16_min)
        return fix16_overflow;
    #endif
    
    result = -result;
  }
  
  return result;
}
#endif

#ifndef FIXMATH_NO_OVERFLOW
/* Wrapper around fix16_div to add saturating arithmetic. */
fix16_t fix16_sdiv(fix16_t inArg0, fix16_t inArg1) {
  fix16_t result = fix16_div(inArg0, inArg1);
  
  if (result == fix16_overflow)
  {
    if ((inArg0 >= 0) == (inArg1 >= 0))
      return fix16_max;
    else
      return fix16_min;
  }
  
  return result;
}
#endif

fix16_t fix16_lerp8(fix16_t inArg0, fix16_t inArg1, uint8_t inFract) {
	int64_t tempOut = int64_mul_i32_i32(inArg0, ((1 << 8) - inFract));
	tempOut = int64_add(tempOut, int64_mul_i32_i32(inArg1, inFract));
	tempOut = int64_shift(tempOut, -8);
	return (fix16_t)int64_lo(tempOut);
}

fix16_t fix16_lerp16(fix16_t inArg0, fix16_t inArg1, uint16_t inFract) {
	int64_t tempOut = int64_mul_i32_i32(inArg0, ((1 << 16) - inFract));
	tempOut = int64_add(tempOut, int64_mul_i32_i32(inArg1, inFract));
	tempOut = int64_shift(tempOut, -16);
	return (fix16_t)int64_lo(tempOut);
}

#ifndef FIXMATH_NO_64BIT
fix16_t fix16_lerp32(fix16_t inArg0, fix16_t inArg1, uint32_t inFract) {
	int64_t tempOut;
	tempOut   = ((int64_t)inArg0 * (0 - inFract));
	tempOut  += ((int64_t)inArg1 * inFract);
	tempOut >>= 32;
	return (fix16_t)tempOut;
}
#endif
