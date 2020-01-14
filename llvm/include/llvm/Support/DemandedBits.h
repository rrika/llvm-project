//===- llvm/Support/DemandedBits.h - Stores known zeros/ones -------*- C++ -*-===//
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

#ifndef LLVM_SUPPORT_DEMANDEDBITS_H
#define LLVM_SUPPORT_DEMANDEDBITS_H

namespace llvm {

class APInt;
class KnownBits;

/// ...
APInt determineLiveOperandBitsAdd(
    unsigned OperandNo, const APInt &AOut,
    const KnownBits &LHS, const KnownBits &RHS);
/// ...
APInt determineLiveOperandBitsSub(
    unsigned OperandNo, const APInt &AOut,
    const KnownBits &LHS, const KnownBits &RHS);

} // end namespace llvm

#endif
