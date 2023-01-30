#include <stdio.h>
//#include <chrono>
#include "pin.H"
#include <string>
#include <exception>

FILE * ftrace;
long long icount=0;
long long ilimit=1000000;
BOOL print_args=false;


string invalid = "invalid_rtn";
const string *Target2String(ADDRINT target)
{
    string name = RTN_FindNameByAddress(target);
    if (name == "")
        return &invalid;
    else
        return new string(name);
}

VOID  do_call_args(const string *s, ADDRINT arg0)
{
    fprintf(ftrace, "\n[%s]\n", (*s).c_str()); //(INS_Disassemble(ins)).c_str()
    fflush(ftrace);
}

/* ===================================================================== */

VOID  do_call_args_indirect(ADDRINT target, BOOL taken, ADDRINT arg0)
{
    if (!taken) return;

    const string *s = Target2String(target);
    do_call_args(s, arg0);

    if (s != &invalid)
        delete s;
}

/* ===================================================================== */

VOID  do_call(const string *s)
{
    fprintf(ftrace, "\n[%s]\n", (*s).c_str()); //(INS_Disassemble(ins)).c_str()
    fflush(ftrace);
}

VOID  do_ret(const string *s)
{
    fprintf(ftrace, "\nDirect RETURN to[%s]\n", (*s).c_str()); //(INS_Disassemble(ins)).c_str()
    fflush(ftrace);
}

/* ===================================================================== */

VOID  do_call_indirect(ADDRINT target, BOOL taken)
{
    if (!taken) return;

    const string *s = Target2String(target);
    do_call(s);

    if (s != &invalid)
        delete s;
}

VOID  do_ret_indirect(ADDRINT target, BOOL taken)
{
    if (!taken) return;

    const string *s = Target2String(target);
    fprintf(ftrace, "\nInDirect RETURN to[%s]\n", (*s).c_str()); //(INS_Disassemble(ins)).c_str()
    fflush(ftrace);

    if (s != &invalid)
        delete s;
}


VOID Instruction(INS ins, VOID *v)
{

    //start = std::chrono::high_resolution_clock::now();
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    fprintf(ftrace, "\n[%s]\n", (INS_Disassemble(ins)).c_str()); //(INS_Disassemble(ins)).c_str()
    fflush(ftrace);
    if (INS_IsCall(ins))
    {
        if (INS_IsDirectBranchOrCall(ins))
        {
            const ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);


            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(do_call_args),
                                     IARG_PTR, Target2String(target), IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_END);

        }
        else
        {
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(do_call_indirect),
                           IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
        }
    }
    else
    {
        if (INS_IsRet(ins))
        {
            if (INS_IsDirectBranchOrCall(ins))
            {
                const ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);


                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(do_ret),
                               IARG_PTR, Target2String(target), IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_END);

            }
            else
            {
                INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(do_ret_indirect),
                               IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
            }
        }
        else
        {
            // sometimes code is not in an image
            RTN rtn = INS_Rtn(ins);

            // also track stup jumps into share libraries
            if (RTN_Valid(rtn) && !INS_IsDirectBranchOrCall(ins) && ".plt" == SEC_Name(RTN_Sec(rtn)))
            {
                if (print_args)
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(do_call_args_indirect),
                                   IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_FUNCARG_CALLSITE_VALUE, 0, IARG_END);
                }
                else
                {
                    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(do_call_indirect),
                                   IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);

                }
            }
        }
    }



    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {

        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            fprintf(ftrace, "R");
            icount++;

        }

        if (INS_MemoryOperandIsWritten(ins, memOp))
        {

            fprintf(ftrace, "W");
            icount++;

        }
    }

}
VOID Fini(INT32 code, VOID *v)
{    fprintf(ftrace, "\n");
    fclose(ftrace);
}
INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n"
               + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
                            "o", "pinatrace2.out", "specify output file name");

KNOB<long long> KnobInsLimit(KNOB_MODE_WRITEONCE, "pintool",
                             "i", "1000", "specify instruction limit");

KNOB<BOOL>   KnobPrintArgs(KNOB_MODE_WRITEONCE, "pintool", "a", "0", "print call arguments ");

int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    if (PIN_Init(argc, argv)) return Usage();
    ilimit=KnobInsLimit.Value();
    ftrace = fopen(KnobOutputFile.Value().c_str(), "w");
    print_args = KnobPrintArgs.Value();
    INS_AddInstrumentFunction(Instruction, 0);
    //TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_StartProgram();

    return 0;
}