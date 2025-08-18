#ifndef __OBJDEPLOADER__
#define __OBJDEPLOADER__

#include "modding.h"
#include "global.h"

// Import the functions
RECOMP_IMPORT("ProxyMM_ObjDepLoader", bool ObjDepLoader_Load(PlayState* play, u8 segment, s16 objectId))
RECOMP_IMPORT("ProxyMM_ObjDepLoader", void ObjDepLoader_Unload(PlayState* play, u8 segment, s16 objectId))

#endif