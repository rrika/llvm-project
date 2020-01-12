//===- DemandedBitsTest.cpp - DemandedBits tests --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/DemandedBits.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
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

template <typename Fn>
static void PropagateBinOp(const KnownBits &Known1, const KnownBits &Known2,
                           const APInt &AOut, APInt &AB1, APInt &AB2,
                           Fn BuildOp) {
  unsigned Bits = AOut.getBitWidth();

  LLVMContext C;
  Module M("test", C);
  Type *NumericTy = Type::getIntNTy(C, Bits);

  Type *ArgTypes[] = {NumericTy, NumericTy};
  Function *F = Function::Create(
      FunctionType::get(NumericTy, ArrayRef<Type *>(ArgTypes, 2), false),
      GlobalValue::ExternalLinkage, "F", &M);
  BasicBlock *BB = BasicBlock::Create(C, "", F);
  IRBuilder<> Builder(BB);

  Value *Known1NZero = Builder.getInt(~Known1.Zero);
  Value *Known1One = Builder.getInt(Known1.One);
  Value *Known2NZero = Builder.getInt(~Known2.Zero);
  Value *Known2One = Builder.getInt(Known2.One);
  Value *AOutImm = Builder.getInt(AOut);

  Argument *Arg1 = F->getArg(0);
  Argument *Arg2 = F->getArg(1);
  Value *Dummy1 = Builder.CreateNot(Arg1);
  Value *Dummy2 = Builder.CreateNot(Arg2);
  Value *Op1 =
      Builder.CreateOr(Builder.CreateAnd(Dummy1, Known1NZero), Known1One);
  Value *Op2 =
      Builder.CreateOr(Builder.CreateAnd(Dummy2, Known2NZero), Known2One);

  Value *IOp = BuildOp(Builder, Op1, Op2);

  Value *IMasked = Builder.CreateAnd(IOp, AOutImm);
  Builder.CreateRet(IMasked);

  AssumptionCache AC(*F);
  DominatorTree DT(*F);
  DemandedBits DB(*F, AC, DT);
  AB1 = DB.getDemandedBits(dyn_cast<Instruction>(Op1));
  AB2 = DB.getDemandedBits(dyn_cast<Instruction>(Op2));
}

template <typename Fn1, typename Fn2>
static void TestBinOpExhaustive(Fn1 BuildOp, Fn2 EvalFn) {
  unsigned Bits = 4;
  unsigned Max = 1 << Bits;
  EnumerateTwoKnownBits(
      Bits, [&](const KnownBits &Known1, const KnownBits &Known2) {
        for (unsigned AOut_ = 0; AOut_ < Max; AOut_++) {
          APInt AOut(Bits, AOut_);
          APInt AB1;
          APInt AB2;
          PropagateBinOp(Known1, Known2, AOut, AB1, AB2, BuildOp);
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

            APInt AB1_;
            APInt AB2_;
            PropagateBinOp(Known1_, Known2_, AOut, AB1_, AB2_, BuildOp);
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
      [](IRBuilder<> &Builder, Value *Op1, Value *Op2) -> Value * {
        return Builder.CreateAdd(Op1, Op2);
      },
      [](APInt N1, APInt N2) -> APInt { return N1 + N2; });
}

TEST(DemandedBitsTest, Sub) {
  TestBinOpExhaustive(
      [](IRBuilder<> &Builder, Value *Op1, Value *Op2) -> Value * {
        return Builder.CreateSub(Op1, Op2);
      },
      [](APInt N1, APInt N2) -> APInt { return N1 - N2; });
}

} // anonymous namespace
