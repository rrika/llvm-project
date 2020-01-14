//===-- KnownBits.cpp - Stores known zeros/ones ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ...
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/DemandedBits.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/KnownBits.h"

using namespace llvm;

static APInt determineLiveOperandBitsAddSub(
		unsigned OperandNo, const APInt &AOut,
    const APInt &BoundLHS, const APInt &BoundRHS)
{
  APInt Bound = BoundLHS & BoundRHS;
  APInt RBound = Bound.reverseBits();
  APInt RAOut = AOut.reverseBits();
  APInt RProp = RAOut + (RAOut | ~RBound);
  APInt RQ = (RProp ^ ~(RAOut | RBound));
  APInt Q = RQ.reverseBits();
  APInt U;
  if (OperandNo == 0)
    U = BoundLHS | ~BoundRHS;
  else
    U = BoundRHS | ~BoundLHS;
  APInt AB = AOut | (Q & (U | (BoundLHS + BoundRHS + 1)));
  return AB;
}

APInt determineLiveOperandBitsAdd(
		unsigned OperandNo, const APInt &AOut,
    const KnownBits &KnownLHS, const KnownBits &KnownRHS)
{
	return determineLiveOperandBitsAddSub(OperandNo, AOut, KnownLHS.Zero, KnownRHS.Zero);
}

APInt determineLiveOperandBitsSub(
		unsigned OperandNo, const APInt &AOut,
    const KnownBits &KnownLHS, const KnownBits &KnownRHS)
{
	return determineLiveOperandBitsAddSub(OperandNo, AOut, KnownLHS.One, KnownRHS.Zero);
}
