/*
 * main.c
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "absyn.h"
#include "errormsg.h"
#include "temp.h" /* needed by translate.h */
#include "tree.h" /* needed by frame.h */
#include "assem.h"
#include "frame.h" /* needed by translate.h and printfrags prototype */
#include "translate.h"
#include "env.h"
#include "semant.h" /* function prototype for transProg */
#include "canon.h"
#include "prabsyn.h"
#include "printtree.h"
#include "escape.h" /* needed by escape analysis */
#include "parse.h"
#include "codegen.h"
#include "regalloc.h"
#include "string.h"

extern bool anyErrors;

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, F_frame frame, T_stm body)
{
 AS_proc proc;
 T_stmList stmList;
 AS_instrList iList;

 F_tempMap = Temp_empty();

 stmList = C_linearize(body);
 stmList = C_traceSchedule(C_basicBlocks(stmList));
 // printStmList(stdout, stmList);
 iList  = F_codegen(frame, stmList); /* 9 */

 struct RA_result ra = RA_regAlloc(frame, iList);  /* 10, 11 */

 AS_print(out, iList->head, NULL);

 fprintf(out, "pushl %%ebp\nmovl %%esp, %%ebp\nsubl $64, %%esp\n");
 fprintf(out, "pushl %%ebx\npushl %%esi\npushl %%edi\n");

 AS_printInstrList (out, iList->tail,
                       Temp_layerMap(F_tempMap,ra.coloring));

 fprintf(out, "popl %%edi\npopl %%esi\npopl %%ebx\n");
 fprintf(out, "leave\nret\n");

}

int main(int argc, string *argv)
{
 A_exp absyn_root;
 S_table base_env, base_tenv;
 F_fragList frags;
 char outfile[100];
 FILE *out = stdout;

 if (argc==2) {
   absyn_root = parse(argv[1]);
   if (!absyn_root)
     return 1;
     
#if 0
   pr_exp(out, absyn_root, 0); /* print absyn data structure */
   fprintf(out, "\n");
#endif
	//If you have implemented escape analysis, uncomment this
   Esc_findEscape(absyn_root); /* set varDec's escape field */

   frags = SEM_transProg(absyn_root);
   if (anyErrors) return 1; /* don't continue */

   /* convert the filename */
   sprintf(outfile, "%s.s", argv[1]);
   out = fopen(outfile, "w");
   /* Chapter 8, 9, 10, 11 & 12 */
   fprintf(out, ".text\n");
   fprintf(out, ".global tigermain\n");
   fprintf(out, ".type tigermain, @function\n");
   for (;frags;frags=frags->tail)
     if (frags->head->kind == F_procFrag) {
       doProc(out, frags->head->u.proc.frame, frags->head->u.proc.body);
       if (frags->tail && frags->tail->head->kind == F_stringFrag) {
          fprintf(out, ".section .rodata\n");
       }
     }
     else if (frags->head->kind == F_stringFrag) {
       fprintf(out, "%s:\n", Temp_labelstring(frags->head->u.stringg.label));
       int l = strlen(frags->head->u.stringg.str);
       char *a = (char*)&l;
       fprintf(out, ".string \"%c%c%c%c%s\"\n", a[0], a[1], a[2], a[3], 
               String_toPut(frags->head->u.stringg.str));     
     }


   fclose(out);
   return 0;
 }
 EM_error(0,"usage: tiger file.tig");
 return 1;
}
