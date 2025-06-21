#include <iostream>
#include <iomanip>
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
        // Return −0 only if both are −0, else +0;
        // -0 is when exp and mantissa are 0, but sign is 1
        return (a_sign & b_sign) << 31;
    }
    if (a_zero)
        return b_bits;
    if (b_zero)
        return a_bits;

    // Add implicit leading 1 if normalized
    uint32_t a_frac = (a_exp == 0) ? a_man : ((1u << 23) | a_man);
    uint32_t b_frac = (b_exp == 0) ? b_man : ((1u << 23) | b_man);

    // Adjust exponent for subnormals (treat subnormals as if exp = 1)
    uint32_t real_a_exp = (a_exp == 0) ? 1 : a_exp;
    uint32_t real_b_exp = (b_exp == 0) ? 1 : b_exp;

    // Align exponents by shifting the smaller fraction
    if (real_a_exp > real_b_exp)
    {
        uint32_t shift = real_a_exp - real_b_exp;
        if (shift > 31)
            shift = 31;
        // Introduce sticky bit for rounding (preserve lost bits)
        uint32_t lost = shift ? b_frac & ((1u << shift) - 1u) : 0u;         // FIX
        b_frac = (shift == 31 ? 0u : (b_frac >> shift)) | (lost ? 1u : 0u); // FIX
        real_b_exp = real_a_exp;
    }
    else if (real_b_exp > real_a_exp)
    {
        uint32_t shift = real_b_exp - real_a_exp;
        if (shift > 31)
            shift = 31;
        uint32_t lost = shift ? a_frac & ((1u << shift) - 1u) : 0u;         // FIX
        a_frac = (shift == 31 ? 0u : (a_frac >> shift)) | (lost ? 1u : 0u); // FIX
        real_a_exp = real_b_exp;
    }

    uint32_t result_sign;
    uint32_t result_frac;

    // Same sign -> ADD
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
            return 0x00000000;
        } // Equal and diff signs -> 0
    }

    // Normalization
    int result_exp = static_cast<int>(real_a_exp);

    // Case: overflow from addition
    if (result_frac & (1u << 24))
    {
        result_frac >>= 1;
        ++result_exp;
        if (result_exp >= 255)
            return (result_sign << 31) | (0xFF << 23); // Infinity
    }

    // Case: under-normalized result (after subtraction or small addition)
    while ((result_frac & (1u << 23)) == 0 && result_exp > 0)
    {
        result_frac <<= 1;
        --result_exp;
    }

    // Optional: shift into subnormal if needed
    if (result_exp <= 0)
    {
        uint32_t shift = static_cast<uint32_t>(1 - result_exp); // how far subnormal
        if (shift > 31)
            shift = 31;
        uint32_t lost = shift ? result_frac & ((1u << shift) - 1u) : 0u;
        result_frac = (shift == 31 ? 0u : (result_frac >> shift)) | (lost ? 1u : 0u);
        result_exp = 0;
    }

    // Extract rounding bits
    uint32_t guard_bit = result_frac & 1u; // guard
    uint32_t round_bit = 0;                // unused (info folded into guard)
    uint32_t sticky_bit = guard_bit;       // lost bits already ORed

    // Shift result to fit 23-bit mantissa (drop lower bits)
    result_frac >>= 1; // move rounding window
    uint32_t mantissa = result_frac & 0x007FFFFF;

    // Round to nearest-even
    if (guard_bit && (mantissa & 1u))
    {
        ++mantissa;

        // Handle mantissa overflow (e.g., 0x007FFFFF + 1)
        if (mantissa >> 23)
        {
            mantissa >>= 1;
            ++result_exp;
            if (result_exp >= 255)
                return (result_sign << 31) | (0xFF << 23); // Infinity
        }
    }

    uint32_t result = (result_sign << 31) | (static_cast<uint32_t>(result_exp) << 23) | (mantissa & 0x007FFFFF);
    return result;
}

struct TestCase
{
    const char *name;
    uint32_t a_bits;
    uint32_t b_bits;
    uint32_t expected;
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
        {"round-to-even", 0x3effffff, 0x3effffff, 0x3f7fffff},
    };

    for (const auto &test : tests)
    {
        uint32_t result = fp_add(test.a_bits, test.b_bits);

        std::cout << test.name << ":\n";
        std::cout << "  result   = " << std::hex << std::setw(8) << std::setfill('0') << result << '\n';
        std::cout << "  expected = " << std::hex << std::setw(8) << std::setfill('0') << test.expected;

        if (result == test.expected)
            std::cout << " ✅ PASS\n";
        else
            std::cout << " ❌ FAIL\n";

        std::cout << '\n';
    }

    return 0;
}