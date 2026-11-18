#include "model.h"  
#include <algorithm>  
  
/* ---------- expand ---------- */  
  
std::vector<WI> expand(const std::vector<WD>& defs) {  
    std::vector<WI> result;  
    for (int i = 0; i < (int)defs.size(); i++) {  
        WI wi;  
        wi.nm      = defs[i].nm;  
        wi.pt      = defs[i].pt;  
        wi.x       = defs[i].x;  
        wi.y       = defs[i].y;  
        wi.baseRot = defs[i].baseRot;  
        wi.fl      = false;  
        wi.op      = defs[i].op;  
        wi.srcIdx  = i;  
        result.push_back(wi);  
        if (defs[i].mir) {  
            WI mir = wi;  
            mir.x       = -wi.x;  
            mir.fl      = true;  
            mir.baseRot = -wi.baseRot;  
            result.push_back(mir);  
        }  
    }  
    return result;  
}  
  
/* ---------- zorderIndices ---------- */  
  
/* static pointer to the array being sorted (avoids lambda, old-compiler safe) */  
static const std::vector<WD>* g_sortDefs = NULL;  
  
static bool zorderCmp(int a, int b) {  
    return (*g_sortDefs)[a].zorder < (*g_sortDefs)[b].zorder;  
}  
  
std::vector<int> zorderIndices(const std::vector<WD>& defs) {  
    std::vector<int> idx(defs.size());  
    for (int i = 0; i < (int)idx.size(); i++) idx[i] = i;  
    g_sortDefs = &defs;  
    std::stable_sort(idx.begin(), idx.end(), zorderCmp);  
    g_sortDefs = NULL;  
    return idx;  
}
