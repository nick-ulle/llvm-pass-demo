//------------------------------------------------------------------------------
// An example of an LLVM FunctionPass for replacing all floating point types
// with FP128.
//
// Disclaimer: This incomplete code doesn't handle many cases, so it generally
// won't produce a valid LLVM module.
//
// Based on code by Eli Bendersky (eliben@gmail.com) at:
// https://github.com/eliben/llvm-clang-samples/
//------------------------------------------------------------------------------
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include <string>
#include <vector>

using namespace llvm;

class WidenFPTypes : public FunctionPass {
public:
  WidenFPTypes() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F) {
    // Setup
    // -----
    bool Modified = false;
    // Get FP128 type.
    Type *FP128Ty = Type::getFP128Ty(F.getContext());
    Type *FP128PtrTy = Type::getFP128PtrTy(F.getContext());

    // Rewrite Arguments
    // -----------------
    for (Function::arg_iterator A = F.arg_begin(), A_e = F.arg_end(); A != A_e;
        A++)
    {
      if (A->getType()->isFloatingPointTy()) {
        A->mutateType(FP128Ty);

        Modified = true;
      }
    }

    // Rewrite Return (FunctionType)
    // -----------------------------
    // See for instance:
    // https://groups.google.com/forum/#!topic/llvm-dev/d73BB2vppMA


    // Rewrite Basic Blocks
    // --------------------
    // Iterate over BasicBlocks
    for (Function::iterator BB = F.begin(), BB_e = F.end(); BB != BB_e; BB++) {

      // Iterate over Instructions
      for (BasicBlock::iterator II = BB->begin(), II_e = BB->end(); II != II_e;
          ) 
      {
        Instruction *Inst = &*II++;

        // Check for alloca.
        if (AllocaInst *Alloca = dyn_cast<AllocaInst>(Inst)) {
          // NOTE: The allocated type is not a pointer, wheres the type of the
          // instruction is.
          Type *AllocType = Alloca->getAllocatedType();
          if (AllocType->isFloatingPointTy()) {
            // Change type to FP128Ty.
            Alloca->setAllocatedType(FP128Ty);
            // Propagate type to all uses.
            Alloca->mutateType(FP128PtrTy);

            Modified = true;
          }

        // Check for load.
        } else if (LoadInst *Load = dyn_cast<LoadInst>(Inst)) {
          if (Load->getType()->isFloatingPointTy()) {
            // Propagate type to all uses.
            Load->mutateType(FP128Ty);

            Modified = true;
          }
        }

      } // for BI
    } // for I

    // Let the PassManager know whether a change was made.
    return Modified;
  }

  static char ID;
};


char WidenFPTypes::ID = 0;

int main(int argc, char **argv) {
  if (argc < 2) {
    errs() << "Usage: " << argv[0] << " <IR file>\n";
    return 1;
  }

  // Parse IR file.
  SMDiagnostic Err;
  LLVMContext Context;
  std::unique_ptr<Module> Mod(parseIRFile(argv[1], Err, Context));
  if (!Mod) {
    Err.print(argv[0], errs());
    return 1;
  }

  // 
  legacy::PassManager PM;
  PM.add(new WidenFPTypes());
  PM.run(*Mod);

  outs() << "Dumping the module:\n";
  Mod->dump();

  return 0;
}
