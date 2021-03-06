#include "chainerx/float16.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace chainerx {
namespace {

bool IsNan(const Half& half) {
    uint16_t exp = half.data() & 0x7c00;
    uint16_t frac = half.data() & 0x03ff;
    return exp == 0x7c00 && frac != 0x0000;
}

// Checks if `d` is equal to FromHalf(ToHalf(d)) with tolerance `tol`.
// This function cannot take NaN as a parameter.  The cast of NaN is tested in `Float16Nan`.
void CheckToHalfFromHalfNear(double d, double tol) {
    Half half{d};
    Half half_float{static_cast<float>(d)};
    EXPECT_EQ(half.data(), half_float.data());
    float f_result = static_cast<float>(half);
    double d_result = static_cast<double>(half);

    ASSERT_FALSE(std::isnan(d));
    EXPECT_FALSE(std::isnan(f_result));
    EXPECT_FALSE(std::isnan(d_result));
    EXPECT_FALSE(IsNan(half));

    if (std::isinf(d)) {
        // Signed inf
        EXPECT_EQ(d, f_result);
        EXPECT_EQ(d, d_result);
    } else {
        // Absolute error or relative error should be less or equal to tol.
        tol = std::max(tol, tol * std::abs(d));
        EXPECT_NEAR(d, f_result, tol);
        EXPECT_NEAR(d, d_result, tol);
    }
}

// Checks if `h` is equal to ToHalf(FromHalf(h)) exactly.
// This function cannot take NaN as a parameter.  The cast of NaN is tested in `Float16Nan`.
void CheckFromHalfToHalfEq(const Half& half) {
    float f = static_cast<float>(half);
    double d = static_cast<double>(half);
    EXPECT_EQ(d, static_cast<double>(f));

    ASSERT_FALSE(IsNan(half));
    EXPECT_FALSE(std::isnan(f));
    EXPECT_FALSE(std::isnan(d));

    EXPECT_EQ(half.data(), Half{f}.data());
    EXPECT_EQ(half.data(), Half{d}.data());
}

TEST(NativeFloat16Test, Float16Zero) {
    EXPECT_EQ(Half{float{0.0}}.data(), 0x0000);
    EXPECT_EQ(Half{float{-0.0}}.data(), 0x8000);
    EXPECT_EQ(Half{double{0.0}}.data(), 0x0000);
    EXPECT_EQ(Half{double{-0.0}}.data(), 0x8000);
    EXPECT_EQ(static_cast<float>(Half::FromData(0x0000)), 0.0);
    EXPECT_EQ(static_cast<float>(Half::FromData(0x8000)), -0.0);
    EXPECT_EQ(static_cast<double>(Half::FromData(0x0000)), 0.0);
    EXPECT_EQ(static_cast<double>(Half::FromData(0x8000)), -0.0);
    // Checks if the value is casted to 0.0 or -0.0
    EXPECT_EQ(1 / static_cast<float>(Half::FromData(0x0000)), std::numeric_limits<float>::infinity());
    EXPECT_EQ(1 / static_cast<float>(Half::FromData(0x8000)), -std::numeric_limits<float>::infinity());
    EXPECT_EQ(1 / static_cast<double>(Half::FromData(0x0000)), std::numeric_limits<float>::infinity());
    EXPECT_EQ(1 / static_cast<double>(Half::FromData(0x8000)), -std::numeric_limits<float>::infinity());
}

TEST(NativeFloat16Test, Float16Normalized) {
    for (double x = 1e-3; x < 1e3; x *= 1.01) {  // NOLINT(clang-analyzer-security.FloatLoopCounter)
        EXPECT_NE(Half{x}.data() & 0x7c00, 0);
        CheckToHalfFromHalfNear(x, 1e-3);
        CheckToHalfFromHalfNear(-x, 1e-3);
    }
    for (uint16_t bit = 0x0400; bit < 0x7c00; ++bit) {
        CheckFromHalfToHalfEq(Half::FromData(bit | 0x0000));
        CheckFromHalfToHalfEq(Half::FromData(bit | 0x8000));
    }
}

TEST(NativeFloat16Test, Float16Denormalized) {
    for (double x = 1e-7; x < 1e-5; x += 1e-7) {  // NOLINT(clang-analyzer-security.FloatLoopCounter)
        // Check if the underflow gap around zero is filled with denormal number.
        EXPECT_EQ(Half{x}.data() & 0x7c00, 0x0000);
        EXPECT_NE(Half{x}.data() & 0x03ff, 0x0000);
        CheckToHalfFromHalfNear(x, 1e-7);
        CheckToHalfFromHalfNear(-x, 1e-7);
    }
    for (uint16_t bit = 0x0000; bit < 0x0400; ++bit) {
        CheckFromHalfToHalfEq(Half::FromData(bit | 0x0000));
        CheckFromHalfToHalfEq(Half::FromData(bit | 0x8000));
    }
}

TEST(NativeFloat16Test, Float16Inf) {
    EXPECT_EQ(Half{std::numeric_limits<float>::infinity()}.data(), 0x7c00);
    EXPECT_EQ(Half{-std::numeric_limits<float>::infinity()}.data(), 0xfc00);
    EXPECT_EQ(Half{std::numeric_limits<double>::infinity()}.data(), 0x7c00);
    EXPECT_EQ(Half{-std::numeric_limits<double>::infinity()}.data(), 0xfc00);
    EXPECT_EQ(std::numeric_limits<float>::infinity(), static_cast<float>(Half::FromData(0x7c00)));
    EXPECT_EQ(-std::numeric_limits<float>::infinity(), static_cast<float>(Half::FromData(0xfc00)));
    EXPECT_EQ(std::numeric_limits<double>::infinity(), static_cast<double>(Half::FromData(0x7c00)));
    EXPECT_EQ(-std::numeric_limits<double>::infinity(), static_cast<double>(Half::FromData(0xfc00)));
}

TEST(NativeFloat16Test, Float16Nan) {
    for (uint16_t bit = 0x7c01; bit < 0x8000; ++bit) {
        EXPECT_TRUE(std::isnan(static_cast<float>(Half::FromData(bit | 0x0000))));
        EXPECT_TRUE(std::isnan(static_cast<float>(Half::FromData(bit | 0x8000))));
        EXPECT_TRUE(std::isnan(static_cast<double>(Half::FromData(bit | 0x0000))));
        EXPECT_TRUE(std::isnan(static_cast<double>(Half::FromData(bit | 0x8000))));
    }
    EXPECT_TRUE(IsNan(Half{float{NAN}}));
    EXPECT_TRUE(IsNan(Half{double{NAN}}));
}

// Get the partial set of all Half values for reduction of test execution time.
// The returned list includes the all values whose trailing 8 digits are `0b00000000` or `0b01010101`.
// This list includes all special values (e.g. signed zero, infinity) and some of normalized/denormalize numbers and NaN.
std::vector<Half> GetFloat16Values() {
    std::vector<Half> half_values;
    half_values.reserve(1 << 9);
    // Use uint32_t instead of uint16_t to avoid overflow
    for (uint32_t bit = 0x0000; bit <= 0xffff; bit += 0x0100) {
        half_values.emplace_back(Half::FromData(bit | 0x0000));
        half_values.emplace_back(Half::FromData(bit | 0x0055));
    }
    return half_values;
}

// Checks if `l` is equal to `r` or both of them are NaN.
void ExpectEqFloat16(const Half& l, const Half& r) {
    if (IsNan(l) && IsNan(r)) {
        return;
    }
    EXPECT_EQ(l.data(), r.data());
}

TEST(NativeFloat16Test, Float16Neg) {
    // Use uint32_t instead of uint16_t to avoid overflow
    for (uint32_t bit = 0x0000; bit <= 0xffff; ++bit) {
        Half x = Half::FromData(bit);
        Half expected{-static_cast<double>(x)};
        ExpectEqFloat16(expected, -x);
    }
}

TEST(NativeFloat16Test, Float16Add) {
    for (const Half& x : GetFloat16Values()) {
        for (const Half& y : GetFloat16Values()) {
            Half expected{static_cast<double>(x) + static_cast<double>(y)};
            ExpectEqFloat16(expected, x + y);
            ExpectEqFloat16(expected, y + x);
        }
    }
}

TEST(NativeFloat16Test, Float16Subtract) {
    for (const Half& x : GetFloat16Values()) {
        for (const Half& y : GetFloat16Values()) {
            Half expected{static_cast<double>(x) - static_cast<double>(y)};
            ExpectEqFloat16(expected, x - y);
        }
    }
}

TEST(NativeFloat16Test, Float16Multiply) {
    for (const Half& x : GetFloat16Values()) {
        for (const Half& y : GetFloat16Values()) {
            Half expected{static_cast<double>(x) * static_cast<double>(y)};
            ExpectEqFloat16(expected, x * y);
            ExpectEqFloat16(expected, y * x);
            EXPECT_EQ(expected.data(), (x * y).data());
        }
    }
}

TEST(NativeFloat16Test, Float16Divide) {
    for (const Half& x : GetFloat16Values()) {
        for (const Half& y : GetFloat16Values()) {
            Half expected{static_cast<double>(x) / static_cast<double>(y)};
            ExpectEqFloat16(expected, x / y);
        }
    }
}

TEST(NativeFloat16Test, Float16AddI) {
    for (const Half& x : GetFloat16Values()) {
        for (Half y : GetFloat16Values()) {
            Half expected{static_cast<double>(y) + static_cast<double>(x)};
            Half z = (y += x);
            ExpectEqFloat16(expected, y);
            ExpectEqFloat16(expected, z);
        }
    }
}

TEST(NativeFloat16Test, Float16SubtractI) {
    for (const Half& x : GetFloat16Values()) {
        for (Half y : GetFloat16Values()) {
            Half expected{static_cast<double>(y) - static_cast<double>(x)};
            Half z = y -= x;
            ExpectEqFloat16(expected, y);
            ExpectEqFloat16(expected, z);
        }
    }
}

TEST(NativeFloat16Test, Float16MultiplyI) {
    for (const Half& x : GetFloat16Values()) {
        for (Half y : GetFloat16Values()) {
            Half expected{static_cast<double>(y) * static_cast<double>(x)};
            Half z = y *= x;
            ExpectEqFloat16(expected, y);
            ExpectEqFloat16(expected, z);
        }
    }
}

TEST(NativeFloat16Test, Float16DivideI) {
    for (const Half& x : GetFloat16Values()) {
        for (Half y : GetFloat16Values()) {
            Half expected{static_cast<double>(y) / static_cast<double>(x)};
            Half z = y /= x;
            ExpectEqFloat16(expected, y);
            ExpectEqFloat16(expected, z);
        }
    }
}

}  // namespace
}  // namespace chainerx
