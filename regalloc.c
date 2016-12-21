#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "liveness.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"


struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here.
	struct RA_result ret;
	G_graph fg = FG_AssemFlowGraph(il, f);
	Temp_map initial = Temp_layerMap(Temp_empty(), F_tempMap);
	Temp_tempList regs = F_registers();
	struct COL_result col_result = COL_color(fg, initial, regs);
	ret.coloring = col_result.coloring;
	ret.il = il;

	return ret;
}
