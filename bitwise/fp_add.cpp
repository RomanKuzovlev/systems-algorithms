#include <cstdint>

uint32_t fp_add(uint32_t a_bits, uint32_t b_bits)
{
    // Extract sign, exponent, mantissa
    uint32_t a_sign = a_bits >> 31;
    uint32_t a_exp = (a_bits >> 23) & 0xFF;
    uint32_t a_man = a_bits & 0x007FFFFF;

    uint32_t b_sign = b_bits >> 31;
    uint32_t b_exp = (b_bits >> 23) & 0xFF;
    uint32_t b_man = b_bits & 0x007FFFFF;

    // Check for NaNs
    bool a_nan = (a_exp == 0xFF && a_man != 0);
    bool b_nan = (b_exp == 0xFF && b_man != 0);
    if (a_nan)
        return a_bits;
    if (b_nan)
        return b_bits;

    // Check for infinities
    bool a_inf = (a_exp == 0xFF && a_man == 0);
    bool b_inf = (b_exp == 0xFF && b_man == 0);

    if (a_inf && b_inf)
    {
        if (a_sign != b_sign)
            return 0x7FC00000; // inf - inf = NaN
        return a_bits;         // same-signed infinities -> return one
    }

    if (a_inf)
        return a_bits;
    if (b_inf)
        return b_bits;

    // Check for zeros
    bool a_zero = (a_exp == 0 && a_man == 0);
    bool b_zero = (b_exp == 0 && b_man == 0);

    if (a_zero && b_zero)
    {
        // Return −0 only if both are −0, else +0; -0 is when exp and mantissa are 0, but sign is 1
        return (a_sign & b_sign) << 31;
    }
    if (a_zero)
        return b_bits;
    if (b_zero)
        return a_bits;

    // Add implicit leading 1 if normalized
    uint32_t a_frac = (a_exp == 0) ? a_man : (1 << 23) | a_man;
    uint32_t b_frac = (b_exp == 0) ? b_man : (1 << 23) | b_man;

    // Adjust exponent for subnormals (treat subnormals as if exp = 1)
    uint32_t real_a_exp = (a_exp == 0) ? 1 : a_exp;
    uint32_t real_b_exp = (b_exp == 0) ? 1 : b_exp;

    return 0;
}