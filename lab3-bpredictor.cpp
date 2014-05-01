#include <iostream>
#include <stdio.h>
#include <assert.h>
#include "pin.H"

static UINT64 takenCorrect = 0;
static UINT64 takenIncorrect = 0;
static UINT64 notTakenCorrect = 0;
static UINT64 notTakenIncorrect = 0;

class BranchPredictor
{
    public:
        BranchPredictor() { }

        virtual bool makePrediction(ADDRINT address) { return false; };

        virtual void makeUpdate(bool takenActually, bool takenPredicted, ADDRINT address) {};

};

class myBranchPredictor: public BranchPredictor
{
    public:
	bool localhis[1024][10];
	bool localpred[1024][3];
	
	bool globalhis[12];
	bool choicepred[4096][2];
	bool globalpred[4096][2];
	
	bool predictor; // 0-local 1-global
	int globalindex;
	
	int bits2int(bool *bits, int l){ // 0 - lsb
		int i;
		int base=1,res=0;
		for(i=0;i<l;i++){
			res+=base*bits[i];
			base*=2;
		}
		return res;
	}

	void bitschange1(bool *bits, bool change, int l){
		int t;
		t=bits2int(bits, l);
		
		if(l==2){
			if(change==1) t++;
			else t--;
			
			switch (t){
				case -1:
				case 0:
					bits[0]=0;
					bits[1]=0;
					break;
				case 1:
					bits[0]=1;
                                        bits[1]=0;
					break;
				case 2:
					bits[0]=0;
                                        bits[1]=1;
                                        break;
				case 3:
				case 4:
					bits[0]=1;
                                        bits[1]=1;
                                        break;
				default:
					printf("ERROR5: t=%d\n",t);
			}		
		}
		
		else if(l==3){
                        if(change==1) t++;
                        else t--;

                        switch (t){
                                case -1:
                                case 0:
                                        bits[0]=0;
                                        bits[1]=0;
					bits[2]=0;
                                        break;
                                case 1:
                                        bits[0]=1;
                                        bits[1]=0;
					bits[2]=0;
                                        break;
                                case 2:
                                        bits[0]=0;
                                        bits[1]=1;
					bits[2]=0;
                                        break;
                                case 3:
					bits[0]=1;
                                        bits[1]=1;
                                        bits[2]=0;
                                        break;
                                case 4:
					bits[0]=0;
                                        bits[1]=0;
                                        bits[2]=1;
                                        break;
				case 5:
					bits[0]=1;
                                        bits[1]=0;
                                        bits[2]=1;
                                        break;
				case 6:
					bits[0]=0;
                                        bits[1]=1;
                                        bits[2]=1;
                                        break;
				case 7:
				case 8:
					bits[0]=1;

                                        bits[1]=1;
                                        bits[2]=1;
                                        break;
                                        
                                default:
                                        printf("ERROR6: t=%d\n",t);
                        }
                }
			
							
	}

	void bitshift1(bool *bits, bool a, int l){
		int i;
		for(i=1;i<l;i++){
			bits[i-1]=bits[i];	
		}	
		bits[l-1]=a;
		
	}
	
        myBranchPredictor() {
		int i=0,j=0;
		for(i=0;i<1024;i++){
			for(j=0;j<10;j++)
				localhis[i][j]=0;
			
				localpred[i][0]=1;
				localpred[i][1]=1;
				localpred[i][2]=0;
			
				choicepred[i][0]=1;
				choicepred[i][1]=0;
				globalpred[i][0]=1;
				globalpred[i][1]=0;
			
		}
		for(;i<4096;i++){
			choicepred[i][0]=1;
                        choicepred[i][1]=0;
                        globalpred[i][0]=1;
                        globalpred[i][1]=0;
                       
		}

		for(i=0;i<12;i++)
			globalhis[i]=0;
	}

        bool makePrediction(ADDRINT address){ 
			
		int index=bits2int(globalhis, 12);
		globalindex=index;
		bool counter = choicepred[index][1];
		if (counter){ // choose local
			long addr=(long) address;
			index=bits2int(localhis[addr%1024],10);
			counter=localpred[index][2];
			predictor=0;
		}
		else{ // choose global
			counter=globalpred[globalindex][1];
			predictor=1;
		}

		return counter; 
	}

        void makeUpdate(bool takenActually, bool takenPredicted, ADDRINT address){
		long addr=(long) address;
                addr = addr%1024;	
	if(predictor==0){ //local
        	int index;

		index = bits2int(localhis[addr],10);

		bitschange1(localpred[index], takenActually, 3);
		if(takenActually==takenPredicted) bitschange1(choicepred[globalindex], 1, 2);
		else bitschange1(choicepred[globalindex], 0, 2);	
	}
	else{ // global
		bitschange1(globalpred[globalindex],takenActually,2);

		if(takenActually==takenPredicted) bitschange1(choicepred[globalindex], 0, 2);
		else bitschange1(choicepred[globalindex], 1, 2);
	}
	bitshift1(globalhis, takenActually, 12);

        bitshift1(localhis[addr], takenActually, 10);

}

};

BranchPredictor* BP;


// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "result.out", "specify the output file name");


// In examining handle branch, refer to quesiton 1 on the homework
void handleBranch(ADDRINT ip, bool direction)
{
    bool prediction = BP->makePrediction(ip);
    BP->makeUpdate(direction, prediction, ip);
    if(prediction)
    {
        if(direction)
        {
            takenCorrect++;
        }
        else
        {
            takenIncorrect++;
        }
    }
    else
    {
        if(direction)
        {
            notTakenIncorrect++;
        }
        else
        {
            notTakenCorrect++;
        }
    }
}


void instrumentBranch(INS ins, void * v)
{   
    if(INS_IsBranch(ins) && INS_HasFallThrough(ins))
    {
        INS_InsertCall(
                ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranch,
                IARG_INST_PTR,
                IARG_BOOL,
                TRUE,
                IARG_END); 

        INS_InsertCall(
                ins, IPOINT_AFTER, (AFUNPTR)handleBranch,
                IARG_INST_PTR,
                IARG_BOOL,
                FALSE,
                IARG_END);
    }
}


/* ===================================================================== */
VOID Fini(int, VOID * v)
{   
    FILE* outfile;
    assert(outfile = fopen(KnobOutputFile.Value().c_str(),"w"));   
    fprintf(outfile, "takenCorrect %lu  takenIncorrect %lu notTakenCorrect %lu notTakenIncorrect %lu\n", takenCorrect, takenIncorrect, notTakenCorrect, notTakenIncorrect);
}


// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    // Make a new branch predictor
    BP = new myBranchPredictor();

    // Initialize pin
    PIN_Init(argc, argv);

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(instrumentBranch, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

