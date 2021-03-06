//===- Combine.td - Combine rule definitions ---------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declare GlobalISel combine rules and provide mechanisms to opt-out.
//
//===----------------------------------------------------------------------===//

// Common base class for GICombineRule and GICombineGroup.
class GICombine {
  // See GICombineGroup. We only declare it here to make the tablegen pass
  // simpler.
  list<GICombine> Rules = ?;
}

// A group of combine rules that can be added to a GICombiner or another group.
class GICombineGroup<list<GICombine> rules> : GICombine {
  // The rules contained in this group. The rules in a group are flattened into
  // a single list and sorted into whatever order is most efficient. However,
  // they will never be re-ordered such that behaviour differs from the
  // specified order. It is therefore possible to use the order of rules in this
  // list to describe priorities.
  let Rules = rules;
}

// Declares a combiner helper class
class GICombinerHelper<string classname, list<GICombine> rules>
    : GICombineGroup<rules> {
  // The class name to use in the generated output.
  string Classname = classname;
  // The name of a run-time compiler option that will be generated to disable
  // specific rules within this combiner.
  string DisableRuleOption = ?;
}
class GICombineRule<dag defs, dag match, dag apply> : GICombine {
  /// Defines the external interface of the match rule. This includes:
  /// * The names of the root nodes (requires at least one)
  /// See GIDefKind for details.
  dag Defs = defs;

  /// Defines the things which must be true for the pattern to match
  /// See GIMatchKind for details.
  dag Match = match;

  /// Defines the things which happen after the decision is made to apply a
  /// combine rule.
  /// See GIApplyKind for details.
  dag Apply = apply;
}

/// The operator at the root of a GICombineRule.Defs dag.
def defs;

/// All arguments of the defs operator must be subclasses of GIDefKind or
/// sub-dags whose operator is GIDefKindWithArgs.
class GIDefKind;
class GIDefKindWithArgs;
/// Declare a root node. There must be at least one of these in every combine
/// rule.
/// TODO: The plan is to elide `root` definitions and determine it from the DAG
///       itself with an overide for situations where the usual determination
///       is incorrect.
def root : GIDefKind;

/// Declares data that is passed from the match stage to the apply stage.
class GIDefMatchData<string type> : GIDefKind {
  /// A C++ type name indicating the storage type.
  string Type = type;
}

def extending_load_matchdata : GIDefMatchData<"PreferredTuple">;
def indexed_load_store_matchdata : GIDefMatchData<"IndexedLoadStoreMatchInfo">;

/// The operator at the root of a GICombineRule.Match dag.
def match;
/// All arguments of the match operator must be either:
/// * A subclass of GIMatchKind
/// * A subclass of GIMatchKindWithArgs
/// * A subclass of Instruction
/// * A MIR code block (deprecated)
/// The GIMatchKind and GIMatchKindWithArgs cases are described in more detail
/// in their definitions below.
/// For the Instruction case, these are collected into a DAG where operand names
/// that occur multiple times introduce edges.
class GIMatchKind;
class GIMatchKindWithArgs;

/// In lieu of having proper macro support. Trivial one-off opcode checks can be
/// performed with this.
def wip_match_opcode : GIMatchKindWithArgs;

/// The operator at the root of a GICombineRule.Apply dag.
def apply;
/// All arguments of the apply operator must be subclasses of GIApplyKind, or
/// sub-dags whose operator is GIApplyKindWithArgs, or an MIR block
/// (deprecated).
class GIApplyKind;
class GIApplyKindWithArgs;

def copy_prop : GICombineRule<
  (defs root:$d),
  (match (COPY $d, $s):$mi,
         [{ return Helper.matchCombineCopy(*${mi}); }]),
  (apply [{ Helper.applyCombineCopy(*${mi}); }])>;
def trivial_combines : GICombineGroup<[copy_prop]>;

def extending_loads : GICombineRule<
  (defs root:$root, extending_load_matchdata:$matchinfo),
  (match (wip_match_opcode G_LOAD, G_SEXTLOAD, G_ZEXTLOAD):$root,
         [{ return Helper.matchCombineExtendingLoads(*${root}, ${matchinfo}); }]),
  (apply [{ Helper.applyCombineExtendingLoads(*${root}, ${matchinfo}); }])>;
def combines_for_extload: GICombineGroup<[extending_loads]>;

def combine_indexed_load_store : GICombineRule<
  (defs root:$root, indexed_load_store_matchdata:$matchinfo),
  (match (wip_match_opcode G_LOAD, G_SEXTLOAD, G_ZEXTLOAD, G_STORE):$root,
         [{ return Helper.matchCombineIndexedLoadStore(*${root}, ${matchinfo}); }]),
  (apply [{ Helper.applyCombineIndexedLoadStore(*${root}, ${matchinfo}); }])>;

// FIXME: Is there a reason this wasn't in tryCombine? I've left it out of
//        all_combines because it wasn't there.
def elide_br_by_inverting_cond : GICombineRule<
  (defs root:$root),
  (match (wip_match_opcode G_BR):$root,
         [{ return Helper.matchElideBrByInvertingCond(*${root}); }]),
  (apply [{ Helper.applyElideBrByInvertingCond(*${root}); }])>;

def all_combines : GICombineGroup<[trivial_combines, combines_for_extload, combine_indexed_load_store]>;
