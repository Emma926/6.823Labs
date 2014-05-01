#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "pin.H"

// The array storing the spacing frequency between two dependant instructions
UINT64 *dependancySpacing;

// Output file name
UINT32 maxSize;
UINT32 regn=0; // inst num
UINT32 writereg[200];
UINT32 readreg[20];
UINT32 registers[200];
// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "result.csv", "specify the output file name");

// This knob will set the maximum spacing between two dependant instructions in the program
KNOB<string> KnobMaxSpacing(KNOB_MODE_WRITEONCE, "pintool", "s", "100", "specify the maximum spacing between two dependant instructions in the program");


// This function is called before every instruction is executed. Have to change
// the code to send in the register names from the Instrumentation function
VOID updateSpacingInfoW(UINT32 r){
        writereg[r]=regn;
}

VOID updateSpacingInfoR(UINT32 reg){
        UINT32 r;
	r=writereg[reg];
        if(r>0 && regn-r-1 < maxSize && r<regn){ 
		dependancySpacing[regn-r-1]++; 
	//	if(regn-r-1<30) registers[reg]++;
	}
}

VOID docount() {regn++; }


// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    // Insert a call to updateSpacingInfo before every instruction.
    // You may need to add arguments to the call.
    	INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);
	UINT32 regnum = INS_MaxNumWRegs(ins);
	UINT32 reg,i,j,flag;
	REG tmp;
	regn++;
	for(i=0; i<regnum; i++){
		tmp=INS_RegW(ins, i);
		if(REG_is_partialreg(tmp)){
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoW,IARG_UINT32, (UINT32)REG_FullRegName(tmp), IARG_END);
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoW,IARG_UINT32, (UINT32)tmp, IARG_END);	

	}
//	if((UINT32)tmp==REG_EAX) printf("eax %u\n",tmp);
//	if((UINT32)tmp==REG_RBASE) printf("base %u\n",tmp);
//	if((UINT32)tmp== REG_ESI) printf("esi %u\n",tmp);
	
		if((UINT32)tmp==REG_AL || (UINT32)tmp==REG_AH) INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoW,IARG_UINT32, (UINT32)REG_AX, IARG_END);
		else if((UINT32)tmp==REG_BL || (UINT32)tmp==REG_BH) INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoW,IARG_UINT32, (UINT32)REG_BX, IARG_END);
		else if((UINT32)tmp==REG_CL || (UINT32)tmp==REG_CH) INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoW,IARG_UINT32, (UINT32)REG_CX, IARG_END);
		else if((UINT32)tmp==REG_DL || (UINT32)tmp==REG_CH) INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoW,IARG_UINT32, (UINT32)REG_DX, IARG_END);
		
	}
	regnum = INS_MaxNumRRegs(ins);
	for(i=0; i<regnum; i++){ 
                reg=(UINT32)INS_RegR(ins, i);
		//if(reg>200 || reg<0) printf("Error2: reg=%u\n",reg);
        	//reg=writereg[reg];
		//if(reg>regn) printf("Error3\n");
		//if(reg>0 && regn-reg < maxSize && reg<regn) dependancySpacing[regn-reg]++;
		flag=0;
                for(j=0;j<i;j++)
                        if(readreg[j]==reg){flag=1;break;}
                if(flag==0)
		{ 
			INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)updateSpacingInfoR, IARG_UINT32, reg, IARG_END);
			readreg[i]=reg;
		
		}
		else{	
			readreg[i]=-1;
		}
	}

}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{	
    FILE* outfile;

/*	UINT32 i,max,maxi,j;
	for(j=0;j<5;j++){
		max=registers[0];
		maxi=0;
		for(i=1;i<200;i++){
			if(registers[i]>max){ 
				max=registers[i];
				maxi=i;
			}
		}
		printf("!!!! %u\n", maxi);
		registers[maxi]=0;
	}	
*/	
    assert(outfile = fopen(KnobOutputFile.Value().c_str(),"w"));
    for(UINT32 i = 0; i < maxSize; i++)
        fprintf(outfile, "%lu,", dependancySpacing[i]);
}


// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{

    // Initialize pin
    PIN_Init(argc, argv);
for(UINT32 i=0;i<200;i++)
{	writereg[i]=0;
	registers[i]=0;
}   
maxSize = atoi(KnobMaxSpacing.Value().c_str());
	
    // Initializing depdendancy Spacing
    dependancySpacing = new UINT64[maxSize];

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

