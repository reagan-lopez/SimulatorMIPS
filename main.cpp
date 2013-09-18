/* 
	Copyright © 2013 Reagan Lopez
	[This program is licensed under the "MIT License"]
	Please see the file LICENSE in the source
	distribution of this software for license terms	
*/

/**************************************************************************************************/
/*
Program to calculate stalls in a MIPS instruction set.
mainV1 (05/26/2013): Draft version of no forwarding.
mainV2 (05/27/2013): Included condition to not add stalls if producer 1 has caused a stall.
					 Included condition to not compare registers of branch instructions.
mainV3 (05/27/2013): Changed the algorithm to handle different types of instructions.
					 Added the code to write to output file.
mainV4 (06/05/2013): Included logic for branch prediction and forwarding.
mainV5 (06/06/2013): Included logic for calculating stalls per hazard type.
mainV6 (06/06/2013): Included logic for calculating effected instructions.
*/
/**************************************************************************************************/
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <time.h>
#include <iomanip>
#include <sstream>


using namespace std;
const int MAX_STALL = 2;                      //max stall that a producer can cause.
const int MAX_LEVEL = 2;                      //max number of levels to be checked for stall producers
const int BRANCH_PROBABILITY = 9;             //branch probability
const int op = 0;                             //index of opcode.
const int r1 = 1;                             //index of first register.
const int r2 = 2;                             //index of second register or immediate number.
const int r3 = 3;                             //index of third register or displacement.
const string ARITHMETIC = "ARITHMETIC";
const string LOGICAL = "LOGICAL";
const string MEMORY = "MEMORY";
const string BRANCH = "BRANCH";

const int RAW = 0;
const int BRCH = 1;
const int MISBRCH = 2;
const int RCNT = 3;
const int BCNT = 4;
const int BMCNT = 5;
struct MIPS
{
   public:
       string inst[4]; //inst[0] is op, inst[1] is rd, inst[2] is rs, inst[3] is rt.
       string optype;
       int stall;
       bool misBranch;
};


/***********************************************************************************/
/*
Function to read input file and generate the MIPS struct.
*/
/***********************************************************************************/
void readInput(MIPS *mips, int &cnt)
{
   string input;
   int j, prev, cur;
   cnt = 0;

   ifstream inputfile ("input.txt");   //read input file

   if (inputfile.is_open())
       {
           while ( inputfile.good() )
           {
               j = 0; prev = 0; cur = 0;
               getline (inputfile,input);  //read instruction.
               const char *p = input.c_str();
               while(*p != '\0')
               {
                   if (*p == ' ')
                   {

                       mips[cnt].inst[j].append(input.substr(prev,cur-prev));
                       prev = cur + 1;
                       j++;
                   }
                   cur++;
                   *p++;
               }
               mips[cnt].inst[j].append(input.substr(prev,cur-prev));
               mips[cnt].stall = 0;             //initialize stalls to 0.
               mips[cnt].misBranch = false;     //initialize misBranchs to false.
               //determine if branch instruction.
               if ((mips[cnt].inst[0].compare(0,2,"LD") == 0) || (mips[cnt].inst[0].compare(0,2,"ST") == 0))
                   mips[cnt].optype = MEMORY;
               else if ((mips[cnt].inst[0].compare(0,3,"ADD") == 0) || (mips[cnt].inst[0].compare(0,1,"SUB") == 0))
                   mips[cnt].optype = ARITHMETIC;
               else if (mips[cnt].inst[0].compare(0,1,"B") == 0)
                   mips[cnt].optype = BRANCH;
               else
                   mips[cnt].optype = LOGICAL;

               cnt++;
           }
               inputfile.close();
               cnt--;
       }
   else cout << "Unable to open file";
   exit;

}


