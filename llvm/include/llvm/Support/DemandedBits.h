//===- llvm/Support/DemandedBits.h - Alive bit propagators ------*- C++ -*-===//
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

/// Compute alive bits of one addition operand from alive output and known
/// operand bits
APInt determineLiveOperandBitsAdd(
    unsigned OperandNo, const APInt &AOut,
    const KnownBits &LHS, const KnownBits &RHS);

/// Compute alive bits of one subtraction operand from alive output and known
/// operand bits
APInt determineLiveOperandBitsSub(
    unsigned OperandNo, const APInt &AOut,
    const KnownBits &LHS, const KnownBits &RHS);

} // end namespace llvm

#endif
