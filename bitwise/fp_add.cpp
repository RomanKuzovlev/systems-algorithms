#include <iostream>
#include <iomanip>
#include <cstdint>

// IEEE 754 is a masterpiece — massive respect to its creators.
// Implementing it is tough (I used AI as an assistant).
// Inventing it is beyond imagination.

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
        return (a_sign != b_sign ? 0x7FC00000u : a_bits);
    if (a_inf)
        return a_bits;
    if (b_inf)
        return b_bits;

    // Check for zeros
    bool a_zero = (a_exp == 0 && a_man == 0);
    bool b_zero = (b_exp == 0 && b_man == 0);

    if (a_zero && b_zero)
    {
        // Return −0 only if both are −0, else +0;
        // -0 is when exp and mantissa are 0, but sign is 1
        return (a_sign & b_sign) << 31;
    }
    if (a_zero)
        return b_bits;
    if (b_zero)
        return a_bits;

    // Rebuild the fractions and reserve 2 low bits (G & R)
    uint32_t a_frac = (a_exp ? (1u << 23) | a_man : a_man) << 2;
    uint32_t b_frac = (b_exp ? (1u << 23) | b_man : b_man) << 2;

    int a_e = a_exp ? a_exp : 1; // subnormal exponent is 1
    int b_e = b_exp ? b_exp : 1;

    uint32_t sticky = 0; // any 1s we shift out live here

    // Align exponents by shifting the smaller fraction
    if (a_e != b_e)
    {
        if (a_e < b_e)
        { // make a the larger‑exp operand
            std::swap(a_e, b_e);
            std::swap(a_frac, b_frac);
            std::swap(a_sign, b_sign);
        }
        uint32_t shift = a_e - b_e;
        if (shift >= 32)
        {
            sticky |= (b_frac != 0);
            b_frac = 0;
        }
        else
        {
            sticky |= b_frac & ((1u << shift) - 1u);
            b_frac >>= shift;
        }
        b_e = a_e;
    }

    // Same sign -> ADD
    uint32_t result_frac;
    uint32_t result_sign;
    if (a_sign == b_sign)
    {
        result_frac = a_frac + b_frac;
        result_sign = a_sign;
    }
    else
    {
        // Different signs -> SUBTRACT
        if (a_frac > b_frac)
        {
            result_frac = a_frac - b_frac;
            result_sign = a_sign;
        }
        else if (b_frac > a_frac)
        {
            result_frac = b_frac - a_frac;
            result_sign = b_sign;
        }
        else
        {
            return 0u;
        } // exact cancellation -> +0
    }

    int result_exp = a_e; // both equal now

    // Normalization
    if (result_frac & (1u << 26))
    {                               // carry into bit 26
        sticky |= result_frac & 1u; // bit that falls off
        result_frac >>= 1;
        if (++result_exp >= 255)
            return (result_sign << 31) | (0xFFu << 23); // overflow -> +-∞
    }

    while ((result_frac & (1u << 25)) == 0 && result_exp > 0)
    {
        result_frac <<= 1;
        --result_exp;
    }

    if (result_exp == 0)
    { // subnormal shift
        uint32_t shift = 1;
        sticky |= result_frac & ((1u << shift) - 1u);
        result_frac >>= shift;
    }

    // Guard / round bits
    uint32_t guard = (result_frac >> 1) & 1u; // bit 1
    uint32_t round = result_frac & 1u;        // bit 0

    result_frac >>= 2;                            // drop G & R
    uint32_t mantissa = result_frac & 0x007FFFFF; // strip hidden 1

    // Round‑to‑nearest, ties‑to‑even
    bool round_up = false;
    if (guard)
    {
        round_up = round | sticky;        // > half
        if (!round_up && (mantissa & 1u)) // exactly half & odd -> up
            round_up = true;
    }

    if (round_up)
    {
        ++mantissa;
        if (mantissa >> 23)
        { // mantissa overflow
            mantissa &= 0x007FFFFF;
            if (++result_exp >= 255)
                return (result_sign << 31) | (0xFFu << 23); // +-∞
        }
    }

    return (result_sign << 31) | (static_cast<uint32_t>(result_exp) << 23) | mantissa;
}

// Test
struct TestCase
{
    const char *name;
    uint32_t a_bits, b_bits, expected;
};

int main()
{
    TestCase tests[] = {
        {"pi + 1.5", 0x40490fdb, 0x3fc00000, 0x409487ee},
        {"+inf + 1", 0x7f800000, 0x3f800000, 0x7f800000},
        {"+inf + -inf", 0x7f800000, 0xff800000, 0x7fc00000},
        {"NaN + 1", 0x7fc01234, 0x3f800000, 0x7fc01234},
        {"1 + (-1)", 0x3f800000, 0xbf800000, 0x00000000},
        {"-0 + -0", 0x80000000, 0x80000000, 0x80000000},
        {"subnormal + sub", 0x00400000, 0x00800000, 0x00c00000},
        {"round to even", 0x3effffff, 0x3effffff, 0x3f7fffff},
    };

    for (const auto &t : tests)
    {
        uint32_t got = fp_add(t.a_bits, t.b_bits);
        std::cout << t.name << ":\n  result   = 0x" << std::hex << std::setw(8)
                  << std::setfill('0') << got << "\n  expected = 0x"
                  << std::setw(8) << t.expected << (got == t.expected ? " ✅\n\n" : " ❌\n\n");
    }
}