/***********************************************************************************/
/*
Function to calculate stalls with the following options:
1. No forwarding and no branch predictor.
2. With forwarding and no branch predictor.
3. With forwarding and branch predictor.
*/
/***********************************************************************************/
void calculateStall(MIPS *mips, int &cnt, int *counts, bool withForwarding, bool withBranchPrediction)
{
   srand (time(NULL));
   int branch, level, comp1, comp2;
   string opcode;

   //initialize all counts
   for (int i = 0; i < 6; i++)
    counts[i] = 0;


   //calculate stalls for rest of the instructions.
   for (int i = 1;i < cnt; i++)
   {

        level = 1;
        comp1 = 0;
        comp2 = 0;
        opcode = mips[i].optype;

        //This section of code is to determine the registers that can be read based on
        //the instruction and instruction type of the current instruction.
        if (opcode == BRANCH)
        {
            if (mips[i].inst[op] == "BZ")
            {
                //compare only rd
                comp1 = r1;
                comp2 = r1;
            }
            else
            {
                comp1 = r1;
                comp2 = r2;
            }
        }
        if (opcode == ARITHMETIC)
        {
            if ((mips[i].inst[op] == "ADDI") || (mips[i].inst[op] == "SUBI"))
            {
                //compare only rs
                comp1 = r2;
                comp2 = r2;
            }
            else
            {
                comp1 = r2;
                comp2 = r3;
            }
        }

        if (opcode == LOGICAL)
        {
            if ((mips[i].inst[op] == "ANDI") || (mips[i].inst[op] == "ORI"))
            {
                //compare only rs
                comp1 = r2;
                comp2 = r2;
            }
            else
            {
                comp1 = r2;
                comp2 = r3;
            }
        }
        if (opcode == MEMORY)
        {
            if ((mips[i].inst[op] == "LD"))
            {
                //compare only rs
                comp1 = r2;
                comp2 = r2;
            }
            else
            {
                comp1 = r1;
                comp2 = r2;
            }
        }

            if (!withBranchPrediction)
            {
               if (mips[i-1].optype == BRANCH)
               {
                   mips[i].stall = MAX_STALL;
                   counts[BRCH] += MAX_STALL;
                   counts[BCNT]++;
               }
            }

            if (withBranchPrediction)
            {
               //if current instruction is a branch instruction then determine branch prediction
               //based on random number generator.
               if ((mips[i].optype == BRANCH) && ((rand() % 10 + 1) > BRANCH_PROBABILITY))
               {
                   mips[i].misBranch = true;
               }

               //if first level instruction was a branch instruction with miss branch prediction,
               //then stall.
               if (mips[i-1].misBranch == true)
               {
                   mips[i].stall = MAX_STALL;
                   counts[MISBRCH] += MAX_STALL;
                   counts[BMCNT]++;
               }

            }

        if (!withForwarding)
        {
            //This section of code is to compare the read registers of the current instruction with the
            //first and second level of instructions one by one.
            do {
                //These are the candidates that can cause a write.
                if ((mips[i-level].optype == ARITHMETIC) || (mips[i-level].optype == LOGICAL) || (mips[i-level].inst[op]== "LD"))
                    //rd of the potential producers is compared with the candidate read registers of the current instruction
                    if ((mips[i-level].inst[r1] == mips[i].inst[comp1]) || (mips[i-level].inst[r1] == mips[i].inst[comp2]))
                        //Number of cycles to be stalled is determined by the level of the producing instruction,
                        //the stalls of the current instruction and the stall of the prev level.
                        {
                            if (((level == 1) || ((level == 2) && (mips[i-level+1].stall == 0))) && (mips[i].stall == 0))
                            {
                                mips[i].stall = MAX_STALL - level + 1;
                                counts[RAW] += MAX_STALL - level + 1;
                                counts[RCNT]++;
                            }
                        }

                level++;
            }while((i != 1) && (level <=MAX_LEVEL)); //Instruction nbr 2 must do it only once. Rest must do it twice.
        }

        if (withForwarding)
        {
            //This section of code is to compare the read registers of the current instruction with the
            //first and second level of instructions one by one.
            //These are the candidates that can cause a write.
            if (mips[i-level].inst[op]== "LD")
                //rd of the potential producers is compared with the candidate read registers of the current instruction
                if ((mips[i-level].inst[r1] == mips[i].inst[comp1]) || (mips[i-level].inst[r1] == mips[i].inst[comp2]))
                    //Number of cycles to be stalled is determined by the level of the producing instruction,
                    //the stalls of the current instruction and the stall of the prev level.
                    {
                        if (mips[i].stall == 0)
                        {
                            mips[i].stall = MAX_STALL - level;
                            counts[RAW] += MAX_STALL - level;
                            counts[RCNT]++;
                        }

                    }
        }
   }
}

