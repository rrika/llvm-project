//===- DemandedBitsTest.cpp - DemandedBits tests --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Support/DemandedBits.h"
#include "llvm/Support/KnownBits.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

template <typename Fn>
static void EnumerateKnownBits(unsigned Bits, Fn TestFn) {
  unsigned Max = 1 << Bits;
  for (unsigned Zero = 0; Zero < Max; Zero++) {
    for (unsigned One = 0; One < Max; One = ((One | Zero) + 1) & ~Zero) {
      KnownBits Known;
      Known.Zero = APInt(Bits, Zero);
      Known.One = APInt(Bits, One);
      TestFn(Known);
    }
  }
}

template <typename Fn>
static void EnumerateTwoKnownBits(unsigned Bits, Fn TestFn) {
  EnumerateKnownBits(Bits, [&](const KnownBits &Known1) {
    EnumerateKnownBits(
        Bits, [&](const KnownBits &Known2) { TestFn(Known1, Known2); });
  });
}

template <typename Fn>
static void EnumerateRemainingBits(const KnownBits &Known, Fn TestFn) {
  unsigned Max = 1 << Known.getBitWidth();
  unsigned Mask = (Known.Zero | Known.One).getLimitedValue();
  for (unsigned Remaining = 0; Remaining < Max;
       Remaining = ((Remaining | Mask) + 1) & ~Mask) {
    TestFn(Known.One | Remaining);
  }
}

template <typename Fn1, typename Fn2>
static void TestBinOpExhaustive(Fn1 PropagateFn, Fn2 EvalFn) {
  unsigned Bits = 4;
  unsigned Max = 1 << Bits;
  EnumerateTwoKnownBits(
      Bits, [&](const KnownBits &Known1, const KnownBits &Known2) {
        for (unsigned AOut_ = 0; AOut_ < Max; AOut_++) {
          APInt AOut(Bits, AOut_);
          APInt AB1 = PropagateFn(0, AOut, Known1, Known2);
          APInt AB2 = PropagateFn(1, AOut, Known1, Known2);
          {
            // If the propagator claims that certain known bits
            // didn't matter, check that the result doesn't
            // change when they become unknown
            KnownBits Known1_;
            KnownBits Known2_;
            Known1_.Zero = Known1.Zero & AB1;
            Known1_.One = Known1.One & AB1;
            Known2_.Zero = Known2.Zero & AB2;
            Known2_.One = Known2.One & AB2;

            APInt AB1_ = PropagateFn(0, AOut, Known1_, Known2_);
            APInt AB2_ = PropagateFn(1, AOut, Known1_, Known2_);
            EXPECT_EQ(AB1, AB1_);
            EXPECT_EQ(AB1, AB1_);
          }
          APInt Z = EvalFn(Known1.One, Known2.One);
          EnumerateRemainingBits(Known1, [&](APInt Value1) {
            EnumerateRemainingBits(Known2, [&](APInt Value2) {
              APInt ReferenceResult = EvalFn((Value1 & AB1), (Value2 & AB2));
              APInt Result = EvalFn(Value1, Value2);
              EXPECT_EQ(Result & AOut, ReferenceResult & AOut);
            });
          });
        }
      });
}

TEST(DemandedBitsTest, Add) {
  TestBinOpExhaustive(
      determineLiveOperandBitsAdd,
      [](APInt N1, APInt N2) -> APInt { return N1 + N2; });
}

TEST(DemandedBitsTest, Sub) {
  TestBinOpExhaustive(
      determineLiveOperandBitsSub,
      [](APInt N1, APInt N2) -> APInt { return N1 - N2; });
}

} // anonymous namespace