/***********************************************************************************/
/*
Function to generate output in a file.
*/
/***********************************************************************************/
void writeOutput(MIPS *mips, int &cnt, int *counts, string outfile)
{
   ofstream outputfile;
   stringstream ss;
   ss << cnt;
   outfile = outfile + "_" + ss.str() + ".txt";
   int total_stall = 0, total_misBranch = 0, total_stall_a = 0, total_stall_b = 0, total_stall_l = 0, total_stall_m = 0;
   outputfile.open (outfile.c_str());
   outputfile<<setw(6)<<left<<"Op"<<setw(10)<<left<<"Rd/Rs"<<setw(10)<<left<<"Rs/imm"<<setw(10)<<left<<"Rt/imm"<<setw(12)<<left<<"Op Type"<<setw(6)<<left<<"Stall"<<setw(10)<<left<<"Branch Miss"<<"\n";
   outputfile<<setw(6)<<left<<"--"<<setw(10)<<left<<"-----"<<setw(10)<<left<<"------"<<setw(10)<<left<<"------"<<setw(12)<<left<<"-------"<<setw(6)<<left<<"-----"<<setw(10)<<left<<"-----------"<<"\n";
   for(int i = 0; i < cnt; i++)
   {
       outputfile<<setw(6)<<left<<mips[i].inst[0]<<setw(10)<<left<<mips[i].inst[1]<<setw(10)<<left<<mips[i].inst[2]<<setw(10)<<left<<mips[i].inst[3]<<setw(12)<<left<<mips[i].optype<<setw(6)<<left<<mips[i].stall<<setw(10)<<left<<mips[i].misBranch<<"\n";
       if (mips[i].optype == ARITHMETIC) total_stall_a += mips[i].stall;
       if (mips[i].optype == LOGICAL) total_stall_l += mips[i].stall;
       if (mips[i].optype == MEMORY) total_stall_m += mips[i].stall;
       if (mips[i].optype == BRANCH) total_stall_b += mips[i].stall;
       total_stall += mips[i].stall;
       total_misBranch += mips[i].misBranch;
   }

   outputfile<<"\nSUMMARY: "<<outfile<<"\n";
   outputfile<<"--------\n";
   outputfile<<"1) Total Cycles                           : "<<4 + cnt + total_stall<<"\n";
   outputfile<<"2) Total Instructions                     : "<<cnt<<"\n";
   outputfile<<"3) Total Branch Mispredictions            : "<<total_misBranch<<"\n";
   outputfile<<"4) Instructions effected due to following : "<<"\n";
   outputfile<<"   4.1) RAW Hazards                       : "<<counts[RCNT]<<"\n";
   outputfile<<"   4.2) Branches                          : "<<counts[BCNT]<<"\n";
   outputfile<<"   4.3) Branch Mispredictions             : "<<counts[BMCNT]<<"\n";
   outputfile<<"5) Total Stalls                           : "<<total_stall<<"\n";
   outputfile<<"   5.1) Stalls per Instruction Type       : "<<"\n";
   outputfile<<"        a) Arithmetic                     : "<<total_stall_a<<"\n";
   outputfile<<"        b) Logical                        : "<<total_stall_l<<"\n";
   outputfile<<"        c) Memory                         : "<<total_stall_m<<"\n";
   outputfile<<"        d) Branch                         : "<<total_stall_b<<"\n";
   outputfile<<"   5.2) Stalls per Hazard                 : "<<"\n";
   outputfile<<"        a) RAW                            : "<<counts[RAW]<<"\n";
   outputfile<<"        b) Branch                         : "<<counts[BRCH]<<"\n";
   outputfile<<"        c) Branch Mispredictions          : "<<counts[MISBRCH]<<"\n\n";

   cout<<"\nSUMMARY: "<<outfile<<"\n";
   cout<<"--------\n";
   cout<<"1) Total Cycles                           : "<<4 + cnt + total_stall<<"\n";
   cout<<"2) Total Instructions                     : "<<cnt<<"\n";
   cout<<"3) Total Branch Mispredictions            : "<<total_misBranch<<"\n";
   cout<<"4) Instructions effected due to following : "<<"\n";
   cout<<"   4.1) RAW Hazards                       : "<<counts[RCNT]<<"\n";
   cout<<"   4.2) Branches                          : "<<counts[BCNT]<<"\n";
   cout<<"   4.3) Branch Mispredictions             : "<<counts[BMCNT]<<"\n";
   cout<<"5) Total Stalls                           : "<<total_stall<<"\n";
   cout<<"   5.1) Stalls per Instruction Type       : "<<"\n";
   cout<<"        a) Arithmetic                     : "<<total_stall_a<<"\n";
   cout<<"        b) Logical                        : "<<total_stall_l<<"\n";
   cout<<"        c) Memory                         : "<<total_stall_m<<"\n";
   cout<<"        d) Branch                         : "<<total_stall_b<<"\n";
   cout<<"   5.2) Stalls per Hazard                 : "<<"\n";
   cout<<"        a) RAW                            : "<<counts[RAW]<<"\n";
   cout<<"        b) Branch                         : "<<counts[BRCH]<<"\n";
   cout<<"        c) Branch Mispredictions          : "<<counts[MISBRCH]<<"\n\n";
   outputfile.close();
}

/***********************************************************************************/
/*
Main Function
*/
/***********************************************************************************/
int main()
{
   MIPS NoForward[1000];
   MIPS WithForwardNoBranch[1000];
   MIPS WithForwardWithBranch[1000];
   int counts[6];
   int cnt = 0;

   //read input file.
   readInput(NoForward, cnt);
   //calculate stalls with no forwarding and no branch prediction.
   calculateStall(NoForward, cnt, counts, false, false);
   //write stalls with no forwarding and no branch prediction.
   writeOutput(NoForward, cnt, counts, "no_forward_no_bp");


   //read input file.
   readInput(WithForwardNoBranch, cnt);
   //calculate stalls with forwarding and no branch prediction.
   calculateStall(WithForwardNoBranch, cnt, counts, true, false);
   //write stalls with no forwarding and no branch prediction.
   writeOutput(WithForwardNoBranch, cnt, counts, "with_forward_no_bp");


   //read input file.
   readInput(WithForwardWithBranch, cnt);
   //calculate stalls with forwarding and with branch prediction.
   calculateStall(WithForwardWithBranch, cnt, counts, true, true);
   //write stalls with no forwarding and no branch prediction.
   writeOutput(WithForwardWithBranch, cnt, counts, "with_forward_with_bp");

   return 0;
}
